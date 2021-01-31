
#ifndef _AUDIO_DEC_BT_H_
#define _AUDIO_DEC_BT_H_

#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "media/audio_decoder.h"

u8 bt_audio_is_running(void);
u8 bt_media_is_running(void);
u8 bt_phone_dec_is_running();;

int a2dp_dec_open(int media_type);
int a2dp_dec_close();

int esco_dec_open(void *, u8);
void esco_dec_close();

void __a2dp_drop_frame(void *p);

#endif

