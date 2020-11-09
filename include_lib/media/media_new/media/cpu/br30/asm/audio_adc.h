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

#define FPGA_BOARD          0

#define LADC_MIC                0
#define LADC_LINEIN0            1
#define LADC_LINEIN1            2
#define LADC_LINEIN             3


#define SOURCE_MONO_LEFT        0
#define SOURCE_MONO_RIGHT       1
#define SOURCE_MONO_LEFT_RIGHT  2

#define ADC_DEFAULT_PNS         128

/* 通道选择 */
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

struct ladc_port {
    u8 channel;
};

struct adc_platform_data {
    u8 mic_channel;
    u8 ladc_num;
    /*
     *MIC0内部上拉电阻档位
     *21:1.18K	20:1.42K 	19:1.55K 	18:1.99K 	17:2.2K 	16:2.4K 	15:2.6K		14:2.91K	13:3.05K 	12:3.5K 	11:3.73K
     *10:3.91K  9:4.41K 	8:5.0K  	7:5.6K		6:6K		5:6.5K		4:7K		3:7.6K		2:8.0K		1:8.5K
     */
    u8 mic_bias_res;
    /*MIC1内部上拉电阻档位
     *21:1.18K	20:1.42K 	19:1.55K 	18:1.99K 	17:2.2K 	16:2.4K 	15:2.6K		14:2.91K	13:3.05K 	12:3.5K 	11:3.73K
     *10:3.91K  9:4.41K 	8:5.0K  	7:5.6K		6:6K		5:6.5K		4:7K		3:7.6K		2:8.0K		1:8.5K
     */
    u8 mic1_bias_res;

    u32 mic_capless : 1;  		//MIC0免电容方案
    u32 mic_diff : 1;  			//MIC0差分模式方案
    u32 mic_ldo_isel: 2; 		//MIC0通道电流档位选择
    u32 mic_ldo_vsel : 3;		//MIC0_LDO[000:1.5v 001:1.8v 010:2.1v 011:2.4v 100:2.7v 101:3.0v]
    u32 mic_bias_inside : 1;	//MIC0电容隔直模式使用内部mic偏置(PC7)
    u32 mic_bias_keep : 1;		//保持内部MIC0偏置输出
    u32 mic_in_sel : 1;			//MICIN选择[0:MIC0 1:MICEXT(ldo5v)]
    u32 mic_ldo_state : 1;      //当前micldo是否打开
    u32 mic1_capless : 1;		//MIC1免电容方案
    u32 mic1_diff : 1;			//MIC1差分模式方案
    u32 mic1_ldo_isel: 2; 		//MIC0通道电流档位选择
    u32 mic1_ldo_vsel : 3;		//MIC1_LDO 00:2.3v 01:2.5v 10:2.7v 11:3.0v
    u32 mic1_bias_inside : 1;	//MIC1电容隔直模式使用内部mic偏置(PC7)
    u32 mic1_bias_keep : 1;		//保持内部MIC1偏置输出
    u32 mic1_in_sel : 1;		//MICIN1选择[0:MIC1 1:MICEXT(ldo5v)]
    u32 mic1_ldo_state : 1;		//当前micldo是否打开
    u32 reserved : 10;
    const struct ladc_port *ladc;
};

struct capless_low_pass {
    u16 bud; //快调边界
    u16 count;
    u16 pass_num;
    u16 tbidx;
    u32 bud_factor;
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
    /*void *priv;
    const struct adc_stream_ops *ops;*/
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
    union {
        struct {
            u8 out2dac : 1;
            u8 out_l : 1;
            u8 out_r : 1;
            u8 out_mix : 1;
            u8 cha_idx : 2;
        } ladc;
        u8 param;
    } u;
#if FADE_OUT_IN
    u8 fade_on;
    volatile u8 ref_gain;
    volatile int fade_timer;
#endif
#if SUPPORT_MIC_CAPLESS
    struct capless_low_pass lp0;
    struct capless_low_pass lp1;
    struct capless_low_pass *lp;
#endif
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

int audio_adc_mic1_open(struct adc_mic_ch *mic, int ch, struct audio_adc_hdl *adc);
int audio_adc_mic1_set_gain(struct adc_mic_ch *mic, int gain);
int audio_adc_mic1_close(struct adc_mic_ch *mic);
void audio_mic_0dB_en(bool en);
void audio_mic1_0dB_en(bool en);
/*
 *en：控制micldo开关
 *pd->mic_bias_keep控制对应对应偏置io输出（micldo经过分压电阻出来的）
 */
int audio_mic_ldo_en(u8 en, struct adc_platform_data *pd);














#endif
