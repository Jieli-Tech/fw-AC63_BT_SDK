#ifndef _EQ_APPLY_H_
#define _EQ_APPLY_H_


#include "system/includes.h"
#include "media/audio_stream.h"
#include "application/eq_config.h"

#define DECREASE_MEM  1  //减少输出buf占用
struct audio_eq_drc_parm {
    u8 eq_en: 1;				//eq是否使能 1:使能  0:关闭
    u8 drc_en: 1;           //drc是否使能 1:使能  0:关闭
    u8 high_bass: 1;        //高低音是否使能， 1：使能  0：关闭
    u8 async_en: 1;         //是否使能异步eq  1:使能  0：关闭
    u8 out_32bit: 1;        //eq后是否输出32bit  1:使能： 0关闭
    u8 divide_en: 1;        //各个声道eq drc效果是否独立，0：使用同个效果
    u8 mode_en: 1;          //没离线文件时，是否支持使用默认系数表做eq
    u8 online_en: 1;        //是否支持在线调试 1：支持  0：不支持
    u8 ch_num: 3;           //输入数据通道数
    u8 eq_name;             //FL FR通道的eq_name 普通音乐eq 使用song_eq_mode,通话下行eq 使用call_eq_mode
    u32 sr;                 //采样率

    audio_eq_filter_cb eq_cb; //获取eq系数的回调函数
    audio_drc_filter_cb drc_cb;//获取drc系数的回调
};
/*
*cmd :
*/
#define AUDIO_EQ_SET_CH        0
#define AUDIO_EQ_CLR_DAT       1
#define AUDIO_EQ_HIGH          2
#define AUDIO_EQ_BASS          3
#define AUDIO_EQ_GET_DATA_LEN  4
#define AUDIO_EQ_HIGH_BASS_DIS 5


struct high_bass {
    int freq;     //频率写0， 内部会用默认125hz  和12khz
    int gain;    //增益范围 -12~12
};

typedef struct eq_fade {
    u16 tmr;
#ifdef EQ_CORE_V1
    float cur_gain[2];
    float use_gain[2];
#else
    int cur_gain[2];
    int use_gain[2];
#endif
} audio_eq_fade_cfg;


struct audio_eq_drc {
    struct audio_eq *eq;
    struct audio_drc *drc;
    struct eq_fade *fade_cfg;//高低音系数表

    s16 *eq_out_buf;
    int out_buf_size;
    int eq_out_points;
    int eq_out_total;
    u8 high_bass_dis;

    struct audio_eq_drc_parm parm;
    struct audio_stream_entry entry;	// 音频流入口
};


/*----------------------------------------------------------------------------*/
/**@brief    打开eq drc，支持接入audio_stream
   @param    parm: 功能使用参数,详见结构体定义
   @return   返回eq drc句柄
   @note
*/
/*----------------------------------------------------------------------------*/
struct audio_eq_drc *audio_eq_drc_open(struct audio_eq_drc_parm *parm);
/*----------------------------------------------------------------------------*/
/**@brief    eq drc接入音频流，数据处理回调
   @param    entry: 数据流节点
   @param    in:输入数据结构
   @param    out:输出数据结构
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_eq_drc_close(struct audio_eq_drc *hdl);

/*----------------------------------------------------------------------------*/
/**@brief    参数更新
   @param    hdl:句柄
   @param    cmd:命令
   @param    parm:参数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int  audio_eq_drc_parm_update(struct  audio_eq_drc  *hdl,  u32 cmd, void  *parm);

/*
 *非数据流方式，eq、drc的run处理接口
 * */
int audio_eq_drc_run(struct audio_eq_drc *hdl, s16 *data, u16 len);
#endif
