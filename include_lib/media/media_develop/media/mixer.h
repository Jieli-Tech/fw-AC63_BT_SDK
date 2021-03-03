#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H

#include "generic/typedef.h"
#include "generic/list.h"
#include "media/audio_stream.h"
#include "application/audio_buf_sync.h"

#ifdef CONFIG_MIXER_CYCLIC

#include "mixer_cyclic.h"

#else /*CONFIG_MIXER_CYCLIC*/

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

struct audio_mixer {
    struct list_head head;	// 链表头
    OS_MUTEX mutex;		// 互斥
    s16 *output;		// 输出buf
    u16 points;			// 输出buf总点数
    u16 remain_points;	// 输出剩余点数
    u32 process_len;	// 输出了多少
    u8  channel_num;	// 声道数
    u8  change_sr_limit_ch_num;	// mixer活动通道小于该值时会重新检查采样率
    u8  sample_sync : 1;// sample同步标记
    volatile u8 active;	// 活动标记。1-正在运行
    u16 lose_to_id; 	// 超时激活ID
    void *remain_ch;	// 直接输出没有输出完成的通道
    u32 sample_rate;	// 当前mixer的采样率
    int need_sr;		// mixer想要变成的采样率
    MIXER_SR_TYPE sr_type;	// 采样率设置类型
    void (*evt_handler)(struct audio_mixer *, int);	// 事件返回接口
    u32(*check_sr)(struct audio_mixer *, u32 sr);	// 检查采样率

    struct audio_stream *stream;		// 音频流
    struct audio_stream_entry entry;	// 音频流入口，后级接dac等
    // struct audio_stream_group group;	// 用于链接前级
};

struct audio_mixer_ch {
    u32 start : 1;		// 启动标记。1-已经启动。输出时标记一次
    u32 open : 1;		// 打开标记。1-已经打开。输出标记为1，reset标记为0
    u32 pause : 1;		// 暂停标记。1-暂停
    u32 no_wait : 1;	// 不等待有数
    u32 lose : 1;		// 丢数标记
    u32 src_en : 1;		// 变采样使能
    u32 src_always : 1;	// 不管采样率是否相同都做变采样
    u32 sample_sync : 1;// sample同步标记
    u32 data_sync : 1;	// sample同步临时保存
    u32 sync_en : 1;	// buf同步使能
    u32 sync_always : 1;// 不管采样率是否相同都做buf同步
    u32 check_sr : 1;	// 需要检查采样率
    u32 src_running : 1;// 执行变采样处理
    u32 wait_resume : 1;
    u32 follow_resample : 1;
    u32 sample_rate;	// 当前通道采样率
    u16 offset;			// 当前通道在输出buf中的偏移位置
    u16 lose_time;		// 超过该时间还没有数据，则以为可以丢数。no_wait置1有效
    unsigned long lose_limit_time;	// 丢数超时中间运算变量
    struct list_head list_entry;	// 链表
    struct audio_mixer *mixer;	// mixer句柄
    struct audio_src_handle *src;	// 变采样
    struct audio_buf_sync_hdl sync;	// 同步
    struct audio_mixer_ch_sync_info sync_info;	// 同步参数
    void *priv;		// 事件回调私有句柄
    void (*event_handler)(void *priv, int event, int param);	// 事件回调接口
    void *follow_priv;
    int (*follow_sample_rate)(void *priv);

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

// 获取非暂停通道总数
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

// 设置通道没数据时不等待（超时直接丢数）
void audio_mixer_ch_set_no_wait(struct audio_mixer_ch *ch, u8 no_wait, u16 time_ms);

// 通道暂停
void audio_mixer_ch_pause(struct audio_mixer_ch *ch, u8 pause);

// 通道重启
int audio_mixer_ch_reset(struct audio_mixer_ch *ch);

// 获取通道剩余长度
int audio_mixer_ch_data_len(struct audio_mixer_ch *ch);

// 通道数据输出
int audio_mixer_ch_write(struct audio_mixer_ch *ch, s16 *data, int len);

void audio_mixer_ch_sample_sync_enable(struct audio_mixer_ch *ch, u8 enable);

void audio_mixer_ch_follow_resample_enable(struct audio_mixer_ch *ch, void *priv, int (*follow_sample_rate)(void *));
#endif /*CONFIG_MIXER_CYCLIC*/

#endif

