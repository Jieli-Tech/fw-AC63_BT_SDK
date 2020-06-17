
#ifndef _LIB_PROFILE_CFG_H_
#define _LIB_PROFILE_CFG_H_

#include "app_config.h"
#include "btcontroller_modules.h"

#define BT_BTSTACK_CLASSIC                   BIT(0)
#define BT_BTSTACK_LE_ADV                    BIT(1)
#define BT_BTSTACK_LE                        BIT(2)

extern const int config_stack_modules;
#define STACK_MODULES_IS_SUPPORT(x)         (config_stack_modules & (x))

///---sdp service record profile- 用户选择支持协议--///
#define USER_SUPPORT_PROFILE_HID          1

//ble demo的例子
#define DEF_BLE_DEMO_NULL                 0 //ble 没有使能
#define DEF_BLE_DEMO_HOGP                 1 //
#define DEF_BLE_DEMO_CLIENT               2 //

//配置选择的demo
#if TCFG_USER_BLE_ENABLE
// #define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_CLIENT
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_HOGP
#else
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_NULL//ble is closed
#endif /* TCFG_USER_BLE_ENABLE */

//配对加密使能
#define TCFG_BLE_SECURITY_EN              0

#endif
