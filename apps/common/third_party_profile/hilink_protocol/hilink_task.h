#ifndef __HILINK_TASK_H__
#define __HILINK_TASK_H__

#include "typedef.h"
#include "os/os_api.h"
#include "circular_buf.h"
#include "hilink_ota.h"

typedef struct __HILINK_PACKET_HEAD {
    u8 packet_channel;
    u16 len;
} HILINK_PACKET_HEAD_T;

typedef struct __HILINK_MSG_T {
    uint8_t cmd_type: 4;
    uint8_t version: 4;
    uint8_t msg_id;
    uint8_t total_frame;
    uint8_t frame_seq;
    uint8_t rev;
    uint8_t encry;
    uint8_t ret;
    uint8_t data[0];
} HILINK_MSG_T;


typedef struct __HILINK_PACKET_CONTROL {
    OS_SEM               hilink_sem;
    cbuffer_t            cbuf;
    volatile u8          wait;
    u8                   *hilink_buf;
} HILINK_PACKET_CONTROL;

#define HILINK_PACKET_HEAD_LEN      (sizeof(HILINK_PACKET_HEAD_T))
#define HILINK_MSG_HEAD_LEN         (sizeof(HILINK_MSG_T))
#define HILINK_MTU_SIZE             (517)
#define HILINK_RESERVED_PACKET_CNT  (3)
#define HILINK_CTL_RESERVED_SIZE    ((HILINK_MSG_HEAD_LEN + HILINK_MTU_SIZE) * HILINK_RESERVED_PACKET_CNT)
#define HILINK_OTA_RESERVED_SIZE    (MAX_CACHE_CNT * OTA_BUF_SIZE)

#define HILINK_MSG_POOL_SIZE        (HILINK_CTL_RESERVED_SIZE > HILINK_OTA_RESERVED_SIZE ? HILINK_CTL_RESERVED_SIZE : HILINK_OTA_RESERVED_SIZE)

enum {
    HILINK_DEVICE_INFO_MSG_CH = 0,
    HILINK_DATA_MSG_CH,
    HILINK_OTA_CTL_MSG_CH,
    HILINK_OTA_DATA_MSG_CH,
};

int  hilink_task_init(void);
void hilink_packet_recieve(void *buf, u16 len);

#endif //__HILINK_TASK_H__
