#ifndef _AUDIO_IIS_H_
#define _AUDIO_IIS_H_

#include "audio_link.h"

extern void *audio_iis_output_start(ALINK_PORT port, u8 ch);
extern void audio_iis_output_stop(ALINK_PORT port);
extern int audio_iis_output_write(s16 *data, u32 len);
extern void audio_iis_output_set_srI(u32 sample_rate);
extern int audio_iis_output_sync_write(s16 *data, u32 len, u8 data_sync);
extern int audio_iis_channel_sync_enable(struct audio_sample_sync *samp_sync);
extern int audio_iis_channel_sync_disable(struct audio_sample_sync *samp_sync);

extern void audio_iis_input_start(ALINK_PORT port, u8 ch, void (*handle)(u8 ch, s16 *buf, u32 len));
extern void audio_iis_input_stop(ALINK_PORT port, u8 ch);

#endif

