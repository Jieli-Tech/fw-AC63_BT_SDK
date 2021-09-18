#ifndef _LE_AT_CHAR_COM_H
#define _LE_AT_CHAR_COM_H

#include <stdint.h>
#include "bt_common.h"

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
void ble_at_register_event_cbk(void *cbk);
int ble_at_adv_enable(u8 enable);
int ble_at_get_adv_state(void);
int ble_at_set_adv_interval(u16 value);
int ble_at_get_adv_interval(void);
u8 *ble_at_get_adv_data(u8 *len);
u8 *ble_at_get_rsp_data(u8 *len);


#endif

