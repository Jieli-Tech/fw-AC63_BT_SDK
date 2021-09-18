#ifndef _LE_AT_CHAR_CLIENT_H
#define _LE_AT_CHAR_CLIENT_H

#if CONFIG_APP_AT_CHAR_COM

#include <stdint.h>
#include "at.h"

int bt_ble_scan_enable(void *priv, u32 en);
static int client_create_connection(u8 *conn_addr, u8 addr_type);
static int client_create_connection_cancel();
int client_read_long_value(u8 *read_handle);

int client_write(u16 write_handle, u8 *data, u8 len);

int client_write_without_respond(u16 write_no_respond_handle, u8 *data, u16 len);
void client_connect_param_update(u16 interval_min, u16 interval_max, u16 latency, u16 timeout);
int ble_at_client_scan_param(u16 scan_interval, u16 scan_window);

//extern void at_send_event(u8 opcode, const u8 *packet, int size);

int le_at_client_scan_enable(u8 enable);
int le_at_client_disconnect(u8 id);
int le_at_client_get_conn_param(u16 *conn_param);
int le_at_client_set_conn_param(u16 *conn_param);
int le_at_client_set_target_uuid16(u16 uuid16);
int le_at_client_get_target_uuid16(void);
int le_at_client_set_channel(u8 cid);

#endif

#endif // _LE_AT_CLIENT_H


