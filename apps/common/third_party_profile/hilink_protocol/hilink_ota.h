#ifndef __HILINK_OTA_H__
#define __HILINK_OTA_H__

#include "typedef.h"

#include "hilink_protocol.h"

#define MAX_CACHE_CNT           (10)    //目前只有10可以正常OTA
#define OTA_BUF_SIZE            (162)
#define OTA_DATA_SIZE           (OTA_BUF_SIZE - 2)
#define FIRST_OTA_DATA_SIZE     (OTA_BUF_SIZE - 11)
#define OTA_WRITE_BUF_SIZE      (OTA_DATA_SIZE * MAX_CACHE_CNT)
#define OTA_BUF_TIMEOUT         (1000)

// APP->DEVICE
#define HILINK_OTA_CTL_MAX_CACHE_CNT        0x0001
#define HILINK_OTA_CTL_HEAD_INFO            0x0002
#define HILINK_OTA_CTL_DEV_INFO_REQ         0x0004

// DEVICE->APP
#define HILINK_OTA_CTL_ERROR_RSP            0x8000
#define HILINK_OTA_CTL_MAX_CACHE_CNT_RSP    0x8001
#define HILINK_OTA_CTL_NEGOTIATE_RESULT     0x8002
#define HILINK_OTA_CTL_UPDATE_RESULT        0x8003
#define HILINK_OTA_CTL_DEV_INFO_RPT         0x8004
#define HILINK_OTA_CTL_SEG_RSP              0x8081

#define HILINK_OTA_NEGOTIATE_PID        0x00
#define HILINK_OTA_NEGOTIATE_FIRM_VER   0x01
#define HILINK_OTA_NEGOTIATE_FIRM_SIZE  0x02
#define HILINK_OTA_NEGOTIATE_MCU_VER    0x03
#define HILINK_OTA_NEGOTIATE_MCU_SIZE   0x04

#define HILINK_OTA_ERROR_NONE           0x00
#define HILINK_OTA_ERROR_VER_SAME       0x01
#define HILINK_OTA_ERROR_PID_NO_MATCH   0x02
#define HILINK_OTA_ERROR_SW_VER_ERR     0x03
#define HILINK_OTA_ERROR_FW_VER_ERR     0x04
#define HILINK_OTA_ERROR_HW_VER_ERR     0x05
#define HILINK_OTA_ERROR_FILE_SIZE_ERR  0x06

typedef struct {
    uint16_t len;
    uint16_t data_index;
    uint8_t *data;
} hilink_hash_list_t;

typedef struct {
    uint16_t len;
    uint16_t data_index;
    uint8_t *data;
} hilink_signature_t;

typedef struct {
    uint32_t file_size;
    uint32_t recv_len;
    uint32_t buff_size;
    uint16_t current_msg_type;
    uint16_t expect_seg_id;
    uint16_t ota_timer;
    uint8_t ota_ready;
    uint8_t ota_bit_map[8];
    uint8_t *hilink_ota_store_buf;
} hilink_ota_t;

typedef struct {
    uint8_t type;
    uint8_t len;
    uint8_t value[0];
} __ota_tlv;

typedef struct {
    char *module_device_name;
    char *module_sub_device_name;
    char *device_type;
    char *device_id;
    char *model_firmware_version;
    char *mcu_device_name;
    char *mcu_sub_device_name;
    char *mcu_device_type;
    char *mcu_device_id;
    char *mcu_firmware_version;
} __ota_device_info;

#endif //__HILINK_OTA_H__
