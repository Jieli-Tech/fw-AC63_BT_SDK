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
#include "audio_common/audio_iis.h"
#include "Resample_api.h"

#ifndef CONFIG_LITE_AUDIO
#include "aec_user.h"
#else
__attribute__((weak))void audio_aec_inbuf(s16 *buf, u16 len)
{
}
#endif/*CONFIG_LITE_AUDIO*/

/* #include "encode/encode_write.h" */
extern struct adc_platform_data adc_data;

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
    RS_STUCT_API *mic_sw_src_api ;
    u8 *mic_sw_src_buf;
};

static void adc_mic_output_handler(void *priv, s16 *data, int len)
{
    //printf("buf:%x,data:%x,len:%d",esco_enc->adc_buf,data,len);
#if (RECORDER_MIX_EN)
    recorder_mix_sco_data_write(data, len);
#endif/*RECORDER_MIX_EN*/
    audio_aec_inbuf(data, len);
}


#if TCFG_IIS_INPUT_EN
#define IIS_MIC_SRC_DIFF_MAX        (50)
#define IIS_MIC_BUF_SIZE    (2*1024)
cbuffer_t *iis_mic_cbuf = NULL;
static RS_STUCT_API *iis_mic_sw_src_api = NULL;
static u8 *iis_mic_sw_src_buf = NULL;
static s16 iis_mic_sw_src_output[ALNK_BUF_POINTS_NUM];
s32 sw_src_in_sr = 0;
s32 sw_src_in_sr_top = 0;
s32 sw_src_in_sr_botton = 0;

int iis_mic_sw_src_init()
{
    printf("%s !!\n", __func__);
    if (iis_mic_sw_src_api) {
        printf("iis mic sw src is already open !\n");
        return -1;
    }
    iis_mic_sw_src_api = get_rs16_context();
    g_printf("iis_mic_sw_src_api:0x%x\n", iis_mic_sw_src_api);
    ASSERT(iis_mic_sw_src_api);
    u32 iis_mic_sw_src_need_buf = iis_mic_sw_src_api->need_buf();
    g_printf("iis_mic_sw_src_buf:%d\n", iis_mic_sw_src_need_buf);
    iis_mic_sw_src_buf = malloc(iis_mic_sw_src_need_buf);
    ASSERT(iis_mic_sw_src_buf);
    RS_PARA_STRUCT rs_para_obj;
    rs_para_obj.nch = 1;
    rs_para_obj.new_insample = TCFG_IIS_INPUT_SR;
    rs_para_obj.new_outsample = 16000;

    sw_src_in_sr = rs_para_obj.new_insample;
    sw_src_in_sr_top = rs_para_obj.new_insample + IIS_MIC_SRC_DIFF_MAX;
    sw_src_in_sr_botton = rs_para_obj.new_insample - IIS_MIC_SRC_DIFF_MAX;
    printf("sw src,in = %d,out = %d\n", rs_para_obj.new_insample, rs_para_obj.new_outsample);
    iis_mic_sw_src_api->open(iis_mic_sw_src_buf, &rs_para_obj);
    return 0;
}

int iis_mic_sw_src_uninit()
{
    printf("%s !!\n", __func__);
    if (iis_mic_sw_src_api) {
        iis_mic_sw_src_api = NULL;
    }
    if (iis_mic_sw_src_buf) {
        free(iis_mic_sw_src_buf);
        iis_mic_sw_src_buf = NULL;
    }
    return 0;
}

static void iis_mic_output_handler(void *priv, s16 *data, int len)
{
    s16 *outdat = NULL;
    int outlen = 0;
    int wlen = 0;
    int i = 0;

    // dual to mono
    for (i = 0; i < len / 2 / 2; i++) {
        /* data[i] = ((s32)(data[i*2]) + (s32)(data[i*2-1])) / 2; */
        data[i] = data[i * 2];
    }
    len >>= 1;

    if (iis_mic_cbuf) {
        if (iis_mic_sw_src_api && iis_mic_sw_src_buf) {
            if (iis_mic_cbuf->data_len > IIS_MIC_BUF_SIZE * 3 / 4) {
                sw_src_in_sr--;
                if (sw_src_in_sr < sw_src_in_sr_botton) {
                    sw_src_in_sr = sw_src_in_sr_botton;
                }
            } else if (iis_mic_cbuf->data_len < IIS_MIC_BUF_SIZE / 4) {
                sw_src_in_sr++;
                if (sw_src_in_sr > sw_src_in_sr_top) {
                    sw_src_in_sr = sw_src_in_sr_top;
                }
            }

            outlen = iis_mic_sw_src_api->run(iis_mic_sw_src_buf,    \
                                             data,                  \
                                             len >> 1,              \
                                             iis_mic_sw_src_output);
            ASSERT(outlen <= (sizeof(iis_mic_sw_src_output) >> 1));
            outlen = outlen << 1;
            outdat = iis_mic_sw_src_output;
        }
        wlen = cbuf_write(iis_mic_cbuf, outdat, outlen);
        if (wlen != outlen) {
            putchar('w');
        }
        esco_enc_resume();
    }
}

static int iis_mic_output_read(s16 *buf, u16 len)
{
    int rlen = 0;
    if (iis_mic_cbuf) {
        rlen = cbuf_read(iis_mic_cbuf, buf, len);
    }
    return rlen;
}

#endif // TCFG_IIS_INPUT_EN

#if	TCFG_MIC_EFFECT_ENABLE
unsigned int jl_sr_table[] = {
    7616,
    10500,
    11424,
    15232,
    21000,
    22848,
    30464,
    42000,
    45696,
};

unsigned int normal_sr_table[] = {
    8000,
    11025,
    12000,
    16000,
    22050,
    24000,
    32000,
    44100,
    48000,
};
static u8 get_sample_rate_index(u32 sr)
{
    u8 i;
    for (i = 0; i < ARRAY_SIZE(normal_sr_table); i++) {
        if (normal_sr_table[i] == sr) {
            return i;
        }
    }
    return i - 1;
}
int mic_sw_src_init(u16 out_sr)
{
    if (!esco_enc) {
        printf(" mic  is not open !\n");
        return -1;
    }
    esco_enc->mic_sw_src_api = get_rsfast_context();
    esco_enc->mic_sw_src_buf = malloc(esco_enc->mic_sw_src_api->need_buf());

    ASSERT(esco_enc->mic_sw_src_buf);
    RS_PARA_STRUCT rs_para_obj;
    rs_para_obj.nch = 1;
    /* rs_para_obj.new_insample = MIC_EFFECT_SAMPLERATE; */
    /* rs_para_obj.new_outsample = out_sr; */
    rs_para_obj.new_insample = jl_sr_table[get_sample_rate_index(MIC_EFFECT_SAMPLERATE)];
    rs_para_obj.new_outsample = jl_sr_table[get_sample_rate_index(out_sr)];
    esco_enc->mic_sw_src_api->open(esco_enc->mic_sw_src_buf, &rs_para_obj);
    return 0;
}

int mic_sw_src_uninit(void)
{
    if (!esco_enc) {
        return 0;
    }
    if (esco_enc->mic_sw_src_buf) {
        free(esco_enc->mic_sw_src_buf);
        esco_enc->mic_sw_src_buf = NULL;
    }
    return 0;
}

#endif //TCFG_MIC_EFFECT_ENABLE

static void adc_mic_output_handler_downsr(void *priv, s16 *data, int len)
{
    //printf("buf:%x,data:%x,len:%d",esco_enc->adc_buf,data,len);
    u16 i;
    s16 temp_buf[160];
    if (esco_enc && esco_enc->mic_sw_src_buf) {
        int wlen = esco_enc->mic_sw_src_api->run(esco_enc->mic_sw_src_buf, data, len / 2, temp_buf);
        audio_aec_inbuf(temp_buf, wlen << 1);
    }
}

__attribute__((weak)) int audio_aec_output_read(s16 *buf, u16 len)
{
    return 0;
}

void esco_enc_resume(void)
{
    if (esco_enc) {
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

#if TCFG_IIS_INPUT_EN
        rlen = iis_mic_output_read(enc->pcm_frame, frame_len);
#else // TCFG_IIS_INPUT_EN

        rlen = audio_aec_output_read(enc->pcm_frame, frame_len);
#endif // TCFG_IIS_INPUT_EN

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

    if (!esco_enc->encoder.enc_priv) {
        log_e("encoder err, maybe coding(0x%x) disable \n", fmt.coding_type);
        err = -EINVAL;
        goto __err;
    }

    audio_encoder_start(&esco_enc->encoder);

    printf("esco sample_rate: %d,mic_gain:%d\n", fmt.sample_rate, app_var.aec_mic_gain);

#if TCFG_IIS_INPUT_EN
    if (iis_mic_cbuf == NULL) {
        iis_mic_cbuf = zalloc(sizeof(cbuffer_t) + IIS_MIC_BUF_SIZE);
        if (iis_mic_cbuf) {
            cbuf_init(iis_mic_cbuf, iis_mic_cbuf + 1, IIS_MIC_BUF_SIZE);
        } else {
            printf("iis_mic_cbuf zalloc err !!!!!!!!!!!!!!\n");
        }
    }
    iis_mic_sw_src_init();
    audio_iis_input_start(TCFG_IIS_INPUT_PORT, TCFG_IIS_INPUT_DATAPORT_SEL, iis_mic_output_handler);
#else // TCFG_IIS_INPUT_EN
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
        /* esco_enc->mic_sw_src_api = get_rsfast_context(); */
        mic_sw_src_init(fmt.sample_rate);
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

#endif // TCFG_IIS_INPUT_EN

    clock_set_cur();

    return 0;

__err:
    audio_encoder_close(&esco_enc->encoder);

    local_irq_disable();
    free(esco_enc);
    esco_enc = NULL;
    local_irq_enable();

    audio_encoder_task_close();

    return err;
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
#if TCFG_IIS_INPUT_EN
    audio_iis_input_stop(TCFG_IIS_INPUT_PORT, TCFG_IIS_INPUT_DATAPORT_SEL);
    if (iis_mic_cbuf) {
        free(iis_mic_cbuf);
        iis_mic_cbuf = NULL;
    }
    iis_mic_sw_src_uninit();
    audio_encoder_close(&esco_enc->encoder);
#else // TCFG_IIS_INPUT_EN
#if 0
    audio_adc_mic_close(&esco_enc->mic_ch);
    //os_sem_post(&esco_enc->pcm_frame_sem);
    audio_encoder_close(&esco_enc->encoder);
    audio_adc_del_output_handler(&adc_hdl, &esco_enc->adc_output);
#else
    audio_mic_close(&esco_enc->mic_ch, &esco_enc->adc_output);
#if	TCFG_MIC_EFFECT_ENABLE
    mic_sw_src_uninit();
#endif //TCFG_MIC_EFFECT_ENABLE

    audio_encoder_close(&esco_enc->encoder);
#endif
#endif // TCFG_IIS_INPUT_EN

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
#define MIC_ADC_IRQ_POINTS    ((MIC_EFFECT_SAMPLERATE/1000)*4)  //不能随便更改
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
int audio_enc_init()
{
    printf("audio_enc_init\n");

    audio_adc_init(&adc_hdl, &adc_data);
    return 0;
}





