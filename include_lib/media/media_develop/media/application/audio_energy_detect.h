/*******************************************************************************************
  File : audio_energy_detect.h
  By   : LinHaibin
  brief: 数据能量检测
         mem : 148 + 56 * channels (Byte)
         mips: 8MHz / 44100 points / sec
  Email: linhaibin@zh-jieli.com
  date : Fri, 24 Jul 2020 18:11:37 +0800
********************************************************************************************/
#ifndef _AUDIO_ENERGY_DETECT_H_
#define _AUDIO_ENERGY_DETECT_H_

#include "typedef.h"

#define AUDIO_E_DET_UNMUTE      (0x00)
#define AUDIO_E_DET_MUTE        (0x01)

typedef struct _audio_energy_detect_param {
    s16 mute_energy;                    // mute 阈值
    s16 unmute_energy;                  // unmute 阈值
    u16 mute_time_ms;                   // 能量低于mute阈值进入mute状态时间
    u16 unmute_time_ms;                 // 能量高于unmute阈值进入unmute状态时间
    u32 sample_rate;                    // 采样率
    void (*event_handler)(u8, u8);      // 事件回调函数 event channel
    u16 count_cycle_ms;                 // 能量计算的时间周期
    u8  ch_total;                       // 通道总数
    u8  pcm_mute: 1;                    // 保留，未使用
    u8  onoff: 1;                       // 保留，未使用
    u8  skip: 1;                        // 1:数据不处理 0:处理
    u8  dcc: 1;                         // 1:去直流打开 0:关闭
} audio_energy_detect_param;

/*******************************************************
* Function name	: audio_energy_detect_entry_get
* Description	: 获取能量计算句柄的数据流entry节点
* Parameter		:
*   @_hdl       	能量获取句柄
* Return        : 返回entry NULL:新建失败
********************************************************/
extern void *audio_energy_detect_entry_get(void *_hdl);

/*******************************************************
* Function name	: audio_energy_detect_open
* Description	: 新建能量计算句柄
* Parameter		:
*   @param	     	能量获取配置参数
* Return        : 返回句柄 NULL:新建失败
********************************************************/
extern void *audio_energy_detect_open(audio_energy_detect_param *param);

/*******************************************************
* Function name	: audio_energy_detect_run
* Description	: 能量计算运算
* Parameter		:
*   @_hdl	     	能量获取句柄
*   @data	     	数据起始地址
*   @len	     	数据字节长度
* Return        : 返回处理长度 -1:出错
********************************************************/
extern int audio_energy_detect_run(void *_hdl, s16 *data, u32 len);

/*******************************************************
* Function name	: audio_energy_detect_close
* Description	: 关闭能量获取句柄
* Parameter		:
*   @_hdl	     	能量获取句柄
* Return        : 0:正常  other:出错
********************************************************/
extern int audio_energy_detect_close(void *_hdl);

/*******************************************************
* Function name	: audio_energy_detect_skip
* Description	: 能量获取跳过计算（不生效）
* Parameter		:
*   @_hdl       	能量获取句柄
*   @channel       	通道(BIT(0)对应通道0，如此类推)
*   @skip       	1:跳过 0:正常计算
* Return        : 0:正常  other:出错
********************************************************/
extern int audio_energy_detect_skip(void *_hdl, u32 channel, u8 skip);

/*******************************************************
* Function name	: audio_energy_detect_energy_get
* Description	: 获取计算出来通道的能量
* Parameter		:
*   @_hdl       	能量获取句柄
*   @ch       		通道(BIT(0)对应通道0，如此类推)
* Return        : 返回能量值  -1:出错
********************************************************/
extern int audio_energy_detect_energy_get(void *_hdl, u8 ch);

/*******************************************************
* Function name	: audio_energy_detect_sample_rate_update
* Description	: 能量获取更新数据采样率
* Parameter		:
*   @_hdl       	能量获取句柄
*   @sample_rate    采样率
* Return        : 0:正常  other:出错
********************************************************/
extern int audio_energy_detect_sample_rate_update(void *_hdl, u32 sample_rate);


/******************************************************************************
                 audio energy detect 接入 audio stream demo

INCLUDE:
	#include "application/audio_energy_detect.h"
	void *audio_e_det_hdl = NULL;
	struct list_head *audio_e_det_entry = NULL;
	void test_e_det_handler(u8 event, u8 ch)
	{
		printf(">>>> ch:%d %s\n", ch, event ? ("MUTE") : ("UNMUTE"));
	}

OPEN:
    audio_energy_detect_param e_det_param = {0};
    e_det_param.mute_energy = 5;
    e_det_param.unmute_energy = 10;
    e_det_param.mute_time_ms = 100;
    e_det_param.unmute_time_ms = 50;
    e_det_param.count_cycle_ms = 100;
    e_det_param.sample_rate = 44100;
    e_det_param.event_handler = test_e_det_handler;
    e_det_param.ch_total = 2;
    e_det_param.dcc = 1;
    audio_e_det_hdl = audio_energy_detect_open(&e_det_param);
    audio_e_det_entry = audio_energy_detect_entry_get(audio_e_det_hdl);

	entries[entry_cnt++] = audio_e_det_entry;

CLOSE:
	audio_energy_detect_close(audio_e_det_hdl);

******************************************************************************/

#endif  // #ifndef _AUDIO_ENERGY_DETECT_H_

