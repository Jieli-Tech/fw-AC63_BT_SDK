
#ifndef _AUDIO_DEC_PC_H_
#define _AUDIO_DEC_PC_H_

#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "media/audio_decoder.h"

// pc解码重新开始
int uac_dec_restart(int magic);
// 推送pc解码重新开始命令
int uac_dec_push_restart(void);

#endif /* TCFG_APP_PC_EN */

