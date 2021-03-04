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


// buf_sync调整参数
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

/*
 * 跟随buf中的数据量动态调整
 * 数据量小于bottom_size时输出采样率增加inc_step，最多调整max_step
 * 数据量大于top_size时输出采样率减小dec_step，最多调整max_step
 * 数据量处于bottom_size和top_size之间时，adjust_step逐步回归于0
 * 建议buf使用大空间，避免总是超出bottom_size、top_size
 */
struct audio_buf_sync_hdl {
    int begin_size;		// 数据量起始大小
    int top_size;		// 数据量限定范围最大值
    int bottom_size;	// 数据量限定范围最小值
    int sample_rate;	// 输入采样率
    int audio_new_rate;	// 输出采样率
    int rate_offset;	// 采样偏移
    s16 adjust_step;	// 动态采样调整值
    u16 max_step;		// 采样率调整最大值
    u8 inc_step;		// 采样率调整增加复读
    u8 dec_step;		// 采样率调整减小复读
    u8 start;			// 启动采样率调整标志
    struct audio_src_handle *src_sync;
};

/*
*********************************************************************
*                  Audio Buf Sync Open
* Description: 按参数配置创建句柄
* Arguments  : *sync	buf同步句柄
*              *param	参数
* Return	 : true		成功
* Note(s)    : None.
*********************************************************************
*/
int audio_buf_sync_open(struct audio_buf_sync_hdl *sync, struct audio_buf_sync_open_param *param);
/*
*********************************************************************
*                  Audio Buf Sync Close
* Description: 关闭句柄
* Arguments  : *sync	buf同步句柄
* Return	 : true		成功
* Note(s)    : None.
*********************************************************************
*/
int audio_buf_sync_close(struct audio_buf_sync_hdl *sync);

/*
*********************************************************************
*                  Audio Buf Sync Adjust
* Description: 跟随数据量动态调整
* Arguments  : *sync		buf同步句柄
*              data_size	当前数据量大小
* Return	 : true		成功
* Note(s)    : None.
*********************************************************************
*/
int audio_buf_sync_adjust(struct audio_buf_sync_hdl *sync, int data_size);
/*
*********************************************************************
*                  Audio Buf Sync Follow Rate
* Description: 跟随输入输出采样变化动态调整
* Arguments  : *sync		buf同步句柄
*              in_rate		跟随的输入采样率
*              out_rate		跟随的输出采样率
* Return	 : true		成功
* Note(s)    : None.
*********************************************************************
*/
int audio_buf_sync_follow_rate(struct audio_buf_sync_hdl *sync, int in_rate, int out_rate);

/*
*********************************************************************
*                  Audio Buf Sync Update out Sample Rate
* Description: 重新设置输出采样率
* Arguments  : *sync		buf同步句柄
*              out_rate		输出采样率
* Return	 : true		成功
* Note(s)    : None.
*********************************************************************
*/
int audio_buf_sync_update_out_sr(struct audio_buf_sync_hdl *sync, int out_sr);

#endif /*_AUDIO_BUF_SYNC_H_*/

