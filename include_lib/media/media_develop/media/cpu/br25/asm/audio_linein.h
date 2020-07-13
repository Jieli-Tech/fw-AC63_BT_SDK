#ifndef _AUDIO_LINEIN_H_
#define _AUDIO_LINEIN_H_

#include "generic/typedef.h"
#include "asm/audio_adc.h"

/*
 *ch:使能对应的输入通道(控制输入)
 *amux_en:是否将linein输入直通到dac(控制输出)
 */
int audio_linein0_open(u8 ch, u8 amux_en);
int audio_linein0_close(u8 ch, u8 amux_en);
int audio_linein1_open(u8 ch, u8 amux_en);
int audio_linein1_close(u8 ch, u8 amux_en);
int audio_linein2_open(u8 ch, u8 amux_en);
int audio_linein2_close(u8 ch, u8 amux_en);
int audio_linein_via_dac_open(u8 ch, u8 amux_en);
int audio_linein_via_dac_close(u8 ch, u8 amux_en);

/*
 *linein mute控制
 */
void audio_linein_mute(u8 mute);

/*
 *linein输入级增益控制，只有0或者1两级
 *0:正常增益
 *1:增益X2
 */
void audio_linein_gain(u8 gain);

void audio_linein_bias(u8 ch, u8 en);
void audio_linein_amux_bias(u8 amux_bias);

/*
 *linein单声道模拟输入的时候，如果dac是两个声道的，想要
 *两个声道都有声音出来，需要将amux通道合并一下，即：
 *call:audio_linein_ch_combine(1,1);
 */
void audio_linein_ch_combine(u8 LR_2_L, u8 LR_2_R);

#endif
