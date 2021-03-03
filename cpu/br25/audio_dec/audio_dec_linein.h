
#ifndef _AUDIO_DEC_LINEIN_H_
#define _AUDIO_DEC_LINEIN_H_

#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "media/audio_decoder.h"

// linein解码释放
void linein_dec_relaese();
// linein解码开始
int linein_dec_start();

// 打开linein解码
int linein_dec_open(u8 source, u32 sample_rate);
// 关闭linein解码
void linein_dec_close(void);
// linein解码重新开始
int linein_dec_restart(int magic);
// 推送linein解码重新开始命令
int linein_dec_push_restart(void);

/***********************linein pcm enc******************************/
// linein录音停止
void linein_pcm_enc_stop(void);
// linein录音开始
int linein_pcm_enc_start(void);
// 检测linein是否在录音
bool linein_pcm_enc_check();

#endif
