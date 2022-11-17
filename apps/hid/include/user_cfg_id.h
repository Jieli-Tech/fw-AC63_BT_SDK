#ifndef _USER_CFG_ID_H_
#define _USER_CFG_ID_H_

//=================================================================================//
//      与APP CASE相关配置项[1 ~ 49], 其他id 在syscfg_id.h有定义使用               //
//NOTE: 不同的example可以复用同一个ID,没用过该功能的ID也可以重新分配占用           //
//=================================================================================//
#define     CFG_BLE_BONDING_REMOTE_INFO      1  //1~4

#define     VM_USB_MIC_GAIN             	 5
#define     VM_ALARM_0                  	 6
#define     VM_ALARM_1                  	 7
#define     VM_ALARM_2                  	 8
#define     VM_ALARM_3                  	 9
#define     VM_ALARM_4                  	 10
#define     VM_ALARM_MASK               	 11
#define     VM_ALARM_NAME_0             	 12
#define     VM_ALARM_NAME_1             	 13
#define     VM_ALARM_NAME_2             	 14
#define     VM_ALARM_NAME_3             	 15
#define     VM_ALARM_NAME_4             	 16

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

#define     CFG_BLE_ADDRESS_BEGIN            30 //记录ble 可变地址表
#define     CFG_BT_IDX_NUM                   4
#define     CFG_EDR_ADDRESS_BEGIN            (CFG_BLE_ADDRESS_BEGIN + CFG_BT_IDX_NUM) //记录配对的对方地址
#define     CFG_BLE_IRK_NUMBER               (CFG_EDR_ADDRESS_BEGIN + CFG_BT_IDX_NUM)
#define     CFG_CUR_BT_IDX                   (CFG_BLE_IRK_NUMBER + CFG_BT_IDX_NUM)
#define     CFG_HID_MODE_BEGIN               (CFG_CUR_BT_IDX + 1)

#define     CFG_AAP_MODE_EDR_ADDR            44
#define     CFG_AAP_MODE_BLE_ADDR            45
#define     CFG_AAP_MODE_24G_ADDR            46
#define     CFG_COORDINATE_ADDR              47

#endif /* #ifndef _USER_CFG_ID_H_ */
