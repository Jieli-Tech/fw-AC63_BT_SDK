
#ifndef _AUDIO_LOUDNESS_API_H_
#define _AUDIO_LOUDNESS_API_H_
#include "equalloudness/loudness_api.h"
// #include "application/audio_config.h"
#include "math.h"
#include "stdlib.h"
/*
 *等响度使用eq实现，同个数据流中，如需要打开等响度，请打开开eq总使能，关闭其其他eq,例如蓝牙模式eq
 * */
typedef struct _equalloudness_open_parm {
    u16 sr;               //采样率
    u8 ch_num;           //通道数
    u8 threadhold_vol;    //触发等响度软件数字音量阈值
    int (*vol_cb)(void);//该函数获取系统软件的数字音量
} loudness_open_parm;


#define LOUDNESS_THREADHOLD_VOL   (15)

typedef struct _equalloudness_hdl {
    struct audio_eq_drc *loudness;
    u16 loudness_timer;
    u8 loudness_en;
    u8 prev_status;

    loudness_open_parm o_parm;
} loudness_hdl;

/*----------------------------------------------------------------------------*/
/**@brief    audio_equal_loudness_open, 等响度 打开
   @param    *_parm: 等响度初始化参数，详见结构体loudness_open_parm
   @return   等响度句柄
   @note
*/
/*----------------------------------------------------------------------------*/
loudness_hdl *audio_equal_loudness_open(loudness_open_parm *parm);

/*----------------------------------------------------------------------------*/
/**@brief   audio_equal_loudness_close 等响度关闭处理
   @param    _hdl:句柄
   @return  0:成功  -1：失败
   @note
*/
/*----------------------------------------------------------------------------*/

int audio_equal_loudness_close(loudness_hdl *hdl);
#endif

