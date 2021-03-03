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

typedef enum {
    MIXER_SR_MAX = 0,	// 最大采样率
    MIXER_SR_MIN,		// 最小采样率
    MIXER_SR_FIRST,		// 第一个通道的采样率
    MIXER_SR_LAST,		// 最后一个通道的采样率
    MIXER_SR_SPEC,		// set指定采样率；get当前mixer采样率
} MIXER_SR_TYPE;

struct audio_mixer_ch_sync_info {
    int begin_per;		// 起始百分比
    int top_per;		// 最大百分比
    int bottom_per;		// 最小百分比
    u8 inc_step;		// 每次调整增加步伐
    u8 dec_step;		// 每次调整减少步伐
    u16 max_step;		// 最大调整步伐
    void *priv;			// get_total,get_size私有句柄
    int (*get_total)(void *priv);	// 获取buf总长
    int (*get_size)(void *priv);	// 获取buf数据长度
};

struct audio_mixer;

struct audio_mixer_task {
    struct list_head head;	// 链表头
    OS_SEM sem;
    volatile u8	busy;
    volatile u8	start;
    const char *task_name;
};

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
    u16 start_points;	// 初始化数据长度。默认为mixer->points/2
    u32 sample_rate;	// 当前通道采样率
    u32 sample_rate_follow;	// 当前通道follow采样率。follow_resample==1有效
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


// 打开一个mixer
int audio_mixer_open(struct audio_mixer *mixer);

// 关闭mixer
void audio_mixer_close(struct audio_mixer *mixer);

// 设置事件回调
void audio_mixer_set_event_handler(struct audio_mixer *mixer,
                                   void (*handler)(struct audio_mixer *, int));

// 设置采样率检测回调
void audio_mixer_set_check_sr_handler(struct audio_mixer *mixer,
                                      u32(*check_sr)(struct audio_mixer *, u32));

// 设置输出buf
void audio_mixer_set_output_buf(struct audio_mixer *mixer, s16 *buf, u16 len);

// 设置声道数
void audio_mixer_set_channel_num(struct audio_mixer *mixer, u8 channel_num);

// 设置mixer数据输出最小限制值
void audio_mixer_set_min_len(struct audio_mixer *mixer, u16 min_len);

// 设置短板效应
void audio_mixer_set_cask_effect(struct audio_mixer *mixer, u8 cask_effect);

// 直通功能
void audio_mixer_set_direct_out(struct audio_mixer *mixer, u8 direct_out_en);

// 设置采样率类型
void audio_mixer_set_sample_rate(struct audio_mixer *mixer, MIXER_SR_TYPE type, u32 sample_rate);
// 按条件获取采样率（原始采样率，不调用check_sr检测）
int audio_mixer_get_original_sample_rate_by_type(struct audio_mixer *mixer, MIXER_SR_TYPE type);
// 按条件获取采样率（有调用check_sr检测）
int audio_mixer_get_sample_rate_by_type(struct audio_mixer *mixer, MIXER_SR_TYPE type);
// 获取采样率（按mixer->sr_type获取）
int audio_mixer_get_sample_rate(struct audio_mixer *mixer);
// 获取mixer当前正在用的采样率
int audio_mixer_get_cur_sample_rate(struct audio_mixer *mixer);

// 获取通道总数
int audio_mixer_get_ch_num(struct audio_mixer *mixer);

// 获取活动通道总数
int audio_mixer_get_active_ch_num(struct audio_mixer *mixer);

// 获取mixer剩余长度
int audio_mixer_data_len(struct audio_mixer *mixer);

// mixer传送stop到数据流
void audio_mixer_output_stop(struct audio_mixer *mixer);

// mixer数据流激活处理
void audio_mixer_stream_resume(void *p);


// 打开一个mixer通道（放在链表结尾处）
int audio_mixer_ch_open(struct audio_mixer_ch *ch, struct audio_mixer *mixer);
// 打开一个mixer通道（放在链表起始处）
int audio_mixer_ch_open_head(struct audio_mixer_ch *ch, struct audio_mixer *mixer);

// 关闭通道
void audio_mixer_ch_close(struct audio_mixer_ch *ch);

// 设置通道事件回调
void audio_mixer_ch_set_event_handler(struct audio_mixer_ch *ch, void *priv, void (*handler)(void *, int, int));

// 设置通道采样率/变采样
void audio_mixer_ch_set_sample_rate(struct audio_mixer_ch *ch, u32 sample_rate);

// 设置通道变采样
void audio_mixer_ch_set_src(struct audio_mixer_ch *ch, u8 src_en, u8 always);

// 设置通道同步
void audio_mixer_ch_set_sync(struct audio_mixer_ch *ch, struct audio_mixer_ch_sync_info *info, u8 sync_en, u8 always);

// 设置通道声道输出
void audio_mixer_ch_set_aud_ch_out(struct audio_mixer_ch *ch, u8 aud_ch, u8 aud_out);

// 设置通道没数据时不等待（超时直接丢数）
void audio_mixer_ch_set_no_wait(struct audio_mixer_ch *ch, u8 no_wait, u16 time_ms);

// 设置通道起始填数长度
void audio_mixer_ch_set_start_len(struct audio_mixer_ch *ch, u16 start_len);

// 通道暂停
void audio_mixer_ch_pause(struct audio_mixer_ch *ch, u8 pause);

// 通道重启
int audio_mixer_ch_reset(struct audio_mixer_ch *ch);

// 获取通道剩余长度
int audio_mixer_ch_data_len(struct audio_mixer_ch *ch);

// 通道数据输出
int audio_mixer_ch_write(struct audio_mixer_ch *ch, s16 *data, int len);

// 设置sync标志
void audio_mixer_ch_sample_sync_enable(struct audio_mixer_ch *ch, u8 enable);

// 淡入（暂停、直通等情况下无效）
int audio_mixer_ch_try_fadein(struct audio_mixer_ch *ch, u16 time);

// 淡出（暂停、直通等情况下无效）。淡出后通道会暂停。
int audio_mixer_ch_try_fadeout(struct audio_mixer_ch *ch, u16 time);

// 关闭通道淡入淡出
void audio_mixer_ch_fade_close(struct audio_mixer_ch *ch);

// 设置通道follow变采样
void audio_mixer_ch_follow_resample_enable(struct audio_mixer_ch *ch, void *priv, int (*follow_sample_rate)(void *));

// mixer任务初始化
int audio_mixer_task_init(struct audio_mixer_task *mtask, const char *task_name);
// mixer任务删除
void audio_mixer_task_destroy(struct audio_mixer_task *mtask);

// mixer添加到mixer任务中
int audio_mixer_task_ch_open(struct audio_mixer *mixer, struct audio_mixer_task *mtask);
// 把mixer从mixer任务中删除
void audio_mixer_task_ch_close(struct audio_mixer *mixer);


#endif /*CONFIG_MIXER_CYCLIC*/

#endif /*AUDIO_MIXER_CYCLIC_H*/

