#ifndef _EQ_API_H_
#define _EQ_API_H_

#include "typedef.h"
#include "asm/hw_eq.h"
#define EQ_CHANNEL_MAX      2

#define EQ_SR_IDX_MAX       9


/*eq type*/
typedef enum {
    EQ_MODE_NORMAL = 0,
    EQ_MODE_ROCK,
    EQ_MODE_POP,
    EQ_MODE_CLASSIC,
    EQ_MODE_JAZZ,
    EQ_MODE_COUNTRY,
    EQ_MODE_CUSTOM,//自定义
    EQ_MODE_MAX,
} EQ_MODE;

/*eq type*/
typedef enum {
    EQ_TYPE_FILE = 0x01,
    EQ_TYPE_ONLINE,
    EQ_TYPE_MODE_TAB,
} EQ_TYPE;

#define audio_eq_filter_info 	eq_coeff_info

typedef int (*audio_eq_filter_cb)(void *eq, int sr, struct audio_eq_filter_info *info);

struct audio_eq_param {
    u32 channels : 2;    //通道数
    u32 online_en : 1;   //是否支持在线调试  1:支持 0：不支持
    u32 mode_en : 1;     //写1
    u32 remain_en : 1;   //写1
    u32 no_wait : 1;     //是否使能异步eq, 1:使能  0：不使能
    u32 max_nsection : 6;//最大的eq段数,根据使用填写，要小于等于EQ_SECTION_MAX
    u32 nsection : 6;    //实际需要的eq段数，需小于等于max_nsection
    u32 drc_en: 1;       //16bit drc时，使用硬件eq对分频器部分进行加速，目前默认使用32bit drc。
    u8 out_32bit : 1;        //是否支持32bit eq输出，仅在 no_wait 写1时，out_32bit 才能写1
    audio_eq_filter_cb cb;//获取eq系数的回调函数
    u32 eq_name;         //eq名字，在线调试时，用于区分不同eq更新系数 播歌一般写song_eq_mode
    u32 sr;              //采样率，更根据当前数据实际采样率填写

    void *priv;
    int (*output)(void *priv, void *data, u32 len);
    void (*irq_callback)(void *priv);
};

#ifdef CONFIG_EQ_SUPPORT_ASYNC
struct audio_eq_async {
    u16 ptr;
    u16 len;
    u16 buf_len;
    u16 clear : 1;
    u16 out_stu : 1;
    char *buf;
};
#endif

struct audio_eq {
    void *eq_ch;
    u32 sr;
    u32 remain_flag : 1;
    u32 updata : 1;
    u32 online_en : 1;
    u32 mode_en : 1;
    u32 remain_en : 1;
    u32 start : 1;
    u32 max_nsection : 6;
    u32 check_hw_running : 1;
    u32 eq_name;

#ifdef CONFIG_EQ_SUPPORT_ASYNC
    void *run_buf;
    void *run_out_buf;
    int run_len;
    struct audio_eq_async async;
#endif

// #if AUDIO_EQ_CLEAR_MEM_BY_MUTE_TIME_MS
    u32 mute_cnt_l;
    u32 mute_cnt_r;
    u32 mute_cnt_max;
// #endif

    audio_eq_filter_cb cb;
    void *output_priv;
    int (*output)(void *priv, void *data, u32 len);

    struct eq_seg_info *eq_seg_tab;
    int *eq_coeff_tab;//高低音系数表
    void *entry;//记录上层流控的 entry,用于eq中断唤醒流控
};


void audio_eq_init(void);

int audio_eq_open(struct audio_eq *eq, struct audio_eq_param *param);

void audio_eq_set_output_handle(struct audio_eq *eq, int (*output)(void *priv, void *data, u32 len), void *output_priv);

void audio_eq_set_output_buf(struct audio_eq *eq, s16 *buf, u32 len);
void audio_eq_set_samplerate(struct audio_eq *eq, int sr);
void audio_eq_set_channel(struct audio_eq *eq, u8 channel);

// 检测到硬件正在运行时不等待其完成，直接返回
// 仅异步EQ有效
int audio_eq_set_check_running(struct audio_eq *eq, u8 check_hw_running);
int audio_eq_set_info(struct audio_eq *eq, u8 channels, u8 out_32bit);
int audio_eq_start(struct audio_eq *eq);
int audio_eq_run(struct audio_eq *eq, s16 *data, u32 len);
int audio_eq_close(struct audio_eq *eq);

extern void eq_app_run_check(struct audio_eq *eq);


#ifdef CONFIG_EQ_SUPPORT_ASYNC
void audio_eq_async_data_clear(struct audio_eq *eq);
#endif

int audio_eq_data_len(struct audio_eq *eq);

#endif

