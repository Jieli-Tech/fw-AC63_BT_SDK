
#ifndef _AUDIO_DEC_H_
#define _AUDIO_DEC_H_

#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "asm/audio_src.h"
#include "audio_digital_vol.h"
#include "application/audio_eq_drc_apply.h"
#include "application/audio_vocal_tract_synthesis.h"
#include "application/audio_dig_vol.h"

#ifndef RB16
#define RB16(b)    (u16)(((u8 *)b)[0] << 8 | (((u8 *)b))[1])
#endif

extern struct audio_decoder_task decode_task;
extern struct audio_mixer mixer;
extern struct audio_mixer recorder_mixer;

u32 audio_output_rate(int input_rate);
u32 audio_output_channel_num(void);
u32 audio_output_channel_type(void);
int audio_output_set_start_volume(u8 state);

int audio_output_start(u32 sample_rate, u8 reset_rate);
void audio_output_stop(void);

struct audio_src_handle *audio_hw_resample_open(void *priv, int (*output_handler)(void *, void *, int),
        u8 channel, u16 input_sample_rate, u16 output_sample_rate);
void audio_hw_resample_close(struct audio_src_handle *hdl);

void audio_resume_all_decoder(void);

u32 audio_output_nor_rate(void);

enum {
    AUDIO_MODE_MAIN_STATE_DEC_A2DP = 1,
    AUDIO_MODE_MAIN_STATE_DEC_ESCO,
    AUDIO_MODE_MAIN_STATE_DEC_FILE,
    AUDIO_MODE_MAIN_STATE_DEC_FM,
    AUDIO_MODE_MAIN_STATE_DEC_LINEIN,
    AUDIO_MODE_MAIN_STATE_DEC_PC,
    AUDIO_MODE_MAIN_STATE_DEC_SPDIF,
    AUDIO_MODE_MAIN_STATE_DEC_LOCALTWS,

    AUDIO_MODE_MAIN_STATE_DEC_MASK = 0xff,
};

enum {
    KARAOKE_SPK_OST,//原声
    KARAOKE_SPK_DBB,//重低音
    KARAOKE_SPK_SURROUND,//全景环绕
    KARAOKE_SPK_3D,//3d环绕
    KARAOKE_SPK_FLOW_VOICE,//流动人声
    KARAOKE_SPK_KING,//王者荣耀
    KARAOKE_SPK_WAR,//刺激战场
    KARAOKE_SPK_MAX,
};

void audio_mode_main_dec_open(u32 state);

//////////////////////////////////////////////////////////////////////////////

// 音频解码初始化
int audio_dec_init();

// mix out后 做高低音
void mix_out_high_bass(u32 cmd, struct high_bass *hb);
// mix out后 是否做高低音处理
void mix_out_high_bass_dis(u32 cmd, u8 dis);
// 切换频响计算
void spectrum_switch_demo(u8 en);

//////////////////////////////////////////////////////////////////////////////
static inline void audio_pcm_mono_to_dual(s16 *dual_pcm, s16 *mono_pcm, int points)
{
    s16 *mono = mono_pcm;
    int i = 0;
    u8 j = 0;

    for (i = 0; i < points; i++, mono++) {
        *dual_pcm++ = *mono;
        *dual_pcm++ = *mono;
    }
}


//////////////////////////////////////////////////////////////////////////////

extern void *sys_digvol_group;
// 数字通道初始化
extern int sys_digvol_group_open(void);
// 数字通道关闭
extern int sys_digvol_group_close(void);
// 打开一个数字通道
extern void *sys_digvol_group_ch_open(char *logo, int vol_start, audio_dig_vol_param *parm);
// 关闭一个数字通道
extern int sys_digvol_group_ch_close(char *logo);



#include "audio_dec_bt.h"
#include "audio_dec_file.h"
#ifndef CONFIG_LITE_AUDIO
#include "audio_dec_fm.h"
#endif/*CONFIG_LITE_AUDIO*/
#include "audio_dec_linein.h"
#include "audio_dec_pc.h"
#include "audio_dec_spdif.h"
#include "audio_dec_record.h"
#include "audio_dec_tone.h"

#include "localtws/localtws.h"

#endif

