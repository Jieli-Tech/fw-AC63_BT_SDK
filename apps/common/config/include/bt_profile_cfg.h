
#ifndef _BT_PROFILE_CFG_H_
#define _BT_PROFILE_CFG_H_

#include "app_config.h"
#include "btcontroller_modules.h"


#if (DUEROS_DMA_EN || TRANS_DATA_EN || RCSP_BTMATE_EN || RCSP_ADV_EN || GMA_EN || SMART_BOX_EN || TME_EN ||ANCS_CLIENT_EN || XM_MMA_EN)
#define    BT_FOR_APP_EN             1
#else
#define    BT_FOR_APP_EN             0
#endif


///---sdp service record profile- 用户选择支持协议--///
#if (BT_FOR_APP_EN || APP_ONLINE_DEBUG)
#undef USER_SUPPORT_PROFILE_SPP
#define USER_SUPPORT_PROFILE_SPP    1
#endif

//ble demo的例子
#define DEF_BLE_DEMO_NULL                 0 //ble 没有使能
#define DEF_BLE_DEMO_ADV                  1 //only adv,can't connect
#define DEF_BLE_DEMO_TRANS_DATA           2 //
#define DEF_BLE_DEMO_DUEROS_DMA           3 //
#define DEF_BLE_DEMO_RCSP_DEMO            4 //
#define DEF_BLE_DEMO_ADV_RCSP             5
#define DEF_BLE_DEMO_GMA                  6
#define DEF_BLE_DEMO_CLIENT               7 //
#define DEF_BLE_TME_ADV					  8
#define DEF_BLE_ANCS_ADV				  9
#define DEF_BLE_DEMO_MI 				  10

//配置选择的demo
#if TCFG_USER_BLE_ENABLE

#if DUEROS_DMA_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_DUEROS_DMA

#elif (SMART_BOX_EN | RCSP_BTMATE_EN)
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_RCSP_DEMO

#elif TRANS_DATA_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_TRANS_DATA

#elif RCSP_ADV_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_ADV_RCSP

#elif GMA_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_GMA

#elif TME_EN
#define TCFG_BLE_DEMO_SELECT		  DEF_BLE_TME_ADV

#elif BLE_CLIENT_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_CLIENT

#elif ANCS_CLIENT_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_ANCS_ADV

#elif XM_MMA_EN
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_MI

#else
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_ADV
#endif

#else
#define TCFG_BLE_DEMO_SELECT          DEF_BLE_DEMO_NULL//ble is closed
#endif

//配对加密使能
#define TCFG_BLE_SECURITY_EN          0



#endif
