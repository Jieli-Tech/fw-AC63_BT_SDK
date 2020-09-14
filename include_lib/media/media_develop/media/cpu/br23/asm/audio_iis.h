/*****************************************************************
>file name : audio_iis.h
>author : lichao
>create time : Fri 29 May 2020 01:52:36 PM CST
*****************************************************************/
#ifndef _ASM_AUDIO_IIS_H_
#define _ASM_AUDIO_IIS_H_
#include "generic/typedef.h"

#define IIS_MAX_POSITION_NUM    3

struct audio_iis_irq_position {
    u32 buf_counter;
    u32 pcm_position;
    u32 clkn;
    u32 clkoffset;
    u16 clkn_phase;
    s16 bitoff;
    u32 buf_num;
};

struct audio_iis_buffer {
    void *addr;
    u32 len;
};

struct audio_iis_sync_handle {
    u8 start;
    u8 channel;
    u8 fast_align;
    u8 index;
    u8 align_start;
    int points;
    int start_points;
    int fast_points;
    u32 pcm_position;
    u32 input_num;
    u32 buf_num;
    u32 buf_counter;
    u16 in_rate;
    u16 out_rate;
    struct audio_iis_irq_position position[IIS_MAX_POSITION_NUM];
    struct audio_src_base_handle *src_base;
    void *priv;
    int (*handler)(void *priv, u8 device, u8 state);
};

struct audio_iis_handle {
    u8 channel;
    struct audio_iis_sync_handle sync;
    void *priv;
    int (*buf_handler)(void *priv, u8 state, struct audio_iis_buffer *buf);
};

#define AUDIO_IIS_GET_OUTPUT_BUFFER     0
#define AUDIO_IIS_PUT_OUTPUT_BUFFER     1

void audio_iis_set_channel(void *iis, u8 channel);

int audio_iis_sync_open(void *iis);

int audio_iis_sync_connect_open(void *_iis,
                                void *priv,
                                int (*handler)(void *priv, u8 state, struct audio_iis_buffer *buf));

void audio_iis_set_sync_handler(void *iis, void *priv, int (*handler)(void *priv, u8 device, u8 state));

int audio_iis_sync_set_channel(void *iis, u8 channel);

int audio_iis_sync_start(void *iis);

int audio_iis_sync_stop(void *iis);

void audio_iis_sync_input_num_correct(void *iis, int num);

int audio_iis_sync_fast_align(void *iis, int in_rate, int out_rate, int fast_output_points, s16 phase_diff);

int audio_iis_sync_flush_data(void *iis);

int audio_iis_sync_write(void *_iis, void *data, int len);

int audio_iis_sync_set_rate(void *iis, u16 in_rate, u16 out_rate);

u32 audio_iis_sync_pcm_position(void *iis, u32 bt_time, u32 bt_time_phase);

void audio_iis_sync_close(void *iis);

void audio_iis_bt_location_lock_read(void *iis, u32 *dabuf, u32 *bt_time, u16 *bt_time_phase, int *clkoffset, int *bitoff);

u32 audio_iis_buf_counter_number(void *iis, u32 bt_time, u32 bt_time_phase);

int audio_iis_sync_set_start_points(void *_iis, int start_points);

int audio_iis_sync_reset(void *_iis);

u8 audio_iis_sync_align_start(void *_iis);

int audio_iis_sync_len_update(struct audio_iis_handle *iis, int len);

bool audio_iis_is_working(void *_iis);
#endif
