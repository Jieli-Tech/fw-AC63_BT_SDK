#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "classic/tws_api.h"
#include "classic/hci_lmp.h"
#include "effectrs_sync.h"
#include "application/eq_config.h"
#include "application/audio_energy_detect.h"
#include "app_config.h"
#include "audio_config.h"
#include "aec_user.h"
#include "audio_enc.h"
#include "app_main.h"
#include "btstack/avctp_user.h"
#include "btstack/a2dp_media_codec.h"
#include "tone_player.h"
#include "audio_syncts.h"
#include "audio_dec_eff.h"
#include "audio_codec_clock.h"
#include "audio_utils.h"
#if TCFG_USER_TWS_ENABLE
#include "bt_tws.h"
#endif/*TCFG_USER_TWS_ENABLE*/
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
#include "audio_digital_vol.h"
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/

#include "audio_plc.h"
#if TCFG_AUDIO_NOISE_GATE
#include "audio_noise_gate.h"
#endif/*TCFG_AUDIO_NOISE_GATE*/

#if TCFG_APP_FM_EMITTER_EN
#include "fm_emitter/fm_emitter_manage.h"
#endif

#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif/*TCFG_AUDIO_ANC_ENABLE*/

#include "phone_message/phone_message.h"



#ifdef SUPPORT_MS_EXTENSIONS
/* #pragma bss_seg(".audio_decoder_bss") */
/* #pragma data_seg(".audio_decoder_data") */
#pragma const_seg(".audio_decoder_const")
#pragma code_seg(".audio_decoder_code")
#endif

#ifdef CONFIG_256K_FLASH
#define CONFIG_MINI_BT_AUDIO_ENABLE     1
#else
#define CONFIG_MINI_BT_AUDIO_ENABLE     0
#endif

#define AUDIO_CODEC_SUPPORT_SYNC	1

#define A2DP_AUDIO_PLC_ENABLE       1

#if A2DP_AUDIO_PLC_ENABLE
#include "media/tech_lib/LFaudio_plc_api.h"
#endif

#define A2DP_RX_AND_AUDIO_DELAY     1

#define ESCO_DRC_EN	0  //通话下行增加限幅器处理，默认关闭

#if (!TCFG_DRC_ENABLE || !TCFG_PHONE_EQ_ENABLE)
#undef ESCO_DRC_EN 0
#define ESCO_DRC_EN	0
#endif

#if AUDIO_OUT_EFFECT_ENABLE
int audio_out_effect_open(void *priv, u16 sample_rate);
void audio_out_effect_close(void);
int audio_out_effect_stream_clear();
struct dec_eq_drc *audio_out_effect = NULL;
#endif /*AUDIO_OUT_EFFECT_ENABLE*/

static u8 audio_out_effect_dis = 0;
static int slience_samples;


struct tone_dec_hdl {
    struct audio_decoder decoder;
    void (*handler)(void *, int argc, int *argv);
    void *priv;
};

struct a2dp_dec_hdl {
    struct audio_decoder decoder;
    struct audio_res_wait wait;
#if AUDIO_OUT_MIXER_ENABLE
    struct audio_mixer_ch mix_ch;
#endif
    enum audio_channel channel;
    u8 start;
    u8 ch;
    s16 header_len;
    u8 remain;
    u8 eq_remain;
    u8 fetch_lock;
    u8 preempt;
    u8 stream_error;
    u8 new_frame;
    u8 repair;
    void *sample_detect;
    void *syncts;
    void *repair_pkt;
    s16 repair_pkt_len;
    u16 missed_num;
    u16 repair_frames;
    u16 overrun_seqn;
    u16 slience_frames;
#if AUDIO_CODEC_SUPPORT_SYNC
    u8 ts_start;
    u8 sync_step;
    void *ts_handle;
    u32 timestamp;
    u32 base_time;
#endif /*AUDIO_CODEC_SUPPORT_SYNC*/
#if A2DP_AUDIO_PLC_ENABLE
    LFaudio_PLC_API *plc_ops;
    void *plc_mem;
#endif /*A2DP_AUDIO_PLC_ENABLE*/
    u32 mix_ch_event_params[3];
    u32 pending_time;
    u16 seqn;
    u16 sample_rate;
    int timer;
    u32 coding_type;
    u16 delay_time;
    u16 detect_timer;
    u8  underrun_feedback;

#if TCFG_EQ_ENABLE&&TCFG_BT_MUSIC_EQ_ENABLE
    struct dec_eq_drc *eq_drc;
#endif//TCFG_BT_MUSIC_EQ_ENABLE

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    dvol_handle *dvol;
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/
};


struct esco_dec_hdl {
    struct audio_decoder decoder;
    struct audio_res_wait wait;
#if AUDIO_OUT_MIXER_ENABLE
    struct audio_mixer_ch mix_ch;
#endif
    u32 coding_type;
    u8 *frame;
    u8 frame_len;
    u8 offset;
    u8 data_len;
    u8 tws_mute_en;
    u8 start;
    u8 enc_start;
    u8 frame_time;
    u8 preempt;
    u8 repair;
    u16 slience_frames;
    void *syncts;
#if AUDIO_CODEC_SUPPORT_SYNC
    void *ts_handle;
    u8 ts_start;
    u8 sync_step;
    u8 preempt_state;
#endif
    u32 mix_ch_event_params[3];
    u8 esco_len;
    u8 frame_err_len;
    u8 remain;

    int data[15];/*ALIGNED(4)*/
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    dvol_handle *dvol;
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/

#if TCFG_EQ_ENABLE&&TCFG_PHONE_EQ_ENABLE
    struct dec_eq_drc *eq_drc;
#endif//TCFG_PHONE_EQ_ENABLE
    u32 hash;
};

#if AUDIO_OUTPUT_AUTOMUTE
void *mix_out_automute_hdl = NULL;
extern void mix_out_automute_open();
extern void mix_out_automute_close();
#endif  //#if AUDIO_OUTPUT_AUTOMUTE

void *audio_sync = NULL;

#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR)
#define MIX_POINTS_NUM  256
#else
#define MIX_POINTS_NUM  128
#endif
#if A2DP_RX_AND_AUDIO_DELAY
#define AUDIO_DAC_DELAY_TIME    30
#else
#define AUDIO_DAC_DELAY_TIME    50
#endif

#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR)
s16 dac_buff[6 * 1024];
#else
s16 dac_buff[2 * 1024 - 512];
#endif
#if AUDIO_CODEC_SUPPORT_SYNC
#define DEC_RUN_BY_ITSELF               0
#define DEC_PREEMTED_BY_PRIORITY        1
#define DEC_PREEMTED_BY_OUTSIDE         3
#define DEC_RESUME_BY_OUTSIDE           2
/* [>AAC解码需要加大同步和MIX的buffer用来提高异步效率<] */

s16 dac_sync_filt[24 * 2];
#endif

#if AUDIO_OUT_MIXER_ENABLE
s16 mix_buff[MIX_POINTS_NUM];
#endif

#define MAX_SRC_NUMBER      3
s16 audio_src_hw_filt[24 * 2 * MAX_SRC_NUMBER];

#define A2DP_TO_PCM_DEVICE_FORMAT           0//48000
#define A2DP_FLUENT_STREAM_MODE             1//流畅模式
#define A2DP_FLUENT_DETECT_INTERVAL         100000//ms 流畅播放延时检测时长
#if A2DP_FLUENT_STREAM_MODE
#define A2DP_MAX_PENDING_TIME               120
#else
#define A2DP_MAX_PENDING_TIME               40
#endif

#define A2DP_STREAM_NO_ERR                  0
#define A2DP_STREAM_UNDERRUN                1
#define A2DP_STREAM_OVERRUN                 2
#define A2DP_STREAM_MISSED                  3
#define A2DP_STREAM_DECODE_ERR              4
#define A2DP_STREAM_LOW_UNDERRUN            5

/*OS_SEM dac_sem;*/
static u16 dac_idle_delay_max = 10000;
static u16 dac_idle_delay_cnt = 10000;
static struct tone_dec_hdl *tone_dec = NULL;
struct audio_decoder_task decode_task;
struct audio_dac_hdl dac_hdl;
#if AUDIO_OUT_MIXER_ENABLE
struct audio_mixer mixer;
#endif
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
extern const int const_surround_en;

struct a2dp_dec_hdl *a2dp_dec = NULL;
struct esco_dec_hdl *esco_dec = NULL;

extern int platform_device_sbc_init();

int lmp_private_get_esco_remain_buffer_size();
int lmp_private_get_esco_data_len();
void *lmp_private_get_esco_packet(int *len, u32 *hash);
void lmp_private_free_esco_packet(void *packet);
extern int lmp_private_esco_suspend_resume(int flag);
extern int bt_audio_sync_nettime_select(u8 basetime);
extern u32 bt_audio_sync_lat_time(void);
void audio_syncts_add(void *priv);
void audio_syncts_del(void *priv);
void audio_output_open(void *priv, u32 sample_rate);
int audio_dac_start_pre(struct audio_dac_hdl *dac);

void *a2dp_play_sync_open(u8 channel, u32 sample_rate, u32 output_rate, u32 coding_type);

void *esco_play_sync_open(u8 channel, u16 sample_rate, u16 output_rate);
//void audio_adc_init(void);
void *audio_adc_open(void *param, const char *source);
int audio_adc_sample_start(void *adc);
void audio_fade_in_fade_out(u8 left_gain, u8 right_gain);

void set_source_sample_rate(u16 sample_rate);
u8 bt_audio_is_running(void);

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
    /*putchar('k');*/
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
 * DA输出源启动后可使用DAC irq做唤醒，省去hi_timer
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
#if TCFG_AUDIO_OUTPUT_IIS
        /*唤醒源为输出目标中断*/
        if (!audio_iis_is_working()) {
            return audio_decoder_wakeup_select(0);
        }

        int err = audio_iis_irq_resume_decoder(a2dp_low_latency ? 2 : AUDIO_DECODE_TASK_WAKEUP_TIME, NULL, audio_decoder_wakeup_timer);
#else
        /*唤醒源为输出目标中断*/
        if (!audio_dac_is_working(&dac_hdl)) {
            return audio_decoder_wakeup_select(0);
        }

        /*int err = audio_dac_set_irq_time(&dac_hdl, a2dp_low_latency ? 2 : AUDIO_DECODE_TASK_WAKEUP_TIME);*/
        int err = audio_dac_irq_enable(&dac_hdl, a2dp_low_latency ? 2 : AUDIO_DECODE_TASK_WAKEUP_TIME,
                                       NULL, audio_decoder_wakeup_timer);
#endif
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

static int _audio_output_handler(s16 *data, u16 len)
{

    int rlen = len;
    int wlen = 0;

    /*os_sem_set(&dac_sem, 0);*/
    /* audio_aec_refbuf(data, len); */
#if AUDIO_OUTPUT_AUTOMUTE
    audio_energy_detect_run(mix_out_automute_hdl, data, len);
#endif  //#if AUDIO_OUTPUT_AUTOMUTE

#if TCFG_AUDIO_OUTPUT_IIS
    u8 dac_connect_mode = audio_dac_get_channel(&dac_hdl);
    if (dac_connect_mode == DAC_OUTPUT_LR) {
        wlen = audio_iis_output_write(data, len);
    } else {
        s16 *mono_data = (s16 *)data;
        s16 point_num = 0;
        u16 remain_points = (len >> 1);
        s16 two_ch_data[IIS_MONO_TO_DUAL_POINTS * 2];

        do {
            point_num = IIS_MONO_TO_DUAL_POINTS;
            if (point_num >= remain_points) {
                point_num = remain_points;
            }
            audio_pcm_mono_to_dual(two_ch_data, mono_data, point_num);
            int tmp_len = audio_iis_output_write(two_ch_data, point_num << 2);
            wlen += tmp_len;
            remain_points -= (tmp_len >> 2);
            if (tmp_len < (point_num << 2)) {
                break;
            }
            mono_data += point_num;
        } while (remain_points);
        wlen = wlen >> 1;
    }

#elif TCFG_APP_FM_EMITTER_EN
    fm_emitter_cbuf_write((u8 *)data, len);
#else
    wlen = audio_dac_write(&dac_hdl, data, len);
#endif

    return wlen;
}


static int audio_output_handler(s16 *data, u16 len)
{
#if AUDIO_OUT_MIXER_ENABLE
    return _audio_output_handler(data, len);
#else
    int wlen = 0;
    if (slience_samples) {
        u16 points = ARRAY_SIZE(dac_buff);
        int slience_len = (slience_samples < points ? slience_samples : points) * 2;
        wlen = _audio_output_handler(dac_buff, slience_len);
        slience_samples -= wlen / 2;
        wlen = 0;
    }
    if (slience_samples == 0) {
        wlen = _audio_output_handler(data, len);
    }
    return wlen;

#endif
}

#if AUDIO_OUT_EFFECT_ENABLE
static int audio_output_effect_handler(s16 *data, u16 len)
{
    if (!audio_out_effect) {
        return audio_output_handler(data, len);
    }

    if (audio_out_effect && audio_out_effect->async) {
        return eq_drc_run(audio_out_effect, data, len);
    }

    if (!audio_out_effect->remain) {
        if (audio_out_effect && !audio_out_effect->async) {
            eq_drc_run(audio_out_effect, data, len);
        }
    }
    int wlen = audio_output_handler(data, len);
    if (wlen == len) {
        audio_out_effect->remain = 0;
    } else {
        audio_out_effect->remain = 1;
    }
    return wlen;
}
#endif

int audio_output_data(s16 *data, u16 len)
{
#if AUDIO_OUT_EFFECT_ENABLE
    return audio_output_effect_handler(data, len);
#else
    return audio_output_handler(data, len);
#endif
}
int audio_output_data_a2dp(void *p, s16 *data, u16 len)
{
#if !AUDIO_OUT_MIXER_ENABLE
    struct a2dp_dec_hdl *dec = p;
    if (dec->start == 1) {
        dec->start = 2;
        audio_output_open(NULL, dec->sample_rate);
        audio_syncts_add(dec->mix_ch_event_params);
    }
#endif
    return audio_output_data(data, len);
}

int audio_output_data_esco(void *p, s16 *data, u16 len)
{
#if !AUDIO_OUT_MIXER_ENABLE
    struct esco_dec_hdl *dec = (struct esco_dec_hdl *)p;
    if (dec->start == 1) {
        dec->start = 2;
        audio_output_open(NULL, dec->decoder.fmt.sample_rate);
        audio_syncts_add(dec->mix_ch_event_params);
    }
#endif
    return audio_output_data(data, len);
}



#if AUDIO_OUTPUT_AUTOMUTE

void audio_mix_out_automute_mute(u8 mute)
{
    printf(">>>>>>>>>>>>>>>>>>>> %s\n", mute ? ("MUTE") : ("UNMUTE"));
    if (mute) {
        app_audio_mute(AUDIO_MUTE_DEFAULT);
    } else {
        app_audio_mute(AUDIO_UNMUTE_DEFAULT);
    }
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

#if AUDIO_OUT_EFFECT_ENABLE

int audio_out_eq_spec_set_gain(u8 idx, int gain)
{
    if (!audio_out_effect) {
        return -1;
    }
    audio_out_eq_set_gain(audio_out_effect, idx, gain);

    return 0;
}

static int mix_output_handler(struct audio_mixer *mixer, s16 *data, u16 len);
static void audio_out_effect_close(void)
{
    if (!audio_out_effect) {
        return ;
    }
    audio_out_eq_drc_free(audio_out_effect);
    audio_out_effect = NULL;
}

static int audio_out_effect_stream_clear()
{
    if (!audio_out_effect) {
        return 0;
    }

    if (audio_out_effect->eq && audio_out_effect->async) {
        audio_eq_async_data_clear(audio_out_effect->eq);
    }

    return 0;
}

static int audio_out_effect_open(void *priv, u16 sample_rate)
{
    audio_out_effect_close();

    u8 ch_num;
#if TCFG_APP_FM_EMITTER_EN
    ch_num = 2;
#else
    u8 dac_connect_mode = audio_dac_get_channel(&dac_hdl);
    if (dac_connect_mode == DAC_OUTPUT_LR) {
        ch_num =  2;
    } else {
        ch_num =  1;
    }
#endif//TCFG_APP_FM_EMITTER_EN

    audio_out_effect = audio_out_eq_drc_setup(priv, (eq_output_cb)mix_output_handler, sample_rate, ch_num, 1, 0);
    return 0;
}


#if AUDIO_OUT_MIXER_ENABLE
static int mix_output_effect_handler(struct audio_mixer *mixer, s16 *data, u16 len)
{
    if (!audio_out_effect) {
        return mix_output_handler(mixer, data, len);
    }

    if (audio_out_effect && audio_out_effect->async) {
        return eq_drc_run(audio_out_effect, data, len);
    }

    if (!audio_out_effect->remain) {
        if (audio_out_effect && !audio_out_effect->async) {
            eq_drc_run(audio_out_effect, data, len);
        }
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
#endif/*AUDIO_OUT_EFFECT_ENABLE*/

extern void audio_adc_mic_demo(u8 mic_idx, u8 gain, u8 mic_2_dac);
extern void dac_analog_power_control(u8 en);
extern void audio_dac_output_enable(struct audio_dac_hdl *dac);
extern void audio_dac_output_disable(struct audio_dac_hdl *dac);

u8 audio_fast_mode = 0;
void audio_fast_mode_test()
{
    audio_fast_mode = 1;
    audio_dac_set_volume(&dac_hdl, app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
    dac_analog_power_control(1);////将关闭基带，不开可发现，不可连接
    audio_dac_start(&dac_hdl);
    audio_adc_mic_demo(AUDIO_ADC_MIC_CH, 10, 1);

}

u8 dac_power_off_flag = 1;
u8 is_dac_power_off()
{
    if (a2dp_dec || esco_dec || get_call_status() != BT_CALL_HANGUP) {
        return 1;
    }
    return dac_power_off_flag;
}

void dac_analog_power_control(u8 en)
{
#ifdef CONFIG_CURRENT_SOUND_DEAL_ENABLE
    if (en) {
        dac_power_off_flag = 0;
        audio_dac_output_enable(&dac_hdl);
    } else {
#if TCFG_AUDIO_ANC_ENABLE
        if (!anc_status_get()) {
            puts("anc dac_power_off");
            /*dac_power_off();*/
            audio_dac_output_disable(&dac_hdl);
        }
#else
        puts("dac_power_off");
        audio_dac_output_disable(&dac_hdl);
        /*dac_power_off();*/
#endif
        dac_power_off_flag = 1;
    }
#endif
}
int audio_dac_add_slience_samples(int samples)
{
    slience_samples = samples;
    return 0;
}

void audio_output_open(void *priv, u32 sample_rate)
{
    slience_samples = 0;
#if AUDIO_OUTPUT_AUTOMUTE
    audio_energy_detect_sample_rate_update(mix_out_automute_hdl, sample_rate);
#endif  //#if AUDIO_OUTPUT_AUTOMUTE

#if TCFG_AUDIO_OUTPUT_IIS
    audio_iis_output_set_srI(audio_mixer_get_sample_rate(priv));
    audio_iis_output_start(1);
#elif TCFG_APP_FM_EMITTER_EN

#else
    if (A2DP_TO_PCM_DEVICE_FORMAT && a2dp_dec && a2dp_dec->start) {
        audio_dac_set_sample_rate(&dac_hdl, A2DP_TO_PCM_DEVICE_FORMAT);
    } else {
        audio_dac_set_sample_rate(&dac_hdl, sample_rate);
    }
    audio_dac_set_volume(&dac_hdl, app_audio_get_volume(APP_AUDIO_CURRENT_STATE));
    audio_dac_start(&dac_hdl);
    dac_analog_power_control(1);
#if AUDIO_OUT_MIXER_ENABLE
    g_printf("MIXER_EVENT_CH_OPEN=%d\n", dac_power_off_flag);
    audio_dac_set_input_correct_callback(&dac_hdl,
                                         (void *)priv,
                                         (void (*)(void *, int))audio_mixer_position_correct);
#endif

#endif
#if AUDIO_OUT_EFFECT_ENABLE
    if (!audio_out_effect_dis) {
        audio_out_effect_open(priv, sample_rate);
    }
#endif
}

void audio_output_close(void *priv)
{
    audio_dac_set_input_correct_callback(&dac_hdl, NULL, NULL);
#if AUDIO_OUT_EFFECT_ENABLE
    audio_out_effect_close();
#endif

#if TCFG_AUDIO_OUTPUT_IIS
    audio_iis_output_stop();
#elif TCFG_APP_FM_EMITTER_EN

#else
    audio_dac_set_protect_time(&dac_hdl, 0, NULL, NULL);
    audio_dac_stop(&dac_hdl);
    dac_analog_power_control(0);
    g_printf("MIXER_EVENT_CH_CLOSE=%d\n", dac_power_off_flag);
    if (audio_fast_mode) {
        audio_fast_mode_test();
    }
#endif
}

#if AUDIO_OUT_MIXER_ENABLE
static void mixer_event_handler(struct audio_mixer *mixer, int event)
{
    switch (event) {
    case MIXER_EVENT_CH_OPEN:
        if (audio_mixer_get_start_ch_num(mixer) == 1) {
            audio_output_open(mixer, audio_mixer_get_sample_rate(mixer));
        }
        break;
    case MIXER_EVENT_CH_CLOSE:
        if (audio_mixer_get_ch_num(mixer) == 0) {
            audio_output_close(mixer);
        }
        break;
    }
}
#endif

static int mix_probe_handler(struct audio_mixer *mixer)
{
    return 0;
}

#define IIS_MONO_TO_DUAL_POINTS  64
static inline void audio_pcm_mono_to_dual(s16 *dual_pcm, s16 *mono_pcm, int points);
static int mix_output_handler(struct audio_mixer *mixer, s16 *data, u16 len)
{
    return audio_output_handler(data, len);
}

static const struct audio_mix_handler mix_handler  = {
    .mix_probe  = mix_probe_handler,

#if AUDIO_OUT_EFFECT_ENABLE
    .mix_output = mix_output_effect_handler,
#else
    .mix_output = mix_output_handler,
#endif
};

void audio_syncts_add(void *priv)
{
    if (priv) {
        u32 *params = (u32 *)priv;
        struct audio_mixer_ch *ch = (struct audio_mixer_ch *)params[0];
        u32 base_time = params[2];
#if defined(TCFG_AUDIO_OUTPUT_IIS) && TCFG_AUDIO_OUTPUT_IIS
        audio_iis_add_syncts_handle((void *)params[1]);
#else
        audio_dac_add_syncts_handle(&dac_hdl, (void *)params[1]);
#endif
        u32 current_time = (bt_audio_sync_lat_time() * 625 * TIME_US_FACTOR);
        u32 time_diff = ((base_time - current_time) & 0xffffffff) / TIME_US_FACTOR;
        /*printf("-----base time : %u, current_time : %u------\n", base_time, current_time);*/
        if (time_diff < 500000) {
            int buf_frame = audio_dac_buffered_frames(&dac_hdl);
#if defined(TCFG_AUDIO_OUTPUT_IIS) && TCFG_AUDIO_OUTPUT_IIS
            buf_frame = audio_iis_buffered_frames();
#endif

#if AUDIO_OUT_MIXER_ENABLE
            int sample_rate = audio_mixer_get_sample_rate(&mixer);
#else
            int sample_rate = audio_dac_get_sample_rate(&dac_hdl);
#endif
            int slience_frames = (u64)time_diff * sample_rate / 1000000 - buf_frame;
            if (slience_frames < 0) {
                slience_frames = 0;
            }
            /*printf("-------slience_frames : %d-------\n", slience_frames);*/
            sound_pcm_update_frame_num((void *)params[1], -slience_frames);
#if AUDIO_OUT_MIXER_ENABLE
            audio_mixer_ch_add_slience_samples(ch, slience_frames * dac_hdl.channel);
#else
            audio_dac_add_slience_samples(slience_frames * dac_hdl.channel);
#endif
        }
    }
}

void audio_syncts_del(void *priv)
{
    if (priv) {
        u32 *params = (u32 *)priv;
#if defined(TCFG_AUDIO_OUTPUT_IIS) && TCFG_AUDIO_OUTPUT_IIS
        audio_iis_remove_syncts_handle((void *)params[1]);
#else
        audio_dac_remove_syncts_handle(&dac_hdl, (void *)params[1]);
#endif
    }
}
void audio_mix_ch_event_handler(void *priv, int event)
{
    switch (event) {
    case MIXER_EVENT_CH_OPEN:
        audio_syncts_add(priv);
        break;
    case MIXER_EVENT_CH_CLOSE:
        audio_syncts_del(priv);
        break;
    }
}

#define RB16(b)    (u16)(((u8 *)b)[0] << 8 | (((u8 *)b))[1])
#define RB32(b)    (u32)(((u8 *)b)[0] << 24 | (((u8 *)b))[1] << 16 | (((u8 *)b))[2] << 8 | (((u8 *)b))[3])

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

#if 0
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
#else
    while (1) {
        len = a2dp_media_try_get_packet(&frame);
        if (len <= 0) {
            break;
        }
        a2dp_media_free_packet(frame);
    }

#endif

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
u8 get_a2dp_drop_frame_flag()
{
    if (a2dp_dec) {
        return a2dp_dec->timer;
    }
    return 0;
}

void a2dp_drop_frame_start()
{
    if (a2dp_dec && (a2dp_dec->timer == 0)) {
        a2dp_dec->timer = sys_timer_add(NULL, __a2dp_drop_frame, 50);
        a2dp_tws_audio_conn_offline();
    }
}

void a2dp_drop_frame_stop()
{
    if (a2dp_dec && a2dp_dec->timer) {
        sys_timer_del(a2dp_dec->timer);
        a2dp_tws_audio_conn_delete();
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

#if TCFG_APP_FM_EMITTER_EN
    channel = AUDIO_CH_LR;
#endif
    if (channel != dec->channel) {
        printf("set_channel: %d\n", channel);
        audio_decoder_set_output_channel(&dec->decoder, channel);
        dec->channel = channel;
#if TCFG_EQ_ENABLE&&TCFG_BT_MUSIC_EQ_ENABLE
        if (dec->eq_drc && dec->eq_drc->eq) {
            audio_eq_set_channel(dec->eq_drc->eq, dec->ch);
        }
#endif
    }
}

/*
 *
 */
static int a2dp_decoder_set_timestamp(struct a2dp_dec_hdl *dec, u16 seqn)
{
#if AUDIO_CODEC_SUPPORT_SYNC
    u32 timestamp;

    timestamp = a2dp_audio_update_timestamp(dec->ts_handle, seqn, audio_syncts_get_dts(dec->syncts));
    if (!dec->ts_start) {
        dec->ts_start = 1;
        dec->mix_ch_event_params[2] = timestamp;
    } else {
        audio_syncts_next_pts(dec->syncts, timestamp);
        audio_syncts_update_sample_rate(dec->syncts, a2dp_audio_sample_rate(dec->ts_handle));;
    }
    dec->timestamp = timestamp;
    /*printf("timestamp : %d, %d\n", seqn, timestamp / TIME_US_FACTOR / 625);*/
#endif
    return 0;
}

static bool a2dp_audio_is_underrun(struct a2dp_dec_hdl *dec)
{
#if AUDIO_CODEC_SUPPORT_SYNC
    if (dec->ts_start != 2) {
        return false;
    }
#endif
    int underrun_time = a2dp_low_latency ? 1 : 20;
#if defined(TCFG_AUDIO_OUTPUT_IIS) && TCFG_AUDIO_OUTPUT_IIS
    if (audio_iis_data_time() < underrun_time) {
        return true;
    }
#else
    if (audio_dac_data_time(&dac_hdl) < underrun_time) {
        return true;
    }
#endif
    return false;
}

static bool a2dp_bt_rx_overrun(void)
{
    return a2dp_media_get_remain_buffer_size() < 768 ? true : false;
}

static void a2dp_decoder_stream_free(struct a2dp_dec_hdl *dec, void *packet)
{
    if (packet) {
        a2dp_media_free_packet(packet);
    }

    if ((void *)packet == (void *)dec->repair_pkt) {
        dec->repair_pkt = NULL;
    }

    if (dec->repair_pkt) {
        a2dp_media_free_packet(dec->repair_pkt);
        dec->repair_pkt = NULL;
    }
    dec->repair_pkt_len = 0;
}

static void a2dp_stream_underrun_feedback(void *priv);
static int a2dp_audio_delay_time(struct a2dp_dec_hdl *dec);
#define a2dp_seqn_before(a, b)  ((a < b && (u16)(b - a) < 1000) || (a > b && (u16)(a - b) > 1000))
static int a2dp_buffered_stream_sample_rate(struct a2dp_dec_hdl *dec, u8 *from_packet, u16 *end_seqn)
{
    u8 *packet = from_packet;
    int len = 0;
    int sample_rate = 0;

    if (!dec->sample_detect) {
        return dec->sample_rate;
    }

    a2dp_frame_sample_detect_start(dec->sample_detect, a2dp_media_dump_rx_time(packet));
    while (1) {
        packet = a2dp_media_fetch_packet(&len, packet);
        if (!packet) {
            break;
        }
        sample_rate = a2dp_frame_sample_detect(dec->sample_detect, packet, len, a2dp_media_dump_rx_time(packet));
        *end_seqn = RB16(packet + 2);
    }

    /*printf("A2DP sample detect : %d - %d\n", sample_rate, dec->sample_rate);*/
    return sample_rate;
}

static int a2dp_stream_overrun_handler(struct a2dp_dec_hdl *dec, u8 **frame, int *len)
{
    u8 *packet = NULL;
    int rlen = 0;

    int msecs = 0;
    int overrun = 0;
    int sample_rate = 0;
    u16 from_seqn = RB16(dec->repair_pkt + 2);
    u16 seqn = from_seqn;
    u16 end_seqn = 0;
    overrun = 1;
    while (1) {
        msecs = a2dp_audio_delay_time(dec);
        if (msecs < (CONFIG_A2DP_DELAY_TIME + 50) && !a2dp_bt_rx_overrun()) {
            break;
        }
        overrun = 0;
        rlen = a2dp_media_try_get_packet(&packet);
        if (rlen <= 0) {
            break;
        }

        sample_rate = a2dp_buffered_stream_sample_rate(dec, dec->repair_pkt, &end_seqn);
        a2dp_decoder_stream_free(dec, NULL);
        dec->repair_pkt = packet;
        dec->repair_pkt_len = rlen;
        seqn = RB16(packet + 2);
        if (!a2dp_seqn_before(seqn, dec->overrun_seqn)) {
            *frame = packet;
            *len = rlen;
            /*printf("------> end frame : %d\n", dec->overrun_seqn);*/
            return 1;
        }

        if (/*sample_rate < (dec->sample_rate * 4 / 5) || */sample_rate > (dec->sample_rate * 4 / 3)) {
            if (a2dp_seqn_before(dec->overrun_seqn, end_seqn)) {
                dec->overrun_seqn = end_seqn;
            }
            continue;
        }
    }
    if (overrun) {
        /*putchar('+');*/
        /* dec->overrun_seqn++; */
    } else {
        /*putchar('-');*/
    }

    *frame = dec->repair_pkt;
    *len = dec->repair_pkt_len;
    return 0;
}

static int a2dp_stream_missed_handler(struct a2dp_dec_hdl *dec, u8 **frame, int *len)
{
    int msecs = a2dp_audio_delay_time(dec);
    *frame = dec->repair_pkt;
    *len = dec->repair_pkt_len;
    if ((msecs >= (dec->delay_time + 50) || a2dp_bt_rx_overrun()) || --dec->missed_num == 0) {
        /*putchar('M');*/
        return 1;
    }
    /*putchar('m');*/
    return 0;
}

static int a2dp_stream_underrun_handler(struct a2dp_dec_hdl *dec, u8 **packet)
{

    if (!a2dp_audio_is_underrun(dec)) {
        putchar('x');
        return 0;
    }
    putchar('X');
    if (dec->stream_error != A2DP_STREAM_UNDERRUN) {
        if (!dec->stream_error) {
            a2dp_stream_underrun_feedback(dec);
        }
        dec->stream_error = a2dp_low_latency ? A2DP_STREAM_LOW_UNDERRUN : A2DP_STREAM_UNDERRUN;
        dec->repair = a2dp_low_latency ? 0 : 1;
    }
    *packet = dec->repair_pkt;
    dec->repair_frames++;
    return dec->repair_pkt_len;
}

static int a2dp_stream_error_filter(struct a2dp_dec_hdl *dec, u8 new_packet, u8 *packet, int len)
{
    int err = 0;

    if (dec->coding_type == AUDIO_CODING_AAC) {
        dec->header_len = get_rtp_header_len(dec->new_frame, packet, len);
        dec->new_frame = 0;
    } else {
        dec->header_len = get_rtp_header_len(1, packet, len);
    }

    if (dec->header_len >= len) {
        printf("##A2DP header error : %d\n", dec->header_len);
        a2dp_decoder_stream_free(dec, packet);
        return -EFAULT;
    }

    u16 seqn = RB16(packet + 2);
    if (new_packet) {
        if (dec->stream_error == A2DP_STREAM_UNDERRUN) {
            int missed_frames = (u16)(seqn - dec->seqn) - 1;
            if (missed_frames > dec->repair_frames) {
                dec->stream_error = A2DP_STREAM_MISSED;
                dec->missed_num = missed_frames - dec->repair_frames + 1;
                /*printf("case 0 : %d, %d\n", missed_frames, dec->repair_frames);*/
                err = -EAGAIN;
            } else if (missed_frames < dec->repair_frames) {
                dec->stream_error = A2DP_STREAM_OVERRUN;
                dec->overrun_seqn = seqn + dec->repair_frames - missed_frames;
                /*printf("case 1 : %d, %d, seqn : %d, %d\n", missed_frames, dec->repair_frames, seqn, dec->overrun_seqn); */
                err = -EAGAIN;
            }
        } else if (!dec->stream_error && (u16)(seqn - dec->seqn) > 1) {
            dec->stream_error = A2DP_STREAM_MISSED;
            if (a2dp_audio_delay_time(dec) < dec->delay_time) {
                dec->missed_num = (u16)(seqn - dec->seqn);
                err = -EAGAIN;
            }
            int pkt_len;
            void *head = a2dp_media_fetch_packet(&pkt_len, NULL);
            /*printf("case 2 : %d, %d, pkt : 0x%x, 0x%x\n", seqn, dec->seqn, (u32)head, (u32)packet);*/
            if (dec->missed_num > 30) {
                printf("##A serious mistake : A2DP stream missed too much, %d\n", dec->missed_num);
                dec->missed_num = 30;
            }
        }
        dec->repair_frames = 0;
    }
    if (!err && new_packet) {
        dec->seqn = seqn;
    }
    dec->repair_pkt = packet;
    dec->repair_pkt_len = len;
    return err;
}

static int a2dp_dec_get_frame(struct audio_decoder *decoder, u8 **frame)
{
    struct a2dp_dec_hdl *dec = container_of(decoder, struct a2dp_dec_hdl, decoder);
    u8 *packet = NULL;
    int len = 0;
    u8 new_packet = 0;

try_again:
    switch (dec->stream_error) {
    case A2DP_STREAM_OVERRUN:
        new_packet = a2dp_stream_overrun_handler(dec, &packet, &len);
        break;
    case A2DP_STREAM_MISSED:
        new_packet = a2dp_stream_missed_handler(dec, &packet, &len);
        break;
    default:
        len = a2dp_media_try_get_packet(&packet);
        if (len <= 0) {
            len = a2dp_stream_underrun_handler(dec, &packet);
        } else {
            a2dp_decoder_stream_free(dec, NULL);
            new_packet = 1;
        }
        break;
    }

    if (len <= 0) {
        return 0;
    }

    int err = a2dp_stream_error_filter(dec, new_packet, packet, len);
    if (err) {
        if (-err == EAGAIN) {
            dec->new_frame = 1;
            goto try_again;
        }
        return 0;
    }

    *frame = packet + dec->header_len;
    len -= dec->header_len;
    if (dec->stream_error && new_packet) {
#if AUDIO_CODEC_SUPPORT_SYNC && TCFG_USER_TWS_ENABLE
        if (dec->ts_handle) {
            tws_a2dp_share_timestamp(dec->ts_handle);
        }
#endif
        dec->stream_error = 0;
    }

    if (dec->slience_frames) {
        dec->slience_frames--;
    }
    a2dp_decoder_set_timestamp(dec, dec->seqn);

    return len;
}

static void a2dp_dec_put_frame(struct audio_decoder *decoder, u8 *frame)
{
    struct a2dp_dec_hdl *dec = container_of(decoder, struct a2dp_dec_hdl, decoder);

    if (frame) {
        if (!a2dp_media_channel_exist() || app_var.goto_poweroff_flag) {
            a2dp_decoder_stream_free(dec, (void *)(frame - dec->header_len));
        }
        /*a2dp_media_free_packet((void *)(frame - dec->header_len));*/
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


#define bt_time_before(t1, t2) \
         (((t1 < t2) && ((t2 - t1) & 0x7ffffff) < 0xffff) || \
          ((t1 > t2) && ((t1 - t2) & 0x7ffffff) > 0xffff))
#define bt_time_to_msecs(clk)   (((clk) * 625) / 1000)
#define msecs_to_bt_time(m)     (((m + 1)* 1000) / 625)

static int a2dp_audio_delay_time(struct a2dp_dec_hdl *dec)
{
    /*struct a2dp_dec_hdl *dec = container_of(decoder, struct a2dp_dec_hdl, decoder);*/
    int msecs = 0;

#if TCFG_USER_TWS_ENABLE
    msecs = a2dp_media_get_remain_play_time(1);
#else
    msecs = a2dp_media_get_remain_play_time(0);
#endif

    if (dec->syncts) {
        msecs += sound_buffered_between_syncts_and_device(dec->syncts, 0);
    }

#if defined(TCFG_AUDIO_OUTPUT_IIS) && TCFG_AUDIO_OUTPUT_IIS
    msecs += audio_iis_data_time();
#else
    msecs += audio_dac_data_time(&dac_hdl);
#endif

    return msecs;
}

static int a2dp_decoder_stream_delay_update(struct audio_decoder *decoder)
{
    struct a2dp_dec_hdl *dec = container_of(decoder, struct a2dp_dec_hdl, decoder);
    int msecs = 0;
    int err = 0;

#if AUDIO_CODEC_SUPPORT_SYNC
    msecs = a2dp_audio_delay_time(dec);
    if (dec->stream_error) {
        return 0;
    }

    if (dec->sync_step == 2) {
        int distance_time = msecs - a2dp_delay_time;
        if (a2dp_bt_rx_overrun() && distance_time < 50) {
            distance_time = 50;
        }

        if (dec->ts_handle) {
            a2dp_audio_delay_offset_update(dec->ts_handle, distance_time);
        }
    }

#endif
    /*printf("%d -> %dms, delay_time : %dms\n", msecs1, msecs, a2dp_delay_time);*/
    return 0;
}

/*
 * A2DP 音频同步控制处理函数
 * 1.包括音频延时浮动参数;
 * 2.处理因为超时等情况丢弃音频样点;
 * 3.调用与蓝牙主机音频延时做同步的功能;
 * 4.处理TWS从机加入与解码被打断的情况。
 *
 */
static int a2dp_decoder_audio_sync_handler(struct audio_decoder *decoder)
{
    struct a2dp_dec_hdl *dec = container_of(decoder, struct a2dp_dec_hdl, decoder);
    int err;

    if (!dec->syncts) {
        return 0;
    }


    err = a2dp_decoder_stream_delay_update(decoder);
    if (err) {
        audio_decoder_suspend(decoder, 0);
        return -EAGAIN;
    }

    return 0;
}

static u16 a2dp_max_interval = 0;
#define A2DP_EST_AUDIO_CAPACITY             350//ms

static void a2dp_stream_underrun_feedback(void *priv)
{
    struct a2dp_dec_hdl *dec = (struct a2dp_dec_hdl *)priv;

    dec->underrun_feedback = 1;

    if (a2dp_delay_time < a2dp_max_interval + 50) {
        a2dp_delay_time = a2dp_max_interval + 50;
    } else {
        a2dp_delay_time += 50;
    }
    if (a2dp_delay_time > A2DP_EST_AUDIO_CAPACITY) {
        a2dp_delay_time = A2DP_EST_AUDIO_CAPACITY;
    }
}

/*void a2dp_stream_interval_time_handler(int time)*/
void reset_a2dp_sbc_instant_time(u16 time)
{
    if (a2dp_max_interval < time) {
        a2dp_max_interval = time;
        if (a2dp_max_interval > 350) {
            a2dp_max_interval = 350;
        }
#if A2DP_FLUENT_STREAM_MODE
        if (a2dp_max_interval > a2dp_delay_time) {
            a2dp_delay_time = a2dp_max_interval + 5;
            if (a2dp_delay_time > A2DP_EST_AUDIO_CAPACITY) {
                a2dp_delay_time = A2DP_EST_AUDIO_CAPACITY;
            }
        }
#endif
        /*printf("Max : %dms\n", time);*/
    }
}

static void a2dp_stream_stability_detect(void *priv)
{
    struct a2dp_dec_hdl *dec = (struct a2dp_dec_hdl *)priv;

    if (dec->underrun_feedback) {
        dec->underrun_feedback = 0;
        return;
    }

    if (a2dp_delay_time > dec->delay_time) {
        if (a2dp_max_interval < a2dp_delay_time) {
            a2dp_delay_time -= 50;
            if (a2dp_delay_time < dec->delay_time) {
                a2dp_delay_time = dec->delay_time;
            }

            if (a2dp_delay_time < a2dp_max_interval) {
                a2dp_delay_time = a2dp_max_interval + 5;
            }
        }
        a2dp_max_interval = dec->delay_time;
    }
}

#if AUDIO_CODEC_SUPPORT_SYNC
static void a2dp_decoder_update_base_time(struct a2dp_dec_hdl *dec)
{
    int distance_time = a2dp_low_latency ? a2dp_delay_time : (a2dp_delay_time - a2dp_media_get_remain_play_time(1));
    if (!a2dp_low_latency) {
        distance_time = a2dp_delay_time;
    } else if (distance_time < 20) {
        distance_time = 20;
    }
    dec->base_time = bt_audio_sync_lat_time() + msecs_to_bt_time(distance_time);
}

#endif
static int a2dp_decoder_stream_is_available(struct a2dp_dec_hdl *dec)
{
    int err = 0;
    u8 *packet = NULL;
    int len = 0;
    int drop = 0;

#if AUDIO_CODEC_SUPPORT_SYNC
    if (dec->sync_step) {
        return 0;
    }

    packet = (u8 *)a2dp_media_fetch_packet(&len, NULL);
    if (!packet) {
        return -EINVAL;
    }

    dec->seqn = RB16(packet + 2);
    if (dec->ts_handle) {
#if TCFG_USER_TWS_ENABLE
        if (!tws_network_audio_was_started() && !a2dp_audio_timestamp_is_available(dec->ts_handle, dec->seqn, 0, &drop)) {
            if (drop) {
                local_irq_disable();
                u8 *check_packet = (u8 *)a2dp_media_fetch_packet(&len, NULL);
                if (check_packet && RB16(check_packet + 2) == dec->seqn) {
                    a2dp_media_free_packet(packet);
                }
                local_irq_enable();
                a2dp_decoder_update_base_time(dec);
                a2dp_audio_set_base_time(dec->ts_handle, dec->base_time);
            }
            return -EINVAL;
        }
#endif
    }
    dec->sync_step = 2;
#endif

    return 0;
}

static int a2dp_dec_probe_handler(struct audio_decoder *decoder)
{
    struct a2dp_dec_hdl *dec = container_of(decoder, struct a2dp_dec_hdl, decoder);
    int err = 0;

    err = a2dp_decoder_stream_is_available(dec);
    if (err) {
        audio_decoder_suspend(decoder, 0);
        return err;
    }

    err = a2dp_decoder_audio_sync_handler(decoder);
    if (err) {
        audio_decoder_suspend(decoder, 0);
        return err;
    }

    dec->new_frame = 1;

    a2dp_dec_set_output_channel(dec);

    return err;
}

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
void a2dp_digital_vol_set(u8 vol)
{
    if (a2dp_dec) {
        audio_digital_vol_set(a2dp_dec->dvol, vol);
    }
}
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/


static int a2dp_output_after_syncts_filter(void *priv, void *data, int len)
{
    int wlen = 0;
    int drop_len = 0;
    struct a2dp_dec_hdl *dec = (struct a2dp_dec_hdl *)priv;

#if TCFG_EQ_ENABLE&&TCFG_BT_MUSIC_EQ_ENABLE
    if (dec->eq_drc && dec->eq_drc->async) {
        return eq_drc_run(dec->eq_drc, data, len);//异步eq
    }
#endif

#if AUDIO_OUT_MIXER_ENABLE
    return audio_mixer_ch_write(&dec->mix_ch, data, len);
#else
    return audio_output_data_a2dp(dec, data, len);
#endif
}

static void a2dp_output_before_syncts_handler(struct a2dp_dec_hdl *dec, void *data, int len)
{
#if AUDIO_CODEC_SUPPORT_SYNC
    if (dec->ts_start == 1) {
        u32 timestamp = dec->timestamp;
        u32 current_time = ((bt_audio_sync_lat_time() + (30000 / 625)) & 0x7ffffff) * 625 * TIME_US_FACTOR;
#define time_after_timestamp(t1, t2) \
        ((t1 > t2 && ((t1 - t2) < (10000000L * TIME_US_FACTOR))) || (t1 < t2 && ((t2 - t1) > (10000000L * TIME_US_FACTOR))))
        if (!time_after_timestamp(timestamp, current_time)) {//时间戳与当前解码时间相差太近，直接丢掉
            audio_syncts_resample_suspend(dec->syncts);
        } else {
            audio_syncts_resample_resume(dec->syncts);
            dec->mix_ch_event_params[2] = timestamp;
            dec->ts_start = 2;
        }
    }
#endif
}

static void audio_filter_resume_decoder(void *priv)
{
    struct audio_decoder *decoder = (struct audio_decoder *)priv;

    audio_decoder_resume(decoder);
}

static int a2dp_decoder_slience_plc_filter(struct a2dp_dec_hdl *dec, void *data, int len)
{
    if (len == 0) {
        a2dp_decoder_stream_free(dec, NULL);
        if (!dec->stream_error) {
            dec->stream_error = A2DP_STREAM_DECODE_ERR;
            dec->repair = 1;
        }
        return 0;
    }
    if (dec->stream_error) {
        memset(data, 0x0, len);
    }
#if TCFG_USER_TWS_ENABLE
    u8 tws_pairing = 0;
    if (dec->preempt || tws_network_audio_was_started()) {
        /*a2dp播放中副机加入，声音淡入进去*/
        tws_network_local_audio_start();
        dec->preempt = 0;
        tws_pairing = 1;
        memset(data, 0x0, len);
        dec->slience_frames = 30;
    }
#endif
#if A2DP_AUDIO_PLC_ENABLE
    if (dec->plc_ops) {
        if (dec->slience_frames) {
            dec->plc_ops->run(dec->plc_mem, data, data, len >> 1, 2);
        } else if (dec->stream_error) {
            dec->plc_ops->run(dec->plc_mem, data, data, len >> 1, dec->repair ? 1 : 2);
            dec->repair = 0;
        } else {
            dec->plc_ops->run(dec->plc_mem, data, data, len >> 1, 0);
        }
    }
#else
    if (dec->slience_frames) {
        memset(data, 0x0, len);
    }
#endif
    return len;
}

static int a2dp_dec_output_handler(struct audio_decoder *decoder, s16 *data, int len, void *priv)
{
    int wlen = 0;
    struct a2dp_dec_hdl *dec = container_of(decoder, struct a2dp_dec_hdl, decoder);
    if (!dec->remain) {
        wlen = a2dp_decoder_slience_plc_filter(dec, data, len);
        if (wlen < len) {
            return wlen;
        }
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
        audio_digital_vol_run(dec->dvol, data, len);
#endif

#if TCFG_DIG_PHASE_INVERTER_EN
        digital_phase_inverter_s16(data, len);
#endif/*TCFG_DIG_PHASE_INVERTER_EN*/

#if TCFG_EQ_ENABLE&&TCFG_BT_MUSIC_EQ_ENABLE
        if (dec->eq_drc && !dec->eq_drc->async) {
            eq_drc_run(dec->eq_drc, data, len);//同步eq
        }
#endif

        a2dp_output_before_syncts_handler(dec, data, len);
    }
#if AUDIO_CODEC_SUPPORT_SYNC
    if (dec->syncts) {
        int wlen = audio_syncts_frame_filter(dec->syncts, data, len);
        if (wlen < len) {
            audio_syncts_trigger_resume(dec->syncts, (void *)decoder, audio_filter_resume_decoder);
        }
        dec->remain = wlen < len ? 1 : 0;
        return wlen;
    }
#endif
    wlen = a2dp_output_after_syncts_filter(dec, data, len);
    dec->remain = wlen < len ? 1 : 0;
    return wlen;
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

    local_irq_disable();
    free(a2dp_dec);
    a2dp_dec = NULL;
    local_irq_enable();
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

static void a2dp_decoder_delay_time_setup(struct a2dp_dec_hdl *dec)
{
#if TCFG_USER_TWS_ENABLE
    int a2dp_low_latency = tws_api_get_low_latency_state();
#endif
    if (a2dp_low_latency) {
        a2dp_delay_time = a2dp_dec->coding_type == AUDIO_CODING_AAC ? CONFIG_A2DP_DELAY_TIME_LO : CONFIG_A2DP_SBC_DELAY_TIME_LO;
    } else {
        a2dp_delay_time = CONFIG_A2DP_DELAY_TIME;
    }
    a2dp_max_interval = 0;

    dec->delay_time = a2dp_delay_time;

    dec->detect_timer = sys_timer_add((void *)dec, a2dp_stream_stability_detect, A2DP_FLUENT_DETECT_INTERVAL);
}



static int a2dp_decoder_syncts_setup(struct a2dp_dec_hdl *dec)
{
    int err = 0;
#if AUDIO_CODEC_SUPPORT_SYNC
    struct audio_syncts_params params = {0};

    params.nch = dec->ch;
#if defined(TCFG_AUDIO_OUTPUT_IIS) && TCFG_AUDIO_OUTPUT_IIS
    params.pcm_device = PCM_OUTSIDE_DAC;
    params.rout_sample_rate = TCFG_IIS_SAMPLE_RATE;
#else
    params.pcm_device = PCM_INSIDE_DAC;
    params.rout_sample_rate = dec->sample_rate;
#endif
    params.network = AUDIO_NETWORK_BT2_1;
    params.rin_sample_rate = dec->sample_rate;
    params.priv = dec;
    params.factor = TIME_US_FACTOR;
    params.output = a2dp_output_after_syncts_filter;

    bt_audio_sync_nettime_select(0);//0 - a2dp主机，1 - tws, 2 - BLE

    a2dp_decoder_update_base_time(dec);
    dec->ts_handle = a2dp_audio_timestamp_create(dec->sample_rate, dec->base_time, TIME_US_FACTOR);

    err = audio_syncts_open(&dec->syncts, &params);
    if (!err) {
#if AUDIO_OUT_MIXER_ENABLE
        dec->mix_ch_event_params[0] = (u32)&dec->mix_ch;
        dec->mix_ch_event_params[1] = (u32)dec->syncts;
        dec->mix_ch_event_params[2] = dec->base_time * 625 * TIME_US_FACTOR;
        audio_mixer_ch_set_event_handler(&dec->mix_ch, (void *)dec->mix_ch_event_params, audio_mix_ch_event_handler);
#else
        dec->mix_ch_event_params[1] = (u32)dec->syncts;
        dec->mix_ch_event_params[2] = dec->base_time * 625 * TIME_US_FACTOR;
#endif
    }

    dec->sync_step = 0;
#endif
    return err;
}

static void a2dp_decoder_syncts_free(struct a2dp_dec_hdl *dec)
{
#if AUDIO_CODEC_SUPPORT_SYNC
    if (dec->ts_handle) {
        a2dp_audio_timestamp_close(dec->ts_handle);
        dec->ts_handle = NULL;
    }
    if (dec->syncts) {
        audio_syncts_close(dec->syncts);
        dec->syncts = NULL;
    }
#endif
}

static int a2dp_decoder_plc_setup(struct a2dp_dec_hdl *dec)
{
#if A2DP_AUDIO_PLC_ENABLE
    int plc_mem_size;
    dec->plc_ops = get_lfaudioPLC_api();
    plc_mem_size = dec->plc_ops->need_buf(dec->ch); // 3660bytes，请衡量是否使用该空间换取PLC处理
    dec->plc_mem = malloc(plc_mem_size);
    if (!dec->plc_mem) {
        dec->plc_ops = NULL;
        return -ENOMEM;
    }
    dec->plc_ops->open(dec->plc_mem, dec->ch, a2dp_low_latency ? 0 : 4);
#endif
    return 0;
}

static void a2dp_decoder_plc_free(struct a2dp_dec_hdl *dec)
{
#if A2DP_AUDIO_PLC_ENABLE
    if (dec->plc_mem) {
        free(dec->plc_mem);
        dec->plc_mem = NULL;
    }
#endif
}

void a2dp_decoder_sample_detect_setup(struct a2dp_dec_hdl *dec)
{
    dec->sample_detect = a2dp_sample_detect_open(dec->sample_rate, dec->coding_type);
}

void a2dp_decoder_sample_detect_free(struct a2dp_dec_hdl *dec)
{
    if (dec->sample_detect) {
        a2dp_sample_detect_close(dec->sample_detect);
        dec->sample_detect = NULL;
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

    a2dp_drop_frame_stop();
    puts("a2dp_dec_start: in\n");
#if TCFG_USER_TWS_ENABLE
    int a2dp_low_latency = tws_api_get_low_latency_state();
#endif

#if CONFIG_MINI_BT_AUDIO_ENABLE
    a2dp_delay_time = a2dp_low_latency ? CONFIG_A2DP_SBC_DELAY_TIME_LO : CONFIG_A2DP_DELAY_TIME;
#else
    if (a2dp_low_latency) {
        a2dp_delay_time = a2dp_dec->coding_type == AUDIO_CODING_AAC ? CONFIG_A2DP_DELAY_TIME_LO : CONFIG_A2DP_SBC_DELAY_TIME_LO;
    } else {
        a2dp_delay_time = CONFIG_A2DP_DELAY_TIME;
    }
    a2dp_max_interval = 0;

    dec->delay_time = a2dp_delay_time;
    if (a2dp_dec->coding_type == AUDIO_CODING_AAC) {
        audio_overlay_load_code(a2dp_dec->coding_type);
    }
#endif /*CONFIG_MINI_BT_AUDIO_ENABLE*/

    err = audio_decoder_open(&dec->decoder, &a2dp_input, &decode_task);
    if (err) {
        goto __err1;
    }
    dec->channel = AUDIO_CH_MAX;

    audio_decoder_set_handler(&dec->decoder, &a2dp_dec_handler);
    audio_decoder_set_event_handler(&dec->decoder, a2dp_dec_event_handler, 0);

#if !CONFIG_MINI_BT_AUDIO_ENABLE
    if (a2dp_dec->coding_type != a2dp_input.coding_type) {
        struct audio_fmt f = {0};
        f.coding_type = a2dp_dec->coding_type;
        err = audio_decoder_set_fmt(&dec->decoder, &f);
        if (err) {
            goto __err2;
        }
    }
#endif /*CONFIG_MINI_BT_AUDIO_ENABLE*/

    err = audio_decoder_get_fmt(&dec->decoder, &fmt);
    if (err) {
        goto __err2;
    }

    if (fmt->sample_rate == 0) {
        log_w("A2DP stream maybe error\n");
        goto __err2;
    }
    //dac_hdl.dec_channel_num = fmt->channel;
    dec->sample_rate = fmt->sample_rate;

    a2dp_decoder_delay_time_setup(dec);
    set_source_sample_rate(fmt->sample_rate);
    a2dp_dec_set_output_channel(dec);

    /*audio_dac_set_protect_time(&dac_hdl, 3, dec, NULL);//a2dp_stream_underrun_feedback);*/
#if AUDIO_OUT_MIXER_ENABLE
    audio_mixer_ch_open(&dec->mix_ch, &mixer);
#if defined(TCFG_AUDIO_OUTPUT_IIS) && TCFG_AUDIO_OUTPUT_IIS
    audio_mixer_ch_set_sample_rate(&dec->mix_ch, TCFG_IIS_SAMPLE_RATE);
#else
    audio_mixer_ch_set_sample_rate(&dec->mix_ch, fmt->sample_rate);
#endif
    audio_mixer_ch_set_resume_handler(&dec->mix_ch, (void *)&dec->decoder, (void (*)(void *))audio_decoder_resume);
#endif

    app_audio_state_switch(APP_AUDIO_STATE_MUSIC, get_max_sys_vol());
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    a2dp_dec->dvol = audio_digital_vol_open(app_audio_get_volume(APP_AUDIO_STATE_MUSIC), SYS_MAX_VOL, 2);
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/
    a2dp_decoder_sample_detect_setup(dec);
#if TCFG_EQ_ENABLE&&TCFG_BT_MUSIC_EQ_ENABLE
    u8 drc_en = 0;
#if TCFG_DRC_ENABLE&&TCFG_BT_MUSIC_DRC_ENABLE
    drc_en = 1;
#endif//TCFG_BT_MUSIC_DRC_ENABLE

#if defined(AUDIO_GAME_EFFECT_CONFIG) && AUDIO_GAME_EFFECT_CONFIG
    if (a2dp_low_latency) {
        drc_en |= BIT(1);
    }
#endif /*AUDIO_GAME_EFFECT_CONFIG*/

#if AUDIO_OUT_MIXER_ENABLE
    dec->eq_drc = dec_eq_drc_setup(&dec->mix_ch, (eq_output_cb)audio_mixer_ch_write, fmt->sample_rate, dec->ch, a2dp_low_latency ? 0 : 1, drc_en);
#else
    dec->eq_drc = dec_eq_drc_setup(dec, (eq_output_cb)audio_output_data_a2dp, fmt->sample_rate, dec->ch, a2dp_low_latency ? 0 : 1, drc_en);
#endif
#endif//TCFG_BT_MUSIC_EQ_ENABLE

    a2dp_decoder_syncts_setup(dec);
    a2dp_decoder_plc_setup(dec);

    dec->remain = 0;
#if 0
    if (dec->coding_type == AUDIO_CODING_SBC) {
#if (TCFG_BT_MUSIC_EQ_ENABLE && ((EQ_SECTION_MAX >= 10) && AUDIO_OUT_EFFECT_ENABLE))
        clk_set_sys_lock(a2dp_low_latency ? SYS_64M : SYS_48M, 0) ;
#elif (TCFG_BT_MUSIC_EQ_ENABLE && ((EQ_SECTION_MAX >= 10) || AUDIO_OUT_EFFECT_ENABLE))
#if A2DP_AUDIO_PLC_ENABLE
        clk_set_sys_lock(a2dp_low_latency ? SYS_64M : SYS_48M, 0) ;
#else
        clk_set_sys_lock(a2dp_low_latency ? SYS_48M : SYS_32M, 0) ;
#endif//A2DP_AUDIO_PLC_ENABLE
#else
#if (!TCFG_AUDIO_ANC_ENABLE)
        clk_set_sys_lock(a2dp_low_latency ? SYS_24M : SYS_16M, 0) ;
#if A2DP_AUDIO_PLC_ENABLE
        clk_set_sys_lock(SYS_32M, 0) ;
#endif//A2DP_AUDIO_PLC_ENABLE
#endif
#endif
#if !CONFIG_MINI_BT_AUDIO_ENABLE
    } else if (dec->coding_type == AUDIO_CODING_AAC) {
#if (TCFG_BT_MUSIC_EQ_ENABLE && ((EQ_SECTION_MAX >= 10) && AUDIO_OUT_EFFECT_ENABLE))
        clk_set_sys_lock(SYS_64M, 0) ;
#elif (TCFG_BT_MUSIC_EQ_ENABLE && ((EQ_SECTION_MAX >= 10) || AUDIO_OUT_EFFECT_ENABLE))
        clk_set_sys_lock(a2dp_low_latency ? SYS_64M : SYS_48M, 0) ;
#else
        clk_set_sys_lock(a2dp_low_latency ? SYS_48M : SYS_48M, 0) ;
#endif
#endif /*CONFIG_MINI_BT_AUDIO_ENABLE*/
    }
#else
    audio_codec_clock_set(AUDIO_A2DP_MODE, dec->coding_type, dec->wait.preemption);
#endif

    err = audio_decoder_start(&dec->decoder);
    if (err) {
        goto __err3;
    }
    if (get_sniff_out_status()) {
        clear_sniff_out_status();
        audio_dac_start_pre(&dac_hdl);
    }

    dec->start = 1;

    return 0;

__err3:
#if AUDIO_OUT_MIXER_ENABLE
    audio_mixer_ch_close(&a2dp_dec->mix_ch);
#else
    audio_output_close(NULL);
#if AUDIO_CODEC_SUPPORT_SYNC
    if (a2dp_dec->syncts) {
        u32 mix_ch_event_params[3] = {0};
        mix_ch_event_params[1] = (u32)a2dp_dec->syncts;
        audio_syncts_del(mix_ch_event_params);
    }
#endif
#endif
    a2dp_decoder_syncts_free(a2dp_dec);

__err2:
    audio_decoder_close(&dec->decoder);
__err1:
    a2dp_dec_release();

    return err;
}

static int __a2dp_audio_res_close(void)
{
    a2dp_dec->start = 0;
    if (a2dp_dec->detect_timer) {
        sys_timer_del(a2dp_dec->detect_timer);
    }
    a2dp_decoder_sample_detect_free(a2dp_dec);
    audio_decoder_close(&a2dp_dec->decoder);
#if AUDIO_OUT_MIXER_ENABLE
    audio_mixer_ch_close(&a2dp_dec->mix_ch);
#else
    audio_output_close(NULL);
#if AUDIO_CODEC_SUPPORT_SYNC
    if (a2dp_dec->syncts) {
        u32 mix_ch_event_params[3] = {0};
        mix_ch_event_params[1] = (u32)a2dp_dec->syncts;
        audio_syncts_del(mix_ch_event_params);
    }
#endif

#endif

    a2dp_decoder_syncts_free(a2dp_dec);
    a2dp_decoder_plc_free(a2dp_dec);
    a2dp_decoder_stream_free(a2dp_dec, NULL);
#if TCFG_EQ_ENABLE&&TCFG_BT_MUSIC_EQ_ENABLE
    if (a2dp_dec->eq_drc) {
        dec_eq_drc_free(a2dp_dec->eq_drc);
        a2dp_dec->eq_drc = NULL;
    }
#endif//TCFG_BT_MUSIC_EQ_ENABLE

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    audio_digital_vol_close(a2dp_dec->dvol);
    a2dp_dec->dvol = NULL;
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/
#if 0
    clk_set_sys_lock(BT_NORMAL_HZ, 0);
#else
    audio_codec_clock_del(AUDIO_A2DP_MODE);
#endif

    a2dp_dec->preempt = 1;
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
int __a2dp_dec_open(int media_type, u8 resume)
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
#if !CONFIG_MINI_BT_AUDIO_ENABLE
    case A2DP_CODEC_MPEG24:
        printf("a2dp_media_type:AAC");
        dec->coding_type = AUDIO_CODING_AAC;
        break;
#endif /*CONFIG_MINI_BT_AUDIO_ENABLE*/
    default:
        printf("a2dp_media_type unsupoport:%d", media_type);
        free(dec);
        return -EINVAL;
    }

#if TCFG_USER_TWS_ENABLE
    if (CONFIG_LOW_LATENCY_ENABLE) {
        a2dp_low_latency = tws_api_get_low_latency_state();
    }
#endif
    a2dp_dec = dec;
    dec->preempt = resume ? 1 : 0;
    dec->wait.priority = 0;
    dec->wait.preemption = 1;
    dec->wait.handler = a2dp_wait_res_handler;
    audio_decoder_task_add_wait(&decode_task, &dec->wait);

    if (a2dp_dec && (a2dp_dec->start == 0)) {
        a2dp_drop_frame_start();
    }

    return 0;
}

int a2dp_dec_open(int media_type)
{
    return __a2dp_dec_open(media_type, 0);
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
        /*a2dp_dec_open(type);*/
        __a2dp_dec_open(type, 1);
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
            drop_a2dp_timer = sys_timer_add(NULL, (void (*)(void *))a2dp_media_clear_packet_before_seqn, 100);
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
            /*a2dp_dec_open(type);*/
            __a2dp_dec_open(type, 1);
        }
    }
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

#define MSBC_FRAME_LEN  58
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

static int esco_frame_head_check(struct esco_dec_hdl *dec)
{
    if (dec->coding_type != AUDIO_CODING_MSBC || dec->data_len) {
        return 0;
    }
    /*新的一帧数据*/
    int len = dec->frame_len - dec->offset;
    u8 *data = (u8 *)dec->frame + dec->offset;
    int offset;

    for (offset = 0; offset < len; offset++) {
        if (data[offset] == 0xAD) {
            if (offset >= 2 && !headcheck(data + offset - 2)) {
                continue;
            }
            goto check_success;
        }
    }

    return -EINVAL;
check_success:
    dec->offset += offset;
    return 0;
}


static int esco_dec_get_frame(struct audio_decoder *decoder, u8 **frame)
{
    int len = 0;
    u32 hash = 0;
    struct esco_dec_hdl *dec = container_of(decoder, struct esco_dec_hdl, decoder);

__again:
    if (dec->repair && dec->coding_type == AUDIO_CODING_MSBC) {
        memcpy((u8 *)dec->data, msbc_mute_data, sizeof(msbc_mute_data));
        *frame = (u8 *)dec->data;
        return dec->esco_len;
    }

    if (dec->frame) {
        int err = esco_frame_head_check(dec);
        if (err) {
            dec->frame_err_len += dec->frame_len;
            if (dec->frame_err_len >= dec->esco_len) {
                lmp_private_free_esco_packet(dec->frame);
                dec->frame = NULL;
                dec->repair = 1;
                dec->frame_err_len = 0;
                goto __again;
            }
        } else {
            dec->frame_err_len = 0;
            int len = dec->frame_len - dec->offset;
            if (len > dec->esco_len - dec->data_len) {
                len = dec->esco_len - dec->data_len;
            }
            /*memcpy((u8 *)dec->data + dec->data_len, msbc_mute_data, sizeof(msbc_mute_data));*/
            memcpy((u8 *)dec->data + dec->data_len, dec->frame + dec->offset, len);
            dec->offset   += len;
            dec->data_len += len;
        }

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
#if AUDIO_CODEC_SUPPORT_SYNC
        u32 timestamp;
        if (dec->ts_handle) {
            timestamp = esco_audio_timestamp_update(dec->ts_handle, hash);
            if (dec->syncts && (((hash - dec->hash) & 0x7ffffff) == dec->frame_time)) {
                audio_syncts_next_pts(dec->syncts, timestamp);
            }
            dec->hash = hash;
            if (!dec->ts_start) {
                dec->ts_start = 1;
                dec->mix_ch_event_params[2] = timestamp;
            }
        }
#endif
        dec->repair = 0;
#if (defined(TCFG_PHONE_MESSAGE_ENABLE) && (TCFG_PHONE_MESSAGE_ENABLE))
        phone_message_enc_write(dec->frame + 2, len - 2);
#endif
        if (dec->data_len) {
            dec->offset = 0;
        } else {
            dec->offset = dec->coding_type == AUDIO_CODING_MSBC ? 2 : 0;
        }
        dec->frame_len = len;
        u32 *error_num = (u32 *)dec->frame;
        if (error_num[0] == 0xAAAA5555 && error_num[1] == 0xAAAAAAAA) {
            if (dec->coding_type == AUDIO_CODING_MSBC) {
                lmp_private_free_esco_packet(dec->frame);
                dec->frame = NULL;
            }
            dec->repair = 1;
        }
        goto __again;
    }
    *frame = (u8 *)dec->data;
    return dec->esco_len;
}

static void esco_dec_put_frame(struct audio_decoder *decoder, u8 *frame)
{
    struct esco_dec_hdl *dec = container_of(decoder, struct esco_dec_hdl, decoder);

    dec->data_len = 0;
    dec->repair = 0;
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

#if 0
    if (!dec->sync_step) {
        if (info->data_len <= 0) {
            return -EAGAIN;
        }
#if 0//TCFG_USER_TWS_ENABLE
        int state = tws_api_get_tws_state();
        if (state & TWS_STA_SIBLING_CONNECTED) {
            if (tws_api_get_role() == TWS_ROLE_SLAVE) {
                audio_dac_sound_reset(&dac_hdl, 150);
            }
        }
#endif
        dec->sync_step = 2;
    }

    if (info->data_len <= 120) {
        info->distance_time = -15;
    } else if (info->data_len > 240) {
        info->distance_time = 15;
    }

#endif
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

#if TCFG_USER_TWS_ENABLE
    if (tws_network_audio_was_started()) {
        /*清除从机声音加入标志*/
        dec->slience_frames = 20;
        tws_network_local_audio_start();
    }
#endif
    if (dec->preempt) {
        dec->preempt = 0;
        dec->slience_frames = 20;
    }
    return err;
}



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
#define MONO_TO_DUAL_POINTS 30


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

static int esco_output_after_syncts_filter(void *priv, s16 *data, int len)
{
    struct esco_dec_hdl *dec = (struct esco_dec_hdl *)priv;
    int wlen = 0;

#if TCFG_AUDIO_OUTPUT_IIS
#if AUDIO_OUT_MIXER_ENABLE
    wlen = audio_mixer_ch_write(&dec->mix_ch, data, len);
#else
    wlen = audio_output_data_esco(dec, data, len);
#endif
#else
    s16 two_ch_data[MONO_TO_DUAL_POINTS * 2];
    s16 point_num = 0;
    u8 mono_to_dual = 0;
    s16 *mono_data = (s16 *)data;
    u16 remain_points = (len >> 1);
    /*
     *如果dac输出是双声道，因为esco解码出来时单声道
     *所以这里要根据dac通道确定是否做单变双
     */

#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR)
    mono_to_dual = 1;
#endif
    if (mono_to_dual) {
        do {
            point_num = MONO_TO_DUAL_POINTS;
            if (point_num >= remain_points) {
                point_num = remain_points;
            }
            audio_pcm_mono_to_dual(two_ch_data, mono_data, point_num);
#if AUDIO_OUT_MIXER_ENABLE
            int tmp_len = audio_mixer_ch_write(&dec->mix_ch, two_ch_data, point_num << 2);
#else
            int tmp_len = audio_output_data_esco(dec, two_ch_data, point_num << 2);
#endif

            wlen += tmp_len;
            remain_points -= (tmp_len >> 2);
            if (tmp_len < (point_num << 2)) {
                break;
            }
            mono_data += point_num;
        } while (remain_points);
        wlen >>= 1;
    } else {
#if AUDIO_OUT_MIXER_ENABLE
        wlen = audio_mixer_ch_write(&dec->mix_ch, data, len);
#else
        wlen = audio_output_data_esco(dec, data, len);
#endif
    }
#endif

    return wlen;

}

static int esco_dec_output_handler(struct audio_decoder *decoder, s16 *buf, int size, void *priv)
{
    int wlen = 0;
    int ret_len = size;
    int len = size;
    short *data = buf;
    struct esco_dec_hdl *dec = container_of(decoder, struct esco_dec_hdl, decoder);

    /*非上次残留数据,进行后处理*/
    if (!dec->remain) {
        if (dec->slience_frames) {
            memset(data, 0x0, len);
            dec->slience_frames--;
        }
#if (defined(TCFG_PHONE_MESSAGE_ENABLE) && (TCFG_PHONE_MESSAGE_ENABLE))
        phone_message_call_api_esco_out_data(data, len);
#endif/*TCFG_PHONE_MESSAGE_ENABLE*/
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
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/

#if TCFG_DIG_PHASE_INVERTER_EN
        digital_phase_inverter_s16(data, len);
#endif/*TCFG_DIG_PHASE_INVERTER_EN*/

#if TCFG_AUDIO_NOISE_GATE
        /*来电去电铃声不做处理*/
        if (get_call_status() == BT_CALL_ACTIVE) {
            audio_noise_gate_run(data, data, len);
        }
#endif/*TCFG_AUDIO_NOISE_GATE*/

#if TCFG_EQ_ENABLE&&TCFG_PHONE_EQ_ENABLE
        eq_drc_run(dec->eq_drc, data, len);
#endif//TCFG_PHONE_EQ_ENABLE

    }

#if AUDIO_CODEC_SUPPORT_SYNC
    if (dec->syncts) {
        wlen = audio_syncts_frame_filter(dec->syncts, data, len);
        if (wlen < len) {
            audio_syncts_trigger_resume(dec->syncts, (void *)decoder, audio_filter_resume_decoder);
        }
        goto ret_handle;
    }
#endif
    wlen = esco_output_after_syncts_filter(dec, data, len);

ret_handle:

    dec->remain = wlen == len ? 0 : 1;

    return wlen;
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
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/

int esco_dec_dac_gain_set(u8 gain)
{
    app_var.aec_dac_gain = gain;
    if (esco_dec) {
        audio_dac_set_analog_vol(&dac_hdl, gain);
    }
    return 0;
}

static int esco_decoder_syncts_setup(struct esco_dec_hdl *dec)
{
    int err = 0;
#if AUDIO_CODEC_SUPPORT_SYNC
#define ESCO_DELAY_TIME     60
    struct audio_syncts_params params = {0};
    int sample_rate = dec->decoder.fmt.sample_rate;

    params.nch = dec->decoder.fmt.channel;
#if defined(TCFG_AUDIO_OUTPUT_IIS) && TCFG_AUDIO_OUTPUT_IIS
    params.pcm_device = PCM_OUTSIDE_DAC;
    params.rout_sample_rate = TCFG_IIS_SAMPLE_RATE;
#else
    params.pcm_device = PCM_INSIDE_DAC;
    params.rout_sample_rate = sample_rate;
#endif
    params.network = AUDIO_NETWORK_BT2_1;
    params.rin_sample_rate = sample_rate;
    params.priv = dec;
    params.factor = TIME_US_FACTOR;
    params.output = (int (*)(void *, void *, int))esco_output_after_syncts_filter;

    bt_audio_sync_nettime_select(0);//0 - 主机，1 - tws, 2 - BLE
    u8 frame_clkn = dec->esco_len >= MSBC_FRAME_LEN ? 12 : 6;
    dec->ts_handle = esco_audio_timestamp_create(frame_clkn, ESCO_DELAY_TIME, TIME_US_FACTOR);
    dec->frame_time = frame_clkn;
    audio_syncts_open(&dec->syncts, &params);
    if (!err) {

#if AUDIO_OUT_MIXER_ENABLE
        dec->mix_ch_event_params[0] = (u32)&dec->mix_ch;
        dec->mix_ch_event_params[1] = (u32)dec->syncts;
        audio_mixer_ch_set_event_handler(&dec->mix_ch, (void *)dec->mix_ch_event_params, audio_mix_ch_event_handler);
#else
        dec->mix_ch_event_params[1] = (u32)dec->syncts;
#endif
    }
    dec->ts_start = 0;
#endif
    return err;
}

static void esco_decoder_syncts_free(struct esco_dec_hdl *dec)
{
#if AUDIO_CODEC_SUPPORT_SYNC
    if (dec->ts_handle) {
        esco_audio_timestamp_close(dec->ts_handle);
        dec->ts_handle = NULL;
    }
    if (dec->syncts) {
        audio_syncts_close(dec->syncts);
        dec->syncts = NULL;
    }
#endif
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
#if AUDIO_OUT_MIXER_ENABLE
    /*audio_mixer_set_output_buf(&mixer, mix_buff, sizeof(mix_buff) / 8);*/
    audio_mixer_set_output_buf(&mixer, mix_buff, mix_buf_len_fix);
    audio_mixer_ch_open(&dec->mix_ch, &mixer);
#if defined(TCFG_AUDIO_OUTPUT_IIS) && TCFG_AUDIO_OUTPUT_IIS
    audio_mixer_ch_set_sample_rate(&dec->mix_ch, TCFG_IIS_SAMPLE_RATE);
#else
    audio_mixer_ch_set_sample_rate(&dec->mix_ch, f.sample_rate);
#endif
    audio_mixer_ch_set_resume_handler(&dec->mix_ch, (void *)&dec->decoder, (void (*)(void *))audio_decoder_resume);
#endif

    app_audio_state_switch(APP_AUDIO_STATE_CALL, app_var.aec_dac_gain);
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    esco_dec->dvol = audio_digital_vol_open(app_audio_get_volume(APP_AUDIO_STATE_CALL), 15, 4);
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/
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
#if TCFG_EQ_ENABLE&&TCFG_PHONE_EQ_ENABLE
    u8 drc_en = 0;
#if TCFG_DRC_ENABLE&&ESCO_DRC_EN
    drc_en = 1;
#endif//ESCO_DRC_EN

    dec->eq_drc = esco_eq_drc_setup(NULL, NULL, f.sample_rate, f.channel, 0, drc_en);
#endif//TCFG_PHONE_EQ_ENABLE



#if AUDIO_CODEC_SUPPORT_SYNC
    esco_decoder_syncts_setup(dec);
#endif/*AUDIO_CODEC_SUPPORT_SYNC*/

    audio_dac_set_delay_time(&dac_hdl, 30, 50);
    lmp_private_esco_suspend_resume(2);
    err = audio_decoder_start(&dec->decoder);
    if (err) {
        goto __err3;
    }
    if (get_sniff_out_status()) {
        clear_sniff_out_status();
        audio_dac_start_pre(&dac_hdl);
    }
    audio_out_effect_dis = 1;//通话模式关闭高低音

    dec->start = 1;
    dec->remain = 0;

    err = audio_aec_init(f.sample_rate);
    if (err) {
        printf("audio_aec_init failed:%d", err);
        //goto __err3;
    }
    err = esco_enc_open(dec->coding_type, dec->esco_len);
    if (err) {
        printf("audio_enc_open failed:%d", err);
        goto __err3;
    }

    audio_codec_clock_set(AUDIO_ESCO_MODE, dec->coding_type, dec->wait.preemption);
#if ESCO_DRC_EN
    clk_set_sys_lock(96 * (1000000L), 0);
#endif
    dec->enc_start = 1; //该函数所在任务优先级低可能未open编码就开始解码，加入enc开始的标志防止解码过快输出
    printf("esco_dec_start ok\n");

    return 0;

__err3:
#if AUDIO_OUT_MIXER_ENABLE
    audio_mixer_ch_close(&dec->mix_ch);
#else
    audio_output_close(NULL);
#if AUDIO_CODEC_SUPPORT_SYNC
    if (dec->syncts) {
        u32 mix_ch_event_params[3] = {0};
        mix_ch_event_params[1] = (u32)dec->syncts;
        audio_syncts_del(mix_ch_event_params);
    }
#endif
#endif
    esco_decoder_syncts_free(dec);

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
    esco_dec->preempt = 1;

    audio_aec_close();
    esco_enc_close();
    app_audio_state_exit(APP_AUDIO_STATE_CALL);
    audio_decoder_close(&esco_dec->decoder);
#if AUDIO_OUT_MIXER_ENABLE
    audio_mixer_ch_close(&esco_dec->mix_ch);
#else
    audio_output_close(NULL);
#if AUDIO_CODEC_SUPPORT_SYNC
    if (esco_dec->syncts) {
        u32 mix_ch_event_params[3] = {0};
        mix_ch_event_params[1] = (u32)esco_dec->syncts;
        audio_syncts_del(mix_ch_event_params);
    }
#endif
#endif

#if A2DP_RX_AND_AUDIO_DELAY
    audio_dac_set_delay_time(&dac_hdl, 20, AUDIO_DAC_DELAY_TIME);
#else
    audio_dac_set_delay_time(&dac_hdl, 30, AUDIO_DAC_DELAY_TIME);
#endif
#if AUDIO_CODEC_SUPPORT_SYNC
    esco_decoder_syncts_free(esco_dec);
    esco_dec->sync_step = 0;
    esco_dec->preempt_state = DEC_PREEMTED_BY_PRIORITY;
#endif
#if TCFG_EQ_ENABLE&&TCFG_PHONE_EQ_ENABLE
    if (esco_dec->eq_drc) {
        esco_eq_drc_free(esco_dec->eq_drc);
        esco_dec->eq_drc = NULL;
    }
#endif



    audio_out_effect_dis = 0;

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    audio_digital_vol_close(esco_dec->dvol);
    esco_dec->dvol = NULL;
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/

    audio_plc_close();
#if TCFG_AUDIO_NOISE_GATE
    audio_noise_gate_close();
#endif/*TCFG_AUDIO_NOISE_GATE*/
    audio_codec_clock_del(AUDIO_ESCO_MODE);
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

    if (codec_type == 3) {
        dec->coding_type = AUDIO_CODING_MSBC;
        dec->esco_len = MSBC_FRAME_LEN;
    } else if (codec_type == 2) {
        dec->coding_type = AUDIO_CODING_CVSD;
        dec->esco_len = esco_len;
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

#if AUDIO_OUT_MIXER_ENABLE
    audio_mixer_set_output_buf(&mixer, mix_buff, sizeof(mix_buff));
#endif
    printf("esco_dec_close succ\n");
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
#endif/*TCFG_WAV_TONE_MIX_ENABLE*/

    tone_play_init();

#if TCFG_KEY_TONE_EN
    // 按键音初始化
    audio_key_tone_init();
#endif

    /*os_sem_create(&dac_sem, 0);*/
    err = audio_decoder_task_create(&decode_task, "audio_dec");
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    audio_digital_vol_init(NULL, 0);
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/
    audio_dac_init(&dac_hdl, &dac_data);
#if TCFG_AUDIO_ANC_ENABLE
    audio_dac_anc_set(&dac_hdl, 1);
#endif/*TCFG_AUDIO_ANC_ENABLE*/

    u32 dacr32 = read_capless_DTB();
    audio_dac_set_capless_DTB(&dac_hdl, dacr32);
    audio_dac_set_buff(&dac_hdl, dac_buff, sizeof(dac_buff));
#if A2DP_RX_AND_AUDIO_DELAY
    audio_dac_set_delay_time(&dac_hdl, 10, AUDIO_DAC_DELAY_TIME);
#else
    audio_dac_set_delay_time(&dac_hdl, 10, AUDIO_DAC_DELAY_TIME);
#endif/*A2DP_RX_AND_AUDIO_DELAY*/
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
#if TCFG_MC_BIAS_AUTO_ADJUST
    mic_trim_run();
#endif/*TCFG_MC_BIAS_AUTO_ADJUST*/
    audio_dac_set_fade_handler(&dac_hdl, NULL, audio_fade_in_fade_out);
#if AUDIO_CODEC_SUPPORT_SYNC
    audio_sync_resample_set_filt(dac_sync_filt, sizeof(dac_sync_filt));
#endif

    /*硬件SRC模块滤波器buffer设置，可根据最大使用数量设置整体buffer*/
    audio_src_base_filt_init(audio_src_hw_filt, sizeof(audio_src_hw_filt));


#if AUDIO_OUT_MIXER_ENABLE
    audio_mixer_open(&mixer);
    audio_mixer_set_handler(&mixer, &mix_handler);
    audio_mixer_set_event_handler(&mixer, mixer_event_handler);
    audio_mixer_set_output_buf(&mixer, mix_buff, sizeof(mix_buff));
#else
    printf("\n\n\n no support mixer !!! \n\n\n");
#endif

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



/* ----------------------debug_function---------------------------- */
/* ----------------------debug_function---------------------------- */
#include "tone_player.h"
static const char *num0_9[] = {
    TONE_NUM_0,
    TONE_NUM_1,
    TONE_NUM_2,
    TONE_NUM_3,
    TONE_NUM_4,
    TONE_NUM_5,
    TONE_NUM_6,
    TONE_NUM_7,
    TONE_NUM_8,
    TONE_NUM_9,
};

static void a2dp_test_tone_warning(int error, int is_seqn)
{
    return ;

    const char *tone_seqn[16];
    if (is_seqn) {
        int ten_num = 0;
        int num = error;
        while (num < 10) {
            num /= 10;
            ten_num++;
        }

        int list_i = 0;
        int i;
        tone_seqn[list_i++] = TONE_RING;
        for (i = ten_num; i > 0; i--) {
            tone_seqn[list_i++] = num0_9[((error / (ten_num * 10)) % 10)];
        }
        tone_seqn[list_i] = num0_9[(error % 10)];
        tone_file_list_play(tone_seqn, 1);
    } else {
        tone_play_index(error, 1);
    }
}

static void a2dp_error_tone_warning(int error)
{
    return;

    int argv[8];
    argv[0] = (int)a2dp_test_tone_warning;
    argv[1] = 2;
    argv[2] = (int)IDEX_TONE_NUM_0 + error;
    argv[3] = (int)0;

    do {
        int err = os_taskq_post_type("app_core", Q_CALLBACK, 4, argv);
        if (err == OS_ERR_NONE) {
            break;
        }

        os_time_dly(2);
    } while (1);
}

void rx_full_tone_warning(void)
{
    /* if (rx_full == 0) { */
    /* rx_full = 1; */
    /* } */
}




/*----------------------------------------------------------------------------*/
/**@brief    获取输出默认采样率
   @param
   @return   0: 采样率可变
   @return   非0: 固定采样率
   @note
*/
/*----------------------------------------------------------------------------*/
u32 audio_output_nor_rate(void)
{
#if (TCFG_IIS_ENABLE && TCFG_AUDIO_OUTPUT_IIS)
    return TCFG_IIS_SAMPLE_RATE;
#endif
#if AUDIO_OUTPUT_INCLUDE_DAC

#if (TCFG_MIC_EFFECT_ENABLE)
    return TCFG_REVERB_SAMPLERATE_DEFUAL;
#endif
    /* return  app_audio_output_samplerate_select(input_rate, 1); */
#elif (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_BT)

#elif (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_FM)
    return 41667;
#else
    return 44100;
#endif

    /* #if TCFG_VIR_UDISK_ENABLE */
    /*     return 44100; */
    /* #endif */

    return 0;
}


/*******************************************************
* Function name	: app_audio_output_samplerate_select
* Description	: 将输入采样率与输出采样率进行匹配对比
* Parameter		:
*   @sample_rate    输入采样率
*   @high:          0 - 低一级采样率，1 - 高一级采样率
* Return        : 匹配后的采样率
********************* -HB ******************************/
int app_audio_output_samplerate_select(u32 sample_rate, u8 high)
{
    return audio_dac_sample_rate_select(&dac_hdl, sample_rate, high);
}



/*----------------------------------------------------------------------------*/
/**@brief    获取输出采样率
   @param    input_rate: 输入采样率
   @return   输出采样率
   @note
*/
/*----------------------------------------------------------------------------*/
u32 audio_output_rate(int input_rate)
{
    u32 out_rate = audio_output_nor_rate();
    if (out_rate) {
        return out_rate;
    }

#if (TCFG_REVERB_ENABLE || TCFG_MIC_EFFECT_ENABLE)
    if (input_rate > 48000) {
        return 48000;
    }
#endif
    return  app_audio_output_samplerate_select(input_rate, 1);
}


