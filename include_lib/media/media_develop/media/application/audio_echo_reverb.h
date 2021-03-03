#ifndef reverb_echo_h__
#define reverb_echo_h__

#include "application/reverb_api.h"

REVERBN_API_STRUCT *open_reverb(REVERBN_PARM_SET *reverb_seting, u16 sample_rate);
void run_reverb(REVERBN_API_STRUCT *reverb_api_obj, short *in, short *out, int len);
void  close_reverb(REVERBN_API_STRUCT *reverb_api_obj);
void pause_reverb(REVERBN_API_STRUCT *reverb_api_obj, u8 run_mark);
void update_reverb_parm(REVERBN_API_STRUCT *reverb_api_obj, REVERBN_PARM_SET *reverb_seting);

// ECHO_API_STRUCT *open_echo(ECHO_PARM_SET *echo_seting, u16 sample_rate);
ECHO_API_STRUCT *open_echo(ECHO_PARM_SET *echo_seting, EF_REVERB_FIX_PARM *fix_parm);
void run_echo(ECHO_API_STRUCT *p_echo_obj, short *in, short *out, int len);
void  close_echo(ECHO_API_STRUCT *echo_api_obj);
void  pause_echo(ECHO_API_STRUCT *echo_api_obj, u8 run_mark);
void update_echo_parm(ECHO_API_STRUCT *echo_api_obj, ECHO_PARM_SET *echo_seting);
void update_echo_gain(ECHO_API_STRUCT *echo_api_obj, int wetgain, int drygain);
#endif // reverb_echo_h__
