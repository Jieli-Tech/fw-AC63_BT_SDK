/*
 ****************************************************************
 *File : audio_splicing.h
 *note : l-双声道0左
 *       r-双声道1右
 *       fl/q0-四声道0左
 *       fr/q1-四声道1右
 *       rl/q2-四声道2左
 *       rr/q3-四声道3右
 *       N-数据不变
 *
 ****************************************************************
 */

#ifndef _AUDIO_SPLICING_H_
#define _AUDIO_SPLICING_H_

#include "generic/typedef.h"

// 单声道转双声道
void pcm_single_to_dual(void *out, void *in, u16 len);
// 单声道转四声道
void pcm_single_to_qual(void *out, void *in, u16 len);
// 双声道转四声道（L,R,L,R）
void pcm_dual_to_qual(void *out, void *in, u16 len);
// 双声道合并（L+R,L+R）
void pcm_dual_mix_to_dual(void *out, void *in, u16 len);
// 双声道转单声道（L+R）
void pcm_dual_to_single(void *out, void *in, u16 len);
// 四声道转单声道（q0+q1+q2+q3）
void pcm_qual_to_single(void *out, void *in, u16 len);
// 四声道转双声道（q0+q2,q1+q3）
void pcm_qual_to_dual(void *out, void *in, u16 len);
// 两个单声道合并成双声道
void pcm_single_l_r_2_dual(void *out, void *in_l, void *in_r, u16 in_len);
// 四个单声道合并成四声道
void pcm_fl_fr_rl_rr_2_qual(void *out, void *in_fl, void *in_fr, void *in_rl, void *in_rr, u16 in_len);
// 两个双声道合并成四声道（fl,fr,rl,rr）
void pcm_flfr_rlrr_2_qual(void *out, void *in_flfr, void *in_rlrr, u16 in_len);
// 双声道填到四声道的01（fl,fr,N,N）
void pcm_fill_flfr_2_qual(void *out, void *in_flfr, u16 in_len);
// 双声道填到四声道的23（N,N,rl,rr）
void pcm_fill_rlrr_2_qual(void *out, void *in_rlrr, u16 in_len);
// 输入数据合并到输出
void pcm_mix_buf(s32 *obuf, s16 *ibuf, u16 len);
// 音频数据溢出处理
void pcm_mix_buf_limit(s16 *obuf, s32 *ibuf, u16 len);
// 双声道数据转换成单声道或者其他双声道
void pcm_dual_to_dual_or_single(u8 ch_type, u8 half_lr, s16 *odata, s16 *idata, int len);

#endif/*_AUDIO_SPLICING_H_*/
