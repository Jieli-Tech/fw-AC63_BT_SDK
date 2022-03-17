#ifndef AUDIO_MIXER_CYCLIC_H
#define AUDIO_MIXER_CYCLIC_H

#include "generic/typedef.h"
#include "generic/list.h"
#include "media/audio_stream.h"
#include "application/audio_buf_sync.h"

#ifdef CONFIG_MIXER_CYCLIC

#define MIXER_AUDIO_CASK_EFFECT_EN	1 //短板效应
#define MIXER_AUDIO_CHANNEL_NUM		4 //最大声道数
#define MIXER_AUDIO_ENDDING_DCC     0
#define MIXER_AUDIO_DIRECT_OUT		1 //直通输出

// mixer事件回调
enum {
    MIXER_EVENT_OPEN,		// 打开一个通道
    MIXER_EVENT_CLOSE,		// 关闭一个通道
    MIXER_EVENT_SR_CHANGE,	// mixer采样率变化
};

// mixer 通道事件回调
enum {
    MIXER_EVENT_CH_OPEN,	// 通道打开
    MIXER_EVENT_CH_CLOSE,	// 通道关闭
    MIXER_EVENT_CH_RESET,	// 通道重新开始
    MIXER_EVENT_CH_LOSE,	// 通道丢数
    MIXER_EVENT_CH_OUT_SR_CHANGE,	// mixer采样率变化
};

// mixer采样率确定方式
typedef enum {
    MIXER_SR_MAX = 0,	// 链表中的最大采样率
    MIXER_SR_MIN,		// 链表中的最小采样率
    MIXER_SR_FIRST,		// 链表中的第一个通道的采样率
    MIXER_SR_LAST,		// 链表中的最后一个通道的采样率
    MIXER_SR_SPEC,		// set指定采样率；get当前mixer采样率
} MIXER_SR_TYPE;

/*
 * mixer通道跟随缓存数据大小来变化采样率处理的参数
 * 该方法是通过判断缓存数据是否超出规定范围来逐步调节采样率，由于变采样需要一定的时间内
 * 才能把数据量调整到规定之内，此时的变采样可能是超过实际采样率的，所有后续会逐步往回调。
 * 因此可能会出现采样飘来飘去的情况，一种优化处理就是加大buf大小，减小调整步伐等来达到
 * 缓慢变化的效果。
 */
struct audio_mixer_ch_sync_info {
    int begin_per;		// 起始百分比，默认60
    int top_per;		// 最大百分比，默认80
    int bottom_per;		// 最小百分比，默认30
    u8 inc_step;		// 每次调整增加步伐，默认5
    u8 dec_step;		// 每次调整减少步伐，默认5
    u16 max_step;		// 最大调整步伐，默认80
    void *priv;			// get_total,get_size私有句柄
    int (*get_total)(void *priv);	// 获取buf总长
    int (*get_size)(void *priv);	// 获取buf数据长度
};

struct audio_mixer;

/*
 * mixer单独输出任务结构体
 * mixer各通道的数据合并后仅写入buf中，不马上推给后续的数据流，由该任务单独推送
 * config_mixer_task使能时有效
 */
struct audio_mixer_task {
    struct list_head head;	// 链表头
    OS_SEM sem;
    volatile u8	busy;
    volatile u8	start;
    u8 ext_resume;	// 由上层应用激活。默认1
    const char *task_name;
    void *check_priv;
    int (*check_out_len)(void *priv, struct audio_mixer *mixer, int len); // 检查输出长度
};

/*
 * mixer处理
 * 主要用于把各个通道的数据合并成指定采样率、指定声道的数据，然后推送到后续的数据流中
 * 支持自动变采样、自动变换声道、超时自动忽略该通道等
 * 支持长板效应和短板效应：短板效应-等待所有通道有数才能输出;
 *                         长板效应-只要某个通道有数就输出，不理会其他通道是否有数
 * 支持直通处理：当只有一个有效通道输出且不需要变换声道时，不经过mixer的buf，直接输出到后续的数据流
 */
struct audio_mixer {
    struct list_head head;	// 链表头
    OS_MUTEX mutex;		// 互斥
    struct audio_mixer_task *task;	// mixer处理任务
    struct list_head list_entry;	// task链表

    s16 *output;		// 输出buf
    u16 points;			// 输出buf总点数
    u16 remain_points;	// 输出剩余点数
    u16 read_points;	// 读地址
    volatile u16 points_len;		// 数据长度
    u16 min_points;		// 最小输出点数，默认为总buf的1/4
    u16 no_sync_points;	// 非sync数据剩余点数
    u32 process_len;	// 输出了多少
    volatile u8 output_state;	// 输出状态
    u32 aud_ch_mask;	// 声道数掩码
    u32 change_sr_limit_ch_num : 4;	// mixer活动通道小于该值时会重新检查采样率
    u32 aud_ch_num : 4;	// 声道数
    u32 sample_sync : 1;// sample同步标记
    u32 ending_ch_dcc : 1;
#if MIXER_AUDIO_CASK_EFFECT_EN
    u32 cask_effect : 1;// 短板效应。默认1，等待所有通道有数才能输出。关闭该功能，只要某个通道有数就输出
    u16 lose_to_id; 	// 超时激活ID
    u16 lose_to_once_ms;// 每次超时计数的时间
#endif
#if MIXER_AUDIO_DIRECT_OUT
    void *remain_ch;	// 直接输出没有输出完成的通道
    u8  direct_out;		// 直通使能。默认1
    u8  cur_is_direct_out;	// 当前为直通输出
#endif
#if MIXER_AUDIO_ENDDING_DCC
    u16 dcc_mixed_samples;
    u32 dcc_sum[MIXER_AUDIO_CHANNEL_NUM];
    s16 end_sample[MIXER_AUDIO_CHANNEL_NUM];
#endif
    u32 sample_rate;	// 当前mixer的采样率
    int need_sr;		// mixer想要变成的采样率
    MIXER_SR_TYPE sr_type;	// 采样率设置类型
    u32(*check_sr)(struct audio_mixer *, u32 sr);	// 检查采样率
    void (*evt_handler)(struct audio_mixer *, int);	// 事件返回接口

    struct audio_stream *stream;		// 音频流
    struct audio_stream_entry entry;	// 音频流入口，后级接dac等
};

/*
 * mixer通道处理
 * 支持固定变采样src_en、跟随缓存数据大小变采样sync_en、跟随外部设定采样follow_resample
 * 支持各个声道单独输出配置aud_ch_out
 * 支持超时自动忽略该通道no_wait
 * 支持淡入淡出等
 */
struct audio_mixer_ch {
    u32 start : 1;		// 启动标记。1-已经启动。输出时标记一次
    u32 open : 1;		// 打开标记。1-已经打开。输出标记为1，reset标记为0
    u32 pause : 1;		// 暂停标记。1-暂停
    u32 src_en : 1;		// 变采样使能
    u32 src_always : 1;	// 不管采样率是否相同都做变采样
    u32 sync_en : 1;	// buf同步使能
    u32 sync_always : 1;// 不管采样率是否相同都做buf同步
    u32 sample_sync : 1;// sample同步标记
    u32 data_sync : 1;	// sample同步临时保存
    u32 check_sr : 1;	// 需要检查采样率
    u32 master_sr_change : 1;	// master采样率变化
    u32 src_running : 1;// 执行变采样处理
    u32 aud_ch_num : 4;	// 声道数
    u32 fade_dir : 2;	// 淡入淡出状态
    u32 follow_resample : 1;	// follow变采样标志
#if MIXER_AUDIO_CASK_EFFECT_EN
    u32 no_wait : 1;	// 不等待有数
    u16 lose_time;		// 超过该时间还没有数据，则以为可以丢数。no_wait置1有效
    u16 lose_cnt;		// 超时计数
    volatile u8 lose;	// 丢数标记
#endif
    volatile u8 need_resume;	// 需要激活
    s16 fade_vol;		// 淡入淡出值
    s16 fade_step;		// 淡入淡出步进
    u16 fade_time;		// 淡入淡出时长
    OS_SEM *sem_fadeout;// 淡出等待信号量
    // 声道输出控制。如，当前通道的第1个声道输出到mixer的第0和第1个声道，aud_ch_out[1] = BIT(0) | BIT(1);
    // mixer单声道默认值：aud_ch_out[n] = BIT(0); 如，aud_ch_out[1] = BIT(0);
    // mixer其他声道默认值：aud_ch_out[n] = BIT(n)|BIT(n+2); 如，aud_ch_out[1] = BIT(1)|BIT(3);
    u8  aud_ch_out[MIXER_AUDIO_CHANNEL_NUM];
#if MIXER_AUDIO_ENDDING_DCC
    s16 end_sample[MIXER_AUDIO_CHANNEL_NUM];
#endif
    u16 points_len;		// 数据长度
    u16 start_points;	// 初始化数据长度。默认为0
    u32 sample_rate;	// 当前通道采样率
    u32 sample_rate_follow;	// 当前通道follow采样率。follow_resample==1有效
    u32 slience_samples;
    u32 slience_offset;
    struct list_head list_entry;	// 链表
    struct audio_mixer *mixer;	// mixer句柄
    struct audio_src_handle *src;	// 变采样
    struct audio_buf_sync_hdl sync;	// buf同步
    struct audio_mixer_ch_sync_info sync_info;	// buf同步参数
    void *priv;		// 事件回调私有句柄
    void (*event_handler)(void *priv, int event, int param);	// 事件回调接口
    void *follow_priv;	// follow变采样私有参数
    int (*follow_sample_rate)(void *priv);	// follow变采样回调

    struct audio_stream_entry entry;	// 音频流入口，通道后面不应该再接其他的音频流，最后由mixer合并后输出
};


/*
*********************************************************************
*                  Audio Mixer Open
* Description: 打开一个mixer
* Arguments  : *mixer	句柄
* Return	 : 0		成功
* Note(s)    : 默认采样率类型为获取第一个通道的采样率；默认设置声道数1；
*              默认使用短板效应；默认开启直通功能
*********************************************************************
*/
int audio_mixer_open(struct audio_mixer *mixer);

/*
*********************************************************************
*                  Audio Mixer Close
* Description: 关闭一个mixer
* Arguments  : *mixer	句柄
* Return	 : None.
* Note(s)    : 所有通道关闭后才能close
*********************************************************************
*/
void audio_mixer_close(struct audio_mixer *mixer);

/*
*********************************************************************
*                  Audio Mixer Set Event Callback Handler
* Description: 设置事件回调
* Arguments  : *mixer	句柄
*              *handler	事件回调句柄
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_mixer_set_event_handler(struct audio_mixer *mixer,
                                   void (*handler)(struct audio_mixer *, int));

/*
*********************************************************************
*                  Audio Mixer Set Check Sample Callback Handler
* Description: 设置采样率检测回调
* Arguments  : *mixer		句柄
*              *check_sr	采样率检测回调句柄
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_mixer_set_check_sr_handler(struct audio_mixer *mixer,
                                      u32(*check_sr)(struct audio_mixer *, u32));

/*
*********************************************************************
*                  Audio Mixer Set Output Buf
* Description: 设置输出buf
* Arguments  : *mixer	句柄
*              *buf		输出buf
*              len		输出buf长度
* Return	 : None.
* Note(s)    : 默认设置min_points为总buf的1/4
*********************************************************************
*/
void audio_mixer_set_output_buf(struct audio_mixer *mixer, s16 *buf, u16 len);

/*
*********************************************************************
*                  Audio Mixer Set Channel Number
* Description: 设置声道数
* Arguments  : *mixer		句柄
*              channel_num	声道数
* Return	 : None.
* Note(s)    : 通道可能会做变采样等，需要设置声道数
*********************************************************************
*/
void audio_mixer_set_channel_num(struct audio_mixer *mixer, u8 channel_num);

/*
*********************************************************************
*                  Audio Mixer Set Output Min Lenght
* Description: 设置mixer数据输出最小限制值
* Arguments  : *mixer	句柄
*              min_len	最小限制值
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_mixer_set_min_len(struct audio_mixer *mixer, u16 min_len);

/*
*********************************************************************
*                  Audio Mixer Set Cask Effect
* Description: 短板效应
* Arguments  : *mixer		句柄
*              cask_effect	短板效应
* Return	 : None.
* Note(s)    : 1 短板效应。等待所有通道有数才能输出
*              0 长板效应。只要某个通道有数就输出，不理会其他通道是否有数
*********************************************************************
*/
void audio_mixer_set_cask_effect(struct audio_mixer *mixer, u8 cask_effect);

/*
*********************************************************************
*                  Audio Mixer Set Direct Output Enable
* Description: 直通功能
* Arguments  : *mixer			句柄
*              direct_out_en	直通输出使能
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_mixer_set_direct_out(struct audio_mixer *mixer, u8 direct_out_en);

/*
*********************************************************************
*                  Audio Mixer Set Sample Rate
* Description: 设置采样率
* Arguments  : *mixer		句柄
*              type			采样率类型
*              sample_rate	采样率
* Return	 : None.
* Note(s)    : 采样率类型为MIXER_SR_SPEC时固定采样，其他类型在设置通道采样率的时候重新获取
*********************************************************************
*/
void audio_mixer_set_sample_rate(struct audio_mixer *mixer, MIXER_SR_TYPE type, u32 sample_rate);

/*
*********************************************************************
*                  Audio Mixer Get Original Sample Rate By Type
* Description: 按条件获取采样率
* Arguments  : *mixer		句柄
*              type			采样率类型
* Return	 : 采样率
* Note(s)    : 原始采样率，不调用check_sr检测
*********************************************************************
*/
int audio_mixer_get_original_sample_rate_by_type(struct audio_mixer *mixer, MIXER_SR_TYPE type);

/*
*********************************************************************
*                  Audio Mixer Get Sample Rate By Type
* Description: 按条件获取采样率
* Arguments  : *mixer		句柄
*              type			采样率类型
* Return	 : 采样率
* Note(s)    : 获取原始采样率之后，再调用check_sr()做了检测
*********************************************************************
*/
int audio_mixer_get_sample_rate_by_type(struct audio_mixer *mixer, MIXER_SR_TYPE type);

/*
*********************************************************************
*                  Audio Mixer Get Sample Rate
* Description: 获取采样率
* Arguments  : *mixer		句柄
* Return	 : 采样率
* Note(s)    : 内部直接调用audio_mixer_get_sample_rate_by_type(mixer, mixer->sr_type);
*********************************************************************
*/
int audio_mixer_get_sample_rate(struct audio_mixer *mixer);

/*
*********************************************************************
*                  Audio Mixer Get Cur Sample Rate
* Description: 直接获取mixer->sample_rate的值
* Arguments  : *mixer		句柄
* Return	 : 采样率
* Note(s)    : None.
*********************************************************************
*/
int audio_mixer_get_cur_sample_rate(struct audio_mixer *mixer);

/*
*********************************************************************
*                  Audio Mixer Get Channel Number
* Description: 获取通道总数
* Arguments  : *mixer		句柄
* Return	 : 通道数
* Note(s)    : None.
*********************************************************************
*/
int audio_mixer_get_ch_num(struct audio_mixer *mixer);

/*
*********************************************************************
*                  Audio Mixer Get Active Channel Number
* Description: 获取活动通道总数
* Arguments  : *mixer		句柄
* Return	 : 通道数
* Note(s)    : None.
*********************************************************************
*/
int audio_mixer_get_active_ch_num(struct audio_mixer *mixer);

/*
*********************************************************************
*                  Audio Mixer Get Data Lenght
* Description: 获取mixer剩余长度
* Arguments  : *mixer		句柄
* Return	 : 剩余数据长度
* Note(s)    : None.
*********************************************************************
*/
int audio_mixer_data_len(struct audio_mixer *mixer);

/*
*********************************************************************
*                  Audio Mixer Push Stop To Stream
* Description: 传送stop到数据流
* Arguments  : *mixer		句柄
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_mixer_output_stop(struct audio_mixer *mixer);

/*
*********************************************************************
*                  Audio Mixer Stream Resume
* Description: 数据流激活处理
* Arguments  : *priv	(struct audio_mixer *)类型句柄
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_mixer_stream_resume(void *p);


/*
*********************************************************************
*                  Audio Mixer Channel Open
* Description: 打开一个mixer通道
* Arguments  : *ch			mixer通道句柄
*              *mixer		mixer句柄
* Return	 : 0			成功
* Note(s)    : 放在链表结尾处
*********************************************************************
*/
int audio_mixer_ch_open(struct audio_mixer_ch *ch, struct audio_mixer *mixer);

/*
*********************************************************************
*                  Audio Mixer Channel Open Head
* Description: 打开一个mixer通道
* Arguments  : *ch			mixer通道句柄
*              *mixer		mixer句柄
* Return	 : 0			成功
* Note(s)    : 放在链表起始处
*********************************************************************
*/
int audio_mixer_ch_open_head(struct audio_mixer_ch *ch, struct audio_mixer *mixer);

/*
*********************************************************************
*                  Audio Mixer Channel Close
* Description: 关闭通道
* Arguments  : *ch			mixer通道句柄
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_mixer_ch_close(struct audio_mixer_ch *ch);

/*
*********************************************************************
*                  Audio Mixer Channel Set Event Callback Handler
* Description: 设置通道事件回调
* Arguments  : *ch			mixer通道句柄
*              *priv		事件回调私有句柄
*              *handler		事件回调接口
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_mixer_ch_set_event_handler(struct audio_mixer_ch *ch, void *priv, void (*handler)(void *, int, int));

/*
*********************************************************************
*                  Audio Mixer Channel Set Sample Rate
* Description: 设置通道采样率
* Arguments  : *ch			mixer通道句柄
*              sample_rate	采样率
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_mixer_ch_set_sample_rate(struct audio_mixer_ch *ch, u32 sample_rate);

/*
*********************************************************************
*                  Audio Mixer Channel Set SRC Enable
* Description: 设置通道变采样
* Arguments  : *ch			mixer通道句柄
*              src_en		变采样使能
*              always		不管输入输出采样是否相同，都做变采样处理
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_mixer_ch_set_src(struct audio_mixer_ch *ch, u8 src_en, u8 always);

/*
*********************************************************************
*                  Audio Mixer Channel Set Buf Sync Enable
* Description: 设置通道跟随缓存大小变采样处理
* Arguments  : *ch			mixer通道句柄
*              *info		同步参数
*              sync_en		同步使能
*              always		不管输入输出采样是否相同，都做同步处理
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_mixer_ch_set_sync(struct audio_mixer_ch *ch, struct audio_mixer_ch_sync_info *info, u8 sync_en, u8 always);

/*
*********************************************************************
*                  Audio Mixer Channel Set Audio Output Channel
* Description: 设置通道声道输出
* Arguments  : *ch			mixer通道句柄
*              aud_ch		设置通道的第几个声道，小于MIXER_AUDIO_CHANNEL_NUM
*              aud_out		设置输出到哪些声道，如输出到第0个和第3个，aud_out=BIT(0)|BIT(3)
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_mixer_ch_set_aud_ch_out(struct audio_mixer_ch *ch, u8 aud_ch, u8 aud_out);

/*
*********************************************************************
*                  Audio Mixer Channel Set No Wait
* Description: 设置通道没数据时不等待
* Arguments  : *ch			mixer通道句柄
*              no_wait		不等待使能
*              time_ms		超时
* Return	 : None.
* Note(s)    : 超时直接丢数
*********************************************************************
*/
void audio_mixer_ch_set_no_wait(struct audio_mixer_ch *ch, u8 no_wait, u16 time_ms);

/*
*********************************************************************
*                  Audio Mixer Channel Set Start Len
* Description: 设置通道起始填数长度
* Arguments  : *ch			mixer通道句柄
*              start_len	起始长度
* Return	 : None.
* Note(s)    : 设置长度不超过mixer空间大小
*********************************************************************
*/
void audio_mixer_ch_set_start_len(struct audio_mixer_ch *ch, u16 start_len);

/*
*********************************************************************
*                  Audio Mixer Channel Pause
* Description: 通道暂停
* Arguments  : *ch			mixer通道句柄
*              pause		暂停
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_mixer_ch_pause(struct audio_mixer_ch *ch, u8 pause);

/*
*********************************************************************
*                  Audio Mixer Channel Reset
* Description: 通道重启
* Arguments  : *ch			mixer通道句柄
* Return	 : 0			成功
* Note(s)    : None.
*********************************************************************
*/
int audio_mixer_ch_reset(struct audio_mixer_ch *ch);

/*
*********************************************************************
*                  Audio Mixer Channel Get Data Lenght
* Description: 获取通道剩余长度
* Arguments  : *ch			mixer通道句柄
* Return	 : 通道剩余数据长度
* Note(s)    : None.
*********************************************************************
*/
int audio_mixer_ch_data_len(struct audio_mixer_ch *ch);

/*
*********************************************************************
*                  Audio Mixer Channel Write Data
* Description: 通道数据输出
* Arguments  : *ch			mixer通道句柄
*              *data		数据
*              len			数据长度
* Return	 : 实际输出长度
* Note(s)    : 输出前会根据配置做变采样/同步处理
*********************************************************************
*/
int audio_mixer_ch_write(struct audio_mixer_ch *ch, s16 *data, int len);

/*
*********************************************************************
*                  Audio Mixer Channel Sample Sync Enable
* Description: 设置通道sync标志
* Arguments  : *ch			mixer通道句柄
*              enable		使能
* Return	 : None.
* Note(s)    : 蓝牙tws同步使用
*********************************************************************
*/
void audio_mixer_ch_sample_sync_enable(struct audio_mixer_ch *ch, u8 enable);

/*
*********************************************************************
*                  Audio Mixer Channel Try Fadein
* Description: 通道淡入
* Arguments  : *ch			mixer通道句柄
*              time			淡入时长
* Return	 : 0			成功
* Note(s)    : 暂停、直通等情况下无效
*********************************************************************
*/
int audio_mixer_ch_try_fadein(struct audio_mixer_ch *ch, u16 time);

/*
*********************************************************************
*                  Audio Mixer Channel Try Fadeout
* Description: 通道淡出
* Arguments  : *ch			mixer通道句柄
*              time			淡出时长
* Return	 : 0			成功
* Note(s)    : 暂停、直通等情况下无效。淡出后通道会暂停。
*********************************************************************
*/
int audio_mixer_ch_try_fadeout(struct audio_mixer_ch *ch, u16 time);

/*
*********************************************************************
*                  Audio Mixer Channel Fade Close
* Description: 关闭通道淡入淡出
* Arguments  : *ch			mixer通道句柄
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_mixer_ch_fade_close(struct audio_mixer_ch *ch);

/*
*********************************************************************
*                  Audio Mixer Channel Follow Resample Enable
* Description: 设置通道follow变采样
* Arguments  : *ch			mixer通道句柄
*              *priv		变采样参数
*              *follow_sample_rate	变采样回调
* Return	 : None.
* Note(s)    : 采样率变化由回调确定
*********************************************************************
*/
void audio_mixer_ch_follow_resample_enable(struct audio_mixer_ch *ch, void *priv, int (*follow_sample_rate)(void *));

/*
*********************************************************************
*                  Audio Mixer Output Task Init
* Description: mixer任务初始化
* Arguments  : *mtask		mixer任务句柄
*              *task_name	任务名字
* Return	 : true			成功
* Note(s)    : None.
*********************************************************************
*/
int audio_mixer_task_init(struct audio_mixer_task *mtask, const char *task_name);

/*
*********************************************************************
*                  Audio Mixer Output Task Destroy
* Description: mixer任务删除
* Arguments  : *mtask		mixer任务句柄
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_mixer_task_destroy(struct audio_mixer_task *mtask);

/*
*********************************************************************
*                  Audio Mixer Output Task Channel Open
* Description: mixer添加到mixer任务中
* Arguments  : *mixer		mixer句柄
*              *mtask		mixer任务句柄
* Return	 : true			成功
* Note(s)    : None.
*********************************************************************
*/
int audio_mixer_task_ch_open(struct audio_mixer *mixer, struct audio_mixer_task *mtask);

/*
*********************************************************************
*                  Audio Mixer Output Task Channel Close
* Description: 把mixer从mixer任务中删除
* Arguments  : *mixer		mixer句柄
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_mixer_task_ch_close(struct audio_mixer *mixer);

/*
*********************************************************************
*                  Mixer Cyclic Task Resume
* Description: mixer任务激活
* Arguments  : *mtask		mixer任务句柄
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_mixer_task_resume(struct audio_mixer_task *mtask);

/*
*********************************************************************
*                  Mixer Cyclic Task Set Check Out Lenght Callback
* Description: mixer检查输出长度回调函数
* Arguments  : *mtask		mixer任务句柄
*			   *check_priv	回调函数私有参数
*			   *check_out_len	输出长度检查回调。返回要输出的长度（不超过len）
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_mixer_task_set_check_out_len(struct audio_mixer_task *mtask,
                                        void *check_priv,
                                        int (*check_out_len)(void *priv, struct audio_mixer *mixer, int len));

#endif /*CONFIG_MIXER_CYCLIC*/

#endif /*AUDIO_MIXER_CYCLIC_H*/

