// binary representation
// attribute size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)

#ifndef _BLE_TRANS_H
#define _BLE_TRANS_H

#include <stdint.h>
#include "app_config.h"
#include "gatt_common/le_gatt_common.h"

extern const gatt_client_cfg_t trans_client_init_cfg;
void trans_client_init(void);
void trans_client_exit(void);
int trans_client_search_remote_profile(u16 conn_handle);
int trans_client_search_remote_stop(u16 conn_handle);

void trans_ios_services_init(void);
void trans_ios_services_exit(void);
#endif
