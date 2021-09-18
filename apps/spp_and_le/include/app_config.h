#ifndef APP_CONFIG_H
#define APP_CONFIG_H

/*
 * 系统打印总开关
 */


#ifdef CONFIG_RELEASE_ENABLE
#define LIB_DEBUG    0
#else
#define LIB_DEBUG    1
#endif

#define CONFIG_DEBUG_LIB(x)         (x & LIB_DEBUG)

#define CONFIG_DEBUG_ENABLE

//apps example 选择,只能选1个,要配置对应的board_config.h
#define CONFIG_APP_SPP_LE                 1 //SPP + LE or LE's client
#define CONFIG_APP_MULTI                  0 //蓝牙LE多连 + spp
#define CONFIG_APP_DONGLE                 0 //usb + 蓝牙(ble 主机),PC hid设备
#define CONFIG_APP_CENTRAL                0 //ble client,中心设备
#define CONFIG_APP_LL_SYNC                0 //腾讯连连
#define CONFIG_APP_BEACON                 0 //蓝牙BLE ibeacon
#define CONFIG_APP_NONCONN_24G            0 //2.4G 非连接收发
#define CONFIG_APP_TUYA                   0 //涂鸦协议
#define CONFIG_APP_AT_COM                 0 //AT com HEX格式命令
#define CONFIG_APP_AT_CHAR_COM            0 //AT com 字符串格式命令
#define CONFIG_APP_IDLE                   0 //空闲任务

// #define LL_SYNC_EN                        CONFIG_APP_LL_SYNC //
// #define TUYA_DEMO_EN                      CONFIG_APP_TUYA

#if CONFIG_APP_SPP_LE
//配置双模同名字，同地址
#define DOUBLE_BT_SAME_NAME                0 //同名字
#define DOUBLE_BT_SAME_MAC                 0 //同地址
#define CONFIG_APP_SPP_LE_TO_IDLE          0 //SPP_AND_LE To IDLE Use

//蓝牙BLE配置
#define CONFIG_BT_GATT_COMMON_ENABLE       1 //配置使用gatt公共模块
#define CONFIG_BT_SM_SUPPORT_ENABLE        0 //配置是否支持加密
#define CONFIG_BT_GATT_CLIENT_NUM          0 //配置主机client个数 (app not support)
#define CONFIG_BT_GATT_SERVER_NUM          1 //配置从机server个数
#define CONFIG_BT_GATT_CONNECTION_NUM      (CONFIG_BT_GATT_SERVER_NUM + CONFIG_BT_GATT_CLIENT_NUM) //配置连接个数

#elif CONFIG_APP_DONGLE
#define EDR_EMITTER_EN                     0 //蓝牙(edr主机)

#define CONFIG_BT_GATT_COMMON_ENABLE       1
#define CONFIG_BT_SM_SUPPORT_ENABLE        1 //
#define CONFIG_BT_GATT_CLIENT_NUM          1 //
#define CONFIG_BT_GATT_SERVER_NUM          0 //(app not support)
#define CONFIG_BT_GATT_CONNECTION_NUM      (CONFIG_BT_GATT_SERVER_NUM + CONFIG_BT_GATT_CLIENT_NUM) //

#elif CONFIG_APP_CENTRAL
#define CONFIG_BT_GATT_COMMON_ENABLE       1
#define CONFIG_BT_SM_SUPPORT_ENABLE        0 //
#define CONFIG_BT_GATT_CLIENT_NUM          1 //
#define CONFIG_BT_GATT_SERVER_NUM          0 //(app not support)
#define CONFIG_BT_GATT_CONNECTION_NUM      (CONFIG_BT_GATT_SERVER_NUM + CONFIG_BT_GATT_CLIENT_NUM) //

#elif CONFIG_APP_MULTI
#define CONFIG_BT_GATT_COMMON_ENABLE       1
#define CONFIG_BT_SM_SUPPORT_ENABLE        0
#define CONFIG_BT_GATT_CLIENT_NUM          1 //range(0~8)
#define CONFIG_BT_GATT_SERVER_NUM          1 //range(0~8)
#define CONFIG_BT_GATT_CONNECTION_NUM      (CONFIG_BT_GATT_SERVER_NUM + CONFIG_BT_GATT_CLIENT_NUM) //range(0~8)

//选择AT: 主机从机二选一
#elif CONFIG_APP_AT_COM
#define TRANS_AT_COM                       1 //串口控制对接蓝牙双模透传
#define TRANS_AT_CLIENT                    0 //串口控制对接蓝牙BLE主机透传

#define CONFIG_BT_GATT_COMMON_ENABLE       0//(app not support)
#define CONFIG_BT_SM_SUPPORT_ENABLE        0//(app not support)
#define CONFIG_BT_GATT_CLIENT_NUM          1
#define CONFIG_BT_GATT_SERVER_NUM          1
#define CONFIG_BT_GATT_CONNECTION_NUM      (CONFIG_BT_GATT_SERVER_NUM + CONFIG_BT_GATT_CLIENT_NUM)

#elif CONFIG_APP_AT_CHAR_COM
#define CONFIG_BT_GATT_COMMON_ENABLE       0//(app not support)
#define CONFIG_BT_SM_SUPPORT_ENABLE        0//(app not support)
#define CONFIG_BT_GATT_CLIENT_NUM          1
#define CONFIG_BT_GATT_SERVER_NUM          1
#define CONFIG_BT_GATT_CONNECTION_NUM      (CONFIG_BT_GATT_SERVER_NUM + CONFIG_BT_GATT_CLIENT_NUM)

#elif CONFIG_APP_TUYA
//配置双模同名字，同地址
#define DOUBLE_BT_SAME_NAME                0 //同名字

//蓝牙配置
#define CONFIG_BT_GATT_COMMON_ENABLE       1 //配置使用gatt公共模块
#define CONFIG_BT_SM_SUPPORT_ENABLE        0 //配置是否支持加密
#define CONFIG_BT_GATT_CLIENT_NUM          0 //配置主机client个数(app not support)
#define CONFIG_BT_GATT_SERVER_NUM          1 //配置从机server个数
#define CONFIG_BT_GATT_CONNECTION_NUM      (CONFIG_BT_GATT_SERVER_NUM + CONFIG_BT_GATT_CLIENT_NUM) //配置连接个数

#elif CONFIG_APP_LL_SYNC
//配置双模同名字，同地址
#define DOUBLE_BT_SAME_NAME                0 //同名字

//蓝牙配置
#define CONFIG_BT_GATT_COMMON_ENABLE       1 //配置使用gatt公共模块
#define CONFIG_BT_SM_SUPPORT_ENABLE        0 //配置是否支持加密
#define CONFIG_BT_GATT_CLIENT_NUM          0 //配置主机client个数(app not support)
#define CONFIG_BT_GATT_SERVER_NUM          1 //配置从机server个数
#define CONFIG_BT_GATT_CONNECTION_NUM      (CONFIG_BT_GATT_SERVER_NUM + CONFIG_BT_GATT_CLIENT_NUM) //配置连接个数

#else
#define CONFIG_BT_GATT_COMMON_ENABLE       0
#define CONFIG_BT_SM_SUPPORT_ENABLE        0
#define CONFIG_BT_GATT_CLIENT_NUM          0
#define CONFIG_BT_GATT_SERVER_NUM          0
#define CONFIG_BT_GATT_CONNECTION_NUM      0

#endif


#if CONFIG_BT_GATT_CONNECTION_NUM > 8
#error "SUPPORT MAX IS 8 !!!"
#endif


#include "board_config.h"

#include "usb_common_def.h"

#include "btcontroller_mode.h"

#include "user_cfg_id.h"

#define APP_PRIVATE_PROFILE_CFG

#if (CONFIG_BT_MODE == BT_NORMAL)
//enable dut mode,need disable sleep
#define TCFG_NORMAL_SET_DUT_MODE                  0

#else

#undef  TCFG_BD_NUM
#define TCFG_BD_NUM						          1

#undef  TCFG_USER_TWS_ENABLE
#define TCFG_USER_TWS_ENABLE                      0     //tws功能使能

#undef  TCFG_USER_BLE_ENABLE
#define TCFG_USER_BLE_ENABLE                      1     //BLE功能使能

#undef  TCFG_AUTO_SHUT_DOWN_TIME
#define TCFG_AUTO_SHUT_DOWN_TIME		          0

#undef  TCFG_SYS_LVD_EN
#define TCFG_SYS_LVD_EN						      0

#undef  TCFG_LOWPOWER_LOWPOWER_SEL
#define TCFG_LOWPOWER_LOWPOWER_SEL                0

#undef TCFG_AUDIO_DAC_LDO_VOLT
#define TCFG_AUDIO_DAC_LDO_VOLT				DACVDD_LDO_2_65V

#undef TCFG_LOWPOWER_POWER_SEL
#define TCFG_LOWPOWER_POWER_SEL				PWR_LDO15

#undef  TCFG_PWMLED_ENABLE
#define TCFG_PWMLED_ENABLE					DISABLE_THIS_MOUDLE

#undef  TCFG_ADKEY_ENABLE
#define TCFG_ADKEY_ENABLE                   DISABLE_THIS_MOUDLE

#undef  TCFG_IOKEY_ENABLE
#define TCFG_IOKEY_ENABLE					DISABLE_THIS_MOUDLE

#undef TCFG_TEST_BOX_ENABLE
#define TCFG_TEST_BOX_ENABLE			    0

#undef TCFG_AUTO_SHUT_DOWN_TIME
#define TCFG_AUTO_SHUT_DOWN_TIME	        0

#undef TCFG_POWER_ON_NEED_KEY
#define TCFG_POWER_ON_NEED_KEY		        0

#undef TCFG_UART0_ENABLE
#define TCFG_UART0_ENABLE					DISABLE_THIS_MOUDLE

#endif


#define BT_FOR_APP_EN                     0

//需要app(BLE)升级要开一下宏定义
#if CONFIG_APP_OTA_ENABLE
#define RCSP_BTMATE_EN                    1
#define RCSP_UPDATE_EN                    1
#define UPDATE_MD5_ENABLE                 0
#else
#define RCSP_BTMATE_EN                    0
#define RCSP_UPDATE_EN                    0
#define UPDATE_MD5_ENABLE                 0
#endif


#ifdef CONFIG_SDFILE_ENABLE
#define SDFILE_DEV				"sdfile"
#define SDFILE_MOUNT_PATH     	"mnt/sdfile"

#if (USE_SDFILE_NEW)
#define SDFILE_APP_ROOT_PATH       	SDFILE_MOUNT_PATH"/app/"  //app分区
#define SDFILE_RES_ROOT_PATH       	SDFILE_MOUNT_PATH"/res/"  //资源文件分区
#else
#define SDFILE_RES_ROOT_PATH       	SDFILE_MOUNT_PATH"/C/"
#endif

#endif
#if TCFG_USER_EDR_ENABLE
#if RCSP_BTMATE_EN
#define CONFIG_BT_RX_BUFF_SIZE  (3 * 512)
#define CONFIG_BT_TX_BUFF_SIZE  (3 * 512)
#else
#define CONFIG_BT_RX_BUFF_SIZE  (5 * 512)
#define CONFIG_BT_TX_BUFF_SIZE  (5 * 512)
#endif
#else
#define CONFIG_BT_RX_BUFF_SIZE  (0)
#define CONFIG_BT_TX_BUFF_SIZE  (0)
#endif

#if (CONFIG_BT_MODE != BT_NORMAL)
////bqb 如果测试3M tx buf 最好加大一点
#undef  CONFIG_BT_TX_BUFF_SIZE
#define CONFIG_BT_TX_BUFF_SIZE  (6 * 1024)

#endif
#define BT_NORMAL_HZ	            CONFIG_BT_NORMAL_HZ
//*********************************************************************************//
//                                 时钟切换配置                                    //
//*********************************************************************************//

#define BT_NORMAL_HZ	            CONFIG_BT_NORMAL_HZ
#define BT_CONNECT_HZ               CONFIG_BT_CONNECT_HZ

#define BT_A2DP_HZ	        	    CONFIG_BT_A2DP_HZ
#define BT_TWS_DEC_HZ	        	CONFIG_TWS_DEC_HZ

//#define MUSIC_DEC_CLOCK			    CONFIG_MUSIC_DEC_CLOCK
//#define MUSIC_IDLE_CLOCK		    CONFIG_MUSIC_IDLE_CLOCK

#define BT_CALL_HZ		            CONFIG_BT_CALL_HZ
#define BT_CALL_ADVANCE_HZ          CONFIG_BT_CALL_ADVANCE_HZ
#define BT_CALL_16k_HZ	            CONFIG_BT_CALL_16k_HZ
#define BT_CALL_16k_ADVANCE_HZ      CONFIG_BT_CALL_16k_ADVANCE_HZ

//*********************************************************************************//
//                                 升级配置                                        //
//*********************************************************************************//
#if (defined(CONFIG_CPU_BR30))
//升级LED显示使能
//#define UPDATE_LED_REMIND
//升级提示音使能
//#define UPDATE_VOICE_REMIND
#endif

#if (defined(CONFIG_CPU_BR23) || defined(CONFIG_CPU_BR25))
//升级IO保持使能
//#define DEV_UPDATE_SUPPORT_JUMP           //目前只有br23\br25支持
#endif

#if (defined(CONFIG_CPU_BR23) || defined(CONFIG_CPU_BR25) || defined(CONFIG_CPU_BD29) || defined(CONFIG_CPU_BD19))
#define USER_UART_UPDATE_ENABLE           0//用于客户开发上位机或者多MCU串口升级方案,请确保串口通信前已经退出powerdown,需要在对应的board添加初始化和唤醒口配置参考board_ac632n_demo.c

#define UART_UPDATE_SLAVE	0
#define UART_UPDATE_MASTER	1

//配置串口升级IO
#define UART_UPDATE_RX_PORT               IO_PORTA_02
#define UART_UPDATE_TX_PORT               IO_PORTA_03

//配置串口升级的角色
#define UART_UPDATE_ROLE	UART_UPDATE_SLAVE

#if USER_UART_UPDATE_ENABLE
#undef TCFG_CHARGESTORE_ENABLE
#undef TCFG_TEST_BOX_ENABLE
#define TCFG_CHARGESTORE_ENABLE				DISABLE_THIS_MOUDLE       //用户串口升级也使用了UART1
#endif

#endif  //USER_UART_UPDATE_ENABLE

#define FLOW_CONTROL           0  //AT 字符串口流控, 目前只有br30做了测试


#endif

