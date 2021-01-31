/*********************************************************************************************
    *   Filename        : btcontroller_modules.h

    *   Description     : Lto 优化Macro 定义

    *   Author          : Bingquan

    *   Email           : bingquan_cai@zh-jieli.com

    *   Last modifiled  : 2018-12-19 16:38

    *   Copyright:(c)JIELI  2011-2017  @ , All Rights Reserved.
*********************************************************************************************/
#ifndef _BTCONTROLLER_H_
#define _BTCONTROLLER_H_

#include "hci_transport.h"
#include "btcontroller_mode.h"

#include "ble/hci_ll.h"

#include "classic/hci_lmp.h"

/*
 *-------------------Module SUPPORT
 *  brief : 运行时优化（LTO）下，代码空间优化；
 */
#define BT_MODULE_CLASSIC                   BIT(0)
#define BT_MODULE_LE                        BIT(1)

extern const int config_btctler_modules;
#define BT_MODULES_IS_SUPPORT(x)            (config_btctler_modules & (x))
/*-----------------------------------------------------------*/
extern const int config_stack_modules;
#define STACK_MODULES_IS_SUPPORT(x)         (config_stack_modules & (x))

/*
 *-------------------Mode SELECT
 *  brief : 运行时优化（LTO）下，代码空间优化；
 */
extern const int config_btctler_mode;
#define BT_MODE_IS(x)            (config_btctler_mode & (x))

/*-----------------------------------------------------------*/

extern const int config_btctler_hci_standard;
#define BT_HCI_STANDARD_IS_SUPPORT(x)        (config_btctler_hci_standard)


extern const int config_bt_function ;
#define BT_ENCTRY_TASK              BIT(0)

#define BT_FUNCTION_IS(x)           (config_bt_function & (x))


extern const int CONFIG_TEST_DUT_CODE;
extern const int CONFIG_TEST_FCC_CODE;
extern const int CONFIG_TEST_DUT_ONLY_BOX_CODE;
extern const int CONFIG_BREDR_INQUIRY;

extern const int CONFIG_INQUIRY_PAGE_OFFSET_ADJUST ;

extern const int CONFIG_LMP_NAME_REQ_ENABLE ;
extern const int CONFIG_LMP_PASSKEY_ENABLE ;
extern const int CONFIG_LMP_MASTER_ESCO_ENABLE ;
extern const int config_btctler_bredr_master ;
extern const int config_bredr_afh_user ;
/********************************************************************************/
/*
 *                   API
 *
 */

/* --------------------------------------------------------------------------*/
/**
 * @brief rf_set_24g_hackable_coded
 *
 * @param coded                 2.4G 配对码
 */
/* ----------------------------------------------------------------------------*/
void rf_set_24g_hackable_coded(int coded);


/* --------------------------------------------------------------------------*/
/**
 * @brief bt_pll_para
 *
 * @param osc
 * @param sys
 * @param low_power
 * @param xosc
 */
/* ----------------------------------------------------------------------------*/
void bt_pll_para(u32 osc, u32 sys, u8 low_power, u8 xosc);


/* --------------------------------------------------------------------------*/
/**
 * @brief bt_production_test
 *
 * @param en
 */
/* ----------------------------------------------------------------------------*/
void bt_production_test(u8 en);

/* --------------------------------------------------------------------------*/
/**
 * @brief bt_set_rxtx_status_enable
 *
 * @param en
 *
 *

                TX     RX
       AI800x   PA13   PA12
       AC692x   PA13   PA12
       AC693x   PA8    PA9
       AC695x   PA9    PA10
       AC696x   PA9    PA10
       AC694x   PB1    PB2
       AC697x   PC2    PC3
       AC631x   PA7    PA8

 */
/* ----------------------------------------------------------------------------*/
void bt_set_rxtx_status_enable(u8 en);

/* --------------------------------------------------------------------------*/
/**
 * @brief bt_osc_offset_ext_save
 *
 * @param offset
 *
 * 更新并且保存频偏
 */
/* ----------------------------------------------------------------------------*/
void bt_osc_offset_ext_save(s32 offset);

/* --------------------------------------------------------------------------*/
/**
 * @brief bt_osc_offset_ext_updata
 *
 * @param offset
 *
 * 更新频偏
 */
/* ----------------------------------------------------------------------------*/
void bt_osc_offset_ext_updata(s32 offset);


/* --------------------------------------------------------------------------*/
/**
 * @brief bt_set_ldos
 *
 * @param mode
 */
/* ----------------------------------------------------------------------------*/
void bt_set_ldos(u8 mode);

/* --------------------------------------------------------------------------*/
/**
 * @brief ble_set_fix_pwr
 *
 * @param fix (0~max)
 * 动态调整BLE的发射功率
 */
/* ----------------------------------------------------------------------------*/
void ble_set_fix_pwr(u8 fix);


/* --------------------------------------------------------------------------*/
/**
 * @brief bredr_set_fix_pwr
 *
 * @param fix (0~max)
 * 动态调整EDR的发射功率
 */
/* ----------------------------------------------------------------------------*/
void bredr_set_fix_pwr(u8 fix);

/* --------------------------------------------------------------------------*/
/**
 * @brief ble_rf_vendor_fixed_channel
 *
 * @param channel_index: range 0~39 fixed freq, or 0xff --close fixed
 * @param pktcnt:        range 1~3
 * 配置ble 的 adv、scan、init 状态定频
 */
/* ----------------------------------------------------------------------------*/
bool ble_rf_vendor_fixed_channel(u8 channel_index, u8 pktcnt);

void set_bt_afh_classs_enc(u8 afh_class);
void set_bt_enhanced_power_control(u8 en);

#endif
