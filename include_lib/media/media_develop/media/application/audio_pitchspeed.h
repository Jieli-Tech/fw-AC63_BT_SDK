
#ifndef _AUDIO_PITCHSPEED_API_H_
#define _AUDIO_PITCHSPEED_API_H_

#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"

#include "asm/ps_cal_api.h"

typedef struct _s_pitchspeed_hdl {
    PS69_API_CONTEXT *ops;
    PS69_CONTEXT_CONF param;
    unsigned char 	*ptr;           	//运算buf指针
    PS69_audio_IO    speed_IO;
    u8 updata_mark;

    struct audio_stream_entry entry;	// 音频流入口
    int out_len;
    int process_len;
    u8 run_en;
    s16 *out_buf;
    int out_buf_len;
} s_pitchspeed_hdl;



s_pitchspeed_hdl *open_pitchspeed(PS69_CONTEXT_CONF *param, PS69_audio_IO *speed_IO);
int run_pitchspeed(s_pitchspeed_hdl *ps_hdl, s16 *indata, int len);
void close_pitchspeed(s_pitchspeed_hdl *ps_hdl);
void updat_pitchspeed_parm(s_pitchspeed_hdl *ps_hdl, PS69_CONTEXT_CONF *param);

#endif //_AUDIO_PITCHSPEED_API_H_
