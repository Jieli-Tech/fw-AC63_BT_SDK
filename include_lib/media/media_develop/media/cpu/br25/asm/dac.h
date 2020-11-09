#ifndef __CPU_DAC_H__
#define __CPU_DAC_H__


#include "generic/typedef.h"
#include "generic/atomic.h"
#include "os/os_api.h"
#include "media/audio_stream.h"


#define DAC_44_1KHZ            0
#define DAC_48KHZ              1
#define DAC_32KHZ              2
#define DAC_22_05KHZ           3
#define DAC_24KHZ              4
#define DAC_16KHZ              5
#define DAC_11_025KHZ          6
#define DAC_12KHZ              7
#define DAC_8KHZ               8

#define DAC_ISEL_FULL_PWR			4
#define DAC_ISEL_HALF_PWR			2
#define DAC_ISEL_THREE_PWR			1

#define DACVDD_LDO_2_40V        0
#define DACVDD_LDO_2_50V        1
#define DACVDD_LDO_2_60V        2
#define DACVDD_LDO_2_70V        3
#define DACVDD_LDO_2_80V        4
#define DACVDD_LDO_2_90V        5
#define DACVDD_LDO_3_00V        6
#define DACVDD_LDO_3_10V        7

#define DAC_OUTPUT_MONO_L                  0    //左声道
#define DAC_OUTPUT_MONO_R                  1    //右声道
#define DAC_OUTPUT_LR                      2    //立体声
#define DAC_OUTPUT_MONO_LR_DIFF            3    //单声道差分输出

#define DAC_OUTPUT_DUAL_LR_DIFF            6    //双声道差分
#define DAC_OUTPUT_FRONT_LR_REAR_L         7    //三声道单端输出 前L+前R+后L (不可设置vcmo公共端)
#define DAC_OUTPUT_FRONT_LR_REAR_R         8    //三声道单端输出 前L+前R+后R (可设置vcmo公共端)
#define DAC_OUTPUT_FRONT_LR_REAR_LR        9    //四声道单端输出

#define AUDIO_DAC_SYNC_IDLE             0
#define AUDIO_DAC_SYNC_PROBE            1
#define AUDIO_DAC_SYNC_START            2
#define AUDIO_DAC_SYNC_NO_DATA          3
#define AUDIO_DAC_SYNC_ALIGN_COMPLETE   4

#define DAC_DSM_6MHz                    0
#define DAC_DSM_12MHz                   1

#define FADE_OUT_IN           1
#define PCM_PHASE_BIT         8

#define DA_LEFT             0
#define DA_RIGHT            1
#define DA_ALL_CH           2
#define DA_SOUND_NORMAL                 0x0
#define DA_SOUND_RESET                  0x1
#define DA_SOUND_WAIT_RESUME            0x2

enum {
    DAC_EVENT_START,
    DAC_EVENT_SAMPLE_BEGIN,
    DAC_EVENT_SAMPLE_STOP,
    DAC_EVENT_SAMPLE_MISS_DATA,
    DAC_EVENT_CLOSE,
};

struct dac_platform_data {
    u32 output : 4;             //DAC输出模式
    u32 ldo_volt : 4;           //DACVDD_LDO电压档选择
    u32 ldo_isel : 4;           //LDO偏置电流选择档位, 0:5u, 1:10u, 2:15u, 3:20u, 4:25u, 5:30u, 6:35u, 7:40u
    u32 lpf_isel : 4;           //LPF bias电流选择, 0:无, 1:0.3125u, 2:0.625u, 3:0.9375, 4:1.25u, 5:1.5625, 6:1.875u, 7:2.1875u, 8:2.5u, 9:2.8125u, 10:3.125u, 11:3.4375u, 12:3.75u, 13:4.0625u, 14:4.375u, 15:4.6875u
    u32 ldo_fb_isel : 2;        //LDO负载电流选择, 0:15u, 1:48u, 2:81u, 3:114u
    u32 vcmo_en : 1;            //VCMO直推使能
    u32 keep_vcmo : 1;
    u32 dither_type : 1;        //0:
    u32 dsm_clk : 1;            //0:dsm 6M dither 187.5KHz    1:dsm 12M dither 375KHz
    u32 vcm_speed : 1;          //0:
    u32 zero_cross_detect : 1;  //模拟增益过零检测配置
};


struct analog_module {
    /*模拟相关的变量*/
    u8 inited;
    u16 dac_test_volt;
};
struct audio_dac_trim {
    s16 left;
    s16 right;
    s16 rear_left;
    s16 rear_right;
};

struct audio_dac_sync {
    u8 start;
    u8 channel;
    u8 fast_align;
    u8 connect_sync;
    u32 buf_counter;
    u32 pcm_position;
    u32 input_num;
    int fast_points;
    int phase_sub;
    u16 in_rate;
    u16 out_rate;
    void *buf;
    int buf_len;
    void *priv;
    int (*handler)(void *priv, u8 state);
};

struct audio_dac_fade {
    u8 enable;
    volatile u8 ref_L_gain;
    volatile u8 ref_R_gain;
    int timer;
};
struct audio_dac_mix_ch {
    u8 open;
    u8 first_in;
    s16 write_ptr;
    s16 max_delay_time;
    u16 data_size;
    u16 dac_data_points;
};

struct audio_dac_hdl {
    struct analog_module analog;
    // const
    struct dac_platform_data *pd;
    OS_SEM sem;
    struct audio_dac_trim trim;
    struct audio_sample_sync *sample_sync; /*样点同步句柄*/
    struct audio_dac_mix_ch mix;
    void (*fade_handler)(u8 left_gain, u8 right_gain);
    u8 inner_sync;
    volatile u8 mute;
    volatile u8 state;
    u8 gain_l;
    u8 gain_r;
    u8 vol_l;
    u8 vol_r;

    u8 channel;
    u16 max_d_volume;
    u16 d_volume[2];
    u32 sample_rate;
    u16 start_ms;
    u16 delay_ms;
    u16 start_points;
    u16 delay_points;
    u16 prepare_points;//未开始让DAC真正跑之前写入的PCM点数

    u16 irq_points;
    u8 channel_mode;
    u8 dec_channel_num;
    u8 mute_event;

    s16 *output_buf;
    u16 output_buf_len;
    u8 sound_state;
    u16 irq_timeout;
    s16 read_offset;
    unsigned long sound_resume_time;
    struct audio_stream_entry entry;
};



int audio_dac_init(struct audio_dac_hdl *dac, const struct dac_platform_data *pd);

void audio_dac_set_capless_DTB(struct audio_dac_hdl *dac, s16 dacr32);

void audio_dac_avdd_level_set(struct audio_dac_hdl *dac, u8 level);

void audio_dac_lpf_level_set(struct audio_dac_hdl *dac, u8 level);

int audio_dac_do_trim(struct audio_dac_hdl *dac, struct audio_dac_trim *dac_trim, u8 fast_trim);

int audio_dac_set_trim_value(struct audio_dac_hdl *dac, struct audio_dac_trim *dac_trim);

int audio_dac_set_delay_time(struct audio_dac_hdl *dac, int start_ms, int max_ms);

void audio_dac_irq_handler(struct audio_dac_hdl *dac);

int audio_dac_set_buff(struct audio_dac_hdl *dac, s16 *buf, int len);

int audio_dac_write(struct audio_dac_hdl *dac, void *buf, int len);

s16 *audio_dac_get_write_ptr(struct audio_dac_hdl *dac, int *len);

int audio_dac_update_write_ptr(struct audio_dac_hdl *dac, int len);

int audio_dac_set_sample_rate(struct audio_dac_hdl *dac, int sample_rate);

int audio_dac_get_sample_rate(struct audio_dac_hdl *dac);

int audio_dac_get_channel(struct audio_dac_hdl *dac);

int audio_dac_set_channel(struct audio_dac_hdl *dac, u8 channel);

int audio_dac_get_pd_output(struct audio_dac_hdl *dac);

int audio_dac_set_pd_output(struct audio_dac_hdl *dac, u8 output);

int audio_dac_set_digital_vol(struct audio_dac_hdl *dac, u16 vol);

int audio_dac_set_analog_vol(struct audio_dac_hdl *dac, u16 vol);
int audio_dac_mix_ch_get_datasize(struct audio_dac_hdl *dac);
int audio_dac_mix_ch_open(struct audio_dac_hdl *dac, int delay_ms);

int audio_dac_mix_ch_close(struct audio_dac_hdl *dac);

int audio_dac_mix_write(struct audio_dac_hdl *dac, void *buf, int len);
int audio_dac_start(struct audio_dac_hdl *dac);

int audio_dac_stop(struct audio_dac_hdl *dac);

int audio_dac_idle(struct audio_dac_hdl *dac);

void audio_dac_mute(struct audio_dac_hdl *hdl, u8 mute);

int audio_dac_open(struct audio_dac_hdl *dac);

int audio_dac_close(struct audio_dac_hdl *dac);

int audio_dac_mute_left(struct audio_dac_hdl *dac);

int audio_dac_mute_right(struct audio_dac_hdl *dac);

int audio_dac_set_volume(struct audio_dac_hdl *dac, u8 gain_l, u8 gain_r);

int audio_dac_set_L_digital_vol(struct audio_dac_hdl *dac, u16 vol);

int audio_dac_set_R_digital_vol(struct audio_dac_hdl *dac, u16 vol);

void audio_dac_set_fade_handler(struct audio_dac_hdl *dac, void *priv, void (*fade_handler)(u8, u8, u8, u8));

int audio_dac_power_off(struct audio_dac_hdl *dac);

bool audio_dac_sample_rate_match(struct audio_dac_hdl *dac, u32 sample_rate);

int audio_dac_sample_rate_select(struct audio_dac_hdl *dac, u32 sample_rate, u8 high);

int audio_dac_power_off(struct audio_dac_hdl *dac);

bool audio_dac_sample_rate_match(struct audio_dac_hdl *dac, u32 sample_rate);

int audio_dac_sample_rate_select(struct audio_dac_hdl *dac, u32 sample_rate, u8 high);

int audio_dac_sound_reset(struct audio_dac_hdl *dac, u32 msecs);

u8 audio_dac_is_working(struct audio_dac_hdl *dac);

void audio_dac_set_irq_time(struct audio_dac_hdl *dac, int time_ms);

int audio_dac_data_time(struct audio_dac_hdl *dac);

int audio_dac_get_status(struct audio_dac_hdl *dac);

int audio_dac_get_max_channel(void);

int audio_dac_ch_analog_gain_set(struct audio_dac_hdl *dac, u8 ch, u8 again);

int audio_dac_ch_analog_gain_get(struct audio_dac_hdl *dac, u8 ch);

int audio_dac_ch_digital_gain_set(struct audio_dac_hdl *dac, u8 ch, u32 dgain);

int audio_dac_ch_digital_gain_get(struct audio_dac_hdl *dac, u8 ch);

void audio_dac_ch_mute(struct audio_dac_hdl *dac, u8 ch, u8 mute);

int audio_dac_set_RL_digital_vol(struct audio_dac_hdl *dac, u16 vol);
int audio_dac_set_RR_digital_vol(struct audio_dac_hdl *dac, u16 vol);

int audio_dac_fifo_set_read(struct audio_dac_hdl *dac, int offset);

/*
 *  fifo数据读取
 *  data - 数据
 *  len - 长度(byte)
 *  channel - 读取DAC的channel, DA_LEFT/DA_RIGHT/DA_ALL_CH
 */
int audio_dac_fifo_read(struct audio_dac_hdl *dac, void *data, int len, u8 channel);

/*================================我是优雅的分割线===================================*/
/*
 * 这里只放DAC同步的接口
 *
 *===================================================================================*/
int audio_dac_add_sample_sync(struct audio_dac_hdl *dac, void *sample_sync, u8 in_dac);

int audio_dac_remove_sample_sync(struct audio_dac_hdl *dac, void *sample_sync);
#endif

