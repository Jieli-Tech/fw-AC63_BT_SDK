// binary representation
// attribute size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)

#ifndef _BLE_MUTIL_H
#define _BLE_MUTIL_H

#include <stdint.h>
#include "app_config.h"

void multi_server_init(void);
void multi_server_exit(void);
void multi_client_init(void);
void multi_client_exit(void);
int multi_client_clear_pair(void);
int multi_server_clear_pair(void);


#endif
