#ifndef _EQ_API_H_
#define _EQ_API_H_

#include "typedef.h"
// #include "asm/audio_platform.h"
#include "asm/hw_eq.h"
// #include "app_config.h"
#include "spinlock.h"

#define EQ_CHANNEL_MAX      2

/* #ifndef EQ_SECTION_MAX */
// #define EQ_SECTION_MAX      10
/* #endif */

#define EQ_SECTION_MAX_DEFAULT   10
// #define EQ_SR_IDX_MAX       9

/* #define AUDIO_EQ_CLEAR_MEM_BY_MUTE_TIME_MS		0//300 //连续多长时间静音就清除EQ MEM */
/* #define AUDIO_EQ_CLEAR_MEM_BY_MUTE_LIMIT		0 //静音判断阀值 */

/* #define song_mode         0 */
// #define call_mode         1
// #define call_narrow_mode  2
// #define aec_mode          3
/* #define aec_narrow_mode   4 */

#define AUDIO_SONG_EQ_NAME  0
// #define AUDIO_FR_EQ_NAME    1
// #define AUDIO_RL_EQ_NAME    2
/* #define AUDIO_RR_EQ_NAME    3 */
#define AUDIO_CALL_EQ_NAME 1
#define AUDIO_AEC_EQ_NAME  3

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

typedef int (*audio_eq_filter_cb)(int sr, struct audio_eq_filter_info *info);


/*EQ_ONLINE_CMD_PARAMETER_SEG*/
typedef struct eq_seg_info EQ_ONLINE_PARAMETER_SEG;

/*-----------------------------------------------------------*/
typedef struct eq_seg_info EQ_CFG_SEG;


struct audio_eq_param {
    u32 channels : 2;
    u32 online_en : 1;
    u32 mode_en : 1;
    u32 remain_en : 1;
    u32 no_wait : 1;
    u32 max_nsection : 6;
    u32 reserved : 20;

    u32 nsection : 6;
    u32 drc_en: 1;
    u32 eq_name;
    audio_eq_filter_cb cb;
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
    u32 sr : 16;
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
};


void audio_eq_init(int eq_section_num);

int audio_eq_open(struct audio_eq *eq, struct audio_eq_param *param);

void audio_eq_set_output_handle(struct audio_eq *eq, int (*output)(void *priv, void *data, u32 len), void *output_priv);

void audio_eq_set_samplerate(struct audio_eq *eq, int sr);
void audio_eq_set_channel(struct audio_eq *eq, u8 channel);

// 检测到硬件正在运行时不等待其完成，直接返回
// 仅异步EQ有效
int audio_eq_set_check_running(struct audio_eq *eq, u8 check_hw_running);

int audio_eq_set_info(struct audio_eq *eq, u8 channels, u8 out_32bit);

int audio_eq_start(struct audio_eq *eq);
int audio_eq_run(struct audio_eq *eq, s16 *data, u32 len);
int audio_eq_close(struct audio_eq *eq);


#ifdef CONFIG_EQ_SUPPORT_ASYNC
void audio_eq_async_data_clear(struct audio_eq *eq);
#endif

int audio_eq_data_len(struct audio_eq *eq);

extern void eq_app_run_check(struct audio_eq *eq);

int eq_get_filter_info(int sr, struct audio_eq_filter_info *info);
int aec_ul_eq_filter(int sr, struct audio_eq_filter_info *info);
int eq_phone_get_filter_info(int sr, struct audio_eq_filter_info *info);


void eq_section_num_set(u8 song, u8 call_16k, u8 call_8k, u8 aec_16k, u8 aec_8k);

#endif

