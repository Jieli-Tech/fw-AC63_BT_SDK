#ifndef AUDIO_DEV_H
#define AUDIO_DEV_H

#include "generic/list.h"
#include "typedef.h"
#include "device/device.h"


#define AUDIOC_QUERYCAP             _IOR('A', 0, sizeof(struct audio_capability))
#define AUDIOC_GET_FMT              _IOR('A', 1, sizeof(struct audio_format))
#define AUDIOC_SET_FMT              _IOW('A', 1, sizeof(struct audio_format))
#define AUDIOC_REQBUFS              _IOR('A', 2, sizeof(int))
#define AUDIOC_DQBUF                _IOR('A', 3, sizeof(int))
#define AUDIOC_QBUF                 _IOR('A', 4, sizeof(int))
#define AUDIOC_STREAM_ON            _IOR('A', 5, sizeof(int))
#define AUDIOC_STREAM_OFF           _IOR('A', 6, sizeof(int))
#define AUDIOC_PLAY                 _IOR('A', 7, sizeof(int))
//#define AUDIOC_PLAY               _IOR('A', 7, sizeof(int))
#define AUDIOC_GET_S_ATTR           _IOR('A', 8, sizeof(struct audio_dev_attr))
#define AUDIOC_SET_S_ATTR           _IOW('A', 9, sizeof(struct audio_dev_attr))
#define AUDIOC_GET_D_ATTR           _IOR('A', 10, sizeof(struct audio_dev_attr))
#define AUDIOC_SET_D_ATTR           _IOW('A', 11, sizeof(struct audio_dev_attr))
#define AUDIOC_SET_SAMPLE_RATE      _IOW('A', 12, sizeof(u32))
#define AUDIOC_STREAM_ALLOC         _IOR('A', 13, sizeof(struct audio_buffer))
#define AUDIOC_PCM_GET_STATS        _IOR('A', 14, sizeof(struct audio_pcm_stats))
#define AUDIOC_PCM_RATE_CTL         _IOW('A', 15, sizeof(struct audio_pcm_src))
#define AUDIOC_DAC_PLAY_CTL         _IOW('A', 16, sizeof(struct audio_dac_ctl))
#define AUDIOC_DAC_SET_K_POINT      _IOR('A', 17, sizeof(int))
#define AUDIOC_DAC_CLEAR_ZEROCASE   _IOR('A', 18, sizeof(int))
#define AUDIOC_GET_SRC_STATS        _IOR('A', 19, sizeof(u8 *))
#define AUDIOC_STREAM_SIZE          _IOR('A', 20, sizeof(u32))



enum audio_sub_unit {
    AUDIO_UNIT_REC = 0x0,
    AUDIO_UNIT_PLAY,
    AUDIO_UNIT_EQ,
    AUDIO_UNIT_AEC,
    AUDIO_UNIT_MIXER,
    AUDIO_UNIT_SLIDER,
};

struct audio_dev_attr {
    enum audio_sub_unit unit;
    void *attr;
};

struct eq_s_attr {
    //TODO
    u8 channel;
    u8 nsection;
    u8 soft_sec_set : 1;
    u8 soft_sec_num : 3;
    u8 Reserved : 4;
    float gain;
    u32 sr;
    void *L_coeff;
    void *R_coeff;
    int (*set_sr_cb)(int sr, void **L_coeff, void **R_coeff);
    //eq limiter
    float AttackTime;
    float ReleaseTime;
    float Threshold;
};

struct aec_s_attr {
    u8 agc_en: 1;
    u8 ul_eq_en: 1;
    u8 wideband: 1;
    u8 toggle: 1;
    u8 wn_en: 1;
    u8 reserved: 3;

    u8 enablebit;
    u8 packet_dump;
    u8 SimplexTail;
    u16 AGC_max_gain;
    u16 AGC_min_gain;
    u16 AGC_threshold;
    u16 AGC_fade;
    u16 hw_delay_offset;
    u16 wn_gain;
    int SimplexThr;
    float ES_AggressFactor;
    float ES_MinSuppress;
    int *ul_eq_coeff;
    float ANS_AggressFactor;
    float ANS_MinSuppress;
    float EchoSupressRateThr;
};
/*
struct audio_rec_attr {
    enum audio_sub_unit unit;
    void *attr;
};
*/

struct audio_req_data {
    u8 channel;
    u8 *data;
    int len;
    int sample_rate;
};

struct audio_endpoint;

struct audio_platform_data {
    u8 type;
    void *private_data;
};



#define AUDIO_CAP_SAMPLING      0x00000001
#define AUDIO_CAP_MP3_ENC       0x00000002

struct audio_capability {
    u32 capabilities;
};


#if 0
#define AUDIO_FMT_PCM          0x01
#define AUDIO_FMT_SPEEX        0x02
#define AUDIO_FMT_AMR          0x03
#define AUDIO_FMT_AAC          0x04
#endif

struct audio_format {
    u8 volume;
    u8 channel;
    u8 kbps;
    u8 priority;
    u8 eq_on;
    u8 src_on;
    u8 mixer;
    u8 fade_on;
    union {
        u8 linein;
        struct {
            u8 out2dac : 1;
            u8 out_l : 1;
            u8 out_r : 1;
            u8 out_mix : 1;
            u8 cha_idx : 2;
        } ladc;
        u8 param;
    } u;
    int sample_rate;
    const char *format;
    const char *sample_source;
    void *private_data;
    void *aec_attr;
};

struct audio_reqbufs {
    void *buf;
    int size;
};

struct audio_buffer {
    u8 index;
    u8 noblock;
    u8 sem_post;
    u16 timeout;
    u32 time_msec;
    u32 len;
    u32 baddr;
    void *priv;
};

#define AUDIO_PLAY_ORIG_MODE        0x00
#define AUDIO_PLAY_MONO_L           0x01
#define AUDIO_PLAY_MONO_R           0x02
#define AUDIO_PLAY_MONO_LR          0x03
#define AUDIO_PLAY_STEREO           0x04

#define AUDIO_INPUT_WRITE_MODE  0
#define AUDIO_INPUT_QUEUE_MODE  1

struct audio_play_s_attr {
    u8 channel; //
    u8 input_mode; //输入pcm buffer方式
    u8 output_mode; //输出模式
};

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
    u8  time_unit; //时间单位（0 - us, 1 - ms, 2 - s)
    u16 points; //启动点数偏移
    u32 time; //时间点(us)
};

struct audio_pcm_mixer {
    u8  on;
    u16 sample_rate;
};

struct audio_pcm_stats {
    u8  clean;
    u16 empty_state;
    u32 kpoints;//当前pcm点总数(声道平均)
    u32 kpoints_r;//当前pcm点总数(声道平均)
    u32 rpoints;//未播点数
};

#define PLAY_STATE_OFF          0
#define PLAY_STATE_ON           1
#define PLAY_STATE_SUSPEND      2

struct audio_play_d_attr {
    u8 state;
    u16 volume;
};

struct audio_eq_d_attr {

};

enum {
    AUDIO_SYS_VOL = 0x00,
    AUDIO_ANA_VOL,
    AUDIO_DIG_VOL,
    AUDIO_MAX_VOL,
    AUDIO_MUTE_VOL,
    AUDIO_MIX_FRONT_VOL,//范围 : 0 - 100
    AUDIO_MIX_BACK_VOL, //范围 : 0 - 100
};

enum mute_value {
    AUDIO_MUTE_DEFAULT = 0,
    AUDIO_UNMUTE_DEFAULT,
    AUDIO_MUTE_L_CH,
    AUDIO_UNMUTE_L_CH,
    AUDIO_MUTE_R_CH,
    AUDIO_UNMUTE_R_CH,
};

struct audio_volume {
    u8  type;
    s16 value;
};

#if 0
struct audio_subdevice_ops {
    int (*init)(struct audio_platform_data *);

    int (*querycap)(struct audio_capability *cap);

    int (*get_format)(struct audio_format *);
    int (*set_format)(struct audio_format *);

    struct audio_endpoint *(*open)(struct audio_format *);

    int (*streamon)(struct audio_endpoint *);

    int (*streamoff)(struct audio_endpoint *);

    int (*response)(struct audio_endpoint *, int cmd, void *);

    int (*write)(struct audio_endpoint *, void *buf, u32 len);

    int (*close)(struct audio_endpoint *);

    int (*ioctl)(struct audio_endpoint *, u32 cmd, u32 arg);
};

struct audio_subdevice {
    u8 id;
    u8 type;
    u32 format;
    struct list_head entry;
    void *private_data;
    void *parent;

    /*struct audio_subdevice *next;*/
    /*void *parent;*/
    /*void *private_data;*/
    //const struct audio_subdevice_ops *ops;
};


struct audio_endpoint {
    struct list_head entry;
    struct audio_subdevice *dev;
    int inused;
    void *parent;
    void *private_data;
};


int audio_subdevice_request(struct audio_endpoint *ep, int req, void *arg);

void *audio_buf_malloc(struct audio_endpoint *ep, u32 size);


void *audio_buf_realloc(struct audio_endpoint *ep, void *buf, int size);


void audio_buf_stream_finish(struct audio_endpoint *ep, void *buf);

void audio_buf_free(struct audio_endpoint *ep, void *buf);

void audio_buf_set_time(struct audio_endpoint *ep, void *buf, u32 msec);


extern struct audio_subdevice audio_subdev_begin[];
extern struct audio_subdevice audio_subdev_end[];


#define REGISTER_AUDIO_SUBDEVICE(dev, _id) \
	const struct audio_subdevice dev sec(.audio_subdev.##_id) = { \
		.id = _id, \



#endif
extern const struct device_operations audio_dev_ops;

#define AUDIO_SYNC_RESAMPLE         0x1
#define AUDIO_SYNC_PCM_DATA         0x2
#define AUDIO_SYNC_SETTING          0x3

// *INDENT-OFF*
struct audio_sync_src_param {
    u16 rate_in;
    u16 rate_out;
    void *priv;
    int (*sync_before_resample)(void *priv, void *data, int len);
    void (*sync_resample_handler)(void *priv, int len, u32 rate_out, int back_delta,int phasev);
};

struct audio_sync_sub_module {
    u8 attr;
    void *(*open)(void *param);
    int (*set_rate)(void *, u16 input, u16 output, s16 delta);
    void (*reset_delta)(void *, u32 flag, u32 *pcm_cnt, u32 *phase);
    u32 (*get_pcm_count)(void *, u8 type);
    u32 (*set_pcm_count)(void *, u8 type);
    void (*close)(void *);
};
// *INDENT-ON*

#define AUDIO_SYNC_MODULE_REGISTER(name) \
    const struct audio_sync_sub_module name sec(.audio_sync_module)

extern struct audio_sync_sub_module audio_sync_module_begin[];
extern struct audio_sync_sub_module audio_sync_module_end[];

#define list_for_each_sync_module(p) \
    for (p = audio_sync_module_begin; p < audio_sync_module_end; p++)


#endif
