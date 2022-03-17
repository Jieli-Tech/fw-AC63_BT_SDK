#ifndef _AUDIO_IIS_H_
#define _AUDIO_IIS_H_



extern void audio_iis_output_start(u8 ch);
extern void audio_iis_output_stop();
extern int audio_iis_output_write(s16 *data, u32 len);
extern void audio_iis_output_set_srI(u32 sample_rate);


void audio_iis_add_syncts_handle(void *syncts);
void audio_iis_remove_syncts_handle(void *syncts);
int audio_iis_data_time();
int audio_iis_buffered_frames();
void audio_iis_count_to_syncts(int frames);
void audio_iis_syncts_latch_trigger();
int audio_iis_irq_resume_decoder(int time_ms, void *priv, void (*callback)(void *));
u8 audio_iis_is_working();

#endif

