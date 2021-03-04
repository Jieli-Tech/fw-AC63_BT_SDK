#ifndef __SW_DRC_H
#define __SW_DRC_H

#include "generic/typedef.h"
#include "generic/list.h"
#include "os/os_api.h"

#define DRC_TYPE_LIMITER		1
#define DRC_TYPE_COMPRESSOR		2


struct drc_limiter {
    int attacktime;   //启动时间
    int releasetime;  //释放时间
    int threshold[2]; //threshold[1]是固定值32768, threshold[0]为界面参数
};

struct drc_compressor {
    int attacktime;   //启动时间
    int releasetime;  //释放时间
    int threshold[3]; //threshold[2]是固定值32768
    int ratio[3];     //ratio[0]为固定值100， ratio[1] ratio[2]为界面参数
};

struct drc_ch {
    u8 nband;		//max<=3，1：全带 2：两段 3：三段
    u8 type;		//0:没有使能限幅和压缩器，1:限幅器   2:压缩器
    u8 reserved[2];     //reserved[1]保留,未用, reserved[0]记录了 多带限幅时，是否再开启一次全带限幅
    int low_freq;       //中低频分频点
    int high_freq;      //中高频分频点
    int order;          //分频器阶数, 2或者4
    union {
        struct drc_limiter		limiter[4];   //限幅器
        struct drc_compressor	compressor[3];//压缩器
    } _p;
};



struct sw_drc {
    void *work_buf[4];    //drc内部驱动句柄
    void *crossoverBuf;   //分频器句柄
    int *run_tmp[3];      //多带限幅器或压缩器，运行buf
    int run_tmp_len;      //多带限幅器或压缩器，运行buf 长度
    u8 nband;             //max<=3，1：全带 2：两段 3：三段
    u8 type;              //0:没有使能限幅和压缩器，1:限幅器   2:压缩器
    u8 channel;           //通道数
    u8 run32bit;          //drc处理输入输出数据位宽是否是32bit 1:32bit  0:16bit
    struct audio_eq *crossover[3];//多带分频器eq句柄
    u32 sample_rate;      //采样率
    u8 nsection;          //分频器eq段数
    u8 other_band_en;     //多带限幅器之后，是否需要再做一次全带的限幅器  1：需要，0：不需要

};
extern void *get_low_sosmatrix();
extern void *get_high_sosmatrix();
extern void *get_band_sosmatrix();
extern int get_crossover_nsection();


extern void *audio_sw_drc_open(struct drc_ch *ch_tmp, u32 sample_rate, u8 channel, u8 drc_name, u8 run32bit);
extern void audio_sw_drc_close(void *hdl);
extern int audio_sw_drc_update(void *hdl, struct drc_ch *ch_tmp, u32 sample_rate, u8 channel);
extern int audio_sw_drc_run(void *hdl, s16 *in_buf, s16 *out_buf, int npoint_per_channel);

#define  EQ_INOUT_EN 0
#if EQ_INOUT_EN
void audio_hw_crossover_open(struct sw_drc *drc, int (*L_coeff)[3], u8 nsection);
void audio_hw_crossover_close(struct sw_drc *drc);
void audio_hw_crossover_run(struct sw_drc *drc, s16 *data, int len);
#endif

#endif
