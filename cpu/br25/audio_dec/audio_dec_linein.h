
#ifndef _AUDIO_DEC_LINEIN_H_
#define _AUDIO_DEC_LINEIN_H_

#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "media/audio_decoder.h"

void linein_dec_relaese();
int linein_dec_start();

int linein_dec_open(u8 source, u32 sample_rate);
void linein_dec_close(void);
int linein_dec_restart(int magic);
int linein_dec_push_restart(void);

/***********************linein pcm enc******************************/
void linein_pcm_enc_stop(void);
int linein_pcm_enc_start(void);
bool linein_pcm_enc_check();

#endif
