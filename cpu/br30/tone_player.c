#include "system/includes.h"
#include "media/includes.h"
#include "tone_player.h"
#include "audio_config.h"
#include "app_main.h"
#include "btstack/avctp_user.h"
#include "aec_user.h"
#include "audio_digital_vol.h"

#if TCFG_APP_FM_EMITTER_EN
#include "fm_emitter/fm_emitter_manage.h"
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
#include "bt_tws.h"
#include "effectrs_sync.h"
#define TWS_TONE_ALIGN_TIME         0
#define TWS_TONE_ALIGN_MIX          1

#define TONE_DEC_NOT_START          0
#define TONE_DEC_WAIT_ALIGN_TIME    1
#define TONE_DEC_WAIT_MIX           2
#define TONE_DEC_CONFIRM            3

#define TWS_TONE_CONFIRM_TIME       250 /*TWS提示音音频同步确认时间(也是音频解码主从确认超时时间)*/

void *local_file_dec_sync_open(u8 channel, u16 sample_rate, u16 output_rate);
#endif

#if TCFG_WAV_TONE_MIX_ENABLE
#define TONE_FILE_DEC_MIX			1 // 提示音叠加播放
#else
#define TONE_FILE_DEC_MIX			0
#endif

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
    u16 src_out_sr;
#if TCFG_USER_TWS_ENABLE
    u32 wait_time;
    u8 tws_align_step;
    void *audio_sync;
#endif
    struct audio_src_handle *hw_src;
    u32 clk_before_dec;
    u8 dec_mix;
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    dvol_handle *dvol;
#endif
};

struct tone_sine_handle {
    u8 start;
    u8 repeat;
    u32 sine_id;
    u32 sine_offset;
    void *sin_maker;
    struct audio_decoder decoder;
    struct audio_mixer_ch mix_ch;
    struct sin_param sin_dynamic_params[8];
    u32 clk_before;
    u8 dec_mix;
    u8 remain;
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
static struct tone_sine_handle *sine_dec;
static char *single_file[2] = {NULL};
struct tone_dec_handle *tone_dec;

int sine_dec_close(void);
int tone_file_dec_start();
u16 get_source_sample_rate();

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


int tone_event_handler(struct tone_dec_handle *dec, u8 end_flag)
{
    int argv[4];
    if (!dec->user_evt_handler) {
        /* log_info("user_evt_handler null\n"); */
        return -1;
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
    if (file_dec || sine_dec) {
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
        if (dec->audio_sync) {
            audio_dac_sync_start(&dac_hdl);
        }
        break;
    case MIXER_EVENT_CH_CLOSE:
    case MIXER_EVENT_CH_RESET:
        if (dec->audio_sync) {
            audio_dac_sync_stop(&dac_hdl);
        }
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

    if (file_dec->hw_src) {
        audio_hw_src_stop(file_dec->hw_src);
        audio_hw_src_close(file_dec->hw_src);
        free(file_dec->hw_src);
        file_dec->hw_src = NULL;
    }

    if (file_dec->start) {
        audio_mixer_ch_close(&file_dec->mix_ch);
        file_dec->start = 0;
    }

#if TCFG_USER_TWS_ENABLE
    if (file_dec->audio_sync) {
        audio_wireless_sync_close(file_dec->audio_sync);
        file_dec->audio_sync = NULL;
    }
#endif

    if (!rpt) {
        if (app_audio_get_state() == APP_AUDIO_STATE_WTONE) {
            app_audio_state_exit(APP_AUDIO_STATE_WTONE);
        }
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
    if (tone_dec && sine_dec && (sine_dec->decoder.state != DEC_STA_WAIT_STOP)) {
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
#if TCFG_WAV_TONE_MIX_ENABLE
    {"wav", AUDIO_CODING_WAV},
#endif
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

#if TCFG_USER_TWS_ENABLE
static u8 tws_tone_dec_confirm_timeout(struct audio_decoder *decoder)
{
    struct tone_file_handle *dec = container_of(decoder, struct tone_file_handle, decoder);

    if (time_after(jiffies, dec->wait_time)) {
        return 1;
    }
    return 0;
}


u8 tws_tone_together_without_bt(void);
void tws_tone_together_clean(void);
u32 tws_tone_local_together_time(void);
extern u8 bt_audio_is_running(void);
#endif

static int tone_dec_probe_handler(struct audio_decoder *decoder)
{
    struct tone_file_handle *dec = container_of(decoder, struct tone_file_handle, decoder);

#if TCFG_USER_TWS_ENABLE
    if (dec->tws_align_step == TONE_DEC_CONFIRM) {
        return 0;
    }

    if (tws_tone_together_without_bt()) {
        if (time_before(jiffies_msec(), tws_tone_local_together_time())) {
            audio_decoder_suspend(decoder, 0);
            return -EAGAIN;
        }
        tws_tone_together_clean();
        dec->tws_align_step = TONE_DEC_CONFIRM;
        return 0;
    }

    local_irq_disable();
    if (tws_tone.confirm &&
        tws_tone.type == TWS_TONE_ALIGN_TIME &&
        bt_time_before(tws_tone.time, bt_tws_future_slot_time(0))) {
        y_printf("tone tws confirm error : %d, %d\n", tws_tone.time, bt_tws_future_slot_time(0));
        tws_tone.confirm = 0;
    }
    local_irq_enable();

    if (dec->tws_align_step == TONE_DEC_WAIT_MIX) {
        if (tws_tone.confirm && tws_tone.type == TWS_TONE_ALIGN_MIX) {
            audio_mixer_ch_set_starting_position(&dec->mix_ch, tws_tone.position, 1000);
            tws_tone.confirm = 0;
            dec->tws_align_step = TONE_DEC_CONFIRM;
            return 0;
        }

        if (tws_tone_dec_confirm_timeout(decoder)) {
            tws_tone.confirm = 0;
            dec->tws_align_step = TONE_DEC_CONFIRM;
            return 0;
        }
        audio_decoder_suspend(decoder, 0);
        return -EAGAIN;
    }

    if (dec->tws_align_step == TONE_DEC_WAIT_ALIGN_TIME) {
        if ((tws_tone.confirm && tws_tone.type == TWS_TONE_ALIGN_TIME) ||
            tws_tone_dec_confirm_timeout(decoder)) {
            struct rt_stream_info info = {0};
            audio_sync_set_tws_together(dec->audio_sync, 1, tws_tone.confirm ? tws_tone.time : 0);
            tws_tone.confirm = 0;
            dec->tws_align_step = TONE_DEC_CONFIRM;
            /*y_printf("tws tone dec confirm success, %d, %d\n", tone_dec->tws_together_time, bt_tws_future_slot_time(0));*/
            audio_wireless_sync_probe(dec->audio_sync, &info);
            return 0;
        }

        audio_decoder_suspend(decoder, 0);
        return -EAGAIN;
    }

    int state = tws_api_get_tws_state();
    if (state & TWS_STA_SIBLING_CONNECTED) {
        dec->tws_align_step = dec->audio_sync ? TONE_DEC_WAIT_ALIGN_TIME : TONE_DEC_WAIT_MIX;
        if (tws_api_get_role() == TWS_ROLE_SLAVE) {
            tws_tone.confirm = 0;
            if (dec->audio_sync) {
                tws_tone.time = bt_tws_future_slot_time(TWS_TONE_CONFIRM_TIME + 50);
                tws_tone.type = TWS_TONE_ALIGN_TIME;
            } else {
                tws_tone.type = TWS_TONE_ALIGN_MIX;
                int sample_rate = audio_mixer_get_sample_rate(&mixer);
                int mix_buff_len = audio_mixer_get_output_buf_len(&mixer) >> 1;
                int position = ((sample_rate * dec->ch_num * 300 / 1000) / mix_buff_len) * mix_buff_len;
                tws_tone.position = audio_mixer_get_input_position(&mixer) + position;
            }
            tws_api_send_data_to_sibling((void *)&tws_tone, sizeof(tws_tone), TWS_FUNC_ID_TONE_ALIGN);
        }
        dec->wait_time = jiffies + msecs_to_jiffies(TWS_TONE_CONFIRM_TIME + 100);
        audio_decoder_suspend(decoder, 0);
        return -EAGAIN;
    }

    dec->tws_align_step = TONE_DEC_CONFIRM;
#endif

    return 0;
}

static int tone_dec_output_handler(struct audio_decoder *decoder, s16 *data, int len, void *priv)
{
    int wlen = 0;
    int remain_len = len;
    struct tone_file_handle *dec = container_of(decoder, struct tone_file_handle, decoder);

#if TCFG_USER_TWS_ENABLE
    if (!dec->remain && dec->audio_sync) {
        audio_wireless_sync_after_dec(dec->audio_sync, data, len);
    }
#endif

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    if (dec->remain == 0) {
        audio_digital_vol_run(file_dec->dvol, data, len);
    }
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/

    do {
        if (dec->hw_src) {
            wlen = audio_src_resample_write(dec->hw_src, data, remain_len);
        } else {
            wlen = audio_mixer_ch_write(&dec->mix_ch, data, remain_len);
        }

        if (wlen == 0) {
            break;
        }
        remain_len -= wlen;
        data += (wlen >> 1);
    } while (remain_len);

    if (remain_len == 0) {
        dec->remain = 0;
    } else {
        dec->remain = 1;
    }

    return len - remain_len;
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
#if TCFG_AUDIO_OUTPUT_IIS
    dec->channel = AUDIO_CH_LR;
    audio_decoder_set_output_channel(&dec->decoder, dec->channel);
    dec->ch_num = 2;
#elif TCFG_APP_FM_EMITTER_EN
    dec->channel = AUDIO_CH_LR;
    audio_decoder_set_output_channel(&dec->decoder, dec->channel);
    dec->ch_num = 2;
#else

    int dac_output = audio_dac_get_channel(&dac_hdl);
    dec->ch_num = 1;
    if (dac_output == DAC_OUTPUT_LR) {
        channel = AUDIO_CH_LR;
        dec->ch_num = 2;
    } else if (dac_output == DAC_OUTPUT_MONO_L) {
        channel = AUDIO_CH_L;
    } else if (dac_output == DAC_OUTPUT_MONO_R) {
        channel = AUDIO_CH_R;
    } else {
        channel = AUDIO_CH_DIFF;
    }

    /* if (channel != dec->channel) { */
    printf("set_channel: %d, dec_channel_num: %d\n", channel, dec->ch_num);
    audio_decoder_set_output_channel(&dec->decoder, channel);
    dec->channel = channel;
    /* } */
#endif
}

static int tone_dec_src_output_handler(struct audio_decoder *decoder, s16 *data, int len)
{
    int wlen = 0;
    int rlen = len;
#if TCFG_APP_FM_EMITTER_EN
    fm_emitter_cbuf_write((u8 *)data, len);
#endif

    return rlen;
}

static int tone_file_dec_src_output_handler(struct audio_decoder *decoder, s16 *data, int len)
{
    int wlen = 0;
    int rlen = len;

    if (!tone_dec || !file_dec || !file_dec->start) {
        /* putchar('O'); */
        return 0;
    }


    do {
        wlen = audio_mixer_ch_write(&file_dec->mix_ch, data, rlen);
        if (!wlen) {
            break;
        }
        data += wlen / 2;
        rlen -= wlen;
    } while (rlen);

    return len - rlen;
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

    if (tone_input.coding_type == AUDIO_CODING_AAC) {
        file_dec->clk_before_dec = clk_get("sys");
        if (get_call_status() == BT_CALL_HANGUP) {
            puts("aac tone play:48M\n");
            clk_set_sys_lock(48 * 1000000L, 1);
        } else {
            puts("aac tone play:64M\n");
            clk_set_sys_lock(64 * 1000000L, 1);
        }
    } else if (tone_input.coding_type == AUDIO_CODING_WAV) {
        /*当前时钟小于wav提示音播放需要得时钟，则自动提高主频*/
        file_dec->clk_before_dec = clk_get("sys");
        u32 wav_tone_play_clk = 96 * 1000000L;
        if (wav_tone_play_clk < file_dec->clk_before_dec) {
            wav_tone_play_clk = file_dec->clk_before_dec;
        }
        printf("wav tone play clk:%d->%d\n", file_dec->clk_before_dec, wav_tone_play_clk);
        clk_set_sys_lock(wav_tone_play_clk, 1);
    }

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
        goto __err1;
    }

    tone_dec_set_output_channel(file_dec);

#if TCFG_APP_FM_EMITTER_EN
    file_dec->hw_src = zalloc(sizeof(struct audio_src_handle));
    if (file_dec->hw_src) {
        audio_hw_src_open(file_dec->hw_src, file_dec->ch_num, SRC_TYPE_RESAMPLE);
        audio_hw_src_set_rate(file_dec->hw_src, fmt->sample_rate, 41667/*fm_emitter_get_sample_rate()*/);
        audio_src_set_output_handler(file_dec->hw_src, &file_dec->decoder, tone_dec_src_output_handler);
    } else {
        printf("hw_src malloc err ==============\n");
    }
#else
    audio_mixer_ch_open(&file_dec->mix_ch, &mixer);

#if TONE_FILE_DEC_MIX
    file_dec->src_out_sr = 0;
    if (file_dec->dec_mix) {
        file_dec->src_out_sr = audio_mixer_get_sample_rate(&mixer);
    }
    if (!file_dec->src_out_sr) {
        file_dec->src_out_sr = fmt->sample_rate;
    }
    audio_mixer_ch_set_sample_rate(&file_dec->mix_ch, file_dec->src_out_sr);
    printf("fmt->sample_rate %d\n", fmt->sample_rate);
    /* printf("mixer sr:[%d]\n\n",audio_mixer_get_sample_rate(&mixer)); */
    /* printf("\n sr:[%d];src sr:[%d] \n\n",fmt->sample_rate,file_dec->src_out_sr); */
    if (fmt->sample_rate != file_dec->src_out_sr) {
        printf("src->sr:%d, or:%d ", fmt->sample_rate, file_dec->src_out_sr);
        file_dec->hw_src = zalloc(sizeof(struct audio_src_handle));
        if (file_dec->hw_src) {
            audio_hw_src_open(file_dec->hw_src, file_dec->ch_num, SRC_TYPE_RESAMPLE);
            audio_hw_src_set_rate(file_dec->hw_src, fmt->sample_rate, file_dec->src_out_sr);
            audio_src_set_output_handler(file_dec->hw_src, &file_dec->decoder, tone_file_dec_src_output_handler);
        }
    }

    if (file_dec->dec_mix && (audio_mixer_get_ch_num(&mixer) > 1)) {
        goto __dec_start;
    }
#else
    audio_mixer_ch_set_sample_rate(&file_dec->mix_ch, fmt->sample_rate);
#endif

#ifdef TCFG_WTONT_ONCE_VOL
    extern u8 get_tone_once_vol(void);
    app_audio_state_switch(APP_AUDIO_STATE_WTONE, get_tone_once_vol());
#else
    app_audio_state_switch(APP_AUDIO_STATE_WTONE, get_tone_vol());
#endif
#endif
#if TCFG_USER_TWS_ENABLE
    file_dec->tws_align_step = 0;
    if (!file_dec->audio_sync && (!file_dec->dec_mix || !bt_audio_is_running())) {
        file_dec->audio_sync = local_file_dec_sync_open(file_dec->ch_num, fmt->sample_rate, audio_mixer_get_sample_rate(&mixer));
    }
    if (file_dec->audio_sync) {
        audio_mixer_ch_set_event_handler(&file_dec->mix_ch, file_dec, tone_mixer_ch_event_handler);
    }
#endif

__dec_start:
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    if ((tone_input.coding_type == AUDIO_CODING_WAV) && (tone_dec->preemption == 0)) {
        audio_digital_vol_bg_fade(1);
    }
    file_dec->dvol = audio_digital_vol_open(SYS_DEFAULT_TONE_VOL, SYS_MAX_VOL, 20);
#endif/*VOL_TYPE_DIGITAL*/
#if TCFG_USER_TWS_ENABLE
    file_dec->tws_align_step = file_dec->tws ? TONE_DEC_NOT_START : TONE_DEC_CONFIRM;
#endif
    err = audio_decoder_start(&file_dec->decoder);
    if (err) {
        goto __err2;
    }
    file_dec->start = 1;

    return 0;

__err2:
#if TCFG_APP_FM_EMITTER_EN

#else
    audio_mixer_ch_close(&file_dec->mix_ch);
#endif
__err1:
    audio_decoder_close(&file_dec->decoder);
    if (file_dec->hw_src) {
        audio_hw_src_stop(file_dec->hw_src);
        audio_hw_src_close(file_dec->hw_src);
        free(file_dec->hw_src);
        file_dec->hw_src = NULL;
        log_info("hw_src close end\n");
    }

    tone_file_dec_release();
    return err;
}

static int tone_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;

    if (event == AUDIO_RES_GET) {
        err = tone_file_dec_start();
    } else if (event == AUDIO_RES_PUT) {
        tone_file_list_stop(0);
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

    if ((tone_input.coding_type == AUDIO_CODING_AAC) || (tone_input.coding_type == AUDIO_CODING_WAV)) {
        printf("tone_play end,clk restore:%d", file_dec->clk_before_dec);
        clk_set_sys_lock(file_dec->clk_before_dec, 2);
    }

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

static int sine_fread(struct audio_decoder *decoder, void *buf, u32 len)
{
    int offset;
    u8 *data = (u8 *)buf;

    offset = sin_tone_make(sine_dec->sin_maker, data, len);
    sine_dec->sine_offset += offset;

    return offset;
}

static int sine_fseek(struct audio_decoder *decoder, u32 offset, int seek_mode)
{
    sine_dec->sine_offset = 0;
    return 0;
}

static int sine_flen(struct audio_decoder *decoder)
{
    return sin_tone_points(sine_dec->sin_maker) * 2;
}



static const struct audio_dec_input sine_input = {
    .coding_type = AUDIO_CODING_PCM,
    .data_type   = AUDIO_INPUT_FILE,
    .ops = {
        .file = {
            .fread = sine_fread,
            .fseek = sine_fseek,
            .flen  = sine_flen,
        }
    }
};


static void sine_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    log_info("sin_dec_event:%x", argv[0]);
    switch (argv[0]) {
    case AUDIO_DEC_EVENT_END:
    case AUDIO_DEC_EVENT_ERR:
        log_info("sine player end\n");
        sine_dec_close();
        break;
    default:
        return;
    }

}





static void sine_param_resample(struct sin_param *dst, const struct sin_param *src, u32 sample_rate)
{
    u32 coef = (sample_rate << 10) / DEFAULT_SINE_SAMPLE_RATE;

    dst->freq = (src->freq << 10) / coef;
    dst->points = ((u64)src->points * coef) >> 10;
    dst->win = src->win;
    if (src->win) {
        dst->decay = ((u64)src->decay << 10) / coef;
    } else {
        dst->decay = ((u64)SINE_TOTAL_VOLUME * src->decay / 100) / dst->points;
    }
}

static struct sin_param *get_default_sine_param(const struct sin_param *data, u32 sample_rate, u8 data_num)
{
    int i = 0;
    for (i = 0; i < data_num; i++) {
        /*sin_dynamic_params[i].idx_increment = ((u64)data[i].idx_increment << 8) / coef;*/
        sine_param_resample(&sine_dec->sin_dynamic_params[i], data + i, sample_rate);
    }
    return sine_dec->sin_dynamic_params;
}

struct sine_param_head {
    u16 repeat_time;
    u8  set_cnt;
    u8  cur_cnt;
};

static struct sin_param *get_sine_file_param(const char *name, u32 sample_rate, u8 *data_num)
{
    FILE *file;
    struct sine_param_head head;
    struct sin_param param;
    int r_len = 0;
    int i = 0;

    file = fopen(name, "r");
    if (!file) {
        return NULL;
    }

    r_len = fread(file, (void *)&head, sizeof(head));
    if (r_len != sizeof(head)) {
        fclose(file);
        return NULL;
    }

    do {
        r_len = fread(file, (void *)&param, sizeof(param));
        if (r_len != sizeof(param)) {
            break;
        }
        /*
        printf("sine param : \nfreq : %d\npoints : %d\nwin : %d\ndecay : %d\n",
               param.freq, param.points, param.win, param.decay);
        */
        if (!param.points) {
            break;
        }

        if (!param.win) {
            param.decay = param.decay * 100 / 32767;
        }
        sine_param_resample(&sine_dec->sin_dynamic_params[i], (const struct sin_param *)&param, sample_rate);
        i++;
    } while (1);

    *data_num = i;
    fclose(file);

    return sine_dec->sin_dynamic_params;
}

#if 0
static const struct sin_param *get_sine_param_data(u8 id, u8 *num)
{
    const struct sin_param *param_data;

    switch (id) {
    case SINE_WTONE_NORAML:
        param_data = sine_16k_normal;
        *num = ARRAY_SIZE(sine_16k_normal);
        break;
#if CONFIG_USE_DEFAULT_SINE
    case SINE_WTONE_TWS_CONNECT:
        param_data = sine_tws_connect_16k;
        *num = ARRAY_SIZE(sine_tws_connect_16k);
        break;
    case SINE_WTONE_TWS_DISCONNECT:
        param_data = sine_tws_disconnect_16k;
        *num = ARRAY_SIZE(sine_tws_disconnect_16k);
        break;
    case SINE_WTONE_LOW_POWER:
        param_data = sine_low_power;
        *num = ARRAY_SIZE(sine_low_power);
        break;
    case SINE_WTONE_RING:
        param_data = sine_ring;
        *num = ARRAY_SIZE(sine_ring);
        break;
    case SINE_WTONE_MAX_VOLUME:
        param_data = sine_tws_max_volume;
        *num = ARRAY_SIZE(sine_tws_max_volume);
        break;
#ifdef SINE_WTONE_LOW_LATENRY_IN
    case SINE_WTONE_LOW_LATENRY_IN:
        param_data = sine_low_latency_in;
        *num = ARRAY_SIZE(sine_low_latency_in);
        break;
    case SINE_WTONE_LOW_LATENRY_OUT:
        param_data = sine_low_latency_out;
        *num = ARRAY_SIZE(sine_low_latency_out);
        break;
#endif
#endif
    default:
        return NULL;
    }

    return param_data;
}
#endif
static get_sine_param_t get_sine_param_data = NULL;

void tone_play_set_sine_param_handler(get_sine_param_t handler)
{
    get_sine_param_data = handler;
}

static struct sin_param *get_sine_param(u32 sine_id, u32 sample_rate, u8 *data_num)
{
    const struct sin_param *sin_data_param;
    u8 num = 0;

    if (IS_DEFAULT_SINE(sine_id)) {
        if (!get_sine_param_data) {
            return NULL;
        }
        sin_data_param = get_sine_param_data(DEFAULT_SINE_ID(sine_id), &num);
        if (!sin_data_param) {
            return NULL;
        }
        *data_num = num;
        return get_default_sine_param(sin_data_param, sample_rate, num);
    } else {
        return get_sine_file_param((const char *)sine_id, sample_rate, data_num);
    }

}


static int sine_dec_probe_handler(struct audio_decoder *decoder)
{
    u8 num = 0;
    const struct sin_param *param;

    if (!sine_dec->sin_maker) {
#ifdef CONFIG_CPU_BR18
        int channel     = 2;
#else
        int channel     = audio_dac_get_channel(&dac_hdl) == DAC_OUTPUT_LR ? 2 : 1;
#endif
        int sample_rate = audio_mixer_get_sample_rate(&mixer);
        if (sample_rate == 0) {
            sample_rate = 16000;
        }

        printf("sine: %d, %d\n", sample_rate, channel);

        param = get_sine_param(sine_dec->sine_id, sample_rate, &num);
        if (!param) {
            return -ENOENT;
        }
        sine_dec->sin_maker = sin_tone_open(param, num, channel, sine_dec->repeat);
        if (!sine_dec->sin_maker) {
            return -ENOENT;
        }
        audio_mixer_ch_set_sample_rate(&sine_dec->mix_ch, sample_rate);

        /*if (audio_dac_idle(&dac_hdl)) {
            audio_dac_set_analog_vol(&dac_hdl, 20);
            audio_dac_start(&dac_hdl);
        }*/
    }

    return 0;
}

static int sine_dec_output_handler(struct audio_decoder *decoder, s16 *data, int len, void *priv)
{

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    if (sine_dec->remain == 0) {
        audio_digital_vol_run(sine_dec->dvol, data, len);
    }
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/
    int wlen = audio_mixer_ch_write(&sine_dec->mix_ch, data, len);
    if (wlen != len) {
        sine_dec->remain = 1;
    } else {
        sine_dec->remain = 0;
    }
    return wlen;
}

static int sine_dec_post_handler(struct audio_decoder *decoder)
{
    return 0;
}

const struct audio_dec_handler sine_dec_handler = {
    .dec_probe  = sine_dec_probe_handler,
    .dec_output = sine_dec_output_handler,
    .dec_post   = sine_dec_post_handler,
};

static void sine_dec_release()
{
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    audio_digital_vol_close(sine_dec->dvol);
    sine_dec->dvol = NULL;
#endif/*VOL_TYPE_DIGITAL*/
    free(sine_dec);
    sine_dec = NULL;
}

int sine_dec_start()
{
    int err;
    int decode_task_state;
    struct audio_fmt *fmt;

    if (!sine_dec) {
        return -EINVAL;
    }
    if (sine_dec->start) {
        return 0;
    }

    printf("sine_dec_start: id = %x, repeat = %d\n", sine_dec->sine_id, sine_dec->repeat);

    err = audio_decoder_open(&sine_dec->decoder, &sine_input, &decode_task);
    if (err) {
        return err;
    }
    err = audio_decoder_get_fmt(&sine_dec->decoder, &fmt);

    decode_task_state = audio_decoder_task_wait_state(&decode_task);
    /*
     *以下情况需要独立设置提示音音量
     *(1)抢断播放
     *(2)当前只有提示音一个解码任务
     */
    if ((tone_dec->wait.preemption == 1) || (decode_task_state == 1)) {
        app_audio_state_switch(APP_AUDIO_STATE_WTONE, SYS_DEFAULT_SIN_VOL);
    }
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    sine_dec->dvol = audio_digital_vol_open(SYS_DEFAULT_SIN_VOL, SYS_MAX_VOL, 20);
#endif/*VOL_TYPE_DIGITAL*/

    /*
     *播放一个不打断的提示音，默认提高提高时钟
     *等提示音播放结束，再恢复原先的时钟
     */
    if (tone_dec->wait.preemption == 0) {
        u32 cur_clk = clk_get("sys");
        u32 need_clk = cur_clk + 12000000L;
        printf("sin_tone_clk_inc:%d->%d", cur_clk, need_clk);
        sine_dec->clk_before = clk_get("sys");
        clk_set_sys_lock(need_clk, 1);
    }

    audio_decoder_set_handler(&sine_dec->decoder, &sine_dec_handler);
    audio_decoder_set_event_handler(&sine_dec->decoder, sine_dec_event_handler, 0);

    audio_mixer_ch_open(&sine_dec->mix_ch, &mixer);

    err = audio_decoder_start(&sine_dec->decoder);
    if (err) {
        goto __err2;
    }
    sine_dec->start = 1;

    return 0;

__err2:
    audio_mixer_ch_close(&sine_dec->mix_ch);
__err1:
    audio_decoder_close(&sine_dec->decoder);
    sine_dec_release();
    return err;

}

static int sine_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;

    if (event == AUDIO_RES_GET) {
        err = sine_dec_start();
    } else if (event == AUDIO_RES_PUT) {

    }

    return err;
}

int sine_dec_open(u32 sine_id, u8 repeat, u8 preemption)
{
    int err = 0;

    if (sine_dec) {
        sine_dec_close();
        if (sine_dec) {
            return -EINVAL;
        }
    }

    sine_dec = zalloc(sizeof(*sine_dec));
    if (!sine_dec) {
        log_error("sine_dec zalloc failed");
        return -ENOMEM;
    }

    sine_dec->repeat      = repeat;
    sine_dec->sine_id     = sine_id;

    tone_dec->wait.priority = 3;
    tone_dec->wait.preemption = preemption;
    tone_dec->wait.handler = sine_wait_res_handler;
    printf("sine_dec_open,preemption = %d", preemption);
    audio_decoder_task_add_wait(&decode_task, &tone_dec->wait);

    if (preemption == 0 && sine_dec->start == 0) {
        err = sine_dec_start();
    }

    return err;
}

int sine_dec_close(void)
{
    if (!sine_dec) {
        return 0;
    }

    puts("sine_dec_close\n");

    audio_decoder_close(&sine_dec->decoder);
    audio_mixer_ch_close(&sine_dec->mix_ch);
    if (sine_dec->sin_maker) {
        sin_tone_close(sine_dec->sin_maker);
    }

    if (app_audio_get_state() == APP_AUDIO_STATE_WTONE) {
        app_audio_state_exit(APP_AUDIO_STATE_WTONE);
    }

    if (tone_dec->wait.preemption == 0) {
        printf("sin_tone_clk_reset:%d", sine_dec->clk_before);
        if (sine_dec->clk_before) {
            clk_set_sys_lock(sine_dec->clk_before, 1);
            sine_dec->clk_before = 0;
        }
    }
    sine_dec_release();

    tone_dec_end_handler(AUDIO_DEC_EVENT_END, NULL);

    return 0;
}


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

    if (IS_DEFAULT_SINE(list[index])) {
        format = "sin";
    } else {
        file = fopen(list[index], "r");
        if (!file) {
            return -EINVAL;
        }
        fget_name(file, file_name, 16);
        format = get_file_ext_name((char *)file_name);
    }

    if (ASCII_StrCmpNoCase(format, "sin", 3) == 0) {
        if (file) {
            fclose(file);
        }
        /*正弦波参数文件*/
        return sine_dec_open((u32)list[index], index == 1, preemption);
    } else {
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
        /*AAC提示音默认打断播放*/
        if (ASCII_StrCmpNoCase(format, "aac", 3) == 0) {
            printf("aac tone,preemption = 1\n");
            tone_dec->wait.preemption = 1;
            tone_dec->wait.format = AUDIO_CODING_AAC;
        } else {
#if TONE_FILE_DEC_MIX
            if (tone_dec->wait.preemption == 0) {
                /*支持叠加的提示音解码文件格式*/
                if ((ASCII_StrCmpNoCase(format, "wav", 3) == 0) || (ASCII_StrCmpNoCase(format, "wtg", 3) == 0)) {
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
        err = audio_decoder_task_add_wait(&decode_task, &tone_dec->wait);
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
        if (err) {
            audio_decoder_task_del_wait(&decode_task, &tone_dec->wait);
        } else {
            if (file_dec->dec_mix && !file_dec->start) {
                err = tone_file_dec_start();
            }
        }
        return err;
    }
}

static int __tone_file_list_play(const char **list, u8 preemption, u8 tws)
{
    int i = 0;
    int err = 0;

    if (!list) {
        return -EINVAL;
    }


    if (tone_dec == NULL) {
        tone_dec = zalloc(sizeof(*tone_dec));
        if (tone_dec == NULL) {
            log_error("tone dec zalloc failed");
            return -ENOMEM;
        }
    }

    while (list[i] != NULL) {
        i++;
    }
    char **p = malloc(4 * (i + 1));
    memcpy(p, list, 4 * (i + 1));

    tone_dec->list[tone_dec->w_index++] = (const char **)p;
    if (tone_dec->w_index >= TONE_LIST_MAX_NUM) {
        tone_dec->w_index = 0;
    }


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
    g_printf("tone_play:%s,preemption:%d", IS_DEFAULT_SINE(name) ? "sine" : name, preemption);

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
    g_printf("tone_play no tws:%s,preemption:%d", IS_DEFAULT_SINE(name) ? "sine" : name, preemption);

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
    g_printf("tone_play:%s,preemption:%d", IS_DEFAULT_SINE(name) ? "sine" : name, preemption);

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
    g_printf("tone_play_add:%s,preemption:%d", IS_DEFAULT_SINE(name) ? "sine" : name, preemption);

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
    sine_dec_close();
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
        } else if (sine_dec) {
            printf("sin_id:0x%x,cmp_id:0x%x\n", sine_dec->sine_id, (u32)name);
            if (sine_dec->sine_id == (u32)name) {
                return 0;
            } else {
                return -1;
            }
        }
    }
    printf("tone_dec idle now\n");
    return -1;
}
