#ifndef _AUDIO_DEC_MIDI_CTRL_H_
#define _AUDIO_DEC_MIDI_CTRL_H_
#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "audio_dec.h"
#include "app_main.h"
#include "asm/dac.h"
#include "clock_cfg.h"
#include "key_event_deal.h"
#include "midi_ctrl_decoder.h"

/*----------------------------------------------------------------------------*/
/**@brief    打开midi ctrl解码
   @param    sample_rate: 采样率
   @param    *path:音色文件路径
   @return   0：成功
   @return   非0：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int midi_ctrl_dec_open(u32 sample_rate, char *path);

/*----------------------------------------------------------------------------*/
/**@brief    关闭midi ctrl解码
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_dec_close(void);


/*----------------------------------------------------------------------------*/
/**@brief   乐器更新
   @param   prog:乐器号
   @param   trk_num :音轨 (0~15)
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_set_porg(u8 prog, u8 trk_num);

/*----------------------------------------------------------------------------*/
/**@brief   按键按下
   @param   nkey:按键序号（0~127）
   @param   nvel:按键力度（0~127）
   @param   chn :通道(0~15)
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_note_on(u8 nkey, u8 nvel, u8 chn);


/*----------------------------------------------------------------------------*/
/**@brief   按键松开
   @param   nkey:按键序号（0~127）
   @param   chn :通道(0~15)
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_note_off(u8 nkey, u8 chn);


/*----------------------------------------------------------------------------*/
/**@brief  midi 配置接口
   @param   cmd:命令
   @param   priv:对应cmd的结构体
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_confing(u32 cmd, void *priv);


/*----------------------------------------------------------------------------*/
/**@brief   midi keyboard 设置按键按下音符发声的衰减系数
   @param   obj:控制句柄
   @param   samp:对应samplerate_tab坐标
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_confing_set_melody_decay(u16 val);

/*----------------------------------------------------------------------------*/
/**@brief  弯音轮配置
   @param   pitch_val:弯音轮值,1 - 65535 ；256是正常值,对音高有作用
   @param   chn :通道(0~15)
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_pitch_bend(u16 pitch_val, u8 chn);

#endif
