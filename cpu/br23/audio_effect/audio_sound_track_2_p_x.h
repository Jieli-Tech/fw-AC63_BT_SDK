#ifndef _SOUND_TRACK_2_P_X_H_
#define _SOUND_TRACK_2_P_X_H_

#include "app_config.h"
#include "clock_cfg.h"
#include "media/includes.h"
#include "asm/includes.h"
#include "application/eq_config.h"
#include "application/audio_eq_drc_apply.h"
#include "application/audio_vocal_tract_synthesis.h"
#include "audio_config.h"

struct sound_track_2_p_x {
    struct audio_eq_drc *eq_drc;    //eq drc句柄
    struct audio_eq_drc *low_bass_rl_rr;//eq drc句柄
    struct audio_drc *low_bass_drc_pre;//做低通滤波前，先做一级衰减


    struct audio_stream_entry entry;	// 音频流入口
    u8 sound_track_type;
    u8 out_ch_num;
    u8  data_sync;
    u16 dat_len;
    u16 dat_total;
    u16 buf_len;
    u16 *buf;

    u16 fade_tmr;
};

struct sound_track_2_p_x *sound_track_2_p_x_open(u8 type, u32 sample_rate, u8 ch_num);
void sound_track_2_p_x_close(struct sound_track_2_p_x *hdl);

void low_bass_set_global_gain(struct sound_track_2_p_x *hdl, float left_global_gain, float right_global_gain);
#endif/*_SOUND_TRACK_2_P_X_H_*/




