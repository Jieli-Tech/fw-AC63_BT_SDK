#ifndef _APP_AUDIO_H_
#define _APP_AUDIO_H_

#include "generic/typedef.h"
#include "board_config.h"

extern u8 get_max_sys_vol(void);
s8 app_audio_get_volume(u8 state);
#define APP_AUDIO_STATE_MUSIC       1

#define SYS_MAX_VOL             16
#define SYS_DEFAULT_VOL         16
#define SYS_DEFAULT_TONE_VOL    16
#define SYS_DEFAULT_SIN_VOL    	8

#endif/*_APP_AUDIO_H_*/
