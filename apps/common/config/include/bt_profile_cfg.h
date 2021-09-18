
#ifndef _BT_PROFILE_CFG_H_
#define _BT_PROFILE_CFG_H_

#include "app_config.h"
#include "btcontroller_modules.h"


#if (TRANS_DATA_EN || RCSP_BTMATE_EN || RCSP_ADV_EN || SMART_BOX_EN || ANCS_CLIENT_EN || LL_SYNC_EN || TUYA_DEMO_EN)
#ifndef BT_FOR_APP_EN
#define    BT_FOR_APP_EN             1
#endif
#else
#ifndef BT_FOR_APP_EN
#define    BT_FOR_APP_EN             0
#endif
#ifndef AI_APP_PROTOCOL
#define    AI_APP_PROTOCOL             0
#endif
#endif


///---sdp service record profile- 用户选择支持协议--///
#if (BT_FOR_APP_EN || APP_ONLINE_DEBUG || AI_APP_PROTOCOL)
#if (LL_SYNC_EN || TUYA_DEMO_EN)
#undef USER_SUPPORT_PROFILE_SPP
#define USER_SUPPORT_PROFILE_SPP    0
#else
#undef USER_SUPPORT_PROFILE_SPP
#define USER_SUPPORT_PROFILE_SPP    1
#endif
#endif

//ble demo的例子
#define DEF_BLE_DEMO_NULL                 0 //ble 没有使能
#define DEF_BLE_DEMO_ADV                  1 //only adv,can't connect
#define DEF_BLE_DEMO_TRANS_DATA           2 //
#define DEF_BLE_DEMO_RCSP_DEMO            4 //
#define DEF_BLE_DEMO_ADV_RCSP             5
#define DEF_BLE_DEMO_CLIENT               7 //
#define DEF_BLE_ANCS_ADV				  9
#define DEF_BLE_DEMO_MULTI                11 //
#define DEF_BLE_DEMO_LL_SYNC              13 //
#define DEF_BLE_DEMO_WIRELESS_MIC_SERVER  14 //
#define DEF_BLE_DEMO_WIRELESS_MIC_CLIENT  15 //

//配置选择的demo
#if TCFG_USER_BLE_ENABLE

#if (SMART_BOX_EN | RCSP_BTMATE_EN)
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_RCSP_DEMO

#elif TRANS_DATA_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_TRANS_DATA

#elif LL_SYNC_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_LL_SYNC

#elif RCSP_ADV_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_ADV_RCSP



#elif BLE_CLIENT_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_CLIENT

#elif TRANS_MULTI_BLE_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_MULTI

#elif ANCS_CLIENT_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_ANCS_ADV

#elif AI_APP_PROTOCOL
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_NULL

#elif (BLE_WIRELESS_MIC_CLIENT_EN)
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_WIRELESS_MIC_CLIENT

#elif (BLE_WIRELESS_MIC_SERVER_EN)
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_WIRELESS_MIC_SERVER

#else
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_ADV
#endif

#else
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_NULL//ble is closed
#endif

//配对加密使能
#define TCFG_BLE_SECURITY_EN          config_le_sm_support_enable



#endif
