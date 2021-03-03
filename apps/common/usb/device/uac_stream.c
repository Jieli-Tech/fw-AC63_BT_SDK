#include "app_config.h"
#include "system/includes.h"
#include "printf.h"
#include "usb/usb_config.h"
#include "usb/device/usb_stack.h"

#if TCFG_PC_ENABLE && (USB_DEVICE_CLASS_CONFIG & AUDIO_CLASS)
#include "usb/device/uac_audio.h"
#include "uac_stream.h"
#include "audio_config.h"

#ifdef CONFIG_MEDIA_DEVELOP_ENABLE
#include "audio_track.h"
#endif


#define LOG_TAG_CONST       USB
#define LOG_TAG             "[UAC]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define     UAC_DEBUG_ECHO_MODE 0

static volatile u8 speaker_stream_is_open = 0;
struct uac_speaker_handle {
    cbuffer_t cbuf;
    volatile u8 need_resume;
    u8 channel;
    void *buffer;
    void *audio_track;
    void (*rx_handler)(int, void *, int);
};

#if (SOUNDCARD_ENABLE)
#define UAC_BUFFER_SIZE     (4 * 1024)
#else
#define UAC_BUFFER_SIZE     (2 * 1024)
#endif

#define UAC_BUFFER_MAX		(UAC_BUFFER_SIZE * 50 / 100)

static struct uac_speaker_handle *uac_speaker = NULL;

#if USB_MALLOC_ENABLE
#else
static struct uac_speaker_handle uac_speaker_handle SEC(.uac_var);
static u8 uac_rx_buffer[UAC_BUFFER_SIZE] ALIGNED(4) SEC(.uac_rx);
#endif
u32 uac_speaker_stream_length()
{
    return UAC_BUFFER_SIZE;
}
u32 uac_speaker_stream_size()
{
    if (!speaker_stream_is_open) {
        return 0;
    }

    if (uac_speaker) {
        return cbuf_get_data_size(&uac_speaker->cbuf);
    }

    return 0;
}

void uac_speaker_stream_buf_clear(void)
{
    if (speaker_stream_is_open) {
        cbuf_clear(&uac_speaker->cbuf);
    }
}

void set_uac_speaker_rx_handler(void *priv, void (*rx_handler)(int, void *, int))
{
    if (uac_speaker) {
        uac_speaker->rx_handler = rx_handler;
    }
}

int uac_speaker_stream_sample_rate(void)
{
#ifdef CONFIG_MEDIA_DEVELOP_ENABLE
    if (uac_speaker && uac_speaker->audio_track) {
        return audio_local_sample_track_rate(uac_speaker->audio_track);
    }
#endif
    return 48000;
}

void uac_speaker_stream_write(const u8 *obuf, u32 len)
{
    if (speaker_stream_is_open) {
        //write dac
#ifdef CONFIG_MEDIA_DEVELOP_ENABLE
        if (uac_speaker->audio_track) {
            audio_local_sample_track_in_period(uac_speaker->audio_track, (len >> 1) / uac_speaker->channel);
        }
#endif
        int wlen = cbuf_write(&uac_speaker->cbuf, (void *)obuf, len);
        if (wlen != len) {
            putchar('W');
        }
        if (uac_speaker->rx_handler) {
            if (uac_speaker->cbuf.data_len >= UAC_BUFFER_MAX) {
                // 马上就要满了，赶紧取走
                /*uac_speaker->need_resume = 1; //2020-12-22注:无需唤醒*/
            }
            if (uac_speaker->need_resume) {
                uac_speaker->need_resume = 0;
                uac_speaker->rx_handler(0, (void *)obuf, len);
            }
        }
    }
}

int uac_speaker_read(void *priv, void *data, u32 len)
{
    int r_len;
    int err = 0;

    local_irq_disable();
    if (!speaker_stream_is_open) {
        local_irq_enable();
        return 0;
    }

    r_len = cbuf_get_data_size(&uac_speaker->cbuf);
    if (r_len) {
        r_len = r_len > len ? len : r_len;
        r_len = cbuf_read(&uac_speaker->cbuf, data, r_len);
        if (!r_len) {
            putchar('U');
        }
    }

    if (r_len == 0) {
        uac_speaker->need_resume = 1;
    }
    local_irq_enable();
    return r_len;
}

void uac_speaker_stream_open(u32 samplerate, u32 ch)
{
    if (speaker_stream_is_open) {
        return;
    }
    log_info("%s", __func__);

    if (!uac_speaker) {
#if USB_MALLOC_ENABLE

        uac_speaker = zalloc(sizeof(struct uac_speaker_handle));
        if (!uac_speaker) {
            return;
        }

        uac_speaker->buffer = malloc(UAC_BUFFER_SIZE);
        if (!uac_speaker->buffer) {
            free(uac_speaker);
            uac_speaker = NULL;
            goto __err;
        }


#else

        uac_speaker = &uac_speaker_handle;

        memset(uac_speaker, 0, sizeof(struct uac_speaker_handle));

        uac_speaker->buffer = uac_rx_buffer;
#endif
        uac_speaker->channel = ch;
#ifdef CONFIG_MEDIA_DEVELOP_ENABLE
        uac_speaker->audio_track = audio_local_sample_track_open(ch, samplerate, 1000);
#endif
    }


    uac_speaker->rx_handler = NULL;

    cbuf_init(&uac_speaker->cbuf, uac_speaker->buffer, UAC_BUFFER_SIZE);
    speaker_stream_is_open = 1;
    struct sys_event event;
    event.type = SYS_DEVICE_EVENT;
    event.arg = (void *)DEVICE_EVENT_FROM_UAC;
    event.u.dev.event = USB_AUDIO_PLAY_OPEN;
    event.u.dev.value = (int)((ch << 24) | samplerate);

#if !UAC_DEBUG_ECHO_MODE
    sys_event_notify(&event);
#endif

    return;

__err:
    return;
}

void uac_speaker_stream_close()
{
    if (speaker_stream_is_open == 0) {
        return;
    }

    log_info("%s", __func__);
    speaker_stream_is_open = 0;

    if (uac_speaker) {
#ifdef CONFIG_MEDIA_DEVELOP_ENABLE
        audio_local_sample_track_close(uac_speaker->audio_track);
#endif
        uac_speaker->audio_track = NULL;
#if USB_MALLOC_ENABLE
        if (uac_speaker->buffer) {
            free(uac_speaker->buffer);
        }
        free(uac_speaker);
#endif
        uac_speaker = NULL;
    }
    struct sys_event event;
    event.type = SYS_DEVICE_EVENT;
    event.arg = (void *)DEVICE_EVENT_FROM_UAC;
    event.u.dev.event = USB_AUDIO_PLAY_CLOSE;
    event.u.dev.value = (int)0;

    sys_event_notify(&event);
}

int uac_get_spk_vol()
{
#if USB_DEVICE_CLASS_CONFIG & AUDIO_CLASS
    int max_vol = get_max_sys_vol();
    int vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
    if (vol * 100 / max_vol < 100) {
        return vol * 100 / max_vol;
    } else {
        return 99;
    }
#endif
    return 0;
}
static u32 mic_stream_is_open;
void uac_mute_volume(u32 type, u32 l_vol, u32 r_vol)
{
    struct sys_event event;
    event.type = SYS_DEVICE_EVENT;
    event.arg = (void *)DEVICE_EVENT_FROM_UAC;

    static u32 last_spk_l_vol = (u32) - 1, last_spk_r_vol = (u32) - 1;
    static u32 last_mic_vol = (u32) - 1;

    switch (type) {
    case MIC_FEATURE_UNIT_ID: //MIC
        if (mic_stream_is_open == 0) {
            return ;
        }
        if (l_vol == last_mic_vol) {
            return;
        }
        last_mic_vol = l_vol;
        event.u.dev.event = USB_AUDIO_SET_MIC_VOL;
        break;
    case SPK_FEATURE_UNIT_ID: //SPK
        if (speaker_stream_is_open == 0) {
            return;
        }
        if (l_vol == last_spk_l_vol && r_vol == last_spk_r_vol) {
            return;
        }
        last_spk_l_vol = l_vol;
        last_spk_r_vol = r_vol;
        event.u.dev.event = USB_AUDIO_SET_PLAY_VOL;
        break;
    default:
        break;
    }

    event.u.dev.value = (int)(r_vol << 16 | l_vol);
    sys_event_notify(&event);
}


static int (*mic_tx_handler)(int, void *, int) SEC(.uac_rx);
int uac_mic_stream_read(u8 *buf, u32 len)
{
    if (mic_stream_is_open == 0) {
        return 0;
    }
#if 0//48K 1ksin
    const s16 sin_48k[] = {
        0, 2139, 4240, 6270, 8192, 9974, 11585, 12998,
        14189, 15137, 15826, 16244, 16384, 16244, 15826, 15137,
        14189, 12998, 11585, 9974, 8192, 6270, 4240, 2139,
        0, -2139, -4240, -6270, -8192, -9974, -11585, -12998,
        -14189, -15137, -15826, -16244, -16384, -16244, -15826, -15137,
        -14189, -12998, -11585, -9974, -8192, -6270, -4240, -2139
    };
    u16 *l_ch = (u16 *)buf;
    u16 *r_ch = (u16 *)buf;
    r_ch++;
    for (int i = 0; i < len / 4; i++) {
        *l_ch = sin_48k[i];
        *r_ch = sin_48k[i];
        l_ch += 2;
        r_ch += 2;
    }
    return len;
#elif   UAC_DEBUG_ECHO_MODE
    uac_speaker_read(NULL, buf, len);
#if MIC_CHANNEL == 2
    const s16 sin_48k[] = {
        0, 2139, 4240, 6270, 8192, 9974, 11585, 12998,
        14189, 15137, 15826, 16244, 16384, 16244, 15826, 15137,
        14189, 12998, 11585, 9974, 8192, 6270, 4240, 2139,
        0, -2139, -4240, -6270, -8192, -9974, -11585, -12998,
        -14189, -15137, -15826, -16244, -16384, -16244, -15826, -15137,
        -14189, -12998, -11585, -9974, -8192, -6270, -4240, -2139
    };
    u16 *r_ch = (u16 *)buf;
    r_ch++;
    for (int i = 0; i < len / 4; i++) {
        *r_ch = sin_48k[i];
        r_ch += 2;
    }
#endif
    return len;
#else
    if (mic_tx_handler) {
#if 1
        return mic_tx_handler(0, buf, len);
#else
        //16bit ---> 24bit
        int rlen = mic_tx_handler(0, tmp_buf, len / 3 * 2);
        rlen /= 2; //sampe point
        for (int i = 0 ; i < rlen ; i++) {
            buf[i * 3] = 0;
            buf[i * 3 + 1] = tmp_buf[i * 2];
            buf[i * 3 + 2] = tmp_buf[i * 2 + 1];
        }
#endif
    }
    return 0;
#endif
    return 0;
}

void set_uac_mic_tx_handler(void *priv, int (*tx_handler)(int, void *, int))
{
    mic_tx_handler = tx_handler;
}

u32 uac_mic_stream_open(u32 samplerate, u32 frame_len, u32 ch)
{
    if (mic_stream_is_open) {
        return 0;
    }

    mic_tx_handler = NULL;
    log_info("%s", __func__);

    struct sys_event event;
    event.type = SYS_DEVICE_EVENT;
    event.arg = (void *)DEVICE_EVENT_FROM_UAC;
    event.u.dev.event = USB_AUDIO_MIC_OPEN;
    event.u.dev.value = (int)((ch << 24) | samplerate);
    mic_stream_is_open = 1;

#if !UAC_DEBUG_ECHO_MODE
    sys_event_notify(&event);
#endif
    return 0;
}

void uac_mic_stream_close()
{
    if (mic_stream_is_open == 0) {
        return ;
    }
    log_info("%s", __func__);
    struct sys_event event;
    event.type = SYS_DEVICE_EVENT;
    event.arg = (void *)DEVICE_EVENT_FROM_UAC;
    event.u.dev.event = USB_AUDIO_MIC_CLOSE;
    event.u.dev.value = (int)0;
    mic_stream_is_open = 0;
    sys_event_notify(&event);
}

_WEAK_
s8 app_audio_get_volume(u8 state)
{
    return 88;
}
_WEAK_
void usb_audio_demo_exit(void)
{

}
_WEAK_
int usb_audio_demo_init(void)
{
    return 0;
}
_WEAK_
u8 get_max_sys_vol(void)
{
    return 100;
}
_WEAK_
void *audio_local_sample_track_open(u8 channel, int sample_rate, int period)
{
    return NULL;
}
_WEAK_
int audio_local_sample_track_in_period(void *c, int samples)
{
    return 0;
}
_WEAK_
void audio_local_sample_track_close(void *c)
{
}
#endif
