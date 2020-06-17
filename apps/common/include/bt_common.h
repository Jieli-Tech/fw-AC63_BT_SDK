// binary representation
// attribute size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)

#ifndef _BT_COMMON_H
#define _BT_COMMON_H
#include <stdint.h>
#include "app_config.h"

#ifdef APP_PRIVATE_PROFILE_CFG
#include "lib_profile_cfg.h"
#else
#include "bt_profile_cfg.h"
#endif

enum {
    ST_BIT_INQUIRY = 0,
    ST_BIT_PAGE_SCAN,
    ST_BIT_BLE_ADV,
    ST_BIT_SPP_CONN,
    ST_BIT_BLE_CONN,
    ST_BIT_WEIXIN_CONN,
};

extern uint16_t little_endian_read_16(const uint8_t *buffer, int pos);
extern uint32_t little_endian_read_24(const uint8_t *buffer, int pos);
extern uint32_t little_endian_read_32(const uint8_t *buffer, int pos);
extern void swapX(const uint8_t *src, uint8_t *dst, int len);


//common api
extern void bt_ble_init(void);
extern void bt_ble_exit(void);
extern void bt_ble_adv_enable(u8 enable);

#endif
