
#ifndef _AUDIO_DEC_MIC2PCM_H_
#define _AUDIO_DEC_MIC2PCM_H_


#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "app_config.h"


#define FIXED_SAMPLE_RATE             44100  //固定采样率

#define PCM_DEC_IN_SIZE	   512
#define PCM_DEC_IN_CBUF_SIZE	(PCM_DEC_IN_SIZE * 8)

struct pcm_dec_hdl {
    struct audio_decoder decoder;
    struct audio_res_wait wait;		// 资源等待句柄
    struct audio_mixer_ch mix_ch;	// 叠加句柄
    struct audio_eq_drc *eq_drc;    //eq drc句柄
    struct audio_src_handle *hw_src; //硬件src
    cbuffer_t pcm_in_cbuf;
    u8 *p_in_cbuf_buf;
    u8 channel;
    u8 output_ch;
    u16 two_ch_remain_len;
    u16 two_ch_remain_addr;
    u16 sample_rate;
    u32 coding_type;
    u32 src_out_sr;
    u32 id;				// 唯一标识符，随机值
    u32 start : 1;		// 正在解码
};

#define ADC_MIC_BUF_NUM        2
#define ADC_MIC_IRQ_POINTS     256
#define ADC_MIC_BUFS_SIZE      (ADC_MIC_BUF_NUM * ADC_MIC_IRQ_POINTS)

struct adc_mic_hdl {
    u8 idle;
    struct audio_adc_output_hdl adc_output;
    struct adc_mic_ch mic_ch;
    s16 adc_buf[ADC_MIC_BUFS_SIZE];    //align 4Bytes
};


/* 打开mic监听功能，设置mic采样率 */
void audio_mic2pcm_dec_open(u16 sample_rate);

/* 关闭mic2pcm监听功能,释放资源 */
void mic2pcm_dec_close(void);

/* mic状态切换,设置mic的采样率,打开mic返回1, 关闭mic返回 0, */
int mic2pcm_open_status_switch(u16 sample_rate);

/* 判断mic是否处于运行状态, 运行返回1，未运行返回0 */
int mic2pcm_dec_is_running(void);

/* mic监听功能重启, 即关闭后延时XX ms重启mic监听 */
void mic2pcm_dec_reset(u16 sample_rate, u16 time);



#endif


