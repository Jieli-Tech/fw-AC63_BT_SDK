
#ifndef _DRC_API_H_
#define _DRC_API_H_

#include "typedef.h"
#include "sw_drc.h"

struct audio_drc_filter_info {
    struct drc_ch *pch;
    struct drc_ch *R_pch;
};

typedef int (*audio_drc_filter_cb)(struct audio_drc_filter_info *info);

struct audio_drc_param {
    u32 channels : 2;
    u32 online_en : 1;
    u32 remain_en : 1;
    u32 stero_div : 1;
    u32 reserved : 22;
    audio_drc_filter_cb cb;

    u8 drc_name;
};

struct audio_drc {
    struct drc_ch sw_drc[2];
    u32 sr : 16;
    u32 channels : 2;
    u32 remain_flag : 1;
    u32 updata : 1;
    u32 online_en : 1;
    u32 remain_en : 1;
    u32 start : 1;
    u32 run32bit : 1;
    u32 need_restart : 1;
    u32 stero_div : 1;
    audio_drc_filter_cb cb;
    void *hdl;
    void *output_priv;
    int (*output)(void *priv, void *data, u32 len);
    u32 drc_name;
};


int audio_drc_open(struct audio_drc *drc, struct audio_drc_param *param);

void audio_drc_set_output_handle(struct audio_drc *drc, int (*output)(void *priv, void *data, u32 len), void *output_priv);

void audio_drc_set_samplerate(struct audio_drc *drc, int sr);

int audio_drc_set_32bit_mode(struct audio_drc *drc, u8 run_32bit);

int audio_drc_start(struct audio_drc *drc);
int audio_drc_run(struct audio_drc *drc, s16 *data, u32 len);
int audio_drc_close(struct audio_drc *drc);
int audio_drc_change(struct audio_drc *drc, u8 sw);

// extern int drc_get_filter_info(struct audio_drc_filter_info *info);
int drc_get_filter_info(struct audio_drc_filter_info *info);
// extern int drc_get_filter_info2(struct audio_drc_filter_info *info);
extern void drc_app_run_check(struct audio_drc *drc);

int audio_drc_run_inOut(struct audio_drc *drc, s16 *indata, s16 *data, u32 len);
#endif

