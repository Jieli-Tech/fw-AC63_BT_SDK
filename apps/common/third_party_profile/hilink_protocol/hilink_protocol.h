#ifndef _HILINK_PROTOCOL_H
#define _HILINK_PROTOCOL_H
#include "typedef.h"
#include "btstack/third_party/app_protocol_event.h"
#include "md.h"
#include "gcm.h"
#include "cJSON.h"

#define HILINK_MCU  0

#define AES128_KEY_LENGTH 128
#define GCM_TAG_LEN 16

#define CMD_TYPE_REQ    0
#define CMD_TYPE_RSP    1
#define CMD_TYPE_RPT    2

#define MSG_WITHOUT_ENCRY   0
#define MSG_ENCRY           1

void hilink_msg_handle(uint8_t *buf, uint16_t len);

typedef struct {
    uint8_t msg_id;
    uint16_t data_len;
    uint16_t data_index;
    uint8_t *data;
} hi_msg_store_t;

typedef struct {
    uint8_t onoff;
} hi_attribute_t;

typedef struct {
    uint8_t hilink_authcode[16];
    uint8_t hilink_sessionkey[16];
    uint8_t sessid[32];
    uint8_t hilink_authcodeid_tmp[33]; //32 + 1多一位用于放字符串末尾的\0
    uint8_t hilink_hmackey[32];
    char    time[13];
    uint8_t hilink_pair_flag;
    hi_attribute_t hilink_attr;
} hi_auth_info_t;

//hilink私有消息
typedef struct {
    const char *prodId;         /**<设备HiLink认证号，长度范围（0,5]*/
    const char *sn;				/**<设备唯一标识，比如sn号，长度范围（0,40]*/
    const char *dev_id;
    const char *model;			/**<设备型号，长度范围（0,32]*/
    const char *dev_t;			/**<设备类型，长度范围（0,4]*/
    const char *manu;			/**<设备制造商，长度范围（0,4]*/
    const char mac[18];			/**<设备MAC地址，字符串形式固定18字节*/
    const char *hiv;			/**<设备Hilink协议版本，长度范围（0,32]*/
    const char *fwv;			/**<设备固件版本，长度范围[0,64]*/
    const char *hwv;			/**<设备硬件版本，长度范围[0,64]*/
    const char *swv;			/**<设备软件版本，长度范围[0,64]*/
    const char *prot_t;			/**<设备协议类型，取值范围[1,3]*/
} dev_info_t;

#endif
