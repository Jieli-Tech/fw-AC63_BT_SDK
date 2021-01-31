/*****************************************************************
>file name : audio_src.h
>author : lichao
>create time : Fri 14 Dec 2018 03:05:49 PM CST
*****************************************************************/
#ifndef _AUDIO_SRC_H_
#define _AUDIO_SRC_H_
#include "media/audio_stream.h"
#include "audio_resample.h"


#define SRC_DATA_MIX    0//各通道数据交叉存放
#define SRC_DATA_SEP    1//各通道数据连续存放

#define SRC_CHI                     2
#define SRC_FILT_POINTS             24

#define SRC_MODE_DEFAULT            0
#define SRC_MODE_FULL_INPUT         1
#define SRC_MODE_FULL_OUTPUT        2

#define SRC_TYPE_RESAMPLE           SRC_MODE_DEFAULT

enum audio_src_event {
    SRC_EVENT_INPUT_DONE = 0x0,
    SRC_EVENT_OUTPUT_DONE,
    SRC_EVENT_ALL_DONE,
    SRC_EVENT_GET_OUTPUT_BUF,
    SRC_EVENT_RATE_UPDATE,
    SRC_EVENT_RISE_IRQ,
    SRC_EVENT_RISE_IRQ_NEEDRUN,
};

enum audio_src_error_code {
    SRC_BASE_NO_ERROR = 0x0,
    SRC_OUTPUT_NO_BUFF = 0x4,
    SRC_OUTPUT_NOT_COMPLETED,
};

#define HW_RESAMPLE_RATE_MATCH_TO_16BIT(in, out) \
    while (in >= 65535L || out >= 65535L) { \
        in >>= 1;out >>= 1;  \
    }

struct audio_src_rate {
    u16 irate;
    u16 orate;
};

struct audio_src_buffer {
    void *addr;
    int len;
};
/*
 * SRC BASE模块接口
 */

// *INDENT-OFF*
struct audio_src_base_handle {
    u8 channels;
    u8 state;
    volatile u8 active;
    u8 mode;
    u8 wait_irq;
    u8 input_malloc;
    u8 rate_update;
	//u8 filt_index;
    //u32 tuned_points;
    struct audio_src_rate rate;
    struct audio_src_buffer input;
    struct audio_src_buffer output;
    struct list_head entry;
    void *event_priv;
    int  (*event_handler)(void *priv, enum audio_src_event event, void *);
    JL_SRC_TypeDef regs;
};
// *INDENT-ON*
int audio_src_base_filt_init(void *filt, int len);

int audio_src_base_open(struct audio_src_base_handle *src, u8 channel, u8 type);

int audio_src_base_set_rate(struct audio_src_base_handle *src, int in_rate, int out_rate);

int audio_src_base_get_rate(struct audio_src_base_handle *src, u16 *in_rate, u16 *out_rate);

int audio_src_base_set_channel(struct audio_src_base_handle *src, u8 channel);

void audio_src_base_set_event_handler(struct audio_src_base_handle *src, void *priv,
                                      int (*handler)(void *priv, enum audio_src_event event, void *));

int audio_src_base_set_input_buff(struct audio_src_base_handle *src, void *buf, int len);

int audio_src_base_write(struct audio_src_base_handle *src, void *data, int len);

int audio_src_base_try_write(struct audio_src_base_handle *src, void *data, int len);

int audio_src_base_data_flush_out(struct audio_src_base_handle *src);

int audio_src_base_sub_phase(struct audio_src_base_handle *src, int sub_phase);

int audio_src_base_get_phase(struct audio_src_base_handle *src);

int audio_src_base_stop(struct audio_src_base_handle *src);

int audio_src_base_idata_len(struct audio_src_base_handle *s);

int audio_src_base_data_len(struct audio_src_base_handle *s);

int audio_src_base_resample(struct audio_src_base_handle *s,
                            struct audio_resample_data *in,
                            struct audio_resample_data *out);

void audio_src_base_close(struct audio_src_base_handle *src);

//int audio_src_is_running(struct audio_src_base_handle *src);
int audio_src_ch_is_running(struct audio_src_base_handle *src);

// *INDENT-OFF*
struct audio_src_handle {
    struct audio_src_base_handle base;
    struct audio_src_buffer output;
    void *output_priv;
    int (*output_handler)(void *priv, void *data, int len);
    void *rise_irq_priv;
    void (*rise_irq_handler)(void *priv);
    u8 *remain_addr;
    int remain_len;
    int in_sample_rate;
    int out_sample_rate;
    u8 channel;
    u8 output_malloc;
    u8 check_hw_running;
    struct audio_stream_entry entry;
};
// *INDENT-ON*

int audio_hw_src_open(struct audio_src_handle *src, u8 channel, u8 type);

int audio_hw_src_set_rate(struct audio_src_handle *src, int in_sample_rate, int out_sample_rate);

int audio_hw_src_correct_rate(struct audio_src_handle *src, int in_step, int out_step);

int audio_src_resample_write(struct audio_src_handle *src, void *data, int len);

void audio_src_set_output_handler(struct audio_src_handle *src, void *priv,
                                  int (*handler)(void *, void *, int));

void audio_src_set_rise_irq_handler(struct audio_src_handle *src, void *priv,
                                    void (*handler)(void *));

int audio_hw_src_set_input_buffer(struct audio_src_handle *src, void *addr, int len);

int audio_hw_src_set_output_buffer(struct audio_src_handle *src, void *addr, int len);

int audio_hw_src_stop(struct audio_src_handle *src);

void audio_hw_src_close(struct audio_src_handle *src);

// 检测到硬件正在运行时不等待其完成，直接返回
int audio_hw_src_set_check_running(struct audio_src_handle *src, u8 check_hw_running);

int audio_hw_src_active(struct audio_src_handle *src);

#endif
