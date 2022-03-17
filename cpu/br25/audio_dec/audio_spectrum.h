#ifndef _AUDIO_SPECTRUM_H_
#define _AUDIO_SPECTRUM_H_

#include "app_config.h"
#include "clock_cfg.h"
#include "media/includes.h"
#include "asm/includes.h"
#include "media/spectrum/spectrum_eq.h"
#include "media/spectrum/spectrum_fft.h"


/*----------------------------------------------------------------------------*/
/**@brief   打开频响统计
   @param   sr:采样率
   @return  hdl:句柄
   @note
*/
/*----------------------------------------------------------------------------*/
spectrum_fft_hdl *spectrum_open_demo(u32 sr, u8 channel);

/*----------------------------------------------------------------------------*/
/**@brief   关闭频响统计
   @param  hdl:句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void spectrum_close_demo(spectrum_fft_hdl *hdl);

/*----------------------------------------------------------------------------*/
/**@brief   频响输出例子
   @return
   @note   如何获取频谱值，参考该函数
*/
/*----------------------------------------------------------------------------*/
void spectrum_get_demo(void *p);


/*----------------------------------------------------------------------------*/
/**@brief  切换频响计算
   @param  en:0 不做频响计算， 1 使能频响计算（通话模式，需关闭频响计算）
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void spectrum_switch_demo(u8 en);

#endif




