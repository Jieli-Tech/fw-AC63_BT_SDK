/*
 ****************************************************************
 *							AUDIO_ADC_DEMO
 * File  : audio_adc_demo.c
 * By    :
 * Notes :Audio_ADC使用demo，请不要修改本demo，如有需求，请拷贝副
 *		  本，自行修改
 ****************************************************************
 */
#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "audio_config.h"

/*AUDIO_ADC采样参数输出到DAC配置(调试使用)*/
#define LADC_2_DAC_ENABLE	1

/*AUDIO_ADC采样参数配置*/
#define LADC_BUF_NUM        2 	/*adc采样buf数*/
#define LADC_CH_NUM         1 	/*adc采样通道数*/
#define LADC_IRQ_POINTS     256	/*adc采样buf长度，即多少点起一次中断*/
#define LADC_BUFS_SIZE      (LADC_CH_NUM * LADC_BUF_NUM * LADC_IRQ_POINTS)

struct ladc_demo {
    struct audio_adc_output_hdl adc_output;
    struct adc_mic_ch mic_ch;
    s16 adc_buf[LADC_BUFS_SIZE];    //align 2Bytes
    s16 temp_buf[LADC_IRQ_POINTS * 2];
};
static struct ladc_demo *ladc = NULL;

extern struct audio_adc_hdl adc_hdl;

/*audio_adc采样数据输出回调*/
static void audio_adc_demo_output(void *priv, s16 *data, int len)
{
    struct audio_adc_hdl *hdl = priv;
    putchar('o');
    if (hdl == NULL) {
        printf("audio_adc_hdl err:NULL!!!!");
        return;
    }
    //printf("hdl:%x,data:%x,len:%d,ch:%d\n",hdl,data,len,hdl->channel);

#if LADC_2_DAC_ENABLE
#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR)//双声道数据结构
    for (int i = 0; i < (len / 2); i++) {
        ladc->temp_buf[i * 2] = data[i];
        ladc->temp_buf[i * 2 + 1] = data[i];
    }
    int wlen = app_audio_output_write(ladc->temp_buf, len * 2);
    if (wlen != (len * 2)) {
        //printf("wlen:%d-%d",wlen,len * 2);
    }
#else //单声道数据结构
    int wlen = app_audio_output_write(data, len * hdl->channel);
    if (wlen != len) {
        //printf("wlen:%d-%d",wlen,len);
    }
#endif/*TCFG_AUDIO_DAC_CONNECT_MODE*/
#endif/*LADC_2_DAC_ENABLE*/
}

static u8 mic_demo_idle_query()
{
    return (ladc ? 0 : 1);
}
REGISTER_LP_TARGET(mic_demo_lp_target) = {
    .name = "mic_demo",
    .is_idle = mic_demo_idle_query,
};

/*
 *Audio_ADC使用demo以及测试手段
 *通过注册回调函数，可以获取ADC采样数据
 */
void audio_adc_open_demo(void)
{
    u16 sr = 16000;
    u8 gain = 10;
    printf("audio_adc_open:%d\n", sr);
    ladc = zalloc(sizeof(struct ladc_demo));
    if (ladc) {
        audio_adc_mic_open(&ladc->mic_ch, AUDIO_ADC_MIC_CH, &adc_hdl);
        audio_adc_mic_set_sample_rate(&ladc->mic_ch, sr);
        audio_adc_mic_set_gain(&ladc->mic_ch, gain);
        audio_adc_mic_set_buffs(&ladc->mic_ch, ladc->adc_buf, LADC_IRQ_POINTS * 2, LADC_BUF_NUM);
        /*注册ADC采样回调函数*/
        ladc->adc_output.handler = audio_adc_demo_output;
        /*传给回调函数的私有参数*/
        ladc->adc_output.priv = &adc_hdl;
        audio_adc_add_output_handler(&adc_hdl, &ladc->adc_output);
        audio_adc_mic_start(&ladc->mic_ch);

        /*可以通过把ADC采样数据推到DAC进行播放，直观判定数据是否初步正常*/
#if LADC_2_DAC_ENABLE
        app_audio_output_samplerate_set(sr);
        app_audio_output_start();
#endif/*LADC_2_DAC_ENABLE*/
    }
}

void audio_adc_demo_close(void)
{
    if (ladc) {
        audio_adc_del_output_handler(&adc_hdl, &ladc->adc_output);
        audio_adc_mic_close(&ladc->mic_ch);
        free(ladc);
        ladc = NULL;
    }
}

/********************************************************/
/*               linein demo                            */
/********************************************************/


/*AUDIO_ADC采样参数配置*/
#define LADC_LINEIN_BUF_NUM        2 	/*adc采样buf数*/
#define LADC_LINEIN_CH_NUM         2 	/*adc采样通道数*/
#define LADC_LINEIN_IRQ_POINTS     256	/*adc采样buf长度，即多少点起一次中断*/
#define LADC_LINEIN_BUFS_SIZE      (LADC_LINEIN_CH_NUM * LADC_LINEIN_BUF_NUM * LADC_LINEIN_IRQ_POINTS)

struct ladc_linein_demo {
    struct audio_adc_output_hdl adc_output;
    struct audio_adc_ch linein_ch;
    s16 adc_buf[LADC_LINEIN_BUFS_SIZE];    //align 2Bytes
    s16 temp_buf[LADC_LINEIN_IRQ_POINTS * 2];
};
static struct ladc_linein_demo *ladc_linein_hdl = NULL;

/*audio_adc采样数据输出回调*/
static void audio_linein_output(void *priv, s16 *data, int len)
{
    struct audio_adc_hdl *hdl = priv;
    putchar('o');
    if (hdl == NULL) {
        printf("audio_adc_hdl err:NULL!!!!");
        return;
    }
    /* printf("hdl:%x,data:%x,len:%d,ch:%d\n",hdl,data,len,hdl->channel); */

#if LADC_2_DAC_ENABLE
#if (TCFG_AUDIO_DAC_CONNECT_MODE == DAC_OUTPUT_LR)//双声道数据结构
    if (hdl->channel == 1) {
        for (int i = 0; i < (len / 2); i++) {
            ladc_linein_hdl->temp_buf[i * 2] = data[i];
            ladc_linein_hdl->temp_buf[i * 2 + 1] = data[i];
        }
        int wlen = app_audio_output_write(ladc_linein_hdl->temp_buf, len * 2);
        if (wlen != (len * 2)) {
            //printf("wlen:%d-%d",wlen,len * 2);
        }
    } else {
        int wlen = app_audio_output_write(ladc_linein_hdl->temp_buf, len * 2);
        if (wlen != (len * 2)) {
            //printf("wlen:%d-%d",wlen,len * 2);
        }

    }
#else //单声道数据结构
    if (hdl->channel == 1) {
        int wlen = app_audio_output_write(data, len * hdl->channel);
        if (wlen != len) {
            //printf("wlen:%d-%d",wlen,len);
        }
    } else {
        for (int i = 0; i < (len / 2); i++) {
            ladc_linein_hdl->temp_buf[i] = data[i * 2] + data[i * 2 + 1];
        }
        int wlen = app_audio_output_write(data, len);
        if (wlen != len) {
            //printf("wlen:%d-%d",wlen,len);
        }

    }
#endif/*TCFG_AUDIO_DAC_CONNECT_MODE*/
#endif/*LADC_2_DAC_ENABLE*/
}

static u8 linein_demo_idle_query()
{
    return (ladc_linein_hdl ? 0 : 1);
}
REGISTER_LP_TARGET(linein_demo_lp_target) = {
    .name = "linein_demo",
    .is_idle = linein_demo_idle_query,
};

void audio_linein_open_demo(void)
{
    /* u16 sr = 16000; */
    u16 sr = 44100;
    u8 gain = 10;
    printf("audio_adc_linein_open:%d\n", sr);

    ladc_linein_hdl = zalloc(sizeof(struct ladc_linein_demo));
    if (ladc_linein_hdl) {
        audio_adc_linein_open(&ladc_linein_hdl->linein_ch, AUDIO_ADC_LINE0_LR, &adc_hdl);
        audio_adc_linein_set_sample_rate(&ladc_linein_hdl->linein_ch, sr);
        audio_adc_linein_set_gain(&ladc_linein_hdl->linein_ch, gain);
        audio_adc_set_buffs(&ladc_linein_hdl->linein_ch, ladc_linein_hdl->adc_buf, LADC_LINEIN_IRQ_POINTS * 2, LADC_LINEIN_BUF_NUM);
        /*注册ADC采样回调函数*/
        ladc_linein_hdl->adc_output.handler = audio_linein_output;
        /*传给回调函数的私有参数*/
        ladc_linein_hdl->adc_output.priv = &adc_hdl;
        audio_adc_add_output_handler(&adc_hdl, &ladc_linein_hdl->adc_output);
        audio_adc_linein_start(&ladc_linein_hdl->linein_ch);

        /*可以通过把ADC采样数据推到DAC进行播放，直观判定数据是否初步正常*/
#if LADC_2_DAC_ENABLE
        app_audio_output_samplerate_set(sr);
        app_audio_output_start();
#endif/*LADC_2_DAC_ENABLE*/
    }
}

void audio_linein_demo_close(void)
{
    if (ladc_linein_hdl) {
        audio_adc_del_output_handler(&adc_hdl, &ladc_linein_hdl->adc_output);
        audio_adc_linein_close(&ladc_linein_hdl->linein_ch);
        free(ladc_linein_hdl);
        ladc_linein_hdl = NULL;
    }
}

