/*****************************************************************
>file name : include_lib/media/audio_dec_server.h
>author : lichao
>create time : Mon 19 Nov 2018 08:45:49 PM CST
*****************************************************************/

#ifndef _AUDIO_DECODE_SERVER_H_
#define _AUDIO_DECODE_SERVER_H_

#include "media/avmodule.h"
#include "audio_dev.h"
#include "fs/fs.h"

enum {
    AUDIO_PLAY_EVENT_CURR_TIME = 0x20,
    AUDIO_PLAY_EVENT_END,
    AUDIO_PLAY_EVENT_ERR,
    AUDIO_PLAY_EVENT_SUSPEND,
    AUDIO_PLAY_EVENT_RESUME,
};

#define AUDIO_REQ_DEC                  0
#define AUDIO_REQ_GET_INFO             1
#define AUDIO_REQ_DEC_AUTO_MUTE        2
#define AUDIO_REQ_GET_BREAKPOINT       3
#define AUDIO_REQ_WIRELESS_SYNC        4

#define AUDIO_DEC_OPEN                 0
#define AUDIO_DEC_START                1
#define AUDIO_DEC_PAUSE                2
#define AUDIO_DEC_STOP                 3
#define AUDIO_DEC_FF                   4
#define AUDIO_DEC_FR                   5
//#define AUDIO_DEC_GET_BREAKPOINT       6
#define AUDIO_DEC_PP                   7
#define AUDIO_DEC_SET_VOLUME           8
#define AUDIO_DEC_PACKET               9
#define AUDIO_DEC_SYNC_OPEN            10
#define AUDIO_DEC_CH_SWITCH            11
#define AUDIO_DEC_GET_STATUS           12

#define AUTO_MUTE_OPEN                 0
#define AUTO_MUTE_GET_STATE            1
#define AUTO_MUTE_CLOSE                2


#define AUDIO_OUTPUT_ORIG_CH           0 //按原始声道输出
#define AUDIO_OUTPUT_STEREO            1 //按立体声
#define AUDIO_OUTPUT_L_CH              2 //只输出原始声道的左声道
#define AUDIO_OUTPUT_R_CH              3 //只输出原始声道的右声道
#define AUDIO_OUTPUT_MONO_LR_CH        4 //输出左右合成的单声道

struct audio_dec_breakpoint {
    int len;
    u32 fptr;
    u8 data[128];
};

struct audio_stream_info {
    u8 channel;
    u8 name_code; /* 0:ansi, 1:unicode_le, 2:unicode_be*/
    int sample_rate;
    int bit_rate;
    int total_time;
};

struct audio_vfs_ops {
    void *(*fopen)(const char *path, const char *mode);
    int (*fread)(void *file, void *buf, u32 len);
    int (*fwrite)(void *file, void *buf, u32 len);
    int (*fseek)(void *file, u32 offset, int seek_mode);
    int (*ftell)(void *file);
    int (*flen)(void *file);
    int (*fclose)(void *file);
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
};

#define PACKET_EVENT_DEC_SUSPEND    1
#define PACKET_EVENT_DEC_RESUME     2

#define DEC_EVENT_AUTO_MUTE         0x10
#define DEC_EVENT_AUTO_UNMUTE       0x11



struct audio_packet_ops {
    int (*get_packet)(void *priv, struct audio_packet_buffer *);
    int (*put_packet)(void *priv, struct audio_packet_buffer *);
    int (*query)(void *priv, struct audio_packet_stats *);
    int (*fetch)(void *priv, struct audio_packet_buffer *);
    int (*event_handler)(void *priv, int event);
    void (*close)(void *priv);
};

struct audio_buf_ops {
    void *(*get_buf)(void *priv, u32 *len);
    void (*put_buf)(void *priv, void *buf, u32 len, u8 repair);
};

#define AUDIO_DEC_ORIG_CH       0
#define AUDIO_DEC_L_CH          1
#define AUDIO_DEC_R_CH          2
#define AUDIO_DEC_MONO_LR_CH    3

struct audio_decoder_info {
    FILE *file;
    void *priv;
    u8 output_channel;
    u8 channels;
    u32 sample_rate;
    u32 bit_rate;
    struct audio_dec_breakpoint *bp;
    const struct audio_buf_ops        *io_ops;
    const struct audio_vfs_ops        *vfs_ops;
};

struct stream_codec_info {
    int  time;
    int  frame_num;
    u32  frame_len;
    int  frame_points;
    int  sequence_number;
    u32  sample_rate;
    u8   channel;
};

struct audio_decoder_ops {
    const char *name;
    void *(*open)(struct audio_decoder_info *info);
    void *(*try_open)(struct audio_decoder_info *info);
    int (*get_audio_info)(void *, struct audio_stream_info *info);
    int (*get_play_time)(void *);
    int (*fast_forward)(void *, int step_s);
    int (*fast_rewind)(void *, int step_s);
    int (*get_breakpoint)(void *, struct audio_dec_breakpoint *);
    int (*channel_switch)(void *, u8);
    int (*stream_info_scan)(void *, struct stream_codec_info *info, void *data, int len);
    int (*start)(void *, void *, u32);
    int (*stop)(void *);
    int (*close)(void *);
};

struct rt_stream_info {
    u8  noblock;
    u16 seqn;
    u16 frame_num;
    u32 data_len;
    u32 remain_len;
};

#define AUDIO_SYNC_EVENT_PREPARE            0
#define AUDIO_SYNC_EVENT_STREAM_FREE        1
#define AUDIO_SYNC_EVENT_STREAM_CLEAN       2
#define AUDIO_SYNC_EVENT_STREAM_SUSPEND     3
#define AUDIO_SYNC_EVENT_QUERY_AND_FREE     4
#define AUDIO_SYNC_EVENT_QUERY              5
// *INDENT-OFF*
struct wireless_sync_ops {
    void *(*open)(void);
    void (*set_handler)(void *, void *priv,
                        void (*handler)(void *, int *, int));
    u32 (*time)(u8 type, u32 time, int *ret_time, int (*local_us_time)(void));
    int (*send)(void *, void *buf, u32 len);
    int (*master)(u8 type);
    u8  (*online)(u8);
    int (*stream_event_handler)(u8 event, void *param);
    void (*close)(void *);
};

// *INDENT-ON*

struct audio_dec_play {
    u8 cmd;
    u8 status;
    u8 volume;   /*音量*/
    u8 ff_fr_step;
    unsigned int channel : 4;
    unsigned int priority : 4; /*解码优先级选择, 多个解码服务时必须要分配好*/
    unsigned int ch_mode : 4;  /*声道输出模式*/
    unsigned int auto_dec : 1; /*自动解码 - 调用层实现vfs ops或使用默认，dec start后自动读取-解码-播放*/
    unsigned int stereo_sync : 1; /*无线立体(tws)同步*/
    unsigned int eq_en : 1;       /*EQ均衡器使能开关*/
    unsigned int fade_en : 1;    /* audio音量淡入淡出使能 */
    unsigned int recovery : 1; /*修复：0 - 静音， 1 - 上一帧*/
    unsigned int mix : 1;
    unsigned int soft_limiter : 1;
    unsigned int noise_limiter : 1;
    unsigned int reserved : 12;
    u32 output_buf_len; /*解码 —> 播放缓冲buffer长度*/
    u32 sample_rate;
    u32 bit_rate;
    u32 total_time;
    void *output_buf;
    FILE *file;
    const char *format;
    const char *try_format;
    struct audio_dec_breakpoint *bp;
    const struct audio_vfs_ops  *vfs_ops; /*自动播放时的vfs操作集合*/
    void (*rt_event_handler)(int *msg, int len);
};


#define WL_PROTOCOL_RTP     0
#define WL_PROTOCOL_SCO     1
#define WL_PROTOCOL_FILE    2
/*
 * 音频（无线）立体同步
 */
struct audio_wireless_sync {
    u8 cmd;
    u8 top_percent; //上限比例
    u8 start_percent; //启动比例限值
    u8 bottom_percent; //下限比例
    u8 protocol; //音频包含协议
    u32 time_before_dec;
    u32 together_time;
    u32 buffer_size;
    const struct wireless_sync_ops *ops;
};

struct audio_auto_mute {
    u8 state;
    u8 mute;
    u8 channel;
    u8 pcm_mute;
    u16 mute_energy;
    u16 filt_points;
    u16 filt_number;
    void (*event_handler)(u8 event, u8 channel);
};

union audio_dec_req {
    struct audio_dec_play play;
    struct audio_stream_info info;
    struct audio_dec_breakpoint bp;
    struct audio_wireless_sync sync;
    struct audio_auto_mute auto_mute;
};


#define REGISTER_AUDIO_DECODER(ops) \
        const struct audio_decoder_ops ops SEC(.audio_decoder)


extern const struct audio_decoder_ops audio_decoder_begin[];
extern const struct audio_decoder_ops audio_decoder_end[];

#define list_for_each_audio_decoder(p) \
    for (p = audio_decoder_begin; p < audio_decoder_end; p++)



#endif
