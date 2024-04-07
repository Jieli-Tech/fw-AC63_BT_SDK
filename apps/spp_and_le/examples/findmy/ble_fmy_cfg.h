// binary representation
// attribute size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)

#ifndef _BLE_FMY_CFG_H
#define _BLE_FMY_CFG_H

#include <stdint.h>
#include "app_config.h"
#include "gatt_common/le_gatt_common.h"

//===============================================================
//配置debug模式还是production
#define  TOKEN_ID_PRODUCTION_MODE                     0 //0--debug, 1--production

//配置写入的token是base64格式还是HEX格式
#define  UUID_TOKEN_IS_BASE64_MODE                    0 //0--hex,   1--base64

#if(TOKEN_ID_PRODUCTION_MODE)
#define  FMY_DEBUG_SERVICE_ENABLE                     0
#else
#define  FMY_DEBUG_SERVICE_ENABLE                     1
#endif

//===============================================================
//firmware version config
#define  FMY_FW_VERSION_MAJOR_NUMBER                  1
#define  FMY_FW_VERSION_MINOR_NUMBER                  0
#define  FMY_FW_VERSION_REVISION_NUMBER               6

//===============================================================
//配置解绑后是否保留地址不变，默认是变化的
#define  FMY_MAC_CHANGE_LOCK                          0 // 0--unpair后修改mac，1--unpair后不修改mac


#endif
