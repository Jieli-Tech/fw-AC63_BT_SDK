#ifndef __MIDI_CTRL_DECODER_H__
#define __MIDI_CTRL_DECODER_H__


#include "media/includes.h"
#include "media/MIDI_CTRL_API.h"

typedef struct _midi_ctrl_open_parm {
    MIDI_CONFIG_PARM cfg_parm;               //初始化参数
    MIDI_CTRL_PARM   ctrl_parm;
    u32 sample_rate;
} midi_ctrl_open_parm;

struct set_prog_parm {
    u8 prog;                //乐器号
    u8 trk_num;             //音轨 (0~15)
};

struct note_on_parm {
    u8 nkey;                //按键序号（0~127）
    u8 nvel;                //按键力度（0~127）
    u8 chn;                 //通道(0~15)
};

struct note_off_parm {
    u8 nkey;                //按键序号（0~127）
    u8 chn;                 //通道(0~15)
};

struct pitch_bend_parm {
    u16 pitch_val;          //弯音轮值,1 - 65535 ；256是正常值,对音高有作用
    u8 chn;                 //通道(0~15)
};

enum {
///midi 模块接口内部相关消息
    MIDI_CTRL_NOTE_ON = 0xf0,
    MIDI_CTRL_NOTE_OFF,
    MIDI_CTRL_SET_PROG,
    MIDI_CTRL_PITCH_BEND,
};

struct midi_ctrl_decoder {
    u16 ch_num : 4;			// 声道数
    u16 output_ch_num : 4;	// 输出声道数
    u16 output_ch_type : 4;	// 输出声道类型
    u16 status;
    u16 sample_rate;	    // 采样率
    struct audio_decoder decoder;	// 解码句柄
};

// 打开midi_ctrl解码
int midi_ctrl_decoder_open(struct midi_ctrl_decoder *, struct audio_decoder_task *decode_task);
// 关闭midi_ctrl解码
void midi_ctrl_decoder_close(struct midi_ctrl_decoder *dec);
// 设置midi_ctrl解码事件回调接口
void midi_ctrl_decoder_set_event_handler(struct midi_ctrl_decoder *dec,
        void (*handler)(struct audio_decoder *, int, int *), u32 maigc);

#endif /*midi_ctrl_DECODER_H*/

