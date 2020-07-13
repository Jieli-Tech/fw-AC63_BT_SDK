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



//app case 选择,只能选1个,要配置对应的board_config.h
#define CONFIG_APP_SPP_LE                 1
#define CONFIG_APP_AT_COM                 0
#define CONFIG_APP_DONGLE                 0

//配置对应的APP的蓝牙功能
#if CONFIG_APP_SPP_LE
#define TRANS_DATA_EN                     1 //蓝牙双模透传
#define TRANS_CLIENT_EN                   0 //蓝牙(ble主机)透传

#if (TRANS_DATA_EN + TRANS_CLIENT_EN > 1)
#error "they can not enable at the same time!"
#endif
#endif

#if CONFIG_APP_AT_COM
#define TRANS_AT_COM                      1 //串口控制对接蓝牙双模透传
#endif

#if CONFIG_APP_DONGLE
#define TRANS_DONGLE_EN                   1 //蓝牙(ble主机)
#endif

#include "board_config.h"

#include "usb_common_def.h"

#include "btcontroller_mode.h"

#define APP_PRIVATE_PROFILE_CFG

#if (CONFIG_BT_MODE != BT_NORMAL)
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
#define RCSP_BTMATE_EN                    0
#define RCSP_UPDATE_EN                    0
#define UPDATE_MD5_ENABLE                 0


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
#define CONFIG_BT_RX_BUFF_SIZE  (2 * 1024 + 512)
#define CONFIG_BT_TX_BUFF_SIZE  (2 * 1024)
#else
#define CONFIG_BT_RX_BUFF_SIZE  (3 * 1024)
#define CONFIG_BT_TX_BUFF_SIZE  (3 * 1024)
#endif
#else
#define CONFIG_BT_RX_BUFF_SIZE  (0)
#define CONFIG_BT_TX_BUFF_SIZE  (0)
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

#endif

