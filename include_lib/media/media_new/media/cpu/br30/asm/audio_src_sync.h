/*****************************************************************
>file name : audio_src_sync.h
>author : lichao
>create time : Fri 14 Dec 2018 03:05:49 PM CST
*****************************************************************/
#ifndef _AUDIO_SRC_SYNC_H_
#define _AUDIO_SRC_SYNC_H_

#include "system/includes.h"
#include "audio_src.h"

enum audio_src_sync_event {
    SRC_SYNC_EVENT_INPUT_DONE = 0x0,
    SRC_SYNC_EVENT_OUTPUT_DONE,
    SRC_SYNC_EVENT_ALL_DONE,
    SRC_SYNC_EVENT_GET_OUTPUT_BUF,
    SRC_SYNC_EVENT_RATE_UPDATE,
};


// *INDENT-OFF*
struct audio_src_sync_handle {
    u8 channels;
    volatile u8 state;
    volatile u8 lock;
    u8 rate_update;
#ifdef CONFIG_BR30_C_VERSION
    int new_insr;
    int new_outsr;
#else
    u32 new_rate;
#endif
    struct audio_src_buffer input;
    struct audio_src_buffer output;
    struct list_head entry;
    void *event_priv;
    int  (*event_handler)(void *priv, enum audio_src_sync_event event, void *);
    OS_SEM sem;
    u8 wait_irq;
    void *wait_irq_priv;
    void (*wait_irq_callback)(void *);
};
// *INDENT-ON*

int audio_src_sync_open(struct audio_src_sync_handle *src, u8 channel);

int audio_src_sync_set_rate(struct audio_src_sync_handle *src, int in_rate, int out_rate);

int audio_src_sync_set_rate_type2(struct audio_src_sync_handle *src, int in_rate, int out_rate);

int audio_src_sync_get_rate(struct audio_src_sync_handle *src, int *in_rate, int *out_rate);

int audio_src_sync_set_channel(struct audio_src_sync_handle *src, u8 channel);

int audio_src_sync_set_buff(struct audio_src_sync_handle *s, void *buf, int len);

int audio_src_sync_set_filt(struct audio_src_sync_handle *s, void *filt_addr, int len);

int audio_src_sync_set_cbuf_addr(struct audio_src_sync_handle *s, void *buf, int len);

int audio_src_sync_set_auto_mode(struct audio_src_sync_handle *s, u8 auto_mode);

void audio_src_sync_set_event_handler(struct audio_src_sync_handle *src, void *priv,
                                      int (*handler)(void *priv, enum audio_src_sync_event event, void *));

int audio_src_sync_write(struct audio_src_sync_handle *src, void *data, int len);

int audio_src_sync_data_flush_out(struct audio_src_sync_handle *src, u8 no_wait);

int audio_src_sync_sub_phase(struct audio_src_sync_handle *src, int sub_phase);

int audio_src_sync_get_phase(struct audio_src_sync_handle *src);

int audio_src_sync_output_lock(struct audio_src_sync_handle *s);

int audio_src_sync_output_unlock(struct audio_src_sync_handle *s);

int audio_src_sync_data_len(struct audio_src_sync_handle *src);

int audio_src_sync_stop(struct audio_src_sync_handle *src);

int audio_src_sync_idata_len(struct audio_src_sync_handle *s);

int audio_src_sync_wait_irq_callback(struct audio_src_sync_handle *s, void *priv, void (*callback)(void *));

void audio_src_sync_close(struct audio_src_sync_handle *src);

#endif
