// binary representation
// attribute size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)

#ifndef _LE_MUTIL_COMMON_H
#define _LE_MUTIL_COMMON_H

#include <stdint.h>
#include "app_config.h"

#if TRANS_MULTI_BLE_EN

#define MULTI_ROLE_CLIENT         1
#define MULTI_ROLE_SERVER         0

#define MULTI_ROLE_MASTER         1
#define MULTI_ROLE_SLAVE          0

//---------------------------------
#define SUPPORT_MAX_SERVER       TRANS_MULTI_BLE_SLAVE_NUMS
#define SUPPORT_MAX_CLIENT       TRANS_MULTI_BLE_MASTER_NUMS
//---------------------------------
//----------------------------------------------------------------------------------------
extern u16 server_con_handle[];
extern u16 client_con_handle[];


s8 mul_get_dev_index(u16 handle, u8 role);
s8 mul_get_idle_dev_index(u8 role);
s8 mul_del_dev_index(u16 handle, u8 role);
bool mul_dev_have_connected(u8 role);
u16 mul_dev_get_conn_handle(u8 index, u8 role);

void bt_multi_trans_init(void);
void bt_multi_trans_exit(void);
void bt_multi_client_init(void);
void bt_multi_client_exit(void);
void trans_cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
void client_cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

void client_profile_init(void);
void server_profile_init(void);
void ble_trans_module_enable(u8 en);
void ble_client_module_enable(u8 en);
void ble_multi_trans_disconnect(void);
void ble_multi_client_disconnect(void);

#endif

#endif
