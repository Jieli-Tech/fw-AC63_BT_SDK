#ifndef __LF_WORDANA_ENC__
#define __LF_WORDANA_ENC__
#include "system/includes.h"
#include "media/audio_encoder.h"
#include "generic/includes.h"
#include "media/w2s_handle_api.h"
#include "asm/audio_adc.h"

struct lfwordana_encoder {
    void *buf_ptr;
    w2s_ana_ops *ops;
    REC_W2S_STUCT w2s_parmIn;
    MIDI_W2S_STRUCT w2s_parmOut;
};

struct lfwordana_encoder *lfwordana_encoder_open(REC_W2S_STUCT *parm);
int lfwordana_encoder_run(struct lfwordana_encoder *ana, void *data, u32 len);
void lfwordana_encoder_close(struct lfwordana_encoder *ana);


#endif /* __LF_WORDANA_ENC__ */
