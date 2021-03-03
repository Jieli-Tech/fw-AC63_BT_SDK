
#ifndef _AUDIO_VBASS_API_H_
#define _AUDIO_VBASS_API_H_
#include "vbass/vbass_api.h"

#define VBASS_USE_AUDIO_STREAM   0 //是否使用audio_sream流节点
typedef struct _vbass_open_parm {
    u16 sr; //输入音频采样率
    u8 channel;//输入音频声道数
} vbass_open_parm;

typedef struct _vbass_update_parm {
    int bass_f;//外放的低音截止频率
    int level;//增强强度(4096 等于 1db， 建议范围：4096 到 16384)
} vbass_update_parm;


typedef struct _vbass_hdl {
    VBASS_FUNC_API *ops;
    void *work_buf;
    OS_MUTEX mutex;
    u8 vbass_en: 1;
    u8 update: 1;

    vbass_open_parm o_parm;
    vbass_update_parm u_parm;
#if VBASS_USE_AUDIO_STREAM
    struct audio_stream_entry entry;	// 音频流入口
#endif
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
/**@brief   虚拟低音数据处理
   @param    data:数据地址
   @parm    len:数据长度
   @return   处理的长度
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_vbass_run(vbass_hdl *_hdl, void *data, u32 len);
/*----------------------------------------------------------------------------*/
/**@brief    audio_vbass_close 虚拟低音关闭处理
   @param    _hdl:句柄
   @return  0:成功  -1：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int audio_vbass_close(vbass_hdl *_hdl);
#endif

