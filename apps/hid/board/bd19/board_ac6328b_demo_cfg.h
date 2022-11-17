#ifndef CONFIG_BOARD_AC6328B_DEMO_H
#define CONFIG_BOARD_AC6328B_DEMO_H

#include "board_ac6328b_demo_global_build_cfg.h"

#ifdef CONFIG_BOARD_AC6328B_DEMO

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
#define TCFG_UART0_ENABLE					ENABLE_THIS_MOUDLE                     //串口打印模块使能
#define TCFG_UART0_RX_PORT					NO_CONFIG_PORT                         //串口接收脚配置（用于打印可以选择NO_CONFIG_PORT）
#define TCFG_UART0_TX_PORT  				IO_PORT_DP                            //串口发送脚配置
#define TCFG_UART0_BAUDRATE  				1000000                                //串口波特率配置

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
//                                 IIC配置                                        //
//*********************************************************************************//
/*软件IIC设置*/
#define TCFG_SW_I2C0_CLK_PORT               IO_PORTA_09                             //软件IIC  CLK脚选择
#define TCFG_SW_I2C0_DAT_PORT               IO_PORTA_10                             //软件IIC  DAT脚选择
#define TCFG_SW_I2C0_DELAY_CNT              50                                      //IIC延时参数，影响通讯时钟频率

/*硬件IIC端口选择
  SCL         SDA
  'A': IO_PORT_DP   IO_PORT_DM
  'B': IO_PORTA_09  IO_PORTA_10
  'C': IO_PORTA_07  IO_PORTA_08
  'D': IO_PORTA_05  IO_PORTA_06
 */
#define TCFG_HW_I2C0_PORTS                  'B'
#define TCFG_HW_I2C0_CLK                    100000                                  //硬件IIC波特率

//*********************************************************************************//
//                                 硬件SPI 配置                                        //
//*********************************************************************************//
#define	TCFG_HW_SPI1_ENABLE		ENABLE_THIS_MOUDLE
//A组IO:    DI: PB2     DO: PB1     CLK: PB0
//B组IO:    DI: PC3     DO: PC5     CLK: PC4
#define TCFG_HW_SPI1_PORT		'A'
#define TCFG_HW_SPI1_BAUD		2000000L
#define TCFG_HW_SPI1_MODE		SPI_MODE_BIDIR_1BIT
#define TCFG_HW_SPI1_ROLE		SPI_ROLE_MASTER

#define	TCFG_HW_SPI2_ENABLE		ENABLE_THIS_MOUDLE
//A组IO:    DI: PB8     DO: PB10    CLK: PB9
//B组IO:    DI: PA13    DO: DM      CLK: DP
#define TCFG_HW_SPI2_PORT		'A'
#define TCFG_HW_SPI2_BAUD		2000000L
#define TCFG_HW_SPI2_MODE		SPI_MODE_BIDIR_1BIT
#define TCFG_HW_SPI2_ROLE		SPI_ROLE_MASTER

//*********************************************************************************//
//                                 FLASH 配置                                      //
//*********************************************************************************//
#define TCFG_CODE_FLASH_ENABLE				DISABLE_THIS_MOUDLE
#define TCFG_FLASH_DEV_SPI_HW_NUM			1// 1: SPI1    2: SPI2
#define TCFG_FLASH_DEV_SPI_CS_PORT	    	IO_PORTB_06
#define TCFG_NORFLASH_DEV_ENABLE            DISABLE_THIS_MOUDLE


//*********************************************************************************//
//                                 key 配置                                        //
//*********************************************************************************//
#define KEY_NUM_MAX                        	10
#define KEY_NUM                            	3

#define MULT_KEY_ENABLE						DISABLE 		//是否使能组合按键消息, 使能后需要配置组合按键映射表
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
#define TCFG_ADKEY_ENABLE                   DISABLE_THIS_MOUDLE//ENABLE_THIS_MOUDLE //是否使能AD按键
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
//                                 Audio配置                                       //
//*********************************************************************************//
#ifdef CONFIG_LITE_AUDIO
#define TCFG_AUDIO_ENABLE					DISABLE
#if TCFG_AUDIO_ENABLE
#define TCFG_DEC_USBC_ENABLE			    DISABLE
#define TCFG_ENC_USBC_ENABLE              	DISABLE
#define TCFG_DEC_LC3_ENABLE              	DISABLE
#define TCFG_ENC_LC3_ENABLE              	DISABLE


//lc3 参数配置
#if (TCFG_ENC_LC3_ENABLE || TCFG_DEC_LC3_ENABLE)
#define LC3_CODING_SAMPLERATE  16000 //lc3 编码的采样率
#define LC3_CODING_FRAME_LEN   50  //帧长度，只支持25，50，100
#define LC3_CODING_CHANNEL     1  //lc3 的通道数
#endif

#endif

#endif
//*********************************************************************************//
//                                 irkey 配置                                      //
//*********************************************************************************//
#define TCFG_IRKEY_ENABLE                   DISABLE_THIS_MOUDLE//是否使能AD按键
#define TCFG_IRKEY_PORT                     IO_PORTA_08        //IR按键端口

//*********************************************************************************//
//                             tocuh key 配置                                      //
//*********************************************************************************//
#define TCFG_LP_TOUCH_KEY_ENABLE 			DISABLE_THIS_MOUDLE 		//是否使能触摸按键

//*********************************************************************************//
//                                  RTC_ALARM配置                                  //
//*********************************************************************************//
#define TCFG_RTC_ALARM_ENABLE               DISABLE_THIS_MOUDLE

//*********************************************************************************//
//                                  充电仓配置(不支持)                             //
//*********************************************************************************//
#define TCFG_TEST_BOX_ENABLE			    DISABLE_THIS_MOUDLE
#define TCFG_CHARGESTORE_PORT				IO_PORTP_00
#define TCFG_CHARGESTORE_UART_ID			IRQ_UART1_IDX

//*********************************************************************************//
//                                  充电参数配置                                   //
//*********************************************************************************//
//是否支持芯片内置充电
#define TCFG_CHARGE_ENABLE					DISABLE_THIS_MOUDLE
//是否支持开机充电
#define TCFG_CHARGE_POWERON_ENABLE			DISABLE
//是否支持拔出充电自动开机功能
#define TCFG_CHARGE_OFF_POWERON_NE			DISABLE
/*
充电截止电压可选配置：
    CHARGE_FULL_V_3962  CHARGE_FULL_V_4002  CHARGE_FULL_V_4044  CHARGE_FULL_V_4086
    CHARGE_FULL_V_4130  CHARGE_FULL_V_4175  CHARGE_FULL_V_4222  CHARGE_FULL_V_4270
    CHARGE_FULL_V_4308  CHARGE_FULL_V_4349  CHARGE_FULL_V_4391  CHARGE_FULL_V_4434
    CHARGE_FULL_V_4472  CHARGE_FULL_V_4517  CHARGE_FULL_V_4564  CHARGE_FULL_V_4611
*/
#define TCFG_CHARGE_FULL_V					CHARGE_FULL_V_4222
/*
充电截止电流可选配置：
    CHARGE_FULL_mA_2	CHARGE_FULL_mA_5	CHARGE_FULL_mA_7	 CHARGE_FULL_mA_10
    CHARGE_FULL_mA_15	CHARGE_FULL_mA_20	CHARGE_FULL_mA_25	 CHARGE_FULL_mA_30
*/
#define TCFG_CHARGE_FULL_MA					CHARGE_FULL_mA_10
/*
充电电流可选配置：
    CHARGE_mA_20		CHARGE_mA_30		CHARGE_mA_40		CHARGE_mA_50
    CHARGE_mA_60		CHARGE_mA_70		CHARGE_mA_80		CHARGE_mA_90
    CHARGE_mA_100		CHARGE_mA_110		CHARGE_mA_120		CHARGE_mA_140
    CHARGE_mA_160		CHARGE_mA_180		CHARGE_mA_200		CHARGE_mA_220
 */
#define TCFG_CHARGE_MA						CHARGE_mA_50

//*********************************************************************************//
//                                  LED 配置                                       //
//*********************************************************************************//
#define TCFG_PWMLED_ENABLE					DISABLE_THIS_MOUDLE			//是否支持PMW LED推灯模块
#define TCFG_PWMLED_IOMODE					LED_ONE_IO_MODE				//LED模式，单IO还是两个IO推灯
#define TCFG_PWMLED_PIN						IO_PORTB_06					//LED使用的IO口
//*********************************************************************************//
//                                  时钟配置                                       //
//*********************************************************************************//
#define TCFG_CLOCK_SYS_SRC					SYS_CLOCK_INPUT_PLL_BT_OSC   //系统时钟源选择
#define TCFG_CLOCK_SYS_HZ					24000000                     //系统时钟设置
#define TCFG_CLOCK_OSC_HZ					24000000                     //外界晶振频率设置
/* #define TCFG_CLOCK_MODE                     CLOCK_MODE_USR//CLOCK_MODE_ADAPTIVE */
#define TCFG_CLOCK_MODE                     CLOCK_MODE_ADAPTIVE

//*********************************************************************************//
//                                  低功耗配置                                     //
//*********************************************************************************//
//#define TCFG_LOWPOWER_POWER_SEL				PWR_DCDC15
#define TCFG_LOWPOWER_POWER_SEL				PWR_LDO15                    //电源模式设置，可选DCDC和LDO
#define TCFG_LOWPOWER_BTOSC_DISABLE			0                            //低功耗模式下BTOSC是否保持
#define TCFG_LOWPOWER_LOWPOWER_SEL			SLEEP_EN                     //SNIFF状态下芯片是否进入powerdown
/*强VDDIO等级配置,可选：
    VDDIOM_VOL_20V    VDDIOM_VOL_22V    VDDIOM_VOL_24V    VDDIOM_VOL_26V
    VDDIOM_VOL_30V    VDDIOM_VOL_30V    VDDIOM_VOL_32V    VDDIOM_VOL_36V*/
#define TCFG_LOWPOWER_VDDIOM_LEVEL			VDDIOM_VOL_30V
/*弱VDDIO等级配置，可选：
    VDDIOW_VOL_21V    VDDIOW_VOL_24V    VDDIOW_VOL_28V    VDDIOW_VOL_32V*/
#define TCFG_LOWPOWER_VDDIOW_LEVEL			VDDIOW_VOL_28V               //弱VDDIO等级配置
#define TCFG_LOWPOWER_OSC_TYPE              OSC_TYPE_LRC
#define TCFG_VD13_CAP_EN					0

//*********************************************************************************//
//                                  g-sensor配置                                   //
//*********************************************************************************//
#define TCFG_GSENSOR_ENABLE                       0     //gSensor使能
#define TCFG_DA230_EN                             0
#define TCFG_SC7A20_EN                            0
#define TCFG_STK8321_EN                           0
#define TCFG_GSENOR_USER_IIC_TYPE                 0     //0:软件IIC  1:硬件IIC

//*********************************************************************************//
//                                  系统配置                                         //
//*********************************************************************************//
#define TCFG_AUTO_SHUT_DOWN_TIME		          0   //没有蓝牙连接自动关机时间
#if (TCFG_LOWPOWER_POWER_SEL == PWR_DCDC15)
#define TCFG_SYS_LVD_EN						      1   //dcdc模式电压低于2.4v的时候切为LDO模式，需要开启电量检测
#else
#define TCFG_SYS_LVD_EN						      0   //电量检测使能
#endif
#define TCFG_POWER_ON_NEED_KEY				      0	  //是否需要按按键开机配置
#define TCFG_HID_AUTO_SHUTDOWN_TIME              (0 * 60)      //HID无操作自动关机(单位：秒)

//*********************************************************************************//
//                                  蓝牙配置                                       //
//*********************************************************************************//
#define TCFG_USER_TWS_ENABLE                      0   //tws功能使能
#define TCFG_USER_BLE_ENABLE                      1   //BLE功能使能,---使能后,请配置TCFG_BLE_DEMO_SELECT选择DEMO例子
#define TCFG_USER_EDR_ENABLE                      1   //EDR功能使能

#define USER_SUPPORT_PROFILE_SPP    0
#define USER_SUPPORT_PROFILE_HFP    0
#define USER_SUPPORT_PROFILE_A2DP   0
#define USER_SUPPORT_PROFILE_AVCTP  0
#define USER_SUPPORT_PROFILE_HID    1
#define USER_SUPPORT_PROFILE_PNP    1
#define USER_SUPPORT_PROFILE_PBAP   0


#if(TCFG_USER_TWS_ENABLE || TCFG_USER_BLE_ENABLE)
#define TCFG_BD_NUM						          1   //连接设备个数配置
#define TCFG_AUTO_STOP_PAGE_SCAN_TIME             0   //配置一拖二第一台连接后自动关闭PAGE SCAN的时间(单位分钟)
#else
#define TCFG_BD_NUM						          1   //连接设备个数配置
#define TCFG_AUTO_STOP_PAGE_SCAN_TIME             0 //配置一拖二第一台连接后自动关闭PAGE SCAN的时间(单位分钟)
#endif

#define BT_INBAND_RINGTONE                        0   //是否播放手机自带来电铃声
#define BT_PHONE_NUMBER                           0   //是否播放来电报号
#define BT_SUPPORT_DISPLAY_BAT                    0   //是否使能电量检测
#define BT_SUPPORT_MUSIC_VOL_SYNC                 0   //是否使能音量同步


//*********************************************************************************//
//                                 电源切换配置                                    //
//*********************************************************************************//

#define CONFIG_PHONE_CALL_USE_LDO15	    1

//*********************************************************************************//
//                                 时钟配置                                    //
//*********************************************************************************//

#define CONFIG_BT_NORMAL_HZ	            (48 * 1000000L)
//#define CONFIG_BT_CONNECT_HZ            (48 * 1000000L)

//*********************************************************************************//
//                                 配置结束                                        //
//*********************************************************************************//

#endif

#endif
