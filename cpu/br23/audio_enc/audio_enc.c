#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "asm/audio_src.h"
#include "audio_enc.h"
#include "app_main.h"
#include "clock_cfg.h"
#include "classic/hci_lmp.h"
#include "app_config.h"

#ifndef CONFIG_LITE_AUDIO
#include "app_task.h"
#include "aec_user.h"
#include "loud_speaker.h"
#include "mic_effect.h"
#else
__attribute__((weak))void audio_aec_inbuf(s16 *buf, u16 len)
{
}
#endif/*CONFIG_LITE_AUDIO*/

/* #include "encode/encode_write.h" */
extern struct adc_platform_data adc_data;

extern int sbc_encoder_init();
extern int msbc_encoder_init();
extern int cvsd_encoder_init();
extern int mp3_encoder_init();
extern int adpcm_encoder_init();
extern int pcm_encoder_init();
extern int opus_encoder_preinit();
extern int speex_encoder_preinit();

struct audio_adc_hdl adc_hdl;
struct esco_enc_hdl *esco_enc = NULL;
struct audio_encoder_task *encode_task = NULL;

static u16 ladc_irq_point_unit = 0;

#define ESCO_ADC_BUF_NUM        3
#define ESCO_ADC_IRQ_POINTS     256
#define ESCO_ADC_BUFS_SIZE      (ESCO_ADC_BUF_NUM * ESCO_ADC_IRQ_POINTS)

struct esco_enc_hdl {
    struct audio_encoder encoder;
    struct audio_adc_output_hdl adc_output;
    struct adc_mic_ch mic_ch;
    //OS_SEM pcm_frame_sem;
    s16 output_frame[30];               //align 4Bytes
    int pcm_frame[60];                 //align 4Bytes
    /* s16 adc_buf[ESCO_ADC_BUFS_SIZE];    //align 4Bytes */
    u8 state;
};

static void adc_mic_output_handler(void *priv, s16 *data, int len)
{
    /* printf("buf:%x,data:%x,len:%d",esco_enc->adc_buf,data,len); */
    audio_aec_inbuf(data, len);
}
static void adc_mic_output_handler_downsr(void *priv, s16 *data, int len)
{
    //printf("buf:%x,data:%x,len:%d",esco_enc->adc_buf,data,len);
    u16 i;
    s16 temp_buf[128];
    for (i = 0; i < len / 4; i++) {
        temp_buf[i] = data[i * 2];

    }
    audio_aec_inbuf(temp_buf, len / 2);
}

__attribute__((weak)) int audio_aec_output_read(s16 *buf, u16 len)
{
    return 0;
}

void esco_enc_resume(void)
{
    if (esco_enc) {
        //os_sem_post(&esco_enc->pcm_frame_sem);
        audio_encoder_resume(esco_enc);
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
        rlen = audio_aec_output_read(enc->pcm_frame, frame_len);
        if (rlen == frame_len) {
            /*esco编码读取数据正常*/
#if (RECORDER_MIX_EN)
            recorder_mix_sco_data_write(enc->pcm_frame, frame_len);
#endif/*RECORDER_MIX_EN*/
            break;
        } else if (rlen == 0) {
            /*esco编码读不到数,返回0*/
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
    return 0;
}

static int esco_enc_output_handler(struct audio_encoder *encoder, u8 *frame, int len)
{
    lmp_private_send_esco_packet(NULL, frame, len);
    //printf("frame:%x,out:%d\n",frame, len);

    return len;
}

const static struct audio_enc_handler esco_enc_handler = {
    .enc_probe = esco_enc_probe_handler,
    .enc_output = esco_enc_output_handler,
};

static void esco_enc_event_handler(struct audio_decoder *decoder, int argc, int *argv)
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
        clock_add(ENC_MSBC_CLK);
    } else if (coding_type == AUDIO_CODING_CVSD) {
        fmt.sample_rate = 8000;
        fmt.coding_type = AUDIO_CODING_CVSD;
        clock_add(ENC_CVSD_CLK);
    } else {
        /*Unsupoport eSCO Air Mode*/
    }

#if (RECORDER_MIX_EN)
    recorder_mix_pcm_set_info(fmt.sample_rate, fmt.channel);
#endif/*RECORDER_MIX_EN*/

    audio_encoder_task_open();

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

    audio_encoder_start(&esco_enc->encoder);

    printf("esco sample_rate: %d,mic_gain:%d\n", fmt.sample_rate, app_var.aec_mic_gain);

#if 0
    audio_adc_mic_open(&esco_enc->mic_ch, AUDIO_ADC_MIC_CH, &adc_hdl);
    audio_adc_mic_set_sample_rate(&esco_enc->mic_ch, fmt.sample_rate);
    audio_adc_mic_set_gain(&esco_enc->mic_ch, app_var.aec_mic_gain);
    audio_adc_mic_set_buffs(&esco_enc->mic_ch, esco_enc->adc_buf,
                            ESCO_ADC_IRQ_POINTS * 2, ESCO_ADC_BUF_NUM);
    esco_enc->adc_output.handler = adc_mic_output_handler;
    audio_adc_add_output_handler(&adc_hdl, &esco_enc->adc_output);

#if (AUDIO_OUTPUT_WAY != AUDIO_OUTPUT_WAY_FM)
    audio_adc_mic_start(&esco_enc->mic_ch);
#endif
#else
#if	TCFG_MIC_EFFECT_ENABLE
    if (fmt.sample_rate != MIC_EFFECT_SAMPLERATE) {//8K时需把mic数据采样率降低
        esco_enc->adc_output.handler = adc_mic_output_handler_downsr;
    } else {
        esco_enc->adc_output.handler = adc_mic_output_handler;
    }
    audio_mic_open(&esco_enc->mic_ch, MIC_EFFECT_SAMPLERATE, app_var.aec_mic_gain);
#else
    esco_enc->adc_output.handler = adc_mic_output_handler;
    audio_mic_open(&esco_enc->mic_ch, fmt.sample_rate, app_var.aec_mic_gain);
#endif
    audio_mic_add_output(&esco_enc->adc_output);
    audio_mic_start(&esco_enc->mic_ch);
#endif
    clock_set_cur();
    esco_enc->state = 1;

    return 0;
}

int esco_adc_mic_en()
{
    if (esco_enc && esco_enc->state) {
        audio_adc_mic_start(&esco_enc->mic_ch);
        /* audio_mic_start(&esco_enc->mic_ch); */
        return 0;
    }
    return -1;
}

void esco_enc_close()
{
    printf("esco_enc_close\n");
    if (!esco_enc) {
        printf("esco_enc NULL\n");
        return;
    }

    if (esco_enc->encoder.fmt.coding_type == AUDIO_CODING_MSBC) {
        clock_remove(ENC_MSBC_CLK);
    } else if (esco_enc->encoder.fmt.coding_type == AUDIO_CODING_CVSD) {
        clock_remove(ENC_CVSD_CLK);
    }
#if 0
    audio_adc_mic_close(&esco_enc->mic_ch);
    //os_sem_post(&esco_enc->pcm_frame_sem);
    audio_encoder_close(&esco_enc->encoder);
    audio_adc_del_output_handler(&adc_hdl, &esco_enc->adc_output);
#else
    audio_mic_close(&esco_enc->mic_ch, &esco_enc->adc_output);
    audio_encoder_close(&esco_enc->encoder);
#endif

    local_irq_disable();
    free(esco_enc);
    esco_enc = NULL;
    local_irq_enable();

    audio_encoder_task_close();
    clock_set_cur();
}

struct __encoder_task {
    u8 init_ok;
    atomic_t used;
    OS_MUTEX mutex;
};

static struct __encoder_task enc_task = {0};

int audio_encoder_task_open(void)
{
    local_irq_disable();
    if (enc_task.init_ok == 0) {
        atomic_set(&enc_task.used, 0);
        os_mutex_create(&enc_task.mutex);
        enc_task.init_ok = 1;
    }
    local_irq_enable();

    os_mutex_pend(&enc_task.mutex, 0);
    if (!encode_task) {
        encode_task = zalloc(sizeof(*encode_task));
        audio_encoder_task_create(encode_task, "audio_enc");
    }
    atomic_inc_return(&enc_task.used);
    os_mutex_post(&enc_task.mutex);
    return 0;
}

void audio_encoder_task_close(void)
{
    os_mutex_pend(&enc_task.mutex, 0);
    if (encode_task) {
        if (atomic_dec_and_test(&enc_task.used)) {
            audio_encoder_task_del(encode_task);
            //local_irq_disable();
            free(encode_task);
            encode_task = NULL;
            //local_irq_enable();
        }
    }
    os_mutex_post(&enc_task.mutex);
}



//////////////////////////////////////////////////////////////////////////////

int g726_encoder_init();
int audio_enc_init()
{
    printf("audio_enc_init\n");

#if TCFG_ENC_CVSD_ENABLE
    cvsd_encoder_init();
#endif
#if TCFG_ENC_MSBC_ENABLE
    msbc_encoder_init();
#endif
#if TCFG_ENC_MP3_ENABLE
    mp3_encoder_init();
#endif
#if TCFG_ENC_G726_ENABLE
    g726_encoder_init();
#endif
#if TCFG_ENC_ADPCM_ENABLE
    adpcm_encoder_init();
#endif
#if TCFG_ENC_PCM_ENABLE
    pcm_encoder_init();
#endif
#if TCFG_ENC_OPUS_ENABLE
    opus_encoder_preinit();
#endif
#if TCFG_ENC_SPEEX_ENABLE
    speex_encoder_preinit();
#endif
    audio_adc_init(&adc_hdl, &adc_data);
#if TCFG_ENC_SBC_ENABLE
    sbc_encoder_init();
#endif

    init_audio_adc();
    return 0;
}


/**************************mic linein 接口***************************************************/
#define LADC_BUF_NUM        2
#define LADC_CH_NUM         3
#if TCFG_MIC_EFFECT_ENABLE
#if (RECORDER_MIX_EN)
#define LADC_IRQ_POINTS     160
#else
#if (TCFG_MIC_EFFECT_SEL == MIC_EFFECT_REVERB)
#define LADC_IRQ_POINTS    REVERB_LADC_IRQ_POINTS
#else
#define LADC_IRQ_POINTS     80
#endif
#endif/*RECORDER_MIX_EN*/
#else
#define LADC_IRQ_POINTS     256
#endif
/* #define LADC_BUFS_SIZE      (LADC_CH_NUM * LADC_BUF_NUM * LADC_IRQ_POINTS) */


/* #define ENABLE_MIC_LINEIN */
#define LADC_LINE_L_MASK			(BIT(0))
#define LADC_LINE_R_MASK			(BIT(1))
#define LADC_MIC_CH_MASK    	    (BIT(2))

typedef struct  {
    struct audio_adc_hdl *p_adc_hdl;
    struct adc_mic_ch mic_ch;
    struct audio_adc_ch linein_ch;
    atomic_t used;
    /* s16 adc_buf[MIC_ADC_BUFS_SIZE];    //align 4Bytes */
    s16 *adc_buf;
    OS_MUTEX mutex;
    u8 init_flag: 2;
    u8 states: 4;
    u16 sample_rate;
    struct list_head mic_head;
    struct list_head linein_head;
    int mic_gain;
    int linein_gain;
    u8 mic_en: 2;
    u8 ladc_en: 2;
    u8 ladc_ch_mark: 3;// adc 组合BIT(2):MIC BIT(1):LIN1 BIT(0):LIN0
    u8 ladc_ch_num: 3;// 1 2 3
    //s16 temp_buf[LADC_IRQ_POINTS];
    s16 *temp_buf;
    struct audio_adc_output_hdl output;


} audio_adc_t;
static audio_adc_t ladc_var = {.init_flag = 1};


static void audio_adc_output_demo(void *priv, s16 *data, int len)
{
    u16 i;
    struct audio_adc_output_hdl *p;
    /* struct audio_adc_hdl *hdl = priv; */
    if (ladc_var.ladc_ch_num == 1) {
        if (ladc_var.ladc_ch_mark & LADC_MIC_CH_MASK) { //单mic
            if (!list_empty(&ladc_var.mic_head)) {
                list_for_each_entry(p, &ladc_var.mic_head, entry) {
                    p->handler(p->priv, data, len);
                }
            }
        } else { //单linein
            if (!list_empty(&ladc_var.linein_head)) {
                list_for_each_entry(p, &ladc_var.linein_head, entry) {
                    p->handler(p->priv, data, len);
                }
            }
        }
    } else if (ladc_var.ladc_ch_num == 2) {
        if (ladc_var.ladc_ch_mark & LADC_MIC_CH_MASK) { //数据结构：LIN0 MIC0 LIN1 MIC1 LIN2 MIC2
            if (!list_empty(&ladc_var.mic_head)) {
                if (ladc_var.ladc_ch_mark & LADC_LINE_R_MASK) {
                    for (i = 0; i < len / 2; i++) {
                        ladc_var.temp_buf[i] = data[i * 2 + 0];
                    }
                } else {
                    for (i = 0; i < len / 2; i++) {
                        ladc_var.temp_buf[i] = data[i * 2 + 1];
                    }
                }

                list_for_each_entry(p, &ladc_var.mic_head, entry) {
                    p->handler(p->priv, ladc_var.temp_buf, len);
                }
            }
            if (!list_empty(&ladc_var.linein_head)) {
                if (ladc_var.ladc_ch_mark & LADC_LINE_R_MASK) {
                    for (i = 0; i < len / 2; i++) {
                        data[i] = data[i * 2 + 1];
                    }
                } else {
                    for (i = 0; i < len / 2; i++) {
                        data[i] = data[i * 2 + 0];
                    }
                }
                list_for_each_entry(p, &ladc_var.linein_head, entry) {
                    p->handler(p->priv, data, len);
                }
            }
        } else { //两路linein  LINL0 LINR0 LINL1 LINR1
            if (!list_empty(&ladc_var.linein_head)) {
                list_for_each_entry(p, &ladc_var.linein_head, entry) {
                    p->handler(p->priv, data, len);
                }
            }
        }

    } else if (ladc_var.ladc_ch_num == 3) {//数据结构：LINL0 LINR0 MIC0
        if (!list_empty(&ladc_var.mic_head)) {
            for (i = 0; i < len / 2; i++) {
                ladc_var.temp_buf[i] = data[i * 3 + 2];
            }
            list_for_each_entry(p, &ladc_var.mic_head, entry) {
                p->handler(p->priv, ladc_var.temp_buf, len);
            }
        }
        if (!list_empty(&ladc_var.linein_head)) {
            for (i = 1; i < len / 2; i++) {
                data[i * 2] = data[i * 3 + 0];
                data[i * 2 + 1] = data[i * 3 + 1];
            }
            list_for_each_entry(p, &ladc_var.linein_head, entry) {
                p->handler(p->priv, data, len);
            }
        }
    } else {

        return;
    }
}

void init_audio_adc()
{
    if (ladc_var.init_flag) {
        log_i("\n mic init_flag \n\n\n\n");
        ladc_var.init_flag = 0;
        atomic_set(&ladc_var.used, 0);
        os_mutex_create(&ladc_var.mutex);
        INIT_LIST_HEAD(&ladc_var.mic_head);
        INIT_LIST_HEAD(&ladc_var.linein_head);
        /* ladc_var.adc_buf = (s16 *)zalloc(LADC_BUFS_SIZE * 2); */
        ladc_var.states = 0;
    }
}

void audio_adc_set_irq_point_unit(u16 point_unit)
{
    ladc_irq_point_unit = point_unit;
}

//------------------
int audio_mic_open(struct adc_mic_ch *mic, u16 sample_rate, u8 gain)
{
    u16 irq_point_unit = LADC_IRQ_POINTS;
    if (ladc_irq_point_unit != 0) {
        irq_point_unit = ladc_irq_point_unit;
    }

#if	TCFG_AUDIO_ADC_ENABLE
    os_mutex_pend(&ladc_var.mutex, 0);
    if (!ladc_var.adc_buf) {
        log_i("\n mic malloc \n\n\n\n");
        ladc_var.ladc_ch_mark = 0;
        ladc_var.ladc_ch_mark |= LADC_MIC_CH_MASK;
        ladc_var.ladc_ch_num = 1;
#if TCFG_MIC_EFFECT_ENABLE //开混响时启用多路AD
#if (TCFG_LINEIN_ENABLE&&(LINEIN_INPUT_WAY == LINEIN_INPUT_WAY_ADC))
        if (TCFG_LINEIN_LR_CH & (AUDIO_LIN0L_CH | AUDIO_LIN1L_CH | AUDIO_LIN2L_CH)) { //判断Line0L Line1L Line2L 是否有打开
            ladc_var.ladc_ch_mark |= LADC_LINE_L_MASK;
            ladc_var.ladc_ch_num++;
        }
        if (TCFG_LINEIN_LR_CH & (AUDIO_LIN0R_CH | AUDIO_LIN1R_CH | AUDIO_LIN2R_CH)) { //判断Line0R Line1R Line2R 是否有打开
            ladc_var.ladc_ch_mark |= LADC_LINE_R_MASK;
            ladc_var.ladc_ch_num++;
        }
#endif
#endif//TCFG_MIC_EFFECT_ENABLE

        ladc_var.adc_buf = (s16 *)zalloc(ladc_var.ladc_ch_num * irq_point_unit * 2 * LADC_BUF_NUM);
        if (ladc_var.ladc_ch_num > 1) {
            ladc_var.temp_buf = (s16 *)zalloc(irq_point_unit * 2);
        }
        printf("ladc_ch_num[%d],[%x]", ladc_var.ladc_ch_num, ladc_var.ladc_ch_mark);
    }
    if (ladc_var.states == 0) {
        atomic_inc_return(&ladc_var.used);
        audio_adc_mic_open(&ladc_var.mic_ch, AUDIO_ADC_MIC_CH, &adc_hdl);
        audio_adc_mic_set_sample_rate(&ladc_var.mic_ch, sample_rate);
        audio_adc_mic_set_gain(&ladc_var.mic_ch, gain);
        ladc_var.mic_gain = gain;
        if (ladc_var.ladc_ch_mark & (LADC_LINE_R_MASK | LADC_LINE_L_MASK)) {
            audio_adc_linein_open(&ladc_var.linein_ch, TCFG_LINEIN_LR_CH << 2, &adc_hdl);
            audio_adc_linein_set_sample_rate(&ladc_var.linein_ch, sample_rate);
            audio_adc_linein_set_gain(&ladc_var.linein_ch, 0);
        }
        ladc_var.linein_gain = -1;
        if (ladc_var.ladc_ch_num == 1) {
            audio_adc_mic_set_buffs(&ladc_var.mic_ch, ladc_var.adc_buf,  ladc_var.ladc_ch_num * irq_point_unit * 2, LADC_BUF_NUM);
        } else {
            audio_adc_set_buffs(&ladc_var.linein_ch, ladc_var.adc_buf,  ladc_var.ladc_ch_num * irq_point_unit * 2, LADC_BUF_NUM);
            JL_ANA->ADA_CON1 |= BIT(17); //默认先关闭linein通路
            JL_ANA->ADA_CON2 |= BIT(13);

        }

        ladc_var.output.handler = audio_adc_output_demo;
        /* ladc_var->output.priv = &adc_hdl; */
        audio_adc_add_output_handler(&adc_hdl, &ladc_var.output);
        ladc_var.sample_rate = sample_rate;
        ladc_var.states = 1;
    } else {
        if (ladc_var.sample_rate != sample_rate) {
            log_e("err: mic is on,sample_rate not match \n");
            os_mutex_post(&ladc_var.mutex);
            return -1;
        }
        if (ladc_var.mic_gain < 0) {
            audio_adc_mic_set_gain(&ladc_var.mic_ch, gain);
            ladc_var.mic_gain = gain;
        }
        atomic_inc_return(&ladc_var.used);
    }
    mic->hdl = &adc_hdl;
    log_i("mic open success \n");
    os_mutex_post(&ladc_var.mutex);
    return 0;
#else
    return -1;
#endif
}
void audio_mic_add_output(struct audio_adc_output_hdl *output)
{
#if	TCFG_AUDIO_ADC_ENABLE
    os_mutex_pend(&ladc_var.mutex, 0);
    if (ladc_var.states == 1) {
        struct audio_adc_output_hdl *p;
        local_irq_disable();
        list_for_each_entry(p, &ladc_var.mic_head, entry) {
            if (p == &output->entry) {
                goto __exit;
            }
        }
        list_add_tail(&output->entry, &ladc_var.mic_head);
__exit:
        local_irq_enable();
    } else {
        printf("audio mic not open \n");
    }
    os_mutex_post(&ladc_var.mutex);
#endif
}
void audio_mic_start(struct adc_mic_ch *mic)
{
#if	TCFG_AUDIO_ADC_ENABLE
    if (!mic || mic->hdl != &adc_hdl) {
        log_i("\n adc_mic_ch not open 1\n");
        return;
    }
    os_mutex_pend(&ladc_var.mutex, 0);
    if (ladc_var.ladc_ch_num == 1) {
        /* audio_adc_start(NULL, &ladc_var.mic_ch); */
        audio_adc_mic_start(&ladc_var.mic_ch);
    } else {
        audio_adc_start(&ladc_var.linein_ch, &ladc_var.mic_ch);
    }

    os_mutex_post(&ladc_var.mutex);
#endif
}
void audio_mic_close(struct adc_mic_ch *mic, struct audio_adc_output_hdl *output)
{
#if	TCFG_AUDIO_ADC_ENABLE
    if (!mic || mic->hdl != &adc_hdl) {
        log_i("\n adc_mic_ch not open 2\n");
        return;
    }
    os_mutex_pend(&ladc_var.mutex, 0);
    struct audio_adc_output_hdl *p;
    local_irq_disable();
    list_for_each_entry(p, &ladc_var.mic_head, entry) {
        if (p == &output->entry) {
            list_del(&output->entry);
            break;
        }
    }
    local_irq_enable();

    if (atomic_dec_and_test(&ladc_var.used)) {
        log_i("\n audio_adc_mic_close \n");
        if (ladc_var.ladc_ch_num == 1) {
            audio_adc_mic_close(&ladc_var.mic_ch);
        } else {
            audio_adc_close(&ladc_var.linein_ch, &ladc_var.mic_ch);
        }
        audio_adc_del_output_handler(&adc_hdl, &ladc_var.output);
        list_del_init(&ladc_var.mic_head);
        list_del_init(&ladc_var.linein_head);
        free(ladc_var.adc_buf);
        ladc_var.adc_buf = NULL;
        if (ladc_var.temp_buf) {
            free(ladc_var.temp_buf);
        }
        ladc_var.temp_buf = NULL;
        ladc_var.states = 0;
        ladc_var.ladc_ch_mark = 0;
    } else {
        if (list_empty(&ladc_var.mic_head)) {
            audio_adc_mic_set_gain(&ladc_var.mic_ch, 0);
            ladc_var.mic_gain = -1;
        }
    }
    os_mutex_post(&ladc_var.mutex);
#endif
}
void audio_mic_set_gain(u8 gain)
{
#if	TCFG_AUDIO_ADC_ENABLE
    os_mutex_pend(&ladc_var.mutex, 0);
    audio_adc_mic_set_gain(&ladc_var.mic_ch, gain);
    ladc_var.mic_gain = gain;
    os_mutex_post(&ladc_var.mutex);
#endif
}
//------------------
int audio_linein_open(struct audio_adc_ch *linein, u16 sample_rate, int gain)
{
    u16 irq_point_unit = LADC_IRQ_POINTS;
    if (ladc_irq_point_unit != 0) {
        irq_point_unit = ladc_irq_point_unit;
    }

#if (TCFG_LINEIN_ENABLE&&(LINEIN_INPUT_WAY == LINEIN_INPUT_WAY_ADC))
    os_mutex_pend(&ladc_var.mutex, 0);
    if (!ladc_var.adc_buf) {
        log_i("\n ladc malloc \n\n\n\n");
        ladc_var.ladc_ch_mark = 0;
#if	(TCFG_AUDIO_ADC_ENABLE&&TCFG_MIC_EFFECT_ENABLE)//开混响时启用多路AD
        ladc_var.ladc_ch_mark |= LADC_MIC_CH_MASK;
        ladc_var.ladc_ch_num = 1;
#else
        ladc_var.ladc_ch_mark = 0;
        ladc_var.ladc_ch_num = 0;
#endif
        /* if (TCFG_LINEIN_LR_CH & (0x15)) { */
        if (TCFG_LINEIN_LR_CH & (AUDIO_LIN0L_CH | AUDIO_LIN1L_CH | AUDIO_LIN2L_CH)) { //判断Line0L Line1L Line2L 是否有打开
            ladc_var.ladc_ch_mark |= LADC_LINE_L_MASK;
            ladc_var.ladc_ch_num++;
        }
        if (TCFG_LINEIN_LR_CH & (AUDIO_LIN0R_CH | AUDIO_LIN1R_CH | AUDIO_LIN2R_CH)) { //判断Line0R Line1R Line2R 是否有打开
            ladc_var.ladc_ch_mark |= LADC_LINE_R_MASK;
            ladc_var.ladc_ch_num++;
        }
        ladc_var.adc_buf = (s16 *)zalloc(ladc_var.ladc_ch_num * irq_point_unit * 2 * LADC_BUF_NUM);
        if (ladc_var.ladc_ch_mark & LADC_MIC_CH_MASK) { //mic通路有打开
            ladc_var.temp_buf = (s16 *)zalloc(irq_point_unit * 2);
        }
        printf("ladc_ch_num[%d],[%x]", ladc_var.ladc_ch_num, ladc_var.ladc_ch_mark);
    }
    if (ladc_var.states == 0) {
        atomic_inc_return(&ladc_var.used);
#if	(TCFG_AUDIO_ADC_ENABLE&&TCFG_MIC_EFFECT_ENABLE)//开混响时启用多路AD
        audio_adc_mic_open(&ladc_var.mic_ch, AUDIO_ADC_MIC_CH, &adc_hdl);
        audio_adc_mic_set_sample_rate(&ladc_var.mic_ch, sample_rate);
        audio_adc_mic_set_gain(&ladc_var.mic_ch, 0);
#endif
        ladc_var.mic_gain = -1 ;


        audio_adc_linein_open(&ladc_var.linein_ch, TCFG_LINEIN_LR_CH << 2, &adc_hdl);
        audio_adc_linein_set_sample_rate(&ladc_var.linein_ch, sample_rate);
        audio_adc_linein_set_gain(&ladc_var.linein_ch, gain);
        ladc_var.linein_gain = gain;

        audio_adc_set_buffs(&ladc_var.linein_ch, ladc_var.adc_buf,  ladc_var.ladc_ch_num * irq_point_unit * 2, LADC_BUF_NUM);
        ladc_var.output.handler = audio_adc_output_demo;
        ladc_var.output.priv = &adc_hdl;
        audio_adc_add_output_handler(&adc_hdl, &ladc_var.output);
        ladc_var.sample_rate = sample_rate;
        ladc_var.states = 1;
        log_i("sample_rate [%d]\n", sample_rate);
    } else {
        if (ladc_var.sample_rate != sample_rate) {
            log_e("err: linein is on,sample_rate not match \n");
            os_mutex_post(&ladc_var.mutex);
            return -1;
        }
        if (ladc_var.linein_gain < 0) {
            audio_adc_linein_set_gain(&ladc_var.linein_ch, gain);
            ladc_var.linein_gain = gain;
        }
        if (ladc_var.ladc_ch_mark & LADC_LINE_L_MASK) {
            JL_ANA->ADA_CON1 &= ~BIT(17);
        }
        if (ladc_var.ladc_ch_mark & LADC_LINE_R_MASK) {
            JL_ANA->ADA_CON2 &= ~BIT(13);
        }

        atomic_inc_return(&ladc_var.used);
    }
    linein->hdl = &adc_hdl;
    log_i("linein open success \n");

    os_mutex_post(&ladc_var.mutex);
    return 0;
#else
    return -1;
#endif
}
void audio_linein_add_output(struct audio_adc_output_hdl *output)
{
#if (TCFG_LINEIN_ENABLE&&(LINEIN_INPUT_WAY == LINEIN_INPUT_WAY_ADC))
    os_mutex_pend(&ladc_var.mutex, 0);
    if (ladc_var.states == 1) {
        struct audio_adc_output_hdl *p;

        local_irq_disable();
        list_for_each_entry(p, &ladc_var.linein_head, entry) {
            if (p == &output->entry) {
                goto __exit;
            }
        }
        list_add_tail(&output->entry, &ladc_var.linein_head);
__exit:
        local_irq_enable();
    } else {
        printf("audio linein not open \n");
    }
    os_mutex_post(&ladc_var.mutex);
#endif
}
void audio_linein_start(struct audio_adc_ch *linein)
{
#if (TCFG_LINEIN_ENABLE&&(LINEIN_INPUT_WAY == LINEIN_INPUT_WAY_ADC))
    if (!linein || linein->hdl != &adc_hdl) {
        log_i("\n adc_mic_ch not open 1\n");
        return;
    }
    os_mutex_pend(&ladc_var.mutex, 0);
    if (ladc_var.ladc_ch_mark & LADC_MIC_CH_MASK) {
        audio_adc_start(&ladc_var.linein_ch, &ladc_var.mic_ch);
    } else {
        audio_adc_start(&ladc_var.linein_ch, NULL);
    }
    os_mutex_post(&ladc_var.mutex);
#endif
}
void audio_linein_close(struct audio_adc_ch *linein, struct audio_adc_output_hdl *output)
{
#if (TCFG_LINEIN_ENABLE&&(LINEIN_INPUT_WAY == LINEIN_INPUT_WAY_ADC))
    if (!linein || linein->hdl != &adc_hdl) {
        log_i("\n adc_mic_ch not open 2\n");
        return;
    }
    os_mutex_pend(&ladc_var.mutex, 0);
    struct audio_adc_output_hdl *p;
    local_irq_disable();
    list_for_each_entry(p, &ladc_var.linein_head, entry) {
        if (p == &output->entry) {
            list_del(&output->entry);
            break;
        }
    }
    local_irq_enable();
    if (atomic_dec_and_test(&ladc_var.used)) {
        log_i("\n audio_adc_mic_close \n");
        audio_adc_close(&ladc_var.linein_ch, &ladc_var.mic_ch);
        audio_adc_del_output_handler(&adc_hdl, &ladc_var.output);
        list_del_init(&ladc_var.mic_head);
        list_del_init(&ladc_var.linein_head);
        free(ladc_var.adc_buf);
        ladc_var.adc_buf = NULL;
        if (ladc_var.temp_buf) {
            free(ladc_var.temp_buf);
        }
        ladc_var.temp_buf = NULL;
        ladc_var.states = 0;
        ladc_var.ladc_ch_mark = 0;
    } else {
        if (list_empty(&ladc_var.linein_head)) {
            audio_adc_linein_set_gain(&ladc_var.linein_ch, 0);
            JL_ANA->ADA_CON1 |= BIT(17);
            JL_ANA->ADA_CON2 |= BIT(13);
            ladc_var.linein_gain = -1;
        }
    }
    os_mutex_post(&ladc_var.mutex);
#endif
}
void audio_linein_set_gain(int gain)
{
#if (TCFG_LINEIN_ENABLE&&(LINEIN_INPUT_WAY == LINEIN_INPUT_WAY_ADC))
    os_mutex_pend(&ladc_var.mutex, 0);
    audio_adc_linein_set_gain(&ladc_var.linein_ch, gain);
    ladc_var.linein_gain = gain;
    os_mutex_post(&ladc_var.mutex);
#endif
}
u8 get_audio_linein_ch_num(void)
{
    u8 ret = 0;
#if (TCFG_LINEIN_ENABLE&&(LINEIN_INPUT_WAY == LINEIN_INPUT_WAY_ADC))
    os_mutex_pend(&ladc_var.mutex, 0);
    if (ladc_var.states == 1) {
        ret = ladc_var.ladc_ch_num;
        if (ladc_var.ladc_ch_mark & LADC_MIC_CH_MASK) {
            ret--;
        }
    }
    os_mutex_post(&ladc_var.mutex);
#endif
    return ret;
}
/*****************************************************************************/
