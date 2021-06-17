#ifndef AUDIO_ADC_H
#define AUDIO_ADC_H

#include "generic/typedef.h"
#include "generic/list.h"
#include "generic/atomic.h"

/*无电容电路*/
#define SUPPORT_MIC_CAPLESS     1

/*Audio_ADC状态*/
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

/*Audio_ADC通道选择 */
#define AUDIO_ADC_MIC_L					    BIT(0)
#define AUDIO_ADC_MIC_R					    BIT(1)
#define AUDIO_ADC_LINE0_L					BIT(2)
#define AUDIO_ADC_LINE0_R					BIT(3)
#define AUDIO_ADC_LINE1_L					BIT(4)
#define AUDIO_ADC_LINE1_R					BIT(5)
#define AUDIO_ADC_MIC_CH					BIT(0)

#define LADC_CH_MIC_L					    BIT(0)
#define LADC_CH_MIC_R					    BIT(1)
#define LADC_CH_LINE0_L						BIT(2)
#define LADC_CH_LINE0_R						BIT(3)
#define LADC_CH_LINE1_L						BIT(4)
#define LADC_CH_LINE1_R						BIT(5)
#define PLNK_MIC							BIT(6)

#define LADC_MIC_MASK					(BIT(0) | BIT(1))
#define LADC_LINE0_MASK					(BIT(2) | BIT(3))
#define LADC_LINE1_MASK					(BIT(4) | BIT(5))

struct adc_platform_data {
    /*
     *MIC0内部上拉电阻档位
     *21:1.18K	20:1.42K 	19:1.55K 	18:1.99K 	17:2.2K 	16:2.4K 	15:2.6K		14:2.91K	13:3.05K 	12:3.5K 	11:3.73K
     *10:3.91K  9:4.41K 	8:5.0K  	7:5.6K		6:6K		5:6.5K		4:7K		3:7.6K		2:8.0K		1:8.5K
     */
    u8 mic_bias_res;

    u16 mic_capless : 1;  		//MIC0免电容方案
    u16 mic_diff : 1;  			//MIC0差分模式方案
    u16 mic_ldo_isel: 2; 		//MIC0_LDO电流档[00:5u 01:10u 10:15u 11:20u]
    u16 mic_ldo_vsel : 3;		//MIC0_LDO电压档[000:1.5v 001:1.8v 010:2.1v 011:2.4v 100:2.7v ]
    u16 mic_bias_inside : 1;	//MIC0电容隔直模式使用内部mic偏置(PA2)
    u16 mic_bias_keep : 1;		//保持内部MIC0偏置输出
    u16 mic_adc_order : 1;		//MICADC阶数[0:一阶 1：两阶]
    u16 reserved : 6;
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
    u16 pause_srp;
    u16 pns;
    u8 fifo_full;
    u8 channel;
    u8 ch_sel;
    u8 input;
    u8 state;
#if SUPPORT_MIC_CAPLESS
    struct capless_low_pass lp0;
    struct capless_low_pass *lp;
#endif/*SUPPORT_MIC_CAPLESS*/
};

struct adc_mic_ch {
    struct audio_adc_hdl *adc;
    u8 gain;
    u8 gain1;
    u8 buf_num;
    u16 buf_size;
    s16 *bufs;
    u16 sample_rate;
    void (*handler)(struct adc_mic_ch *, s16 *, u16);
};

void audio_adc_init(struct audio_adc_hdl *, const struct adc_platform_data *);

void audio_adc_add_output_handler(struct audio_adc_hdl *, struct audio_adc_output_hdl *);

void audio_adc_del_output_handler(struct audio_adc_hdl *, struct audio_adc_output_hdl *);

void audio_adc_irq_handler(struct audio_adc_hdl *adc);

int audio_adc_mic_open(struct adc_mic_ch *mic, int ch, struct audio_adc_hdl *adc);

int audio_adc_mic_set_sample_rate(struct adc_mic_ch *mic, int sample_rate);

int audio_adc_mic_set_gain(struct adc_mic_ch *mic, int gain);

int audio_adc_mic_set_buffs(struct adc_mic_ch *mic, s16 *bufs, u16 buf_size, u8 buf_num);

int audio_adc_mic_set_output_handler(struct adc_mic_ch *mic,
                                     void (*handler)(struct adc_mic_ch *, s16 *, u16));

int audio_adc_mic_start(struct adc_mic_ch *mic);

int audio_adc_mic_close(struct adc_mic_ch *mic);

void audio_mic_0dB_en(bool en);

int audio_mic_ldo_en(u8 en, struct adc_platform_data *pd);

#endif
