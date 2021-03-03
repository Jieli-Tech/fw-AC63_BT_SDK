#ifndef _AUDIO_RECORDER_MIX_H_
#define _AUDIO_RECORDER_MIX_H_

#include "system/includes.h"
#include "media/includes.h"

void recorder_mix_data_stream_resume(void);
void recorder_mix_audio_stream_entry_add(struct audio_stream_entry *start_entry);
void recorder_mix_audio_stream_entry_del(void);
int recorder_mix_start(void);
void recorder_mix_stop(void);
int recorder_mix_get_status(void);
int recorder_get_enc_time();

u32 recorder_mix_sco_data_write(u8 *data, u16 len);
u32 recorder_mix_get_coding_type(void);
void recorder_mix_call_status_change(u8 active);
void recorder_mix_bt_status_event(struct bt_event *e);
void recorder_mix_pcm_set_info(u16 sample_rate, u8 channel);

#endif//_AUDIO_RECORDER_MIX_H_


