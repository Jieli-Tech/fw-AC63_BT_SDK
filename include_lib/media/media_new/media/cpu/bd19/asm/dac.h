#ifndef __CPU_DAC_H__
#define __CPU_DAC_H__


#include "generic/typedef.h"
#include "generic/atomic.h"
#include "os/os_api.h"
#include "audio_src.h"

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

#define DACVDD_LDO_1_20V        0
#define DACVDD_LDO_1_25V        1
#define DACVDD_LDO_1_30V        2
#define DACVDD_LDO_1_35V        3

#define DAC_OUTPUT_MONO_L                  0    //左声道
#define DAC_OUTPUT_MONO_R                  1    //右声道
#define DAC_OUTPUT_LR                      2    //立体声
#define DAC_OUTPUT_MONO_LR_DIFF            3    //单声道差分输出

#define DAC_DSM_DEFAULT                 0
#define DAC_DSM_6MHz                    1
#define DAC_DSM_12MHz                   2

// #define DAC_OUTPUT_DUAL_LR_DIFF            6    //双声道差分
// #define DAC_OUTPUT_FRONT_LR_REAR_L         7    //三声道单端输出 前L+前R+后L (不可设置vcmo公共端)
// #define DAC_OUTPUT_FRONT_LR_REAR_R         8    //三声道单端输出 前L+前R+后R (可设置vcmo公共端)
// #define DAC_OUTPUT_FRONT_LR_REAR_LR        9    //四声道单端输出

#define FADE_OUT_IN           1

#define AUDIO_DAC_SYNC_IDLE             0
#define AUDIO_DAC_SYNC_PROBE            1
#define AUDIO_DAC_SYNC_START            2
#define AUDIO_DAC_SYNC_NO_DATA          3
#define AUDIO_DAC_ALIGN_NO_DATA         4
#define AUDIO_DAC_SYNC_ALIGN_COMPLETE   5
#define AUDIO_DAC_SYNC_KEEP_RATE_DONE   6

#define AUDIO_SRC_SYNC_ENABLE   1
#define SYNC_LOCATION_FLOAT      1
#if SYNC_LOCATION_FLOAT
#define PCM_PHASE_BIT           0
#else
#define PCM_PHASE_BIT           8
#endif

#define DA_LEFT        0
#define DA_RIGHT       1

#define DA_SOUND_NORMAL                 0x0
#define DA_SOUND_RESET                  0x1
#define DA_SOUND_WAIT_RESUME            0x2

#define DACR32_DEFAULT		8192
#define DA_SYNC_INPUT_BITS              20
#define DA_SYNC_MAX_NUM                 (1 << DA_SYNC_INPUT_BITS)

#define DIFF_OSC_CLK                    0   //差分晶振配置
#define DIGITAL_SINGLE_CLK              1   //数字单端配置

struct audio_dac_hdl;
struct dac_platform_data {
    void (*analog_open_cb)(struct audio_dac_hdl *);
    void (*analog_close_cb)(struct audio_dac_hdl *);
    u32 output : 4;             //DAC输出模式
    u32 ldo_volt : 2;           //DACVDD_LDO电压档选择,0 : 1.2V, 1 : 1.25V, 2 : 1.3V, 3 : 1.35V
    u32 ldo_isel : 2;           //LDO偏置电流选择档位, 0:5u, 1:10u, 2:15u, 3:20u
    u32 lpf_isel : 4;           //LPF bias电流选择, 0:无, 1:0.625u, 2:1.25u, 3:2.1875u, 4:2.5u, 5:3.125u, 6:3.75u, 7:4.375u
    u32 ldo_fb_isel : 2;        //LDO负载电流选择, 0:14u, 1:47u, 2:80u, 3:113u
    u32 vcmo_en : 1;            //VCMO直推使能
    u32 dsm_clk : 1;
    u32 zero_cross_detect : 1;  //过零点检测使能
    u32 light_close : 1;        //使能轻量级关闭，最低功耗保持dac开启
    u32 dac_analog_clock_source : 1;          //dac 模拟时钟来源配置，选0为差分晶振，选1为数字单端配置
};


struct analog_module {
    /*模拟相关的变量*/
#if 0
    unsigned int ldo_en : 1;
    unsigned int ldo_volt : 3;
    unsigned int output : 4;
    unsigned int vcmo : 1;
    unsigned int inited : 1;
    unsigned int lpf_isel : 4;
    unsigned int keep_vcmo : 1;
    unsigned int reserved : 17;
#endif
    u8 inited;
    u16 dac_test_volt;
};
struct audio_dac_trim {
    s16 left;
    s16 right;
    s16 vcomo;
};

// *INDENT-OFF*
struct audio_dac_sync {
    u32 channel : 3;
    u32 start : 1;
    u32 fast_align : 1;
    u32 connect_sync : 1;
    u32 release_by_bt : 1;
    u32 slience_scale : 1;
    u32 input_num : DA_SYNC_INPUT_BITS;
    int fast_points;
    int keep_points;
    int phase_sub;
    int in_rate;
    int out_rate;
    int bt_clkn;
    int bt_clkn_phase;
#if AUDIO_SRC_SYNC_ENABLE
    struct audio_src_sync_handle *src_sync;
    void *buf;
    int buf_len;
    void *filt_buf;
    int filt_len;
#else
    struct audio_src_base_handle *src_base;
#endif
#if SYNC_LOCATION_FLOAT
    float pcm_position;
#else
    u32 pcm_position;
#endif
    void *priv;
    int (*handler)(void *priv, u8 state);
    void *correct_priv;
    void (*correct_cabllback)(void *priv, int diff);
};
// *INDENT-ON*

struct audio_dac_fade {
    u8 enable;
    volatile u8 ref_L_gain;
    volatile u8 ref_R_gain;
    int timer;
};

struct audio_dac_sync_node {
    void *hdl;
    struct list_head entry;
};

struct audio_dac_hdl {
    struct analog_module analog;
    const struct dac_platform_data *pd;
    OS_SEM sem;
    struct audio_dac_trim trim;
    void (*fade_handler)(u8 left_gain, u8 right_gain);
    volatile u8 mute;
    volatile u8 state;
    volatile u8 agree_on;
    u8 gain;
    u8 vol_l;
    u8 vol_r;
    u8 channel;
    u8 bit24_mode_en;
    u16 max_d_volume;
    u16 d_volume[2];
    u32 sample_rate;
    u16 start_ms;
    u16 delay_ms;
    u16 start_points;
    u16 delay_points;
    u16 prepare_points;//未开始让DAC真正跑之前写入的PCM点数
    u16 irq_points;
    s16 protect_time;
    s16 protect_pns;
    s16 fadein_frames;
    s16 fade_vol;
    u8 protect_fadein;
    u8 vol_set_en;

    u8 sound_state;
    unsigned long sound_resume_time;
    s16 *output_buf;
    u16 output_buf_len;
    u8 anc_toggle;
    u8 anc_dac_gain_l;
    u8 anc_dac_gain_r;
    u8 anc_dac_open;
    u8 dsm_sel;
    struct audio_dac_sync sync;
    struct list_head sync_list;
    void *feedback_priv;
    void (*underrun_feedback)(void *priv);
};



int audio_dac_init(struct audio_dac_hdl *dac, const struct dac_platform_data *pd);

void audio_dac_set_capless_DTB(struct audio_dac_hdl *dac, u32 dacr32);

void audio_dac_avdd_level_set(struct audio_dac_hdl *dac, u8 level);

void audio_dac_lpf_level_set(struct audio_dac_hdl *dac, u8 level);

int audio_dac_do_trim(struct audio_dac_hdl *dac, struct audio_dac_trim *dac_trim, u8 fast_trim);

int audio_dac_set_trim_value(struct audio_dac_hdl *dac, struct audio_dac_trim *dac_trim);

int audio_dac_set_delay_time(struct audio_dac_hdl *dac, int start_ms, int max_ms);

void audio_dac_irq_handler(struct audio_dac_hdl *dac);

int audio_dac_set_buff(struct audio_dac_hdl *dac, s16 *buf, int len);

int audio_dac_write(struct audio_dac_hdl *dac, void *buf, int len);

int audio_dac_get_write_ptr(struct audio_dac_hdl *dac, s16 **ptr);

int audio_dac_update_write_ptr(struct audio_dac_hdl *dac, int len);

int audio_dac_set_sample_rate(struct audio_dac_hdl *dac, int sample_rate);

int audio_dac_get_sample_rate(struct audio_dac_hdl *dac);

int audio_dac_set_channel(struct audio_dac_hdl *dac, u8 channel);

int audio_dac_get_channel(struct audio_dac_hdl *dac);

int audio_dac_set_digital_vol(struct audio_dac_hdl *dac, u16 vol);

int audio_dac_set_analog_vol(struct audio_dac_hdl *dac, u16 vol);

int audio_dac_ch_set_analog_vol(struct audio_dac_hdl *dac, u8 channel, u16 vol);

int audio_dac_start(struct audio_dac_hdl *dac);

int audio_dac_stop(struct audio_dac_hdl *dac);

int audio_dac_idle(struct audio_dac_hdl *dac);

void audio_dac_mute(struct audio_dac_hdl *hdl, u8 mute);

int audio_dac_open(struct audio_dac_hdl *dac);

int audio_dac_close(struct audio_dac_hdl *dac);

int audio_dac_mute_left(struct audio_dac_hdl *dac);

int audio_dac_mute_right(struct audio_dac_hdl *dac);

int audio_dac_set_volume(struct audio_dac_hdl *dac, u8 gain);

int audio_dac_set_L_digital_vol(struct audio_dac_hdl *dac, u16 vol);

int audio_dac_set_R_digital_vol(struct audio_dac_hdl *dac, u16 vol);

void audio_dac_ch_mute(struct audio_dac_hdl *dac, u8 ch, u8 mute);

void audio_dac_zero_detect_onoff(struct audio_dac_hdl *dac, u8 onoff);

void audio_dac_set_fade_handler(struct audio_dac_hdl *dac, void *priv, void (*fade_handler)(u8, u8));

int audio_dac_sound_reset(struct audio_dac_hdl *dac, u32 msecs);

int audio_dac_set_bit_mode(struct audio_dac_hdl *dac, u8 bit24_mode_en);

int audio_dac_get_status(struct audio_dac_hdl *dac);

u8 audio_dac_init_status(void);

u8 audio_dac_is_working(struct audio_dac_hdl *dac);

int audio_dac_set_irq_time(struct audio_dac_hdl *dac, int time_ms);

int audio_dac_data_time(struct audio_dac_hdl *dac);

void audio_dac_anc_set(struct audio_dac_hdl *dac, u8 toggle);

int audio_dac_irq_enable(struct audio_dac_hdl *dac, int time_ms, void *priv, void (*callback)(void *));

void audio_dac_output_enable(struct audio_dac_hdl *dac);

void audio_dac_output_disable(struct audio_dac_hdl *dac);

int audio_dac_set_protect_time(struct audio_dac_hdl *dac, int time, void *priv, void (*feedback)(void *));

int audio_dac_buffered_frames(struct audio_dac_hdl *dac);

void audio_dac_add_syncts_handle(struct audio_dac_hdl *dac, void *syncts);

void audio_dac_remove_syncts_handle(struct audio_dac_hdl *dac, void *syncts);
/*
 * 音频同步
 */
void audio_dac_set_input_correct_callback(struct audio_dac_hdl *dac,
        void *priv,
        void (*callback)(void *priv, int diff));

int audio_dac_set_sync_buff(struct audio_dac_hdl *dac, void *buf, int len);

int audio_dac_set_sync_filt_buff(struct audio_dac_hdl *dac, void *buf, int len);

int audio_dac_sync_open(struct audio_dac_hdl *dac);

int audio_dac_sync_set_channel(struct audio_dac_hdl *dac, u8 channel);

int audio_dac_sync_set_rate(struct audio_dac_hdl *dac, int in_rate, int out_rate);

int audio_dac_sync_auto_update_rate(struct audio_dac_hdl *dac, u8 on_off);

int audio_dac_sync_flush_data(struct audio_dac_hdl *dac);

int audio_dac_sync_fast_align(struct audio_dac_hdl *dac, int in_rate, int out_rate, int fast_output_points, float phase_diff);

#if SYNC_LOCATION_FLOAT
float audio_dac_sync_pcm_position(struct audio_dac_hdl *dac);
#else
u32 audio_dac_sync_pcm_position(struct audio_dac_hdl *dac);
#endif

int audio_dac_sync_keep_rate(struct audio_dac_hdl *dac, int points);

int audio_dac_sync_pcm_input_num(struct audio_dac_hdl *dac);

void audio_dac_sync_input_num_correct(struct audio_dac_hdl *dac, int num);

void audio_dac_set_sync_handler(struct audio_dac_hdl *dac, void *priv, int (*handler)(void *priv, u8 state));

int audio_dac_sync_start(struct audio_dac_hdl *dac);

int audio_dac_sync_stop(struct audio_dac_hdl *dac);

int audio_dac_sync_reset(struct audio_dac_hdl *dac);

int audio_dac_sync_data_lock(struct audio_dac_hdl *dac);

int audio_dac_sync_data_unlock(struct audio_dac_hdl *dac);

void audio_dac_sync_close(struct audio_dac_hdl *dac);




u32 local_audio_us_time_set(u16 time);

int local_audio_us_time(void);

int audio_dac_start_time_set(void *_dac, u32 us_timeout, u32 cur_time, u8 on_off);

u32 audio_dac_sync_pcm_total_number(void *_dac);

void audio_dac_sync_set_pcm_number(void *_dac, u32 output_points);

u32 audio_dac_pcm_total_number(void *_dac, int *pcm_r);

u8 audio_dac_sync_empty_state(void *_dac);

void audio_dac_sync_empty_reset(void *_dac, u8 state);

void audio_dac_set_empty_handler(void *_dac, void *empty_priv, void (*handler)(void *priv, u8 empty));

void audio_dac_set_dcc(u8 dcc);

/*关闭audio相关模块使能*/
void audio_disable_all(void);

int audio_dac_sample_rate_select(struct audio_dac_hdl *dac, u32 sample_rate, u8 high);


int audio_dac_get_hrp(struct audio_dac_hdl *dac);
#endif

