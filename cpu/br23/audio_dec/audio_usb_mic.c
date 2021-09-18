#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "audio_enc.h"
#include "app_main.h"
#include "app_config.h"
#include "audio_splicing.h"
#include "application/audio_echo_reverb.h"
#include "audio_common/audio_iis.h"

/*usb mic的数据是否经过AEC,包括里面的ANS模块*/
#define USB_MIC_AEC_EN		0
/*AEC模块只能处理16k数据，如果经过aec，就需要对输出数据做变采样*/
#define USB_MIC_SRC_ENABLE	1 //同步以及变采样使能

#if USB_MIC_AEC_EN
#include "aec_user.h"
#endif/*USB_MIC_AEC_EN*/

#if TCFG_APP_PC_EN
#include "device/uac_stream.h"

#if USB_MIC_SRC_ENABLE
#ifdef CONFIG_MEDIA_DEVELOP_ENABLE
#include "audio_track.h"
#endif
#include "Resample_api.h"
#endif/*USB_MIC_SRC_ENABLE*/



#define PCM_ENC2USB_OUTBUF_LEN		(4 * 1024)

/* #define USB_MIC_BUF_NUM        3 */
/* #define USB_MIC_CH_NUM         1 */
/* #define USB_MIC_IRQ_POINTS     1//256 */
/* #define USB_MIC_BUFS_SIZE      (USB_MIC_CH_NUM * USB_MIC_BUF_NUM * USB_MIC_IRQ_POINTS) */

#define USB_MIC_STOP  0x00
#define USB_MIC_START 0x01

extern struct audio_adc_hdl adc_hdl;
extern u16 uac_get_mic_vol(const u8 usb_id);
extern int usb_output_sample_rate();
extern int usb_audio_mic_write_base(void *data, u16 len);

#if USB_MIC_SRC_ENABLE
typedef struct {
    u8 start;
    u8 busy;
    u16 in_sample_rate;
    u32 *runbuf;
    s16 output[320 * 3 + 16];
    RS_STUCT_API *ops;
    void *audio_track;
    u8 input_ch;
} usb_mic_sw_src_t;
static usb_mic_sw_src_t *usb_mic_src = NULL;
#endif/*USB_MIC_SRC_ENABLE*/

struct _usb_mic_hdl {
    struct audio_adc_output_hdl adc_output;
    struct adc_mic_ch    mic_ch;
    struct audio_adc_ch linein_ch;
#if SOUNDCARD_ENABLE //4取1
    s16 *temp_buf;
    u32 temp_buf_len;
#endif
    enum enc_source source;

    cbuffer_t output_cbuf;
    u8 *output_buf;//[PCM_ENC2USB_OUTBUF_LEN];
    u8 rec_tx_channels;
    u8 mic_data_ok;/*mic数据等到一定长度再开始发送*/
    u8 status;
    u8 mic_busy;
#if	TCFG_USB_MIC_ECHO_ENABLE
    ECHO_API_STRUCT *p_echo_hdl;
#endif
};

static struct _usb_mic_hdl *usb_mic_hdl = NULL;
#if TCFG_USB_MIC_ECHO_ENABLE
ECHO_PARM_SET usbmic_echo_parm_default = {
    .delay = 200,				//回声的延时时间 0-300ms
    .decayval = 50,				// 0-70%
    .direct_sound_enable = 1,	//直达声使能  0/1
    .filt_enable = 1,			//发散滤波器使能
};
EF_REVERB_FIX_PARM usbmic_echo_fix_parm_default = {
    .wetgain = 2048,			////湿声增益：[0:4096]
    .drygain = 4096,				////干声增益: [0:4096]
    .sr = MIC_EFFECT_SAMPLERATE,		////采样率
    .max_ms = 200,				////所需要的最大延时，影响 need_buf 大小
};
#endif

#if TCFG_IIS_INPUT_EN

#include "audio_link.h"
#include "Resample_api.h"
#define IIS_MIC_SRC_DIFF_MAX        (50)
#define IIS_MIC_BUF_SIZE    (2*1024)

static RS_STUCT_API *iis_mic_sw_src_api = NULL;
static u8 *iis_mic_sw_src_buf = NULL;
static s16 iis_mic_sw_src_output[ALNK_BUF_POINTS_NUM * 3 / 2];
static s32 sw_src_in_sr = 0;
static s32 sw_src_in_sr_top = 0;
static s32 sw_src_in_sr_botton = 0;

static int iis_mic_sw_src_init()
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
    rs_para_obj.new_outsample = MIC_AUDIO_RATE;

    sw_src_in_sr = rs_para_obj.new_insample;
    sw_src_in_sr_top = rs_para_obj.new_insample + IIS_MIC_SRC_DIFF_MAX;
    sw_src_in_sr_botton = rs_para_obj.new_insample - IIS_MIC_SRC_DIFF_MAX;
    printf("sw src,in = %d,out = %d\n", rs_para_obj.new_insample, rs_para_obj.new_outsample);
    iis_mic_sw_src_api->open(iis_mic_sw_src_buf, &rs_para_obj);
    return 0;
}

static int iis_mic_sw_src_uninit()
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


static void iis_mic_output_handler(u8 ch, s16 *data, u32 len)
{
    s16 *outdat = data;
    int outlen = len;
    if (usb_mic_hdl == NULL) {
        return ;
    }
    if (usb_mic_hdl->status == USB_MIC_STOP) {
        return ;
    }
    // dual to mono
    for (int i = 0; i < len / 4; i++) {
        data[i] = data[2 * i];
    }
    len = len >> 1;
    outlen >>= 1;

    if (iis_mic_sw_src_api && iis_mic_sw_src_buf) {
        if (usb_mic_hdl->output_cbuf.data_len > PCM_ENC2USB_OUTBUF_LEN * 3 / 4) {
            sw_src_in_sr++;
            if (sw_src_in_sr > sw_src_in_sr_top) {
                sw_src_in_sr = sw_src_in_sr_top;
            }
            //printf(">>  sw_src_in_sr++ = %d\n",sw_src_in_sr);
        } else if (usb_mic_hdl->output_cbuf.data_len < PCM_ENC2USB_OUTBUF_LEN / 4) {
            sw_src_in_sr--;
            if (sw_src_in_sr < sw_src_in_sr_botton) {
                sw_src_in_sr = sw_src_in_sr_botton;
            }
            //printf(">>  sw_src_in_sr-- = %d\n",sw_src_in_sr);
        }

        iis_mic_sw_src_api->set_sr(iis_mic_sw_src_buf, sw_src_in_sr);

        outlen = iis_mic_sw_src_api->run(iis_mic_sw_src_buf,    \
                                         data,                  \
                                         len >> 1,              \
                                         iis_mic_sw_src_output);
        ASSERT(outlen <= (sizeof(iis_mic_sw_src_output) >> 1));
        outlen = outlen << 1;
        outdat = iis_mic_sw_src_output;
    }

    switch (usb_mic_hdl->source) {
    case ENCODE_SOURCE_MIC:
    case ENCODE_SOURCE_LINE0_LR:
    case ENCODE_SOURCE_LINE1_LR:
    case ENCODE_SOURCE_LINE2_LR: {
#if USB_MIC_AEC_EN
        audio_aec_inbuf(outdat, outlen);
#else
#if	TCFG_USB_MIC_ECHO_ENABLE
        if (usb_mic_hdl->p_echo_hdl) {
            run_echo(usb_mic_hdl->p_echo_hdl, outdat, outdat, outlen);
        }
#endif


        usb_audio_mic_write_base(outdat, outlen);
#endif
    }
    break;
    default:
        break;
    }

}


#endif // TCFG_IIS_INPUT_EN


static int usb_audio_mic_sync(u32 data_size)
{
#if 0
    int change_point = 0;

    if (data_size > __this->rec_top_size) {
        change_point = -1;
    } else if (data_size < __this->rec_bottom_size) {
        change_point = 1;
    }

    if (change_point) {
        struct audio_pcm_src src;
        src.resample = 0;
        src.ratio_i = (1024) * 2;
        src.ratio_o = (1024 + change_point) * 2;
        src.convert = 1;
        dev_ioctl(__this->rec_dev, AUDIOC_PCM_RATE_CTL, (u32)&src);
    }
#endif

    return 0;
}
static int usb_audio_mic_tx_handler(int event, void *data, int len)
{
    if (usb_mic_hdl == NULL) {
        return 0;
    }
    if (usb_mic_hdl->status == USB_MIC_STOP) {
        return 0;
    }

    int i = 0;
    int r_len = 0;
    u8 ch = 0;
    u8 double_read = 0;

    int rlen = 0;

    if (usb_mic_hdl->mic_data_ok == 0) {
        if (usb_mic_hdl->output_cbuf.data_len > (PCM_ENC2USB_OUTBUF_LEN / 4)) {
            usb_mic_hdl->mic_data_ok = 1;
        } else {
            //y_printf("mic_tx NULL\n");
            memset(data, 0, len);
            return 0;
        }
    }

    /* usb_audio_mic_sync(size); */
    if (usb_mic_hdl->rec_tx_channels == 2) {
        rlen = cbuf_get_data_size(&usb_mic_hdl->output_cbuf);
        if (rlen) {
            rlen = rlen > (len / 2) ? (len / 2) : rlen;
            rlen = cbuf_read(&usb_mic_hdl->output_cbuf, data, rlen);
        } else {
            /* printf("uac read err1\n"); */
            usb_mic_hdl->mic_data_ok = 0;
            return 0;
        }

        len = rlen * 2;
        s16 *tx_pcm = (s16 *)data;
        int cnt = len / 2;
        for (cnt = len / 2; cnt >= 2;) {
            tx_pcm[cnt - 1] = tx_pcm[cnt / 2 - 1];
            tx_pcm[cnt - 2] = tx_pcm[cnt / 2 - 1];
            cnt -= 2;
        }
        rlen = len;
    } else {
        rlen = cbuf_get_data_size(&usb_mic_hdl->output_cbuf);
        if (rlen) {
            rlen = rlen > len ? len : rlen;
            rlen = cbuf_read(&usb_mic_hdl->output_cbuf, data, rlen);
        } else {
            /* printf("uac read err2\n"); */
            usb_mic_hdl->mic_data_ok = 0;
            return 0;
        }
    }
    return rlen;
}



int usb_audio_mic_write_base(void *data, u16 len)
{
    int outlen = len;
    s16 *obuf = data;

#if !TCFG_IIS_INPUT_EN
#if USB_MIC_SRC_ENABLE
    if (usb_mic_src && usb_mic_src->start) {
        usb_mic_src->busy = 1;
#ifdef CONFIG_MEDIA_DEVELOP_ENABLE
        u32 sr = usb_output_sample_rate();
        usb_mic_src->ops->set_sr(usb_mic_src->runbuf, sr);
#endif
        outlen = usb_mic_src->ops->run(usb_mic_src->runbuf, data, len >> 1, usb_mic_src->output);
        usb_mic_src->busy = 0;
        ASSERT(outlen <= (sizeof(usb_mic_src->output) >> 1));
        /* printf("16->48k:%d,%d,%d\n",len >> 1,outlen,sizeof(sw_src->output)); */
        obuf = usb_mic_src->output;
        outlen = outlen << 1;
    }

#endif/*USB_MIC_SRC_ENABLE*/
#endif

    int wlen = cbuf_write(&usb_mic_hdl->output_cbuf, obuf, outlen);
    if (wlen != (outlen)) {
        putchar('L');
        //r_printf("usb_mic write full:%d-%d\n", wlen, len);
    }
    return wlen;

}

int usb_audio_mic_write_do(void *data, u16 len)
{
    int wlen = len;
    if (usb_mic_hdl && usb_mic_hdl->status == USB_MIC_START) {
        usb_mic_hdl->mic_busy = 1;

#if SOUNDCARD_ENABLE //4取1
        if (usb_mic_hdl->temp_buf) {
            if (usb_mic_hdl->temp_buf_len < len / 4) {
                free(usb_mic_hdl->temp_buf);
                usb_mic_hdl->temp_buf = NULL;
            }
        }

        if (!usb_mic_hdl->temp_buf) {
            usb_mic_hdl->temp_buf_len = len / 4;
            usb_mic_hdl->temp_buf = (s16 *) zalloc(usb_mic_hdl->temp_buf_len);
        }
        s16 *temp_pcm = (s16 *)data;
        for (int cnt = 0; cnt < len / 8; cnt++) {
            usb_mic_hdl->temp_buf[cnt] = temp_pcm[cnt * 4 + 2];//取RL通道
        }
        len = len / 4;
        /* wlen = cbuf_write(&usb_mic_hdl->output_cbuf, usb_mic_hdl->temp_buf, len); */
        wlen = usb_audio_mic_write_base(usb_mic_hdl->temp_buf, len);
#else
        wlen = usb_audio_mic_write_base(data, len);
#endif //SOUNDCARD_ENABLE

        usb_mic_hdl->mic_busy = 0;
    }
    return wlen;
}



int usb_audio_mic_write(void *data, u16 len)
{
    pcm_dual_to_single(data, data, len);
    int wlen = usb_audio_mic_write_do(data, len / 2);
    return wlen;
}

static void adc_output_to_cbuf(void *priv, s16 *data, int len)
{
    if (usb_mic_hdl == NULL) {
        return ;
    }
    if (usb_mic_hdl->status == USB_MIC_STOP) {
        return ;
    }

    switch (usb_mic_hdl->source) {
    case ENCODE_SOURCE_MIC:
    case ENCODE_SOURCE_LINE0_LR:
    case ENCODE_SOURCE_LINE1_LR:
    case ENCODE_SOURCE_LINE2_LR: {
#if USB_MIC_SRC_ENABLE
#ifdef CONFIG_MEDIA_DEVELOP_ENABLE
        if (usb_mic_src && usb_mic_src->audio_track) {
            audio_local_sample_track_in_period(usb_mic_src->audio_track, (len >> 1) / usb_mic_src->input_ch);
        }
#endif
#endif/*USB_MIC_SRC_ENABLE*/

#if USB_MIC_AEC_EN
        audio_aec_inbuf(data, len);
#else
#if	TCFG_USB_MIC_ECHO_ENABLE
        if (usb_mic_hdl->p_echo_hdl) {
            run_echo(usb_mic_hdl->p_echo_hdl, data, data, len);
        }
#endif
        usb_audio_mic_write_base(data, len);
#endif
    }
    break;
    default:
        break;
    }
}

#if USB_MIC_SRC_ENABLE
static int sw_src_init(u32 in_sr, u32 out_sr)
{
    usb_mic_src = zalloc(sizeof(usb_mic_sw_src_t));
    ASSERT(usb_mic_src);
    /* usb_mic_src->ops = get_rs16_context(); */
    usb_mic_src->ops = get_rsfast_context();
    ASSERT(usb_mic_src->ops);
    u32 need_buf = usb_mic_src->ops->need_buf();
    printf("sw_src need_buf:%d\n", need_buf);
    usb_mic_src->runbuf = zalloc(need_buf);
    ASSERT(usb_mic_src->runbuf);

    RS_PARA_STRUCT rs_para_obj;
    rs_para_obj.nch = 1;
    rs_para_obj.new_insample = in_sr;
    rs_para_obj.new_outsample = out_sr;
    printf("sw src,in = %d,out = %d\n", rs_para_obj.new_insample, rs_para_obj.new_outsample);
    usb_mic_src->ops->open(usb_mic_src->runbuf, &rs_para_obj);

#ifdef CONFIG_MEDIA_DEVELOP_ENABLE
    usb_mic_src->input_ch = rs_para_obj.nch;
    usb_mic_src->in_sample_rate = in_sr;
    usb_mic_src->audio_track = audio_local_sample_track_open(usb_mic_src->input_ch, in_sr, 1000);
#endif

    usb_mic_src->start = 1;
    return 0;
}

static int sw_src_exit(void)
{
    if (usb_mic_src) {
        usb_mic_src->start = 0;
        while (usb_mic_src->busy) {
            putchar('w');
            os_time_dly(2);
        }
#ifdef CONFIG_MEDIA_DEVELOP_ENABLE
        audio_local_sample_track_close(usb_mic_src->audio_track);
        usb_mic_src->audio_track = NULL;
#endif

        local_irq_disable();
        if (usb_mic_src->runbuf) {
            free(usb_mic_src->runbuf);
            usb_mic_src->runbuf = 0;
        }
        free(usb_mic_src);
        usb_mic_src = NULL;
        local_irq_enable();
        printf("sw_src_exit\n");
    }
    printf("sw_src_exit ok\n");
    return 0;
}
#endif/*USB_MIC_SRC_ENABLE*/

static int usb_mic_aec_output(s16 *data, u16 len)
{
    //putchar('k');
    if (usb_mic_hdl == NULL) {
        return len ;
    }
    if (usb_mic_hdl->status == USB_MIC_STOP) {
        return len;
    }

    u16 olen = len;
    s16 *obuf = data;

    usb_audio_mic_write_do(obuf, olen);
    return len;
}

int uac_mic_vol_switch(int vol)
{
    return vol * 14 / 100;
}

int usb_audio_mic_open(void *_info)
{
    printf("usb_audio_mic_open -----------------------------------------------------\n");
    if (usb_mic_hdl) {
        return 0;
    }
    struct _usb_mic_hdl *hdl = NULL;
    hdl = zalloc(sizeof(struct _usb_mic_hdl));
    if (!hdl) {
        return -EFAULT;
    }

    local_irq_disable();
    usb_mic_hdl = hdl;
    local_irq_enable();

    hdl->status = USB_MIC_STOP;
#if 0
    hdl->adc_buf = zalloc(USB_MIC_BUFS_SIZE * 2);
    if (!hdl->adc_buf) {
        printf("hdl->adc_buf NULL\n");
        if (hdl) {
            free(hdl);
            hdl = NULL;
        }
        return -EFAULT;
    }
#endif
    hdl->output_buf = zalloc(PCM_ENC2USB_OUTBUF_LEN);
    if (!hdl->output_buf) {
        printf("hdl->output_buf NULL\n");
        /* if (hdl->adc_buf) { */
        /* free(hdl->adc_buf); */
        /* } */
        if (hdl) {
            free(hdl);
            hdl = NULL;
        }
        return -EFAULT;
    }

    u32 sample_rate = (u32)_info & 0xFFFFFF;
    u32 output_rate = sample_rate;
    hdl->rec_tx_channels = (u32)_info >> 24;
    hdl->source = ENCODE_SOURCE_MIC;
    printf("usb mic sr:%d ch:%d\n", sample_rate, hdl->rec_tx_channels);

#if USB_MIC_AEC_EN
    printf("usb mic sr[aec]:%d\n", sample_rate);
    sample_rate = 16000;
    //audio_aec_init(sample_rate);
    audio_aec_open(sample_rate, ANS_EN, usb_mic_aec_output);
#endif/*USB_MIC_AEC_EN*/

#if !TCFG_IIS_INPUT_EN
#if USB_MIC_SRC_ENABLE
    sw_src_init(sample_rate, output_rate);
#endif/*USB_MIC_SRC_ENABLE*/
#endif

    cbuf_init(&hdl->output_cbuf, hdl->output_buf, PCM_ENC2USB_OUTBUF_LEN);
#if (SOUNDCARD_ENABLE)
    ///从mix output哪里获取usb mic上行数据
#if(TCFG_USB_MIC_DATA_FROM_DAC)
    mic_effect_to_usbmic_onoff(1);
#endif//TCFG_USB_MIC_DATA_FROM_DAC
#else

#if TCFG_MIC_EFFECT_ENABLE
    app_var.usb_mic_gain = mic_effect_get_micgain();
#else
    app_var.usb_mic_gain = uac_mic_vol_switch(uac_get_mic_vol(0));
#endif//TCFG_MIC_EFFECT_ENABLE

#if TCFG_IIS_INPUT_EN
    iis_mic_sw_src_init();
    audio_iis_input_start(TCFG_IIS_INPUT_PORT, TCFG_IIS_INPUT_DATAPORT_SEL, iis_mic_output_handler);
#else //TCFG_IIS_INPUT_EN
#if (TCFG_USB_MIC_DATA_FROM_MICEFFECT)
    mic_effect_to_usbmic_onoff(1);
#else
#if 0
    audio_adc_mic_open(&hdl->mic_ch, AUDIO_ADC_MIC_CH, &adc_hdl);
    audio_adc_mic_set_sample_rate(&hdl->mic_ch, sample_rate);
    audio_adc_mic_set_gain(&hdl->mic_ch, app_var.usb_mic_gain);
    audio_adc_mic_set_buffs(&hdl->mic_ch, hdl->adc_buf, USB_MIC_IRQ_POINTS * 2, USB_MIC_BUF_NUM);
    hdl->adc_output.handler = adc_output_to_cbuf;
    audio_adc_add_output_handler(&adc_hdl, &hdl->adc_output);
    audio_adc_mic_start(&hdl->mic_ch);
#else
    audio_mic_open(&hdl->mic_ch, sample_rate, app_var.usb_mic_gain);
    hdl->adc_output.handler = adc_output_to_cbuf;
    audio_mic_add_output(&hdl->adc_output);
    audio_mic_start(&hdl->mic_ch);
#endif
#endif//TCFG_USB_MIC_DATA_FROM_MICEFFECT
#endif //TCFG_IIS_INPUT_EN
#endif//SOUNDCARD_ENABLE
#if TCFG_USB_MIC_ECHO_ENABLE
    hdl->p_echo_hdl = open_echo(&usbmic_echo_parm_default, &usbmic_echo_fix_parm_default);
#endif
    set_uac_mic_tx_handler(NULL, usb_audio_mic_tx_handler);

    hdl->status = USB_MIC_START;
    hdl->mic_busy = 0;

    /* __this->rec_begin = 0; */
    return 0;
}

u32 usb_mic_is_running()
{
    if (usb_mic_hdl) {
        return SPK_AUDIO_RATE;
    }
    return 0;
}

/*
 *************************************************************
 *
 *	usb mic gain save
 *
 *************************************************************
 */
static int usb_mic_gain_save_timer;
static u8  usb_mic_gain_save_cnt;
static void usb_audio_mic_gain_save_do(void *priv)
{
    //printf(" usb_audio_mic_gain_save_do %d\n", usb_mic_gain_save_cnt);
    local_irq_disable();
    if (++usb_mic_gain_save_cnt >= 5) {
        sys_hi_timer_del(usb_mic_gain_save_timer);
        usb_mic_gain_save_timer = 0;
        usb_mic_gain_save_cnt = 0;
        local_irq_enable();
        printf("USB_GAIN_SAVE\n");
        syscfg_write(VM_USB_MIC_GAIN, &app_var.usb_mic_gain, 1);
        return;
    }
    local_irq_enable();
}

static void usb_audio_mic_gain_change(u8 gain)
{
    local_irq_disable();
    app_var.usb_mic_gain = gain;
    usb_mic_gain_save_cnt = 0;
    if (usb_mic_gain_save_timer == 0) {
        usb_mic_gain_save_timer = sys_hi_timer_add(NULL, usb_audio_mic_gain_save_do, 1000);
    }
    local_irq_enable();
}

int usb_audio_mic_get_gain(void)
{
    return app_var.usb_mic_gain;
}

void usb_audio_mic_set_gain(int gain)
{
#if (SOUNDCARD_ENABLE)
    return ;
#endif//SOUNDCARD_ENABLE

    if (usb_mic_hdl == NULL) {
        r_printf("usb_mic_hdl NULL gain");
        return;
    }
    gain = uac_mic_vol_switch(gain);
    audio_adc_mic_set_gain(&usb_mic_hdl->mic_ch, gain);
    usb_audio_mic_gain_change(gain);
}
int usb_audio_mic_close(void *arg)
{
    if (usb_mic_hdl == NULL) {
        r_printf("usb_mic_hdl NULL close");
        return 0;
    }
    printf("usb_mic_hdl->status %x\n", usb_mic_hdl->status);
    if (usb_mic_hdl && usb_mic_hdl->status == USB_MIC_START) {
        printf("usb_audio_mic_close in\n");
        usb_mic_hdl->status = USB_MIC_STOP;
#if USB_MIC_AEC_EN
        audio_aec_close();
#endif/*USB_MIC_AEC_EN*/

#if !TCFG_IIS_INPUT_EN
#if USB_MIC_SRC_ENABLE
        sw_src_exit();
#endif/*USB_MIC_SRC_ENABLE*/
#endif

#if (SOUNDCARD_ENABLE)
        //从声卡输出端获取， 没有打开mic， 所以这里不需要关
#if (TCFG_USB_MIC_DATA_FROM_DAC)
        mic_effect_to_usbmic_onoff(0);
#endif//TCFG_USB_MIC_DATA_FROM_DAC
#else
#if (TCFG_USB_MIC_DATA_FROM_MICEFFECT)
        mic_effect_to_usbmic_onoff(0);
#else
#if TCFG_IIS_INPUT_EN
        audio_iis_input_stop(TCFG_IIS_INPUT_PORT, TCFG_IIS_INPUT_DATAPORT_SEL);
        iis_mic_sw_src_uninit();
#else // TCFG_IIS_INPUT_EN
#if 0
        audio_adc_mic_close(&usb_mic_hdl->mic_ch);
        audio_adc_del_output_handler(&adc_hdl, &usb_mic_hdl->adc_output);
#else
        audio_mic_close(&usb_mic_hdl->mic_ch, &usb_mic_hdl->adc_output);
#endif
#endif // TCFG_IIS_INPUT_EN
#endif//TCFG_USB_MIC_DATA_FROM_MICEFFECT
#endif//SOUNDCARD_ENABLE

#if TCFG_USB_MIC_ECHO_ENABLE
        if (usb_mic_hdl->p_echo_hdl) {
            close_echo(usb_mic_hdl->p_echo_hdl);
        }
#endif
        cbuf_clear(&usb_mic_hdl->output_cbuf);
        if (usb_mic_hdl) {
            while (usb_mic_hdl->mic_busy) {
                os_time_dly(3);
            }

            local_irq_disable();

#if SOUNDCARD_ENABLE //4取1
            if (usb_mic_hdl->temp_buf) {
                free(usb_mic_hdl->temp_buf);
                usb_mic_hdl->temp_buf = NULL;
            }
            usb_mic_hdl->temp_buf_len = 0;
#endif

            if (usb_mic_hdl->output_buf) {
                free(usb_mic_hdl->output_buf);
                usb_mic_hdl->output_buf = NULL;
            }
            free(usb_mic_hdl);
            usb_mic_hdl = NULL;
            local_irq_enable();
        }
    }
    printf("usb_audio_mic_close out\n");

    return 0;
}

int usb_mic_stream_sample_rate(void)
{
#if USB_MIC_SRC_ENABLE
#ifdef CONFIG_MEDIA_DEVELOP_ENABLE
    if (usb_mic_src && usb_mic_src->audio_track) {
        int sr = audio_local_sample_track_rate(usb_mic_src->audio_track);
        if ((sr < (usb_mic_src->in_sample_rate + 500)) && (sr > (usb_mic_src->in_sample_rate - 500))) {
            return sr;
        }
        /* printf("uac audio_track reset \n"); */
        local_irq_disable();
        audio_local_sample_track_close(usb_mic_src->audio_track);
        usb_mic_src->audio_track = audio_local_sample_track_open(SPK_CHANNEL, usb_mic_src->in_sample_rate, 1000);
        local_irq_enable();
        return usb_mic_src->in_sample_rate;
    }
#endif
#endif /*USB_MIC_SRC_ENABLE*/

    return 0;
}

u32 usb_mic_stream_size()
{
    if (!usb_mic_hdl) {
        return 0;
    }
    if (usb_mic_hdl->status == USB_MIC_START) {
        if (usb_mic_hdl) {
            return cbuf_get_data_size(&usb_mic_hdl->output_cbuf);
        }
    }

    return 0;
}

u32 usb_mic_stream_length()
{
    return PCM_ENC2USB_OUTBUF_LEN;
}

int usb_output_sample_rate()
{
    int sample_rate = usb_mic_stream_sample_rate();
    int buf_size = usb_mic_stream_size();
    /* printf("sample_rate %d\n", sample_rate); */
    /* printf("buf_size %d %d\n", buf_size, usb_mic_stream_length()); */

    if (buf_size >= (usb_mic_stream_length() * 3 / 4)) {
        sample_rate += (sample_rate * 5 / 10000);
        /* putchar('+'); */
    }
    if (buf_size <= (usb_mic_stream_length() / 4)) {
        sample_rate -= (sample_rate * 5 / 10000);
        /* putchar('-'); */
    }
    return sample_rate;
}

#endif
