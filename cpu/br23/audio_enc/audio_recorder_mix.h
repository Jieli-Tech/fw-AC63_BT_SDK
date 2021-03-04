#ifndef _AUDIO_RECORDER_MIX_H_
#define _AUDIO_RECORDER_MIX_H_

#include "system/includes.h"
#include "media/includes.h"


struct recorder_mix_stream {
    u8						  source;
    struct audio_stream_entry entry;
};

void recorder_mix_pcm_set_info(u16 sample_rate, u8 channel);
u32 recorder_mix_pcm_write(u8 *data, u16 len);
u32 recorder_mix_pcm_write(u8 *data, u16 len);
u32 recorder_mix_sco_data_write(u8 *data, u16 len);
void recorder_mix_stream_resume(void);
void recorder_mix_init(struct audio_mixer *mixer, s16 *mix_buf, u16 buf_size);
int recorder_mix_start(void);
void recorder_mix_stop(void);
int recorder_mix_get_status(void);
u16 recorder_mix_get_samplerate(void);
u32 recorder_mix_get_coding_type(void);
void recorder_mix_bt_status(u8 status);
void recorder_mix_call_status_change(u8 active);
void recorder_mix_bt_status_event(struct bt_event *e);
int recorder_mix_pcm_stream_open(u16 sample_rate, u8 channel);
void recorder_mix_pcm_stream_close(void);

#endif//_AUDIO_RECORDER_MIX_H_

