#ifndef AUDIO_ADC_H
#define AUDIO_ADC_H

#include "generic/typedef.h"
#include "generic/list.h"
#include "generic/atomic.h"

/*无电容电路*/
#define SUPPORT_MIC_CAPLESS          1

#define BR22_ADC_BIT_FLAG_FADE_ON	BIT(31)

#define FADE_OUT_IN           	1
#define FADE_OUT_TIME_MS		2

#define LADC_STATE_INIT			1
#define LADC_STATE_OPEN      	2
#define LADC_STATE_START     	3
#define LADC_STATE_STOP      	4

#define FPGA_BOARD          	0

#define LADC_MIC                0
#define LADC_LINEIN0            1
#define LADC_LINEIN1            2
#define LADC_LINEIN             3


#define SOURCE_MONO_LEFT        0
#define SOURCE_MONO_RIGHT       1
#define SOURCE_MONO_LEFT_RIGHT  2

#define ADC_DEFAULT_PNS         128

/* 通道选择 */
#define AUDIO_ADC_MIC_L		    BIT(0)
#define AUDIO_ADC_MIC_R		    BIT(1)
#define AUDIO_ADC_LINE0_L		BIT(2)
#define AUDIO_ADC_LINE0_R		BIT(3)
#define AUDIO_ADC_LINE1_L		BIT(4)
#define AUDIO_ADC_LINE1_R		BIT(5)
#define AUDIO_ADC_LINE2_L		BIT(6)
#define AUDIO_ADC_LINE2_R		BIT(7)
#define AUDIO_ADC_MIC_CH		BIT(0)

#define AUDIO_ADC_LINE0_LR		(AUDIO_ADC_LINE0_L | AUDIO_ADC_LINE0_R)
#define AUDIO_ADC_LINE1_LR		(AUDIO_ADC_LINE1_L | AUDIO_ADC_LINE1_R)
#define AUDIO_ADC_LINE2_LR		(AUDIO_ADC_LINE2_L | AUDIO_ADC_LINE2_R)

#define LADC_CH_MIC_L			BIT(0)
#define LADC_CH_MIC_R			BIT(1)
#define LADC_CH_LINE0_L			BIT(2)
#define LADC_CH_LINE0_R			BIT(3)
#define LADC_CH_LINE1_L			BIT(4)
#define LADC_CH_LINE1_R			BIT(5)

#define LADC_MIC_MASK			(BIT(0) | BIT(1))
#define LADC_LINE0_MASK			(BIT(2) | BIT(3))
#define LADC_LINE1_MASK			(BIT(4) | BIT(5))

/*LINEIN通道定义*/
#define AUDIO_LIN0L_CH			BIT(0)//PA0
#define AUDIO_LIN0R_CH			BIT(1)//PA1
#define AUDIO_LIN1L_CH			BIT(2)//PB7
#define AUDIO_LIN1R_CH			BIT(3)//PB8
#define AUDIO_LIN2L_CH			BIT(4)//PB9
#define AUDIO_LIN2R_CH			BIT(5)//PB10
#define AUDIO_LIN_DACL_CH		BIT(6)
#define AUDIO_LIN_DACR_CH		BIT(7)
#define AUDIO_LIN0_LR			(AUDIO_LIN0L_CH | AUDIO_LIN0R_CH)
#define AUDIO_LIN1_LR			(AUDIO_LIN1L_CH | AUDIO_LIN1R_CH)
#define AUDIO_LIN2_LR			(AUDIO_LIN2L_CH | AUDIO_LIN2R_CH)

struct ladc_port {
    u8 channel;
};

struct adc_platform_data {
    u8 mic_channel;
    u8 mic_ldo_isel; //MIC通道电流档位选择
    u8 mic_capless;  //MIC免电容方案
    u8 mic_bias_res; //MIC免电容方案需要设置，影响MIC的偏置电压 1:16K 2:7.5K 3:5.1K 4:6.8K 5:4.7K 6:3.5K 7:2.9K  8:3K  9:2.5K 10:2.1K 11:1.9K  12:2K  13:1.8K 14:1.6K  15:1.5K 16:1K 31:0.6K
    u8 mic_ldo_vsel : 2;//00:2.3v 01:2.5v 10:2.7v 11:3.0v
    u8 mic_bias_inside : 1;//MIC电容隔直模式使用内部mic偏置(PC7)
    u8 mic_bias_keep : 1;//保持内部mic偏置输出
    u8 reserved: 4;
    u8 ladc_num;
    const struct ladc_port *ladc;
};

struct capless_low_pass {
    u16 bud; //快调边界
    u16 count;
    u16 pass_num;
    u16 tbidx;
};

struct audio_adc_attr {
    u8 gain;
    u16 sample_rate;
    u16 irq_time;
    u16 irq_points;
};

struct adc_stream_ops {
    void (*buf_reset)(void *priv);
    void *(*alloc_space)(void *priv, u32 *len);
    void (*output)(void *priv, void *buf, u32 len);
};

struct audio_adc_output_hdl {
    struct list_head entry;
    void *priv;
    void (*handler)(void *, s16 *, int);
};

struct audio_adc_hdl {
    struct list_head head;
    const struct adc_platform_data *pd;
    atomic_t ref;
    struct audio_adc_attr attr;
    u8 channel;
    u8 input;
    u8 state;
#if FADE_OUT_IN
    u8 fade_on;
    volatile u8 ref_gain;
    volatile int fade_timer;
#endif

#if SUPPORT_MIC_CAPLESS
    struct capless_low_pass lp;
    int last_dacr32;
#endif/*SUPPORT_MIC_CAPLESS*/
};

struct adc_mic_ch {
    struct audio_adc_hdl *hdl;
    u8 gain;
    u8 buf_num;
    u16 buf_size;
    s16 *bufs;
    u16 sample_rate;
    void (*handler)(struct adc_mic_ch *, s16 *, u16);
};

struct audio_adc_ch {
    u8 gain;
    u8 buf_num;
    u8 ch;
    u16 buf_size;
    u16 sample_rate;
    s16 *bufs;
    struct audio_adc_hdl *hdl;
    void (*handler)(struct audio_adc_ch *, s16 *, u16);
};


void audio_adc_init(struct audio_adc_hdl *, const struct adc_platform_data *);

void audio_adc_add_output_handler(struct audio_adc_hdl *, struct audio_adc_output_hdl *);

void audio_adc_del_output_handler(struct audio_adc_hdl *, struct audio_adc_output_hdl *);

void audio_adc_irq_handler(struct audio_adc_hdl *adc);

int audio_adc_mic_open(struct adc_mic_ch *mic, int ch, struct audio_adc_hdl *adc);

int audio_adc_mic_set_sample_rate(struct adc_mic_ch *mic, int sample_rate);
/*
 *mic adc的增益范围：
 *0(0dB)~14(28dB),step:2dB
 */
int audio_adc_mic_set_gain(struct adc_mic_ch *mic, int gain);

int audio_adc_mic_set_buffs(struct adc_mic_ch *mic, s16 *bufs, u16 buf_size, u8 buf_num);

int audio_adc_mic_set_output_handler(struct adc_mic_ch *mic,
                                     void (*handler)(struct adc_mic_ch *, s16 *, u16));

int audio_adc_mic_start(struct adc_mic_ch *mic);

int audio_adc_mic_close(struct adc_mic_ch *mic);



int audio_adc_linein_open(struct audio_adc_ch *adc, int ch, struct audio_adc_hdl *hdl);
int audio_adc_linein_set_sample_rate(struct audio_adc_ch *ch, int sample_rate);
/*
 *(1)linein adc的增益范围：
 *0(-8dB) ~ 15(7dB),8(0dB),step:1dB
 *(2)增益设置规则：
 *A:默认linein两个通道的增益一致:
 *	audio_adc_linein_set_gain(ch,gain);
 *B:如果要分开设置，可以把参数gain分成高低16bit来设置:
 *	audio_adc_linein_set_gain(ch,(gain_l | (gain_r << 16)));
 */
int audio_adc_linein_set_gain(struct audio_adc_ch *ch, int gain);
int audio_adc_set_buffs(struct audio_adc_ch *ch, s16 *bufs, u16 buf_size, u8 buf_num);
int audio_adc_linein_start(struct audio_adc_ch *ch);
int audio_adc_linein_close(struct audio_adc_ch *ch);

int audio_adc_start(struct audio_adc_ch *linein_ch, struct adc_mic_ch *mic_ch);
int audio_adc_close(struct audio_adc_ch *linein_ch, struct adc_mic_ch *mic_ch);

void audio_mic_0dB_en(bool en);

#endif
