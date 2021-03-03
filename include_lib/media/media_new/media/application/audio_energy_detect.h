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
    s16 mute_energy;
    s16 unmute_energy;
    u16 mute_time_ms;
    u16 unmute_time_ms;
    u32 sample_rate;
    void (*event_handler)(u8, u8);       // event channel
    u16 count_cycle_ms;
    u8  ch_total;
    u8  pcm_mute: 1;
    u8  onoff: 1;
    u8  skip: 1;
    u8  dcc: 1;
} audio_energy_detect_param;

extern void *audio_energy_detect_entry_get(void *_hdl);
extern void *audio_energy_detect_open(audio_energy_detect_param *param);
extern int audio_energy_detect_run(void *_hdl, s16 *data, u32 len);
extern int audio_energy_detect_close(void *_hdl);
extern int audio_energy_detect_skip(void *_hdl, u32 channel, u8 skip);
extern int audio_energy_detect_energy_get(void *_hdl, u8 ch);
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

