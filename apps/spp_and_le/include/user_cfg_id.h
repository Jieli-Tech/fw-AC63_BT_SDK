#ifndef _USER_CFG_ID_H_
#define _USER_CFG_ID_H_

//=================================================================================//
//                            与APP CASE相关配置项[1 ~ 60]                         //
//=================================================================================//
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
#define     CFG_DONGLE_PAIR_INFO            29
#define     CFG_BT_IDX_NUM                   4
#define     CFG_EDR_ADDRESS_BEGIN            (CFG_BLE_ADDRESS_BEGIN + CFG_BT_IDX_NUM)
#define     CFG_CUR_BT_IDX                   (CFG_EDR_ADDRESS_BEGIN + CFG_BT_IDX_NUM)
#define     CFG_HID_MODE_BEGIN               (CFG_CUR_BT_IDX + 1)

#define     VM_USB_MIC_GAIN             	 5
#endif /* #ifndef _USER_CFG_ID_H_ */
