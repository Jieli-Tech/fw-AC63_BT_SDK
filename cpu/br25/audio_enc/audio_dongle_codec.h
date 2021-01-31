#ifndef _AUDIO_DONGLE_CODEC_H_
#define _AUDIO_DONGLE_CODEC_H_


#include "generic/typedef.h"
#include "generic/list.h"
#include "media/audio_base.h"
#include "media/audio_stream.h"
#include "media/mixer.h"

#define DONGLE_OUTPUT_SAMPLE_RATE		16000

struct dongle_emitter_hdl {
    struct audio_mixer mixer;
    struct audio_mixer_ch mix_ch;
    struct audio_stream_entry entry;	// 音频流入口
};
extern struct dongle_emitter_hdl dongle_emitter;


void audio_dongle_emitter_init(void);
void audio_usbdongle_ctrl(void);

#endif

