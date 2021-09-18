#ifndef __LFWORDANA_ENC_API__H
#define __LFWORDANA_ENC_API__H
#include "application/lfwordana_enc.h"
#include "audio_enc.h"

void midi_w2s_parm_get(void *parm);
void midi_w2s_parm_set(void *data, u32 len);
void lfwordana_process();

/*midi支持外部音源替换主旋律, 0:不支持， 1：支持*/
#define MIDI_SUPPORT_W2S   0


extern int *MIDI_W2S_FILE_PATH;
#endif/* __LFWORDANA_ENC_API__H */

