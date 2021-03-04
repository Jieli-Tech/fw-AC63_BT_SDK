
#ifndef _AUDIO_VBASS_API_H_
#define _AUDIO_VBASS_API_H_
#include "system/includes.h"
#include "media/audio_stream.h"
#include "vbass/vbass_api.h"

typedef struct _vbass_open_parm {
    u16 sr;                   //输入音频采样率
    u8 channel;               //输入音频声道数
} vbass_open_parm;

typedef struct _vbass_update_parm {
    int bass_f;               //外放的低音截止频率
    int level;                //增强强度(4096 等于 1db， 建议范围：4096 到 16384)
} vbass_update_parm;


typedef struct _vbass_hdl {
    VBASS_FUNC_API *ops;      //vbass 底层io
    void *work_buf;           //vbass 运行句柄及buf
    OS_MUTEX mutex;           //互斥锁
    u8 vbass_en: 1;           //模块使能 1:使能 0：不是能
    u8 update: 1;             //参数是否需要更新 1：是 0：否

    vbass_open_parm o_parm;   //打开传入的参数
    vbass_update_parm u_parm; //需要更新的参数
    struct audio_stream_entry entry;	// 音频流入口
} vbass_hdl;

/*cmd*/
enum {
    VBASS_UPDATE_PARM = 0,    //参数更新
    VBASS_SW,				  //运行过程开关
};
/*----------------------------------------------------------------------------*/
/**@brief   audio_vbass_open  虚拟低音打开
   @param    *_parm: 始化参数，详见结构体vbass_open_parm
   @return   句柄
   @note
*/
/*----------------------------------------------------------------------------*/
vbass_hdl *audio_vbass_open(vbass_open_parm *_parm);

/*----------------------------------------------------------------------------*/
/**@brief   audio_vbass_parm_update 虚拟低音参数更新
   @param    cmd:VBASS_UPDATE_PARM,VBASS_SW
   @param    *_parm:对应cmd的参数
   @return   0：成功  -1: 失败
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_vbass_parm_update(vbass_hdl *_hdl, u32 cmd, void *_parm);
// 参数更新例子
/* vbass_update_parm def_parm = {0}; */
// def_parm.bass_f = 300;
// def_parm.level = 8192;
/* audio_vbass_parm_update(vbass_hdl, VBASS_UPDATE_PARM, &def_parm); */

// 运行过程 做开关
// u8 en = 0;//关
/* audio_vbass_parm_update(vbass_hdl, VBASS_SW, (void *)en); */
// u8 en = 1;//开
/* audio_vbass_parm_update(vbass_hdl, VBASS_SW, (void *)en); */


/*----------------------------------------------------------------------------*/
/**@brief    audio_vbass_close 虚拟低音关闭处理
   @param    _hdl:句柄
   @return  0:成功  -1：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_vbass_close(vbass_hdl *_hdl);
#endif

