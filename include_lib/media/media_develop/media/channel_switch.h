
#ifndef _CHANNEL_SWITCH_H_
#define _CHANNEL_SWITCH_H_

#include "media/audio_stream.h"

struct channel_switch {
    u8 out_ch_num;
    enum audio_channel out_ch_type;		// 声道类型
    struct audio_stream_entry entry;	// 音频流入口
    u8  data_sync;
    u16 dat_len;
    u16 dat_total;
    u16 buf_len;
    u8  buf[0];
};

struct channel_switch *channel_switch_open(enum audio_channel out_ch_type, int buf_len);
void channel_switch_close(struct channel_switch **);

#endif/*_AUDIO_SPLICING_H_*/
