
#ifndef _AUDIO_VOCAL_REMOVE_API_H_
#define _AUDIO_VOCAL_REMOVE_API_H_

#include "system/includes.h"
#include "media/includes.h"

typedef struct _vocal_remove_open_parm {
    u8 channel;//输入音频声道数
} vocal_remove_open_parm;


typedef struct _vocal_remove_hdl {
    u32 vocal_remove_dis: 1;
    vocal_remove_open_parm o_parm;
    struct audio_stream_entry entry;	// 音频流入口
} vocal_remove_hdl;
/*----------------------------------------------------------------------------*/
/**@brief   audio_vocal_remove_open  人声消除打开
   @param    *_parm: 始化参数，详见结构体vocal_remove_open_parm
   @return   句柄
   @note
*/
/*----------------------------------------------------------------------------*/
vocal_remove_hdl *audio_vocal_remove_open(vocal_remove_open_parm *_parm);

/*----------------------------------------------------------------------------*/
/**@brief    audio_vocal_remove_close 人声关闭处理
   @param    _hdl:句柄
   @return  0:成功  -1：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_vocal_remove_close(vocal_remove_hdl *_hdl);


/*----------------------------------------------------------------------------*/
/**@brief    audio_vocal_remove_sw 运行过程开关处理
   @param    _hdl:句柄
   @param    dis: 0：人声消除有效  1:人声消除无效
   @return  0:成功  -1：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_vocal_remove_sw(vocal_remove_hdl *_hdl, u32 dis);

#endif

