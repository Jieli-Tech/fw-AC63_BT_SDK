
#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "classic/hci_lmp.h"
#include "asm/audio_src.h"
#include "audio_enc.h"
#include "app_main.h"
#include "btstack/avctp_user.h"
#include "audio_config.h"
#include "sbc_enc.h"
#include "api/mesh_config.h"

#if ENC_DEMO_EN
const unsigned char sin44K[88] ALIGNED(4) = {
    0x00, 0x00, 0x45, 0x0E, 0x41, 0x1C, 0xAA, 0x29, 0x3B, 0x36, 0xB2, 0x41, 0xD5, 0x4B, 0x6E, 0x54,
    0x51, 0x5B, 0x5A, 0x60, 0x70, 0x63, 0x82, 0x64, 0x8A, 0x63, 0x8E, 0x60, 0x9D, 0x5B, 0xD1, 0x54,
    0x4D, 0x4C, 0x3D, 0x42, 0xD5, 0x36, 0x50, 0x2A, 0xF1, 0x1C, 0xFB, 0x0E, 0xB7, 0x00, 0x70, 0xF2,
    0x6E, 0xE4, 0xFD, 0xD6, 0x60, 0xCA, 0xD9, 0xBE, 0xA5, 0xB4, 0xF7, 0xAB, 0xFC, 0xA4, 0xDA, 0x9F,
    0xAB, 0x9C, 0x7F, 0x9B, 0x5E, 0x9C, 0x3F, 0x9F, 0x19, 0xA4, 0xCE, 0xAA, 0x3D, 0xB3, 0x3A, 0xBD,
    0x92, 0xC8, 0x0A, 0xD5, 0x60, 0xE2, 0x50, 0xF0
};

extern struct audio_encoder_task *encode_task;
#define ENC_BUF_NUM        (2)
#define ENC_ADC_IRQ_POINTS     (320)
#define ENC_ADC_BUFS_SIZE      (ENC_BUF_NUM * ENC_ADC_IRQ_POINTS)

#define MIC_USE_MIC_CHANNEL    (1)
#define ENC_IN_SIZE		(ENC_ADC_IRQ_POINTS * 2)
#define ENC_OUT_SIZE       (ENC_ADC_IRQ_POINTS)
#define ENC_CLK  96 * 1000000L    //编码时候的时钟

struct demo_enc_hdl {
    struct audio_encoder encoder;
    OS_SEM pcm_frame_sem;
    u8 output_frame[ENC_OUT_SIZE];
    u8  pcm_frame[ENC_IN_SIZE];
    u8 frame_size;
    u8 in_cbuf_buf[ENC_IN_SIZE * 6];
    cbuffer_t pcm_in_cbuf;
    u32 clk_before;
#if mic_enc_PACK_ENABLE
    u16 cp_type;
    u16 packet_head_sn;
#endif
#if MIC_USE_MIC_CHANNEL
    struct audio_adc_output_hdl adc_output;
    struct adc_mic_ch mic_ch;
    s16 adc_buf[ENC_ADC_BUFS_SIZE];    //align 4Bytes
#endif
    int (*demo_output)(void *priv, void *buf, int len);

};

static struct demo_enc_hdl *demo_enc = NULL;


static u16 demo_frame_test_tmr = 0;

static void demo_enc_output_func_register(int (*output_func)(void *priv, void *buf, int len))
{
    if (demo_enc) {
        demo_enc->demo_output = output_func;
    }
}

void demo_enc_resume(void)
{
    if (demo_enc) {
        os_sem_post(&demo_enc->pcm_frame_sem);
    }
}

static int demo_enc_pcm_get(struct audio_encoder *encoder, s16 **frame, u16 frame_len)
{
    int pcm_len = 0;
    if (demo_enc == NULL) {
        r_printf("demo_enc NULL\n");
        return 0;
    }
    /* putchar('!'); */
    if ((&demo_enc->pcm_in_cbuf)->data_len < frame_len) {
        /* putchar('#'); */
        os_sem_set(&demo_enc->pcm_frame_sem, 0);
        os_sem_pend(&demo_enc->pcm_frame_sem, 5);
        if (demo_enc == NULL) {
            printf("demo_enc is NULL\n");
            return 0;
        }
    }

    pcm_len = cbuf_read(&demo_enc->pcm_in_cbuf, demo_enc->pcm_frame, frame_len);

    if (pcm_len != frame_len) {
        putchar('L');
    }
    /* putchar('D'); */

    *frame = demo_enc->pcm_frame;
    return pcm_len;
}

static void demo_enc_pcm_put(struct audio_encoder *encoder, s16 *frame)
{
}

static const struct audio_enc_input demo_enc_input = {
    .fget = demo_enc_pcm_get,
    .fput = demo_enc_pcm_put,
};

static int demo_enc_probe_handler(struct audio_encoder *encoder)
{
    return 0;
}

static int demo_enc_output_handler(struct audio_encoder *encoder, u8 *frame, int len)
{
    if (encoder == NULL) {
        r_printf("encoder NULL");
    }
    wdt_clear();
    printf("demo frame len:%d \n", len);
    if (demo_enc && demo_enc->demo_output) {
        demo_enc->demo_output(NULL, frame, len);
    }
    put_buf(frame, 16);
    return len;
}

const static struct audio_enc_handler demo_enc_handler = {
    .enc_probe = demo_enc_probe_handler,
    .enc_output = demo_enc_output_handler,
};

static void demo_enc_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    printf("demo_enc_event_handler:0x%x,%d\n", argv[0], argv[0]);
    switch (argv[0]) {
    case AUDIO_ENC_EVENT_END:
        puts("AUDIO_ENC_EVENT_END\n");
        break;
    }
}

int read_44k_sine_data(void *buf, int bytes, int offset, u8 channel)
{
    s16 *sine = (s16 *)sin44K;
    s16 *data = (s16 *)buf;
    int frame_len = (bytes >> 1) / channel;
    int sin44k_frame_len = sizeof(sin44K) / 2;
    int i, j;

    offset = offset % sin44k_frame_len;

    for (i = 0; i < frame_len; i++) {
        for (j = 0; j < channel; j++) {
            *data++ = sine[offset];
        }
        if (++offset >= sin44k_frame_len) {
            offset = 0;
        }
    }

    return i * 2 * channel;
}

static  u16 frames_offset = 0;
static  s16 frame_bytes = 0;
static  s16 pcm_frames[640];

static void adc_mic_output_handler(void *priv, s16 *data, int len)
{
    if (demo_enc) {
        u16 wlen = cbuf_write(&demo_enc->pcm_in_cbuf, data, len);
        if (wlen != len) {
            putchar('@');
        }
        audio_encoder_resume(&demo_enc->encoder);
        demo_enc_resume();
    }
}

static void demo_frame_test_time_func(void *param)
{
    u32 len = 320; //每次写的字节数

    frame_bytes = read_44k_sine_data(pcm_frames, len, frames_offset, 1);
    frames_offset += (frame_bytes >> 1) / 1;   //固定传320
    if (demo_enc) {
        /* put_buf(pcm_frames,len); */   //是不是这里写数据写漏了
        u16 wlen = cbuf_write(&demo_enc->pcm_in_cbuf, pcm_frames, len);
        if (wlen != len) {
            putchar('@');
        }

        audio_encoder_resume(&demo_enc->encoder);
        demo_enc_resume();
    }
}

#define HIGHT_COMPLEX		0
#define LOW_COMPLEX			(BIT(4))

extern struct audio_adc_hdl adc_hdl;
int audio_demo_enc_open(int (*demo_output)(void *priv, void *buf, int len), u32 code_type, u8 ai_type)  //第一个参数为编码输出回调，在回调中可以对编码后的数据进行处理
{
    int err;
    struct audio_fmt fmt = {0};
    switch (code_type) {
#if TCFG_ENC_OPUS_ENABLE
    case AUDIO_CODING_OPUS:
        //1. quality:bitrate     0:16kbps    1:32kbps    2:64kbps
        //   quality: MSB_2:(bit7_bit6)     format_mode    //0:百度_无头.                   1:酷狗_eng+range.
        //   quality:LMSB_2:(bit5_bit4)     low_complexity //0:高复杂度,高质量.兼容之前库.  1:低复杂度,低质量.
        //2. sample_rate         sample_rate=16k         ignore
        fmt.quality = 0 | ai_type/*| LOW_COMPLEX*/;  //32位的
        fmt.sample_rate = 16000;
        fmt.coding_type = AUDIO_CODING_OPUS;
        break;
#endif

#if TCFG_ENC_SPEEX_ENABLE
    case AUDIO_CODING_SPEEX:
        fmt.quality = 5;
        fmt.complexity = 2;
        fmt.sample_rate = 16000;
        fmt.coding_type = AUDIO_CODING_SPEEX;
        break;
#endif

#if TCFG_ENC_ADPCM_ENABLE
    case AUDIO_CODING_WAV:   //adpcm 编码,在lib_media_config.c 里面配置 const_sel_adpcm_type 来选择 ima格式的或者ms格式的
        fmt.sample_rate = 16000;
        fmt.bit_rate = 1024;  //blockSize,可配成256/512/1024/2048
        fmt.channel = 2;
        fmt.coding_type = AUDIO_CODING_WAV;
        break;
#endif

#if TCFG_ENC_LC3_ENABLE
    case AUDIO_CODING_LC3:
        fmt.bit_rate = 64000;
        fmt.sample_rate = LC3_CODING_SAMPLERATE;
        fmt.frame_len = LC3_CODING_FRAME_LEN;
        fmt.channel = LC3_CODING_CHANNEL;
        fmt.coding_type = AUDIO_CODING_LC3;
        break;
#endif

#if TCFG_ENC_SBC_ENABLE
    case AUDIO_CODING_SBC:
        sbc_t sbc_enc_parm = {
            .frequency = SBC_FREQ_44100,
            .blocks = SBC_BLK_16,
            .subbands = SBC_SB_8,
            .mode = SBC_MODE_STEREO,
            .allocation = 0,
            .endian = SBC_LE,
            .bitpool = 53
        };
        fmt.priv = (void *)(&sbc_enc_parm);
        fmt.frame_len = 640; //sbc 编码每次读取的源数据的字节数,值要大于编成一个帧需要的长度
        fmt.coding_type = AUDIO_CODING_SBC;
        break;
#endif

#if TCFG_ENC_MSBC_ENABLE
    case AUDIO_CODING_MSBC:  //msbc没有外部配置的参数
        fmt.coding_type = AUDIO_CODING_MSBC;
        break;
#endif

    default:
        printf("do not support this type !!!\n");
        return -1;
        break;
    }

    if (!encode_task) {
        encode_task = zalloc(sizeof(*encode_task));
        if (!encode_task) {
            printf("encode_task NULL !!!\n");
        }
        audio_encoder_task_create(encode_task, "audio_enc");
    }
    if (!demo_enc) {
        demo_enc = zalloc(sizeof(*demo_enc));
        if (!demo_enc) {
            printf("demo_enc NULL !!!\n");
        }
        memset(demo_enc, 0x00, sizeof(*demo_enc));
    }

    demo_enc_output_func_register(demo_output);

    cbuf_init(&demo_enc->pcm_in_cbuf, demo_enc->in_cbuf_buf, ENC_IN_SIZE * 6);
    os_sem_create(&demo_enc->pcm_frame_sem, 0);
    audio_encoder_open(&demo_enc->encoder, &demo_enc_input, encode_task);
    audio_encoder_set_handler(&demo_enc->encoder, &demo_enc_handler);
    audio_encoder_set_fmt(&demo_enc->encoder, &fmt);
    audio_encoder_set_event_handler(&demo_enc->encoder, demo_enc_event_handler, 0);
    audio_encoder_set_output_buffs(&demo_enc->encoder, demo_enc->output_frame,
                                   sizeof(demo_enc->output_frame), 1);
    if (!demo_enc->encoder.enc_priv) {
        log_e("encoder err, maybe coding(0x%x) disable \n", fmt.coding_type);
        err = -EINVAL;
        goto __err;
    }

    int start_err = audio_encoder_start(&demo_enc->encoder);
    // 用timer模拟填数,填入需要编码的源数据
    /* demo_frame_test_tmr = sys_hi_timer_add(NULL, demo_frame_test_time_func, 40); */
    /* printf("id:%d \n", demo_frame_test_tmr); */
//把mic采到的数据编码
#if MIC_USE_MIC_CHANNEL
    demo_enc->clk_before = clk_get("sys");
    if (demo_enc->clk_before < ENC_CLK) {
        clk_set("sys", ENC_CLK);
    }
    u32 clk_debug = clk_get("sys");
    fmt.sample_rate = 16000;
#if !defined(CONFIG_MEDIA_DEVELOP_ENABLE)
    audio_mic_pwr_ctl(MIC_PWR_ON);
#endif
    audio_adc_mic_open(&demo_enc->mic_ch, AUDIO_ADC_MIC_CH, &adc_hdl);
    audio_adc_mic_set_sample_rate(&demo_enc->mic_ch, fmt.sample_rate);
#if (TCFG_AUDIO_ENABLE && MESH_AUDIO_TEST)
    app_var.aec_mic_gain = 14;
#endif
    printf(">>>>>>>>>mic gain:%d \n", app_var.aec_mic_gain);
    audio_adc_mic_set_gain(&demo_enc->mic_ch, app_var.aec_mic_gain);
    audio_adc_mic_set_buffs(&demo_enc->mic_ch, demo_enc->adc_buf,
                            ENC_ADC_IRQ_POINTS * 2, ENC_BUF_NUM);
    demo_enc->adc_output.handler = adc_mic_output_handler;
    audio_adc_add_output_handler(&adc_hdl, &demo_enc->adc_output);
    //app_audio_output_samplerate_set(44100);
    //app_audio_output_start();
    audio_adc_mic_start(&demo_enc->mic_ch);
#endif
    printf("demo_enc_open ok %d\n", start_err);
    return 0;
__err:
    audio_encoder_close(&demo_enc->encoder);
    local_irq_disable();
    free(demo_enc);
    demo_enc = NULL;
    local_irq_enable();
    return err;
}

int audio_demo_enc_close()
{
    if (!demo_enc) {
        return -1;
    }
    printf("audio_demo_enc_close\n");
#if MIC_USE_MIC_CHANNEL
    clk_set("sys", demo_enc->clk_before);
    audio_adc_mic_close(&demo_enc->mic_ch);
    audio_adc_del_output_handler(&adc_hdl, &demo_enc->adc_output);
#endif

    demo_enc_resume();
    audio_encoder_close(&demo_enc->encoder);

    /* if (encode_task) { */
    /* audio_encoder_task_del(encode_task); */
    /* free(encode_task); */
    /* encode_task = NULL; */
    /* } */

    free(demo_enc);
    demo_enc = NULL;

    printf("audio_demo_enc_close end\n");
    return 0;
}

#endif
