

#ifndef _3TH_PROFILE_API_H
#define _3TH_PROFILE_API_H

#include<string.h>
#include <stdint.h>
#include "le_common.h"

enum {
    TWS_AI_CONN_STA = 0,
    TWS_AI_DISCONN_STA,
    TWS_AI_SPEECH_START,
    TWS_AI_SPEECH_STOP,
};


#define TYPE_NULL        0
#define TYPE_BLE         1
#define TYPE_SPP         2

#define TYPE_MASTER_BLE  3
#define TYPE_SLAVE_BLE   4

#define TYPE_MASTER_SPP  5
#define TYPE_SLAVE_SPP   6

#define SOURCE_TYPE        0
#define SINK_TYPE_MASTER   1
#define SINK_TYPE_SLAVE    2

void set_app_connect_type(u8 type);
u8 get_app_connect_type(void);
u8 get_ble_connect_type(void);
void set_ble_connect_type(u8 type);

void tws_mic_speech_msg_deal(u8 event, u32 event_type);
#endif
