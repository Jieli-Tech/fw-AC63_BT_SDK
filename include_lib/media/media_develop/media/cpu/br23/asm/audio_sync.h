/*****************************************************************
>file name : audio_sync.h
>author : lichao
>create time : Fri 29 May 2020 11:11:23 AM CST
*****************************************************************/
#ifndef _ASM_AUDIO_SYNC_H_
#define _ASM_AUDIO_SYNC_H_
#include "media/audio_stream.h"

#define AUDIO_SYNC_OUTPUT_IDLE                  0
#define AUDIO_SYNC_OUTPUT_PROBE                 1 //输出前处理
#define AUDIO_SYNC_OUTPUT_START                 2 //输出开始
#define AUDIO_SYNC_OUTPUT_MISS_DATA             3 //输出采样丢失样点
#define AUDIO_SYNC_OUTPUT_ALIGN_COMPLETE        4 //输出对齐完成

#define AUDIO_SYNC_DAC                          0
#define AUDIO_SYNC_IIS                          1

#define PCM_PHASE_BIT                           8

#define AUDIO_STANDARD_STREAM                   1
#define AUDIO_OUTPUT_STREAM                     2


#define SAMP_STREAM_FRAME_OUTPUT                0
#define SAMP_STREAM_CBUF_OUTPUT                 1

#define SAMPLE_SYNC_DEVICE_IDLE                 0
#define SAMPLE_SYNC_DEVICE_START                1

#define BT_AHB_BASE_ADR             0x1c0000L

#define SAMPLECLK                   (*(volatile unsigned long *)(BT_AHB_BASE_ADR + 0x7 *4))
#define SLOTCLK                     (*(volatile unsigned long *)(BT_AHB_BASE_ADR + 0x8 *4))
#define FINETIMECNT                 (*(volatile unsigned long *)(BT_AHB_BASE_ADR + 0x9 *4))

#define BT_SLOT_CLKN()              ({u32 slot_clk;SAMPLECLK = 1;while(SAMPLECLK);slot_clk = SLOTCLK;slot_clk;})
#define BT_FINETIME_CNT()           (FINETIMECNT)

struct audio_bt_position {
    u32 sample_counter;
    u32 pcm_position;
    u32 bt_time;
    u32 clkoffset;
    u16 bt_time_phase;
    s16 bitoff;
    u32 buf_num;
};

struct audio_output_position {
    u32 out_samples;            //输出样点个数
    u32 bt_time;                //蓝牙时间
    u16 bt_time_phase;          //蓝牙时间相位
};

struct audio_input_position {
    u32 buf_num;            //输出buf当前缓冲样点个数
    u32 bt_time;            //蓝牙时间
    u32 clkoffset;          //与主机的偏差
    int bitoff;             //与主机偏差值的相位
    u16 bt_time_phase;      //蓝牙时间的相位
    u32 pcm_position;       //样点输入位置
};

struct resample_output_cbuf {
    s16 *(*write_alloc)(void *buffer, int *len);
    int (*write_update)(void *buffer, int len);
};

struct resample_output_frame {
    u8 free;
    void *addr;
    int len;
    void *remain_addr;
    int remain_len;
    int (*output)(void *buffer, void *data, int len);
};

struct sample_sync_device_ops {
    int (*data_len)(void *priv);
    int (*state_query)(void *priv);
};

struct audio_sample_output {
    u8 mode;
    void *priv;
    /*
    union {
        struct resample_output_cbuf cbuf;
        struct resample_output_frame frame;
    } u;
    */
    u8 free;
    void *addr;
    int len;
    void *remain_addr;
    int remain_len;
    struct sample_sync_device_ops device;
};

/*************************************************************************
 * AUDIO PCM采样同步结构
 *
 * HISTORY :
 *      2020/6/18 by LC.
 *=======================================================================*/
struct audio_sample_sync {
    u8 start;
    u8 bt_time_enable;
    u8 fixed_sample_rate;
    u8 channels;
    u8 quick_align;
    u8 position_num;
    u8 block_start;
    s8 index;
    int quick_output_count;
    int sample_rate;
    int input_sample_rate;
    u16 in_rate;
    u16 out_rate;
    u32 pcm_position;
    u32 input_num;
    u32 sample_counter;
    u32 output_counter;
    u32 bt_clkn;
    s16 bt_clkn_phase;
    struct audio_sample_output output;
    struct audio_bt_position *position;
    void *event_priv;
    int (*event_handler)(void *priv, void *output, u8 event);
    struct audio_stream_entry entry;
    struct audio_src_base_handle *src;
    void *irq_priv;
    void (*irq_callback)(void *priv);
};

struct audio_sample_sync *audio_sample_sync_open(u8 stream_mode);

/*AUDIO PCM样点同步初始化*/
int audio_sample_sync_init_resample(struct audio_sample_sync *s,
                                    int input_sample_rate,
                                    int sample_rate,
                                    u8 data_channels,
                                    u8 position_num);

/*AUDIO 样点同步模块关闭*/
void audio_sample_sync_close(struct audio_sample_sync *s);

/*AUDIO 样点同步设置fifo输出*/
int audio_sample_sync_set_fifo_output(struct audio_sample_sync *s,
                                      void *priv,
                                      struct resample_output_cbuf *fifo);

int audio_sample_sync_set_device(struct audio_sample_sync *s,
                                 void *priv,
                                 struct sample_sync_device_ops *device);

void audio_sample_sync_set_event_handler(struct audio_sample_sync *s,
        void *priv,
        int (*handler)(void *priv, void *ch, u8 event));

void audio_sample_sync_set_irq_handler(struct audio_sample_sync *s, void *priv, void (*handler)(void *));
#if 0
/*AUDIO 样点同步设置普通buffer的输出*/
int audio_sample_sync_set_output_handler(struct audio_sample_sync *s,
        void *priv,
        int (*data_len)(void *priv);
        int (*output)(void *priv, void *data, int len));
#endif
/*AUDIO 样点同步输出当前在做的数据*/
int audio_sample_sync_flush_data(struct audio_sample_sync *s);

/*AUDIO 样点同步起始设置*/
int audio_sample_sync_start(struct audio_sample_sync *s);

/*AUDIO 样点同步停止设置*/
int audio_sample_sync_stop(struct audio_sample_sync *s);

/*AUDIO 样点同步写函数，用于接入到数据流程中*/
int audio_sample_sync_write(struct audio_sample_sync *s, void *buf, int len);

/*AUDIO 使用同步的时间开始采样*/
int audio_sample_start_by_sync_time(struct audio_sample_sync *s, void *priv, void (*callback)(void *));

int audio_sample_sync_us_time_distance(struct audio_sample_sync *s);

int audio_sample_sync_bt_time_disable(struct audio_sample_sync *s);
/*AUDIO 输出设备更新样点缓冲计数*/
int audio_sample_sync_update_count(struct audio_sample_sync *s, int points);

/*AUDIO 样点同步查询输入位置(某个时间和对应具体某个样点位置)*/
int audio_sample_sync_get_in_position(struct audio_sample_sync *s, struct audio_input_position *pos);

/*AUDIO 样点同步查询输出位置(某个输出整点和对应的时间)*/
int audio_sample_sync_get_out_position(struct audio_sample_sync *s, struct audio_output_position *pos);

/*AUDIO 样点同步变采样输出剩余样点查询*/
int audio_sample_sync_out_remain_len(struct audio_sample_sync *s);

/*AUDIO 设备采样开始设置起始函数*/
int audio_sample_sync_output_begin(struct audio_sample_sync *s, int samples, int sample_rate);

int audio_sample_sync_output_rate(struct audio_sample_sync *s);

/*AUDIO 设备丢失样点设置函数*/
int audio_sample_sync_output_miss_data(struct audio_sample_sync *s);

/*AUDIO 样点同步位置修正函数*/
void audio_sample_sync_position_correct(struct audio_sample_sync *s, int num);

/*AUDIO 样点同步与网络其他相同模块对齐控制*/
int audio_sample_sync_align_control(struct audio_sample_sync *s, int in_rate, int out_rate, int points, s16 phase_diff);

/*AUDIO 样点同步本地速率控制*/
int audio_sample_sync_rate_control(struct audio_sample_sync *s, int in_rate, int out_rate);

/*AUDIO 中断模式设备更新中断消耗样点位置*/
int audio_irq_update_sample_sync_position(struct audio_sample_sync *s, int irq_sample_num);

int audio_sample_sync_set_bt_time(struct audio_sample_sync *s, u32 bt_clkn, int phase);

int audio_sample_sync_time_distance(struct audio_sample_sync *s);

int audio_sample_sync_output_query(struct audio_sample_sync *s);

int audio_sample_sync_is_working(struct audio_sample_sync *s);

u32 audio_sample_sync_input_pcm_num(struct audio_sample_sync *s);
#endif
