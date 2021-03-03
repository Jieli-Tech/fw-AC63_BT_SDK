
#ifndef _AUDIO_ENC_RECODER_H_
#define _AUDIO_ENC_RECODER_H_

#include "media/audio_encoder.h"


int audio_adc_mic_init(u16 sr);
void audio_adc_mic_exit(void);

void linein_sample_set_resume_handler(void *priv, void (*resume)(void));
void fm_inside_output_handler(void *priv, s16 *data, int len);
int linein_sample_read(void *hdl, void *data, int len);
int linein_sample_size(void *hdl);
int linein_sample_total(void *hdl);
int linein_stream_sample_rate(void *hdl);
void *linein_sample_open(u8 source, u16 sample_rate);
void linein_sample_close(void *hdl);
void *fm_sample_open(u8 source, u16 sample_rate);
void fm_sample_close(void *hdl, u8 source);

////>>>>>>>>>>>>>>record_player api录音接口<<<<<<<<<<<<<<<<<<<<<///
void recorder_pcm2file_write_pcm_ex(s16 *data, int len);
void recorder_encode_stop(void);
u32 recorder_get_encoding_time();
///检查录音是否正在进行
int recorder_is_encoding(void);
void recorder_device_offline_check(char *logo);


#endif

