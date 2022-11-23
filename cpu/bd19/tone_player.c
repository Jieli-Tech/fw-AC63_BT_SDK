#include "system/includes.h"
#include "media/includes.h"
#include "tone_player.h"
#include "audio_config.h"
#include "app_main.h"
#include "btstack/avctp_user.h"
#include "aec_user.h"
/* #include "audio_digital_vol.h" */
#include "audio_codec_clock.h"
#include "board_config.h"
#include "audio_digital_vol.h"

#if TCFG_APP_FM_EMITTER_EN
#include "fm_emitter/fm_emitter_manage.h"
#endif

#if TCFG_AUDIO_MUSIC_SIDETONE_ENABLE     //播歌时侧耳监听功能
#include "audio_dec_mic2pcm.h"
#endif

#ifdef CONFIG_LITE_AUDIO
#include "tone_player_api.h"
#endif /*CONFIG_LITE_AUDIO*/

#define LOG_TAG_CONST   APP_TONE
#define LOG_TAG         "[APP-TONE]"
#define LOG_ERROR_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#include "debug.h"

#define TONE_LIST_MAX_NUM 4

#if TCFG_USER_TWS_ENABLE
#include "media/bt_audio_timestamp.h"
#include "audio_syncts.h"
#include "bt_tws.h"

#define msecs_to_bt_time(m)     (((m + 1)* 1000) / 625)
#define TWS_TONE_ALIGN_TIME         0
#define TWS_TONE_ALIGN_MIX          1

#define TONE_DEC_NOT_START          0
#define TONE_DEC_WAIT_ALIGN_TIME    1
#define TONE_DEC_WAIT_MIX           2
#define TONE_DEC_CONFIRM            3

#define TWS_TONE_CONFIRM_TIME       250 /*TWS提示音音频同步确认时间(也是音频解码主从确认超时时间)*/

#endif

#if TCFG_WAV_TONE_MIX_ENABLE
#define TONE_FILE_DEC_MIX			1 // 提示音叠加播放
#else
#define TONE_FILE_DEC_MIX			0
#endif

//软件数字音量(调节解码输出数据的音量),选择此音量类型需要把audio_digital_vol.h中BG_DVOL_FADE_ENABLE	这个宏置0
#define VOL_TYPE_DIGITAL		0
#define VOL_TYPE_NULL           1  //不使用音量
#define SYS_VOL_TYPE            VOL_TYPE_NULL

static OS_MUTEX tone_mutex;
struct tone_file_handle {
    u8 start;
    u8 idx;
    u8 repeat_begin;
    u8 remain;
    u8 tws;
    u16 loop;
    u32 magic;
    void *file;
    const char **list;
    enum audio_channel channel;
    struct audio_decoder decoder;
    struct audio_mixer_ch mix_ch;
    u8 ch_num;
    u16 target_sample_rate;
#if TCFG_USER_TWS_ENABLE
    u32 wait_time;
    u8 tws_align_step;
    u8 ts_start;
    void *audio_sync;
    void *syncts;
    void *ts_handle;
    u32 time_base;
#endif
    u32 mix_ch_event_params[3];
    struct audio_src_handle *hw_src;
    u32 clk_before_dec;
    u8 dec_mix;
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    dvol_handle *dvol;
#endif
};

struct tone_dec_handle {
    u8 r_index;
    u8 w_index;
    u8 list_cnt;
    u8 preemption;
    const char **list[4];
    struct audio_res_wait wait;
    u8 dec_mix;

    const char *user_evt_owner;
    void (*user_evt_handler)(void *priv);
    void *priv;
};


extern struct audio_mixer mixer;
extern struct audio_dac_hdl dac_hdl;
extern struct audio_decoder_task decode_task;

static struct tone_file_handle *file_dec;
static char *single_file[2] = {NULL};
struct tone_dec_handle *tone_dec;

int tone_file_dec_start();
u16 get_source_sample_rate();
extern void audio_mix_ch_event_handler(void *priv, int event);
extern int bt_audio_sync_nettime_select(u8 basetime);
extern u32 bt_audio_sync_lat_time(void);
static void file_decoder_syncts_free(struct tone_file_handle *dec);

void tone_event_to_user(u8 event, const char *name);
void tone_event_clear()
{
    struct sys_event e = {0};
    e.type = SYS_DEVICE_EVENT;
    e.arg  = (void *)DEVICE_EVENT_FROM_TONE;
    sys_event_clear(&e);
}
void tone_set_user_event_handler(struct tone_dec_handle *dec, void (*user_evt_handler)(void *priv), void *priv)

{
    printf("tone_set_user_event_handler:%d\n", *(u8 *)priv);
    dec->user_evt_owner = os_current_task();
    dec->user_evt_handler = user_evt_handler;
    dec->priv = priv;
}

__attribute__((weak)) void audio_pwm_set_resume(void (*resume)(void *))
{
    return;
}


int tone_event_handler(struct tone_dec_handle *dec, u8 end_flag)
{
    int argv[4];
    if (!dec->user_evt_handler) {
        /* log_info("user_evt_handler null\n"); */
        return -1;
    }

    if (strcmp(os_current_task(), dec->user_evt_owner) == 0) {
        dec->user_evt_handler(dec->priv);
        return 0;
    }
    /* dec->user_evt_handler(dec->priv); */
    argv[0] = (int)dec->user_evt_handler;
    argv[1] = 1;
    argv[2] = (int)dec->priv;
    argv[3] = (int)end_flag;//是否是被打断 关闭，0正常关闭，1被打断关闭, 模式切换时，由file_dec->end_flag, 决定该值

    return os_taskq_post_type(dec->user_evt_owner, Q_CALLBACK, 4, argv);
    /* return 0; */
}

__attribute__((weak))
int audio_dac_stop(struct audio_dac_hdl *p)
{
    return 0;
}
__attribute__((weak))
int audio_dac_write(struct audio_dac_hdl *dac, void *data, int len)
{
    return len;
}

__attribute__((weak))
int audio_dac_get_sample_rate(struct audio_dac_hdl *p)
{
    return 16000;
}

__attribute__((weak))
void audio_dac_clear(struct audio_dac_hdl *dac)
{

}
static u8 tone_dec_idle_query()
{
    if (file_dec) {
        return 0;
    }
    return 1;
}
REGISTER_LP_TARGET(tone_dec_lp_target) = {
    .name = "tone_dec",
    .is_idle = tone_dec_idle_query,
};

#if TCFG_USER_TWS_ENABLE

#define bt_time_before(t1, t2) \
         (((t1 < t2) && ((t2 - t1) & 0x7ffffff) < 0xffff) || \
          ((t1 > t2) && ((t1 - t2) & 0x7ffffff) > 0xffff))

#define TWS_FUNC_ID_TONE_ALIGN \
	(((u8)('T' + 'O' + 'N' + 'E') << (2 * 8)) | \
	 ((u8)('P' + 'L' + 'A' + 'Y' + 'E' + 'R') << (1 * 8)) | \
	 ((u8)('S' + 'Y' + 'N' + 'C') << (0 * 8)))

struct tws_tone_align {
    u8 confirm;
    u8 type;
    union {
        int time;
        int position;
    };
};
struct tws_tone_align tws_tone = {0};
/*static u8 tws_tone_align = 0;*/
/*static int tws_tone_align_time = 0;*/
static void tws_tone_play_rx_align_data(void *data, u16 len, bool rx)
{
    local_irq_disable();
    memcpy(&tws_tone, data, sizeof(tws_tone));
    tws_tone.confirm = 1;
    local_irq_enable();
    y_printf("tone tws confirm rx : %d\n", tws_tone.time);
}

REGISTER_TWS_FUNC_STUB(tone_play_align) = {
    .func_id = TWS_FUNC_ID_TONE_ALIGN,
    .func = tws_tone_play_rx_align_data,
};

static void tone_mixer_ch_event_handler(void *priv, int event)
{
    struct tone_file_handle *dec = (struct tone_file_handle *)priv;

    switch (event) {
    case MIXER_EVENT_CH_OPEN:
        break;
    case MIXER_EVENT_CH_CLOSE:
    case MIXER_EVENT_CH_RESET:
        break;
    default:
        break;
    }
}
#endif

void tone_event_to_user(u8 event, const char *name)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = (void *)DEVICE_EVENT_FROM_TONE;
    e.u.dev.event = event;
    e.u.dev.value = (int)name;
    sys_event_notify(&e);
}

static char *get_file_ext_name(char *name)
{
    int len = strlen(name);
    char *ext = (char *)name;

    while (len--) {
        if (*ext++ == '.') {
            break;
        }
    }

    return ext;
}

struct audio_dec_input tone_input;
static void tone_file_dec_release()
{
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    if ((tone_input.coding_type == AUDIO_CODING_WAV) && (tone_dec->preemption == 0)) {
        audio_digital_vol_bg_fade(0);
    }
    audio_digital_vol_close(file_dec->dvol);
    file_dec->dvol = NULL;
#endif
    free(file_dec);
    file_dec = NULL;
}

int tone_list_play_start(const char **list, u8 preemption, u8 tws);

static int tone_file_list_clean(u8 decoding)
{
    int i = 0;

    if (!tone_dec) {
        return 0;
    }

    for (i = 0; i < TONE_LIST_MAX_NUM; i++) {
        if (tone_dec->list[i]) {
            if (decoding && i == tone_dec->r_index) {
                continue;
            }
            free(tone_dec->list[i]);
            tone_dec->list[i] = NULL;
        }
    }

    if (decoding) {
        tone_dec->w_index = tone_dec->r_index + 1;
        if (tone_dec->w_index >= TONE_LIST_MAX_NUM) {
            tone_dec->w_index = 0;
        }
        tone_dec->list_cnt = 1;
    } else {
        tone_dec->list_cnt = 0;
    }
    return 0;
}

void tone_dec_release()
{
    if (!tone_dec) {
        os_mutex_post(&tone_mutex);
        return;
    }

    tone_event_handler(tone_dec, 0);
    audio_decoder_task_del_wait(&decode_task, &tone_dec->wait);

    tone_file_list_clean(0);
    free(tone_dec);
    tone_dec = NULL;
    os_mutex_post(&tone_mutex);
}

void tone_dec_end_handler(int event, const char *name)
{
    const char **list;

    list = tone_dec->list[tone_dec->r_index];

    if (++tone_dec->r_index >= TONE_LIST_MAX_NUM) {
        tone_dec->r_index = 0;
    }
    if (--tone_dec->list_cnt > 0) {
        tone_list_play_start(tone_dec->list[tone_dec->r_index], tone_dec->preemption, 1);
    } else {
        tone_dec_release();
    }

    tone_event_to_user(event, name);
}


static int tone_file_list_repeat(struct audio_decoder *decoder)
{
    int err = 0;

    file_dec->idx++;
    if (!file_dec->list[file_dec->idx]) {
        log_info("repeat end 1:idx end");
        return 0;
    }

    if (IS_REPEAT_END(file_dec->list[file_dec->idx])) {
        //log_info("repeat_loop:%d",file_dec->loop);
        if (file_dec->loop) {
            file_dec->loop--;
            file_dec->idx = file_dec->repeat_begin;
        } else {
            file_dec->idx++;
            if (!file_dec->list[file_dec->idx]) {
                log_info("repeat end 2:idx end");
                return 0;
            }
        }
    }

    if (IS_REPEAT_BEGIN(file_dec->list[file_dec->idx])) {
        if (!file_dec->loop) {
            file_dec->loop = TONE_REPEAT_COUNT(file_dec->list[file_dec->idx]);
            log_info("repeat begin:%d", file_dec->loop);
        }
        file_dec->idx++;
        file_dec->repeat_begin = file_dec->idx;
    }

    log_info("repeat idx:%d,%s", file_dec->idx, file_dec->list[file_dec->idx]);
    file_dec->file = fopen(file_dec->list[file_dec->idx], "r");
    if (!file_dec->file) {
        log_error("repeat end:fopen repeat file faild");
        return 0;
    }

    return 1;
}

static int tone_audio_res_close(u8 rpt)
{
    if (file_dec->start) {
        audio_decoder_close(&file_dec->decoder);
    }

    if (file_dec->start) {
        file_dec->start = 0;
    }

    return 0;
}

static void tone_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    int repeat = 0;
    switch (argv[0]) {
    case AUDIO_DEC_EVENT_END:
    case AUDIO_DEC_EVENT_ERR:
        if (argv[1] != file_dec->magic) {
            log_error("file_dec magic no match:%d-%d", argv[1], file_dec->magic);
            break;
        }
        repeat = tone_file_list_repeat(decoder);
        log_info("AUDIO_DEC_EVENT_END,err=%x,repeat=%d\n", argv[0], repeat);

        if (repeat) {
            tone_audio_res_close(repeat);

            tone_file_dec_start();
        } else {
            tone_file_list_stop(0);
        }
        break;
    default:
        return;
    }
}

int tone_get_status()
{
    return tone_dec ? TONE_START : TONE_STOP;
}

int tone_get_dec_status()
{
    if (tone_dec && file_dec && (file_dec->decoder.state != DEC_STA_WAIT_STOP)) {
        return TONE_START;
    }
    return 	TONE_STOP;
}
int tone_dec_wait_stop(u32 timeout_ms)
{
    u32 to_cnt = 0;
    while (tone_get_dec_status()) {
        /* putchar('t'); */
        os_time_dly(1);
        if (timeout_ms) {
            to_cnt += 10;
            if (to_cnt >= timeout_ms) {
                break;
            }
        }
    }
    return tone_get_dec_status();
}

static int tone_fread(struct audio_decoder *decoder, void *buf, u32 len)
{
    int rlen = 0;

    if (!file_dec->file) {
        return 0;
    }

    rlen = fread(file_dec->file, buf, len);
    if (rlen < len) {
        fclose(file_dec->file);
        file_dec->file = NULL;
    }
    return rlen;
}

static int tone_fseek(struct audio_decoder *decoder, u32 offset, int seek_mode)
{
    if (!file_dec->file) {
        return 0;
    }
    return fseek(file_dec->file, offset, seek_mode);
}

static int tone_flen(struct audio_decoder *decoder)
{
    void *tone_file = NULL;
    int len = 0;

    if (file_dec->file) {
        len = flen(file_dec->file);
        return len;
    }

    tone_file = fopen(file_dec->list[file_dec->idx], "r");
    if (tone_file) {
        len = flen(tone_file);
        fclose(tone_file);
        tone_file = NULL;
    }
    return len;
}

static int tone_fclose(void *file)
{
    if (file_dec->file) {
        fclose(file_dec->file);
        file_dec->file = NULL;
    }

    file_dec->idx = 0;
    return 0;
}

struct tone_format {
    const char *fmt;
    u32 coding_type;
};

const struct tone_format tone_fmt_support_list[] = {
    {"wtg", AUDIO_CODING_G729},
    {"msbc", AUDIO_CODING_MSBC},
    {"sbc", AUDIO_CODING_SBC},
    {"mty", AUDIO_CODING_MTY},
    {"aac", AUDIO_CODING_AAC},
#if TCFG_DEC_OPUS_ENABLE
    {"opus", AUDIO_CODING_OPUS},
#endif/*TCFG_DEC_WTGV2_ENABLE*/
#if TCFG_DEC_SPEEX_ENABLE
    {"speex", AUDIO_CODING_SPEEX},
#endif/*TCFG_DEC_WTGV2_ENABLE*/
#if TCFG_DEC_WTGV2_ENABLE
    {"wts", AUDIO_CODING_WTGV2},
#endif/*TCFG_DEC_WTGV2_ENABLE*/
#if TCFG_DEC_MP3_ENABLE
    {"mp3", AUDIO_CODING_MP3},
#endif/*TCFG_DEC_MP3_ENABLE*/
#if (TCFG_WAV_TONE_MIX_ENABLE || TCFG_DEC_WAV_ENABLE)
    {"wav", AUDIO_CODING_WAV},
#endif/*TCFG_WAV_TONE_MIX_ENABLE*/
};

struct audio_dec_input tone_input = {
    .coding_type = AUDIO_CODING_G729,
    .data_type   = AUDIO_INPUT_FILE,
    .ops = {
        .file = {
            .fread = tone_fread,
            .fseek = tone_fseek,
            .flen  = tone_flen,
        }
    }
};

static u32 tone_file_format_match(char *fmt)
{
    int list_num = ARRAY_SIZE(tone_fmt_support_list);
    int i = 0;

    if (fmt == NULL) {
        return AUDIO_CODING_UNKNOW;
    }

    for (i = 0; i < list_num; i++) {
        if (ASCII_StrCmpNoCase(fmt, tone_fmt_support_list[i].fmt, 4) == 0) {
            return tone_fmt_support_list[i].coding_type;
        }
    }

    return AUDIO_CODING_UNKNOW;
}


static int tone_dec_probe_handler(struct audio_decoder *decoder)
{
    struct tone_file_handle *dec = container_of(decoder, struct tone_file_handle, decoder);
    int err = 0;

#if TCFG_USER_TWS_ENABLE
    if (dec->tws_align_step == 0 && dec->ts_handle) {
        if (!tws_file_timestamp_available(dec->ts_handle)) {
            audio_decoder_suspend(decoder, 0);
            return -EINVAL;
        }
        dec->tws_align_step = 1;
    }
#endif

    return 0;
}


static int tone_final_output_handler(struct tone_file_handle *dec, s16 *data, int len)
{
#if 1

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    if (file_dec->dvol) {
        audio_digital_vol_run(file_dec->dvol, data, len);
    }
#endif
    u32 wlen = audio_pwm_write(data, len);
    if (wlen != len) {
        /* putchar('W'); */
    }
    return wlen;
#else
    put_buf(data, len);
    return len;
#endif

}

static int tone_output_after_syncts_filter(void *priv, void *data, int len)
{
    struct tone_file_handle *dec = (struct tone_file_handle *)priv;
    int wlen = 0;

    if (dec->remain == 0) {
        //get_sine_data(&tmp_cnnt, data, len/2, 1);
    }


    wlen = tone_final_output_handler(dec, data, len);

ret:
    dec->remain = wlen < len ? 1 : 0;

    return wlen;
}

static int tone_dec_output_handler(struct audio_decoder *decoder, s16 *data, int len, void *priv)
{
    int wlen = 0;
    int remain_len = len;
    struct tone_file_handle *dec = container_of(decoder, struct tone_file_handle, decoder);

    return tone_output_after_syncts_filter(dec, data, len);
}

static int tone_dec_post_handler(struct audio_decoder *decoder)
{
    return 0;
}

const struct audio_dec_handler tone_dec_handler = {
    .dec_probe  = tone_dec_probe_handler,
    .dec_output = tone_dec_output_handler,
    .dec_post   = tone_dec_post_handler,
};


static void tone_dec_set_output_channel(struct tone_file_handle *dec)
{
    int state;
    enum audio_channel channel;
    /* dec->channel = AUDIO_CH_LR; */
    dec->channel = AUDIO_CH_L;
    audio_decoder_set_output_channel(&dec->decoder, dec->channel);
    dec->ch_num = 1;
}


static int file_decoder_syncts_setup(struct tone_file_handle *dec)
{
    int err = 0;
#if TCFG_USER_TWS_ENABLE
    if (dec->hw_src) {
        audio_hw_src_close(dec->hw_src);
        free(dec->hw_src);
        dec->hw_src = NULL;
    }
    struct audio_syncts_params params = {0};
    params.nch = dec->ch_num;
#if defined(TCFG_AUDIO_OUTPUT_IIS) && TCFG_AUDIO_OUTPUT_IIS
    params.pcm_device = PCM_OUTSIDE_DAC;
    params.rout_sample_rate = TCFG_IIS_SAMPLE_RATE;
#else
    params.pcm_device = PCM_INSIDE_DAC;
    /* params.rout_sample_rate = dec->target_sample_rate; */

    params.rout_sample_rate = 32000;
#endif
    params.network = AUDIO_NETWORK_BT2_1;
    params.rin_sample_rate = dec->decoder.fmt.sample_rate;
    params.priv = dec;
    params.factor = TIME_US_FACTOR;
    params.output = tone_output_after_syncts_filter;

    u8 base = 3;
    int state = tws_api_get_tws_state();
    if (state & TWS_STA_SIBLING_CONNECTED) {
        base = dec->dec_mix ? 3 : 1;
    }

    bt_audio_sync_nettime_select(base);//3 - 优先选择远端主机为网络时钟

    dec->ts_start = 0;
    dec->ts_handle = file_audio_timestamp_create(0,
                     dec->decoder.fmt.sample_rate,
                     bt_audio_sync_lat_time(),
                     TWS_TONE_CONFIRM_TIME,
                     TIME_US_FACTOR);
    err = audio_syncts_open(&dec->syncts, &params);
    if (!err) {
        dec->mix_ch_event_params[0] = (u32)&dec->mix_ch;
        dec->mix_ch_event_params[1] = (u32)dec->syncts;
        audio_mixer_ch_set_event_handler(&dec->mix_ch, (void *)dec->mix_ch_event_params, audio_mix_ch_event_handler);
    } else {
        log_e("tone audio syncts open err\n");
    }

#endif
    return err;
}

static void file_decoder_syncts_free(struct tone_file_handle *dec)
{
#if TCFG_USER_TWS_ENABLE
    if (dec->ts_handle) {
        file_audio_timestamp_close(dec->ts_handle);
        dec->ts_handle = NULL;
    }

    if (dec->syncts) {
        audio_syncts_close(dec->syncts);
        dec->syncts = NULL;
    }
#endif
}

void tone_play_resume_handler(void *priv)
{
    if (file_dec) {
        /* putchar('Z'); */
        audio_decoder_resume(&file_dec->decoder);
    }
}




int tone_file_dec_start()
{
    int err;
    struct audio_fmt *fmt;
    u8 file_name[16];

    if (!file_dec || !file_dec->file) {
        return -EINVAL;
    }

    if (file_dec->start) {
        return 0;
    }


    fget_name(file_dec->file, file_name, 16);
    tone_input.coding_type = tone_file_format_match(get_file_ext_name((char *)file_name));
    if (tone_input.coding_type == AUDIO_CODING_UNKNOW) {
        log_e("unknow tone file format\n");
        return -EINVAL;
    }

#if 0
    if (tone_input.coding_type == AUDIO_CODING_AAC) {
        file_dec->clk_before_dec = clk_get("sys");
        if (get_call_status() == BT_CALL_HANGUP) {
            puts("aac tone play:48M\n");
            clk_set_sys_lock(48 * 1000000L, 1);
        } else {
            puts("aac tone play:64M\n");
            clk_set_sys_lock(64 * 1000000L, 1);
        }
    } else if (1) {//(tone_input.coding_type == AUDIO_CODING_WAV) || (tone_input.coding_type == AUDIO_CODING_MP3)) {
        /*当前时钟小于wav/mp3提示音播放需要得时钟，则自动提高主频*/
        file_dec->clk_before_dec = clk_get("sys");
        u32 wav_tone_play_clk = 96 * 1000000L;
        if (wav_tone_play_clk < file_dec->clk_before_dec) {
            wav_tone_play_clk = file_dec->clk_before_dec;
        }
        printf("wav/mp3 tone play clk:%d->%d\n", file_dec->clk_before_dec, wav_tone_play_clk);
        clk_set_sys_lock(wav_tone_play_clk, 1);
    }
#else
    audio_codec_clock_set(AUDIO_TONE_MODE, tone_input.coding_type, tone_dec->wait.preemption);
#endif

    err = audio_decoder_open(&file_dec->decoder, &tone_input, &decode_task);
    if (err) {
        return err;
    }

    audio_decoder_set_handler(&file_dec->decoder, &tone_dec_handler);
    /*用于处理DEC_EVENT与当前解码的匹配*/
    file_dec->magic = rand32();
    audio_decoder_set_event_handler(&file_dec->decoder, tone_dec_event_handler, file_dec->magic);

    err = audio_decoder_get_fmt(&file_dec->decoder, &fmt);
    if (err) {
        printf("tone file get fmt err \n");
        goto __err1;
    }

    tone_dec_set_output_channel(file_dec);


    audio_pwm_set_resume((void (*)(void *))tone_play_resume_handler);
    /* audio_pwm_start(); */
    set_state(2);

    file_dec->target_sample_rate = fmt->sample_rate;
__dec_start:
#if TCFG_USER_TWS_ENABLE
    /* if (file_dec->tws) { */
    /* file_decoder_syncts_setup(file_dec); */
    /* } */
    /* file_dec->tws_align_step = 0; */
#endif
    /* sys_hi_timer_add(NULL,tone_play_resume_handler,3); */

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    if ((tone_input.coding_type == AUDIO_CODING_WAV) && (tone_dec->preemption == 0)) {
        /* audio_digital_vol_bg_fade(1); */
    }
    file_dec->dvol = audio_digital_vol_open(SYS_DEFAULT_TONE_VOL, SYS_MAX_VOL, 20);
#endif/*VOL_TYPE_DIGITAL*/
    err = audio_decoder_start(&file_dec->decoder);
    if (err) {
        goto __err2;
    }
    file_dec->start = 1;

    return 0;

__err2:
#if TCFG_APP_FM_EMITTER_EN

#else
#endif
__err1:
    audio_decoder_close(&file_dec->decoder);
    tone_file_dec_release();
    return err;
}







static int tone_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;

    if (event == AUDIO_RES_GET) {
        printf(">>> %s %d\n", __func__, __LINE__);
        err = tone_file_dec_start();
        printf(">>> %s %d\n", __func__, __LINE__);
    } else if (event == AUDIO_RES_PUT) {
        printf(">>> %s %d\n", __func__, __LINE__);
        tone_file_list_stop(0);
        printf(">>> %s %d\n", __func__, __LINE__);
    }
    return err;
}


int tone_file_list_stop(u8 no_end)
{
    const char *name = NULL;
    log_info("tone_file_list_stop\n");

    os_mutex_pend(&tone_mutex, 0);

    if (!file_dec) {
        log_info("tone_file_list_stop out 0\n");
        os_mutex_post(&tone_mutex);
        return 0;
    }

    tone_audio_res_close(0);


    /* extern void audio_pwm_close(void); */

    /* audio_pwm_close(); */

#if 0
    if ((tone_input.coding_type == AUDIO_CODING_AAC) || (tone_input.coding_type == AUDIO_CODING_WAV)) {
        printf("tone_play end,clk restore:%d", file_dec->clk_before_dec);
        clk_set_sys_lock(file_dec->clk_before_dec, 2);
    }
#else
    audio_codec_clock_del(AUDIO_TONE_MODE);
#endif

    if (file_dec->list[file_dec->idx]) {
        name = (const char *)file_dec->list[file_dec->idx];
    } else if (file_dec->idx) {
        name = (const char *)file_dec->list[file_dec->idx - 1];
    }
    if (file_dec->file) {
        fclose(file_dec->file);
    }

    tone_file_dec_release();

    tone_dec_end_handler(AUDIO_DEC_EVENT_END, name);

    log_info("tone_file_list_stop out 1\n");
    /* audio_pwm_stop(); */
    set_state(0);
    return 0;
}





/*static const u8 pcm_wav_header[] = {
    'R', 'I', 'F', 'F',         //rid
    0xff, 0xff, 0xff, 0xff,     //file length
    'W', 'A', 'V', 'E',         //wid
    'f', 'm', 't', ' ',         //fid
    0x14, 0x00, 0x00, 0x00,     //format size
    0x01, 0x00,                 //format tag
    0x01, 0x00,                 //channel num
    0x80, 0x3e, 0x00, 0x00,     //sr 16K
    0x00, 0x7d, 0x00, 0x00,     //avgbyte
    0x02, 0x00,                 //blockalign
    0x10, 0x00,                 //persample
    0x02, 0x00,
    0x00, 0x00,
    'f', 'a', 'c', 't',         //f2id
    0x40, 0x00, 0x00, 0x00,     //flen
    0xff, 0xff, 0xff, 0xff,     //datalen
    'd', 'a', 't', 'a',         //"data"
    0xff, 0xff, 0xff, 0xff,     //sameple  size
};*/




int audio_decoder_find_coding_type(struct audio_decoder_task *task, u32 coding_type);
int tone_list_play_start(const char **list, u8 preemption, u8 tws)
{
    int err;
    u8 file_name[16];
    char *format = NULL;
    FILE *file = NULL;
    int index = 0;

    if (IS_REPEAT_BEGIN(list[0])) {
        index = 1;
    }

    printf(">>> %s %d\n", __func__, __LINE__);
    file = fopen(list[index], "r");
    if (!file) {
        return -EINVAL;
    }
    printf(">>> %s %d\n", __func__, __LINE__);
    fget_name(file, file_name, 16);
    format = get_file_ext_name((char *)file_name);

    file_dec = zalloc(sizeof(*file_dec));

    file_dec->list = list;
    file_dec->idx  = index;
    file_dec->file = file;
    file_dec->tws = tws;
    if (index == 1) {
        file_dec->loop = TONE_REPEAT_COUNT(list[0]);
    }

#if 0
    if (!preemption) {
        file_dec->dec_mix = 1;
        tone_dec->wait.protect = 1;
    }
#endif
    tone_dec->wait.priority = 3;
    tone_dec->wait.preemption = preemption;
    printf(">>> %s %d\n", __func__, __LINE__);
    /*AAC提示音默认打断播放*/
    if (ASCII_StrCmpNoCase(format, "aac", 3) == 0) {
        printf("aac tone,preemption = 1\n");
        tone_dec->wait.preemption = 1;
        tone_dec->wait.format = AUDIO_CODING_AAC;
    } else {
#if TONE_FILE_DEC_MIX
        fadsf
        if (tone_dec->wait.preemption == 0) {
            /*支持叠加的提示音解码文件格式*/
            if ((ASCII_StrCmpNoCase(format, "wav", 3) == 0)	|| \
                (ASCII_StrCmpNoCase(format, "mp3", 3) == 0)	|| \
                (ASCII_StrCmpNoCase(format, "wtg", 3) == 0)) {
                // 叠加播放
                file_dec->dec_mix = 1;
                /*tone_dec->wait.protect = 1;*/ //提示音播放不用来做低优先级的背景音
                tone_dec->wait.preemption = 0;
            }
        } else {
            file_dec->dec_mix = 0;
        }
#endif
    }
    tone_dec->wait.handler = tone_wait_res_handler;
    printf(">>> %s %d\n", __func__, __LINE__);
    err = audio_decoder_task_add_wait(&decode_task, &tone_dec->wait);
    printf(">>> %s %d\n", __func__, __LINE__);
#if 0
    if (!err && file_dec->start == 0) {
        /*decoder中有该解码器，则强制使用打断方式，防止overlay冲突*/
        if (audio_decoder_find_coding_type(&decode_task, tone_file_format_match(format))) {
            tone_dec->wait.preemption = 1;
            err = audio_decoder_task_add_wait(&decode_task, &tone_dec->wait);
        } else {
            err = tone_file_dec_start();
        }
    }
#endif
    printf(">>> %s %d\n", __func__, __LINE__);
    if (err) {
        if (tone_dec) {
            audio_decoder_task_del_wait(&decode_task, &tone_dec->wait);
        }
    } else {
        if (file_dec->dec_mix && !file_dec->start) {
            err = tone_file_dec_start();
        }
    }
    printf(">>> %s %d\n", __func__, __LINE__);
    return err;
}

static int __tone_file_list_play(const char **list, u8 preemption, u8 tws)
{
    int i = 0;
    int err = 0;

    if (!list) {
        return -EINVAL;
    }


    printf(">>> %s %d\n", __func__, __LINE__);
    if (tone_dec == NULL) {
        tone_dec = zalloc(sizeof(*tone_dec));
        if (tone_dec == NULL) {
            log_error("tone dec zalloc failed");
            return -ENOMEM;
        }
    }
    printf(">>> %s %d\n", __func__, __LINE__);

    while (list[i] != NULL) {
        i++;
    }
    printf(">>> %s %d\n", __func__, __LINE__);
    char **p = malloc(4 * (i + 1));
    memcpy(p, list, 4 * (i + 1));

    tone_dec->list[tone_dec->w_index++] = (const char **)p;
    if (tone_dec->w_index >= TONE_LIST_MAX_NUM) {
        tone_dec->w_index = 0;
    }


    printf(">>> %s %d\n", __func__, __LINE__);
    tone_dec->list_cnt++;
    tone_dec->preemption = preemption;
    if (tone_dec->list_cnt == 1) {
        err = tone_list_play_start(tone_dec->list[tone_dec->r_index], tone_dec->preemption, tws);
        if (err == -EINVAL) {
            free(p);
            free(tone_dec);
            tone_dec = NULL;
        }
        return err;
    } else {
        puts("tone_file_add_tail\n");
    }
    printf(">>> %s %d\n", __func__, __LINE__);

    return 0;
}

int tone_file_list_play(const char **list, u8 preemption)
{
    return __tone_file_list_play(list, preemption, 1);
}

int tone_file_list_play_with_callback(const char **list, u8 preemption, void (*user_evt_handler)(void *priv), void *priv)
{
    int i = 0;
    int err = 0;

    putchar('A');
    if (!list) {
        return -EINVAL;
    }

    putchar('B');

    if (tone_dec == NULL) {
        tone_dec = zalloc(sizeof(*tone_dec));
        if (tone_dec == NULL) {
            log_error("tone dec zalloc failed");
            return -ENOMEM;
        }
    }
    putchar('C');

    while (list[i] != NULL) {
        i++;
    }
    char **p = malloc(4 * (i + 1));
    memcpy(p, list, 4 * (i + 1));

    tone_dec->list[tone_dec->w_index++] = (const char **)p;
    if (tone_dec->w_index >= TONE_LIST_MAX_NUM) {
        tone_dec->w_index = 0;
    }

    putchar('D');
    if (user_evt_handler) {
        tone_set_user_event_handler(tone_dec, user_evt_handler, priv);
    }

    tone_dec->list_cnt++;
    tone_dec->preemption = preemption;
    if (tone_dec->list_cnt == 1) {
        err = tone_list_play_start(tone_dec->list[tone_dec->r_index], tone_dec->preemption, 1);
        if (err == -EINVAL) {
            free(p);
            free(tone_dec);
            tone_dec = NULL;
        }
        return err;
    } else {
        puts("tone_file_add_tail\n");
    }

    return 0;
}


static void tone_stop(u8 force);
int tone_play(const char *name, u8 preemption)
{

    if (tone_dec) {
        log_info("tone dec busy now,tone stop first");
        tone_stop(0);
    }
    single_file[0] = (char *)name;
    single_file[1] = NULL;
    return tone_file_list_play((const char **)single_file, preemption);
}

int tone_play_no_tws(const char *name, u8 preemption)
{

    if (tone_dec) {
        log_info("tone dec busy now,tone stop first");
        tone_stop(0);
    }
    single_file[0] = (char *)name;
    single_file[1] = NULL;
    return __tone_file_list_play((const char **)single_file, preemption, 0);
}


int tone_play_with_callback(const char *name, u8 preemption, void (*user_evt_handler)(void *priv), void *priv)
{

    if (tone_dec) {
        tone_event_clear();
        log_info("tone dec busy now,tone stop first");
        tone_stop(0);
    }

    single_file[0] = (char *)name;
    single_file[1] = NULL;
    return tone_file_list_play_with_callback((const char **)single_file, preemption, user_evt_handler, priv);
}

int tone_play_add(const char *name, u8 preemption)
{

    if (tone_dec) {
        log_info("tone dec busy now,tone file add next");
        //tone_stop(0);
    }
    single_file[0] = (char *)name;
    single_file[1] = NULL;
    return tone_file_list_play((const char **)single_file, preemption);
}

const char *get_playing_tone_name(u8 index)
{
    if (tone_dec) {
        const char **list = tone_dec->list[tone_dec->r_index + index];
        if (list) {
            return list[0];
        }
    }
    return 0;
}


static void tone_stop(u8 force)
{
    if (tone_dec == NULL) {
        return;
    }

    if (force) {
        tone_file_list_clean(1);
    }
    tone_file_list_stop(0);
    tone_dec_release();

}

static u8 audio_tone_idle_query()
{
    if (tone_dec) {
        return 0;
    }
    return 1;
}
REGISTER_LP_TARGET(audio_tone_lp_target) = {
    .name = "audio_tone",
    .is_idle = audio_tone_idle_query,
};



#if 0 //(USE_DMA_TONE)
static volatile u8 tone_play_falg = 0; //用于播放提示音后，作相应的动作
void set_tone_play_falg(u8 flag)
{
    tone_play_falg = flag;
}

u8 get_tone_play_flag(void)
{
    return tone_play_falg;
}

int tone_play_index_for_dma(u8 index, u8 flag)
{
    set_tone_play_falg(flag);
    return tone_play_index(index, 1);

}

static u32 dma_tone_arg_before = 0; //记录下按键的arg值
void set_dma_tone_arg_before(u32 arg)
{
    dma_tone_arg_before = arg;
}

u32 get_dma_tone_arg_before(void)
{
    return dma_tone_arg_before;
}

void clear_dma_tone_arg_before(void)
{
    dma_tone_arg_before = 0;
}
#endif



static volatile s32 tone_play_end_cmd = TONE_PLAY_END_CB_CMD_NONE; //用于播放提示音后，作相应的动作
void tone_play_end_cb_cmd_set(int cmd)
{
    tone_play_end_cmd = cmd;
}

s32 tone_play_end_cb_cmd_get(void)
{
    return tone_play_end_cmd;
}

int tone_play_index_with_cb_cmd(u8 index, int flag)
{
    tone_play_end_cb_cmd_set(flag);
    return tone_play_index(index, 1);

}

static u32 tone_arg_storage = 0; //记录下按键的arg值
void tone_arg_store(u32 arg)
{
    tone_arg_storage = arg;
}

u32 tone_arg_restore(void)
{
    return tone_arg_storage;
}

void tone_arg_storage_clean(void)
{
    tone_arg_storage = 0;
}


int tone_play_init(void)
{
    os_mutex_create(&tone_mutex);
    return 0;
}


int tone_play_stop(void)
{
    log_info("tone_play_stop");
    tone_stop(1);
    return 0;
}

/*
 *@brief:提示音比较，确认目标提示音和正在播放的提示音是否一致
 *@return: 0 	匹配
 *		   非0	不匹配或者当前没有提示音播放
 *@note:通过提示音名字比较
 */
int tone_name_compare(const char *name)
{
    if (tone_dec) {
        if (file_dec) {
            printf("file_name:%s,cmp_name:%s\n", file_dec->list[file_dec->idx], name);
            return strcmp(name, file_dec->list[file_dec->idx]);
        }
    }
    printf("tone_dec idle now\n");
    return -1;
}
