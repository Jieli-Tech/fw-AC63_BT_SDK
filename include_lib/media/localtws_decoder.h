#ifndef LOCALTWS_DECODER_H
#define LOCALTWS_DECODER_H


#include "media/includes.h"

enum {
    LOCALTWS_DEC_STATUS_STOP = 0,	// 解码停止
    LOCALTWS_DEC_STATUS_PLAY,		// 正在解码
};

struct localtws_decoder {
    u32 ch_num : 4;			// 声道数
    u32 output_ch_num : 4;	// 输出声道数
    u32 output_ch_type : 4;	// 输出声道类型
    u32 status : 3;			// 解码状态
    u32 read_en : 1;		// 开始解码
    u32 tmp_pause : 1;		// 被其他解码打断，临时暂停
    u32 need_get_fmt : 1;	// 需要获取音频信息
    u32 sync_step : 3;
    u32 preempt_state : 4;
    u32 remain_flag : 1;	// 输出剩余标记
    u32 fade_out : 1;		// 淡出
    s16 fade_step;			// 淡入淡出步进数
    s16 fade_value;			// 淡入淡出记录值
    u32 fade_out_mute_points;	// 淡出后静音点数
    u32 wait_time;
    int begin_delay_time;
    void *sync;
    u16 resume_timeout;
    // u16 drop_samples;
    u32 dec_type;			// 解码类型
    u16 sample_rate;		// 采样率
    u16 sbc_header_len;		// sbc头部长度
    u8 *tws_ptr;			// tws数据
    u32 tws_len;			// tws数据长度
    u32 tws_ulen;			// tws数据使用长度
    struct audio_decoder_task *decode_task; // 解码任务
    enum audio_channel ch_type;		// 输出类型
    struct audio_decoder decoder;	// 解码句柄
    u8(*tws_master)(void);
};


int localtws_decoder_open(struct localtws_decoder *, struct audio_decoder_task *decode_task);

void localtws_decoder_close(struct localtws_decoder *dec);

void localtws_decoder_set_output_channel(struct localtws_decoder *dec);

void localtws_decoder_stream_sync_enable(struct localtws_decoder *dec,
        void *sync,
        int delay_time,
        u8(*master)(void));

void localtws_decoder_resume_pre(void);

#endif /*A2DP_DECODER_H*/

