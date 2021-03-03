
#ifndef _AUDIO_DEC_APP_H_
#define _AUDIO_DEC_APP_H_

#include "asm/includes.h"
#include "media/includes.h"
#include "media/audio_stream.h"
#include "system/includes.h"
#include "channel_switch.h"

//////////////////////////////////////////////////////////////////////////////
#define AUDIO_DEC_APP_MASK			(('A' << 24) | ('D' << 16) | ('P' << 8) | ('M' << 8))

//////////////////////////////////////////////////////////////////////////////
enum {
    AUDIO_DEC_APP_STATUS_STOP = 0,
    AUDIO_DEC_APP_STATUS_PLAY,
    AUDIO_DEC_APP_STATUS_PAUSE,
};

enum {
    AUDIO_DEC_APP_EVENT_NULL = 0,
    AUDIO_DEC_APP_EVENT_DEC_PROBE,
    AUDIO_DEC_APP_EVENT_DEC_OUTPUT,	// param[0]:data, param[1]:len
    AUDIO_DEC_APP_EVENT_DEC_POST,
    AUDIO_DEC_APP_EVENT_DEC_STOP,
    AUDIO_DEC_APP_EVENT_START_INIT_OK,
    AUDIO_DEC_APP_EVENT_START_OK,
    AUDIO_DEC_APP_EVENT_START_ERR,
    AUDIO_DEC_APP_EVENT_DEC_CLOSE,
    AUDIO_DEC_APP_EVENT_PLAY_END,
    AUDIO_DEC_APP_EVENT_STREAM_OPEN,
    AUDIO_DEC_APP_EVENT_STREAM_CLOSE,
};

struct audio_dec_app_file_hdl {
    int (*read)(void *hdl, void *buf, u32 len);
    int (*seek)(void *hdl, int offset, int orig);
    int (*len)(void *hdl);
};

struct audio_dec_app_hdl {
    u32 mask;	// 固定为AUDIO_DEC_APP_MASK
    u32 id;		// 唯一标识符，随机值
    struct list_head list_entry;	// 链表
    struct audio_stream *stream;	// 音频流
    struct audio_decoder decoder;
    struct audio_res_wait wait;
    struct channel_switch *ch_switch;//声道变换
    struct audio_mixer_ch mix_ch;
    enum audio_channel ch_type;
    u32 status : 3;
    u32 ch_num : 4;		// channel通道数
    u32 out_ch_num : 4;	// 输出声道数
    u32 tmp_pause : 1;
    u32 dec_mix : 1;	// 1:叠加模式
    u32 remain : 1;
    u32 frame_type : 1;	// 1:frame格式；0:file格式
    u32 close_by_res_put : 1; //被打断就自动close

    u16 frame_pkt_len;	// frame格式帧长
    u16 frame_data_len;	// frame格式当前数据长度
    u8 *frame_buf;		// frame格式帧buf

    u32 dec_type;		// 指定解码格式，start后为实际解码格式
    u16 sample_rate;	// 指定采样率，start后为实际解码采样率
    u16 resume_tmr_id;

    struct audio_decoder_task 	*p_decode_task;
    struct audio_mixer 			*p_mixer;

    struct audio_stream_entry 	**entries;

    int (*evt_cb)(void *, int event, int *param); // 事件回调
    void *evt_priv;

    struct audio_dec_input dec_input;
    struct audio_dec_input *input;		// 指定使用input接口
    struct audio_dec_handler *handler;	// 指定使用handler接口

    void *file_hdl;
    struct audio_dec_app_file_hdl *file;

    int sync_confirm_time;	// tws同步时间，0:不同步
    void *sync;

    void *app_hdl;	// 指向上一级句柄
};

#define AUDIO_DEC_FILE_FLAG_AUDIO_STATE_TONE		(0)
#define AUDIO_DEC_FILE_FLAG_AUDIO_STATE_MUSIC		(1)
#define AUDIO_DEC_FILE_FLAG_AUDIO_STATE_MASK		(0x000000ff)

struct audio_dec_format_hdl {
    const char *fmt;	// 后缀名
    u32 coding_type;	// 解码类型
};

struct audio_dec_file_app_hdl {
    struct audio_dec_app_hdl *dec;
    struct audio_dec_format_hdl	*format;
    u32   flag;		// 标签，可用于设置audio通道等
    void *file_hdl;
    void *priv;		// 私有参数
};

struct audio_sine_param_head {
    u16 repeat_time;
    u8  set_cnt;
    u8  cur_cnt;
};

struct audio_sin_param {
    //int idx_increment;
    int freq;
    int points;
    int win;
    int decay;
};

#define AUDIO_DEC_SINE_APP_NUM_MAX		8
struct audio_dec_sine_app_hdl {
    struct audio_dec_app_hdl *dec;
    void *sin_maker;
    struct audio_sin_param sin_parm[AUDIO_DEC_SINE_APP_NUM_MAX];
    struct audio_sin_param *sin_src;
    u8	  sin_num;
    u8	  sin_repeat;
    u16   sin_default_sr;
    u32   sin_volume;
    u32   flag;
    void *file_hdl;
    void *priv;
};

struct audio_dec_stream_entries_hdl {
    struct audio_stream_entry **entries_addr;
    u16 entries_cnt;
    u16 entries_total;
};

//////////////////////////////////////////////////////////////////////////////
// 通过名字判断解码类型
u32 audio_dec_app_get_format_by_name(char *name, struct audio_dec_format_hdl *format);

// 创建一个dec_app解码。默认优先级是3，dec->wait.priority=3;
struct audio_dec_app_hdl *audio_dec_app_create(void *priv, int (*evt_cb)(void *, int event, int *param), u8 mix);
// 打开dec_app解码
int audio_dec_app_open(struct audio_dec_app_hdl *dec);
// 关闭dec_app解码
void audio_dec_app_close(struct audio_dec_app_hdl *dec);
// 设置dec_app文件信息
void audio_dec_app_set_file_info(struct audio_dec_app_hdl *dec, void *file_hdl);
// 设置dec_app流数据信息
void audio_dec_app_set_frame_info(struct audio_dec_app_hdl *dec, u16 pkt_len, u32 coding_type);
// dec_app解码暂停播放
int audio_dec_app_pp(struct audio_dec_app_hdl *dec);
// 获取dec_app解码状态。负数：错误
int audio_dec_app_get_status(struct audio_dec_app_hdl *dec);
// 检测一下hdl是否存在。true：存在
int audio_dec_app_check_hdl(struct audio_dec_app_hdl *dec);

// 创建一个文件解码
// 默认开启可抢断同优先级解码，hdl->dec->wait.snatch_same_prio=1;
// 开启被打断就自动close功能，hdl->dec->close_by_res_put=1;
struct audio_dec_file_app_hdl *audio_dec_file_app_create(char *name, u8 mix);
// 打开文件解码
int audio_dec_file_app_open(struct audio_dec_file_app_hdl *file_dec);
// 关闭文件解码
void audio_dec_file_app_close(struct audio_dec_file_app_hdl *file_dec);

// 创建一个正弦波文件解码
// 默认开启可抢断同优先级解码，hdl->dec->wait.snatch_same_prio=1;
// 开启被打断就自动close功能，hdl->dec->close_by_res_put=1;
struct audio_dec_sine_app_hdl *audio_dec_sine_app_create(char *name, u8 mix);
// 创建一个正弦波数组解码
struct audio_dec_sine_app_hdl *audio_dec_sine_app_create_by_parm(struct audio_sin_param *sin, u8 sin_num, u8 mix);
// 打开正弦波解码
int audio_dec_sine_app_open(struct audio_dec_sine_app_hdl *sine_dec);
// 关闭正弦波解码
void audio_dec_sine_app_close(struct audio_dec_sine_app_hdl *sine_dec);
// 正弦波解码参数预处理
void audio_dec_sine_app_probe(struct audio_dec_sine_app_hdl *sine_dec);
extern const int config_divide_en;

// sync
void *audio_dec_app_sync_open(struct audio_dec_app_hdl *dec, int sample_rate, int output_sample_rate, u8 channels, int confirm_time);
void audio_dec_app_sync_close(void *sync);
struct audio_stream_entry *audio_dec_app_sync_get_entry(void *sync);
struct audio_stream_entry *audio_dec_app_sync_get_resample_entry(void *sync);
int audio_dec_app_sync_probe(void *sync);

#endif /*_AUDIO_DEC_APP_H_*/

