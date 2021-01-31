/*****************************************************************
>file name : audio_cfifo.h
>author : lichao
>create time : Tue 10 Nov 2020 02:22:37 PM CST
*****************************************************************/
#ifndef _AUDIO_CFIFO_H_
#define _AUDIO_CFIFO_H_
#include "generic/list.h"
#include "typedef.h"

#define WRITE_MODE_BLOCK            0
#define WRITE_MODE_FORCE            1

struct audio_cfifo {
    u8 sample_channel;
    s16 *addr;
    u16 sample_size;
    u16 wp;
    u16 rp;
    u16 lock_rp;
    u16 free_samples;
    s16 unread_samples;
    int sample_rate;
    struct list_head head;
};

struct audio_cfifo_channel {
    u8  write_mode;
    u16 delay_time;
    u16 rsp;
    u16 wsp;
    s16 unread_samples;
    u16 max_samples;
    struct audio_cfifo *fifo;
    struct list_head entry;
};

int audio_cfifo_init(struct audio_cfifo *fifo, void *buf, int len, int sample_rate, u8 channel);

int audio_cfifo_reset(struct audio_cfifo *fifo, void *buf, int len, int sample_rate, u8 channel);

int audio_cfifo_channel_add(struct audio_cfifo *fifo, struct audio_cfifo_channel *ch, int delay_time, u8 write_mode);

void audio_cfifo_channel_del(struct audio_cfifo_channel *ch);

int audio_cfifo_read_update(struct audio_cfifo *fifo, int samples);

int audio_cfifo_channel_write(struct audio_cfifo_channel *ch, void *data, int len);

int audio_cfifo_channel_write_fixed_data(struct audio_cfifo_channel *ch, s16 data, int len);

int audio_cfifo_channel_clear(struct audio_cfifo_channel *ch);

int audio_cfifo_get_write_offset(struct audio_cfifo *fifo);

int audio_cfifo_get_unread_samples(struct audio_cfifo *fifo);

int audio_cfifo_channel_unread_diff_samples(struct audio_cfifo_channel *ch);

int audio_cfifo_channel_num(struct audio_cfifo *fifo);

void audio_cfifo_set_readlock_samples(struct audio_cfifo *fifo, int samples);

int audio_cfifo_read_with_callback(struct audio_cfifo *fifo, int offset, int samples,
                                   void *priv, int (*read_callback)(void *priv, void *data, int len));

int audio_cfifo_channel_unread_samples(struct audio_cfifo_channel *ch);

int audio_cfifo_channel_write_offset(struct audio_cfifo_channel *ch);

struct audio_cfifo_channel *audio_cfifo_min_samples_channel(struct audio_cfifo *fifo);
#endif
