#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "classic/hci_lmp.h"
#include "aec_user.h"
#include "app_main.h"
#include "audio_config.h"
#include "pdm_link.h"
#include "audio_enc.h"
#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif

#include "phone_message/phone_message.h"

extern struct adc_platform_data adc_data;

audio_plnk_t *audio_plnk_mic = NULL;
struct audio_adc_hdl adc_hdl;
struct esco_enc_hdl *esco_enc = NULL;
struct audio_encoder_task *encode_task = NULL;

#define ESCO_ADC_BUF_NUM        3
#define ESCO_ADC_IRQ_POINTS     256
#if TCFG_AUDIO_DUAL_MIC_ENABLE
#define ESCO_ADC_CH			    2
#else
#define ESCO_ADC_CH			    1
#endif
#define ESCO_ADC_BUFS_SIZE      (ESCO_ADC_BUF_NUM * ESCO_ADC_IRQ_POINTS * ESCO_ADC_CH)

static const u16 esco_sync_word = 0x0801; /*1000b 0000b 0001b */
static const u16 esco_seq_num[4] = {0x0, 0x3, 0xC, 0xF}; /*0000b,0011b,1100b,1111b*/

struct esco_enc_hdl {
    struct audio_encoder encoder;
    struct audio_adc_output_hdl adc_output;
    struct adc_mic_ch mic_ch;
    u8 seqN;
    //OS_SEM pcm_frame_sem;
    s16 output_frame[30];               //align 4Bytes
    int pcm_frame[60];                 //align 4Bytes
    s16 adc_buf[ESCO_ADC_BUFS_SIZE];    //align 4Bytes
#if (ESCO_ADC_CH == 2)
    s16 tmp_buf[ESCO_ADC_IRQ_POINTS];
#endif
};


/*
 *mic电源管理
 *设置mic vdd对应port的状态
 */
void audio_mic_pwr_io(u32 gpio, u8 out_en)
{
    gpio_set_die(gpio, 1);
    gpio_set_pull_up(gpio, 0);
    gpio_set_pull_down(gpio, 0);
    gpio_direction_output(gpio, out_en);
}
void audio_mic_pwr_ctl(u8 state)
{
#if TCFG_AUDIO_MIC_PWR_CTL
    switch (state) {
    case MIC_PWR_INIT:
    case MIC_PWR_OFF:
        /*mic vdd:low*/
#ifdef TCFG_AUDIO_MIC_PWR_PORT
        audio_mic_pwr_io(TCFG_AUDIO_MIC_PWR_PORT, 0);
#endif
        /*
         *mic in port config
         *mic0:PA1
         */
        audio_mic_pwr_io(IO_PORTA_01, 0);
        break;
    case MIC_PWR_ON:
        /*mic vdd:high*/
#ifdef TCFG_AUDIO_MIC_PWR_PORT
        audio_mic_pwr_io(TCFG_AUDIO_MIC_PWR_PORT, 1);
#endif
        break;
    case MIC_PWR_DOWN:
        break;
    }
#endif/*TCFG_AUDIO_MIC_PWR_CTL*/
}

void esco_enc_resume(void);

static void adc_mic_output_handler(void *priv, s16 *data, int len)
{
    //printf("buf:%x,data:%x,len:%d",esco_enc->adc_buf,data,len);
    if (esco_enc) {
#if (defined(TCFG_PHONE_MESSAGE_ENABLE) && (TCFG_PHONE_MESSAGE_ENABLE))
        int ret = phone_message_mic_write(data, len);
        if (ret >= 0) {
            esco_enc_resume();
            return ;
        }
#endif

#if (ESCO_ADC_CH == 2)/*DualMic*/
        s16 *mic0_data = data;
        s16 *mic1_data = esco_enc->tmp_buf;
        s16 *mic1_data_pos = data + (len / 2);
        //printf("mic_data:%x,%x,%d\n",data,mic1_data_pos,len);
        for (u16 i = 0; i < (len >> 1); i++) {
            mic0_data[i] = data[i * 2];
            mic1_data[i] = data[i * 2 + 1];
        }
        memcpy(mic1_data_pos, mic1_data, len);

#if 0 /*debug*/
        static u16 mic_cnt = 0;
        if (mic_cnt++ > 300) {
            putchar('1');
            audio_aec_inbuf(mic1_data_pos, len);
            if (mic_cnt > 600) {
                mic_cnt = 0;
            }
        } else {
            putchar('0');
            audio_aec_inbuf(mic0_data, len);
        }
#else
        audio_aec_inbuf_ref(mic1_data_pos, len);
        audio_aec_inbuf(data, len);
#endif/*debug end*/
#else/*SingleMic*/
        audio_aec_inbuf(data, len);
#endif/*ESCO_ADC_CH*/
    }
}

static void audio_plnk_mic_output(void *priv, void *dat, u16 len, u8 buf_flag)
{
    s16 *buf = dat;
    s16 *mic0;
    mic0 = (s16 *)buf;
    mic0 += buf_flag * len;
    if (esco_enc) {
#if (defined(TCFG_PHONE_MESSAGE_ENABLE) && (TCFG_PHONE_MESSAGE_ENABLE))
        int ret = phone_message_mic_write(mic0, len << 1);
        if (ret >= 0) {
            esco_enc_resume();
            return ;
        }
#endif
        audio_aec_inbuf(mic0, len << 1);
    }
}

__attribute__((weak)) int audio_aec_output_read(s16 *buf, u16 len)
{
    return 0;
}

void esco_enc_resume(void)
{
    if (esco_enc) {
        //os_sem_set(&esco_enc->pcm_frame_sem, 0);
        //os_sem_post(&esco_enc->pcm_frame_sem);
        audio_encoder_resume(&esco_enc->encoder);
    }
}

static int esco_enc_pcm_get(struct audio_encoder *encoder, s16 **frame, u16 frame_len)
{
    int rlen = 0;
    if (encoder == NULL) {
        r_printf("encoder NULL");
    }
    struct esco_enc_hdl *enc = container_of(encoder, struct esco_enc_hdl, encoder);

    if (enc == NULL) {
        r_printf("enc NULL");
    }

    while (1) {
#if (defined(TCFG_PHONE_MESSAGE_ENABLE) && (TCFG_PHONE_MESSAGE_ENABLE))
        rlen = phone_message_output_read(enc->pcm_frame, frame_len);
        if (rlen == frame_len) {
            break;
        } else if (rlen == 0) {
            return 0;
        }
#endif
        rlen = audio_aec_output_read(enc->pcm_frame, frame_len);
        if (rlen == frame_len) {
            /*esco编码读取数据正常*/
            break;
        } else if (rlen == 0) {
            /*esco编码读不到数，返回0*/
            return 0;
            /*esco编码读不到数，pend住*/
            /* int ret = os_sem_pend(&enc->pcm_frame_sem, 100);
            if (ret == OS_TIMEOUT) {
                r_printf("esco_enc pend timeout\n");
                break;
            } */
        } else {
            /*通话结束，aec已经释放*/
            printf("audio_enc end:%d\n", rlen);
            rlen = 0;
            break;
        }
    }

    *frame = enc->pcm_frame;
    return rlen;
}
static void esco_enc_pcm_put(struct audio_encoder *encoder, s16 *frame)
{
}

static const struct audio_enc_input esco_enc_input = {
    .fget = esco_enc_pcm_get,
    .fput = esco_enc_pcm_put,
};

static int esco_enc_probe_handler(struct audio_encoder *encoder)
{
#if (defined(TCFG_PHONE_MESSAGE_ENABLE) && (TCFG_PHONE_MESSAGE_ENABLE))
    phone_message_call_api_start();
#endif
    return 0;
}

static int esco_enc_output_handler(struct audio_encoder *encoder, u8 *frame, int len)
{
    struct esco_enc_hdl *enc = container_of(encoder, struct esco_enc_hdl, encoder);

    if (encoder->fmt.coding_type == AUDIO_CODING_MSBC) {
        u16 head_H2 = esco_sync_word | (esco_seq_num[enc->seqN] << 12);
        if (++enc->seqN > 3) {
            enc->seqN = 0;
        }
        memcpy(frame, &head_H2, sizeof(head_H2));
    }
    lmp_private_send_esco_packet(NULL, frame, len);
    //printf("frame:%x,out:%d\n",frame, len);

    return len;
}

const static struct audio_enc_handler esco_enc_handler = {
    .enc_probe = esco_enc_probe_handler,
    .enc_output = esco_enc_output_handler,
};

static void esco_enc_event_handler(struct audio_encoder *encoder, int argc, int *argv)
{
    printf("esco_enc_event_handler:0x%x,%d\n", argv[0], argv[0]);
    switch (argv[0]) {
    case AUDIO_ENC_EVENT_END:
        puts("AUDIO_ENC_EVENT_END\n");
        break;
    }
}

int esco_enc_open(u32 coding_type, u8 frame_len)
{
    int err;
    struct audio_fmt fmt;

    printf("esco_enc_open: %d,frame_len:%d\n", coding_type, frame_len);

    fmt.channel = 1;
    fmt.frame_len = frame_len;
    if (coding_type == AUDIO_CODING_MSBC) {
        fmt.sample_rate = 16000;
        fmt.coding_type = AUDIO_CODING_MSBC;
    } else if (coding_type == AUDIO_CODING_CVSD) {
        fmt.sample_rate = 8000;
        fmt.coding_type = AUDIO_CODING_CVSD;
    } else {
        /*Unsupoport eSCO Air Mode*/
    }

    if (!encode_task) {
        encode_task = zalloc(sizeof(*encode_task));
        audio_encoder_task_create(encode_task, "audio_enc");
    }
    if (!esco_enc) {
        esco_enc = zalloc(sizeof(*esco_enc));
    }
    //os_sem_create(&esco_enc->pcm_frame_sem, 0);

    audio_encoder_open(&esco_enc->encoder, &esco_enc_input, encode_task);
    audio_encoder_set_handler(&esco_enc->encoder, &esco_enc_handler);
    audio_encoder_set_fmt(&esco_enc->encoder, &fmt);
    audio_encoder_set_event_handler(&esco_enc->encoder, esco_enc_event_handler, 0);
    audio_encoder_set_output_buffs(&esco_enc->encoder, esco_enc->output_frame,
                                   sizeof(esco_enc->output_frame), 1);

    if (!esco_enc->encoder.enc_priv) {
        log_e("encoder err, maybe coding(0x%x) disable \n", fmt.coding_type);
        err = -EINVAL;
        goto __err;
    }

    audio_encoder_start(&esco_enc->encoder);

#if TCFG_AUDIO_ANC_ENABLE
    app_var.aec_mic_gain = anc_mic_gain_get();
#endif

    printf("esco sample_rate: %d,mic_gain:%d\n", fmt.sample_rate, app_var.aec_mic_gain);


#if (TCFG_AUDIO_ADC_MIC_CHA == PLNK_MIC)
    JL_PORTA->DIR &= ~BIT(2);
    JL_PORTA->OUT |= BIT(2);
    //6976B PA2 PA3双绑
    JL_PORTA->DIR |= BIT(3);
    JL_PORTA->PU &= ~BIT(3);
    JL_PORTA->PD &= ~BIT(3);

    audio_plnk_mic = zalloc(sizeof(audio_plnk_t));
    if (audio_plnk_mic) {
        audio_plnk_mic->ch_num = 1;//PLNK_MIC_CH;
        audio_plnk_mic->sr = fmt.sample_rate;
        audio_plnk_mic->output = audio_plnk_mic_output;
        audio_plnk_open(audio_plnk_mic);
    }

#else

    audio_mic_pwr_ctl(MIC_PWR_ON);
    audio_adc_mic_open(&esco_enc->mic_ch, AUDIO_ADC_MIC_CH, &adc_hdl);
#if (ESCO_ADC_CH == 2)
    //audio_adc_mic1_open(&esco_enc->mic_ch, AUDIO_ADC_MIC_CH, &adc_hdl);
    //audio_adc_mic1_set_gain(&esco_enc->mic_ch, app_var.aec_mic_gain);
#endif/*ESCO_ADC_CH*/
    audio_adc_mic_set_sample_rate(&esco_enc->mic_ch, fmt.sample_rate);
    audio_adc_mic_set_gain(&esco_enc->mic_ch, app_var.aec_mic_gain);
    audio_adc_mic_set_buffs(&esco_enc->mic_ch, esco_enc->adc_buf,
                            ESCO_ADC_IRQ_POINTS * 2, ESCO_ADC_BUF_NUM);
    esco_enc->adc_output.handler = adc_mic_output_handler;
    audio_adc_add_output_handler(&adc_hdl, &esco_enc->adc_output);

    audio_adc_mic_start(&esco_enc->mic_ch);


#endif

    return 0;
__err:
    audio_encoder_close(&esco_enc->encoder);

    local_irq_disable();
    free(esco_enc);
    esco_enc = NULL;
    local_irq_enable();

    return err;
}

void esco_enc_close()
{
    y_printf("esco_enc_close\n");
    if (!esco_enc) {
        printf("esco_enc NULL\n");
        return;
    }
#if (TCFG_AUDIO_ADC_MIC_CHA == PLNK_MIC)
    audio_plnk_close();
    free(audio_plnk_mic);
#else
    audio_adc_mic_close(&esco_enc->mic_ch);
#if TCFG_AUDIO_ANC_ENABLE
    //y_printf("anc_status:%d\n",anc_status_get());
    if (anc_status_get() == 0) {
        audio_mic_pwr_ctl(MIC_PWR_OFF);
    }
#else
    audio_mic_pwr_ctl(MIC_PWR_OFF);
#endif
    //os_sem_post(&esco_enc->pcm_frame_sem);
    audio_encoder_close(&esco_enc->encoder);
    audio_adc_del_output_handler(&adc_hdl, &esco_enc->adc_output);
#endif

    local_irq_disable();
    free(esco_enc);
    esco_enc = NULL;
    local_irq_enable();

    if (encode_task) {
        audio_encoder_task_del(encode_task);
        free(encode_task);
        encode_task = NULL;
    }

    y_printf("esco_enc_close ok\n");
}

int esco_enc_get_fmt(struct audio_fmt *pfmt)
{
    if (!esco_enc) {
        return false;
    }
    memcpy(pfmt, &esco_enc->encoder.fmt, sizeof(struct audio_fmt));
    return true;
}

//////////////////////////////////////////////////////////////////////////////
#if (defined(TCFG_PCM_ENC2TWS_ENABLE) && (TCFG_PCM_ENC2TWS_ENABLE))

#define PCM_ENC2TWS_OUTBUF_LEN				(4 * 1024)
struct pcm2tws_enc_hdl {
    struct audio_encoder encoder;
    OS_SEM pcm_frame_sem;
    s16 output_frame[30];               //align 4Bytes
    int pcm_frame[60];                 //align 4Bytes
    u8 output_buf[PCM_ENC2TWS_OUTBUF_LEN];
    cbuffer_t output_cbuf;
    void (*resume)(void);
    u32 status : 3;
    u32 reserved: 29;
};
struct pcm2tws_enc_hdl *pcm2tws_enc = NULL;

extern void local_tws_start(u32 coding_type, u32 rate);
extern void local_tws_stop(void);
extern int local_tws_resolve(u32 coding_type, u32 rate);
extern int local_tws_output(s16 *data, int len);

void pcm2tws_enc_close();
void pcm2tws_enc_resume(void);

int pcm2tws_enc_output(void *priv, s16 *data, int len)
{
    if (!pcm2tws_enc) {
        return 0;
    }
    u16 wlen = cbuf_write(&pcm2tws_enc->output_cbuf, data, len);
    os_sem_post(&pcm2tws_enc->pcm_frame_sem);
    if (!wlen) {
        /* putchar(','); */
    }
    /* printf("wl:%d ", wlen); */
    pcm2tws_enc_resume();
    return wlen;
}

void pcm2tws_enc_set_resume_handler(void (*resume)(void))
{
    if (pcm2tws_enc) {
        pcm2tws_enc->resume = resume;
    }
}

static void pcm2tws_enc_need_data(void)
{
    if (pcm2tws_enc && pcm2tws_enc->resume) {
        pcm2tws_enc->resume();
    }
}

static int pcm2tws_enc_pcm_get(struct audio_encoder *encoder, s16 **frame, u16 frame_len)
{
    int rlen = 0;
    if (encoder == NULL) {
        r_printf("encoder NULL");
    }
    struct pcm2tws_enc_hdl *enc = container_of(encoder, struct pcm2tws_enc_hdl, encoder);

    if (enc == NULL) {
        r_printf("enc NULL");
    }
    os_sem_set(&pcm2tws_enc->pcm_frame_sem, 0);
    /* printf("l:%d", frame_len); */

    do {
        rlen = cbuf_read(&pcm2tws_enc->output_cbuf, enc->pcm_frame, frame_len);
        if (rlen == frame_len) {
            break;
        }
        if (rlen == -EINVAL) {
            return 0;
        }
        if (!pcm2tws_enc->status) {
            return 0;
        }
        pcm2tws_enc_need_data();
        os_sem_pend(&pcm2tws_enc->pcm_frame_sem, 2);
    } while (1);

    *frame = enc->pcm_frame;
    return rlen;
}
static void pcm2tws_enc_pcm_put(struct audio_encoder *encoder, s16 *frame)
{
}

static const struct audio_enc_input pcm2tws_enc_input = {
    .fget = pcm2tws_enc_pcm_get,
    .fput = pcm2tws_enc_pcm_put,
};

static int pcm2tws_enc_probe_handler(struct audio_encoder *encoder)
{
    return 0;
}

static int pcm2tws_enc_output_handler(struct audio_encoder *encoder, u8 *frame, int len)
{
    struct pcm2tws_enc_hdl *enc = container_of(encoder, struct pcm2tws_enc_hdl, encoder);
    local_tws_resolve(encoder->fmt.coding_type, encoder->fmt.sample_rate | (encoder->fmt.channel << 16));
    int ret = local_tws_output(frame, len);
    if (!ret) {
        /* putchar('L'); */
    } else {
        /* printf("w:%d ", len);	 */
    }
    return ret;
}

const static struct audio_enc_handler pcm2tws_enc_handler = {
    .enc_probe = pcm2tws_enc_probe_handler,
    .enc_output = pcm2tws_enc_output_handler,
};



int pcm2tws_enc_open(u32 codec_type, u32 info)
{
    int err;
    struct audio_fmt fmt;
    u16 rate = info & 0x0000ffff;
    u16 channel = (info >> 16) & 0x0f;

    printf("pcm2tws_enc_open: %d\n", codec_type);

    fmt.channel = channel;
    fmt.sample_rate = rate;
    fmt.coding_type = codec_type;

    if (!encode_task) {
        encode_task = zalloc(sizeof(*encode_task));
        audio_encoder_task_create(encode_task, "audio_enc");
    }
    if (pcm2tws_enc) {
        pcm2tws_enc_close();
    }
    pcm2tws_enc = zalloc(sizeof(*pcm2tws_enc));

    os_sem_create(&pcm2tws_enc->pcm_frame_sem, 0);

    cbuf_init(&pcm2tws_enc->output_cbuf, pcm2tws_enc->output_buf, PCM_ENC2TWS_OUTBUF_LEN);

    audio_encoder_open(&pcm2tws_enc->encoder, &pcm2tws_enc_input, encode_task);
    audio_encoder_set_handler(&pcm2tws_enc->encoder, &pcm2tws_enc_handler);
    audio_encoder_set_fmt(&pcm2tws_enc->encoder, &fmt);
    audio_encoder_set_output_buffs(&pcm2tws_enc->encoder, pcm2tws_enc->output_frame,
                                   sizeof(pcm2tws_enc->output_frame), 1);
    if (!pcm2tws_enc->encoder.enc_priv) {
        log_e("encoder err, maybe coding(0x%x) disable \n", fmt.coding_type);
        err = -EINVAL;
        goto __err;
    }


    local_tws_start(pcm2tws_enc->encoder.fmt.coding_type, pcm2tws_enc->encoder.fmt.sample_rate | (pcm2tws_enc->encoder.fmt.channel << 16));

    pcm2tws_enc->status = 1;

    audio_encoder_start(&pcm2tws_enc->encoder);

    printf("sample_rate: %d\n", fmt.sample_rate);

    return 0;
__err:
    audio_encoder_close(&pcm2tws_enc->encoder);

    local_irq_disable();
    free(pcm2tws_enc);
    pcm2tws_enc = NULL;
    local_irq_enable();

    return err;
}

void pcm2tws_enc_close()
{
    if (!pcm2tws_enc) {
        return;
    }
    pcm2tws_enc->status = 0;
    printf("pcm2tws_enc_close");
    local_tws_stop();
    audio_encoder_close(&pcm2tws_enc->encoder);
    free(pcm2tws_enc);
    pcm2tws_enc = NULL;
    if (encode_task) {
        audio_encoder_task_del(encode_task);
        free(encode_task);
        encode_task = NULL;
    }
}

void pcm2tws_enc_resume(void)
{
    if (pcm2tws_enc && pcm2tws_enc->status) {
        audio_encoder_resume(&pcm2tws_enc->encoder);
    }
}

#endif

//////////////////////////////////////////////////////////////////////////////
int audio_enc_init()
{
    printf("audio_enc_init\n");

    audio_adc_init(&adc_hdl, &adc_data);

    return 0;
}

extern struct audio_dac_hdl dac_hdl;
extern struct audio_adc_hdl adc_hdl;

#define ADC_DEMO_BUF_NUM        2
#define ADC_DEMO_IRQ_POINTS     256
#define ADC_DEMO_BUFS_SIZE      (ADC_DEMO_BUF_NUM * ADC_DEMO_IRQ_POINTS)

struct adc_demo_demo {
    u8 idle;
    u8 adc_2_dac;
    struct audio_adc_output_hdl adc_output;
    struct adc_mic_ch mic_ch;
    s16 adc_buf[ADC_DEMO_BUFS_SIZE];    //align 4Bytes
};
struct adc_demo_demo *adc_demo;

static void adc_mic_demo_output(void *priv, s16 *data, int len)
{
    putchar('o');
    if (adc_demo && adc_demo->adc_2_dac) {
        int wlen = audio_dac_write(&dac_hdl, data, len);
    }
    //printf("adc:%x,len:%d",data,len);
    //printf("wlen:%d",wlen);
}

static u8 mic_demo_idle_query()
{
    return 1;
}
REGISTER_LP_TARGET(mic_demo_lp_target) = {
    .name = "mic_demo",
    .is_idle = mic_demo_idle_query,
};

void audio_adc_mic_demo(u8 mic_idx, u8 gain, u8 mic_2_dac)
{
    u16 mic_sr = 16000;
    u8 mic_gain = 10;
    y_printf("audio_adc_mic_demo...");
    adc_demo = zalloc(sizeof(*adc_demo));
    if (adc_demo) {
        audio_adc_mic_open(&adc_demo->mic_ch, AUDIO_ADC_MIC_CH, &adc_hdl);
        audio_adc_mic_set_sample_rate(&adc_demo->mic_ch, mic_sr);
        audio_adc_mic_set_gain(&adc_demo->mic_ch, mic_gain);
        audio_adc_mic_set_buffs(&adc_demo->mic_ch, adc_demo->adc_buf, ADC_DEMO_IRQ_POINTS * 2, ADC_DEMO_BUF_NUM);
        adc_demo->adc_output.handler = adc_mic_demo_output;
        audio_adc_add_output_handler(&adc_hdl, &adc_demo->adc_output);
        audio_adc_mic_start(&adc_demo->mic_ch);

        adc_demo->adc_2_dac = mic_2_dac;
        if (mic_2_dac) {
            printf("max_sys_vol:%d\n", get_max_sys_vol());
            app_audio_state_switch(APP_AUDIO_STATE_MUSIC, get_max_sys_vol());
            printf("cur_vol:%d\n", app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
            audio_dac_set_volume(&dac_hdl, app_audio_get_volume(APP_AUDIO_STATE_MUSIC));
            audio_dac_set_sample_rate(&dac_hdl, mic_sr);
            audio_dac_start(&dac_hdl);
        }
    }
    y_printf("audio_adc_mic_demo start succ\n");
}
void audio_adc_mic_demo_close()
{
    if (adc_demo) {
        audio_adc_del_output_handler(&adc_hdl, &adc_demo->adc_output);
        audio_adc_mic_close(&adc_demo->mic_ch);
        free(adc_demo);
        adc_demo = NULL;
    }
}

void audio_dac_open_test()
{
    audio_dac_start(&dac_hdl);
    JL_AUDIO->DAC_CON |= BIT(6);
    JL_AUDIO->DAC_CON |= BIT(5);
    SFR(JL_ADDA->DAA_CON1,  4,  4,  0);         // RG_SEL_11v[3:0]
    SFR(JL_ADDA->DAA_CON1,  0,  4,  10);         // LG_SEL_11v[3:0]  ///////                    spk gain

}

void audio_adc_mic_trim_open(u8 mic_idx, u8 gain)
{
    u16 mic_sr = 16000;
    g_printf("audio_adc_mic_trim_open:%d\n", mic_idx);
    audio_adc_init(&adc_hdl, &adc_data);
    adc_demo = zalloc(sizeof(*adc_demo));
    if (adc_demo) {
        audio_adc_mic_open(&adc_demo->mic_ch, AUDIO_ADC_MIC_CH, &adc_hdl);
        audio_adc_mic_set_gain(&adc_demo->mic_ch, gain);
        audio_adc_mic_set_sample_rate(&adc_demo->mic_ch, mic_sr);
        audio_adc_mic_set_buffs(&adc_demo->mic_ch, adc_demo->adc_buf, ADC_DEMO_IRQ_POINTS * 2, ADC_DEMO_BUF_NUM);
        adc_demo->adc_output.handler = adc_mic_demo_output;
        audio_adc_add_output_handler(&adc_hdl, &adc_demo->adc_output);
        audio_adc_mic_start(&adc_demo->mic_ch);
        g_printf("audio_adc_mic_trim open succ\n");
    } else {
        r_printf("audio_adc_mic_trim open err\n");
    }
}

void audio_adc_mic_trim_close(u8 mic_idx)
{
    g_printf("audio_adc_mic_trim close:%d\n", mic_idx);
    if (adc_demo) {
        audio_adc_mic_close(&adc_demo->mic_ch);
        audio_adc_del_output_handler(&adc_hdl, &adc_demo->adc_output);
        free(adc_demo);
        adc_demo = NULL;
    }
}

