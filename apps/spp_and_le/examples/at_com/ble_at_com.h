// binary representation
// attribute size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)

#ifndef _LE_AT_COM_H
#define _LE_AT_COM_H

#include <stdint.h>
#include "bt_common.h"

#if CONFIG_APP_AT_COM

int ble_at_set_address(u8 *addr);
int ble_at_get_address(u8 *addr);
int ble_at_set_name(u8 *name, u8 len);
int ble_at_get_name(u8 *name);
int ble_at_set_visibility(u8 en);
int ble_at_send_data(u8 *data, u8 len);
int ble_at_send_data_default(u8 *data, u8 len);
int ble_at_set_pair_mode(u8 mode);
int ble_at_disconnect(void);
int ble_at_confirm_gkey(u8 *key_info);
int ble_at_set_adv_data(u8 *data, u8 len);
int ble_at_set_rsp_data(u8 *data, u8 len);
u8 ble_at_get_staus(void);
void ble_at_register_event_cbk(void *cbk);



#endif
#endif
