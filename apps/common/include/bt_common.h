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


// #define SYS_BT_EVENT_FORM_COMMON       (('C' << 24) | ('M' << 16) | ('M' << 8) | '\0')
enum {
    COMMON_EVENT_EDR_REMOTE_TYPE = 1,
    COMMON_EVENT_BLE_REMOTE_TYPE,
};


extern uint16_t little_endian_read_16(const uint8_t *buffer, int pos);
extern uint32_t little_endian_read_24(const uint8_t *buffer, int pos);
extern uint32_t little_endian_read_32(const uint8_t *buffer, int pos);
extern void swapX(const uint8_t *src, uint8_t *dst, int len);


//common api
extern void bt_ble_init(void);
extern void bt_ble_exit(void);
extern void bt_ble_adv_enable(u8 enable);
extern void ble_app_disconnect(void);
extern void ble_module_enable(u8 en);
extern const u8 *bt_get_mac_addr();
extern void lib_make_ble_address(u8 *ble_address, u8 *edr_address);
extern void user_spp_data_handler(u8 packet_type, u16 ch, u8 *packet, u16 size);
extern void bt_get_vm_mac_addr(u8 *addr);
extern void set_remote_test_flag(u8 own_remote_test);
extern void bredr_power_get(void);
extern void bredr_power_put(void);
extern void radio_set_eninv(int v);
extern void transport_spp_init(void);
extern const char *bt_get_local_name();
extern void wdt_clear(void);
extern u8 ble_update_get_ready_jump_flag(void);
extern void reset_PK_cb_register(void (*reset_pk)(u32 *));
extern void att_server_flow_enable(u8 enable);
extern void le_device_db_init(void);
extern bool get_remote_test_flag();

#endif
