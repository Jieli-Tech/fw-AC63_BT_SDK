#ifndef CONFIG_BOARD_AC631N_DEMO_H
#define CONFIG_BOARD_AC631N_DEMO_H

#include "board_ac631n_demo_global_build_cfg.h"

#ifdef CONFIG_BOARD_AC631N_DEMO

#define CONFIG_SDFILE_ENABLE

//*********************************************************************************//
//                                 配置开始                                        //
//*********************************************************************************//
#define ENABLE_THIS_MOUDLE					1
#define DISABLE_THIS_MOUDLE					0

#define ENABLE								1
#define DISABLE								0

#define NO_CONFIG_PORT						(-1)

//*********************************************************************************//
//                                 UART配置                                        //
//*********************************************************************************//
#define TCFG_UART0_ENABLE					ENABLE_THIS_MOUDLE
#define TCFG_UART0_RX_PORT					NO_CONFIG_PORT
#define TCFG_UART0_TX_PORT  				IO_PORTA_00
#define TCFG_UART0_BAUDRATE  				1000000

//*********************************************************************************//
//                                 USB 配置                                        //
//*********************************************************************************//
#define TCFG_PC_ENABLE						DISABLE_THIS_MOUDLE //PC模块使能
#define TCFG_UDISK_ENABLE					DISABLE_THIS_MOUDLE //U盘模块使能
#define TCFG_HID_HOST_ENABLE                DISABLE_THIS_MOUDLE//ENABLE_THIS_MOUDLE  //游戏盒子模式
#define TCFG_ADB_ENABLE                     DISABLE_THIS_MOUDLE//ENABLE_THIS_MOUDLE
#define TCFG_AOA_ENABLE                     DISABLE_THIS_MOUDLE//ENABLE_THIS_MOUDLE

#define TCFG_OTG_USB_DEV_EN                 (BIT(0) | BIT(1))//USB0 = BIT(0)  USB1 = BIT(1)
//*********************************************************************************//
//                                 key配置                                       //
//*********************************************************************************//
#define  KEY_NUM_MAX                        10
#define  KEY_NUM                            3

//*********************************************************************************//
//                                 iokey 配置                                      //
//*********************************************************************************//
#define TCFG_IOKEY_ENABLE					DISABLE_THIS_MOUDLE //是否使能IO按键

#define TCFG_IOKEY_POWER_CONNECT_WAY		ONE_PORT_TO_LOW    //按键一端接低电平一端接IO

#define TCFG_IOKEY_POWER_ONE_PORT			IO_PORTB_01        //IO按键端口

#define TCFG_IOKEY_PREV_CONNECT_WAY			ONE_PORT_TO_LOW  //按键一端接低电平一端接IO
#define TCFG_IOKEY_PREV_ONE_PORT			IO_PORTB_00

#define TCFG_IOKEY_NEXT_CONNECT_WAY 		ONE_PORT_TO_LOW  //按键一端接低电平一端接IO
#define TCFG_IOKEY_NEXT_ONE_PORT			IO_PORTB_02

//*********************************************************************************//
//                                 adkey 配置                                      //
//*********************************************************************************//
#define TCFG_ADKEY_ENABLE                   ENABLE_THIS_MOUDLE //是否使能AD按键
#define TCFG_ADKEY_PORT                     IO_PORTB_01         //AD按键端口(需要注意选择的IO口是否支持AD功能)
/*AD通道选择，需要和AD按键的端口相对应:
    AD_CH_PA1    AD_CH_PA3    AD_CH_PA4    AD_CH_PA5
    AD_CH_PA9    AD_CH_PA1    AD_CH_PB1    AD_CH_PB4
    AD_CH_PB6    AD_CH_PB7    AD_CH_DP     AD_CH_DM
    AD_CH_PB2
*/
#define TCFG_ADKEY_AD_CHANNEL               AD_CH_PB1
#define TCFG_ADKEY_EXTERN_UP_ENABLE         ENABLE_THIS_MOUDLE //是否使用外部上拉

#if TCFG_ADKEY_EXTERN_UP_ENABLE
#define R_UP    220                 //22K，外部上拉阻值在此自行设置
#else
#define R_UP    100                 //10K，内部上拉默认10K
#endif

//必须从小到大填电阻，没有则同VDDIO,填0x3ffL
#define TCFG_ADKEY_AD0      (0)                                 //0R
#define TCFG_ADKEY_AD1      (0x3ffL * 30   / (30   + R_UP))     //3k
#define TCFG_ADKEY_AD2      (0x3ffL * 62   / (62   + R_UP))     //6.2k
#define TCFG_ADKEY_AD3      (0x3ffL * 91   / (91   + R_UP))     //9.1k
#define TCFG_ADKEY_AD4      (0x3ffL * 150  / (150  + R_UP))     //15k
#define TCFG_ADKEY_AD5      (0x3ffL * 240  / (240  + R_UP))     //24k
#define TCFG_ADKEY_AD6      (0x3ffL * 330  / (330  + R_UP))     //33k
#define TCFG_ADKEY_AD7      (0x3ffL * 510  / (510  + R_UP))     //51k
#define TCFG_ADKEY_AD8      (0x3ffL * 1000 / (1000 + R_UP))     //100k
#define TCFG_ADKEY_AD9      (0x3ffL * 2200 / (2200 + R_UP))     //220k
#define TCFG_ADKEY_VDDIO    (0x3ffL)

#define TCFG_ADKEY_VOLTAGE0 ((TCFG_ADKEY_AD0 + TCFG_ADKEY_AD1) / 2)
#define TCFG_ADKEY_VOLTAGE1 ((TCFG_ADKEY_AD1 + TCFG_ADKEY_AD2) / 2)
#define TCFG_ADKEY_VOLTAGE2 ((TCFG_ADKEY_AD2 + TCFG_ADKEY_AD3) / 2)
#define TCFG_ADKEY_VOLTAGE3 ((TCFG_ADKEY_AD3 + TCFG_ADKEY_AD4) / 2)
#define TCFG_ADKEY_VOLTAGE4 ((TCFG_ADKEY_AD4 + TCFG_ADKEY_AD5) / 2)
#define TCFG_ADKEY_VOLTAGE5 ((TCFG_ADKEY_AD5 + TCFG_ADKEY_AD6) / 2)
#define TCFG_ADKEY_VOLTAGE6 ((TCFG_ADKEY_AD6 + TCFG_ADKEY_AD7) / 2)
#define TCFG_ADKEY_VOLTAGE7 ((TCFG_ADKEY_AD7 + TCFG_ADKEY_AD8) / 2)
#define TCFG_ADKEY_VOLTAGE8 ((TCFG_ADKEY_AD8 + TCFG_ADKEY_AD9) / 2)
#define TCFG_ADKEY_VOLTAGE9 ((TCFG_ADKEY_AD9 + TCFG_ADKEY_VDDIO) / 2)

#define TCFG_ADKEY_VALUE0                   0
#define TCFG_ADKEY_VALUE1                   1
#define TCFG_ADKEY_VALUE2                   2
#define TCFG_ADKEY_VALUE3                   3
#define TCFG_ADKEY_VALUE4                   4
#define TCFG_ADKEY_VALUE5                   5
#define TCFG_ADKEY_VALUE6                   6
#define TCFG_ADKEY_VALUE7                   7
#define TCFG_ADKEY_VALUE8                   8
#define TCFG_ADKEY_VALUE9                   9

//*********************************************************************************//
//                                 irkey 配置                                      //
//*********************************************************************************//
#define TCFG_IRKEY_ENABLE                   DISABLE_THIS_MOUDLE//是否使能AD按键
#define TCFG_IRKEY_PORT                     IO_PORTA_08        //IR按键端口

//*********************************************************************************//
//                                  RTC_ALARM配置                                  //
//*********************************************************************************//
#define TCFG_RTC_ALARM_ENABLE               DISABLE_THIS_MOUDLE

//*********************************************************************************//
//                                  充电仓配置(不支持)                             //
//*********************************************************************************//
#define TCFG_CHARGESTORE_ENABLE				DISABLE_THIS_MOUDLE
#define TCFG_TEST_BOX_ENABLE			    0
#define TCFG_CHARGESTORE_PORT				IO_PORTB_05
#define TCFG_CHARGESTORE_UART_ID			IRQ_UART1_IDX

//*********************************************************************************//
//                                  充电参数配置                                   //
//*********************************************************************************//
#define TCFG_CHARGE_ENABLE				    DISABLE_THIS_MOUDLE
#define TCFG_CHARGE_POWERON_ENABLE			DISABLE//(不支持配置)
#define TCFG_CHARGE_OFF_POWERON_NE			DISABLE//(不支持配置)
#define TCFG_HANDSHAKE_ENABLE               DISABLE//是否支持lighting握手协议
#define TCFG_HANDSHAKE_IO_DATA1             IO_PORTB_02//握手IO靠近lighting座子中间的
#define TCFG_HANDSHAKE_IO_DATA2             IO_PORTB_07//握手IO在lighting座子边上的
#define TCFG_CHARGE_FULL_V					CHARGE_FULL_V_4202
#define TCFG_CHARGE_FULL_MA					CHARGE_FULL_mA_10
#define TCFG_CHARGE_MA						CHARGE_mA_50

//*********************************************************************************//
//                                  LED 配置                                       //
//*********************************************************************************//
#define TCFG_PWMLED_ENABLE					DISABLE_THIS_MOUDLE
#define TCFG_PWMLED_PORT					IO_PORTB_06
#define TCFG_REDLED_LIGHT					1   //1 ~ 10, value越大, (红灯)亮度越高
#define TCFG_BLUELED_LIGHT					1   //1 ~ 10, value越大, (蓝灯)亮度越高
#define TCFG_SINGLE_SLOW_FLASH_FREQ			3	//1 ~ 8, value越大, LED单独慢闪速度越慢, value * 0.5s闪烁一次
#define TCFG_SINGLE_FAST_FLASH_FREQ			4	//1 ~ 4, value越大, LED单独快闪速度越慢, value * 100ms闪烁一次
#define TCFG_DOUBLE_SLOW_FLASH_FREQ			3	//1 ~ 8, value越大, LED交替慢闪速度越慢, value * 0.5s闪烁一次
#define TCFG_DOUBLE_FAST_FLASH_FREQ			4	//1 ~ 4, value越大, LED交替快闪速度越慢, value * 100ms闪烁一次

//*********************************************************************************//
//                                  时钟配置                                       //
//*********************************************************************************//
#if CONFIG_PLL_SOURCE_USING_LRC
#define TCFG_CLOCK_SYS_SRC     SYS_CLOCK_INPUT_PLL_RCL   //系统时钟源选择
#else
#define TCFG_CLOCK_SYS_SRC     SYS_CLOCK_INPUT_PLL_BT_OSC   //系统时钟源选择
#endif

#define TCFG_CLOCK_SYS_HZ					24000000
#define TCFG_CLOCK_OSC_HZ					24000000
#define TCFG_CLOCK_MODE                     CLOCK_MODE_ADAPTIVE

//*********************************************************************************//
//                                  低功耗配置                                     //
//*********************************************************************************//
//#define TCFG_LOWPOWER_POWER_SEL				PWR_DCDC15//
#define TCFG_LOWPOWER_POWER_SEL				PWR_LDO15//
#define TCFG_LOWPOWER_BTOSC_DISABLE			0
#define TCFG_LOWPOWER_LOWPOWER_SEL			SLEEP_EN
#define TCFG_LOWPOWER_VDDIOM_LEVEL			VDDIOM_VOL_30V
#define TCFG_LOWPOWER_VDDIOW_LEVEL			VDDIOW_VOL_28V
#define TCFG_LOWPOWER_OSC_TYPE              OSC_TYPE_LRC


//*********************************************************************************//
//                                  g-sensor配置                                   //
//*********************************************************************************//
#define TCFG_GSENSOR_ENABLE                       0     //gSensor使能
#define TCFG_DA230_EN                             0

//*********************************************************************************//
//                                  系统配置                                         //
//*********************************************************************************//
#define TCFG_AUTO_SHUT_DOWN_TIME		          0   //没有蓝牙连接自动关机时间
#define TCFG_SYS_LVD_EN						      1   //电量检测使能
#define TCFG_POWER_ON_NEED_KEY				      0	  //是否需要按按键开机配置
#define TCFG_HID_AUTO_SHUTDOWN_TIME              (0 * 60)  //鼠标无操作自动关机(单位：秒)

//*********************************************************************************//
//                                  蓝牙配置                                       //
//*********************************************************************************//
#define TCFG_USER_TWS_ENABLE                      0   //tws功能使能
#define TCFG_USER_BLE_ENABLE                      1   //BLE功能使能,---使能后,请配置TCFG_BLE_DEMO_SELECT选择DEMO例子
#define TCFG_USER_EDR_ENABLE                      1   //EDR功能使能

#if TCFG_USER_EDR_ENABLE
#define USER_SUPPORT_PROFILE_SPP    0
#define USER_SUPPORT_PROFILE_HFP    0
#define USER_SUPPORT_PROFILE_A2DP   0
#define USER_SUPPORT_PROFILE_AVCTP  0
#define USER_SUPPORT_PROFILE_HID    1
#define USER_SUPPORT_PROFILE_PNP    1
#define USER_SUPPORT_PROFILE_PBAP   0
#endif

#if(TCFG_USER_TWS_ENABLE || TCFG_USER_BLE_ENABLE)
#define TCFG_BD_NUM						          1   //连接设备个数配置
#define TCFG_AUTO_STOP_PAGE_SCAN_TIME             0   //配置一拖二第一台连接后自动关闭PAGE SCAN的时间(单位分钟)
#else
#define TCFG_BD_NUM						          2   //连接设备个数配置
#define TCFG_AUTO_STOP_PAGE_SCAN_TIME             180 //配置一拖二第一台连接后自动关闭PAGE SCAN的时间(单位分钟)
#endif

#define BT_INBAND_RINGTONE                        0   //是否播放手机自带来电铃声
#define BT_PHONE_NUMBER                           0   //是否播放来电报号
#define BT_SUPPORT_DISPLAY_BAT                    0   //是否使能电量检测
#define BT_SUPPORT_MUSIC_VOL_SYNC                 0   //是否使能音量同步

//*********************************************************************************//
//                                 时钟配置                                    //
//*********************************************************************************//
#define CONFIG_BT_NORMAL_HZ	            (24 * 1000000L)

//*********************************************************************************//
//                                 配置结束                                        //
//*********************************************************************************//

#if TCFG_HID_HOST_ENABLE

#undef TCFG_LOWPOWER_LOWPOWER_SEL
#define TCFG_LOWPOWER_LOWPOWER_SEL			0

#undef TCFG_LOWPOWER_VDDIOM_LEVEL
#define TCFG_LOWPOWER_VDDIOM_LEVEL			VDDIOM_VOL_32V

#undef TCFG_LOWPOWER_VDDIOW_LEVEL
#define TCFG_LOWPOWER_VDDIOW_LEVEL			VDDIOW_VOL_32V

#undef TCFG_USER_EDR_ENABLE
#define     TCFG_USER_EDR_ENABLE    0
#endif

#endif

#endif
