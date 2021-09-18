/*****************************************************************
>file name : audio_sync_resample.h
>author : lichao
>create time : Sat 26 Dec 2020 02:27:11 PM CST
*****************************************************************/
#ifndef _AUDIO_SYNC_RESAMPLE_H_
#define _AUDIO_SYNC_RESAMPLE_H_
#include "typedef.h"
#include "system/includes.h"
#include "os/os_api.h"

#ifdef CONFIG_BR30_C_VERSION
#define RESAMPLE_HW_24BIT                           1
#else
#define RESAMPLE_HW_24BIT                           0
#endif
#define INPUT_FRAME_BITS                            18//20 -- 整数位减少可提高单精度浮点的运算精度
#define RESAMPLE_INPUT_BIT_RANGE                    ((1 << INPUT_FRAME_BITS) - 1)
#define RESAMPLE_INPUT_BIT_NUM                      (1 << INPUT_FRAME_BITS)

struct resample_output_buffer {
    u8 mode;
    void *priv;
    union {
        struct resample_outside_buffer {
            int (*get)(void *, s16 **);
            void (*put)(void *, s16 *, int);
        } outside;

        struct resample_inside_buffer {
            void *addr;
            int size;
            int (*output)(void *, void *, int);
        } inside;
    };
};

struct resample_buffer_data {
    void *addr;
    int  len;
};

// *INDENT-OFF*
struct audio_sync_resample {
    u8 channels;
    u8 state;
    u8 active;
    volatile u8 wait_irq;
    u8 config_update;
    u8 config_lock;
    u8 slience;
    //u8 id;
    int normal_insr;
    int normal_outsr;
    int new_insr;
    int new_outsr;
    u32 input_frames : INPUT_FRAME_BITS;
    float resampled_frames;
    u32 resampled_out_frames;
    int scale_frames;
    void *wait_irq_priv;
    void (*wait_irq_callback)(void *);
    struct resample_output_buffer buffer;
    struct resample_buffer_data input;
    struct resample_buffer_data output;
    JL_SRC_SYNC_TypeDef regs;
    OS_SEM sem;
    struct list_head entry;
    void *remain_addr;
    int remain_len;
    s16 fade_volume;
    s16 fade_step;
};
// *INDENT-ON*

int audio_sync_resample_set_filt(void *filt, int len);

void *audio_sync_resample_open(void);
int audio_sync_resample_init(void *resample, int input_sample_rate, int sample_rate, u8 data_channels);
int audio_sync_resample_set_output_handler(void *resample,
        void *priv,
        int (*handler)(void *priv, void *data, int len));
void audio_sync_resample_close(void *resample);

int audio_sync_resample_set_slience(void *resample, u8 slience, int fade_time);
int audio_sync_resample_set_in_buffer(void *resample, void *buf, int len);
int audio_sync_resample_config(void *resample, int in_rate, int out_rate);
int audio_sync_resample_flush_out(void *resample, u8 no_wait);
int audio_sync_resample_write(void *resample, void *data, int len);
int audio_sync_resample_stop(void *resample);
void audio_sync_resample_reset(void *resample);
float audio_sync_resample_position(void *resample);
int audio_sync_resample_scale_output(void *resample, int in_sample_rate, int out_sample_rate, int frames);
int audio_sync_resample_bufferd_frames(void *resample);
int audio_sync_resample_all_samples(void *resample);
u32 audio_sync_resample_out_frames(void *resample);
int audio_sync_resample_wait_irq_callback(void *resample, void *priv, void (*callback)(void *));
#endif

