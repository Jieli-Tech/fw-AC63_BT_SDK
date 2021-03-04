
#ifndef _AUDIO_DEC_SPDIF_H_
#define _AUDIO_DEC_SPDIF_H_

#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "media/audio_decoder.h"

// 写入spdif数据
// int spdif_dec_write_data(s16 *data, int len);
int spdif_dec_write_data(void *data, int len);
void spdif_init(void);
int spdif_dec_start(void);
int spdif_dec_stop(void);
int spdif_dec_open(struct audio_fmt fmt);
void spdif_dec_close(void);
bool spdif_dec_check(void);

void spdif_dec_init(void);
#endif

