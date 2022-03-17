#ifndef SPECTRUM_FFT_API_H
#define SPECTRUM_FFT_API_H

#include "media/spectrum/SpectrumShow_api.h"
#include "media/audio_stream.h"

//该模块 mips 消耗5M
//RAM 单声道时：4.4Kbyte
//RAM 双声道时：5.4Kbyte
typedef struct _spectrum_fft_open_parm {
    u32 sr;              //采样率
    u32 channel: 4;      //通道数
    u32 mode: 4;         //模式，双声道起作用，0 计算的是第一声道的频谱值，
    //1计算的是第二声道频谱值，2为第一声道与第二声道相加除2的频谱值

    float attackFactor;//下降因子[0,1)
    float releaseFactor;//上升因子[0,1)
} spectrum_fft_open_parm;


typedef struct _spectrum_fft_hdl {
    void *work_buf;
    u32 run_en: 1;

    s16 *out_buf;
    u32 out_buf_size;
    u32 offset;

    spectrum_fft_open_parm parm;
    struct audio_stream_entry entry;	// 音频流入口
} spectrum_fft_hdl;

/*----------------------------------------------------------------------------*/
/**@brief   打开
   @param    *_parm: 始化参数，详见结构体spectrum_fft_open_parm
   @return   句柄
   @note
*/
/*----------------------------------------------------------------------------*/
spectrum_fft_hdl *audio_spectrum_fft_open(spectrum_fft_open_parm *parm);

/*----------------------------------------------------------------------------*/
/**@brief    audio_spectrum_fft_close 关闭处理
   @param    _hdl:句柄
   @return  0:成功  -1：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_spectrum_fft_close(spectrum_fft_hdl *hdl);



/*----------------------------------------------------------------------------*/
/**@brief    audio_spectrum_fft_run 同步处理,每次run都会把输入buf消耗完，才会往下走
   @param    _hdl:句柄
   @param    data:输入数据
   @param    len:输入数据长度
   @return  len
   @note    频谱计算处理，只获取输入的数据，不改变输入的数据
*/
/*----------------------------------------------------------------------------*/
int audio_spectrum_fft_run(spectrum_fft_hdl *hdl, s16 *data, int len);

/*----------------------------------------------------------------------------*/
/**@brief    audio_spectrum_fft_switch 运行过程做开关处理
   @param    hdl:句柄
   @param    en:0 关闭频响运算  1 打开频响运算  (通话模式，不支持频谱计算.通话模式已经使用fft， 需关闭频谱计算)
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void audio_spectrum_fft_switch(spectrum_fft_hdl *hdl, u8 en);
/*----------------------------------------------------------------------------*/
/**@brief    audio_spectrum_fft_get_num 获取频谱个数
   @param    _hdl:句柄
   @return  返回频谱的个数
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_spectrum_fft_get_num(spectrum_fft_hdl *hdl);
/*----------------------------------------------------------------------------*/
/**@brief    audio_spectrum_fft_get_val 获取频谱值
   @param    _hdl:句柄
   @return  返回存储频谱值的地址
   @note
*/
/*----------------------------------------------------------------------------*/
short *audio_spectrum_fft_get_val(spectrum_fft_hdl *hdl);
#endif
