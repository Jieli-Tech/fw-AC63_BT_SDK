/*********************************************************************************************
    *   Filename        : app_keyboard.c

    *   Description     :

    *   Author          :

    *   Email           :

    *   Last modifiled  : 2019-07-05 10:09

    *   Copyright:(c)JIELI  2011-2019  @ , All Rights Reserved.
*********************************************************************************************/
#include "system/app_core.h"
#include "system/includes.h"
#include "server/server_core.h"
#include "app_config.h"
#include "app_action.h"
#include "os/os_api.h"
#include "btcontroller_config.h"
#include "btctrler/btctrler_task.h"
#include "config/config_transport.h"
#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "bt_common.h"
#include "edr_hid_user.h"
/* #include "code_switch.h" */
/* #include "omsensor/OMSensor_manage.h" */
#include "le_common.h"
#include <stdlib.h>
#include "standard_hid.h"
#include "rcsp_bluetooth.h"
#include "app_charge.h"
#include "app_power_manage.h"
#include "app_chargestore.h"
#include "app_comm_bt.h"

#if(CONFIG_APP_KEYPAGE)

#define LOG_TAG_CONST       KEYPAGE
#define LOG_TAG             "[KEYPAGE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define trace_run_debug_val(x)   //log_info("\n## %s: %d,  0x%04x ##\n",__FUNCTION__,__LINE__,x)


static u16 g_auto_shutdown_timer = 0;

//---------------------------------------------------------------------
#if SNIFF_MODE_RESET_ANCHOR
#define SNIFF_MODE_TYPE               SNIFF_MODE_ANCHOR
#define SNIFF_CNT_TIME                1/////<空闲?S之后进入sniff模式

#define SNIFF_MAX_INTERVALSLOT        16
#define SNIFF_MIN_INTERVALSLOT        16
#define SNIFF_ATTEMPT_SLOT            2
#define SNIFF_TIMEOUT_SLOT            1
#define SNIFF_CHECK_TIMER_PERIOD      200
#else

#define SNIFF_MODE_TYPE               SNIFF_MODE_DEF
#define SNIFF_CNT_TIME                5/////<空闲?S之后进入sniff模式

#define SNIFF_MAX_INTERVALSLOT        800
#define SNIFF_MIN_INTERVALSLOT        100
#define SNIFF_ATTEMPT_SLOT            4
#define SNIFF_TIMEOUT_SLOT            1
#define SNIFF_CHECK_TIMER_PERIOD      1000
#endif

//默认配置
static const edr_sniff_par_t keypage_sniff_param = {
    .sniff_mode = SNIFF_MODE_TYPE,
    .cnt_time = SNIFF_CNT_TIME,
    .max_interval_slots = SNIFF_MAX_INTERVALSLOT,
    .min_interval_slots = SNIFF_MIN_INTERVALSLOT,
    .attempt_slots = SNIFF_ATTEMPT_SLOT,
    .timeout_slots = SNIFF_TIMEOUT_SLOT,
    .check_timer_period = SNIFF_CHECK_TIMER_PERIOD,
};

typedef enum {
    HID_MODE_NULL = 0,
    HID_MODE_EDR,
    HID_MODE_BLE,
    HID_MODE_INIT = 0xff
} bt_mode_e;
static bt_mode_e bt_hid_mode;
static volatile u8 is_keypage_active = 0; //1-临界点,系统不允许进入低功耗，0-系统可以进入低功耗
static u8 connect_remote_type = REMOTE_DEV_UNKNOWN;        //

#define REMOTE_IS_IOS() (connect_remote_type == REMOTE_DEV_IOS)

//----------------------------------
static const u8 keypage_report_map[] = {
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x03,        //   Report ID (3)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x0B,        //   Report Count (11)
    0x09, 0xEA,        //   Usage (Volume Decrement)
    0x09, 0xE9,        //   Usage (Volume Increment)
    0x0A, 0xAE, 0x01,  //   Usage (AL Keyboard Layout)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x0D,        //   Report Size (13)
    0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
    0x05, 0x0D,        // Usage Page (Digitizer)
    0x09, 0x02,        // Usage (Pen)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x02,        //   Report ID (2)
    0x09, 0x22,        //   Usage (Finger)
    0xA1, 0x02,        //   Collection (Logical)
    0x09, 0x42,        //     Usage (Tip Switch)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x75, 0x01,        //     Report Size (1)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x32,        //     Usage (In Range)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x06,        //     Report Count (6)
    0x81, 0x03,        //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x08,        //     Report Size (8)
    0x09, 0x51,        //     Usage (0x51)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x26, 0xFF, 0x0F,  //     Logical Maximum (4095)
    0x75, 0x10,        //     Report Size (16)
    0x55, 0x0E,        //     Unit Exponent (-2)
    0x65, 0x33,        //     Unit (System: English Linear, Length: Inch)
    0x09, 0x30,        //     Usage (X)
    0x35, 0x00,        //     Physical Minimum (0)
    0x46, 0xB5, 0x04,  //     Physical Maximum (1205)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x46, 0x8A, 0x03,  //     Physical Maximum (906)
    0x09, 0x31,        //     Usage (Y)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0x05, 0x0D,        //   Usage Page (Digitizer)
    0x09, 0x54,        //   Usage (0x54)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x08,        //   Report ID (8)
    0x09, 0x55,        //   Usage (0x55)
    0x25, 0x05,        //   Logical Maximum (5)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection

    // 119 bytes
};


static const u8 keypage_report_map_ios[] = {
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x03,        //   Report ID (3)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x0B,        //   Report Count (11)
    0x09, 0xEA,        //   Usage (Volume Decrement)
    0x09, 0xE9,        //   Usage (Volume Increment)
    0x0A, 0xAE, 0x01,  //   Usage (AL Keyboard Layout)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x0D,        //   Report Size (13)
    0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
    0x05, 0x0D,        // Usage Page (Digitizer)
    0x09, 0x02,        // Usage (Pen)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x02,        //   Report ID (2)
    0x09, 0x22,        //   Usage (Finger)
    0xA1, 0x02,        //   Collection (Logical)
    0x09, 0x42,        //     Usage (Tip Switch)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x75, 0x01,        //     Report Size (1)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x32,        //     Usage (In Range)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x06,        //     Report Count (6)
    0x81, 0x03,        //     Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x08,        //     Report Size (8)
    0x09, 0x51,        //     Usage (0x51)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x26, 0xFF, 0x0F,  //     Logical Maximum (4095)
    0x75, 0x10,        //     Report Size (16)
    0x55, 0x0E,        //     Unit Exponent (-2)
    0x65, 0x33,        //     Unit (System: English Linear, Length: Inch)
    0x09, 0x30,        //     Usage (X)
    0x35, 0x00,        //     Physical Minimum (0)
    0x46, 0xB5, 0x04,  //     Physical Maximum (1205)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x46, 0x8A, 0x03,  //     Physical Maximum (906)
    0x09, 0x31,        //     Usage (Y)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0x05, 0x0D,        //   Usage Page (Digitizer)
    0x09, 0x54,        //   Usage (0x54)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x85, 0x08,        //   Report ID (8)
    0x09, 0x55,        //   Usage (0x55)
    0x25, 0x05,        //   Logical Maximum (5)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x04,        //   Report ID (4)
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x95, 0x05,        //     Report Count (5)
    0x75, 0x01,        //     Report Size (1)
    0x05, 0x09,        //     Usage Page (Button)
    0x19, 0x01,        //     Usage Minimum (0x01)
    0x29, 0x05,        //     Usage Maximum (0x05)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //     Report Count (1)
    0x75, 0x03,        //     Report Size (3)
    0x81, 0x01,        //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x01,        //     Report Count (1)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x38,        //     Usage (Wheel)
    0x15, 0x81,        //     Logical Minimum (-127)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x0C,        //     Usage Page (Consumer)
    0x0A, 0x38, 0x02,  //     Usage (AC Pan)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0x85, 0x05,        //   Report ID (5)
    0x09, 0x01,        //   Usage (Consumer Control)
    0xA1, 0x00,        //   Collection (Physical)
    0x75, 0x0C,        //     Report Size (12)
    0x95, 0x02,        //     Report Count (2)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x16, 0x01, 0xF8,  //     Logical Minimum (-2047)
    0x26, 0xFF, 0x07,  //     Logical Maximum (2047)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0xC0,              // End Collection

    // 203 bytes
};

//----------------------------------
static const edr_init_cfg_t keypage_edr_config = {
    .page_timeout = 8000,
    .super_timeout = 8000,
    .io_capabilities = 3,
    .passkey_enable = 0,
    .authentication_req = 2,
    .oob_data = 0,
    .sniff_param = &keypage_sniff_param,
    .class_type = BD_CLASS_KEYBOARD,
    .report_map = keypage_report_map,
    .report_map_size = sizeof(keypage_report_map),
};

//----------------------------------
static const ble_init_cfg_t keypage_ble_config = {
    .same_address = 0,
    .appearance = BLE_APPEARANCE_HID_KEYBOARD,
    .report_map = keypage_report_map,
    .report_map_size = sizeof(keypage_report_map),
};
static int handle_send_packect(const u8 *packet_table, u16 table_size, u8 packet_len);



#define PACKET_DELAY_TIME()        os_time_dly(3)
#define DOUBLE_KEY_DELAY_TIME()    os_time_dly(5)

///音量加减
#define  REPROT_CONSUMER_LEN      (1+3)
static const u8 key_vol_add[][REPROT_CONSUMER_LEN] = {
    {3, 0x02, 0x00, 0x00 },
    {3, 0x00, 0x00, 0x00 },
};
static const u8 key_vol_dec[][REPROT_CONSUMER_LEN] = {
    {3, 0x01, 0x00, 0x00 },
    {3, 0x00, 0x00, 0x00 },
};

void key_vol_ctrl(u8 add_dec)
{
    r_printf("%s[%d]", __func__, add_dec);
    if (add_dec) {
        handle_send_packect(key_vol_add, sizeof(key_vol_add), REPROT_CONSUMER_LEN);
    } else {
        handle_send_packect(key_vol_dec, sizeof(key_vol_dec), REPROT_CONSUMER_LEN);
    }
}

//report_id + data
#define  REPROT_INFO_LEN0         (1+3)
#define  REPROT_INFO_LEN1         (1+8)
//------------------------------------------------------
static const u8 key_idle_ios_0[][REPROT_INFO_LEN0] = {
    //松手
    {5, 0x00, 0xE0, 0xFF },
    {4, 0x00, 0x00, 0x00 },
};
static const u8 key_idle_ios_1[][REPROT_INFO_LEN0] = {
    //松手
    {5, 0x00, 0x10, 0x00 },
    {4, 0x00, 0x00, 0x00 },
};

extern u8 get_key_active(void);
void bt_connect_timer_loop(void *priv)
{
    static u8 flag = 0;
#if TCFG_USER_BLE_ENABLE
    if (!ble_hid_is_connected()) {
        return;
    }
#endif
#if TCFG_USER_EDR_ENABLE
    if (!edr_hid_is_connected()) {
        return;
    }
#endif
    if (get_key_active()) {
        return;
    }
    if (REMOTE_IS_IOS()) {
        if (flag) {
            handle_send_packect(key_idle_ios_0, sizeof(key_idle_ios_0), REPROT_INFO_LEN0);
            flag = 0;
        } else {
            handle_send_packect(key_idle_ios_1, sizeof(key_idle_ios_1), REPROT_INFO_LEN0);
            flag = 1;
        }
    }
}

//连接复位坐标
static const u8 key_connect_before[][REPROT_INFO_LEN0] = {
    {4, 0x00, 0x00, 0x00 },
    {4, 0x00, 0x00, 0x00 },
    /* {5, 0x01, 0xF8, 0x7F }, //左下*/
    /* {5, 0xFF, 0x17, 0x80 }, //右上 */
    {5, 0x01, 0x18, 0x80 }, //左上
    {5, 0x01, 0x18, 0x80 }, //左上
    {5, 0x01, 0x18, 0x80 }, //左上
    //复位坐标点，发松手坐标

    {5, 0x30, 0x00, 0x00 },
    {5, 0x30, 0x00, 0x00 },
    {5, 0x30, 0x00, 0x00 },

    {5, 0x00, 0x00, 0x04 },
    {5, 0x00, 0x00, 0x04 },
    {5, 0x00, 0x00, 0x04 },

    {4, 0x00, 0x00, 0x00 },
};

void bt_connect_reset_xy(void)
{
    r_printf("%s", __func__);
#if TCFG_USER_BLE_ENABLE
    if (!ble_hid_is_connected()) {
        return;
    }
#endif
#if TCFG_USER_EDR_ENABLE
    if (!edr_hid_is_connected()) {
        return;
    }
#endif
    if (REMOTE_IS_IOS()) {
        handle_send_packect(key_connect_before, sizeof(key_connect_before), REPROT_INFO_LEN0);
    }
}

//上键（短按）
static const u8 key_up_before[][REPROT_INFO_LEN0] = {//上一个
    {4, 0x01, 0x00, 0x00 },
    {5, 0x00, 0x00, 0x02 },
    {5, 0x00, 0x00, 0x03 },
    {4, 0x00, 0x00, 0x00 },

    {5, 0x00, 0x00, 0xFE },
    {5, 0x00, 0x00, 0xFD },
    {4, 0x00, 0x00, 0x00 },
};

//Report ID (2)
static const u8 key_up_click[][REPROT_INFO_LEN1] = {
    {2, 0x07, 0x06, 0x70, 0x07, 0xf4, 0x03, 0x01, 0x00 },
    {2, 0x07, 0x06, 0x70, 0x07, 0x4c, 0x04, 0x01, 0x00 },
    {2, 0x07, 0x06, 0x70, 0x07, 0x78, 0x05, 0x01, 0x00 },
    {2, 0x07, 0x06, 0x70, 0x07, 0xa4, 0x06, 0x01, 0x00 },
    {2, 0x07, 0x06, 0x70, 0x07, 0xd0, 0x07, 0x01, 0x00 },
    {2, 0x07, 0x06, 0x70, 0x07, 0xfc, 0x08, 0x01, 0x00 },
    {2, 0x07, 0x06, 0x70, 0x07, 0x28, 0x0a, 0x01, 0x00 },
    {2, 0x07, 0x06, 0x70, 0x07, 0x54, 0x0b, 0x01, 0x00 },
    {2, 0x07, 0x06, 0x70, 0x07, 0x80, 0x0c, 0x01, 0x00 },
    {2, 0x00, 0x06, 0x70, 0x07, 0xac, 0x0d, 0x00, 0x00 },
};

//上键（HOLD） 收到按键HOLD消息发1次，
//Report ID (3)
static const u8 key_up_hold_press[][REPROT_INFO_LEN0] = {
    {3, 0x02, 0x00, 0x00 },
};
static const u8 key_up_hold_release[][REPROT_INFO_LEN0] = {
    {3, 0x00, 0x00, 0x00 },
};

//------------------------------------------------------
//下键（短按）
static const u8 key_down_before[][REPROT_INFO_LEN0] = {//下一个
    {4, 0x01, 0x00, 0x00 },
    {5, 0x00, 0x00, 0xFE },
    {5, 0x00, 0x00, 0xFD },
    {4, 0x00, 0x00, 0x00 },

    {5, 0x00, 0x00, 0x02 },
    {5, 0x00, 0x00, 0x03 },
    {4, 0x00, 0x00, 0x00 },
};

static const u8 key_down_click[][REPROT_INFO_LEN1] = {
    //Report ID (2)
    {2, 0x07, 0x06, 0x70, 0x07, 0x80, 0x0c, 0x01, 0x00 },
    {2, 0x07, 0x06, 0x70, 0x07, 0x28, 0x0a, 0x01, 0x00 },
    {2, 0x07, 0x06, 0x70, 0x07, 0xfc, 0x08, 0x01, 0x00 },
    {2, 0x07, 0x06, 0x70, 0x07, 0xd0, 0x07, 0x01, 0x00 },
    {2, 0x07, 0x06, 0x70, 0x07, 0xa4, 0x06, 0x01, 0x00 },
    {2, 0x07, 0x06, 0x70, 0x07, 0x78, 0x05, 0x01, 0x00 },
    {2, 0x07, 0x06, 0x70, 0x07, 0x4c, 0x04, 0x01, 0x00 },
    {2, 0x07, 0x06, 0x70, 0x07, 0x20, 0x03, 0x01, 0x00 },
    {2, 0x07, 0x06, 0x70, 0x07, 0xf4, 0x01, 0x01, 0x00 },
    {2, 0x00, 0x06, 0x70, 0x07, 0xc8, 0x00, 0x00, 0x00 },
};

//下键（HOLD） 收到按键HOLD消息发1次
//Report ID (3)
static const u8 key_down_hold_press[][REPROT_INFO_LEN0] = {
    {3, 0x01, 0x00, 0x00 },
};

static const u8 key_down_hold_release[][REPROT_INFO_LEN0] = {
    {3, 0x00, 0x00, 0x00 }
};

//---------------------------------------------------------
//左键（短按）
static const u8 key_left_before[][REPROT_INFO_LEN0] = { //左滑
    {4, 0x01, 0x00, 0x00 },
    {5, 0x20, 0x00, 0x00 },
    {5, 0x20, 0x00, 0x00 },
    {4, 0x00, 0x00, 0x00 },

    {5, 0xE0, 0x0F, 0x00 },
    {5, 0xE0, 0x0F, 0x00 },
    {4, 0x00, 0x00, 0x00 },
};

static const u8 key_left_click[][REPROT_INFO_LEN1] = {
    //Report ID (2)
    {2, 0x07, 0x04, 0x00, 0x02, 0x70, 0x04, 0x01, 0x00 },
    {2, 0x07, 0x04, 0x8a, 0x02, 0x70, 0x04, 0x01, 0x00 },
    {2, 0x07, 0x04, 0xb4, 0x03, 0x70, 0x04, 0x01, 0x00 },
    {2, 0x07, 0x04, 0xe2, 0x04, 0x70, 0x04, 0x01, 0x00 },
    {2, 0x07, 0x04, 0x0e, 0x06, 0x70, 0x04, 0x01, 0x00 },
    {2, 0x07, 0x04, 0x3a, 0x07, 0x70, 0x04, 0x01, 0x00 },
    {2, 0x07, 0x04, 0x66, 0x08, 0x70, 0x04, 0x01, 0x00 },
    {2, 0x07, 0x04, 0x92, 0x09, 0x70, 0x04, 0x01, 0x00 },
    {2, 0x07, 0x04, 0xbe, 0x0a, 0x70, 0x04, 0x01, 0x00 },
    {2, 0x07, 0x04, 0xea, 0x0b, 0x70, 0x04, 0x01, 0x00 },
    {2, 0x00, 0x04, 0x16, 0x0d, 0x70, 0x04, 0x00, 0x00 },
};

//---------------------------------------------------------
//右键（短按）
static const u8 key_right_before[][REPROT_INFO_LEN0] = {//右滑
    {4, 0x01, 0x00, 0x00 },
    {5, 0xE0, 0x0F, 0x00 },
    {5, 0xE0, 0x0F, 0x00 },
    {4, 0x00, 0x00, 0x00 },

    {5, 0x20, 0x00, 0x00 },
    {5, 0x20, 0x00, 0x00 },
    {4, 0x00, 0x00, 0x00 },
};

static const u8 key_right_click[][REPROT_INFO_LEN1] = {
    //Report ID (2)
    {2, 0x07, 0x05, 0x00, 0x0d, 0x70, 0x04, 0x01, 0x00 },
    {2, 0x07, 0x05, 0x48, 0x0d, 0x70, 0x04, 0x01, 0x00 },
    {2, 0x07, 0x05, 0x1c, 0x0c, 0x70, 0x04, 0x01, 0x00 },
    {2, 0x07, 0x05, 0xf0, 0x0a, 0x70, 0x04, 0x01, 0x00 },
    {2, 0x07, 0x05, 0xc4, 0x09, 0x70, 0x04, 0x01, 0x00 },
    {2, 0x07, 0x05, 0x98, 0x08, 0x70, 0x04, 0x01, 0x00 },
    {2, 0x07, 0x05, 0x6c, 0x07, 0x70, 0x04, 0x01, 0x00 },
    {2, 0x07, 0x05, 0x40, 0x06, 0x70, 0x04, 0x01, 0x00 },
    {2, 0x07, 0x05, 0x14, 0x05, 0x70, 0x04, 0x01, 0x00 },
    {2, 0x07, 0x05, 0xe8, 0x03, 0x70, 0x04, 0x01, 0x00 },
    {2, 0x00, 0x05, 0xbc, 0x02, 0x70, 0x04, 0x00, 0x00 },
};

//---------------------------------------------------------
//中键（短按，长按）
//-------按下-------
static u8 key_pp_press[][REPROT_INFO_LEN1] = {
    //Report ID (2)
    {2, 0x07, 0x07, 0x70, 0x07, 0x70, 0x07, 0x01, 0x00 },
};

//-------松开-------
static u8 key_pp_release_before[][REPROT_INFO_LEN0] = {//点赞
    {4, 0x01, 0x00, 0x00 },
    {4, 0x00, 0x00, 0x00 },
};

static u8 key_pp_release[][REPROT_INFO_LEN1] = {
    //Report ID (2)
    {2, 0x00, 0x07, 0x70, 0x07, 0x70, 0x07, 0x00, 0x00  },
};

//双击相当于连续发两次
//HOLD动作 就是先发按下，等松手再发松开包

//---------------------------------------------------------
//单键（短按，长按）
//-------按下-------
static const u8 key_one_press_before[][REPROT_INFO_LEN0] = {//暂停
    {4, 0x01, 0x00, 0x00 },
};

static const u8 key_one_press[][REPROT_INFO_LEN1] = {
    //Report ID (2)
    {2, 0x07, 0x08, 0x00, 0x08, 0x60, 0x0d, 0x01, 0x00  },
};

//-------松开-------
static const u8 key_one_release_before[][REPROT_INFO_LEN0] = {
    //Report ID (4)
    {4, 0x00, 0x00, 0x00 },
};
static const u8 key_one_release[][REPROT_INFO_LEN1] = {
    //Report ID (2)
    {2, 0x00, 0x08, 0x00, 0x08, 0x60, 0x0d, 0x00, 0x00  },
};

//------
//双击相当于连续发两次
//HOLD动作 就是先发按下，等松手再发松开包

//------------------------------------------------------
static u16  x_lab, y_lab;
static u16  x_lab_low = 0, x_lab_hig = 0, y_lab_low = 0, y_lab_hig = 0;
void keypage_coordinate_vm_deal(u8 flag);
extern void p33_soft_reset(void);
extern int ble_hid_data_send(u8 report_id, u8 *data, u16 len);
extern int ble_hid_is_connected(void);
static void keypage_set_soft_poweroff(void);
static void keypage_app_select_btmode(u8 mode);

//------------------------------------------------------

static int handle_send_packect(const u8 *packet_table, u16 table_size, u8 packet_len)
{
    int i;
    u8 *pt = packet_table;
    void (*hid_data_send_pt)(u8 report_id, u8 * data, u16 len) = NULL;

    if (bt_hid_mode == HID_MODE_EDR) {
#if TCFG_USER_EDR_ENABLE
        hid_data_send_pt = edr_hid_data_send;
#endif
    } else {
#if TCFG_USER_BLE_ENABLE
        hid_data_send_pt = ble_hid_data_send;
#endif
    }

    if (!hid_data_send_pt) {
        log_info("no bt");
        return -1;
    }

    log_info("remote_type:%d", connect_remote_type);
    log_info("send_packet:%08x,size=%d,len=%d", packet_table, table_size, packet_len);

    for (i = 0; i < table_size; i += packet_len) {
        put_buf(pt, packet_len);
        hid_data_send_pt(pt[0], &pt[1], packet_len - 1);
        pt += packet_len;
        PACKET_DELAY_TIME();
    }
    return 0;
}

static void keypage_auto_shutdown_disable(void)
{
    if (g_auto_shutdown_timer) {
        sys_timeout_del(g_auto_shutdown_timer);
    }
}

static u8 key_focus_ios[][REPROT_INFO_LEN0] = {//相机移动焦点
    {4, 0x00, 0x00, 0x00 },
    {5, 0x00, 0x00, 0x00 },
};
#define MOUSE_STEP   16//8
static void keypage_coordinate_equal_ios(u8 x, u8 y)
{
    if ((x == 0) && (y == 0)) { /// 复位原点
        bt_connect_reset_xy();
        return;
    }
    u16 ios_x = 0, ios_y = 0;
    u8 *pos = key_focus_ios[1];
    if (x == 1) {
        ios_x = MOUSE_STEP;
    }
    if (x == 2) {
        ios_x = 0x0FFF - MOUSE_STEP;
    }
    if (y == 1) {
        ios_y = MOUSE_STEP;
    }
    if (y == 2) {
        ios_y = 0x0FFF - MOUSE_STEP;
    }
    pos[1] = (ios_x & 0xFF);
    pos[2] = ((ios_y & 0x0F) << 4) | (ios_x >> 8 & 0x0F);
    pos[3] = (ios_y >> 4 & 0xFF);
    put_buf(pos, REPROT_INFO_LEN0);
    if (REMOTE_IS_IOS()) {
        handle_send_packect(key_focus_ios, sizeof(key_focus_ios), REPROT_INFO_LEN0);
    }
}

static void keypage_coordinate_equal(void)
{
    if (REMOTE_IS_IOS()) {
        return;
    }
    x_lab_low = x_lab & 0xff;
    x_lab_hig = x_lab >> 8;
    y_lab_low = y_lab & 0xff;
    y_lab_hig = y_lab >> 8;
    key_pp_press[0][3] = x_lab_low;
    key_pp_press[0][4] = x_lab_hig;
    key_pp_press[0][5] = y_lab_low;
    key_pp_press[0][6] = y_lab_hig;
    key_pp_release[0][3] = x_lab_low;
    key_pp_release[0][4] = x_lab_hig;
    key_pp_release[0][5] = y_lab_low;
    key_pp_release[0][6] = y_lab_hig;
    /* r_printf("0x%x,0x%x\n",y_lab_low,y_lab_hig); */
    put_buf(key_pp_press, sizeof(key_pp_press));
    handle_send_packect(key_pp_press, sizeof(key_pp_press), REPROT_INFO_LEN1);
    handle_send_packect(key_pp_release, sizeof(key_pp_release), REPROT_INFO_LEN1);
}

//---------------------------------------------------------------------------------------
static void keypage_app_key_deal_test(u8 key_type, u8 key_value)
{
    u16 key_msg = 0;
    log_info("app_key_evnet: %d,%d\n", key_type, key_value);

#if TCFG_USER_EDR_ENABLE
    if (bt_hid_mode == HID_MODE_EDR && !edr_hid_is_connected()) {
        if (bt_connect_phone_back_start()) { //回连
            log_info("edr reconnect");
            return;
        }
    }
#endif

    if (key_type == KEY_EVENT_CLICK) {
        switch (key_value) {
        case 0:
            if (REMOTE_IS_IOS()) {
                handle_send_packect(key_up_before, sizeof(key_up_before), REPROT_INFO_LEN0);
            } else {
                handle_send_packect(key_up_click, sizeof(key_up_click), REPROT_INFO_LEN1);
            }
            break;

        case 1:
            if (REMOTE_IS_IOS()) {
                handle_send_packect(key_down_before, sizeof(key_down_before), REPROT_INFO_LEN0);
            } else {
                handle_send_packect(key_down_click, sizeof(key_down_click), REPROT_INFO_LEN1);
            }
            break;

        case 2:
            if (REMOTE_IS_IOS()) {
                handle_send_packect(key_left_before, sizeof(key_left_before), REPROT_INFO_LEN0);
            } else {
                handle_send_packect(key_left_click, sizeof(key_left_click), REPROT_INFO_LEN1);
            }
            break;

        case 3:
            if (REMOTE_IS_IOS()) {
                handle_send_packect(key_right_before, sizeof(key_right_before), REPROT_INFO_LEN0);
            } else {
                handle_send_packect(key_right_click, sizeof(key_right_click), REPROT_INFO_LEN1);
            }
            break;

        case 4:
            if (REMOTE_IS_IOS()) {
                handle_send_packect(key_pp_release_before, sizeof(key_pp_release_before), REPROT_INFO_LEN0);
                bt_connect_reset_xy();
            } else {
                handle_send_packect(key_pp_press, sizeof(key_pp_press), REPROT_INFO_LEN1);
                handle_send_packect(key_pp_release, sizeof(key_pp_release), REPROT_INFO_LEN1);
            }
            break;

        case 5:
            if (REMOTE_IS_IOS()) {
                handle_send_packect(key_one_press_before, sizeof(key_one_press_before), REPROT_INFO_LEN0);
                handle_send_packect(key_one_release_before, sizeof(key_one_release_before), REPROT_INFO_LEN0);
            } else {
                handle_send_packect(key_one_press, sizeof(key_one_press), REPROT_INFO_LEN1);
                handle_send_packect(key_one_release, sizeof(key_one_release), REPROT_INFO_LEN1);
            }
            break;

        default:
            break;
        }
    }
    if (key_type == KEY_EVENT_DOUBLE_CLICK) {
        switch (key_value) {
        case 0:
            key_vol_ctrl(1);
            break;
        case 1:
            key_vol_ctrl(0);
            break;
        case 4: {
            u8 count = 2;
            while (count--) {
                if (REMOTE_IS_IOS()) {
                    handle_send_packect(key_pp_release_before, sizeof(key_pp_release_before), REPROT_INFO_LEN0);
                } else {
                    handle_send_packect(key_pp_press, sizeof(key_pp_press), REPROT_INFO_LEN1);
                    handle_send_packect(key_pp_release, sizeof(key_pp_release), REPROT_INFO_LEN1);
                }
                DOUBLE_KEY_DELAY_TIME();
            }
        }
        break;

        case 5: {
            u8 count = 2;
            while (count--) {
                if (REMOTE_IS_IOS()) {
                    handle_send_packect(key_one_press_before, sizeof(key_one_press_before), REPROT_INFO_LEN0);
                    handle_send_packect(key_one_release_before, sizeof(key_one_release_before), REPROT_INFO_LEN0);
                }
                handle_send_packect(key_one_press, sizeof(key_one_press), REPROT_INFO_LEN1);
                handle_send_packect(key_one_release, sizeof(key_one_release), REPROT_INFO_LEN1);
                DOUBLE_KEY_DELAY_TIME();
            }
        }
        break;
        default:
            break;
        }
    }


    if (key_type == KEY_EVENT_LONG) {
        switch (key_value) {
        case 0:
            key_vol_ctrl(1);
            break;
            //for test
#if (TCFG_USER_EDR_ENABLE && TCFG_USER_BLE_ENABLE)
            //switch ble & edr
            /* if (edr_hid_is_connected() || ble_hid_is_connected()) { */
            /* log_info("switch mode fail,disconnect firstly!!!"); */
            /* return; */
            /* } */
            is_keypage_active = 1;
            if (HID_MODE_BLE == bt_hid_mode) {
                keypage_app_select_btmode(HID_MODE_EDR);
            } else {
                keypage_app_select_btmode(HID_MODE_BLE);
            }
            os_time_dly(WAIT_DISCONN_TIME_MS / 10);
            p33_soft_reset();
            while (1);
#endif
            break;

        case 1:
            break;

        case 2:
            break;

        case 3:
            break;

        default:
            break;
        }
    }
    /* 通过按键三击来调整X Y的坐标达到拍照按键位置的调整，存储在VM，掉电后依旧保留之前的坐标   */
    if (key_type == KEY_EVENT_TRIPLE_CLICK) {
        switch (key_value) {
        case 0:
            y_lab += 128;
            if (y_lab >= 4100) {
                y_lab = 0;
            }
            keypage_coordinate_equal_ios(0, 2);
            keypage_coordinate_equal();
            break;
        case 1:
            y_lab -= 128;
            if (y_lab <= 0) {
                y_lab =  4100 ;
            }
            keypage_coordinate_equal_ios(0, 1);
            keypage_coordinate_equal();
            break;

        case 2:
            x_lab -= 128;
            if (x_lab <= 0) {
                x_lab = 4095;
            }
            keypage_coordinate_equal_ios(2, 0);
            keypage_coordinate_equal();
            break;

        case 3:
            x_lab += 128;
            if (x_lab > 4095) {
                x_lab = 0;
            }
            keypage_coordinate_equal_ios(1, 0);
            keypage_coordinate_equal();
            break;

        case 4:
            break;
        case 5:
            //send press
            if (REMOTE_IS_IOS()) {
                handle_send_packect(key_one_press_before, sizeof(key_one_press_before), REPROT_INFO_LEN0);
            } else {
                handle_send_packect(key_one_press, sizeof(key_one_press), REPROT_INFO_LEN1);
            }
            break;

        default:
            break;
        }
    }

    if (key_type == KEY_EVENT_HOLD) {
        switch (key_value) {
        case 0:
            handle_send_packect(key_up_hold_press, sizeof(key_up_hold_press), REPROT_INFO_LEN0);
            handle_send_packect(key_up_hold_release, sizeof(key_up_hold_release), REPROT_INFO_LEN0);
            break;

        case 1:
            handle_send_packect(key_down_hold_press, sizeof(key_down_hold_press), REPROT_INFO_LEN0);
            handle_send_packect(key_down_hold_release, sizeof(key_down_hold_release), REPROT_INFO_LEN0);
            break;

        case 2:
            break;

        case 3:
            break;

        case 4:
            break;

        case 5:
            break;

        default:
            break;
        }
    }

    if (key_type == KEY_EVENT_UP) {
        switch (key_value) {
        case 0:
            break;

        case 1:
            break;

        case 2:
            break;

        case 3:
            break;

        case 4:
            //send release
            if (REMOTE_IS_IOS()) {
                handle_send_packect(key_pp_release_before, sizeof(key_pp_release_before), REPROT_INFO_LEN0);
            } else {
                handle_send_packect(key_pp_release, sizeof(key_pp_release), REPROT_INFO_LEN1);
            }
            break;

        case 5:
            //send release
            if (REMOTE_IS_IOS()) {
                handle_send_packect(key_one_release_before, sizeof(key_one_release_before), REPROT_INFO_LEN0);
            } else {
                handle_send_packect(key_one_release, sizeof(key_one_release), REPROT_INFO_LEN1);
            }
            break;

        default:
            break;
        }
    }
    keypage_coordinate_vm_deal(1);
}
//----------------------------------

typedef struct {
    u16 head_tag;
    u8  mode;

} hid_vm_cfg_t;

#define	HID_VM_HEAD_TAG (0x3AA3)
static void keypage_vm_deal(u8 rw_flag)
{
    hid_vm_cfg_t info;
    int ret;
    int vm_len = sizeof(hid_vm_cfg_t);

    log_info("hid_info_vm_do:%d\n", rw_flag);
    memset(&info, 0, vm_len);

    if (rw_flag == 0) {
        bt_hid_mode = HID_MODE_NULL;
        ret = syscfg_read(CFG_AAP_MODE_INFO, (u8 *)&info, vm_len);
        if (!ret) {
            log_info("-null--\n");
        } else {
            if (HID_VM_HEAD_TAG == info.head_tag) {
                log_info("-exist--\n");
                log_info_hexdump((u8 *)&info, vm_len);
                bt_hid_mode = info.mode;
            }
        }

        if (HID_MODE_NULL == bt_hid_mode) {
#if TCFG_USER_BLE_ENABLE
            bt_hid_mode = HID_MODE_BLE;
#else
            bt_hid_mode = HID_MODE_EDR;
#endif
        } else {
            if (!TCFG_USER_BLE_ENABLE) {
                bt_hid_mode = HID_MODE_EDR;
            }

            if (!TCFG_USER_EDR_ENABLE) {
                bt_hid_mode = HID_MODE_BLE;
            }

            if (bt_hid_mode != info.mode) {
                log_info("-write00--\n");
                info.mode = bt_hid_mode;
                syscfg_write(CFG_AAP_MODE_INFO, (u8 *)&info, vm_len);
            }
        }
    } else {
        info.mode = bt_hid_mode;
        info.head_tag = HID_VM_HEAD_TAG;

        syscfg_write(CFG_AAP_MODE_INFO, (u8 *)&info, vm_len);
        log_info("-write11--\n");
        log_info_hexdump((u8 *)&info, vm_len);
    }
}

#define COORDINATE_VM_HEAD_TAG   (0x4000)
typedef struct {
    u8   head_tag;
    u16  x_vm_lab;
    u16  y_vm_lab;
} hid_vm_lab;

void keypage_coordinate_vm_deal(u8 flag)
{
    hid_vm_lab lab;
    int ret;
    int len = sizeof(hid_vm_lab);
    memset(&lab, 0, sizeof(hid_vm_lab));
    if (flag == 0) {
        ret = syscfg_read(CFG_COORDINATE_ADDR, (u8 *)&lab, sizeof(lab));
        if (ret <= 0) {
            log_info("init null \n");
            x_lab =  0x0770;
            y_lab =  0x0770;
        } else {

            x_lab = lab.x_vm_lab;
            y_lab = lab.y_vm_lab;
        }
    } else {
        lab.x_vm_lab = x_lab;
        lab.y_vm_lab = y_lab;
        ret = syscfg_write(CFG_COORDINATE_ADDR, (u8 *)&lab, sizeof(lab));
    }
}

void keypage_power_event_to_user(u8 event)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = (void *)DEVICE_EVENT_FROM_POWER;
    e.u.dev.event = event;
    e.u.dev.value = 0;
    sys_event_notify(&e);
}

static void keypage_set_soft_poweroff(void)
{
    log_info("keypage_set_soft_poweroff\n");
    is_keypage_active = 1;

#if TCFG_USER_BLE_ENABLE
    btstack_ble_exit(0);
#endif

#if TCFG_USER_EDR_ENABLE
    btstack_edr_exit(0);
#endif

#if (TCFG_USER_EDR_ENABLE || TCFG_USER_BLE_ENABLE)
    //延时300ms，确保BT退出链路断开
    sys_timeout_add(NULL, power_set_soft_poweroff, WAIT_DISCONN_TIME_MS);
#else
    power_set_soft_poweroff();
#endif
}

extern void bt_pll_para(u32 osc, u32 sys, u8 low_power, u8 xosc);
static void keypage_app_start()
{
    log_info("=======================================");
    log_info("-------------keypage demo--------------");
    log_info("=======================================");
//    bt_osc_offset_ext_save(20);//频偏多少填多少,-10K填-10

    clk_set("sys", BT_NORMAL_HZ);

    //有蓝牙
#if (TCFG_USER_EDR_ENABLE || TCFG_USER_BLE_ENABLE)
    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);

#if TCFG_USER_EDR_ENABLE
    btstack_edr_start_before_init(&keypage_edr_config, 0);
#endif

#if TCFG_USER_BLE_ENABLE
    btstack_ble_start_before_init(&keypage_ble_config, 0);
#endif

    btstack_init();

#else
    //no bt,to for test
    log_info("not_bt!!!!!!");
#endif

    keypage_coordinate_vm_deal(0);
    /* 按键消息使能 */
    sys_key_event_enable();

#if (TCFG_HID_AUTO_SHUTDOWN_TIME)
    //无操作定时软关机
    g_auto_shutdown_timer = sys_timeout_add((void *)POWER_EVENT_POWER_SOFTOFF, keypage_power_event_to_user, TCFG_HID_AUTO_SHUTDOWN_TIME * 1000);
#endif
}

static int keypage_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        if (!it) {
            break;
        }
        switch (it->action) {
        case ACTION_KEYPAGE:
            keypage_app_start();
            break;
        }
        break;
    case APP_STA_PAUSE:
        break;
    case APP_STA_RESUME:
        break;
    case APP_STA_STOP:
        break;
    case APP_STA_DESTROY:
        log_info("APP_STA_DESTROY\n");
        break;
    }

    return 0;
}

static int keypage_bt_hci_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_info("----%s reason %x %x", __FUNCTION__, bt->event, bt->value);

#if TCFG_USER_EDR_ENABLE
    bt_comm_edr_hci_event_handler(bt);
#endif

#if TCFG_USER_BLE_ENABLE
    bt_comm_ble_hci_event_handler(bt);
#endif

    return 0;
}

static int keypage_bt_connction_status_event_handler(struct bt_event *bt)
{
    log_info("----%s %d", __FUNCTION__, bt->event);

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        /*
         * 蓝牙初始化完成
         */
        log_info("BT_STATUS_INIT_OK\n");

        keypage_vm_deal(0);//bt_hid_mode read for VM

        //根据模式执行对应蓝牙的初始化
        if (bt_hid_mode == HID_MODE_BLE) {
#if TCFG_USER_BLE_ENABLE
            btstack_ble_start_after_init(0);
#endif
        } else {
#if TCFG_USER_EDR_ENABLE
            btstack_edr_start_after_init(0);
#endif
        }

        keypage_app_select_btmode(HID_MODE_INIT);//
        break;

    default:
#if TCFG_USER_EDR_ENABLE
        bt_comm_edr_status_event_handler(bt);
        if (bt->event == BT_STATUS_SECOND_CONNECTED || bt->event == BT_STATUS_FIRST_CONNECTED) {
            sys_timeout_add(NULL, bt_connect_reset_xy, 200);//复位触点
        }
#endif

#if TCFG_USER_BLE_ENABLE
        bt_comm_ble_status_event_handler(bt);
#endif

        break;
    }
    return 0;
}

static void keypage_key_event_handler(struct sys_event *event)
{
    /* u16 cpi = 0; */
    u8 event_type = 0;
    u8 key_value = 0;

    if (event->arg == (void *)DEVICE_EVENT_FROM_KEY) {
        event_type = event->u.key.event;
        key_value = event->u.key.value;
        keypage_app_key_deal_test(event_type, key_value);
    }
}

static int keypage_common_event_handler(struct bt_event *bt)
{
    log_info("----%s reason %x %x", __FUNCTION__, bt->event, bt->value);

    switch (bt->event) {
    case COMMON_EVENT_EDR_REMOTE_TYPE:
        log_info(" COMMON_EVENT_EDR_REMOTE_TYPE,%d \n", bt->value);
#if TCFG_USER_EDR_ENABLE
        connect_remote_type = bt->value;
        if (connect_remote_type == REMOTE_DEV_IOS) {
            user_hid_set_ReportMap(keypage_report_map_ios, sizeof(keypage_report_map_ios));
        } else {
            user_hid_set_ReportMap(keypage_report_map, sizeof(keypage_report_map));
        }
#endif
        break;

    case COMMON_EVENT_BLE_REMOTE_TYPE:
        log_info(" COMMON_EVENT_BLE_REMOTE_TYPE,%d \n", bt->value);
#if TCFG_USER_BLE_ENABLE
        connect_remote_type = bt->value;
        if (connect_remote_type == REMOTE_DEV_IOS) {
            le_hogp_set_ReportMap(keypage_report_map_ios, sizeof(keypage_report_map_ios));
        } else {
            le_hogp_set_ReportMap(keypage_report_map, sizeof(keypage_report_map));
        }
#endif
        break;

    case COMMON_EVENT_SHUTDOWN_DISABLE:
        keypage_auto_shutdown_disable();
        break;

    default:
        break;

    }
    return 0;
}

static void keypage_ble_status_callback(ble_state_e status, u8 reason)
{
    log_info("%s[status:0x%x reason:0x%x]", __func__, status, reason);
    switch (status) {
    case BLE_ST_IDLE:
        break;
    case BLE_ST_ADV:
        break;
    case BLE_ST_CONNECT:
        break;
    case BLE_ST_SEND_DISCONN:
        break;
    case BLE_ST_DISCONN:
        break;
    case BLE_ST_NOTIFY_IDICATE:
        break;
    case BLE_PRIV_MSG_PAIR_CONFIRM:
        break;
    case BLE_PRIV_PAIR_ENCRYPTION_CHANGE:
        sys_timeout_add(NULL, bt_connect_reset_xy, 2000);//复位触点
        break;
    default:
        break;
    }
}

static int keypage_event_handler(struct application *app, struct sys_event *event)
{
#if (TCFG_HID_AUTO_SHUTDOWN_TIME)
    //重置无操作定时计数
    if (event->type != SYS_DEVICE_EVENT || DEVICE_EVENT_FROM_POWER != event->arg) { //过滤电源消息
        sys_timer_modify(g_auto_shutdown_timer, TCFG_HID_AUTO_SHUTDOWN_TIME * 1000);
    }
#endif

#if TCFG_USER_EDR_ENABLE
    bt_comm_edr_sniff_clean();
#endif

    switch (event->type) {
    case SYS_KEY_EVENT:
        keypage_key_event_handler(event);
        return 0;

    case SYS_BT_EVENT:
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            keypage_bt_connction_status_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
            keypage_bt_hci_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_BLE_STATUS) {
            keypage_ble_status_callback(event->u.bt.event, event->u.bt.value);
        } else if ((u32)event->arg == SYS_BT_EVENT_FORM_COMMON) {
            return keypage_common_event_handler(&event->u.dev);
        }
        return 0;

    case SYS_DEVICE_EVENT:
        if ((u32)event->arg == DEVICE_EVENT_FROM_POWER) {
            return app_power_event_handler(&event->u.dev, keypage_set_soft_poweroff);
        }
#if TCFG_CHARGE_ENABLE
        else if ((u32)event->arg == DEVICE_EVENT_FROM_CHARGE) {
            app_charge_event_handler(&event->u.dev);
        }
#endif
        return 0;

    default:
        return 0;
    }

    return 0;
}



extern void ble_module_enable(u8 en);
static void keypage_app_select_btmode(u8 mode)
{
    if (mode != HID_MODE_INIT) {
        if (bt_hid_mode == mode) {
            return;
        }
        bt_hid_mode = mode;
    } else {
        //init start
    }

    log_info("###### %s: %d,%d\n", __FUNCTION__, mode, bt_hid_mode);

    if (bt_hid_mode == HID_MODE_BLE) {
        //ble
        log_info("---------app select ble--------\n");
        if (!STACK_MODULES_IS_SUPPORT(BT_BTSTACK_LE) || !BT_MODULES_IS_SUPPORT(BT_MODULE_LE)) {
            log_info("not surpport ble,make sure config !!!\n");
            ASSERT(0);
        }

#if TCFG_USER_EDR_ENABLE
        //close edr
        bt_comm_edr_mode_enable(0);
#endif

#if TCFG_USER_BLE_ENABLE
        if (mode == HID_MODE_INIT) {
            ble_module_enable(1);
        }
#endif
    } else {
        //edr
        log_info("---------app select edr--------\n");
        if (!STACK_MODULES_IS_SUPPORT(BT_BTSTACK_CLASSIC) || !BT_MODULES_IS_SUPPORT(BT_MODULE_CLASSIC)) {
            log_info("not surpport edr,make sure config !!!\n");
            ASSERT(0);
        }

#if TCFG_USER_BLE_ENABLE
        //close ble
        ble_module_enable(0);
#endif

#if TCFG_USER_EDR_ENABLE
        if (mode == HID_MODE_INIT) {
            if (!bt_connect_phone_back_start()) {
                bt_wait_phone_connect_control(1);
            }
        }
#endif

    }

    keypage_vm_deal(1);
}


//-----------------------
//system check go sleep is ok
static u8 keypage_idle_query(void)
{
    return !is_keypage_active;
}

REGISTER_LP_TARGET(app_keypage_lp_target) = {
    .name = "app_keypage",
    .is_idle = keypage_idle_query,
};


static const struct application_operation app_keypage_ops = {
    .state_machine  = keypage_state_machine,
    .event_handler  = keypage_event_handler,
};

/*
 * 注册模式
 */
REGISTER_APPLICATION(app_keypage) = {
    .name   = "keypage",
    .action	= ACTION_KEYPAGE,
    .ops    = &app_keypage_ops,
    .state  = APP_STA_DESTROY,
};

#endif


