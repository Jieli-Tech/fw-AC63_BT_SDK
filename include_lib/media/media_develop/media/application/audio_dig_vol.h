/*******************************************************************************************
  File : audio_dig_vol.h
  By   : LinHaibin
  brief: 数据数字音量与分组管理
  Email: linhaibin@zh-jieli.com
  date : Fri, 24 Jul 2020 18:00:39 +0800
********************************************************************************************/
#ifndef _AUDIO_DIG_VOL_H_
#define _AUDIO_DIG_VOL_H_


#define AUDIO_DIG_VOL_CH(x)     (BIT(x))
#define AUDIO_DIG_VOL_ALL_CH    (0xFFFF)

typedef struct _audio_dig_vol_param {
    u8  vol_start;
    u8  vol_max;
    u8  ch_total;
    u8  fade_en;
    u16 fade_points_step;
    u16 fade_gain_step;
    u16 *vol_list;        // NULL:default
} audio_dig_vol_param;

extern void *audio_dig_vol_entry_get(void *_hdl);
extern void *audio_dig_vol_open(audio_dig_vol_param *param);
extern int audio_dig_vol_run(void *hdl, s16 *data, u32 len);
extern int audio_dig_vol_set(void *hdl, u32 channel, u8 vol);
extern int audio_dig_vol_get(void *hdl, u32 channel);
extern int audio_dig_vol_skip(void *hdl, u32 channel, u8 skip_en);
extern int audio_dig_vol_fade(void *hdl, u32 channel, u8 fade_en);
extern int audio_dig_vol_close(void *_hdl);


/******************************************************************************
                audio digital vol 接入 audio stream demo

INCLUDE:
    #include "application/audio_dig_vol.h"
    void *digvol_last = NULL;
    void *digvol_last_entry = NULL;

OPEN:
    audio_dig_vol_param digvol_last_param = {
        .vol_start = 30,
        .vol_max = 30,
        .ch_total = 2,
        .fade_en = 1,
        .fade_points_step = 5,
        .fade_gain_step = 10,
        .vol_list = NULL,
    };
    digvol_last = audio_dig_vol_open(&digvol_last_param);
    digvol_last_entry = audio_dig_vol_entry_get(digvol_last);
    entries[entry_cnt++] = digvol_last_entry;

SET:
    audio_dig_vol_set(digvol_last, AUDIO_DIG_VOL_CH(0), vol);

CLOSE:
    audio_stream_del_entry(digvol_last_entry);
    audio_dig_vol_close(digvol_last);

******************************************************************************/



extern void *audio_dig_vol_group_open(void);
extern int audio_dig_vol_group_add(void *group_head, void *dig_vol_hdl, char *logo);
extern int audio_dig_vol_group_del(void *group_head, char *logo);
extern int audio_dig_vol_group_close(void *group_head);
extern void *audio_dig_vol_group_hdl_get(void *group_head, char *logo);
extern int audio_dig_vol_group_dodge(void *group_head, char *logo, u8 weight, u8 other_weight);

/******************************************************************************
                audio digital group demo

OPEN:
    void *dig_vol_group1;
    dig_vol_group1 = audio_dig_vol_group_open();

ADD:
    audio_dig_vol_group_add(dig_vol_group1, dig_vol_hdl1 ,"dig_vol_1");
    audio_dig_vol_group_add(dig_vol_group1, dig_vol_hdl2 ,"dig_vol_2");
    audio_dig_vol_group_add(dig_vol_group1, dig_vol_hdl3 ,"dig_vol_3");

DEL:
    audio_dig_vol_group_del(dig_vol_group1, "dig_vol_2");

CLOSE:
    audio_dig_vol_group_close(dig_vol_group1);

DODGE:
    audio_dig_vol_group_dodge(dig_vol_group1, "dig_vol_1", 100, 0);         // dodge
    audio_dig_vol_group_dodge(dig_vol_group1, "dig_vol_1", 100, 100);       // no dodge

******************************************************************************/

#endif  // #ifndef _AUDIO_DIG_VOL_H_
