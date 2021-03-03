
#ifndef _AUDIO_DEC_BT_H_
#define _AUDIO_DEC_BT_H_

#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "media/audio_decoder.h"

// a2dp或者esco正在播放
u8 bt_audio_is_running(void);
// a2dp正在播放
u8 bt_media_is_running(void);
// esco正在播放
u8 bt_phone_dec_is_running();;

// 打开a2dp解码
int a2dp_dec_open(int media_type);
// 关闭a2dp解码
int a2dp_dec_close();

// 打开esco解码
int esco_dec_open(void *, u8);
// 关闭esco解码
void esco_dec_close();

// a2dp传输中止
void __a2dp_drop_frame(void *p);

#endif

