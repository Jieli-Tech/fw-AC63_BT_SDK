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
#include "rcsp_bluetooth.h"
#include "rcsp_user_update.h"
#include "app_charge.h"
#include "app_power_manage.h"
#include "le_client_demo.h"
#include "usb/device/hid.h"
#include "usb/device/cdc.h"
#include "app_comm_bt.h"

#define LOG_TAG_CONST       DONGLE
#define LOG_TAG             "[DONGLE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if CONFIG_APP_DONGLE

#if 0//TCFG_USER_EDR_ENABLE && TCFG_USER_BLE_ENABLE
//不支持同时打开
#error " not support double bt !!!!!!"
#endif

#if USER_SUPPORT_PROFILE_HID && USER_SUPPORT_PROFILE_SPP
//不支持同时打开
#error " not support double profile!!!!!!"
#endif

//2.4G模式: 0---ble, 非0---2.4G配对码
#define CFG_RF_24G_CODE_ID       (0) //32bits
/* #define CFG_RF_24G_CODE_ID  (0x5555AAAA) */

static u8  is_app_dongle_active = 0;

/*测试两个usb设备上行 send*/
#define CONFIG_HIDKEY_REPORT_TEST    0//(BIT(0)|BIT(1))/*for test usb channel:bit0~ch1,bit1-ch2*/

//配置选择上报PC的描述符
//---------------------------------------------------------------------
//==========hid_key
#define HIDKEY_REPORT_ID               0x1

static const u8 sHIDReportDesc_hidkey[] = {
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, HIDKEY_REPORT_ID,  //   Report ID (1)
    0x09, 0xE9,        //   Usage (Volume Increment)
    0x09, 0xEA,        //   Usage (Volume Decrement)
    0x09, 0xCD,        //   Usage (Play/Pause)
    0x09, 0xE2,        //   Usage (Mute)
    0x09, 0xB6,        //   Usage (Scan Previous Track)
    0x09, 0xB5,        //   Usage (Scan Next Track)
    0x09, 0xB3,        //   Usage (Fast Forward)
    0x09, 0xB4,        //   Usage (Rewind)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x10,        //   Report Count (16)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
    // 35 bytes
};

// consumer key
#define CONSUMER_VOLUME_INC             0x0001
#define CONSUMER_VOLUME_DEC             0x0002
#define CONSUMER_PLAY_PAUSE             0x0004
#define CONSUMER_MUTE                   0x0008
#define CONSUMER_SCAN_PREV_TRACK        0x0010
#define CONSUMER_SCAN_NEXT_TRACK        0x0020
#define CONSUMER_SCAN_FRAME_FORWARD     0x0040
#define CONSUMER_SCAN_FRAME_BACK        0x0080




//==========键盘 1
#define KEYBOARD_REPORT_ID          0x1
#define COUSTOM_CONTROL_REPORT_ID   0x2
#define MOUSE_POINT_REPORT_ID       0x3

static const u8 sHIDReportDesc_keyboard1[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, KEYBOARD_REPORT_ID,//   Report ID (1)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0,        //   Usage Minimum (0xE0)
    0x29, 0xE7,        //   Usage Maximum (0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x03,        //   Report Count (3)
    0x75, 0x01,        //   Report Size (1)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x03,        //   Usage Maximum (Scroll Lock)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x91, 0x01,        //   Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, COUSTOM_CONTROL_REPORT_ID,//   Report ID (3)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x01,        //   Report Count (1)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0x8C, 0x02,  //   Logical Maximum (652)
    0x19, 0x00,        //   Usage Minimum (Unassigned)
    0x2A, 0x8C, 0x02,  //   Usage Maximum (AC Send)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
//
    // Dummy mouse collection starts here
    //
    0x05, 0x01,                         // USAGE_PAGE (Generic Desktop)     0
    0x09, 0x02,                         // USAGE (Mouse)                    2
    0xa1, 0x01,                         // COLLECTION (Application)         4
    0x85, MOUSE_POINT_REPORT_ID,               //   REPORT_ID (Mouse)              6
    0x09, 0x01,                         //   USAGE (Pointer)                8
    0xa1, 0x00,                         //   COLLECTION (Physical)          10
    0x05, 0x09,                         //     USAGE_PAGE (Button)          12
    0x19, 0x01,                         //     USAGE_MINIMUM (Button 1)     14
    0x29, 0x02,                         //     USAGE_MAXIMUM (Button 2)     16
    0x15, 0x00,                         //     LOGICAL_MINIMUM (0)          18
    0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)          20
    0x75, 0x01,                         //     REPORT_SIZE (1)              22
    0x95, 0x02,                         //     REPORT_COUNT (2)             24
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)         26
    0x95, 0x06,                         //     REPORT_COUNT (6)             28
    0x81, 0x03,                         //     INPUT (Cnst,Var,Abs)         30
    0x05, 0x01,                         //     USAGE_PAGE (Generic Desktop) 32
    0x09, 0x30,                         //     USAGE (X)                    34
    0x09, 0x31,                         //     USAGE (Y)                    36
    0x15, 0x81,                         //     LOGICAL_MINIMUM (-127)       38
    0x25, 0x7f,                         //     LOGICAL_MAXIMUM (127)        40
    0x75, 0x08,                         //     REPORT_SIZE (8)              42
    0x95, 0x02,                         //     REPORT_COUNT (2)             44
    0x81, 0x06,                         //     INPUT (Data,Var,Rel)         46
    0xc0,                               //   END_COLLECTION                 48
    0xc0                                // END_COLLECTION                   49/50
};


//==========键盘 2
static const u8 sHIDReportDesc_stand_keyboard2[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x04,//   Report ID (1)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0,        //   Usage Minimum (0xE0)
    0x29, 0xE7,        //   Usage Maximum (0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x03,        //   Report Count (3)
    0x75, 0x01,        //   Report Size (1)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x03,        //   Usage Maximum (Scroll Lock)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x91, 0x01,        //   Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x05,//   Report ID (3)
    0x75, 0x10,        //   Report Size (16)
    0x95, 0x01,        //   Report Count (1)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0x8C, 0x02,  //   Logical Maximum (652)
    0x19, 0x00,        //   Usage Minimum (Unassigned)
    0x2A, 0x8C, 0x02,  //   Usage Maximum (AC Send)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
//
    // Dummy mouse collection starts here
    //
    0x05, 0x01,                         // USAGE_PAGE (Generic Desktop)     0
    0x09, 0x02,                         // USAGE (Mouse)                    2
    0xa1, 0x01,                         // COLLECTION (Application)         4
    0x85, 0x06,               //   REPORT_ID (Mouse)              6
    0x09, 0x01,                         //   USAGE (Pointer)                8
    0xa1, 0x00,                         //   COLLECTION (Physical)          10
    0x05, 0x09,                         //     USAGE_PAGE (Button)          12
    0x19, 0x01,                         //     USAGE_MINIMUM (Button 1)     14
    0x29, 0x02,                         //     USAGE_MAXIMUM (Button 2)     16
    0x15, 0x00,                         //     LOGICAL_MINIMUM (0)          18
    0x25, 0x01,                         //     LOGICAL_MAXIMUM (1)          20
    0x75, 0x01,                         //     REPORT_SIZE (1)              22
    0x95, 0x02,                         //     REPORT_COUNT (2)             24
    0x81, 0x02,                         //     INPUT (Data,Var,Abs)         26
    0x95, 0x06,                         //     REPORT_COUNT (6)             28
    0x81, 0x03,                         //     INPUT (Cnst,Var,Abs)         30
    0x05, 0x01,                         //     USAGE_PAGE (Generic Desktop) 32
    0x09, 0x30,                         //     USAGE (X)                    34
    0x09, 0x31,                         //     USAGE (Y)                    36
    0x15, 0x81,                         //     LOGICAL_MINIMUM (-127)       38
    0x25, 0x7f,                         //     LOGICAL_MAXIMUM (127)        40
    0x75, 0x08,                         //     REPORT_SIZE (8)              42
    0x95, 0x02,                         //     REPORT_COUNT (2)             44
    0x81, 0x06,                         //     INPUT (Data,Var,Rel)         46
    0xc0,                               //   END_COLLECTION                 48
    0xc0                                // END_COLLECTION                   49/50
};

//==========鼠标
static const u8 sHIDReportDesc_mouse[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
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
    0x85, 0x02,        //   Report ID (2)
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
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x03,        //   Report ID (3)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x01,        //   Report Count (1)
    0x09, 0xCD,        //   Usage (Play/Pause)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x0A, 0x83, 0x01,  //   Usage (AL Consumer Control Configuration)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xB5,        //   Usage (Scan Next Track)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xB6,        //   Usage (Scan Previous Track)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xEA,        //   Usage (Volume Decrement)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xE9,        //   Usage (Volume Increment)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x0A, 0x25, 0x02,  //   Usage (AC Forward)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x0A, 0x24, 0x02,  //   Usage (AC Back)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x05,        //   Usage (Headphone)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x02,        //   Report Count (2)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
// 149 bytes
};


//---------------------------------------------------------------------
static void dongle_edr_hid_input_handler(u8 *packet, u16 size, u16 channel);
extern void dongle_custom_hid_rx_handler(void *priv, u8 *buf, u32 len);
extern int ble_hid_data_send_ext(u8 report_type, u8 report_id, u8 *data, u16 len);
//---------------------------------------------------------------------
void dongle_power_event_to_user(u8 event)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = (void *)DEVICE_EVENT_FROM_POWER;
    e.u.dev.event = event;
    e.u.dev.value = 0;
    sys_event_notify(&e);
}

static void dongle_set_soft_poweroff(void)
{
    log_info("set_soft_poweroff\n");
    is_app_dongle_active = 1;
    //必须先主动断开蓝牙链路,否则要等链路超时断开

#if TCFG_USER_BLE_ENABLE
#if (USER_SUPPORT_PROFILE_HID==1)
    user_hid_exit();
#endif
    btstack_ble_exit(0);
#endif

#if TCFG_USER_EDR_ENABLE
    btstack_edr_exit(0);
#endif

    //延时300ms，确保BT退出链路断开
    sys_timeout_add(NULL, power_set_soft_poweroff, WAIT_DISCONN_TIME_MS);
}

static const char *edr_bd_name_filt[] = {
    /* "AC632N_TEST", */
    /* "BR30_HID", */
    /* "Bluetooth Keyboard 3.0 A2", */
    "BlueTooth_Keyboard  3.0",
    "AC630N_mx",
    "br30_mx",
};


static void dongle_timer_handle_test(void)
{
    log_info("not_bt");
}

// cdc send test
static void usb_cdc_send_test()
{
#if TCFG_USB_SLAVE_CDC_ENABLE
    log_info("-send test cdc data-");
    u8 cdc_test_buf[3] = {0x11, 0x22, 0x33};
    cdc_write_data(USB0, cdc_test_buf, 3);
    /* char test_char[] = "cdc test"; */
    /* cdc_write_data(USB0, test_char, sizeof(test_char)-1); */
#endif
}

static const u8 fix_target_address[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
void bt_set_mac_addr(u8 *addr);
static void dongle_app_start()
{
    log_info("=======================================");
    log_info("---------usb + dongle demo---------");
    log_info("=======================================");

    log_info("app_file: %s", __FILE__);

    clk_set("sys", BT_NORMAL_HZ);

//有蓝牙
#if (TCFG_USER_EDR_ENABLE || TCFG_USER_BLE_ENABLE)
    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);

#if TCFG_USER_EDR_ENABLE
    btstack_edr_start_before_init(NULL, 0);
    __change_hci_class_type(0);//
//    bt_set_mac_addr(fix_target_address);//for test
#if EDR_EMITTER_EN
    bt_emitter_set_match_name(&edr_bd_name_filt, sizeof(edr_bd_name_filt) / (sizeof(edr_bd_name_filt[0])));
#endif

#if (USER_SUPPORT_PROFILE_HID==1)
    user_hid_init(dongle_edr_hid_input_handler);
#endif

#endif

#if TCFG_USER_BLE_ENABLE
    btstack_ble_start_before_init(NULL, 0);
    rf_set_24g_hackable_coded(CFG_RF_24G_CODE_ID);
#endif

    btstack_init();

#endif

    /* 按键消息使能 */
    sys_key_event_enable();

#if (TCFG_PC_ENABLE)
    extern void usb_start();
    extern void usb_hid_set_repport_map(const u8 * map, int size);
    extern void usb_hid_set_second_repport_map(const u8 * map, int size);
    extern void dongle_return_online_list(void);

    //配置选择上报PC的描述符
    //first device
    log_info("register channel 1");
#if CONFIG_HIDKEY_REPORT_TEST & BIT(0)
    usb_hid_set_repport_map(sHIDReportDesc_hidkey, sizeof(sHIDReportDesc_hidkey));
#else
    usb_hid_set_repport_map(sHIDReportDesc_mouse, sizeof(sHIDReportDesc_mouse));
#endif
    /* usb_hid_set_repport_map(sHIDReportDesc_keyboard1, sizeof(sHIDReportDesc_keyboard1)); */

#if (CONFIG_BT_GATT_CLIENT_NUM == 2)
    //second device
    log_info("register channel 2");
#if CONFIG_HIDKEY_REPORT_TEST & BIT(1)
    usb_hid_set_second_repport_map(sHIDReportDesc_hidkey, sizeof(sHIDReportDesc_hidkey));
#else
    usb_hid_set_second_repport_map(sHIDReportDesc_stand_keyboard2, sizeof(sHIDReportDesc_stand_keyboard2));
#endif
#endif

    usb_start();

#if RCSP_BTMATE_EN
    dongle_ota_init();
    custom_hid_set_rx_hook(NULL, dongle_custom_hid_rx_handler);//重注册接收回调到dongle端
    /* download_buf = malloc(1024); */
    /* dongle_return_online_list(); */
    sys_timeout_add(NULL, dongle_return_online_list, 4000);
#endif
#endif

}

static int dongle_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        if (!it) {
            break;
        }
        switch (it->action) {
        case ACTION_DONGLE_MAIN:
            dongle_app_start();
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

//edr 接收设备数据
static void dongle_edr_hid_input_handler(u8 *packet, u16 size, u16 channel)
{
    log_info("edr_hid_data_input:chl=%d,size=%d", channel, size);
    put_buf(packet, size);
    if (packet[0] == 0xA1) {
#if TCFG_PC_ENABLE
        putchar('@');
        if (hid_send_data(packet + 1, size - 1)) {
            putchar('f');
        }
#endif
    }
}

//ble 接收设备数据
int dongle_ble_hid_input_handler(u8 *packet, u16 size)
{
    /* log_info("ble_hid_data_input:size=%d", size); */
    /* put_buf(packet, size); */
#if TCFG_PC_ENABLE
    putchar('&');
    return hid_send_data(packet, size);
#else
    log_info("chl1 disable!!!\n");
    return 0;
#endif
}

//ble 接收第二个设备数据
int dongle_second_ble_hid_input_handler(u8 *packet, u16 size)
{
    /* log_info("ble_hid_data_input:size=%d", size); */
    /* put_buf(packet, size); */

    putchar('#');
#if TCFG_PC_ENABLE && (CONFIG_BT_GATT_CLIENT_NUM == 2)
    return hid_send_second_data(packet, size);
#else
    log_info("chl2 disable!!!\n");
    return 0;
#endif
}

void dongle_hid_output_handler(u8 report_type, u8 report_id, u8 *packet, u16 size)
{
    log_info("hid_data_output:type= %02x,report_id= %d,size=%d", report_type, report_id, size);
    put_buf(packet, size);

#if (TCFG_USER_EDR_ENABLE)
    edr_hid_data_send_ext(report_type, report_id, packet, size);
#elif (TCFG_USER_BLE_ENABLE)
    ble_hid_data_send_ext(report_type, report_id, packet, size);
#endif
}


static int dongle_bt_hci_event_handler(struct bt_event *bt)
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

static int dongle_bt_connction_status_event_handler(struct bt_event *bt)
{
    log_info("----%s %d", __FUNCTION__, bt->event);

#if TCFG_USER_EDR_ENABLE
    bt_comm_edr_status_event_handler(bt);
#endif

#if TCFG_USER_BLE_ENABLE
    bt_comm_ble_status_event_handler(bt);
#endif

    return 0;
}

static void dongle_key_event_handler(struct sys_event *event)
{
    u8 event_type = 0;
    u8 key_value = 0;

    if (event->arg == (void *)DEVICE_EVENT_FROM_KEY) {
        event_type = event->u.key.event;
        key_value = event->u.key.value;
        log_info("app_key_evnet: %d,%d\n", event_type, key_value);

        if (event_type == KEY_EVENT_TRIPLE_CLICK
            && (key_value == TCFG_ADKEY_VALUE3 || key_value == TCFG_ADKEY_VALUE0)) {
            dongle_power_event_to_user(POWER_EVENT_POWER_SOFTOFF);
            return;
        }

        if (event_type == KEY_EVENT_DOUBLE_CLICK && key_value == TCFG_ADKEY_VALUE0) {
#if TCFG_USB_SLAVE_CDC_ENABLE
            log_info(">>>test to cdc send\n");
            usb_cdc_send_test();
#endif
        }

#if (USER_SUPPORT_PROFILE_HID ==1)
        if (event_type == KEY_EVENT_LONG && key_value == TCFG_ADKEY_VALUE0) {
            log_info("hid disconnec test");
            user_hid_disconnect();
        }
#endif

#if TCFG_PC_ENABLE && CONFIG_HIDKEY_REPORT_TEST
        if (event_type == KEY_EVENT_CLICK && key_value == TCFG_ADKEY_VALUE0) {
            u8 packet_buf[3] = {HIDKEY_REPORT_ID, CONSUMER_PLAY_PAUSE & 0x0ff, CONSUMER_PLAY_PAUSE >> 8}; //pp key
            log_info("key_00");
#if CONFIG_HIDKEY_REPORT_TEST & BIT(0)
            log_info("usb channel1 test");
            hid_send_data(packet_buf, sizeof(packet_buf));
            os_time_dly(1);
            packet_buf[1] = 0;
            hid_send_data(packet_buf, sizeof(packet_buf));
#endif
        }

        if (event_type == KEY_EVENT_CLICK && key_value == TCFG_ADKEY_VALUE1) {
            u8 packet_buf[3] = {HIDKEY_REPORT_ID, CONSUMER_MUTE & 0x0ff, CONSUMER_MUTE >> 8}; //pp key
            log_info("key_01");
#if (CONFIG_HIDKEY_REPORT_TEST & BIT(1)) && (CONFIG_BT_GATT_CLIENT_NUM == 2)
            log_info("usb channel2 test");
            hid_send_second_data(packet_buf, sizeof(packet_buf));
            os_time_dly(1);
            packet_buf[1] = 0;
            hid_send_second_data(packet_buf, sizeof(packet_buf));
#endif
        }
#endif

    }
}

static int dongle_event_handler(struct application *app, struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        dongle_key_event_handler(event);
        return 0;

    case SYS_BT_EVENT:
#if (TCFG_USER_EDR_ENABLE || TCFG_USER_BLE_ENABLE)
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            dongle_bt_connction_status_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
            dongle_bt_hci_event_handler(&event->u.bt);
#if RCSP_BTMATE_EN
        } else if ((u32)event->arg == DEVICE_EVENT_FROM_PC) {
            dongle_pc_event_handler(&event->u.bt);//dongle pc命令回调处理
        } else if ((u32)event->arg == DEVICE_EVENT_FROM_OTG) {
            dongle_otg_event_handler(&event->u.bt);//dongle ota升级数据透传
#endif
        }
#endif
        return 0;

    case SYS_DEVICE_EVENT:
        if ((u32)event->arg == DEVICE_EVENT_FROM_POWER) {
            return app_power_event_handler(&event->u.dev, dongle_set_soft_poweroff);
        }

#if TCFG_CHARGE_ENABLE
        else if ((u32)event->arg == DEVICE_EVENT_FROM_CHARGE) {
            app_charge_event_handler(&event->u.dev);
        }
#endif
        return 0;

    default:
        return FALSE;
    }
    return FALSE;
}

static const struct application_operation app_dongle_ops = {
    .state_machine  = dongle_state_machine,
    .event_handler 	= dongle_event_handler,
};

/*
 * 注册AT Module模式
 */
REGISTER_APPLICATION(app_dongle) = {
    .name 	= "dongle",
    .action	= ACTION_DONGLE_MAIN,
    .ops 	= &app_dongle_ops,
    .state  = APP_STA_DESTROY,
};

//-----------------------
//system check go sleep is ok
static u8 dongle_state_idle_query(void)
{
    return !is_app_dongle_active;
}

REGISTER_LP_TARGET(dongle_state_lp_target) = {
    .name = "dongle_state_deal",
    .is_idle = dongle_state_idle_query,
};

#endif


