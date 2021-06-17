#ifndef _AUD_DEC_EFF_H
#define _AUD_DEC_EFF_H
#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "application/eq_config.h"
#include "app_config.h"
#include "audio_config.h"


typedef int (*eq_output_cb)(void *, void *, int);

struct dec_eq_drc {
    s16 *eq_out_buf;
    int eq_out_buf_len;
    int eq_out_points;
    int eq_out_total;

    void *priv;
    eq_output_cb  out_cb;
    void *drc;
    void *eq;
    u8 async;
    u8 remain;
};

void user_sat16(s32 *in, s16 *out, u32 npoint);
/*----------------------------------------------------------------------------*/
/**@brief    非数据流方式eq drc 打开
   @param    priv:输出回调私有指针
   @param    eq_output_cb:输出回调（eq使用异步时，数据从该回调函数输出）
   @param    sample_rate:采样率
   @param    channel:输入数据通道数
   @param    async:eq是否采样异步方式处理 1：是， 0：否
   @param    drc_en:是否使能drc 1:是， 0：否
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void *dec_eq_drc_open_demo(void *priv, int (*eq_output_cb)(void *, void *, int), u32 sample_rate, u8 channel, u8 async, u8 drc_en);
/*----------------------------------------------------------------------------*/
/**@brief    非数据流方式eq drc 关闭
   @param    eff句柄(dec_eq_drc_open_demo的返回值)
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void dec_eq_drc_close_demo(void *eff);
/*----------------------------------------------------------------------------*/
/**@brief    非数据流方式eq drc 处理
   @param    eff句柄(dec_eq_drc_open_demo的返回值)
   @param    data:输入数据地址
   @param    len:输入数据字节长度
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
int dec_eq_drc_run_demo(void *priv, void *data, u32 len);


/*----------------------------------------------------------------------------*/
/**@brief    数据流方式eq drc 打开 demo
   @param    sample_rate:采样率
   @param    ch_num:通道个数
   @param    drc_en:是否使用drc(限幅器)1:是  0:否
   @parm     mode:播歌song_eq_mode,通话下行call_eq_mode,播歌后RL_RR通道用rl_eq_mode
   @return   句柄
   @note     支持在线调试，支持使用eq_cfg_hw.bin效果文件的
*/
/*----------------------------------------------------------------------------*/
void *stream_eq_drc_open_demo(u32 sample_rate, u8 ch_num, u8 drc_en, u8 mode);
/*----------------------------------------------------------------------------*/
/**@brief    数据流方式eq drc 关闭 demo
   @param    句柄(eq_drc_open_demo的返回值)
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void stream_eq_drc_close_demo(struct audio_eq_drc *eq_drc);
#endif
