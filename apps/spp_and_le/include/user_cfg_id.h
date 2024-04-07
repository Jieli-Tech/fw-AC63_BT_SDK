#ifndef _USER_CFG_ID_H_
#define _USER_CFG_ID_H_

//=================================================================================//
//      与APP CASE相关配置项[1 ~ 49], 其他id 在syscfg_id.h有定义使用               //
//NOTE: 不同的example可以复用同一个ID,没用过该功能的ID也可以重新分配占用           //
//=================================================================================//

#define     VM_USB_MIC_GAIN                  1
#define     VM_ALARM_0                  	 2
#define     VM_ALARM_1                  	 3
#define     VM_ALARM_2                  	 4
#define     VM_ALARM_3                  	 5
#define     VM_ALARM_4                  	 6
#define     VM_ALARM_MASK               	 7
#define     VM_ALARM_NAME_0             	 8
#define     VM_ALARM_NAME_1             	 9
#define     VM_ALARM_NAME_2             	 10
#define     VM_ALARM_NAME_3             	 11
#define     VM_ALARM_NAME_4             	 12

#define     CFG_HILINK_AUTH_INFO_SETTING     13
#define     CFG_HILINK_ATTR_INFO_SETTING     14

#define     VM_LP_TOUCH_KEY_ALOG_CFG         17
#define     LP_KEY_EARTCH_TRIM_VALUE         18

// #define		CFG_RCSP_ADV_HIGH_LOW_VOL		 19
// #define     CFG_RCSP_ADV_EQ_MODE_SETTING     20
// #define     CFG_RCSP_ADV_EQ_DATA_SETTING     21
#define     ADV_SEQ_RAND                     22
#define     CFG_RCSP_ADV_TIME_STAMP          23
#define     CFG_RCSP_ADV_WORK_SETTING        24
#define     CFG_RCSP_ADV_MIC_SETTING         25
#define     CFG_RCSP_ADV_LED_SETTING         26
#define     CFG_RCSP_ADV_KEY_SETTING         27
#define     CFG_AAP_MODE_INFO                28
#define     CFG_DONGLE_PAIR_INFO             29

#define     CFG_BLE_BONDING_REMOTE_INFO      30
#define     CFG_BLE_BONDING_REMOTE_INFO2     31

//findmy,id 复用
#define     CFG_FMNA_BLE_ADDRESS_INFO        30
#define     CFG_FMNA_SOFTWARE_AUTH_START     31
#define     CFG_FMNA_SOFTWARE_AUTH_END       (CFG_FMNA_SOFTWARE_AUTH_START + 4)
#define     CFG_FMY_INFO                     36

#define     VM_VIR_RTC_TIME             47
#define     VM_VIR_ALM_TIME             48
#define     VM_VIR_SUM_NSEC             49
#endif /* #ifndef _USER_CFG_ID_H_ */
