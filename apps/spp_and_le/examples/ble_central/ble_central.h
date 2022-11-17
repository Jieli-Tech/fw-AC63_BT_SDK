// binary representation
// attribute size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)

#ifndef _BLE_CENTRAL_H
#define _BLE_CENTRAL_H

#include <stdint.h>
#include "app_config.h"
#include "gatt_common/le_gatt_common.h"

void central_server_init(void);
void central_set_server_conn_handle(u16 conn_handle);
extern const gatt_server_cfg_t central_server_init_cfg;

#endif
