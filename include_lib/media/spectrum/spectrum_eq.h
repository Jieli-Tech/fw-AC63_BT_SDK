#ifndef SPECTRUM_API_H
#define SPECTRUM_API_H

#include "media/spectrum/coeff_calculate_api.h"
#include "media/spectrum/rms_api.h"
#include "application/audio_eq.h"
#include "media/audio_stream.h"


typedef struct _spectrum_open_parm {
    u32 sr;
    u32 channel: 4;
    u32 section: 6;

    u32 rms_en: 1;
    u32 time_ms: 4;
    u32 d_time: 16; //50ms~500ms
    float alpha;//(0~1)
    float attackFactor;
    float releaseFactor;

    int *freq_center;
    int *quality_factor;

    void *priv;
    int (*output)(void *priv, float *db, u32 len);//user define
} spectrum_open_parm;


typedef struct _spectrum_hdl {
    void *rms_hdl;//算db值
    int *coeff;   //存放系数的地址
    struct audio_eq *eq[10];
    u32 data_len;
    s16 *data;//输出数据地址

    spectrum_open_parm parm;
    struct audio_stream_entry entry;	// 音频流入口
} spectrum_hdl;


spectrum_hdl *audio_spectrum_open(spectrum_open_parm *parm);
int audio_spectrum_close(spectrum_hdl *hdl);
#if 0
//例子


/*----------------------------------------------------------------------------*/
/**@brief   频响输出回调
   @param   data:db值存放的首地址，连续存放 len 个 float型的db值
   @return  len
   @note
*/
/*----------------------------------------------------------------------------*/
int spectrum_output(void *priv, float *data, u32 len)
{
    u8 db_num = len;
    float *tar_data = data;
    for (int i = 0; i < db_num; i++) {
        //输出10个 浮点db值
        /* printf("tar_data db 0x%x\n", *(int *)&tar_data[i]); */
        //printf("tar_data db %d.%d\n", (int )tar_data[i],(int)((int)(tar_data[i]*100)%100));
    }
    return len;
}

static u32 freq[] = {31, 62, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};
static u32 quality[] = {
    (int)(0.7f * (1 << 24)), (int)(0.7f * (1 << 24)),
    (int)(0.7f * (1 << 24)), (int)(0.7f * (1 << 24)),
    (int)(0.7f * (1 << 24)), (int)(0.7f * (1 << 24)),
    (int)(0.7f * (1 << 24)), (int)(0.7f * (1 << 24)),
    (int)(0.7f * (1 << 24)), (int)(0.7f * (1 << 24))
};
/*----------------------------------------------------------------------------*/
/**@brief   打开频响统计
   @param   sr:采样率
   @return  hdl:句柄
   @note
*/
/*----------------------------------------------------------------------------*/
spectrum_hdl *spectrum_open_demo(u32 sr)
{
#if TCFG_EQ_ENABLE
    spectrum_hdl *hdl = NULL;
    spectrum_open_parm parm = {0};
    parm.sr = sr;
    parm.channel = 1;
    parm.section =  EQ_SECTION_MAX;
    parm.rms_en = 1;
    parm.time_ms = 5;
    parm.attackFactor = 0.9;
    parm.releaseFactor = 0.9;
    parm.freq_center = freq;
    parm.quality_factor = quality;
    parm.d_time = 100;
    parm.alpha = 0.9;
    parm.priv = NULL;
    parm.output = spectrum_output;
    hdl = audio_spectrum_open(&parm);
    return hdl;
#else
    return NULL;
#endif
}

#endif
#endif
