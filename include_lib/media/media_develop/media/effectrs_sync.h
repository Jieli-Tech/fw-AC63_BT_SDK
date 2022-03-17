#ifndef EFFECTRS_SYNC_H
#define EFFECTRS_SYNC_H

#include "generic/typedef.h"
#include "generic/list.h"

#define WL_PROTOCOL_RTP     0
#define WL_PROTOCOL_SCO     1
#define WL_PROTOCOL_FILE    2

#define SYNC_ERR_NONE               0
#define SYNC_ERR_PLAY_REJECTED      1
#define SYNC_ERR_PAUSE              2
#define SYNC_ERR_DISCARD_PKT        3
#define SYNC_ERR_PLAY_PREPARE       4
#define SYNC_ERR_NULL_PKT           5
#define SYNC_ERR_GRAB_AUDIO_STREAM  6
#define SYNC_ERR_DEC_RESET          7
enum {
    SYNC_DAC_CTL = 1,
    SYNC_GET_PCM_TOTAL,
    SYNC_GET_SRC_IN_LEN,
    SYNC_GET_SRC_STATE,
    SYNC_SET_PLAY_O_POINT,
    SYNC_WITE_DAC_MUTE_DATE,
    SET_LOCAL_US_TIME,
    SET_SYNC_RESET,
    SYNC_SWITCH_N_CH,
};
struct audio_packet_buffer {
    u8 	noblock;
    u8  state;
    u16 timeout;
    u32 slot_time;
    u32 len;
    u32 baddr;
};
struct audio_packet_stats {
    u32 total_size;
    u32 data_size;
    u32 remain_size;
    u8 ch;
    u8 grab_audio_stream_info_num;
};
typedef struct _SYNC_POINTER_ {
    volatile int *src_in_len;
    volatile int *set_sr_in;
    volatile int *set_sr_out;
    volatile int *more_less_points;

    volatile int *play_point;
    volatile int *fake_n_points;
} CPUSYNC_POINTERS;

/*
 * 播放动态src配置，start为配置一次变点
 */
struct audio_pcm_src {
    u8  convert; //启动转换
    u8  resample;//设置为变采样
    u16 ratio_i; //转换输入
    u16 ratio_o; //转换输出
};

/*
 * 同步需要的DAC协定启动配置
 */
struct audio_dac_ctl {
    u8  agreement; //
    u8  on; //启动/暂停
    // u8  time_unit; //时间单位（0 - us, 1 - ms, 2 - s)
    u32 pre_time;
    u32 time; //时间点(us)
};
// *INDENT-OFF*
struct sync_ops_t {
    void *(*open)(void);
    void (*set_handler)(void *, void *priv,
                        void (*handler)(void *, int *, int));
    u32 (*time)(u8 type, u32 time,u32 *pre_time);
    int (*send)(void *, void *buf, u32 len);
    int (*master)(u8 type);
    u8  (*online)(u8);
    void (*role_lock)(void *, u8);
    void (*close)(void *);
    void *(*audio_sync_open)(void *empty_priv,void (*empty_handler)(void *, u8),u8 protocol,u16 ch,u16 sr,int (*sync_output)(void *priv, void *data, int len),void*);
    int (*audio_sync_close)(void *sync);
    int (*audio_sync_run)(void *_sync,void *buf, int points_per_channel);
    int (*audio_sync_set_samplerate)(void *sync,int sr_in,int sr_out);
	int (*audio_sync_control)(void* _priv,int cmd,u32 arg);
};

struct audio_sync_parm {
    u8 channel;
    u8 top_percent; //上限比例
    u8 start_percent; //启动比例限值
    u8 bottom_percent; //下限比例
    u8 protocol; //音频包含协议
    u32 buffer_size;
	u8 sync_time;
    const struct sync_ops_t *ops;
	void *audio_dev;
    int (*sync_output)(void *priv, void *data, int len);
};
// *INDENT-ON*

extern void *audio_decoder_sync_open(struct audio_decoder *dec, struct audio_sync_parm *sync_parm);
extern void audio_decoder_sync_close(void *c);
extern int audio_decodr_sync_start(void *c, struct audio_packet_stats *stats, struct audio_packet_buffer *pkt);
extern int audio_decoder_sync_run(void *c, void *buf, int len);
extern int audio_decoder_sync_do(void *c, s16 *buf, int len);
extern int audio_sync_reset(void *c);
int audio_sync_set_remain_len(void *c, int len);

#if (defined CONFIG_CPU_BR26 || \
	 defined CONFIG_CPU_BR23 || \
     defined CONFIG_CPU_BR25 || \
     defined CONFIG_CPU_BR30 || \
     defined CONFIG_CPU_BR34 || \
	 defined CONFIG_CPU_BR18 || \
	 defined CONFIG_CPU_BR28 || \
	 defined CONFIG_CPU_BR36 || \
     defined CONFIG_CPU_BR27)
/*
 * BR26同步
 */
#define SYNC_ERR_NONE               0
#define SYNC_ERR_DEC_NOT_ALLOWED    1
#define SYNC_ERR_TIMEOUT            2
#define SYNC_ERR_STREAM_PASS        3
#define SYNC_ERR_STREAM_END         4
#define SYNC_ERR_PREPARE            5
#define SYNC_ERR_ALIGNED_AND_PASS   6
#define SYNC_ERR_STREAM_RESET       7

/***************** the step mean to get more or less  (sample_rate/step) points/sencond ********************/
#define SRC_DEC_STEP         2     //step to speed down
#define SRC_INC_STEP         2     // step  to speed up

#define SRC_POINT_AMPLIPY          5

#define AUDIO_SYNC_TARGET_DAC   0
#define AUDIO_SYNC_TARGET_FM    1
#define AUDIO_SYNC_TARGET_IIS   2

#define RX_DELAY_NULL           0 //延时未抖动
#define RX_DELAY_UP             1 //延时上浮
#define RX_DELAY_DOWN           2 //延时下浮

struct rt_stream_info {
    u8 noblock;
    u8 rx_delay;
    u16 seqn;
    u16 frame_num;
    u32 data_len;
    u32 remain_len;
    void *baddr;
    int len;
};

struct remote_stream_info {
    u8 rx_delay; //音频接收数据流延时波动
    u16 seqn;    //音频数据包号信息
    u32 remain_len; //蓝牙接收buffer剩余空间
};

// *INDENT-OFF*
struct audio_tws_conn_ops {
    void *(*open)(void);
    void (*set_handler)(void *, void *priv,
                        void (*handler)(void *, int *, int));
    u32 (*time)(u8 type, u32 time, u32 *pre_time);
    int (*send)(void *, void *buf, u32 len);
    int (*master)(u8 type);
    u8  (*online)(u8);
    void (*close)(void *);
};

struct audio_wireless_sync_info {
    u8 channel;
    u8 target;  //本地目标设备
    u8 protocol; //音频同步协议类型
    u8 reset_enable; //内部检测异常后返回复位信息的使能
    u16 sample_rate; //解码采样率
    u16 output_rate; //输出采样率
    void *dev;
    void *sample_ch; //同步采样通道
    const struct audio_tws_conn_ops *tws_ops; //TWS相关的调用接口
};
// *INDENT-ON*

struct audio_wireless_sync {
    struct audio_stream_entry *entry;
    struct audio_stream_entry *resample_entry;
    void *context;
    void *sample_sync;
};

void *audio_wireless_sync_open(struct audio_wireless_sync_info *info, struct audio_stream_entry **entry);

int audio_wireless_sync_add_output(void *c, struct audio_wireless_sync_info *info);

int audio_wireless_sync_add_dev(void *c, struct audio_wireless_sync_info *info);

int audio_wireless_sync_probe(void *c, struct rt_stream_info *info);

void audio_wireless_sync_info_init(void *c, u32 sample_rate, u32 output_rate, u8 channel);

int audio_sync_set_tws_together(void *c, u8 together, u32 together_time);

int audio_wireless_sync_after_dec(void *c, s16 *data, int len);

int audio_wireless_sync_get_rate(void *c, u16 *input_rate, u16 *output_rate);

int audio_wireless_sync_stop(void *c);

int audio_wireless_sync_reset(void *c);

int audio_wireless_sync_suspend(void *c);

void audio_wireless_sync_close(void *c);

int audio_wireless_sync_add_output(void *c, struct audio_wireless_sync_info *info);

int audio_wireless_sync_with_stream(void *c, struct remote_stream_info *info);

int audio_wireless_sync_drop_samples(void *c, int samples);

int audio_wireless_sync_set_tws_time(void *c, int time);

int audio_wireless_sync_is_working(void *c);

int audio_wireless_sync_sound_reset(void *c, int time);

int audio_wireless_sync_sound_fade_in(void *c, u16 time);
#endif
/*
#if (defined CONFIG_CPU_BR26 || \
	(defined CONFIG_CPU_BR23 || \
     defined CONFIG_CPU_BR25)
*/
#endif
