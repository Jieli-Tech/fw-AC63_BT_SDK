#ifndef MIDI_CTRL_API_h__
#define MIDI_CTRL_API_h__

#include "MIDI_DEC_API.h"

#include "typedef.h"


typedef struct _EVENT_CONTEXT_ {
    u8 msg;
    u8 val1;
    u8 val2;
    u8 trk_num;
    u16 delta;
} EVENT_CONTEXT;

typedef struct _MIDI_CTRL_PARM_ {
    char track_num;                //0-15支持音轨的最大个数
    unsigned int tempo;             //tempo
    void *priv;                    //模块内部使用
    u32(*output)(void *priv, void *data, int len);//模块内部使用
} MIDI_CTRL_PARM;

typedef struct _MIDI_CTRL_CONTEXT_ {
    u32(*need_workbuf_size)() ;		                                   //<获取需要的buffer
    u32(*open)(void *work_buf, void *dec_parm, void *parm);                //跟解码一样的配置
    u32(*run)(void *work_buf);                                             //播放
    u32(*set_prog)(void *work_buf, u8 prog, u8 trk_num);                   //设置乐器号， 音轨0~15
    u32(*note_on)(void *work_buf, u8 nkey, u8 nvel, u8 chn);               //指定播放单个音符,nkey跟nvel的有效值是0-127,chn 0~15
    u32(*note_off)(void *work_buf, u8 nkey, u8 chn);                       //指定播放单个音符,nkey跟nvel的有效值是0-127
    u32(*pitch_bend)(void *work_buf, u16 pitch_val, u8 chn);               //弯音轮值pitch_val  1 - 65535 ；256是正常值,对音高有作用
    u32(*ctl_confing)(void *work_buf, u32 cmd, void *parm);
} MIDI_CTRL_CONTEXT;


#define MIDI_CTRLC_NOTEOFF  0x80                        //按键松开
#define MIDI_CTRLC_NOTEON   0x90                        //按键按下
#define MIDI_CTRLC_CTLCHG   0xB0
#define MIDI_CTRLC_PRGCHG   0xC0                        //改变乐器
#define MIDI_CTRLC_PWCHG    0xE0

#define MIDI_CTRLC_VOL     0x07
#define MIDI_CTRLC_EXPR    0x0B
#define MIDI_CTRLC_SFT_ON  0x43



extern MIDI_CTRL_CONTEXT *get_midi_ctrl_ops();


#endif // MIDI_CTRL_API_h__

