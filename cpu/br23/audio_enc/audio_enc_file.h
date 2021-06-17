
#ifndef _AUDIO_ENC_FILE_H_
#define _AUDIO_ENC_FILE_H_

#include "media/audio_encoder.h"
#include "dev_manager.h"


// 写pcm数据
int pcm2file_enc_write_pcm(void *priv, s16 *data, int len);
void *pcm2file_enc_open(struct audio_fmt *pfmt, char *logo, char *folder, char *filename);
void pcm2file_enc_write_file_set_limit(void *hdl, u32 cut_size, u32 limit_size);
void pcm2file_enc_set_evt_handler(void *hdl, void (*handler)(struct audio_encoder *, int, int *), u32 maigc);
void pcm2file_enc_start(void *hdl);
void enc_change_status(void *hdl);
void pcm2file_enc_close(void **hdl);
int pcm2file_enc_is_work(void *hdl);
int get_pcm2file_enc_file_len(void *hdl);
struct audio_encoder *get_pcm2file_encoder_hdl(void *hdl);
int pcm2file_enc_get_time(void *hdl);
int pcm2file_enc_get_tmark(void *hdl, u8 *buf, u16 len);

#endif






