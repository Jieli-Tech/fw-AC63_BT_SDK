#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "classic/tws_api.h"
#include "classic/hci_lmp.h"
#include "effectrs_sync.h"
#include "application/audio_eq.h"
#include "application/audio_drc.h"
#include "application/audio_energy_detect.h"
#include "application/audio_surround.h"
#include "app_config.h"
#include "audio_config.h"
#include "aec_user.h"
#include "audio_enc.h"
#include "app_main.h"
#include "btstack/avctp_user.h"
#include "btstack/a2dp_media_codec.h"
#include "tone_player.h"
#include "application/audio_vbass.h"
#include "audio_iis.h"

#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
#include "audio_digital_vol.h"
#endif

#include "audio_plc.h"
#if TCFG_AUDIO_NOISE_GATE
#include "audio_noise_gate.h"
#endif/*TCFG_AUDIO_NOISE_GATE*/

#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif/*TCFG_AUDIO_ANC_ENABLE*/

#if TCFG_APP_FM_EMITTER_EN
#include "fm_emitter/fm_emitter_manage.h"
#endif

#include "phone_message/phone_message.h"

#define AUDIO_CODEC_SUPPORT_SYNC	1

#define A2DP_RX_AND_AUDIO_DELAY     1

#define A2DP_EQ_SUPPORT_ASYNC		1
#define MUSIC_EQ_SUPPORT_ASYNC		1

#ifndef CONFIG_EQ_SUPPORT_ASYNC
#undef A2DP_EQ_SUPPORT_ASYNC
#define A2DP_EQ_SUPPORT_ASYNC		0
#endif

#ifndef CONFIG_EQ_SUPPORT_ASYNC
#undef MUSIC_EQ_SUPPORT_ASYNC
#define MUSIC_EQ_SUPPORT_ASYNC	0
#endif


#if A2DP_EQ_SUPPORT_ASYNC && TCFG_BT_MUSIC_EQ_ENABLE
#if TCFG_DRC_ENABLE
#define A2DP_EQ_SUPPORT_32BIT		1
#else
#define A2DP_EQ_SUPPORT_32BIT		0
#endif
#else
#define A2DP_EQ_SUPPORT_32BIT		0
#endif

#if (defined(TCFG_AUDIO_OUT_EQ_ENABLE) && (TCFG_AUDIO_OUT_EQ_ENABLE != 0))
#define AUDIO_OUT_EFFECT_ENABLE			1	// 音频输出时的音效处理
#else
#define AUDIO_OUT_EFFECT_ENABLE			0
#endif


#if AUDIO_OUT_EFFECT_ENABLE

#define AUDIO_OUT_EQ_SUPPORT_ASYNC		1

#if	((!defined(CONFIG_EQ_SUPPORT_ASYNC)))
#undef AUDIO_OUT_EQ_SUPPORT_ASYNC
#define AUDIO_OUT_EQ_SUPPORT_ASYNC		0
#endif

#if (defined(TCFG_AUDIO_OUT_EQ_ENABLE) && (TCFG_AUDIO_OUT_EQ_ENABLE != 0))
#define AUDIO_OUT_EQ_USE_SPEC_NUM		2	// 使用特定的eq段
#else
#define AUDIO_OUT_EQ_USE_SPEC_NUM		0
#endif

struct audio_out_effect_hdl {
#if (defined(TCFG_AUDIO_OUT_EQ_ENABLE) && (TCFG_AUDIO_OUT_EQ_ENABLE != 0))
    struct audio_eq *eq;
#endif
#if AUDIO_OUT_EQ_USE_SPEC_NUM
    int EQ_Coeff_table[AUDIO_OUT_EQ_USE_SPEC_NUM * 5];
#endif
#if AUDIO_OUT_EQ_SUPPORT_ASYNC
    u8 eq_async_remain;
#endif
    u8 remain;
};
struct audio_out_effect_hdl *audio_out_effect = NULL;

#endif /*AUDIO_OUT_EFFECT_ENABLE*/
static u8 audio_out_effect_dis = 0;


struct tone_dec_hdl {
    struct audio_decoder decoder;
    void (*handler)(void *, int argc, int *argv);
    void *priv;
};

struct a2dp_dec_hdl {
    struct audio_decoder decoder;
    struct audio_res_wait wait;
    struct audio_mixer_ch mix_ch;
    enum audio_channel channel;
    u8 start;
    u8 ch;
    s16 header_len;
    u8 remain;
    u8 eq_remain;
    u8 fetch_lock;
#if AUDIO_CODEC_SUPPORT_SYNC
    u8 sync_step;
    u8 preempt_state;
    u16 drop_points;
    u16 droped_points;
#if TCFG_USER_TWS_ENABLE
    u32 wait_time;
#endif
#endif
    u16 seqn;
    u16 sample_rate;
    int timer;
    u32 coding_type;

#if A2DP_EQ_SUPPORT_32BIT
    s16 *eq_out_buf;
    int eq_out_buf_len;
    int eq_out_points;
    int eq_out_total;
#endif
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    dvol_handle *dvol;
#endif

#if AUDIO_SURROUND_CONFIG
    surround_hdl *surround;         //环绕音效句柄
#endif

#if AUDIO_VBASS_CONFIG
    vbass_hdl *vbass;               //虚拟低音句柄
#endif
};

#define AUDIO_VBASS_TEST 0

#if AUDIO_VBASS_TEST
u16 vbass_t = 0;
extern void vbass_udate_parm_test(void *p);
#endif

struct esco_dec_hdl {
    struct audio_decoder decoder;
    struct audio_res_wait wait;
    struct audio_mixer_ch mix_ch;
    u32 coding_type;
    u8 *frame;
    u8 frame_len;
    u8 offset;
    u8 data_len;
    u8 tws_mute_en;
    u8 start;
    u8 enc_start;
#if AUDIO_CODEC_SUPPORT_SYNC
    u8 sync_step;
    u8 preempt_state;
#endif
    u8 esco_len;
    u16 remain;
    int data[15];/*ALIGNED(4)*/
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    dvol_handle *dvol;
#endif
};

#if TCFG_BT_MUSIC_EQ_ENABLE
struct audio_eq *a2dp_eq = NULL;
#endif

#if TCFG_PHONE_EQ_ENABLE
struct audio_eq *esco_eq = NULL;
#endif

#if AUDIO_OUTPUT_AUTOMUTE
void *mix_out_automute_hdl = NULL;
extern void mix_out_automute_open();
extern void mix_out_automute_close();
#endif  //#if AUDIO_OUTPUT_AUTOMUTE

#if TCFG_BT_MUSIC_DRC_ENABLE
struct audio_drc *a2dp_drc = NULL;
#endif
void *audio_sync = NULL;

#define MIX_POINTS_NUM  128
#if A2DP_RX_AND_AUDIO_DELAY
#define AUDIO_DAC_DELAY_TIME    30
#else
#define AUDIO_DAC_DELAY_TIME    50
#endif

s16 dac_buff[4 * 1024];
#if AUDIO_CODEC_SUPPORT_SYNC
#define DEC_RUN_BY_ITSELF               0
#define DEC_PREEMTED_BY_PRIORITY        1
#define DEC_PREEMTED_BY_OUTSIDE         3
#define DEC_RESUME_BY_OUTSIDE           2
#if TCFG_BT_SUPPORT_AAC
/*AAC解码需要加大同步和MIX的buffer用来提高异步效率*/
s16 dac_sync_buff[128];
#undef MIX_POINTS_NUM
#define MIX_POINTS_NUM  128
#else
s16 dac_sync_buff[128];
#endif
s16 dac_sync_filt[24 * 2];
#endif
s16 mix_buff[MIX_POINTS_NUM];

#define MAX_SRC_NUMBER      3
s16 audio_src_hw_filt[24 * 2 * MAX_SRC_NUMBER];

/*OS_SEM dac_sem;*/
static u16 dac_idle_delay_max = 10000;
static u16 dac_idle_delay_cnt = 10000;
static struct tone_dec_hdl *tone_dec = NULL;
struct audio_decoder_task decode_task;
struct audio_dac_hdl dac_hdl;
struct audio_mixer mixer;
extern struct dac_platform_data dac_data;
extern struct audio_adc_hdl adc_hdl;

static u16 a2dp_delay_time;
static u8 a2dp_low_latency = 0;
static u16 drop_a2dp_timer;
static u16 a2dp_low_latency_seqn  = 0;
extern const int CONFIG_LOW_LATENCY_ENABLE;
extern const int CONFIG_A2DP_DELAY_TIME;
extern const int CONFIG_A2DP_DELAY_TIME_LO;
extern const int CONFIG_A2DP_SBC_DELAY_TIME_LO;


struct a2dp_dec_hdl *a2dp_dec = NULL;
struct esco_dec_hdl *esco_dec = NULL;

extern int pcm_decoder_enable();
extern int cvsd_decoder_init();
extern int msbc_decoder_init();
extern int g729_decoder_init();
extern int sbc_decoder_init();
extern int mp3_decoder_init();
extern int wma_decoder_init();
extern int wav_decoder_init();
extern int mty_decoder_init();
extern int flac_decoder_init();
extern int ape_decoder_init();
extern int m4a_decoder_init();
extern int amr_decoder_init();
extern int dts_decoder_init();
extern int mp3pick_decoder_init();
extern int wmapick_decoder_init();
extern int aac_decoder_init();

extern int platform_device_sbc_init();

int lmp_private_get_esco_remain_buffer_size();
int lmp_private_get_esco_data_len();
void *lmp_private_get_esco_packet(int *len, u32 *hash);
void lmp_private_free_esco_packet(void *packet);
extern int lmp_private_esco_suspend_resume(int flag);


void *a2dp_play_sync_open(u8 channel, u32 sample_rate, u32 output_rate, u32 coding_type);

void *esco_play_sync_open(u8 channel, u16 sample_rate, u16 output_rate);
//void audio_adc_init(void);
void *audio_adc_open(void *param, const char *source);
int audio_adc_sample_start(void *adc);
void audio_fade_in_fade_out(u8 left_gain, u8 right_gain);

void set_source_sample_rate(u16 sample_rate);
u8 bt_audio_is_running(void);
#if AUDIO_SURROUND_CONFIG
void surround_switch_test(void *p);
#endif
void audio_resume_all_decoder(void)
{
    audio_decoder_resume_all(&decode_task);
}

void audio_src_isr_deal(void)
{
    audio_resume_all_decoder();
}

#define AUDIO_DECODE_TASK_WAKEUP_TIME	4	//ms
#if AUDIO_DECODE_TASK_WAKEUP_TIME
#include "timer.h"
static void audio_decoder_wakeup_timer(void *priv)
{
    //putchar('k');
    audio_decoder_resume_all(&decode_task);
}
int audio_decoder_task_add_probe(struct audio_decoder_task *task)
{
    local_irq_disable();
    if (task->wakeup_timer == 0) {
        task->wakeup_timer = usr_timer_add(NULL, audio_decoder_wakeup_timer, AUDIO_DECODE_TASK_WAKEUP_TIME, 1);
    }
    local_irq_enable();
    log_i("audio_decoder_task_add_probe:%d\n", task->wakeup_timer);
    return 0;
}
int audio_decoder_task_del_probe(struct audio_decoder_task *task)
{
    log_i("audio_decoder_task_del_probe\n");
    if (audio_decoder_task_wait_state(task) > 0) {
        /*解码任务列表还有任务*/
        return 0;
    }

    local_irq_disable();
    if (task->wakeup_timer) {
        log_i("audio_decoder_task_del_probe:%d\n", task->wakeup_timer);
        usr_timer_del(task->wakeup_timer);
        task->wakeup_timer = 0;
    }
    local_irq_enable();
    return 0;
}

int audio_decoder_wakeup_modify(int msecs)
{
    if (decode_task.wakeup_timer) {
        usr_timer_modify(decode_task.wakeup_timer, msecs);
    }

    return 0;
}

/*
 * BR30 : DA输出源启动后可使用DAC irq做唤醒，省去hi_timer
 */
int audio_decoder_wakeup_select(u8 source)
{
    if (source == 0) {
        /*唤醒源为hi_timer*/
        local_irq_disable();
        if (!decode_task.wakeup_timer) {
            decode_task.wakeup_timer = usr_timer_add(NULL, audio_decoder_wakeup_timer, AUDIO_DECODE_TASK_WAKEUP_TIME, 1);
        }
        local_irq_enable();
    } else if (source == 1) {
        /*唤醒源为输出目标中断*/
        if (!audio_dac_is_working(&dac_hdl)) {
            return audio_decoder_wakeup_select(0);
        }

        /*int err = audio_dac_set_irq_time(&dac_hdl, a2dp_low_latency ? 2 : AUDIO_DECODE_TASK_WAKEUP_TIME);*/
        int err = audio_dac_irq_enable(&dac_hdl, a2dp_low_latency ? 2 : AUDIO_DECODE_TASK_WAKEUP_TIME,
                                       NULL, audio_decoder_wakeup_timer);
        if (err) {
            return audio_decoder_wakeup_select(0);
        }

        local_irq_disable();
        if (decode_task.wakeup_timer) {
            usr_timer_del(decode_task.wakeup_timer);
            decode_task.wakeup_timer = 0;
        }
        local_irq_enable();
    }
    return 0;
}

#endif/*AUDIO_DECODE_TASK_WAKEUP_TIME*/

static u8 bt_dec_idle_query()
{
    if (a2dp_dec || esco_dec) {
        return 0;
    }

    return 1;
}
REGISTER_LP_TARGET(bt_dec_lp_target) = {
    .name = "bt_dec",
    .is_idle = bt_dec_idle_query,
};


___interrupt
static void audio_irq_handler()
{
    /* putchar('A'); */
    if (JL_AUDIO->DAC_CON & BIT(7)) {
        JL_AUDIO->DAC_CON |= BIT(6);
        if (JL_AUDIO->DAC_CON & BIT(5)) {
            audio_decoder_resume_all(&decode_task);
            audio_dac_irq_handler(&dac_hdl);
            /*r_printf("resuem\n");*/
        }
    }

    if (JL_AUDIO->ADC_CON & BIT(7)) {
        JL_AUDIO->ADC_CON |= BIT(6);
        if ((JL_AUDIO->ADC_CON & BIT(5)) && (JL_AUDIO->ADC_CON & BIT(4))) {
            audio_adc_irq_handler(&adc_hdl);
        }
    }
}

#if A2DP_EQ_SUPPORT_32BIT
void a2dp_eq_32bit_out(struct a2dp_dec_hdl *dec)
{
    int wlen = 0;
    wlen = audio_mixer_ch_write(&dec->mix_ch, &dec->eq_out_buf[dec->eq_out_points], (dec->eq_out_total - dec->eq_out_points) * 2);
    dec->eq_out_points += wlen / 2;
}
#endif /*A2DP_EQ_SUPPORT_32BIT*/

#if TCFG_BT_MUSIC_EQ_ENABLE
static int a2dp_eq_output(void *priv, s16 *buf, u32 len)
{
    int wlen = 0;
    int rlen = len;
    s16 *data = buf;
    struct a2dp_dec_hdl *dec = priv;

#if A2DP_EQ_SUPPORT_ASYNC
    if (!dec->eq_remain) {

#if A2DP_EQ_SUPPORT_32BIT
        if (dec->eq_out_buf && (dec->eq_out_points < dec->eq_out_total)) {
            a2dp_eq_32bit_out(dec);
            if (dec->eq_out_points < dec->eq_out_total) {
                return 0;
            }
        }
#endif /*A2DP_EQ_SUPPORT_32BIT*/

#if TCFG_BT_MUSIC_DRC_ENABLE

        if (a2dp_drc) {
            audio_drc_run(a2dp_drc, data, len);
        }
#endif

#if A2DP_EQ_SUPPORT_32BIT
        if ((!dec->eq_out_buf) || (dec->eq_out_buf_len < len / 2)) {
            if (dec->eq_out_buf) {
                free(dec->eq_out_buf);
            }
            dec->eq_out_buf_len = len / 2;
            dec->eq_out_buf = malloc(dec->eq_out_buf_len);
            ASSERT(dec->eq_out_buf);
        }
        s32 *idat = data;
        s16 *odat = dec->eq_out_buf;
        for (int i = 0; i < len / 4; i++) {
            s32 outdat = *idat++;
            if (outdat > 32767) {
                outdat = 32767;
            } else if (outdat < -32768) {
                outdat = -32768;
            }
            *odat++ = outdat;
        }
        dec->eq_out_points = 0;
        dec->eq_out_total = len / 4;

        a2dp_eq_32bit_out(dec);
        return len;
#endif /*A2DP_EQ_SUPPORT_32BIT*/
    }

    /* printf("dec:0x%x, a2dp:0x%x ", dec, a2dp_dec); */
    /* printf("data:0x%x, len:%d ", data, len); */
#if 0
    do {
        /*wlen = audio_dac_write(&dac_hdl, data, len);*/
        wlen = audio_mixer_ch_write(&dec->mix_ch, data, len);
        if (!wlen) {
            break;
        }

        data += (wlen >> 1);
        len -= wlen;
    } while (len);
    return rlen - len;
#else
    return audio_mixer_ch_write(&dec->mix_ch, data, len);
#endif
#endif
    return rlen;
}
#endif


#if AUDIO_OUTPUT_AUTOMUTE

void audio_mix_out_automute_mute(u8 mute)
{
    printf(">>>>>>>>>>>>>>>>>>>> %s\n", mute ? ("MUTE") : ("UNMUTE"));
}

/* #define AUDIO_E_DET_UNMUTE      (0x00) */
/* #define AUDIO_E_DET_MUTE        (0x01) */
void mix_out_automute_handler(u8 event, u8 ch)
{
    printf(">>>> ch:%d %s\n", ch, event ? ("MUTE") : ("UNMUTE"));
    if (ch == dac_hdl.channel) {
        audio_mix_out_automute_mute(event);
    }
}

void mix_out_automute_skip(u8 skip)
{
    u8 mute = !skip;
    if (mix_out_automute_hdl) {
        audio_energy_detect_skip(mix_out_automute_hdl, 0xFFFF, skip);
        audio_mix_out_automute_mute(mute);
    }
}

void mix_out_automute_open()
{
    if (mix_out_automute_hdl) {
        printf("mix_out_automute is already open !\n");
        return;
    }
    audio_energy_detect_param e_det_param = {0};
    e_det_param.mute_energy = 5;
    e_det_param.unmute_energy = 10;
    e_det_param.mute_time_ms = 1000;
    e_det_param.unmute_time_ms = 50;
    e_det_param.count_cycle_ms = 10;
    e_det_param.sample_rate = 44100;
    e_det_param.event_handler = mix_out_automute_handler;
    e_det_param.ch_total = dac_hdl.channel;
    e_det_param.dcc = 1;
    mix_out_automute_hdl = audio_energy_detect_open(&e_det_param);
}

void mix_out_automute_close()
{
    if (mix_out_automute_hdl) {
        audio_energy_detect_close(mix_out_automute_hdl);
    }
}
#endif  //#if AUDIO_OUTPUT_AUTOMUTE

#if (defined(TCFG_AUDIO_OUT_EQ_ENABLE) && (TCFG_AUDIO_OUT_EQ_ENABLE != 0))

#if AUDIO_OUT_EQ_USE_SPEC_NUM

static struct eq_seg_info audio_out_eq_tab[AUDIO_OUT_EQ_USE_SPEC_NUM] = {
    {0, EQ_IIR_TYPE_BAND_PASS, 125,   0 << 20, (int)(0.7f * (1 << 24))},
    {1, EQ_IIR_TYPE_BAND_PASS, 12000, 0 << 20, (int)(0.3f * (1 << 24))},
};
#define AUDIO_EQ_FADE_EN  1
#if AUDIO_EQ_FADE_EN
typedef struct {
    u16 tmr;
    int cur_gain[AUDIO_OUT_EQ_USE_SPEC_NUM];
    int use_gain[AUDIO_OUT_EQ_USE_SPEC_NUM];
} _EQ_FADE_CFG;
static _EQ_FADE_CFG eq_fade_cfg = {0};
#endif





static int audio_out_eq_get_filter_info(int sr, struct audio_eq_filter_info *info)
{
    if (!audio_out_effect) {
        return -1;
    }
    local_irq_disable();
    for (int i = 0; i < AUDIO_OUT_EQ_USE_SPEC_NUM; i++) {
        eq_seg_design(&audio_out_eq_tab[i], sr, &audio_out_effect->EQ_Coeff_table[5 * i]);
    }
    local_irq_enable();

    info->L_coeff = info->R_coeff = (void *)audio_out_effect->EQ_Coeff_table;
    info->L_gain = info->R_gain = 0;
    info->nsection = AUDIO_OUT_EQ_USE_SPEC_NUM;
    return 0;
}
int audio_out_eq_spec_set_info(u8 idx, int freq, int gain)
{
    if (idx >= AUDIO_OUT_EQ_USE_SPEC_NUM) {
        return false;
    }
    if (gain > 12) {
        gain = 12;
    }
    if (gain < -12) {
        gain = -12;
    }
    if (freq) {
        audio_out_eq_tab[idx].freq = freq;
    }
    audio_out_eq_tab[idx].gain = gain << 20;

    /* printf("audio out eq, idx:%d, freq:%d,%d, gain:%d,%d \n", idx, freq, audio_out_eq_tab[idx].freq, gain, audio_out_eq_tab[idx].gain); */

#if !AUDIO_EQ_FADE_EN
    local_irq_disable();
    if (audio_out_effect && audio_out_effect->eq) {
        audio_out_effect->eq->updata = 1;
    }
    local_irq_enable();
#endif

    return true;
}

#if AUDIO_EQ_FADE_EN

int eq_fade_run(void *p)
{
    _EQ_FADE_CFG *fade_cfg = (_EQ_FADE_CFG *)p;

    u8 design = 0;
    if (fade_cfg->cur_gain[0] > fade_cfg->use_gain[0]) {
        fade_cfg->cur_gain[0] -= 1;
        if (fade_cfg->cur_gain[0] < fade_cfg->use_gain[0]) {
            fade_cfg->cur_gain[0] = fade_cfg->use_gain[0];
        }
        design = 1;
    } else if (fade_cfg->cur_gain[0] < fade_cfg->use_gain[0]) {
        fade_cfg->cur_gain[0] += 1;
        if (fade_cfg->cur_gain[0] > fade_cfg->use_gain[0]) {
            fade_cfg->cur_gain[0] = fade_cfg->use_gain[0];
        }
        design = 1;
    }

    if (design) {
        design = 0;
        audio_out_eq_spec_set_info(0, 0, fade_cfg->cur_gain[0]);//低音
        local_irq_disable();
        if (audio_out_effect && audio_out_effect->eq) {
            audio_out_effect->eq->updata = 1;
        }
        local_irq_enable();
    }

    if (fade_cfg->cur_gain[1] > fade_cfg->use_gain[1]) {
        fade_cfg->cur_gain[1] -= 1;
        if (fade_cfg->cur_gain[1] < fade_cfg->use_gain[1]) {
            fade_cfg->cur_gain[1] = fade_cfg->use_gain[1];
        }
        design = 1;
    } else if (fade_cfg->cur_gain[1] < fade_cfg->use_gain[1]) {
        fade_cfg->cur_gain[1] += 1;
        if (fade_cfg->cur_gain[1] > fade_cfg->use_gain[1]) {
            fade_cfg->cur_gain[1] = fade_cfg->use_gain[1];
        }
        design = 1;
    }

    if (design) {
        design = 0;
        audio_out_eq_spec_set_info(1, 0, fade_cfg->cur_gain[1]);//高音

        local_irq_disable();
        if (audio_out_effect && audio_out_effect->eq) {
            audio_out_effect->eq->updata = 1;
        }
        local_irq_enable();
    }

    return 0;
}



#endif


int audio_out_eq_spec_set_gain(u8 idx, int gain)
{
    if (!idx) {
        idx = 0;
    } else {
        idx = 1;
    }

#if AUDIO_EQ_FADE_EN
    eq_fade_cfg.use_gain[idx] = gain;
    if (eq_fade_cfg.tmr == 0) {
        eq_fade_cfg.tmr = sys_hi_timer_add(&eq_fade_cfg, eq_fade_run, 20);
    }
#else
    if (!idx) {
        audio_out_eq_spec_set_info(0, 0, gain);//低音
    } else {
        audio_out_eq_spec_set_info(1, 0, gain);//高音
    }
#endif

    return true;
}

#endif /*AUDIO_OUT_EQ_USE_SPEC_NUM*/


static int mix_output_handler(struct audio_mixer *mixer, s16 *data, u16 len);
static int audio_out_eq_output(void *priv, s16 *data, u32 len)
{
#if AUDIO_OUT_EQ_SUPPORT_ASYNC
    if (!audio_out_effect->eq_async_remain) {
    }
    int wlen = mix_output_handler(priv, data, len);
    if (wlen == len) {
        audio_out_effect->eq_async_remain = 0;
    } else {
        audio_out_effect->eq_async_remain = 1;
    }
    return wlen;
#endif
    return len;
}


static void audio_out_effect_close(void)
{
    if (!audio_out_effect) {
        return ;
    }
#if (defined(TCFG_AUDIO_OUT_EQ_ENABLE) && (TCFG_AUDIO_OUT_EQ_ENABLE != 0))

#if AUDIO_EQ_FADE_EN
    if (eq_fade_cfg.tmr) {
        sys_hi_timer_del(eq_fade_cfg.tmr);
        eq_fade_cfg.tmr = 0;
    }
#endif
    if (audio_out_effect->eq) {
        audio_eq_close(audio_out_effect->eq);
        local_irq_disable();
        free(audio_out_effect->eq);
        audio_out_effect->eq = NULL;
        local_irq_enable();
    }
#endif
    local_irq_disable();
    free(audio_out_effect);
    audio_out_effect = NULL;
    local_irq_enable();
}

static int audio_out_effect_stream_clear()
{
    if (!audio_out_effect) {
        return 0;
    }

    if (audio_out_effect->eq) {
#if AUDIO_OUT_EQ_SUPPORT_ASYNC
        audio_eq_async_data_clear(audio_out_effect->eq);
#endif
    }

    return 0;
}

static int audio_out_effect_open(void *priv, u16 sample_rate, u8 ch_num)
{
    audio_out_effect_close();

    struct audio_out_effect_hdl *hdl = zalloc(sizeof(struct audio_out_effect_hdl));
    if (!hdl) {
        return false;
    }
#if (defined(TCFG_AUDIO_OUT_EQ_ENABLE) && (TCFG_AUDIO_OUT_EQ_ENABLE != 0))
    hdl->eq = zalloc(sizeof(struct audio_eq) + sizeof(struct hw_eq_ch));
    if (hdl->eq) {
        hdl->eq->eq_ch = (struct hw_eq_ch *)((int)hdl->eq + sizeof(struct audio_eq));
        struct audio_eq_param eq_param = {0};
        eq_param.channels = ch_num;
#if AUDIO_OUT_EQ_USE_SPEC_NUM
        eq_param.max_nsection = AUDIO_OUT_EQ_USE_SPEC_NUM;
        eq_param.cb = audio_out_eq_get_filter_info;
#else
        eq_param.online_en = 1;
        eq_param.mode_en = 1;
        eq_param.remain_en = 1;
        eq_param.max_nsection = EQ_SECTION_MAX;
        eq_param.cb = eq_get_filter_info;
#endif

#if AUDIO_OUT_EQ_SUPPORT_ASYNC
        eq_param.no_wait = 1;//异步
#endif
        eq_param.eq_name = 0;
        audio_eq_open(hdl->eq, &eq_param);
        audio_eq_set_samplerate(hdl->eq, sample_rate);
        audio_eq_set_output_handle(hdl->eq, audio_out_eq_output, priv);
        audio_eq_start(hdl->eq);
    }
#endif
    audio_out_effect = hdl;
    return true;
}


static int mix_output_effect_handler(struct audio_mixer *mixer, s16 *data, u16 len)
{
    if (!audio_out_effect) {
        return mix_output_handler(mixer, data, len);
    }
#if AUDIO_OUT_EQ_SUPPORT_ASYNC
    if (audio_out_effect->eq) {
        return audio_eq_run(audio_out_effect->eq, data, len);
    }
#endif /*AUDIO_OUT_EQ_SUPPORT_ASYNC*/


    if (!audio_out_effect->remain) {
#if (defined(TCFG_AUDIO_OUT_EQ_ENABLE) && (TCFG_AUDIO_OUT_EQ_ENABLE != 0))
        if (audio_out_effect->eq) {
            audio_eq_run(audio_out_effect->eq, data, len);
        }
#endif
    }
    int wlen = mix_output_handler(mixer, data, len);
    if (wlen == len) {
        audio_out_effect->remain = 0;
    } else {
        audio_out_effect->remain = 1;
    }
    return wlen;
}
#endif

static void mixer_event_handler(struct audio_mixer *mixer, int event)
{
    switch (event) {
    case MIXER_EVENT_CH_OPEN:
        if (audio_mixer_get_start_ch_num(mixer) == 1) {
#if AUDIO_OUTPUT_AUTOMUTE
            audio_energy_detect_sample_rate_update(mix_out_automute_hdl, audio_mixer_get_sample_rate(mixer));
#endif  //#if AUDIO_OUTPUT_AUTOMUTE

#if TCFG_AUDIO_OUTPUT_IIS
            audio_iis_output_set_srI(audio_mixer_get_sample_rate(mixer));
            audio_iis_output_start(1);
#elif TCFG_APP_FM_EMITTER_EN

#else
            audio_dac_set_sample_rate(&dac_hdl, audio_mixer_get_sample_rate(mixer));
            audio_dac_set_volume(&dac_hdl, app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
            audio_dac_start(&dac_hdl);
            audio_dac_set_input_correct_callback(&dac_hdl,
                                                 (void *)mixer,
                                                 (void (*)(void *, int))audio_mixer_position_correct);
#endif
#if AUDIO_OUT_EFFECT_ENABLE
#if TCFG_AUDIO_OUTPUT_IIS
            u8 ch_num = 2;
#elif TCFG_APP_FM_EMITTER_EN
            u8 ch_num = 2;
#else
            u8 ch_num = AUDIO_CH_LR ? 2 : 1;
#endif

            if (!audio_out_effect_dis) {
                audio_out_effect_open(mixer, audio_mixer_get_sample_rate(mixer), ch_num);
            }
#endif

        }
        break;
    case MIXER_EVENT_CH_CLOSE:
        if (audio_mixer_get_ch_num(mixer) == 0) {
            audio_dac_set_input_correct_callback(&dac_hdl, NULL, NULL);
#if AUDIO_OUT_EFFECT_ENABLE
            audio_out_effect_close();
#endif

#if TCFG_AUDIO_OUTPUT_IIS
            audio_iis_output_stop();
#elif TCFG_APP_FM_EMITTER_EN

#else
            audio_dac_stop(&dac_hdl);
#endif
        }
        break;
    }
}

static int mix_probe_handler(struct audio_mixer *mixer)
{
    return 0;
}

static int mix_output_handler(struct audio_mixer *mixer, s16 *data, u16 len)
{
    int rlen = len;
    int wlen = 0;

    /*os_sem_set(&dac_sem, 0);*/
    /* audio_aec_refbuf(data, len); */

#if AUDIO_OUTPUT_AUTOMUTE
    audio_energy_detect_run(mix_out_automute_hdl, data, len);
#endif  //#if AUDIO_OUTPUT_AUTOMUTE

#if TCFG_AUDIO_OUTPUT_IIS
    wlen = audio_iis_output_write(data, len);
    if (wlen == len) {
        audio_decoder_resume_all(&decode_task);
    }
#elif TCFG_APP_FM_EMITTER_EN
    fm_emitter_cbuf_write((u8 *)data, len);
#else
    /*
    do {
        wlen = audio_dac_write(&dac_hdl, data, len);
        if (wlen < len) {
            int err = os_sem_pend(&dac_sem, 6);
            if (err) {
                break;
            }
        }
        len -= wlen;
        data += wlen / 2;
    } while (len);
    */
    wlen = audio_dac_write(&dac_hdl, data, len);
    if (wlen == len) {
        audio_decoder_resume_all(&decode_task);
    }
#endif

    return wlen;
}

static const struct audio_mix_handler mix_handler  = {
    .mix_probe  = mix_probe_handler,

#if AUDIO_OUT_EFFECT_ENABLE
    .mix_output = mix_output_effect_handler,
#else
    .mix_output = mix_output_handler,
#endif
};

static void bt_audio_mixer_ch_event_handler(void *priv, int event)
{
    struct a2dp_dec_hdl *dec = (struct a2dp_dec_hdl *)priv;

    switch (event) {
    case MIXER_EVENT_CH_OPEN:
#if AUDIO_CODEC_SUPPORT_SYNC
        audio_mixer_position_correct(&mixer, dec->droped_points);
        if (audio_sync) {
            audio_dac_sync_start(&dac_hdl);
        }
#endif
        break;
    case MIXER_EVENT_CH_CLOSE:
    case MIXER_EVENT_CH_RESET:
#if AUDIO_CODEC_SUPPORT_SYNC
        if (audio_sync) {
            audio_dac_sync_stop(&dac_hdl);
        }
#endif
        break;
    default:
        break;
    }
}

#define RB16(b)    (u16)(((u8 *)b)[0] << 8 | (((u8 *)b))[1])

static int get_rtp_header_len(u8 new_frame, u8 *buf, int len)
{
    int ext, csrc;
    int byte_len;
    int header_len = 0;
    u8 *data = buf;

    csrc = buf[0] & 0x0f;
    ext  = buf[0] & 0x10;

    byte_len = 12 + 4 * csrc;
    buf += byte_len;

    if (ext) {
        ext = (RB16(buf + 2) + 1) << 2;
    }

    if (new_frame) {
        header_len = byte_len + ext + 1;
    } else {
        header_len = byte_len + ext;
    }
    if (a2dp_dec->coding_type == AUDIO_CODING_SBC) {
        while (data[header_len] != 0x9c) {
            if (++header_len > len) {
                return len;
            }
        }
    }

    return header_len;
}

__attribute__((weak))
int audio_dac_get_channel(struct audio_dac_hdl *p)
{
    return 0;
}

void __a2dp_drop_frame(void *p)
{
    int len;
    u8 *frame;

    int num = a2dp_media_get_packet_num();
    if (num > 1) {
        for (int i = 0; i < num; i++) {
            len = a2dp_media_get_packet(&frame);
            if (len <= 0) {
                break;
            }
            //printf("a2dp_drop_frame: %d\n", len);
            a2dp_media_free_packet(frame);
        }
    }
}

static void __a2dp_clean_frame_by_number(struct a2dp_dec_hdl *dec, u16 num)
{
    u16 end_seqn = dec->seqn + num;
    if (end_seqn == 0) {
        end_seqn++;
    }
    /*__a2dp_drop_frame(NULL);*/
    /*dec->drop_seqn = end_seqn;*/
    a2dp_media_clear_packet_before_seqn(end_seqn);
}

static void a2dp_tws_clean_frame(void *arg)
{
    u8 master = 0;
#if TCFG_USER_TWS_ENABLE
    if (tws_api_get_role() == TWS_ROLE_MASTER) {
        master = 1;
    }
#else
    master = 1;
#endif
    if (!master) {
        return;
    }

    int msecs = a2dp_media_get_remain_play_time(0);
    if (msecs <= 0) {
        return;
    }

    if (a2dp_dec && a2dp_dec->fetch_lock) {
        return;
    }
    int len = 0;
    u16 seqn = 0;
    u8 *packet = a2dp_media_fetch_packet(&len, NULL);
    if (!packet) {
        return;
    }
    seqn = RB16(packet + 2) + 10;
    if (seqn == 0) {
        seqn = 1;
    }
    a2dp_media_clear_packet_before_seqn(seqn);
}

static u8 a2dp_suspend = 0;
static u32 a2dp_resume_time = 0;



int a2dp_decoder_pause(void)
{
    if (a2dp_dec) {
        return audio_decoder_pause(&(a2dp_dec->decoder));
    }

    return 0;
}

int a2dp_decoder_start(void)
{
    if (a2dp_dec) {
        return audio_decoder_start(&(a2dp_dec->decoder));
    }

    return 0;
}

void a2dp_drop_frame_start()
{
    if (a2dp_dec && (a2dp_dec->timer == 0)) {
        a2dp_dec->timer = sys_timer_add(NULL, __a2dp_drop_frame, 50);
    }
}

void a2dp_drop_frame_stop()
{
    if (a2dp_dec && a2dp_dec->timer) {
        sys_timer_del(a2dp_dec->timer);
        a2dp_dec->timer = 0;
    }
}

static void a2dp_dec_set_output_channel(struct a2dp_dec_hdl *dec)
{
    int state = 0;
    enum audio_channel channel;
    u8 dac_connect_mode = 0;

#if TCFG_USER_TWS_ENABLE
    state = tws_api_get_tws_state();
    if (state & TWS_STA_SIBLING_CONNECTED) {
        channel = tws_api_get_local_channel() == 'L' ? AUDIO_CH_L : AUDIO_CH_R;
        dec->ch = 1;
    } else {
        dac_connect_mode = audio_dac_get_channel(&dac_hdl);
        if (dac_connect_mode == DAC_OUTPUT_LR) {
            channel = AUDIO_CH_LR;
            dec->ch = 2;
        } else {
            channel = AUDIO_CH_DIFF;
            dec->ch = 1;
        }
    }

#else
    dac_connect_mode = audio_dac_get_channel(&dac_hdl);
    if (dac_connect_mode == DAC_OUTPUT_LR) {
        channel = AUDIO_CH_LR;
        dec->ch = 2;
    } else {
        channel = AUDIO_CH_DIFF;
        dec->ch = 1;
    }
#endif

#if TCFG_AUDIO_OUTPUT_IIS
    channel = AUDIO_CH_LR;
#elif TCFG_APP_FM_EMITTER_EN
    channel = AUDIO_CH_LR;
#endif
    dac_hdl.dec_channel_num = dec->ch;
    if (channel != dec->channel) {
        printf("set_channel: %d\n", channel);
        audio_decoder_set_output_channel(&dec->decoder, channel);
        dec->channel = channel;
#if TCFG_BT_MUSIC_EQ_ENABLE
        if (a2dp_eq) {
            audio_eq_set_channel(a2dp_eq, dec->ch);
        }
#endif

#if TCFG_USER_TWS_ENABLE
#if AUDIO_SURROUND_CONFIG
        u8 nch = EFFECT_CH_L;
        if (channel == AUDIO_CH_L) {
            nch = EFFECT_CH_L;
        } else if (channel == AUDIO_CH_R) {
            nch = EFFECT_CH_R;
        } else if (channel == AUDIO_CH_LR) {
            nch = 2;
        }

        if (dec->surround) {
            audio_surround_switch_nch(dec->surround, nch);
        }
#endif
#endif

    }
}

static int a2dp_dec_get_frame(struct audio_decoder *decoder, u8 **frame)
{
    struct a2dp_dec_hdl *dec = container_of(decoder, struct a2dp_dec_hdl, decoder);
    u8 *packet = NULL;
    int len = 0;

    len = a2dp_media_get_packet(&packet);
    if (len < 0) {
        putchar('X');
    }

    if (len > 0) {
        if (dec->coding_type == AUDIO_CODING_AAC && dec->seqn != RB16(packet + 2)) {
            dec->header_len = get_rtp_header_len(0, packet, len);
        } else {
            dec->header_len = get_rtp_header_len(1, packet, len);
        }
        dec->seqn = RB16(packet + 2);
        if (dec->header_len >= len) {
            a2dp_media_free_packet(packet);
            return -EFAULT;
        }

        *frame = packet + dec->header_len;
        len -= dec->header_len;
    }

    return len;
}

static void a2dp_dec_put_frame(struct audio_decoder *decoder, u8 *frame)
{
    struct a2dp_dec_hdl *dec = container_of(decoder, struct a2dp_dec_hdl, decoder);

    if (frame) {
        a2dp_media_free_packet((void *)(frame - dec->header_len));
    }
}

static int a2dp_dec_fetch_frame(struct audio_decoder *decoder, u8 **frame)
{
    struct a2dp_dec_hdl *dec = container_of(decoder, struct a2dp_dec_hdl, decoder);
    u8 *packet = NULL;
    int len = 0;
    u32 wait_timeout = 0;

    if (!dec->start) {
        wait_timeout = jiffies + msecs_to_jiffies(500);
    }

    dec->fetch_lock = 1;
__retry_fetch:
    packet = a2dp_media_fetch_packet(&len, NULL);
    if (packet) {
        dec->header_len = get_rtp_header_len(1, packet, len);
        *frame = packet + dec->header_len;
        len -= dec->header_len;
    } else if (!dec->start) {
        if (time_before(jiffies, wait_timeout)) {
            os_time_dly(1);
            goto __retry_fetch;
        }
    }

    dec->fetch_lock = 0;
    return len;
}

static const struct audio_dec_input a2dp_input = {
    .coding_type = AUDIO_CODING_SBC,
    .data_type   = AUDIO_INPUT_FRAME,
    .ops = {
        .frame = {
            .fget = a2dp_dec_get_frame,
            .fput = a2dp_dec_put_frame,
            .ffetch = a2dp_dec_fetch_frame,
        }
    }
};

static int a2dp_dec_rx_info_check(struct rt_stream_info *info)
{
    int len = 0;
    u8 fetch_cnt = 0;
    info->remain_len = a2dp_media_get_remain_buffer_size();

    if (a2dp_media_get_packet_num() < 1) {
        if (!a2dp_media_channel_exist()) {
            return 0;
        }
    }

    while (fetch_cnt++ < 5) {
        info->baddr = (void *)a2dp_media_fetch_packet_and_wait(&len, NULL, 40);
        if (info->baddr) {
            info->seqn = RB16(info->baddr + 2);
            if (a2dp_dec->sync_step) {
                if ((u16)(info->seqn - a2dp_dec->seqn) > 1) {
                    log_e("rx seqn error : %d, %d\n", a2dp_dec->seqn, info->seqn);
                    a2dp_dec->seqn = info->seqn;
                    return -EFAULT;
                }
            }
            a2dp_dec->seqn = info->seqn;
            return 0;
        }

        if (!a2dp_media_channel_exist()) {
            return 0;
        }

        if (audio_dac_is_working(&dac_hdl) && (audio_dac_data_time(&dac_hdl) < 10)) {
            log_w("wait rx packet timeout, da time : %dms\n", audio_dac_data_time(&dac_hdl));
            return -EFAULT;
        }
    }


    return -EINVAL;
}

static int a2dp_dec_stop_and_restart(struct audio_decoder *decoder, u8 lite_reset)
{
    struct a2dp_dec_hdl *dec = container_of(decoder, struct a2dp_dec_hdl, decoder);

#if AUDIO_CODEC_SUPPORT_SYNC
    if (audio_sync) {
        if (!lite_reset) {
            __a2dp_clean_frame_by_number(dec, dec->coding_type == AUDIO_CODING_AAC ? 20 : 20);
            a2dp_resume_time = jiffies + msecs_to_jiffies(80);
        }
#if TCFG_BT_MUSIC_EQ_ENABLE
        if (a2dp_eq) {
            audio_eq_async_data_clear(a2dp_eq);
        }
#endif
        dec->remain = 0;
        dec->drop_points = 0;
#if AUDIO_OUT_EFFECT_ENABLE
        audio_out_effect_stream_clear();
#endif
        audio_mixer_ch_reset(&dec->mix_ch);
        audio_wireless_sync_stop(audio_sync);
        audio_decoder_reset(decoder);
        dec->droped_points = 0;
#if TCFG_USER_TWS_ENABLE
        int state = tws_api_get_tws_state();
        if ((state & TWS_STA_SIBLING_CONNECTED)) {
            audio_dac_sound_reset(&dac_hdl, lite_reset ? 300 : 500);
        } else
#endif
        {
            audio_dac_sound_reset(&dac_hdl, 0);
        }
        dec->sync_step = 0;
    }
#endif
    return 0;
}
#define bt_time_before(t1, t2) \
         (((t1 < t2) && ((t2 - t1) & 0x7ffffff) < 0xffff) || \
          ((t1 > t2) && ((t1 - t2) & 0x7ffffff) > 0xffff))
#define bt_time_to_msecs(clk)   (((clk) * 625) / 1000)
#define msecs_to_bt_time(m)     (((m + 1)* 1000) / 625)

#if (TCFG_USER_TWS_ENABLE && AUDIO_CODEC_SUPPORT_SYNC)
static u8 a2dp_tws_align = 0;
static int a2dp_tws_align_time = 0;
static int a2dp_tws_delay = 0;
#define TWS_FUNC_ID_A2DP_DEC \
	((int)(('A' + '2' + 'D' + 'P') << (2 * 8)) | \
	 (int)(('D' + 'E' + 'C') << (1 * 8)) | \
	 (int)(('S' + 'Y' + 'N' + 'C') << (0 * 8)))

static void tws_a2dp_dec_align_time(void *data, u16 len, bool rx)
{
    int time;
    int expect_time;
    int rx_time;

    memcpy(&time, data, sizeof(time));
    /*rx_time = time;*/
    local_irq_disable();
    expect_time = bt_tws_future_slot_time(50); //期望tws同时的时间在当前收到之后50ms
    if (bt_time_before(time, expect_time)) {
        a2dp_tws_delay = (((expect_time - time) & 0x7ffffff) + 1) / 2 * 2;
        time = (time + a2dp_tws_delay) & 0x7ffffff;
    } else {
        a2dp_tws_delay = 0;
    }
    a2dp_tws_align = 1;
    a2dp_tws_align_time = time;
    local_irq_enable();
    /*printf("tws a2dp align rx : %d, %d\n", rx_time, time);*/
}

REGISTER_TWS_FUNC_STUB(a2dp_dec_align_time) = {
    .func_id = TWS_FUNC_ID_A2DP_DEC,
    .func    = tws_a2dp_dec_align_time,
};
#endif

static int a2dp_dec_prepare_to_start(struct audio_decoder *decoder, int msecs, int rx_remain)
{
    struct a2dp_dec_hdl *dec = container_of(decoder, struct a2dp_dec_hdl, decoder);

#if TCFG_USER_TWS_ENABLE
    if (!dec->sync_step) {
        if (!msecs) {
            return -EAGAIN;
        }
        int state = tws_api_get_tws_state();
        int distance_time = a2dp_delay_time - msecs;
        if (distance_time < 0) {
            if (state & TWS_STA_SIBLING_CONNECTED) {
                log_w("a2dp tws dec align, distance : %dms, confirm : %d\n", distance_time, a2dp_tws_align);
            } else {
                log_d("reach to time : %dms\n", msecs);
            }
            dec->sync_step = 2;
            return 0;
        }

        local_irq_disable();
        if (a2dp_tws_align &&
            (bt_time_before(a2dp_tws_align_time, bt_tws_future_slot_time(0)) ||
             bt_time_to_msecs(__builtin_abs(a2dp_tws_align_time - (int)bt_tws_future_slot_time(0))) > (a2dp_delay_time * 2))) {
            /*printf("bt time before : %d, %d\n", a2dp_tws_align_time, bt_tws_future_slot_time(0));*/
            a2dp_tws_align = 0;
        }
        local_irq_enable();

        if (state & TWS_STA_SIBLING_CONNECTED) {
            if (tws_api_get_role() == TWS_ROLE_SLAVE || dec->preempt_state == DEC_PREEMTED_BY_PRIORITY) {
                int time;
#if A2DP_RX_AND_AUDIO_DELAY
                time = bt_tws_future_slot_time(distance_time + 10);
#else
                time = bt_tws_future_slot_time(distance_time + AUDIO_DAC_DELAY_TIME);
#endif
                tws_api_send_data_to_sibling((u8 *)&time, sizeof(time), TWS_FUNC_ID_A2DP_DEC);
                /*printf("confirm delay time : %dms\n", distance_time);*/
            }
            dec->sync_step = 1;
            dec->wait_time = jiffies + msecs_to_jiffies(a2dp_delay_time + AUDIO_DAC_DELAY_TIME);
            audio_decoder_wakeup_modify(2);
            return -EAGAIN;
        }
        if (msecs < a2dp_delay_time && rx_remain > 768) {
            return -EAGAIN;
        }
        dec->sync_step = 2;
    } else if (dec->sync_step == 1) {
        if (!msecs) {
            dec->sync_step = 0;
            return -EAGAIN;
        }

        if (a2dp_tws_align) {
            log_d("a2dp tws together time : %d, %d\n", a2dp_tws_align_time, bt_tws_future_slot_time(0));
            if (bt_time_to_msecs(__builtin_abs(a2dp_tws_align_time - (int)bt_tws_future_slot_time(0))) > a2dp_delay_time) {
                log_e("tws together time error\n");
                a2dp_tws_align_time = bt_tws_future_slot_time(0) + msecs_to_bt_time(a2dp_delay_time - msecs);
            }
            audio_sync_set_tws_together(audio_sync, 1, a2dp_tws_align_time);
            a2dp_tws_align = 0;
            if (a2dp_tws_delay) {
                dec->drop_points = 625 * a2dp_tws_delay / 1000 * (dec->sample_rate / 1000);
                a2dp_tws_delay = 0;
            }
            audio_dac_sync_input_num_correct(&dac_hdl, dec->drop_points);
            dec->sync_step = 2;
            audio_decoder_wakeup_modify(AUDIO_DECODE_TASK_WAKEUP_TIME);
            return 0;
        }

        if (time_after(jiffies, dec->wait_time)) {
            log_w("a2dp wait tws confirm timeout\n");
            dec->sync_step = 2;
            dec->drop_points = (msecs - a2dp_delay_time) * (dec->sample_rate / 1000);
            audio_dac_sync_input_num_correct(&dac_hdl, dec->drop_points);
            audio_decoder_wakeup_modify(AUDIO_DECODE_TASK_WAKEUP_TIME);
            return 0;
        }
        return -EAGAIN;
    }
#else
    if (!dec->sync_step) {
        if (msecs < a2dp_delay_time && rx_remain > 768) {
            return -EAGAIN;
        }
        dec->sync_step = 2;
    }
#endif

    return 0;
}

static int a2dp_audio_delay_time(struct audio_decoder *decoder)
{
    struct a2dp_dec_hdl *dec = container_of(decoder, struct a2dp_dec_hdl, decoder);
    int msecs = 0;
#if TCFG_BT_MUSIC_EQ_ENABLE
#if A2DP_EQ_SUPPORT_ASYNC
    if (a2dp_eq) {
        msecs += (((audio_eq_data_len(a2dp_eq) >> 1) / dec->ch * 1000000) / dec->sample_rate) / 1000;
    }
#endif
#endif
    msecs += ((audio_mixer_ch_data_len(&dec->mix_ch) >> 1) / dec->ch * 1000000) / dec->sample_rate / 1000;

    msecs += audio_dac_data_time(&dac_hdl);

    return msecs;
}

static int a2dp_dec_rx_delay_monitor(struct audio_decoder *decoder, struct rt_stream_info *info)
{
    struct a2dp_dec_hdl *dec = container_of(decoder, struct a2dp_dec_hdl, decoder);
    int msecs = 0;
    int err = 0;

#if TCFG_USER_TWS_ENABLE
    msecs = a2dp_media_get_remain_play_time(1);
#else
    msecs = a2dp_media_get_remain_play_time(0);
#endif

    err = a2dp_dec_prepare_to_start(decoder, msecs, info->remain_len);
    if (err) {
        return err;
    }

#if A2DP_RX_AND_AUDIO_DELAY
    msecs += a2dp_audio_delay_time(decoder);
#endif

    info->rx_delay = RX_DELAY_NULL;
#if A2DP_RX_AND_AUDIO_DELAY
    if (msecs < a2dp_delay_time) {
        info->rx_delay = RX_DELAY_DOWN;
    } else if (msecs >= a2dp_delay_time + 10) {
        info->rx_delay = RX_DELAY_UP;
    }
#else
    if (msecs < a2dp_delay_time) {
        info->rx_delay = RX_DELAY_DOWN;
    } else if (msecs >= a2dp_delay_time + 10) {
        info->rx_delay = RX_DELAY_UP;
    }
#endif

    if (info->remain_len < 768) {
        info->rx_delay = RX_DELAY_UP;
    }

    /*printf("%dms\n", msecs);*/
    return 0;
}

static int a2dp_dec_probe_handler(struct audio_decoder *decoder)
{
    struct a2dp_dec_hdl *dec = container_of(decoder, struct a2dp_dec_hdl, decoder);
    int err = 0;

    if (a2dp_suspend) {
        audio_decoder_suspend(decoder, 0);
        return -EAGAIN;
    }

#if AUDIO_CODEC_SUPPORT_SYNC
    if (audio_sync) {
        if (time_before(jiffies, a2dp_resume_time)) {
            audio_decoder_suspend(decoder, 0);
            return -EAGAIN;
        }
        if (dec->preempt_state == DEC_PREEMTED_BY_OUTSIDE) {
            audio_decoder_pause(decoder);
            return -EINVAL;
        }

        if (dec->preempt_state == DEC_RESUME_BY_OUTSIDE) {
            audio_wireless_sync_reset(audio_sync);
            a2dp_dec_stop_and_restart(decoder, 1);
            dec->preempt_state = DEC_RUN_BY_ITSELF;
            return -EAGAIN;
        }
#if 1
        if (a2dp_media_is_clearing_frame() && dec->sync_step && tws_api_get_role() == TWS_ROLE_SLAVE) {
            audio_wireless_sync_reset(audio_sync);
            a2dp_dec_stop_and_restart(decoder, 1);
            return -EAGAIN;
        }
#endif
        struct rt_stream_info rts_info = {0};
        err = a2dp_dec_rx_info_check(&rts_info);
        if (err) {
            if (err == -EFAULT) {
                audio_wireless_sync_suspend(audio_sync);
                a2dp_dec_stop_and_restart(decoder, 0);
            } else {
                audio_decoder_suspend(decoder, 0);
            }
            if (dec->sync_step == 1) {
                dec->sync_step = 0;
            }
            return -EAGAIN;
        }

        err = a2dp_dec_rx_delay_monitor(decoder, &rts_info);
        if (err) {
            audio_decoder_suspend(decoder, 0);
            return -EAGAIN;
        }

        err = audio_wireless_sync_probe(audio_sync, &rts_info);
        if (err) {
            if (err == SYNC_ERR_STREAM_RESET) {
                a2dp_dec_stop_and_restart(decoder, 0);
            }
            return -EAGAIN;
        }

#if TCFG_USER_TWS_ENABLE
        if (tws_network_audio_was_started()) {
            /*a2dp播放中副机加入，声音复位500ms*/
            tws_network_local_audio_start();
            audio_dac_sound_reset(&dac_hdl, 500);
        }
#endif
        if (dec->preempt_state == DEC_PREEMTED_BY_PRIORITY) {
            dec->preempt_state = DEC_RUN_BY_ITSELF;
            audio_dac_sound_reset(&dac_hdl, 500);
        }
    }
    a2dp_dec_set_output_channel(dec);
#endif

    return err;
}

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
void a2dp_digital_vol_set(u8 vol)
{
    if (a2dp_dec) {
        audio_digital_vol_set(a2dp_dec->dvol, vol);
    }
}
#endif

static int a2dp_dec_output_handler(struct audio_decoder *decoder, s16 *data, int len, void *priv)
{
    int wlen = 0;
    int drop_len = 0;
    struct a2dp_dec_hdl *dec = container_of(decoder, struct a2dp_dec_hdl, decoder);


    if (!dec->remain) {
#if AUDIO_CODEC_SUPPORT_SYNC
        if (audio_sync) {
            audio_wireless_sync_after_dec(audio_sync, data, len);
        }
        if (dec->drop_points) {
            int points = (len >> 1) / dec->ch;
            if (points <= dec->drop_points) {
                dec->drop_points -= points;
                dec->droped_points += len >> 1;
                return len;
            }
            drop_len = (dec->drop_points << 1) * dec->ch;
            dec->droped_points += drop_len >> 1;
            len -= drop_len;
            data += (dec->drop_points * dec->ch);
            dec->drop_points = 0;
        }
#endif
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
        audio_digital_vol_run(a2dp_dec->dvol, data, len);
#endif

#if AUDIO_SURROUND_CONFIG
        if (dec->surround) {
            audio_surround_run(dec->surround, data, len);
        }
#endif

#if AUDIO_VBASS_CONFIG
        if (dec->vbass) {
            audio_vbass_run(dec->vbass, data, len);
        }
#endif

    }
#if TCFG_BT_MUSIC_EQ_ENABLE
#if A2DP_EQ_SUPPORT_ASYNC
    if (a2dp_eq) {
        int eqlen = 0;
#if AUDIO_CODEC_SUPPORT_SYNC
        if (len > sizeof(dac_sync_buff)) {
            /*大于异步SYNC的buf按SYNC的buffer大小做异步eq，防止异步EQ存入过多延时*/
            eqlen = audio_eq_run(a2dp_eq, data, sizeof(dac_sync_buff));
        } else {
            eqlen = audio_eq_run(a2dp_eq, data, len);
        }
#else
        eqlen = audio_eq_run(a2dp_eq, data, len);
#endif
        if (eqlen >= len) {
            dec->remain = 0;
        } else {
            dec->remain = 1;
        }
        return eqlen + drop_len;
    }
#endif
#endif


    if (!dec->remain) {
#if TCFG_BT_MUSIC_EQ_ENABLE
        if (a2dp_eq) {
            audio_eq_run(a2dp_eq, data, len);
        }
#endif
#if TCFG_BT_MUSIC_DRC_ENABLE
        if (a2dp_drc) {
            audio_drc_run(a2dp_drc, data, len);
        }
#endif

    }

    wlen = audio_mixer_ch_write(&dec->mix_ch, data, len);

    int remain_len = len - wlen;

    if (remain_len == 0) {
        dec->remain = 0;
    } else {
        dec->remain = 1;
    }
    return wlen + drop_len;
}

static int a2dp_dec_post_handler(struct audio_decoder *decoder)
{
    return 0;
}

static const struct audio_dec_handler a2dp_dec_handler = {
    .dec_probe  = a2dp_dec_probe_handler,
    .dec_output = a2dp_dec_output_handler,
    .dec_post   = a2dp_dec_post_handler,
};

void a2dp_dec_close();

static void a2dp_dec_release()
{
    audio_decoder_task_del_wait(&decode_task, &a2dp_dec->wait);
    a2dp_drop_frame_stop();

    free(a2dp_dec);
    a2dp_dec = NULL;
}

static void a2dp_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_DEC_EVENT_END:
        puts("AUDIO_DEC_EVENT_END\n");
        a2dp_dec_close();
        break;
    }
}



void audio_overlay_load_code(u32 type);
int a2dp_dec_start()
{
    int err;
    struct audio_fmt *fmt;
    enum audio_channel channel;
    struct a2dp_dec_hdl *dec = a2dp_dec;

    if (!a2dp_dec) {
        return -EINVAL;
    }

    puts("a2dp_dec_start: in\n");
#if TCFG_USER_TWS_ENABLE
    int a2dp_low_latency = tws_api_get_low_latency_state();
#endif
    if (a2dp_low_latency) {
        a2dp_delay_time = a2dp_dec->coding_type == AUDIO_CODING_AAC ? CONFIG_A2DP_DELAY_TIME_LO : CONFIG_A2DP_SBC_DELAY_TIME_LO;
    } else {
        a2dp_delay_time = CONFIG_A2DP_DELAY_TIME;
    }

    if (a2dp_dec->coding_type == AUDIO_CODING_AAC) {
        audio_overlay_load_code(a2dp_dec->coding_type);
    }

    err = audio_decoder_open(&dec->decoder, &a2dp_input, &decode_task);
    if (err) {
        goto __err1;
    }
    dec->channel = AUDIO_CH_MAX;

    audio_decoder_set_handler(&dec->decoder, &a2dp_dec_handler);
    audio_decoder_set_event_handler(&dec->decoder, a2dp_dec_event_handler, 0);

    if (a2dp_dec->coding_type != a2dp_input.coding_type) {
        struct audio_fmt f = {0};
        f.coding_type = a2dp_dec->coding_type;
        err = audio_decoder_set_fmt(&dec->decoder, &f);
        if (err) {
            goto __err2;
        }
    }

    err = audio_decoder_get_fmt(&dec->decoder, &fmt);
    if (err) {
        goto __err2;
    }

    //dac_hdl.dec_channel_num = fmt->channel;
    dec->sample_rate = fmt->sample_rate;

    set_source_sample_rate(fmt->sample_rate);
    a2dp_dec_set_output_channel(dec);

    audio_mixer_ch_open(&dec->mix_ch, &mixer);
    audio_mixer_ch_set_sample_rate(&dec->mix_ch, fmt->sample_rate);

    app_audio_state_switch(APP_AUDIO_STATE_MUSIC, get_max_sys_vol());
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    a2dp_dec->dvol = audio_digital_vol_open(app_audio_get_volume(APP_AUDIO_STATE_MUSIC), SYS_MAX_VOL, 2);
#endif

#if TCFG_EQ_ENABLE
#if TCFG_BT_MUSIC_EQ_ENABLE
    a2dp_eq = zalloc(sizeof(struct audio_eq) + sizeof(struct hw_eq_ch));
    if (a2dp_eq) {
        a2dp_eq->eq_ch = (struct hw_eq_ch *)((int)a2dp_eq + sizeof(struct audio_eq));
        struct audio_eq_param a2dp_eq_param = {0};
        a2dp_eq_param.channels = dec->ch;//fmt->channel;
        a2dp_eq_param.online_en = 1;
        a2dp_eq_param.mode_en = 1;
        a2dp_eq_param.remain_en = 1;
#if A2DP_EQ_SUPPORT_ASYNC
        a2dp_eq_param.no_wait = 1;
#endif
        a2dp_eq_param.max_nsection = EQ_SECTION_MAX;
        a2dp_eq_param.cb = eq_get_filter_info;
        audio_eq_open(a2dp_eq, &a2dp_eq_param);
        audio_eq_set_samplerate(a2dp_eq, fmt->sample_rate);
#if A2DP_EQ_SUPPORT_32BIT
        audio_eq_set_info(a2dp_eq, a2dp_eq_param.channels, 1);
#endif
        audio_eq_set_output_handle(a2dp_eq, a2dp_eq_output, dec);
        audio_eq_start(a2dp_eq);
    }
#endif


#if TCFG_BT_MUSIC_DRC_ENABLE
    a2dp_drc = malloc(sizeof(struct audio_drc));
    if (a2dp_drc) {
        struct audio_drc_param drc_param = {0};
        drc_param.channels = dec->ch;
        drc_param.online_en = 1;
        drc_param.remain_en = 1;
        drc_param.cb = drc_get_filter_info;
        audio_drc_open(a2dp_drc, &drc_param);
        audio_drc_set_samplerate(a2dp_drc, fmt->sample_rate);
#if A2DP_EQ_SUPPORT_32BIT
        audio_drc_set_32bit_mode(a2dp_drc, 1);
#endif
        audio_drc_set_output_handle(a2dp_drc, NULL, NULL);
        audio_drc_start(a2dp_drc);
    }
#endif
#endif

#if AUDIO_SURROUND_CONFIG
    u8 nch = EFFECT_CH_L;
    if (dec->channel == AUDIO_CH_L) {
        nch = EFFECT_CH_L;
    } else if (dec->channel == AUDIO_CH_R) {
        nch = EFFECT_CH_R;
    } else if (dec->channel == AUDIO_CH_LR) {
        nch = 2;
    }
    surround_open_parm parm = {0};
    parm.channel = nch;
    parm.surround_effect_type = EFFECT_3D_PANORAMA;//打开时默认使用3d全景音,使用者，根据需求修改
    dec->surround = audio_surround_open(&parm);
    //sur_test = sys_timer_add(dec->surround, surround_switch_test, 10000);
#endif


#if AUDIO_VBASS_CONFIG
    vbass_open_parm vparm = {0};
    vparm.sr = fmt->sample_rate;
    vparm.channel = dec->ch;
    dec->vbass = audio_vbass_open(&vparm);
    if (dec->vbass) {
        vbass_update_parm def_parm = {0};
        def_parm.bass_f = 200;//外放的低音截止频率
        def_parm.level = 4096;//增强强度(4096 等于 1db， 建议范围：4096 到 16384);
        audio_vbass_parm_update(dec->vbass, VBASS_UPDATE_PARM, &def_parm);

#if AUDIO_VBASS_TEST
        //vbass_t = sys_timer_add(dec->vbass, vbass_udate_parm_test, 2000);
#endif
    }
#endif


#if AUDIO_CODEC_SUPPORT_SYNC
    if (!audio_sync) {
        audio_sync = a2dp_play_sync_open(dec->ch, fmt->sample_rate, fmt->sample_rate, dec->coding_type);
    }

    if (audio_sync) {
        audio_mixer_ch_set_event_handler(&dec->mix_ch, dec, bt_audio_mixer_ch_event_handler);
        audio_wireless_sync_info_init(audio_sync, fmt->sample_rate, fmt->sample_rate, dec->ch);
    }
#endif
    a2dp_drop_frame_stop();
    dec->remain = 0;
    /*
     * A2DP解码时钟设置：
     * 1、普通设置
     *    开始解码: clk_set_sys_lock(xxx, 0);
     *    退出解码: clk_set_sys_lock(xxx, 0);
     * 2、特殊设置（使用wav或其他提示音并带有操作开始/暂停 A2DP音频）
     *    开始解码: clk_set_sys_lock(xxx, 1);
     *    退出解码: clk_set_sys_lock(xxx, 2);
     */
    if (dec->coding_type == AUDIO_CODING_SBC) {
#if (TCFG_BT_MUSIC_EQ_ENABLE && ((EQ_SECTION_MAX >= 10) && AUDIO_OUT_EFFECT_ENABLE))
        clk_set_sys_lock(SYS_64M, 0) ;
#elif (TCFG_BT_MUSIC_EQ_ENABLE && ((EQ_SECTION_MAX >= 10) || AUDIO_OUT_EFFECT_ENABLE))
        clk_set_sys_lock(SYS_48M, 0) ;
#else
#if (!TCFG_AUDIO_ANC_ENABLE)
        clk_set_sys_lock(SYS_24M, 0) ;
#endif
#endif
    } else if (dec->coding_type == AUDIO_CODING_AAC) {
#if (TCFG_BT_MUSIC_EQ_ENABLE && ((EQ_SECTION_MAX >= 10) || AUDIO_OUT_EFFECT_ENABLE))
        clk_set_sys_lock(SYS_64M, 0) ;
#else
        clk_set_sys_lock(SYS_48M, 0) ;
#endif
    }

#if (AUDIO_VBASS_CONFIG || AUDIO_SURROUND_CONFIG)
    clk_set_sys_lock(SYS_64M, 0) ;
#endif
    err = audio_decoder_start(&dec->decoder);
    if (err) {
        goto __err3;
    }

    dec->start = 1;

    return 0;

__err3:
    audio_mixer_ch_close(&a2dp_dec->mix_ch);
__err2:
    audio_decoder_close(&dec->decoder);
__err1:
    a2dp_dec_release();

    return err;
}

static int __a2dp_audio_res_close(void)
{
    a2dp_dec->start = 0;
    audio_decoder_close(&a2dp_dec->decoder);
    audio_mixer_ch_close(&a2dp_dec->mix_ch);
#if AUDIO_CODEC_SUPPORT_SYNC
    if (audio_sync) {
        audio_wireless_sync_close(audio_sync);
        audio_sync = NULL;
        a2dp_dec->preempt_state = DEC_PREEMTED_BY_PRIORITY;
        a2dp_dec->sync_step = 0;
    }
    a2dp_dec->droped_points = 0;
#endif
#if TCFG_BT_MUSIC_EQ_ENABLE
    if (a2dp_eq) {
        audio_eq_close(a2dp_eq);
        free(a2dp_eq);
        a2dp_eq = NULL;
    }
#endif

#if TCFG_BT_MUSIC_DRC_ENABLE
    if (a2dp_drc) {
        audio_drc_close(a2dp_drc);
        free(a2dp_drc);
        a2dp_drc = NULL;
    }
#endif

#if A2DP_EQ_SUPPORT_32BIT
    if (a2dp_dec->eq_out_buf) {
        free(a2dp_dec->eq_out_buf);
        a2dp_dec->eq_out_buf = NULL;
    }
#endif

#if AUDIO_SURROUND_CONFIG
    if (a2dp_dec->surround) {
        audio_surround_close(a2dp_dec->surround);
        a2dp_dec->surround = NULL;
    }
#endif

#if AUDIO_VBASS_CONFIG
    if (a2dp_dec->vbass) {
        audio_vbass_close(a2dp_dec->vbass);
        a2dp_dec->vbass = NULL;
    }
#endif


#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    audio_digital_vol_close(a2dp_dec->dvol);
    a2dp_dec->dvol = NULL;
#endif
    clk_set_sys_lock(BT_NORMAL_HZ, 0);

    app_audio_state_exit(APP_AUDIO_STATE_MUSIC);
    return 0;
}

static int a2dp_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;

    printf("a2dp_wait_res_handler: %d\n", event);

    if (event == AUDIO_RES_GET) {
        err = a2dp_dec_start();
    } else if (event == AUDIO_RES_PUT) {
        if (a2dp_dec->start) {
            __a2dp_audio_res_close();
            a2dp_drop_frame_start();
        }
    }
    return err;
}

#define A2DP_CODEC_SBC			0x00
#define A2DP_CODEC_MPEG12		0x01
#define A2DP_CODEC_MPEG24		0x02
int a2dp_dec_open(int media_type)
{
    struct a2dp_dec_hdl *dec;

    if (strcmp(os_current_task(), "app_core") != 0) {
        log_e("a2dp dec open in task : %s\n", os_current_task());
    }

#if TCFG_AUDIO_ANC_ENABLE
#ifdef CONFIG_ANC_OVERLAY
    if (anc_train_open_query()) {
        printf("anc_train_open,suspend a2dp_dec\n");
        if (drop_a2dp_timer == 0) {
            drop_a2dp_timer = sys_timer_add(NULL, __a2dp_drop_frame, 50);
        }
        return 0;
    }
#endif/*CONFIG_ANC_OVERLAY*/
#endif /*TCFG_AUDIO_ANC_ENABLE*/

    if (a2dp_suspend) {
        if (tws_api_get_role() == TWS_ROLE_MASTER) {
            if (drop_a2dp_timer == 0) {
                drop_a2dp_timer = sys_timer_add(NULL,
                                                a2dp_media_clear_packet_before_seqn, 100);
            }
        }
        return 0;
    }

    if (a2dp_dec) {
        return 0;
    }

    media_type = a2dp_media_get_codec_type();
    printf("a2dp_dec_open: %d\n", media_type);

    dec = zalloc(sizeof(*dec));
    if (!dec) {
        return -ENOMEM;
    }

    switch (media_type) {
    case A2DP_CODEC_SBC:
        printf("a2dp_media_type:SBC");
        dec->coding_type = AUDIO_CODING_SBC;
        break;
    case A2DP_CODEC_MPEG24:
        printf("a2dp_media_type:AAC");
        dec->coding_type = AUDIO_CODING_AAC;
        break;
    default:
        printf("a2dp_media_type unsupoport:%d", media_type);
        free(dec);
        return -EINVAL;
    }

    a2dp_dec = dec;

    dec->wait.priority = 0;
    dec->wait.preemption = 1;
    dec->wait.handler = a2dp_wait_res_handler;
    audio_decoder_task_add_wait(&decode_task, &dec->wait);

    if (a2dp_dec && (a2dp_dec->start == 0)) {
        a2dp_dec->preempt_state = DEC_PREEMTED_BY_PRIORITY;
        a2dp_drop_frame_start();
    }

    return 0;
}

void a2dp_dec_close()
{
    if (!a2dp_dec) {
        if (CONFIG_LOW_LATENCY_ENABLE) {
            a2dp_low_latency_seqn = 0;
        }
        if (drop_a2dp_timer) {
            sys_timer_del(drop_a2dp_timer);
            drop_a2dp_timer = 0;
        }
        return;
    }

    if (a2dp_dec->start) {
        __a2dp_audio_res_close();
    }

    a2dp_dec_release();

    puts("a2dp_dec_close: exit\n");
}


static void a2dp_low_latency_clear_a2dp_packet(u8 *data, int len, int rx)
{
    if (rx) {
        a2dp_low_latency_seqn = (data[0] << 8) | data[1];
    }
}
REGISTER_TWS_FUNC_STUB(audio_dec_clear_a2dp_packet) = {
    .func_id = 0x132A6578,
    .func    = a2dp_low_latency_clear_a2dp_packet,
};


static void low_latency_drop_a2dp_frame(void *p)
{
    int len;

    /*y_printf("low_latency_drop_a2dp_frame\n");*/

    if (a2dp_low_latency_seqn == 0) {
        a2dp_media_clear_packet_before_seqn(0);
        return;
    }
    while (1) {
        u8 *packet = a2dp_media_fetch_packet(&len, NULL);
        if (!packet) {
            return;
        }
        u16 seqn = (packet[2] << 8) | packet[3];
        if (seqn_after(seqn, a2dp_low_latency_seqn)) {
            printf("clear_end: %d\n", seqn);
            break;
        }
        a2dp_media_free_packet(packet);
        /*printf("clear: %d\n", seqn);*/
    }

    if (drop_a2dp_timer) {
        sys_timer_del(drop_a2dp_timer);
        drop_a2dp_timer = 0;
    }
    int type = a2dp_media_get_codec_type();
    if (type >= 0) {
        a2dp_dec_open(type);
    }

    if (a2dp_low_latency == 0) {
        tws_api_auto_role_switch_enable();
    }

    printf("a2dp_delay: %d\n", a2dp_media_get_remain_play_time(1));
}


int earphone_a2dp_codec_set_low_latency_mode(int enable, int msec)
{
    int ret = 0;
    int len, err;

    if (CONFIG_LOW_LATENCY_ENABLE == 0) {
        return -EINVAL;
    }

    if (esco_dec) {
        return -EINVAL;
    }
    if (drop_a2dp_timer) {
        return -EINVAL;
    }
    if (a2dp_suspend) {
        return -EINVAL;
    }

    a2dp_low_latency = enable;
    a2dp_low_latency_seqn = 0;

    r_printf("a2dp_low_latency: %d, %d, %d\n", a2dp_dec->seqn, a2dp_delay_time, enable);

    if (!a2dp_dec || a2dp_dec->start == 0) {
#if TCFG_USER_TWS_ENABLE
        tws_api_low_latency_enable(enable);
#endif
        return 0;
    }

    if (a2dp_dec->coding_type == AUDIO_CODING_SBC) {
        a2dp_low_latency_seqn = a2dp_dec->seqn + (msec + a2dp_delay_time) / 15;
    } else {
        a2dp_low_latency_seqn = a2dp_dec->seqn + (msec + a2dp_delay_time) / 20;
    }

#if TCFG_USER_TWS_ENABLE
    u8 data[2];
    data[0] = a2dp_low_latency_seqn >> 8;
    data[1] = a2dp_low_latency_seqn;
    err = tws_api_send_data_to_slave(data, 2, 0x132A6578);
    if (err == -ENOMEM) {
        return -EINVAL;
    }
#endif

    a2dp_dec_close();

    a2dp_media_clear_packet_before_seqn(0);

#if TCFG_USER_TWS_ENABLE
    if (enable) {
        tws_api_auto_role_switch_disable();
    }
    tws_api_low_latency_enable(enable);
#endif

    drop_a2dp_timer = sys_timer_add(NULL, low_latency_drop_a2dp_frame, 40);

    /*r_printf("clear_to_seqn: %d\n", a2dp_low_latency_seqn);*/

    return 0;
}

int earphone_a2dp_codec_get_low_latency_mode()
{
#if TCFG_USER_TWS_ENABLE
    return tws_api_get_low_latency_state();
#endif
    return a2dp_low_latency;
}


int a2dp_tws_dec_suspend(void *p)
{
    r_printf("a2dp_tws_dec_suspend\n");
    /*mem_stats();*/

    if (a2dp_suspend) {
        return -EINVAL;
    }
    a2dp_suspend = 1;

    if (a2dp_dec) {
        a2dp_dec_close();
        a2dp_media_clear_packet_before_seqn(0);
        if (tws_api_get_role() == 0) {
            drop_a2dp_timer = sys_timer_add(NULL, a2dp_media_clear_packet_before_seqn, 100);
        }
    }

    int err = audio_decoder_fmt_lock(&decode_task, AUDIO_CODING_AAC);
    if (err) {
        log_e("AAC_dec_lock_faild\n");
    }

    return err;
}


void a2dp_tws_dec_resume(void)
{
    r_printf("a2dp_tws_dec_resume\n");

    if (a2dp_suspend) {
        a2dp_suspend = 0;

        if (drop_a2dp_timer) {
            sys_timer_del(drop_a2dp_timer);
            drop_a2dp_timer = 0;
        }

        audio_decoder_fmt_unlock(&decode_task, AUDIO_CODING_AAC);

        int type = a2dp_media_get_codec_type();
        printf("codec_type: %d\n", type);
        if (type >= 0) {
            if (tws_api_get_role() == 0) {
                a2dp_media_clear_packet_before_seqn(0);
            }
            a2dp_resume_time = jiffies + msecs_to_jiffies(80);
            a2dp_dec_open(type);
        }
    }
}

static int esco_dec_stop_and_restart(struct audio_decoder *decoder)
{
    struct esco_dec_hdl *dec = container_of(decoder, struct esco_dec_hdl, decoder);

#if AUDIO_CODEC_SUPPORT_SYNC
    if (audio_sync) {
        audio_wireless_sync_stop(audio_sync);
        audio_mixer_ch_reset(&dec->mix_ch);
#if AUDIO_OUT_EFFECT_ENABLE
        audio_out_effect_stream_clear();
#endif
        audio_dac_sound_reset(&dac_hdl, 300);
        dec->sync_step = 0;
    }
#endif

    return 0;
}


static const u8 msbc_mute_data[] = {
    0xAD, 0x00, 0x00, 0xC5, 0x00, 0x00, 0x00, 0x00, 0x77, 0x6D, 0xB6, 0xDD, 0xDB, 0x6D, 0xB7, 0x76,
    0xDB, 0x6D, 0xDD, 0xB6, 0xDB, 0x77, 0x6D, 0xB6, 0xDD, 0xDB, 0x6D, 0xB7, 0x76, 0xDB, 0x6D, 0xDD,
    0xB6, 0xDB, 0x77, 0x6D, 0xB6, 0xDD, 0xDB, 0x6D, 0xB7, 0x76, 0xDB, 0x6D, 0xDD, 0xB6, 0xDB, 0x77,
    0x6D, 0xB6, 0xDD, 0xDB, 0x6D, 0xB7, 0x76, 0xDB, 0x6C, 0x00,
};

static int headcheck(u8 *buf)
{
    int sync_word = buf[0] | ((buf[1] & 0x0f) << 8);
    return (sync_word == 0x801) && (buf[2] == 0xAD);
}

static int esco_dump_rts_info(struct rt_stream_info *pkt)
{
    u32 hash = 0xffffffff;
    int read_len = 0;
    pkt->baddr = lmp_private_get_esco_packet(&read_len, &hash);
    pkt->seqn = hash;
    /* printf("hash0=%d,%d ",hash,pkt->baddr ); */
    if (pkt->baddr && read_len) {
        pkt->remain_len = lmp_private_get_esco_remain_buffer_size();
        pkt->data_len = lmp_private_get_esco_data_len();
        return 0;
    }
    if (read_len == -EINVAL) {
        //puts("----esco close\n");
        return -EINVAL;
    }
    if (read_len < 0) {

        return -ENOMEM;
    }
    return ENOMEM;
}

static int esco_dec_get_frame(struct audio_decoder *decoder, u8 **frame)
{
    int len = 0;
    u32 hash = 0;
    struct esco_dec_hdl *dec = container_of(decoder, struct esco_dec_hdl, decoder);

__again:
    if (dec->frame) {
        int len = dec->frame_len - dec->offset;
        if (len > dec->esco_len - dec->data_len) {
            len = dec->esco_len - dec->data_len;
        }
        /*memcpy((u8 *)dec->data + dec->data_len, msbc_mute_data, sizeof(msbc_mute_data));*/
        memcpy((u8 *)dec->data + dec->data_len, dec->frame + dec->offset, len);
        dec->offset   += len;
        dec->data_len += len;
        if (dec->offset == dec->frame_len) {
            lmp_private_free_esco_packet(dec->frame);
            dec->frame = NULL;
        }
    }

    if (dec->data_len < dec->esco_len) {
        dec->frame = lmp_private_get_esco_packet(&len, &hash);
        /* printf("hash1=%d,%d ",hash,dec->frame ); */
        if (len <= 0) {
            printf("rlen=%d ", len);
            return -EIO;
        }

#if (defined(TCFG_PHONE_MESSAGE_ENABLE) && (TCFG_PHONE_MESSAGE_ENABLE))
        phone_message_enc_write(dec->frame + 2, len - 2);
#endif
        dec->offset = 0;
        dec->frame_len = len;
        goto __again;
    }
    *frame = (u8 *)dec->data;
    return dec->esco_len;
}

static void esco_dec_put_frame(struct audio_decoder *decoder, u8 *frame)
{
    struct esco_dec_hdl *dec = container_of(decoder, struct esco_dec_hdl, decoder);

    dec->data_len = 0;
    /*lmp_private_free_esco_packet((void *)frame);*/
}

static const struct audio_dec_input esco_input = {
    .coding_type = AUDIO_CODING_MSBC,
    .data_type   = AUDIO_INPUT_FRAME,
    .ops = {
        .frame = {
            .fget = esco_dec_get_frame,
            .fput = esco_dec_put_frame,
        }
    }
};

u32 lmp_private_clear_sco_packet(u8 clear_num);
static void esco_dec_clear_all_packet(struct esco_dec_hdl *dec)
{
    lmp_private_clear_sco_packet(0xff);
}

static int esco_dec_rx_delay_monitor(struct audio_decoder *decoder, struct rt_stream_info *info)
{
    struct esco_dec_hdl *dec = container_of(decoder, struct esco_dec_hdl, decoder);

    if (!dec->sync_step) {
        if (info->data_len <= 0) {
            return -EAGAIN;
        }
#if TCFG_USER_TWS_ENABLE
        int state = tws_api_get_tws_state();
        if (state & TWS_STA_SIBLING_CONNECTED) {
            if (tws_api_get_role() == TWS_ROLE_SLAVE) {
                audio_dac_sound_reset(&dac_hdl, 150);
            }
        }
#endif
        dec->sync_step = 2;
    }

    info->rx_delay = RX_DELAY_NULL;
    if (info->data_len <= 120) {
        info->rx_delay = RX_DELAY_DOWN;
    } else if (info->data_len > 240) {
        info->rx_delay = RX_DELAY_UP;
    }

    if (info->remain_len < 256) {
        info->rx_delay = RX_DELAY_UP;
    }

    /*printf("%d - %d\n", info->data_len, info->remain_len);*/
    return 0;
}

static int esco_dec_probe_handler(struct audio_decoder *decoder)
{
    struct esco_dec_hdl *dec = container_of(decoder, struct esco_dec_hdl, decoder);
    int err = 0;
    int find_packet = 0;
    struct rt_stream_info rts_info = {0};
    err = esco_dump_rts_info(&rts_info);
    if (err == -EINVAL) {
        return err;
    }
    if (err || !dec->enc_start) {
        audio_decoder_suspend(decoder, 0);
        return -EAGAIN;
    }

#if AUDIO_CODEC_SUPPORT_SYNC
    if (audio_sync) {
        err = esco_dec_rx_delay_monitor(decoder, &rts_info);
        if (err) {
            audio_decoder_suspend(decoder, 0);
            return -EAGAIN;
        }
        err = audio_wireless_sync_probe(audio_sync, &rts_info);
        if (err) {
            if (err == SYNC_ERR_STREAM_RESET) {
                puts("esco_dec_stop_and_restart\n");
                esco_dec_stop_and_restart(decoder);
            }
            return -EAGAIN;
        }
#if TCFG_USER_TWS_ENABLE
        if (tws_network_audio_was_started()) {
            tws_network_local_audio_start();
            audio_dac_sound_reset(&dac_hdl, 500);
        }
#endif

        if (dec->preempt_state == DEC_PREEMTED_BY_PRIORITY) {
            dec->preempt_state = DEC_RUN_BY_ITSELF;
            audio_dac_sound_reset(&dac_hdl, 500);
        }
    }
#endif

    return err;
}

#if TCFG_PHONE_EQ_ENABLE

static int esco_eq_output(void *priv, void *data, u32 len)
{
    return len;
}

#endif

static int audio_esco_int2short_convert_output(void *priv, s16 *data, int len)
{
    /* put_buf(data,len); */
    return audio_dac_write(&dac_hdl, data, len);

}
static int audio_esco_short2int_convert_output(void *priv, void *data, int len)
{
#if 0//AUDIO_CODEC_SUPPORT_SYNC
    if (audio_sync) {
        audio_decoder_sync_run(audio_sync, data, len);
#if TCFG_PHONE_EQ_ENABLE
    } else if (esco_eq) {
        return audio_eq_run(esco_eq, data, len);
#endif
    } else if (int2short_convert) {
        /*return int2short_convert_run(int2short_convert, data, len);*/
    }
#endif
    return len;
}

/*level:0~15*/
static const u16 esco_dvol_tab[] = {
    0,	//0
    111,//1
    161,//2
    234,//3
    338,//4
    490,//5
    708,//6
    1024,//7
    1481,//8
    2142,//9
    3098,//10
    4479,//11
    6477,//12
    9366,//13
    14955,//14
    16384 //15
};
#if TCFG_AUDIO_OUTPUT_IIS
static u8 t_esco_buf[480];
#endif

#if TCFG_ESCO_DL_NS_ENABLE
#include "commproc_ns.h"
#define NS_FRAME_POINTS	160
#define NS_FRAME_SIZE	(NS_FRAME_POINTS << 1)
typedef struct {
    s16 out[NS_FRAME_POINTS];
    s16 in[512];
    cbuffer_t cbuf;
    u16 offset;
} dl_ns_t;
dl_ns_t *dl_ns = NULL;

int esco_dl_ns_run(short *data, u16 len)
{
    int wlen = cbuf_write(&dl_ns->cbuf, data, len);
    if (wlen != len) {
        printf("dl_cbuf full\n");
    }
    if (dl_ns->cbuf.data_len >= NS_FRAME_SIZE) {
        cbuf_read(&dl_ns->cbuf, dl_ns->out, NS_FRAME_SIZE);
        /*接通的时候再开始做降噪*/
        if (get_call_status() == BT_CALL_ACTIVE) {
            noise_suppress_run(dl_ns->out, dl_ns->out, NS_FRAME_POINTS);
        }
        return NS_FRAME_SIZE;
    } else {
        return 0;
    }
}

static noise_suppress_param ns_para = {
    .wideband = 1,
    .mode = 0,
    .NoiseLevel = 1.0e2f,
    .AggressFactor = 1.25f,
    .MinSuppress = 0.04f,
};
int esco_dl_ns_open(u16 sr)
{
    dl_ns = zalloc(sizeof(dl_ns_t));
    cbuf_init(&dl_ns->cbuf, dl_ns->in, sizeof(dl_ns->in));

    if (sr == 8000) {
        ns_para.wideband = 1;
    }
    noise_suppress_open(&ns_para);
    printf("esco_dl_ns_open ok\n");
    return 0;
}
int esco_dl_ns_close(void)
{
    noise_suppress_close();
    if (dl_ns) {
        free(dl_ns);
        dl_ns = NULL;
    }
    printf("esco_dl_ns_close ok\n");
    return 0;
}
#endif/*TCFG_ESCO_DL_NS_ENABLE*/

static int esco_dec_output_handler(struct audio_decoder *decoder, s16 *buf, int size, void *priv)
{
    int wlen = 0;
    struct esco_dec_hdl *dec = container_of(decoder, struct esco_dec_hdl, decoder);

    int ret_len = size;
    int len = size;
    short *data = buf;

#if TCFG_ESCO_DL_NS_ENABLE
    if (dec->remain == 0) {
        wlen = esco_dl_ns_run(data, size);
        //输入消耗完毕，没有输出
        if (wlen == 0) {
            return ret_len;
        }
        data = dl_ns->out;
        len = wlen;
        dl_ns->offset = 0;
        ret_len = size;
    } else {
        data = dl_ns->out + (dl_ns->offset / 2);
        len = dec->remain;
        ret_len = 0;
    }
#endif/*TCFG_ESCO_DL_NS_ENABLE*/

    /*非上次残留数据,进行后处理*/
    if (!dec->remain) {
#if (defined(TCFG_PHONE_MESSAGE_ENABLE) && (TCFG_PHONE_MESSAGE_ENABLE))
        phone_message_call_api_esco_out_data(data, len);
#endif

#if AUDIO_CODEC_SUPPORT_SYNC
        if (audio_sync) {
            audio_wireless_sync_after_dec(audio_sync, data, len);
        }
#endif
        if (priv) {
            audio_plc_run(data, len, *(u8 *)priv);
        }
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
        audio_digital_vol_run(esco_dec->dvol, data, len);
        u16 dvol_val = esco_dvol_tab[app_var.aec_dac_gain];
        for (u16 i = 0; i < len / 2; i++) {
            s32 tmp_data = data[i];
            if (tmp_data < 0) {
                tmp_data = -tmp_data;
                tmp_data = (tmp_data * dvol_val) >> 14;
                tmp_data = -tmp_data;
            } else {
                tmp_data = (tmp_data * dvol_val) >> 14;
            }
            data[i] = tmp_data;
        }
#endif

#if TCFG_AUDIO_NOISE_GATE
        /*来电去电铃声不做处理*/
        if (get_call_status() == BT_CALL_ACTIVE) {
            audio_noise_gate_run(data, data, len);
        }
#endif/*TCFG_AUDIO_NOISE_GATE*/

#if TCFG_PHONE_EQ_ENABLE
        if (esco_eq) {
            audio_eq_run(esco_eq, data, len);
        }
#endif
    }

#if TCFG_AUDIO_OUTPUT_IIS
    s16 *tdata = (s16 *)t_esco_buf;
    int n = 0;
    for (n = 0; n < len / 2; n++) {
        tdata[n * 2] = data[n];
        tdata[n * 2 + 1] = data[n];
    }

    wlen = audio_mixer_ch_write(&dec->mix_ch, tdata, len << 1);
    wlen >>= 1;
#else
    wlen = audio_mixer_ch_write(&dec->mix_ch, data, len);
#endif

    int remain_len = len - wlen;

#if TCFG_ESCO_DL_NS_ENABLE
    if (remain_len == 0) {
        dl_ns->offset = 0;
    } else {
        dl_ns->offset += wlen;
    }
#else
    ret_len = wlen;
#endif/*TCFG_ESCO_DL_NS_ENABLE*/
    dec->remain = remain_len;

    return ret_len;
}

static int esco_dec_post_handler(struct audio_decoder *decoder)
{

    return 0;
}

static const struct audio_dec_handler esco_dec_handler = {
    .dec_probe  = esco_dec_probe_handler,
    .dec_output = esco_dec_output_handler,
    .dec_post   = esco_dec_post_handler,
};

void esco_dec_release()
{
    audio_decoder_task_del_wait(&decode_task, &esco_dec->wait);
    free(esco_dec);
    esco_dec = NULL;
}

void esco_dec_close();

static void esco_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_DEC_EVENT_END:
        puts("AUDIO_DEC_EVENT_END\n");
        esco_dec_close();
        break;
    }
}


static void esco_dec_set_output_channel(struct esco_dec_hdl *dec)
{
    u8 dac_connect_mode = audio_dac_get_channel(&dac_hdl);

    if (dac_connect_mode == DAC_OUTPUT_LR) {
        dac_hdl.dec_channel_num = 1;
    }
}

u16 source_sr;
void set_source_sample_rate(u16 sample_rate)
{
    source_sr = sample_rate;
}

u16 get_source_sample_rate()
{
    if (bt_audio_is_running()) {
        return source_sr;
    }
    return 0;
}

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
void esco_digital_vol_set(u8 vol)
{
    if (esco_dec) {
        audio_digital_vol_set(esco_dec->dvol, vol);
    }
}
#endif

int esco_dec_dac_gain_set(u8 gain)
{
    app_var.aec_dac_gain = gain;
    if (esco_dec) {
        audio_dac_set_analog_vol(&dac_hdl, gain);
    }
    return 0;
}

int esco_dec_start()
{
    int err;
    struct audio_fmt f;
    enum audio_channel channel;
    struct esco_dec_hdl *dec = esco_dec;
    u16 mix_buf_len_fix = 240;

    if (!esco_dec) {
        return -EINVAL;
    }

    err = audio_decoder_open(&dec->decoder, &esco_input, &decode_task);
    if (err) {
        goto __err1;
    }

    audio_decoder_set_handler(&dec->decoder, &esco_dec_handler);
    audio_decoder_set_event_handler(&dec->decoder, esco_dec_event_handler, 0);

    if (dec->coding_type == AUDIO_CODING_MSBC) {
        f.coding_type = AUDIO_CODING_MSBC;
        f.sample_rate = 16000;
        f.channel = 1;
    } else if (dec->coding_type == AUDIO_CODING_CVSD) {
        f.coding_type = AUDIO_CODING_CVSD;
        f.sample_rate = 8000;
        f.channel = 1;
        mix_buf_len_fix = 120;
    }

    set_source_sample_rate(f.sample_rate);
    esco_dec_set_output_channel(esco_dec);

    err = audio_decoder_set_fmt(&dec->decoder, &f);
    if (err) {
        goto __err2;
    }

    /*
     *虽然mix有直通的处理，但是如果混合第二种声音进来的时候，就会按照mix_buff
     *的大小来混合输出，该buff太大，回导致dac没有连续的数据播放
     */
    /*audio_mixer_set_output_buf(&mixer, mix_buff, sizeof(mix_buff) / 8);*/
    audio_mixer_set_output_buf(&mixer, mix_buff, mix_buf_len_fix);
    audio_mixer_ch_open(&dec->mix_ch, &mixer);
    audio_mixer_ch_set_sample_rate(&dec->mix_ch, f.sample_rate);

    app_audio_state_switch(APP_AUDIO_STATE_CALL, app_var.aec_dac_gain);
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    esco_dec->dvol = audio_digital_vol_open(app_audio_get_volume(APP_AUDIO_STATE_CALL), 15, 4);
#endif
    printf("max_vol:%d,call_vol:%d", app_var.aec_dac_gain, app_audio_get_volume(APP_AUDIO_STATE_CALL));
    app_audio_set_volume(APP_AUDIO_STATE_CALL, app_var.call_volume, 1);

    audio_plc_open(f.sample_rate);

#if TCFG_AUDIO_NOISE_GATE
    /*限幅器上限*/
#define LIMITER_THR	 -10000 /*-12000 = -12dB,放大1000倍,(-10000参考)*/
    /*小于CONST_NOISE_GATE的当成噪声处理,防止清0近端声音*/
#define LIMITER_NOISE_GATE  -40000 /*-12000 = -12dB,放大1000倍,(-30000参考)*/
    /*低于噪声门限阈值的增益 */
#define LIMITER_NOISE_GAIN  (0 << 30) /*(0~1)*2^30*/
    audio_noise_gate_open(f.sample_rate, LIMITER_THR, LIMITER_NOISE_GATE, LIMITER_NOISE_GAIN);
#endif/*TCFG_AUDIO_NOISE_GATE*/

#if TCFG_PHONE_EQ_ENABLE
    esco_eq = zalloc(sizeof(struct audio_eq) + sizeof(struct hw_eq_ch));
    if (esco_eq) {
        esco_eq->eq_ch = (struct hw_eq_ch *)((int)esco_eq + sizeof(struct audio_eq));
        struct audio_eq_param esco_eq_param;
        esco_eq_param.channels = f.channel;
        esco_eq_param.online_en = 1; // 支持在线调试
        esco_eq_param.mode_en = 0;
        esco_eq_param.remain_en = 0;
        esco_eq_param.max_nsection = EQ_SECTION_MAX;
        esco_eq_param.cb = eq_phone_get_filter_info;
        esco_eq_param.eq_name = AUDIO_CALL_EQ_NAME;
        audio_eq_open(esco_eq, &esco_eq_param);
        audio_eq_set_samplerate(esco_eq, f.sample_rate);
        audio_eq_set_output_handle(esco_eq, esco_eq_output, dec);
        audio_eq_start(esco_eq);
    }
#endif


#if AUDIO_CODEC_SUPPORT_SYNC
    if (!audio_sync) {
        audio_sync = esco_play_sync_open(f.channel, f.sample_rate, f.sample_rate);
    }

    if (audio_sync) {
        audio_mixer_ch_set_event_handler(&dec->mix_ch, dec, bt_audio_mixer_ch_event_handler);
        audio_wireless_sync_info_init(audio_sync, f.sample_rate, f.sample_rate, f.channel);
    }
#endif


    audio_dac_set_delay_time(&dac_hdl, 30, 50);
    lmp_private_esco_suspend_resume(2);
    err = audio_decoder_start(&dec->decoder);
    if (err) {
        goto __err3;
    }
    audio_out_effect_dis = 1;//通话模式关闭高低音

    dec->start = 1;
    dec->remain = 0;


    err = audio_aec_init(f.sample_rate);
    if (err) {
        printf("audio_aec_init failed:%d", err);
        //goto __err3;
    }

#if TCFG_ESCO_DL_NS_ENABLE
    esco_dl_ns_open(f.sample_rate);
#endif/*TCFG_ESCO_DL_NS_ENABLE*/

    err = esco_enc_open(dec->coding_type, dec->esco_len);
    if (err) {
        printf("audio_enc_open failed:%d", err);
        //goto __err3;
    }

    dec->enc_start = 1; //该函数所在任务优先级低可能未open编码就开始解码，加入enc开始的标志防止解码过快输出
    printf("esco_dec_start ok\n");

    return 0;

__err3:
    audio_mixer_ch_close(&dec->mix_ch);
__err2:
    audio_decoder_close(&dec->decoder);
__err1:
    esco_dec_release();
    return err;
}

static int __esco_dec_res_close(void)
{
    if (!esco_dec->start) {
        return 0;
    }
    esco_dec->start = 0;
    esco_dec->enc_start = 0;

    audio_aec_close();
    esco_enc_close();
    app_audio_state_exit(APP_AUDIO_STATE_CALL);
    audio_decoder_close(&esco_dec->decoder);
    audio_mixer_ch_close(&esco_dec->mix_ch);
#if A2DP_RX_AND_AUDIO_DELAY
    audio_dac_set_delay_time(&dac_hdl, 20, AUDIO_DAC_DELAY_TIME);
#else
    audio_dac_set_delay_time(&dac_hdl, 30, AUDIO_DAC_DELAY_TIME);
#endif
#if AUDIO_CODEC_SUPPORT_SYNC
    if (audio_sync) {
        audio_wireless_sync_close(audio_sync);
        audio_sync = NULL;
        esco_dec->sync_step = 0;
        esco_dec->preempt_state = DEC_PREEMTED_BY_PRIORITY;
    }
#endif
#if TCFG_PHONE_EQ_ENABLE
    if (esco_eq) {
        audio_eq_close(esco_eq);
        free(esco_eq);
        esco_eq = NULL;
    }
#endif
    audio_out_effect_dis = 0;

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    audio_digital_vol_close(esco_dec->dvol);
    esco_dec->dvol = NULL;
#endif

#if TCFG_ESCO_DL_NS_ENABLE
    esco_dl_ns_close();
#endif/*TCFG_ESCO_DL_NS_ENABLE*/

    audio_plc_close();
#if TCFG_AUDIO_NOISE_GATE
    audio_noise_gate_close();
#endif/*TCFG_AUDIO_NOISE_GATE*/

    return 0;
}
static int esco_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;

    printf("esco_wait_res_handler:%d", event);
    if (event == AUDIO_RES_GET) {
        err = esco_dec_start();
    } else if (event == AUDIO_RES_PUT) {
        err = __esco_dec_res_close();
        lmp_private_esco_suspend_resume(1);
    }

    return err;
}

int esco_dec_open(void *param, u8 mute)
{
    int err;
    struct esco_dec_hdl *dec;
    u32 esco_param = *(u32 *)param;
    int esco_len = esco_param >> 16;
    int codec_type = esco_param & 0x000000ff;

#if TCFG_AUDIO_ANC_ENABLE
#ifdef CONFIG_ANC_OVERLAY
    if (anc_train_open_query()) {
        printf("anc_train_open,suspend esco_dec\n");
        lmp_private_esco_suspend_resume(1);
        return 0;
    }
#endif/*CONFIG_ANC_OVERLAY*/
#endif /*TCFG_AUDIO_ANC_ENABLE*/

    printf("esco_dec_open, type=%d,len=%d\n", codec_type, esco_len);

    dec = zalloc(sizeof(*dec));
    if (!dec) {
        return -ENOMEM;
    }
    esco_dec = dec;

    dec->esco_len = esco_len;
    if (codec_type == 3) {
        dec->coding_type = AUDIO_CODING_MSBC;
    } else if (codec_type == 2) {
        dec->coding_type = AUDIO_CODING_CVSD;
    }

    dec->tws_mute_en = mute;

    dec->wait.priority = 2;
    dec->wait.preemption = 1;
    dec->wait.handler = esco_wait_res_handler;
    err = audio_decoder_task_add_wait(&decode_task, &dec->wait);
    if (esco_dec && esco_dec->start == 0) {
        lmp_private_esco_suspend_resume(1);
    }

#if AUDIO_OUTPUT_AUTOMUTE
    mix_out_automute_skip(1);
#endif

    return err;
}


void esco_dec_close()
{
    if (!esco_dec) {
        return;
    }

    __esco_dec_res_close();
    esco_dec_release();

#if AUDIO_OUTPUT_AUTOMUTE
    mix_out_automute_skip(0);
#endif

#if (defined(TCFG_PHONE_MESSAGE_ENABLE) && (TCFG_PHONE_MESSAGE_ENABLE))
    phone_message_call_api_stop();
#endif

    audio_mixer_set_output_buf(&mixer, mix_buff, sizeof(mix_buff));
    puts("esco_dec_close: exit\n");
}


//////////////////////////////////////////////////////////////////////////////
u8 bt_audio_is_running(void)
{
    return (a2dp_dec || esco_dec);
}
u8 bt_media_is_running(void)
{
    return a2dp_dec != NULL;
}
u8 bt_phone_dec_is_running()
{
    return esco_dec != NULL;
}

static void audio_dac_trim_init(int arg)
{
    struct audio_dac_trim dac_trim;

    audio_dac_do_trim(&dac_hdl, &dac_trim, 0);
    syscfg_write(CFG_DAC_TRIM_INFO, (void *)&dac_trim, sizeof(dac_trim));

    audio_dac_set_trim_value(&dac_hdl, &dac_trim);
}

extern u32 read_capless_DTB(void);
static u8 audio_dec_inited = 0;
//////////////////////////////////////////////////////////////////////////////
int audio_dec_init()
{
    int err;

    printf("audio_dec_init\n");

#if TCFG_WAV_TONE_MIX_ENABLE
    wav_decoder_init();
#endif

    tone_play_init();

    /*os_sem_create(&dac_sem, 0);*/
    err = audio_decoder_task_create(&decode_task, "audio_dec");
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    audio_digital_vol_init();
#endif
    audio_dac_init(&dac_hdl, &dac_data);

#if TCFG_AUDIO_ANC_ENABLE
    audio_dac_anc_set(&dac_hdl, 1);
#endif
    u32 dacr32 = read_capless_DTB();

    audio_dac_set_capless_DTB(&dac_hdl, dacr32);
    audio_dac_set_buff(&dac_hdl, dac_buff, sizeof(dac_buff));
#if A2DP_RX_AND_AUDIO_DELAY
    audio_dac_set_delay_time(&dac_hdl, 20, AUDIO_DAC_DELAY_TIME);
#else
    audio_dac_set_delay_time(&dac_hdl, 30, AUDIO_DAC_DELAY_TIME);
#endif
    audio_dac_set_analog_vol(&dac_hdl, 0);

    request_irq(IRQ_AUDIO_IDX, 2, audio_irq_handler, 0);
    struct audio_dac_trim dac_trim;
    int len = syscfg_read(CFG_DAC_TRIM_INFO, (void *)&dac_trim, sizeof(dac_trim));
    if (len != sizeof(dac_trim)) {
#if 0 //DAC 异步trim
        int argv[8];
        argv[0] = (int)audio_dac_trim_init;
        argv[1] = 2;
        do {
            int err = os_taskq_post_type("audio_dec", Q_CALLBACK, 4, argv);
            if (err == OS_ERR_NONE) {
                break;
            }
            if (err != OS_Q_FULL) {
                break;
            }
            os_time_dly(2);
        } while (1);
#else
        audio_dac_trim_init(0);
#endif
    } else {
        audio_dac_set_trim_value(&dac_hdl, &dac_trim);
    }
    if (len != sizeof(dac_trim)) {
        audio_dac_do_trim(&dac_hdl, &dac_trim, 0);
        syscfg_write(CFG_DAC_TRIM_INFO, (void *)&dac_trim, sizeof(dac_trim));
    }
#if TCFG_MC_BIAS_AUTO_ADJUST
    mic_trim_run();
#endif/*TCFG_MC_BIAS_AUTO_ADJUST*/
    audio_dac_set_fade_handler(&dac_hdl, NULL, audio_fade_in_fade_out);
#if AUDIO_CODEC_SUPPORT_SYNC
    audio_dac_set_sync_buff(&dac_hdl, dac_sync_buff, sizeof(dac_sync_buff));
    audio_dac_set_sync_filt_buff(&dac_hdl, dac_sync_filt, sizeof(dac_sync_filt));
#endif

    /*硬件SRC模块滤波器buffer设置，可根据最大使用数量设置整体buffer*/
    audio_src_base_filt_init(audio_src_hw_filt, sizeof(audio_src_hw_filt));


    audio_mixer_open(&mixer);
    audio_mixer_set_handler(&mixer, &mix_handler);
    audio_mixer_set_event_handler(&mixer, mixer_event_handler);
    audio_mixer_set_output_buf(&mixer, mix_buff, sizeof(mix_buff));

#if AUDIO_OUTPUT_AUTOMUTE
    mix_out_automute_open();
#endif  //#if AUDIO_OUTPUT_AUTOMUTE

    audio_dec_inited = 1;
    return err;
}

static u8 audio_dec_init_complete()
{
    /*不支持Audio功能，返回idle*/
#if (defined TCFG_AUDIO_ENABLE && (TCFG_AUDIO_ENABLE == 0))
    return 1;
#endif/*TCFG_AUDIO_ENABLE*/
    if (!audio_dec_inited) {
        return 0;
    }

    return 1;
}
REGISTER_LP_TARGET(audio_dec_init_lp_target) = {
    .name = "audio_dec_init",
    .is_idle = audio_dec_init_complete,
};


/*----------------------------------------------------------------------------*/
/**@brief    环绕音效切换测试例子
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
#if AUDIO_SURROUND_CONFIG
void surround_switch_test(void *p)
{

    if (!p) {
        return;
    }

    static u8 cnt_type = 0;
    if (EFFECT_OFF == cnt_type) {
        //中途关开测试
        static u8 en = 0;
        en = !en;
        audio_surround_parm_update(p, cnt_type, (surround_update_parm *)en);
    } else {
        //音效切换测试
        audio_surround_parm_update(p, cnt_type, NULL);
    }
    printf("cnt_type 0x%x\n", cnt_type);
    if (++cnt_type > EFFECT_OFF) {
        cnt_type = EFFECT_3D_PANORAMA;
    }
}
#endif


/*----------------------------------------------------------------------------*/
/**@brief    虚拟低音参数更新例子
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/

void vbass_udate_parm_test(void *p)
{
    if (!p) {
        return;
    }
#if AUDIO_VBASS_CONFIG
    vbass_hdl *vbass = p;
    //
#if 0
    //虚拟低音增益调节例子
    static  u32 test_level  = 4096;
    if (vbass) {
        vbass_update_parm def_parm = {0};
        def_parm.bass_f = 300;
        test_level += 100;
        if (test_level > 16384) {
            test_level = 4096;
        }
        def_parm.level = test_level;
        audio_vbass_parm_update(vbass, VBASS_UPDATE_PARM, &def_parm);
    }
#endif
#if 0
    //开关虚拟低音例子
    static u8 en = 0;// 0关  1开
    en = !en;
    audio_vbass_parm_update(vbass, VBASS_SW, (void *)en);
#endif
#endif
}


