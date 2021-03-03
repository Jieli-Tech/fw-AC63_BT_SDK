/***********************************Jieli tech************************************************
  File : audio_localtws.c
  By   : Huxi
  brief:
  Email: huxi@zh-jieli.com
  date : 2020-06
********************************************************************************************/

#ifndef _AUDIO_LOCALTWS_H_
#define _AUDIO_LOCALTWS_H_

#include "asm/includes.h"
#include "system/includes.h"
#include "media/mixer.h"

#define LOCALTWS_SBC_ENC_IN_SIZE	512
#define LOCALTWS_SBC_ENC_OUT_SIZE	256

#define LOCALTWS_CODE_CONVERT_BUF_LEN		(2 * 1024)

#define LOCALTWS_MEDIA_INFO_LEN		(1)
#define LOCALTWS_MEDIA_FMT_LEN		(LOCALTWS_MEDIA_INFO_LEN+sizeof(u16))	// B0:INFO, B[1-2]:SEQN

#define LOCALTWS_MEDIA_INFO_STOP	(0)		// 特殊包，停止
#define LOCALTWS_MEDIA_INFO_PAUSE	(0xff)	// 特殊包，暂停


enum {
    LOCALTWS_GLOBLE_EVENT_MEDIA_START = 1,	// localtws启动事件
    LOCALTWS_GLOBLE_EVENT_MEDIA_STOP,		// localtws停止事件
    LOCALTWS_GLOBLE_EVENT_MEDIA_TIMEOUT,	// localtws超时事件
};

enum {
    LOCALTWS_MEDIA_SEND_PAUSE_START = 1,	// 发送暂停命令
    LOCALTWS_MEDIA_SEND_PAUSE_END,			// 发送完成
    LOCALTWS_MEDIA_DEC_PAUSE,				// 暂停
    LOCALTWS_MEDIA_DEC_PAUSE_RECOVER,		// 暂停恢复
};

extern const u16 LOCALTWS_MEDIA_BUF_LEN;
extern const u16 LOCALTWS_MEDIA_BUF_LIMIT_LEN;
extern const u16 LOCALTWS_MIXER_BUF_LEN;
extern const u16 LOCALTWS_MEDIA_TO_MS;


// 数据推送到tws
struct localtws_push_hdl {
    u16 resume_tmr_id;
    u8  wait_resume;
    struct audio_stream_entry entry;	// 音频流入口
};

// localtws
struct localtws_globle_hdl {
    u16 tmrout;		// 接受超时
    u16 drop_timer;	// 中止传输定时
    u16 seqn;		// 传输包序号
    u32 media_value;		// 当前localtws媒体信息
    u32 drop_frame_start : 1;	// 中止数据传输
    u32 tws_send_pause : 3;	// localtws正在发送暂停命令
    u32 tws_stop : 1;		// localtws正在停止
    // u32 mixer_init : 1;		// mixer初始化ok
    u32 media_buf_malloc : 1;	// 1:使用malloc。0:使用固定地址
    u8  fade_ms;			// 淡入淡出时长
    u8  fade_out_mute_ms;	// 淡出后静音时长
    void *media_buf;		// buf地址
    int (*event_cb)(int event, int *parm);
    int (*check_active)();	// 检查是否是活动设备
    void (*resume)(); 		// 激活解码
    int (*dec_restart)(); 	// 解码重新开始
    int (*data_abandon)();	// 抛弃数据
    int (*frame_drop)();	// 中断传输
    OS_MUTEX mutex;			// 互斥

    u8 *mixer_buf;			// 混合使用的buf
    struct audio_mixer mixer;	// 混合输出

    struct localtws_push_hdl push;	// 数据推送
};
extern struct localtws_globle_hdl g_localtws;

// localtws压缩为sbc或者mp3
// sbc直接传输；mp3会转换成localtws可用的格式再传输
struct localtws_enc_hdl {
    struct audio_encoder encoder;	// 编码器
    struct audio_encoder_task *encode_task;	// 编码任务
    s16 *output_frame;		// 编码输出帧
    int output_frame_len;	// 编码输出帧长
    int pcm_frame[LOCALTWS_SBC_ENC_IN_SIZE / 4]; // 音频编码读数据缓存
    u8 *output_buf;			// 音频缓存
    int output_buf_len;		// 音频缓存长度
    cbuffer_t output_cbuf;	// 音频缓存cbuf
    u32 status : 1;			// 编码状态
    u32 dec_is_start : 1;	// 后级是否已经开始解码
    u32 dec_check_start_ok : 1;	// 判断后级解码完成

    u8 sbc_fame_cnt;	// sbc帧打包计数
    u8 *sbc_out_buf;	// sbc帧缓存
    u16 sbc_fame_len;	// sbc帧长
    u16 sbc_fame_sn;	// sbc帧计数
    void *p_sbc_param;	// sbc编码参数

    int (*check_dec_start)(void);	// 检测后级是否开始解码
    u32 dec_start_delay_ms;			// 开始解码后静音延迟多久
    unsigned long dec_start_time;	// 延迟时间戳

    u8 *out_data;		// 数据流
    int out_data_total;	// 数据流总长
    int out_data_len;	// 数据流输出长
    struct audio_stream_entry entry;	// 音频流入口
};
extern struct localtws_enc_hdl *localtws_enc;

// 把mp3或者wma等数据转换为可供localtws使用的数据
struct localtws_code_convert_hdl {
    struct audio_decoder decoder;	// 解码器
    struct audio_decoder_task *decode_task;	// 解码任务
    u8 file_buf[LOCALTWS_CODE_CONVERT_BUF_LEN];	// 解码数据缓存
    cbuffer_t file_cbuf;	// 解码数据cbuf
    u32 status : 1;			// 解码状态

    u8 *out_data;		// 数据流
    int out_data_total;	// 数据流总长
    int out_data_len;	// 数据流输出长
    struct audio_stream_entry entry;	// 音频流入口
};
extern struct localtws_code_convert_hdl *localtws_code_convert;


// localtws底层收数处理
int localtws_tws_rx_handler_notify(u8 *data, int len, u8 rx);

// localtws设置事件回调接口（非活动设备才回调。活动设备由上层主动控制）
void localtws_globle_set_event_cb(int (*event_cb)(int event, int *parm));
// localtws设置检测活动设备接口
void localtws_globle_set_check_active(int (*check_active)());
// localtws设置激活接口
void localtws_globle_set_resume(void (*resume)());
// localtws设置解码重启接口
void localtws_globle_set_dec_restart(int (*dec_restart)());
// localtws设置数据抛弃判断接口（data_abandon返回true代表抛弃该数据）
void localtws_globle_set_data_abandon(int (*data_abandon)());
// localtws设置中断传输判断接口（frame_drop返回true代表中断传输）
void localtws_globle_set_frame_drop(int (*frame_drop)());

// localtws媒体使能（申请空间，使能tws底层传输）
void localtws_media_enable(void);
// localtws媒体关闭使能（关闭tws底层传输，释放空间）
void localtws_media_disable(void);
// localtws把音频信息转换为传输信息
int localtws_media_set_info(u8 *data, struct audio_fmt *pfmt);
// localtws把传输信息转换为音频信息
int localtws_media_get_info(u8 *data, struct audio_fmt *pfmt);

// localtws推送音频数据
int localtws_media_push_data(struct audio_fmt *pfmt, s16 *data, int len);
// localtws推送一个byte（超时等待推送）
int localtws_media_push_byte(u8 info, int tmrout);
// localtws推送一个end消息（超时等待推送）
int localtws_media_send_end(int tmrout);
// localtws推送一个暂停消息（超时等待推送）
int localtws_media_send_pause(int tmrout);

// 打开localtws音频数据推送
int localtws_push_open(void);
// 关闭localtws音频数据推送
void localtws_push_close(void);

// 打开localtws音频转换解码
int localtws_code_convert_open(struct audio_fmt *pfmt);
// 关闭localtws音频转换解码
void localtws_code_convert_close(void);
// localtws转换激活
void localtws_code_convert_resume(void);
// localtws转换写入数据
int localtws_code_convert_wfile(struct audio_fmt *pfmt, s16 *data, int len);

// 打开localtws音频编码
int localtws_enc_open(struct audio_fmt *pfmt);
// 关闭localtws音频编码
void localtws_enc_close();
// localtws编码设置数据源为流数据的处理
// 数据源为流数据时需要先传输静音数据，避免开始播放时的丢数杂音
void localtws_enc_set_stream_data_ctrl(int (*check_dec)(void), u32 dly_ms);
// localtws音频编码激活
void localtws_enc_resume(void);
// localtws编码数据写入
int localtws_enc_write(void *priv, s16 *data, int len);
// localtws音频编码清除数据
void localtws_enc_clear(void);

// localtws获取sbc编码支持的采样率
int localtws_enc_sbc_sample_rate_select(int rate, u8 high);

// 启动localtws传输中止
void localtws_drop_frame_start();
// 关闭localtws传输中止
void localtws_drop_frame_stop();

#endif /*_AUDIO_LOCALTWS_H_*/

