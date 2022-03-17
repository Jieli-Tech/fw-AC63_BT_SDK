#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "device/uac_stream.h"
#include "audio_enc.h"
#include "app_main.h"
#include "user_cfg_id.h"
#include "board_config.h"
/* #include "application/audio_echo_reverb.h" */
/*usb mic的数据是否经过AEC,包括里面的ANS模块*/
#define USB_MIC_AEC_EN				0
/*AEC模块只能处理16k数据，如果经过aec，就需要对输出数据做变采样*/
#define USB_MIC_SRC_ENABLE	1 //同步以及变采样使能


#if USB_MIC_AEC_EN
#include "aec_user.h"
#endif/*USB_MIC_AEC_EN*/

#if USB_MIC_SRC_ENABLE
/* #ifdef CONFIG_MEDIA_DEVELOP_ENABLE */
#include "audio_track.h"
/* #endif */
#include "Resample_api.h"
#endif/*USB_MIC_SRC_ENABLE*/

#if TCFG_PC_ENABLE

#define PCM_ENC2USB_OUTBUF_LEN		(5 * 1024)

#define USB_MIC_BUF_NUM        3
#define USB_MIC_CH_NUM         1
#define USB_MIC_IRQ_POINTS     256
#define USB_MIC_BUFS_SIZE      (USB_MIC_CH_NUM * USB_MIC_BUF_NUM * USB_MIC_IRQ_POINTS)

#define USB_MIC_STOP  0x00
#define USB_MIC_START 0x01

#ifndef VM_USB_MIC_GAIN
#define     VM_USB_MIC_GAIN             	 5
#endif

extern struct audio_adc_hdl adc_hdl;
extern u16 uac_get_mic_vol(const u8 usb_id);
extern int usb_output_sample_rate();
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
    s16 adc_buf[USB_MIC_BUFS_SIZE];
    enum enc_source source;

    cbuffer_t output_cbuf;
    u8 *output_buf;//[PCM_ENC2USB_OUTBUF_LEN];
    u8 rec_tx_channels;
    u8 mic_data_ok;/*mic数据等到一定长度再开始发送*/
    u8 status;
    u8 drop_data; //用来丢掉刚开mic的前几帧数据
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

    /* int rlen = 0; */

    if (usb_mic_hdl->mic_data_ok == 0) {
        if (usb_mic_hdl->output_cbuf.data_len > (PCM_ENC2USB_OUTBUF_LEN / 4)) {
            usb_mic_hdl->mic_data_ok = 1;
        }
        //y_printf("mic_tx NULL\n");
        memset(data, 0, len);
        return len;
    }

    /* usb_audio_mic_sync(size); */
    if (usb_mic_hdl->rec_tx_channels == 2) {
        len = cbuf_read(&usb_mic_hdl->output_cbuf, data, len / 2);
        s16 *tx_pcm = (s16 *)data;
        int cnt = len / 2;
        for (cnt = len / 2; cnt >= 2;) {
            tx_pcm[cnt - 1] = tx_pcm[cnt / 2 - 1];
            tx_pcm[cnt - 2] = tx_pcm[cnt / 2 - 1];
            cnt -= 2;
        }
        len *= 2;
    } else {
        len = cbuf_read(&usb_mic_hdl->output_cbuf, data, len);
    }
    return len;
}


int usb_audio_mic_write(void *data, u16 len)
{
    int wlen = len;
    if (usb_mic_hdl) {
        if (usb_mic_hdl->status == USB_MIC_STOP) {
            return len;
        }

        int outlen = len;
        s16 *obuf = data;

#if USB_MIC_SRC_ENABLE
        if (usb_mic_src && usb_mic_src->start) {
            usb_mic_src->busy = 1;
            /* #ifdef CONFIG_MEDIA_DEVELOP_ENABLE */
            u32 sr = usb_output_sample_rate();
            usb_mic_src->ops->set_sr(usb_mic_src->runbuf, sr);
            /* #endif */
            outlen = usb_mic_src->ops->run(usb_mic_src->runbuf, data, len >> 1, usb_mic_src->output);
            usb_mic_src->busy = 0;
            ASSERT(outlen <= (sizeof(usb_mic_src->output) >> 1));
            //printf("16->48k:%d,%d,%d\n",len >> 1,outlen,sizeof(usb_mic_src->output));
            obuf = usb_mic_src->output;
            outlen = outlen << 1;
        }
#endif/*USB_MIC_SRC_ENABLE*/
        wlen = cbuf_write(&usb_mic_hdl->output_cbuf, obuf, outlen);
#if 0
        static u32 usb_mic_data_max = 0;
        if (usb_mic_data_max < usb_mic_hdl->output_cbuf.data_len) {
            usb_mic_data_max = usb_mic_hdl->output_cbuf.data_len;
            y_printf("usb_mic_max:%d", usb_mic_data_max);
        }
#endif
        if (wlen != outlen) {
            r_printf("usb_mic write full:%d-%d\n", wlen, outlen);
        }
    }
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
        /* #ifdef CONFIG_MEDIA_DEVELOP_ENABLE */
        if (usb_mic_src && usb_mic_src->audio_track) {
            audio_local_sample_track_in_period(usb_mic_src->audio_track, (len >> 1) / usb_mic_src->input_ch);
        }
        /* #endif */
#endif/*USB_MIC_SRC_ENABLE*/

#if USB_MIC_AEC_EN
        audio_aec_inbuf(data, len);
#else
        if (usb_mic_hdl->drop_data != 0) {
            usb_mic_hdl-> drop_data--;
            memset(data, 0, len);
        }
#if	TCFG_USB_MIC_ECHO_ENABLE
        if (usb_mic_hdl->p_echo_hdl) {
            run_echo(usb_mic_hdl->p_echo_hdl, data, data, len);
        }
#endif

        usb_audio_mic_write(data, len);
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
    g_printf("ops:0x%x\n", usb_mic_src->ops);
    ASSERT(usb_mic_src->ops);
    u32 need_buf = usb_mic_src->ops->need_buf();
    g_printf("runbuf:%d\n", need_buf);
    usb_mic_src->runbuf = malloc(need_buf);
    ASSERT(usb_mic_src->runbuf);
    RS_PARA_STRUCT rs_para_obj;
    rs_para_obj.nch = 1;

    rs_para_obj.new_insample = in_sr;
    rs_para_obj.new_outsample = out_sr;
    printf("sw src,in = %d,out = %d\n", rs_para_obj.new_insample, rs_para_obj.new_outsample);
    usb_mic_src->ops->open(usb_mic_src->runbuf, &rs_para_obj);

    /* #ifdef CONFIG_MEDIA_DEVELOP_ENABLE */
    usb_mic_src->input_ch = rs_para_obj.nch;
    usb_mic_src->in_sample_rate = in_sr;
    usb_mic_src->audio_track = audio_local_sample_track_open(usb_mic_src->input_ch, in_sr, 1000);
    /* #endif */

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

        /* #ifdef CONFIG_MEDIA_DEVELOP_ENABLE */
        audio_local_sample_track_close(usb_mic_src->audio_track);
        usb_mic_src->audio_track = NULL;
        /* #endif */

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



#if USB_MIC_AEC_EN
static int usb_mic_aec_output(s16 *data, u16 len)
{
    //putchar('k');
    if (usb_mic_hdl == NULL) {
        return len ;
    }
    if (usb_mic_hdl->status == USB_MIC_STOP) {
        return len;
    }

    int outlen = len;
    s16 *outdat = data;
    /* if (aec_hdl->dump_packet) { */
    /*     aec_hdl->dump_packet--; */
    /*     //memset(outdat, 0, outlen); */
    /* } */
    u16 wlen = usb_audio_mic_write(outdat, outlen);
    return len;

}
#endif

int uac_mic_vol_switch(int vol)
{
    return vol * 14 / 100;
}

int usb_audio_mic_open(void *_info)
{
    if (usb_mic_hdl) {
        return 0;
    }
    usb_mic_hdl = zalloc(sizeof(*usb_mic_hdl));
    if (!usb_mic_hdl) {
        return -EFAULT;
    }
    usb_mic_hdl->status = USB_MIC_STOP;

    usb_mic_hdl->output_buf = zalloc(PCM_ENC2USB_OUTBUF_LEN);
    if (!usb_mic_hdl->output_buf) {
        printf("usb_mic_hdl->output_buf NULL\n");

        if (usb_mic_hdl) {
            free(usb_mic_hdl);
            usb_mic_hdl = NULL;
        }
        return -EFAULT;
    }

    u32 sample_rate = (u32)_info & 0xFFFFFF;
    usb_mic_hdl->rec_tx_channels = (u32)_info >> 24;
    usb_mic_hdl->source = ENCODE_SOURCE_MIC;
    printf("usb mic sr:%d ch:%d\n", sample_rate, usb_mic_hdl->rec_tx_channels);


    usb_mic_hdl->drop_data = 2; //用来丢掉前几帧数据
    u32 out_sample_rate = sample_rate;
#if USB_MIC_AEC_EN
    sample_rate = 16000;
    printf("usb mic sr[aec]:%d\n", sample_rate);
    //audio_aec_init(sample_rate);
    audio_aec_open(sample_rate, ANS_EN, usb_mic_aec_output);
#endif

#if USB_MIC_SRC_ENABLE
    sw_src_init(sample_rate, out_sample_rate);
#endif /*USB_MIC_SRC_ENABLE*/


    cbuf_init(&usb_mic_hdl->output_cbuf, usb_mic_hdl->output_buf, PCM_ENC2USB_OUTBUF_LEN);

#if TCFG_MIC_EFFECT_ENABLE
    app_var.usb_mic_gain = mic_effect_get_micgain();
#else
    app_var.usb_mic_gain = uac_mic_vol_switch(uac_get_mic_vol(0));
#endif//TCFG_MIC_EFFECT_ENABLE

#if (TCFG_USB_MIC_DATA_FROM_MICEFFECT)
    mic_effect_to_usbmic_onoff(1);
#else
#if 1
    audio_adc_mic_open(&usb_mic_hdl->mic_ch, AUDIO_ADC_MIC_CH, &adc_hdl);
    audio_adc_mic_set_sample_rate(&usb_mic_hdl->mic_ch, sample_rate);
    audio_adc_mic_set_gain(&usb_mic_hdl->mic_ch, app_var.usb_mic_gain);
    audio_adc_mic_set_buffs(&usb_mic_hdl->mic_ch, usb_mic_hdl->adc_buf, USB_MIC_IRQ_POINTS * 2, USB_MIC_BUF_NUM);
    usb_mic_hdl->adc_output.handler = adc_output_to_cbuf;
    audio_adc_add_output_handler(&adc_hdl, &usb_mic_hdl->adc_output);
    audio_adc_mic_start(&usb_mic_hdl->mic_ch);
#else
    audio_mic_open(&usb_mic_hdl->mic_ch, sample_rate, app_var.usb_mic_gain);
    usb_mic_hdl->adc_output.handler = adc_output_to_cbuf;
    audio_mic_add_output(&usb_mic_hdl->adc_output);
    audio_mic_start(&usb_mic_hdl->mic_ch);
#endif
#if TCFG_USB_MIC_ECHO_ENABLE
    usb_mic_hdl->p_echo_hdl = open_echo(&usbmic_echo_parm_default, &usbmic_echo_fix_parm_default);
    if (usb_mic_hdl->p_echo_hdl) {
        update_echo_gain(usb_mic_hdl->p_echo_hdl, usbmic_echo_fix_parm_default.wetgain, usbmic_echo_fix_parm_default.drygain);
    }
#endif
#endif//TCFG_USB_MIC_DATA_FROM_MICEFFECT
    set_uac_mic_tx_handler(NULL, usb_audio_mic_tx_handler);

    usb_mic_hdl->status = USB_MIC_START;
    /* __this->rec_begin = 0; */
    return 0;

    return -EFAULT;
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
#endif
#if (TCFG_USB_MIC_DATA_FROM_MICEFFECT)
        mic_effect_to_usbmic_onoff(0);
#else
#if 1
        audio_adc_mic_close(&usb_mic_hdl->mic_ch);
        audio_adc_del_output_handler(&adc_hdl, &usb_mic_hdl->adc_output);
#else
        audio_mic_close(&usb_mic_hdl->mic_ch, &usb_mic_hdl->adc_output);
#endif
#if TCFG_USB_MIC_ECHO_ENABLE
        if (usb_mic_hdl->p_echo_hdl) {
            close_echo(usb_mic_hdl->p_echo_hdl);
        }
#endif
#endif

#if USB_MIC_SRC_ENABLE
        sw_src_exit();
#endif /*USB_MIC_SRC_ENABLE*/
        cbuf_clear(&usb_mic_hdl->output_cbuf);
        if (usb_mic_hdl) {

            if (usb_mic_hdl->output_buf) {
                free(usb_mic_hdl->output_buf);
                usb_mic_hdl->output_buf = NULL;
            }
            free(usb_mic_hdl);
            usb_mic_hdl = NULL;
        }
    }
    printf("usb_audio_mic_close out\n");

    return 0;
}

int usb_mic_stream_sample_rate(void)
{
#if USB_MIC_SRC_ENABLE
    /* #ifdef CONFIG_MEDIA_DEVELOP_ENABLE */
    if (usb_mic_src && usb_mic_src->audio_track) {
        int sr = audio_local_sample_track_rate(usb_mic_src->audio_track);
        if ((sr < (usb_mic_src->in_sample_rate + 500)) && (sr > (usb_mic_src->in_sample_rate - 500))) {
            return sr;
        }
        /* printf("uac audio_track reset \n"); */
        local_irq_disable();
        audio_local_sample_track_close(usb_mic_src->audio_track);
        usb_mic_src->audio_track = audio_local_sample_track_open(USB_MIC_CH_NUM, usb_mic_src->in_sample_rate, 1000);
        local_irq_enable();
        return usb_mic_src->in_sample_rate;
    }
    /* #endif */
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

#endif /* TCFG_APP_PC_EN */
