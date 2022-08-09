#ifndef _LE_AT_CLIENT_H
#define _LE_AT_CLIENT_H

// #if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_AT_CLIENT)
#if CONFIG_APP_AT_COM

#include <stdint.h>
#include "at.h"


#define   AT_UART_FIFIO_BUFFER_SIZE  1024

int ble_at_set_address(u8 *addr);
int ble_at_set_name(u8 *name, u8 len);
u8 ble_at_get_staus(void);
int ble_at_disconnect(void);
int ble_at_get_address(u8 *addr);
int ble_at_get_name(u8 *name);
int bt_ble_scan_enable(void *priv, u32 en);
void client_create_connection(u8 *conn_addr, u8 addr_type);
void client_create_connection_cancel();
void client_search_profile(u8 search_type, u8 *UUID) ;
void client_receive_ccc(u16 value_handle, u8 ccc_type);
int client_read_long_value(u8 *read_handle);

int client_write(u16 write_handle, u8 *data, u8 len);

int client_write_without_respond(u16 write_no_respond_handle, u8 *data, u16 len);
void client_connect_param_update(u16 interval_min, u16 interval_max, u16 latency, u16 timeout);
void ble_at_scan_param(u16 scan_interval, u16 scan_window);

extern void at_send_event(u8 opcode, const u8 *packet, int size);

#endif

#endif // _LE_AT_CLIENT_H

