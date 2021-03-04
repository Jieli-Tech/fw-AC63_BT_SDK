
#ifndef _AUDIO_DEC_FM_H_
#define _AUDIO_DEC_FM_H_

#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "media/audio_decoder.h"
#include "mic_effect.h"

// fm数据填充
void fm_sample_output_handler(s16 *data, int len);
// fm解码释放
void fm_dec_relaese();

// fm解码开始
int fm_dec_start();
// 打开fm解码
int fm_dec_open(u8 source, u32 sample_rate);
// 关闭fm解码
void fm_dec_close(void);
// fm解码重新开始
int fm_dec_restart(int magic);
// 推送fm解码重新开始命令
int fm_dec_push_restart(void);
// 暂停/启动 fm解码mix ch输出
void fm_dec_pause_out(u8 pause);

/***********************inein pcm enc******************************/
// fm录音停止
void fm_pcm_enc_stop(void);
// fm录音开始
int fm_pcm_enc_start(void);
// 检测fm是否在录音
bool fm_pcm_enc_check();

#endif
