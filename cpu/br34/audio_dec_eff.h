#ifndef _AUD_DEC_EFF_H
#define _AUD_DEC_EFF_H
#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "classic/tws_api.h"
#include "classic/hci_lmp.h"
#include "application/eq_config.h"
#include "application/audio_surround.h"
#include "app_config.h"
#include "audio_config.h"
#include "app_main.h"
#include "application/audio_vbass.h"


struct dec_sur {
#if AUDIO_SURROUND_CONFIG
    surround_hdl *surround;         //环绕音效句柄
    u8 surround_eff;  //音效模式记录
#endif

};

#if (defined(TCFG_AUDIO_OUT_EQ_ENABLE) && (TCFG_AUDIO_OUT_EQ_ENABLE != 0))
#define AUDIO_OUT_EQ_USE_SPEC_NUM		2	// 使用特定的eq段
#else
#define AUDIO_OUT_EQ_USE_SPEC_NUM		0
#endif
#define AUDIO_EQ_FADE_EN  1
#define HIGH_BASS_EQ_FADE_STEP (1)

#if TCFG_EQ_ENABLE&&TCFG_AUDIO_OUT_EQ_ENABLE
#define AUDIO_OUT_EFFECT_ENABLE			1	// 音频输出时的音效处理
#else
#define AUDIO_OUT_EFFECT_ENABLE			0
#endif//TCFG_AUDIO_OUT_EQ_ENABLE


typedef int (*eq_output_cb)(void *, void *, int);

struct eq_filter_fade {
    u16 tmr;
    int cur_gain[AUDIO_OUT_EQ_USE_SPEC_NUM];
    int use_gain[AUDIO_OUT_EQ_USE_SPEC_NUM];
};
struct dec_eq_drc {
    s16 *eq_out_buf;
    int eq_out_buf_len;
    int eq_out_points;
    int eq_out_total;

    void *priv;
    eq_output_cb  out_cb;
    void *drc_prev;
    void *eq;
    void *drc;
    u8 async;
    u8 drc_bef_eq;
    struct eq_filter_fade fade;
    u8 remain;
};

struct eq_parm_new {
    u8 in_mode: 2;
    u8 run_mode: 2;
    u8 data_in_mode: 2;
    u8 data_out_mode: 2;
};
void *audio_surround_setup(u8 channel, u8 eff);
void audio_surround_free(void *sur);
void audio_surround_set_ch(void *sur, u8 channel);
void audio_surround_voice(void *sur, u8 en);


vbass_hdl *audio_vbass_setup(u32 sample_rate, u8 channel);
void audio_vbass_free(vbass_hdl *vbass);


void *dec_eq_drc_setup(void *priv, int (*eq_output_cb)(void *, void *, int), u32 sample_rate, u8 channel, u8 async, u8 drc_en);
void dec_eq_drc_free(void *eff);


void *esco_eq_drc_setup(void *priv, int (*eq_output_cb)(void *, void *, int), u32 sample_rate, u8 channel, u8 async, u8 drc_en);
void esco_eq_drc_free(void *eff);


void *audio_out_eq_drc_setup(void *priv, int (*eq_output_cb)(void *, void *, int), u32 sample_rate, u8 channel, u8 async, u8 drc_en);
void audio_out_eq_drc_free(void *eff);
int audio_out_eq_set_gain(void *eff, u8 idx, int gain);

int eq_drc_run(void *priv, void *data, u32 len);

void mix_out_drc_open(u16 sample_rate);
void mix_out_drc_close();
void mix_out_drc_run(s16 *data, u32 len);
/*----------------------------------------------------------------------------*/
/**@brief    mix_out后限幅器系数更新
   @param    threadhold限幅器阈值，-60~0,单位db
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void mix_out_drc_threadhold_update(float threadhold);
#define V1_GAME_EFF  1
#define NOR_GAME_EFF 2

#endif
