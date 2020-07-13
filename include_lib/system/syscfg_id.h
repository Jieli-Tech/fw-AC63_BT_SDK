
#ifndef __JL_CFG_DEC_H__
#define __JL_CFG_DEC_H__

#include "typedef.h"

struct btif_item {
    u16 id;
    u16 data_len;
};


struct syscfg_operataions {
    int (*init)(void);
    int (*check_id)(u16 item_id);
    int (*read)(u16 item_id, u8 *buf, u16 len);
    int (*write)(u16 item_id, u8 *buf, u16 len);
    int (*dma_write)(u16 item_id, u8 *buf, u16 len);
    int (*read_string)(u16 item_id, u8 *buf, u16 len, u8 ver);
    u8 *(*ptr_read)(u16 item_id, u16 *len);
};

#define REGISTER_SYSCFG_OPS(cfg, pri) \
	const struct syscfg_operataions cfg SEC_USED(.syscfg.pri.ops)

//=================================================================================//
//                        系统配置项(VM, BTIF, cfg_bin)读写接口                    //
//接口说明:                                			   						       //
// 	1.输入参数                                			     					   //
// 		1)item_id: 配置项ID号, 由本文件统一分配;                                   //
// 		2)buf: 用于存储read/write数据内容;                                    	   //
// 		3)len: buf的长度(byte), buf长度必须大于等于read/write数据长度;             //
// 	2.返回参数:                                 			     				   //
// 		1)执行正确: 返回值等于实际上所读到的数据长度(大于0);                       //
// 		2)执行错误: 返回值小于等于0, 小于0表示相关错误码;                          //
// 	3.读写接口使用注意事项:             										   //
// 		1)不能在中断里调用写(write)接口;                                   		   //
// 		2)调用本读写接口时应该习惯性判断返回值来检查read/write动作是否执行正确;    //
//=================================================================================//
int syscfg_read(u16 item_id, void *buf, u16 len);
int syscfg_write(u16 item_id, void *buf, u16 len);

//该接口默认会以dma的方式写vm区域, 请注意buf地址需要按照4byte对齐
int syscfg_dma_write(u16 item_id, void *buf, u16 len);

//读取同一个配置项存在多份数据中的某一份数据, ver读取表示第几份数据, ver从 0 开始;
//典型应用: 读取配置项CFG_BT_NAME中多个蓝牙名中的某一个蓝牙名;
int syscfg_read_string(u16 item_id, void *buf, u16 len, u8 ver);

//读取配置项, 返回: ptr: 配置项地址指针(可以用cpu直接访问); len为配置项长度
//注: 只支持cfg_tools.bin文件中的配置项读取
u8 *syscfg_ptr_read(u16 item_id, u16 *len);


//==================================================================================================//
//                                 配置项ID分配说明                                					//
//	1.配置项ID号根据存储区域进行分配;                                			   					//
//	2.存储区域有3个:  1)VM区域; 2)sys_cfg.bin; 3)BTIF区域                      	   					//
//	3.配置项ID号分配如下:  												   		   					//
//		0)[0]: 配置项ID号0为配置项工具保留ID号; 			   	   									//
//		1)[  1 ~  49]: 共49项, 预留给用户自定义, 只存于VM区域; 			   	   						//
//		2)[ 50 ~  99]: 共50项, sdk相关配置项, 只存于VM区域; 			   		   					//
//		3)[100 ~ 127]: 共28项, sdk相关配置项, 可以存于VM区域, sys_cfg.bin(作为默认值) 和 BTIF区域;	//
//		4)[512 ~ 700]: 共188项, sdk相关配置项, 只存于sys_cfg.bin; 		   		   					//
//==================================================================================================//

//=================================================================================//
//                             用户自定义配置项[1 ~ 49]                            //
//=================================================================================//
#define 	CFG_USER_DEFINE_BEGIN		1

#define 	VM_FM_EMITTER_FREQ			2
#define 	VM_FM_EMITTER_DIG_VOL		3

#define 	VM_MUSIC_LAST_DEV	    	4

#ifdef CONFIG_CPU_BR21
#define 	VM_WAKEUP_PND				5//用于BR21 中判断充电唤醒源
#endif

#define     VM_USB_MIC_GAIN             6

#define		VM_PMU_VOLTAGE              7

#define     VM_ALARM_0                 (20)
#define     VM_ALARM_1                 (VM_ALARM_0+1)
#define     VM_ALARM_2                 (VM_ALARM_0+2)
#define     VM_ALARM_3                 (VM_ALARM_0+3)
#define     VM_ALARM_4                 (VM_ALARM_0+4)
#define     VM_ALARM_MASK              (VM_ALARM_0+5)

#define     VM_ALARM_NAME_0            (VM_ALARM_0+6)
#define     VM_ALARM_NAME_1            (VM_ALARM_0+7)
#define     VM_ALARM_NAME_2            (VM_ALARM_0+8)
#define     VM_ALARM_NAME_3            (VM_ALARM_0+9)
#define     VM_ALARM_NAME_4            (VM_ALARM_0+10)
#define     VM_FM_INFO				   (VM_ALARM_0+11)

#define		CFG_RCSP_ADV_HIGH_LOW_VOL		 40
#define     CFG_RCSP_ADV_EQ_MODE_SETTING     41
#define     CFG_RCSP_ADV_EQ_DATA_SETTING     42
#define     ADV_SEQ_RAND                     43
#define     CFG_RCSP_ADV_TIME_STAMP          44
#define     CFG_RCSP_ADV_WORK_SETTING        45
#define     CFG_RCSP_ADV_MIC_SETTING         46
#define     CFG_RCSP_ADV_LED_SETTING         47
#define     CFG_RCSP_ADV_KEY_SETTING         48
#define 	CFG_USER_DEFINE_END			     49

//=================================================================================//
//                             只存VM配置项[50 ~ 99]                         	   //
//=================================================================================//
#define 	CFG_STORE_VM_ONLY_BEGIN		50

#define 	CFG_REMOTE_DB_INFO		    50
#define 	CFG_REMOTE_DB_00			51
#define 	CFG_REMOTE_DB_01			52
#define 	CFG_REMOTE_DB_02			53
#define 	CFG_REMOTE_DB_03			54
#define 	CFG_REMOTE_DB_04			55
#define 	CFG_REMOTE_DB_05			56
#define 	CFG_REMOTE_DB_06			57
#define 	CFG_REMOTE_DB_07			58
#define 	CFG_REMOTE_DB_08			59
#define 	CFG_REMOTE_DB_09			60
#define 	CFG_REMOTE_DB_10			61
#define 	CFG_REMOTE_DB_11			62
#define 	CFG_REMOTE_DB_12			63
#define 	CFG_REMOTE_DB_13			64
#define 	CFG_REMOTE_DB_14			65
#define 	CFG_REMOTE_DB_15			66
#define 	CFG_REMOTE_DB_16			67
#define 	CFG_REMOTE_DB_17			68
#define 	CFG_REMOTE_DB_18			69
#define 	CFG_REMOTE_DB_19			70
#define 	CFG_TWS_INFO				71
#define 	CFG_PHONE_VOL			    72
#define 	CFG_BT_OSC_INT_INFO		    73
#define 	CFG_FLASH_BREAKPOINT		74
#define 	CFG_USB_BREAKPOINT		    75
#define 	CFG_SD0_BREAKPOINT		    76
#define 	CFG_SD1_BREAKPOINT		    77
#define 	CFG_MUSIC_DEVICE			78
#define 	CFG_FM_RECEIVER_INFO		79
#define 	CFG_FM_TRANSMIT_INFO		80
#define 	CFG_DAC_TEST_VOLT		    81
#define 	CFG_BLE_MODE_INFO 		    82
#define     CFG_TWS_PAIR_AA             83
#define     CFG_TWS_CONNECT_AA          84
#define     CFG_MUSIC_VOL               85
#define     CFG_USER_RESET_SOURCE       86
#define     CFG_CHARGESTORE_TWS_CHANNEL 87
#define 	CFG_DAC_DTB					88
#define 	CFG_MC_BIAS					89
#define 	CFG_POR_FLAG				90
#define 	CFG_MIC_LDO_VSEL			91
#define 	CFG_AAP_MODE_INFO 		    92
#define 	CFG_DAC_TRIM_INFO		    93
#define     CFG_BT_TRIM_INFO			94
#define     CFG_ANC_COEFF				95
#define     CFG_PBG_MODE_INFO           96
#define 	CFG_EARTCH_ENABLE_ID 		97
#define     CFG_UI_SYS_INFO             98

#define 	CFG_STORE_VM_ONLY_END		99

//=================================================================================//
//         可以存于VM, sys_cfg.bin(默认值)和BTIF区域的配置项[100 ~ 127]            //
//		   (VM支持扩展到511)    												   //
//=================================================================================//
#define 	CFG_STORE_VM_BIN_BTIF_BEGIN	100

#define		CFG_BT_NAME    				101
#define     CFG_BT_MAC_ADDR             102
#define		CFG_DEFAULT_VOL_ID  		103
#define		CFG_SYS_VOL                 104
#define		CFG_SYS_EQ					105
#define		CFG_PC_SPK_VOL				106
#define		CFG_PC_MIC_VOL				107
#define 	VM_GMA_ALI_PARA				108
#define     CFG_BT_FRE_OFFSET			110
#define 	CFG_TWS_LOCAL_ADDR			111
#define 	CFG_TWS_REMOTE_ADDR			112
#define     CFG_TWS_COMMON_ADDR         113
#define     CFG_TWS_CHANNEL             114
#define 	VM_DMA_RAND					115
#define 	VM_GMA_MAC					116
#define 	VM_TME_AUTH_COOKIE			116

#if (VM_ITEM_MAX_NUM > 128)
#define		CFG_FLASH_BREAKPOINT0		117
#define		CFG_FLASH_BREAKPOINT1		118
#define		CFG_FLASH_BREAKPOINT2		119
#define		CFG_FLASH_BREAKPOINT3		120
#define		CFG_FLASH_BREAKPOINT4		121
#define		CFG_FLASH_BREAKPOINT5		122
#define		CFG_FLASH_BREAKPOINT6		123
#define		CFG_FLASH_BREAKPOINT7		124
#define		CFG_FLASH_BREAKPOINT8		125
#define		CFG_FLASH_BREAKPOINT9		126

#define 	CFG_USB_BREAKPOINT0			127
#define 	CFG_USB_BREAKPOINT1			128
#define 	CFG_USB_BREAKPOINT2			129
#define 	CFG_USB_BREAKPOINT3			130
#define 	CFG_USB_BREAKPOINT4			131
#define 	CFG_USB_BREAKPOINT5			132
#define 	CFG_USB_BREAKPOINT6			133
#define 	CFG_USB_BREAKPOINT7			134
#define 	CFG_USB_BREAKPOINT8			135
#define 	CFG_USB_BREAKPOINT9			136


#define 	CFG_SD0_BREAKPOINT0		    137
#define 	CFG_SD0_BREAKPOINT1		    138
#define 	CFG_SD0_BREAKPOINT2		    139
#define 	CFG_SD0_BREAKPOINT3		    140
#define 	CFG_SD0_BREAKPOINT4		    141
#define 	CFG_SD0_BREAKPOINT5		    142
#define 	CFG_SD0_BREAKPOINT6		    143
#define 	CFG_SD0_BREAKPOINT7		    144
#define 	CFG_SD0_BREAKPOINT8		    145
#define 	CFG_SD0_BREAKPOINT9		    146


#define 	CFG_SD1_BREAKPOINT0		    147
#define 	CFG_SD1_BREAKPOINT1		    148
#define 	CFG_SD1_BREAKPOINT2		    149
#define 	CFG_SD1_BREAKPOINT3		    150
#define 	CFG_SD1_BREAKPOINT4		    151
#define 	CFG_SD1_BREAKPOINT5		    152
#define 	CFG_SD1_BREAKPOINT6		    153
#define 	CFG_SD1_BREAKPOINT7		    154
#define 	CFG_SD1_BREAKPOINT8		    155
#define 	CFG_SD1_BREAKPOINT9		    156

#define     CFG_CHGBOX_ADDR             157

#define 	CFG_REMOTE_DN_00		    158
#define 	CFG_REMOTE_DN_01		    159
#define 	CFG_REMOTE_DN_02		    160
#define 	CFG_REMOTE_DN_03		    161
#define 	CFG_REMOTE_DN_04		    162
#define 	CFG_REMOTE_DN_05		    163
#define 	CFG_REMOTE_DN_06		    164
#define 	CFG_REMOTE_DN_07		    165
#define 	CFG_REMOTE_DN_08		    166
#define 	CFG_REMOTE_DN_09		    167
#define 	CFG_REMOTE_DN_10		    168
#define 	CFG_REMOTE_DN_11		    169
#define 	CFG_REMOTE_DN_12		    170
#define 	CFG_REMOTE_DN_13		    171
#define 	CFG_REMOTE_DN_14		    172
#define 	CFG_REMOTE_DN_15		    173
#define 	CFG_REMOTE_DN_16		    174
#define 	CFG_REMOTE_DN_17		    175
#define 	CFG_REMOTE_DN_18		    176
#define 	CFG_REMOTE_DN_19		    177


#endif//(VM_ITEM_MAX_NUM > 128)

#define 	CFG_STORE_VM_BIN_BTIF_END	(VM_ITEM_MAX_NUM - 1) //在app_cfg文件中配置128/256

//=================================================================================//
//                   只存于sys_cfg.bin的配置项[512 ~ 700]                		   //
//=================================================================================//
#define 	CFG_STORE_BIN_ONLY_BEGIN	512
//硬件类配置项[513 ~ 600]
#define		CFG_UART_ID		    		513
#define		CFG_HWI2C_ID				514
#define		CFG_SWI2C_ID				515
#define		CFG_HWSPI_ID				516
#define		CFG_SWSPI_ID				517
#define		CFG_SD_ID					518
#define		CFG_USB_ID					519
#define		CFG_LCD_ID					520
#define		CFG_TOUCH_ID				521
#define		CFG_IOKEY_ID				522
#define		CFG_ADKEY_ID				523
#define		CFG_AUDIO_ID				524
#define		CFG_VIDEO_ID				525
#define		CFG_WIFI_ID					526
#define		CFG_NIC_ID					527
#define		CFG_LED_ID					528
#define		CFG_POWER_MANG_ID			529
#define		CFG_IRFLT_ID				530
#define		CFG_PLCNT_ID				531
#define		CFG_PWMLED_ID				532
#define		CFG_RDEC_ID					533
#define		CFG_CHARGE_STORE_ID			534
#define		CFG_CHARGE_ID				535
#define    	CFG_LOWPOWER_V_ID   	    536
#define    	CFG_MIC_TYPE_ID   			537
#define    	CFG_COMBINE_SYS_VOL_ID 		538
#define    	CFG_COMBINE_CALL_VOL_ID 	539
#define    	CFG_LP_TOUCH_KEY_ID   		540

//蓝牙类配置项[601 ~ 650]
#define     CFG_BT_RF_POWER_ID			601
#define    	CFG_TWS_PAIR_CODE_ID   		602
#define    	CFG_AUTO_OFF_TIME_ID   	    603
#define    	CFG_AEC_ID   	            604
#define    	CFG_UI_TONE_STATUS_ID   	605
#define    	CFG_KEY_MSG_ID   			606
#define    	CFG_LRC_ID   				607

//其它类配置项[651 ~ 700]
#define 	CFG_STORE_BIN_ONLY_END		700



#endif

