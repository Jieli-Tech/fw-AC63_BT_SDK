#ifndef reverb_echo_h__
#define reverb_echo_h__

#include "application/reverb_api.h"
/*
 * 打开声卡混响模块
 */
REVERBN_API_STRUCT *open_reverb(REVERBN_PARM_SET *reverb_seting, u16 sample_rate);
/*
 * 声卡混响处理
 */
void run_reverb(REVERBN_API_STRUCT *reverb_api_obj, short *in, short *out, int len);
/*
 * 关闭声卡混响模块
 */
void  close_reverb(REVERBN_API_STRUCT *reverb_api_obj);
/*
 * 暂停声卡混响处理
 */
void pause_reverb(REVERBN_API_STRUCT *reverb_api_obj, u8 run_mark);
/*
 * 声卡混响参数更新
 */
void update_reverb_parm(REVERBN_API_STRUCT *reverb_api_obj, REVERBN_PARM_SET *reverb_seting);


/*
 * 打开echo混响模块
 */
ECHO_API_STRUCT *open_echo(ECHO_PARM_SET *echo_seting, EF_REVERB_FIX_PARM *fix_parm);
/*
 * echo混响处理
 */
void run_echo(ECHO_API_STRUCT *p_echo_obj, short *in, short *out, int len);
/*
 * 关闭echo混响模块
 */
void  close_echo(ECHO_API_STRUCT *echo_api_obj);
/*
 * 暂停echo混响处理
 */
void  pause_echo(ECHO_API_STRUCT *echo_api_obj, u8 run_mark);
/*
 * 更新echo混响通用参数
 */
void update_echo_parm(ECHO_API_STRUCT *echo_api_obj, ECHO_PARM_SET *echo_seting);
/*
 * 更新echo混响gain参数
 */
void update_echo_gain(ECHO_API_STRUCT *echo_api_obj, int wetgain, int drygain);
#endif // reverb_echo_h__
