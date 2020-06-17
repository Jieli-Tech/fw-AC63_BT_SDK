/*********************************************************************************************
    *   Filename        : ll_config.h

    *   Description     : Lto 优化Macro 定义

    *   Author          : Bingquan

    *   Email           : bingquan_cai@zh-jieli.com

    *   Last modifiled  : 2018-12-19 16:12

    *   Copyright:(c)JIELI  2011-2017  @ , All Rights Reserved.
*********************************************************************************************/
#ifndef _LL_CONFIG_H_
#define _LL_CONFIG_H_

/*
 *-------------------LE FEATURE SUPPORT
 *  brief : 运行时优化（LTO）下，代码空间优化；
 */
#define LE_ENCRYPTION                       BIT(0)
#define CONNECTION_PARAMETER_REQUEST        BIT(1)
#define EXTENDED_REJECT_INDICATION          BIT(2)
#define LE_SLAVE_INIT_FEATURES_EXCHANGE     BIT(3)
#define LE_PING                             BIT(4)
#define LE_DATA_PACKET_LENGTH_EXTENSION     BIT(5)
#define LL_PRIVACY                          BIT(6)
#define EXTENDED_SCANNER_FILTER_POLICIES    BIT(7)
#define LE_2M_PHY                           BIT(8)
#define LE_CODED_PHY                        BIT(11)
#define LE_EXTENDED_ADVERTISING             BIT(12)
#define LE_PERIODIC_ADVERTISING             BIT(13)
#define CHANNEL_SELECTION_ALGORITHM_2       BIT(14)

#define LE_CORE_V50_FEATURES \
( \
    LE_2M_PHY | \
    LE_CODED_PHY | \
    LE_EXTENDED_ADVERTISING | \
    LE_PERIODIC_ADVERTISING | \
    CHANNEL_SELECTION_ALGORITHM_2 | \
    0 \
)

#if (LE_CORE_V50_FEATURES & LE_PERIODIC_ADVERTISING)
#if ((LE_CORE_V50_FEATURES & LE_EXTENDED_ADVERTISING) == 0)
#error "enable <LE_PERIODIC_ADVERTISING> must enable <LE_EXTENDED_ADVERTISING> at the same time"
#endif
#endif

extern const int config_btctler_le_features;
#define LE_FEATURES_IS_SUPPORT(x)           (config_btctler_le_features & (x))

#define LE_FEATURES_IS_SUPPORT_OPTIMIZE(x)  if (LE_FEATURES_IS_SUPPORT(x) == 0x0)   return
/*-----------------------------------------------------------*/

/*
 *-------------------LE ROLES SUPPORT
 *  brief : 运行时优化（LTO）下，代码空间优化；
 */
#define LE_MASTER                           BIT(0)
#define LE_SLAVE                            BIT(1)
#define LE_ADV                              BIT(2)
#define LE_SCAN                             BIT(3)
#define LE_INIT                             BIT(4)

extern const int config_btctler_le_roles;
#define LE_ROLES_IS_SUPPORT(x)              (config_btctler_le_roles & x)

#define LE_ROLES_IS_SUPPORT_OPTIMIZE(x)     if (LE_ROLES_IS_SUPPORT(x) == 0x0)   return
/*-----------------------------------------------------------*/

extern const int config_btctler_le_tws;
#define LE_TWS_IS_SUPPORT()                (config_btctler_le_tws)


/*-----------------------------------------------------------*/

extern const int config_btctler_le_afh_en;
#define LE_AFH_IS_SUPPORT()                (config_btctler_le_afh_en)

#define LE_AFH_IS_SUPPORT_OPTIMIZE(x)     if (LE_AFH_IS_SUPPORT() == 0x0)   return
/*
 *-------------------LE RAM CONTROL
 *
 */
extern const int config_btctler_le_hw_nums;
extern const int config_btctler_le_rx_nums;
extern const int config_btctler_le_acl_packet_length;
extern const int config_btctler_le_acl_total_nums;
/*-----------------------------------------------------------*/

#endif //_LL_CONFIG_H_
