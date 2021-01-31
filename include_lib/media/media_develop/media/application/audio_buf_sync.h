/***********************************Jieli tech************************************************
  File : audio_buf_sync.c
  By   : Huxi
  brief: 跟随buf变化动态同步调整采样率
  Email: huxi@zh-jieli.com
  date : 2020-06-11
********************************************************************************************/

#ifndef _AUDIO_BUF_SYNC_H_
#define _AUDIO_BUF_SYNC_H_

#include "asm/includes.h"
#include "generic/includes.h"
#include "system/includes.h"
// #include "media/includes.h"
#include "asm/cpu_includes.h"
#include "asm/audio_src.h"
#include "media/audio_decoder.h"
#include "media/audio_encoder.h"


struct audio_buf_sync_open_param {
    int total_len;		// buf总长度
    int begin_per;		// 起始百分比，默认60
    int top_per;		// 最大百分比，默认80
    int bottom_per;		// 最小百分比，默认30
    int in_sr;			// 输入采样率
    int out_sr;			// 输出采样率
    u8 inc_step;		// 每次调整增加步伐，默认5
    u8 dec_step;		// 每次调整减少步伐，默认5
    u16 max_step;		// 最大调整步伐，默认80
    u8 check_in_out;	// 检测in和out是否相同，默认不检测
    u8 ch_num;			// 通道数
    void *output_hdl;
    int (*output_handler)(void *hdl, void *, int);
};

// 跟随buf大小动态调整
struct audio_buf_sync_hdl {
    int begin_size;
    int top_size;
    int bottom_size;
    int sample_rate;
    int audio_new_rate;
    int rate_offset;
    s16 adjust_step;
    u16 max_step;
    u8 inc_step;
    u8 dec_step;
    u8 start;
    struct audio_src_handle *src_sync;
};

int audio_buf_sync_open(struct audio_buf_sync_hdl *sync, struct audio_buf_sync_open_param *param);
int audio_buf_sync_close(struct audio_buf_sync_hdl *sync);

int audio_buf_sync_adjust(struct audio_buf_sync_hdl *sync, int data_size);
int audio_buf_sync_follow_rate(struct audio_buf_sync_hdl *sync, int in_rate, int out_rate);

int audio_buf_sync_update_out_sr(struct audio_buf_sync_hdl *sync, int out_sr);

#endif /*_AUDIO_BUF_SYNC_H_*/

