#ifndef _AUDIO_IIS_H_
#define _AUDIO_IIS_H_



extern void audio_iis_output_start(u8 ch);
extern void audio_iis_output_stop();
extern int audio_iis_output_write(s16 *data, u32 len);
extern void audio_iis_output_set_srI(u32 sample_rate);





#endif

