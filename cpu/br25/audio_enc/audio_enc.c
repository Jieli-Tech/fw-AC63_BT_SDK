#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "asm/audio_src.h"
#include "audio_enc.h"
#include "app_main.h"
#include "app_action.h"
#include "clock_cfg.h"
#include "classic/hci_lmp.h"
#include "app_config.h"

#ifndef CONFIG_LITE_AUDIO
#include "aec_user.h"
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
extern int opus_encoder_preinit();
extern int speex_encoder_preinit();

struct audio_adc_hdl adc_hdl;
struct esco_enc_hdl *esco_enc = NULL;
struct audio_encoder_task *encode_task = NULL;

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
    s16 adc_buf[ESCO_ADC_BUFS_SIZE];    //align 4Bytes
};

static void adc_mic_output_handler(void *priv, s16 *data, int len)
{
    //printf("buf:%x,data:%x,len:%d",esco_enc->adc_buf,data,len);
#if (RECORDER_MIX_EN)
    recorder_mix_sco_data_write(data, len);
#endif/*RECORDER_MIX_EN*/
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
            /* #if (RECORDER_MIX_EN) */
            /* recorder_mix_sco_data_write(enc->pcm_frame, frame_len); */
            /* #endif[>RECORDER_MIX_EN<] */
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

    /* if (!encode_task) { */
    /* encode_task = zalloc(sizeof(*encode_task)); */
    /* audio_encoder_task_create(encode_task, "audio_enc"); */
    /* } */

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

    audio_adc_mic_start(&esco_enc->mic_ch);
#else

#if	TCFG_MIC_EFFECT_ENABLE
    if (fmt.sample_rate != MIC_EFFECT_SAMPLERATE) {
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

    return 0;
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
    /* if (encode_task) { */
    /* audio_encoder_task_del(encode_task); */
    /* local_irq_disable(); */
    /* free(encode_task); */
    /* encode_task = NULL; */
    /* local_irq_enable(); */
    /* } */
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



/**************************mic ladc 接口***************************************************/
#define MIC_ADC_BUF_NUM        2
#if TCFG_MIC_EFFECT_ENABLE
#define MIC_ADC_IRQ_POINTS     64 //不能随便更改
#else
#define MIC_ADC_IRQ_POINTS     256
#endif
#define MIC_ADC_BUFS_SIZE      (MIC_ADC_BUF_NUM * MIC_ADC_IRQ_POINTS)

typedef struct  {
    struct audio_adc_hdl *p_adc_hdl;
    struct adc_mic_ch mic_ch;
    atomic_t used;
    /* s16 adc_buf[MIC_ADC_BUFS_SIZE];    //align 4Bytes */
    s16 *adc_buf;
    OS_MUTEX mutex;
    u8 init_flag: 2;
    u8 states: 4;
    u16 sample_rate;
} audio_mic_t;
static audio_mic_t mic_var = {.init_flag = 1};

int audio_mic_open(struct adc_mic_ch *mic, u16 sample_rate, u8 gain)
{

    if (mic_var.init_flag) {
        log_i("\n mic init_flag \n\n\n\n");
        mic_var.init_flag = 0;
        atomic_set(&mic_var.used, 0);
        os_mutex_create(&mic_var.mutex);
        mic_var.adc_buf = (s16 *)zalloc(MIC_ADC_BUFS_SIZE * 2);
        mic_var.states = 0;
    }

    os_mutex_pend(&mic_var.mutex, 0);
    if (!mic_var.adc_buf) {
        log_i("\n mic malloc \n\n\n\n");
        mic_var.adc_buf = (s16 *)zalloc(MIC_ADC_BUFS_SIZE * 2);
    }
    if (mic_var.states == 0) {
        atomic_inc_return(&mic_var.used);
        audio_adc_mic_open(mic, AUDIO_ADC_MIC_CH, &adc_hdl);
        audio_adc_mic_set_sample_rate(mic, sample_rate);
        audio_adc_mic_set_gain(mic, gain);
        audio_adc_mic_set_buffs(mic, mic_var.adc_buf,
                                MIC_ADC_IRQ_POINTS * 2, MIC_ADC_BUF_NUM);
        mic_var.sample_rate = sample_rate;
        mic_var.states = 1;
    } else {
        if (mic_var.sample_rate != sample_rate) {
            log_e("err: mic is on,sample_rate not match \n");
            os_mutex_post(&mic_var.mutex);
            return -1;
        }
        atomic_inc_return(&mic_var.used);
        mic->hdl = &adc_hdl;
    }
    log_i("mic open success \n");
    os_mutex_post(&mic_var.mutex);
    return 0;
}
void audio_mic_add_output(struct audio_adc_output_hdl *output)
{
    audio_adc_add_output_handler(&adc_hdl, output);
}
void audio_mic_start(struct adc_mic_ch *mic)
{
    if (!mic || mic->hdl != &adc_hdl) {
        log_i("\n adc_mic_ch not open \n");
        return;
    }

    os_mutex_pend(&mic_var.mutex, 0);
    if (adc_hdl.state != LADC_STATE_START) {
        audio_adc_mic_start(mic);
    }
    os_mutex_post(&mic_var.mutex);
}
void audio_mic_close(struct adc_mic_ch *mic, struct audio_adc_output_hdl *output)
{
    audio_adc_del_output_handler(&adc_hdl, output);
    if (!mic || mic->hdl != &adc_hdl) {
        log_i("\n adc_mic_ch not open \n");
        return;
    }
    os_mutex_pend(&mic_var.mutex, 0);
    if (atomic_dec_and_test(&mic_var.used)) {
        log_i("\n audio_adc_mic_close \n");
        audio_adc_mic_close(mic);
        free(mic_var.adc_buf);
        mic_var.adc_buf = NULL;
        /* mic_var.init_flag =1; */
        mic_var.states = 0;
    }
    os_mutex_post(&mic_var.mutex);
}

void audio_mic_set_gain(struct adc_mic_ch *mic, u8 gain)
{
    audio_adc_mic_set_gain(mic, gain);
}

/*****************************************************************************/

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
#if TCFG_ENC_ADPCM_ENABLE
    adpcm_encoder_init();
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
    return 0;
}





