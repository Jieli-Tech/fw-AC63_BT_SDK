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
#include "hid_user.h"
/* #include "code_switch.h" */
/* #include "omsensor/OMSensor_manage.h" */
#include "le_common.h"
#include <stdlib.h>
#include "standard_hid.h"
#include "rcsp_bluetooth.h"
#include "user_cfg.h"
#include "usb/host/usb_hid_keys.h"
#include "matrix_keyboard.h"
#include "user_cfg.h"
#include "app_charge.h"
#include "app_power_manage.h"

#if(CONFIG_APP_STANDARD_KEYBOARD)

#define LOG_TAG_CONST       HID_KEY
#define LOG_TAG             "[HID_KEY]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define SUPPORT_KEYBOARD_NO_CONFLICT        0    //无冲按键支持
#define SUPPORT_USER_PASSKEY                0
#define CAP_LED_PIN                         -1
#define CAP_LED_ON_VALUE                    1

typedef enum {
    EDR_OPERATION_NULL = 0,
    EDR_OPERATION_RECONN,
    EDR_OPERATION_PAGESCAN_IRQUIRY_SCAN,
} edr_operation_t;

static u16 g_auto_shutdown_timer = 0;
static volatile u8 cur_bt_idx = 0;
static edr_operation_t edr_operation = EDR_OPERATION_NULL;
static u8 key_pass_enter  = 0;
static u8 remote_addr[6] = {0};

void hid_set_soft_poweroff(void);
static void sys_auto_sniff_controle(u8 enable, u8 *addr);
static void bt_sniff_ready_clean(void);
static void bt_wait_phone_connect_control(u8 enable);
static void edr_led_status_callback(u8 *buffer, u16 len);

extern void ble_module_enable(u8 en);
extern int ble_hid_data_send(u8 report_id, u8 *data, u16 len);
extern void edr_hid_data_send(u8 report_id, u8 *data, u16 len);
extern int edr_hid_is_connected(void);
extern int ble_hid_is_connected(void);
extern void delete_link_key(bd_addr_t *bd_addr, u8 id);
extern void le_hogp_set_output_callback(void *cb);
extern void modify_ble_name(const char *name);

#define WAIT_DISCONN_TIME_MS     (300)


#define KEYBOARD_ENTER_PAIR0    0x0
#define KEYBOARD_ENTER_PAIR1    0x1
#define KEYBOARD_ENTER_PAIR2    0x2
#define KEYBOARD_ENTER_PAIR3    0x3

typedef struct _special_key {
    u8 row;
    u8 col;
    u8 is_user_key;
} special_key;

#if 0
#define FN_ROW                   (2)
#define FN_COL                   (13)
const u16 matrix_key_table[ROW_MAX][COL_MAX] = {                //高八位用来标识是否为特殊键
//  0               1              2           3        4       5                   6          7        8              9                      10            11 12 13    14     15                            16
    {_KEY_Q,     _KEY_W,        _KEY_E,   _KEY_R,  _KEY_U,  _KEY_I,           _KEY_O,   _KEY_O,    _KEY_P,             0,                      0,           0, 0, 0,  _KEY_P,   0},      //0
    {_KEY_TAB,   _KEY_CAPSLOCK, _KEY_F3,  _KEY_T,  _KEY_Z,  _KEY_RIGHTBRACE,  _KEY_F7,  _KEY_F7,   _KEY_LEFTBRACE,     0,                  _KEY_BACKSPACE,  0, 0, 0,  _KEY_LEFTBRACE,   _KEY_BACKSPACE, S_KEY(_KEY_MOD_LSHIFT)},
    {_KEY_A,     _KEY_S,        _KEY_D,   _KEY_F,  _KEY_J,  _KEY_K,           _KEY_L,      0,      _KEY_SEMICOLON,  S_KEY(_KEY_MOD_LCTRL), _KEY_BACKSLASH,  0, 0, 0,    0,   _KEY_BACKSLASH,   S_KEY(_KEY_MOD_RSHIFT)},     //1
    {_KEY_ESC,      0,            0,      _KEY_G,  _KEY_H,  _KEY_F6,          _KEY_F6,  _KEY_UP,   _KEY_APOSTROPHE, _KEY_KPCOMMA,              0,          _KEY_SPACE, 0, S_KEY(_KEY_MOD_LALT), 0, 0, 0, _KEY_SPACE},
    {_KEY_Y,     _KEY_X,        _KEY_C,   _KEY_V,  _KEY_M,  _KEY_COMMA,       _KEY_DOT,    0,         0,               0,                      0,          _KEY_F11, S_KEY(_KEY_RIGHTCTRL), 0, 0,   _KEY_ENTER, 0, _KEY_F11},
    {0,             0,            0,      _KEY_B,  _KEY_N,  _KEY_N,              0,     _KEY_LEFT, _KEY_SLASH,      _KEY_RIGHT,                0,          _KEY_DOWN,  _KEY_RIGHT, S_KEY(_KEY_MOD_RALT),  _KEY_SLASH, 0, 0, _KEY_DOWN},
    {_KEY_GRAVE, _KEY_F1,      _KEY_F2,   _KEY_5,  _KEY_6,  _KEY_EQUAL,       _KEY_F8,  _KEY_F8,   _KEY_MINUS,         0,                  _KEY_F9,        _KEY_SCROLLLOCK, S_KEY(_KEY_MOD_LCTRL), 0, _KEY_MINUS, _KEY_F9},
    {_KEY_1,     _KEY_2,       _KEY_3,    _KEY_4,  _KEY_7,  _KEY_8,           _KEY_9,   _KEY_9,    _KEY_0,          _KEY_F12,              _KEY_F10,        0, _KEY_F5, 0,   _KEY_0, _KEY_F10},
};
special_key fn_remap_key[13 + 4] = {
    {.row = 3, .col = 0},       //ESC
    {.row = 6, .col = 1},       //F1
    {.row = 6, .col = 2},       //F2
    {.row = 1, .col = 2},       //F3
    {.row = 3, .col = 2},       //F4
    {.row = 7, .col = 12},       //F5
    {.row = 3, .col = 5},        //F6
    {.row = 1, .col = 6},       //F7
    {.row = 6, .col = 6},       //F8
    {.row = 6, .col = 15},      //F9
    {.row = 7, .col = 10},      //F10
    {.row = 4, .col = 17},      //F11
    {.row = 7, .col = 13},       //F12
    {.row = 7, .col = 0, .is_user_key = 1},       //1
    {.row = 7, .col = 1, .is_user_key = 1},       //2
    {.row = 7, .col = 2, .is_user_key = 1},       //3
    {.row = 7, .col = 3, .is_user_key = 1},       //4
};
const u16 fn_remap_event[13 + 4] = {_KEY_CUSTOM_CTRL_HOME, _KEY_BRIGHTNESS_REDUCTION, _KEY_BRIGHTNESS_INCREASE, _KEY_CUSTOM_CTRL_CALCULATOR, \
                                    _KEY_CUSTOM_SELECT_ALL, _KEY_CUSTOM_COPY, _KEY_CUSTOM_PASTE, _KEY_CUSTOM_CUT, \
                                    0, 0, 0, _KEY_CUSTOM_CTRL_VOL_DOWN, _KEY_CUSTOM_CTRL_VOL_UP, \
                                    KEYBOARD_ENTER_PAIR0, KEYBOARD_ENTER_PAIR1, KEYBOARD_ENTER_PAIR2, KEYBOARD_ENTER_PAIR3,
                                   };
#else
#define FN_ROW                   (6)
#define FN_COL                   (0)
const u16 matrix_key_table[ROW_MAX][COL_MAX] = {                //高八位用来标识是否为特殊键
//  0                        1         2               3         4       5         6                7        8                9                         10            11 12 13    14     15                            16
    {0,                     _KEY_Q,   _KEY_W,         _KEY_E,  _KEY_R,  _KEY_U,  _KEY_I,          _KEY_O,    _KEY_P,             0,                       0,          0, 0, 0,    0,   0},      //0
    {0,                     _KEY_TAB, _KEY_CAPSLOCK,  _KEY_F3, _KEY_T,  _KEY_Y,  _KEY_RIGHTBRACE, _KEY_F7,   _KEY_LEFTBRACE,     0,                 _KEY_BACKSPACE,   0, 0, 0,    0,   S_KEY(_KEY_MOD_LSHIFT), S_KEY(_KEY_MOD_LALT)},
    {0,                     _KEY_A,   _KEY_S,         _KEY_D,  _KEY_F,  _KEY_J,  _KEY_K,          _KEY_L,    _KEY_SEMICOLON,  S_KEY(_KEY_MOD_LCTRL), _KEY_BACKSLASH,  0, 0, 0,    0,   S_KEY(_KEY_MOD_RSHIFT)},     //1
    {0,                     _KEY_ESC,    0,           _KEY_F4, _KEY_G,  _KEY_H,  _KEY_F6,            0,      _KEY_APOSTROPHE, _KEY_KPCOMMA,              0,          _KEY_SPACE, 0, 0, _KEY_UP},
    {S_KEY(_KEY_MOD_RALT),  _KEY_Z,   _KEY_X,         _KEY_C,  _KEY_V,  _KEY_M,  _KEY_COMMA,      _KEY_DOT,   0,                 0,                 _KEY_ENTER,      _KEY_F11, _KEY_MINUS},
    {0,                       0,         0,             0,     _KEY_B,  _KEY_N,    0,                0,      _KEY_SLASH,     _KEY_KPCOMMA,               0,          _KEY_DOWN,  _KEY_RIGHT, 0,   _KEY_LEFT},
    {0,                     _KEY_GRAVE, _KEY_F1,       _KEY_F2, _KEY_5,  _KEY_6,  _KEY_EQUAL,      _KEY_F8,   _KEY_MINUS,         0,                _KEY_F9,        _KEY_SCROLLLOCK},
    {_KEY_F5,               _KEY_1,   _KEY_2,         _KEY_3,  _KEY_4,  _KEY_7,  _KEY_8,          _KEY_9,    _KEY_0,          _KEY_F12,             _KEY_F10},
};
special_key fn_remap_key[12 + 4] = {
    {.row = 6, .col = 2},       //F1
    {.row = 6, .col = 3},       //F2
    {.row = 1, .col = 3},       //F3
    {.row = 3, .col = 3},       //F4
    {.row = 7, .col = 0},       //F5
    {.row = 3, .col = 6},       //F6
    {.row = 1, .col = 7},       //F7
    {.row = 6, .col = 7},       //F8
    {.row = 6, .col = 10},      //F9
    {.row = 7, .col = 10},      //F10
    {.row = 4, .col = 11},      //F11
    {.row = 7, .col = 9},       //F12
    {.row = 7, .col = 1, .is_user_key = 1},       //1
    {.row = 7, .col = 2, .is_user_key = 1},       //2
    {.row = 7, .col = 3, .is_user_key = 1},       //3
    {.row = 7, .col = 4, .is_user_key = 1},       //4
};
const u16 fn_remap_event[12 + 4] = {_KEY_CUSTOM_CTRL_HOME, _KEY_CUSTOM_CTRL_EMAIL, _KEY_CUSTOM_CTRL_SEARCH, _KEY_CUSTOM_CTRL_CALCULATOR, \
                                    _KEY_CUSTOM_CTRL_MUSIC, _KEY_CUSTOM_CTRL_BACK, _KEY_CUSTOM_CTRL_STOP, _KEY_CUSTOM_CTRL_FORWARD, \
                                    _KEY_CUSTOM_CTRL_MUTE, _KEY_CUSTOM_CTRL_VOL_DOWN, _KEY_CUSTOM_CTRL_VOL_UP, 0, \
                                    KEYBOARD_ENTER_PAIR0, KEYBOARD_ENTER_PAIR1, KEYBOARD_ENTER_PAIR2, KEYBOARD_ENTER_PAIR3,
                                   };
#endif


special_key other_key[] = {
    {.row = 6, .col = 13, .is_user_key = 1},
};


const u16 other_key_map[] = {KEYBOARD_ENTER_PAIR0};
u16 key_status_array[6] = {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff};
u16 remap_event = 0;

static int user_key_timer_hdl = 0;

typedef enum {
    HID_MODE_NULL = 0,
    HID_MODE_EDR = BIT(0),
    HID_MODE_BLE = BIT(1),
    HID_MODE_COMBO = (BIT(0) | BIT(1)),
    HID_MODE_INIT = 0xff
} bt_mode_e;
static bt_mode_e bt_hid_mode = HID_MODE_NULL;

static volatile u8 is_hid_active = 0;//1-临界点,系统不允许进入低功耗，0-系统可以进入低功耗

#define KEYBOARD_REPORT_ID          0x1
#define COUSTOM_CONTROL_REPORT_ID   0x2
#define MOUSE_POINT_REPORT_ID       0x3


const static u8  hid_report_map[] = {
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

void edr_set_enable(u8 en)
{
    printf("edr enable :%d\n", en);
    if (en) {
        user_hid_enable(1);
        btctrler_task_init_bredr();
        bt_wait_phone_connect_control(1);
        sys_auto_sniff_controle(1, NULL);
    } else {
        user_hid_enable(0);
        bt_wait_phone_connect_control(0);
        sys_auto_sniff_controle(0, NULL);
        btctrler_task_close_bredr();
    }
}

void ble_reset_addr(u8 *new_addr)
{
#if TCFG_USER_BLE_ENABLE
    ble_module_enable(0);
    printf("new ble address:");
    put_buf(new_addr, 6);
    le_controller_set_mac((void *)new_addr);
    ble_module_enable(1);
#endif
}

void user_key_deal(u16 user_key, u8 timeout)
{
    int ret = 0;
    u8 tmp_ble_addr[6];
    u8 zero_addr[6] = {0};
    u8 vm_hid_mode = HID_MODE_NULL;
    u8 user_event = user_key & 0xff;

    switch (user_event) {
    case KEYBOARD_ENTER_PAIR0:
    case KEYBOARD_ENTER_PAIR1:
    case KEYBOARD_ENTER_PAIR2:
    case KEYBOARD_ENTER_PAIR3:
        printf("user_event:0x%x\n", user_event);
        syscfg_read(CFG_CUR_BT_IDX, &cur_bt_idx, 1);
        if ((cur_bt_idx == user_event) && !timeout) {
#if TCFG_USER_EDR_ENABLE
            if (edr_hid_is_connected()) {                   //短按且HID连接不响应按键
                return;
            }
#endif

#if TCFG_USER_BLE_ENABLE
            if (ble_hid_is_connected()) {
                return;
            }
#endif
        }

        printf(">>>bt_hid_mode:%d\n", bt_hid_mode);
        switch (bt_hid_mode) {                                                  //根据连接模式断开当前连接
#if TCFG_USER_EDR_ENABLE
        case HID_MODE_EDR:
            if (edr_hid_is_connected()) {
                user_send_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL);
            }
            break;
#endif
#if TCFG_USER_BLE_ENABLE
        case HID_MODE_BLE:
            if (ble_hid_is_connected()) {
                ble_module_enable(0);
            }
            break;
#endif
        case HID_MODE_NULL:
            break;
        }

        cur_bt_idx = user_event;
        syscfg_write(CFG_CUR_BT_IDX, &cur_bt_idx, 1);                           //记录当前使用哪组蓝牙

        if (timeout) {                                                          //如果是长按超时直接打开3.0 和 4.0 并清除VM信息
            g_printf("timeout\n");
            vm_hid_mode = HID_MODE_COMBO;
            syscfg_write(CFG_HID_MODE_BEGIN + cur_bt_idx, &vm_hid_mode, 1);
#if TCFG_USER_EDR_ENABLE
            if (syscfg_read(CFG_EDR_ADDRESS_BEGIN + cur_bt_idx, remote_addr, 6) == 6) {
                delete_link_key(remote_addr, get_remote_dev_info_index());
                syscfg_write(CFG_EDR_ADDRESS_BEGIN + cur_bt_idx, zero_addr, 6); //清空EDR回连地址
            }
            edr_set_enable(1);
            if (get_curr_channel_state()) {
                printf("edr operation pagscan...\n");
                edr_operation = EDR_OPERATION_PAGESCAN_IRQUIRY_SCAN;
            }
#endif
#if TCFG_USER_BLE_ENABLE
            get_random_number(tmp_ble_addr, 6);                             //重新生成BLE地址
            syscfg_write(CFG_BLE_ADDRESS_BEGIN + cur_bt_idx, tmp_ble_addr, 6);
            ble_reset_addr(tmp_ble_addr);
            ble_module_enable(1);
#endif
            return;
        }

        ret = syscfg_read(CFG_HID_MODE_BEGIN + cur_bt_idx, &vm_hid_mode, 1);    //读取HID模式
        printf("ret = %d vm_hid_mode:%d\n", ret, vm_hid_mode);

#if  TCFG_USER_EDR_ENABLE
        if ((ret == 1) && (vm_hid_mode == HID_MODE_EDR)) {
            //last conn is edr
            edr_set_enable(1);
            ret = syscfg_read(CFG_EDR_ADDRESS_BEGIN + cur_bt_idx, remote_addr, 6);
            if ((ret == 6) && memcmp(zero_addr, remote_addr, 6)) {              //read remote addr error
                printf("remote_addr:0x%x:");
                put_buf(remote_addr, 6);
                if (get_curr_channel_state()) {
                    edr_operation = EDR_OPERATION_RECONN;
                } else {
                    printf("reconnect edr at once...\n");
                    user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, remote_addr);
                }
            }
            return;
        }
#endif

#if TCFG_USER_BLE_ENABLE
        if ((ret == 1) && (vm_hid_mode == HID_MODE_BLE)) {
            //last conn is ble
            g_printf("sw ble...\n");
            if (syscfg_read(CFG_BLE_ADDRESS_BEGIN + cur_bt_idx, tmp_ble_addr, 6) != 6) {       //read current index ble address error
                get_random_number(tmp_ble_addr, 6);
                syscfg_write(CFG_BLE_ADDRESS_BEGIN + cur_bt_idx, tmp_ble_addr, 6);
            }
            ble_reset_addr(tmp_ble_addr);
            return;
        }
#endif

        //last conn is null
        g_printf("last conn is null\n");
#if TCFG_USER_EDR_ENABLE
        edr_set_enable(1);
        if (get_curr_channel_state()) {
            printf("edr operation pagscan...\n");
            edr_operation = EDR_OPERATION_PAGESCAN_IRQUIRY_SCAN;
        }
#endif

#if TCFG_USER_BLE_ENABLE
        if (syscfg_read(CFG_BLE_ADDRESS_BEGIN + cur_bt_idx, tmp_ble_addr, 6) != 6) {       //read current index ble address error
            get_random_number(tmp_ble_addr, 6);
            syscfg_write(CFG_BLE_ADDRESS_BEGIN + cur_bt_idx, tmp_ble_addr, 6);
        }
        ble_reset_addr(tmp_ble_addr);
#endif
        break;
    }
}

void hid_report_send(u8 report_id, u8 *data, u16 len)
{
    if (bt_hid_mode == HID_MODE_EDR) {
#if TCFG_USER_EDR_ENABLE
        edr_hid_data_send(report_id, data, len);
#endif
    } else {
#if TCFG_USER_BLE_ENABLE
        ble_hid_data_send(report_id, data, len);
#endif
    }
}

void bt_send_keypress(u8 key)
{
    user_send_cmd_prepare(USER_CTRL_KEYPRESS, 1, &key);
}

void send_matrix_key_report(u16 *key)
{
    u8 i, fill = 0;
    u8 key_report[8] = {0};
    u16 key_value = 0;

    for (i = 0; i < 6; i++) {
        if (key[i] == 0xffff) {
            break;
        }

        key_value = matrix_key_table[(key[i] & 0xf)][key[i] >> 8];

#if SUPPORT_USER_PASSKEY
        if (key_pass_enter && key_pass_enter < 7) {
            if (key_value >= _KEY_1  && key_value <= _KEY_0) {
                printf("enter key:%d\n", key_value - _KEY_1 + 1);
                bt_send_keypress((key_value == _KEY_0) ? 0 : (key_value - _KEY_1 + 1));
                key_pass_enter ++;
            }
            return;
        } else {
            key_pass_enter = 0;
        }
#endif

        if (key_value >> 8) {
            key_report[0] |= key_value;
        } else {
            key_report[2 + (fill++)] = key_value;
        }
    }

    put_buf(key_report, 8);
    hid_report_send(KEYBOARD_REPORT_ID, key_report, 8);
}

void Phantomkey_process(void)
{
    u8 i, j = 0;
    u16 temp = 0;
    for (i = 0; i < (sizeof(key_status_array) / sizeof(u16)) - 1; i++) {    //先对按键表进行排序按col来排序
        for (j = 0; j < (sizeof(key_status_array) / sizeof(u16)) - i - 1; j++) {
            if ((key_status_array[j] >> 8) > (key_status_array[j + 1] >> 8)) {
                temp = key_status_array[j];
                key_status_array[j] = key_status_array[j + 1];
                key_status_array[j + 1] = temp;
            }
        }
    }
    for (i = 0; i < (sizeof(key_status_array) / sizeof(u16)) - 1; i++) {
        if (key_status_array[i] == 0xffff) {
            return;
        }
        if ((key_status_array[i] >> 8) == (key_status_array[i + 1] >> 8)) { //寻找col相同的按键
            //printf("%x %x\n", key_status_array[i], key_status_array[i + 1]);
            for (j = 0; j < sizeof(key_status_array) / sizeof(u16); j++) {
                //寻找是否有相邻row的按键，如果有就是幻影键
                if ((j != i) && (j != i + 1) && ((key_status_array[i] & 0xff) == (key_status_array[j] & 0xff))) {           //寻找与这两个键row相同的按键
                    //printf("Phantomkey mark0...\n");
                    memset(key_status_array, 0xff, sizeof(key_status_array));
                    return;
                }
                if ((j != i) && (j != i + 1) && ((key_status_array[i + 1] & 0xff) == (key_status_array[j] & 0xff))) {
                    //printf("Phantomkey mark1...\n");
                    memset(key_status_array, 0xff, sizeof(key_status_array));
                    return;
                }
            }
        }
    }
}

#include "asm/power/p33.h"
//void p33_tx_1byte(u16 addr, u8 data0);
void full_key_array(u8 row, u8 col, u8 st)
{
    static u8 wk_toggle = 0;
    u8 offset = 0;
    u8 mark = 0, mark_offset = 0;
    //最多推6个按键出来，如果需要推多个按键需要自行修改，每个u16 低八位标识row 高八位标识col
    u16 key_value = (row | col << 8);
    for (offset = 0; offset < sizeof(key_status_array) / sizeof(u16); offset++) {
        if (key_status_array[offset] == key_value) {         //找到列表中有当前按键且按键状态抬起,则将按键移除列表
            mark = 1;                                                               //记录相同键值所在位置
            mark_offset = offset;
            if (mark_offset == sizeof(key_status_array) / sizeof(u16) - 1) {
                key_status_array[mark_offset] = 0xffff;
                break;
            }
        } else if (key_status_array[offset] == 0xffff) {
            if (mark && st == MATRIX_KEY_UP) {
                memcpy(key_status_array + mark_offset, key_status_array + mark_offset + 1, (offset - mark_offset - 1) * 2);
                key_status_array[offset - 1] = 0xffff;
            } else if (!mark && st == MATRIX_KEY_SHORT) {                                    //需要状态为短按才会把键值填充到数组中，防止按键满了之后把抬起也填充进去
                key_status_array[offset] = key_value;
            }
            break;
        } else if (mark && (offset == sizeof(key_status_array) / sizeof(u16) - 1)) {
            memcpy(key_status_array + mark_offset, key_status_array + mark_offset + 1, (sizeof(key_status_array) / sizeof(u16) - mark_offset - 1) * 2);
            key_status_array[sizeof(key_status_array) / sizeof(u16) - 1] = 0xffff;
            break;
        }
    }
    /* for (offset = 0; offset < sizeof(key_status_array) / sizeof(u16); offset++) { */
    /* } */
}

void user_key_timeout(void *priv)
{
    user_key_deal(remap_event, 1);
    user_key_timer_hdl = 0;
}

u8  special_key_deal(u8 *map, special_key *key_tab, u8 key_tab_size, u16 *remap_event_tab, u8 is_fn_remap_deal)
{
    static u8 special_key_press = 0;
    static u16 last_user_key = 0xff;
    u8 i = 0;

    if (map[FN_COL] & BIT(FN_ROW) || !is_fn_remap_deal) {        //判断fn键是否按下
        for (i = 0; i < key_tab_size; i++) {
            if (map[key_tab[i].col] & BIT(key_tab[i].row)) {
                special_key_press = 1;
                remap_event = remap_event_tab[i];
                if (key_tab[i].is_user_key) {
                    if (user_key_timer_hdl) {
                        if (remap_event != last_user_key) {
                            sys_timeout_del(user_key_timer_hdl);
                        }
                    } else {
                        user_key_timer_hdl = sys_timeout_add(NULL, user_key_timeout, 3000);
                    }
                    last_user_key = remap_event;
                } else {
                    printf("last_event:0x%x\n", remap_event);
                    hid_report_send(COUSTOM_CONTROL_REPORT_ID, &remap_event, 2);
                }
                return 1;
            } else {
                remap_event = 0;
            }
        }
        if (user_key_timer_hdl) {       //timer hdl不为0说明超时还没到，timeout = 0识别为短按
            sys_timeout_del(user_key_timer_hdl);
            user_key_timer_hdl = 0;
            user_key_deal(last_user_key, 0);
        }
    }
    if (special_key_press == 1) {
        if (user_key_timer_hdl) {       //timer hdl不为0说明超时还没到，timeout = 0识别为短按
            sys_timeout_del(user_key_timer_hdl);
            user_key_timer_hdl = 0;
            user_key_deal(last_user_key, 0);
        }
        special_key_press = 0;
        remap_event = 0;
        hid_report_send(COUSTOM_CONTROL_REPORT_ID, &remap_event, 2);
        last_user_key = 0xff;
        return 1;
    }
    return 0;
}


void matrix_key_map_deal(u8 *map)
{
    u8 row, col, i = 0;
    static u8 fn_press = 0;

    if (special_key_deal(map, fn_remap_key, sizeof(fn_remap_key) / sizeof(special_key), fn_remap_event, 1)) {
        printf("fn mark...\n");
        return;
    }

    if (special_key_deal(map, other_key, sizeof(other_key) / sizeof(special_key), other_key_map, 0)) {
        return;
    }

    for (col = 0; col < COL_MAX; col++) {
        for (row = 0; row < ROW_MAX; row++) {
            if (map[col] & BIT(row)) {
                full_key_array(row, col, MATRIX_KEY_SHORT);
            } else {
                full_key_array(row, col, MATRIX_KEY_UP);
            }
        }
    }
    Phantomkey_process();
    send_matrix_key_report(key_status_array);
}


static s16 gradient_acceleration(s16 src)
{
#define GRADIENT_1              3
#define GRADIENT_2              10
#define ACCELERATION_1(x)       (x * 3 / 2) // 1.5
#define ACCELERATION_2(x)       (x * 2) // 2

    if ((src > GRADIENT_2) || (src < -GRADIENT_2)) {
        src = ACCELERATION_2(src);
    } else if ((src > GRADIENT_1) || (src < -GRADIENT_1)) {
        src = ACCELERATION_1(src);
    }

    return src;
}

void touch_pad_event_deal(struct sys_event *event)
{
    u8 mouse_report[8] = {0};
    if ((event->u).touchpad.gesture_event) {
        //g_printf("touchpad gesture_event:0x%x\n", (event->u).touchpad.gesture_event);
        switch ((event->u).touchpad.gesture_event) {
        case 0x1:
            mouse_report[0] |= _KEY_MOD_LMETA;
            mouse_report[2] = _KEY_EQUAL;
            hid_report_send(KEYBOARD_REPORT_ID, mouse_report, 8);
            memset(mouse_report, 0x0, 8);
            hid_report_send(KEYBOARD_REPORT_ID, mouse_report, 8);
            return;
        case 0x2:
            mouse_report[0] |= _KEY_MOD_LMETA;
            mouse_report[2] = _KEY_MINUS;
            /* mouse_report[1] = (_KEY_ZOOM_IN + (event->u).touchpad.gesture_event - 1) >> 8; */
            /* mouse_report[0] = (_KEY_ZOOM_OUT + (event->u).touchpad.gesture_event - 1) & 0xff; */
            hid_report_send(KEYBOARD_REPORT_ID, mouse_report, 8);
            memset(mouse_report, 0x0, 8);
            hid_report_send(KEYBOARD_REPORT_ID, mouse_report, 8);
            return;
        case 0x3:
            mouse_report[0] |= BIT(0);			//鼠标左键
            break;
        case 0x4:
            mouse_report[0] |= BIT(1);
            break;
        }
    }
    if ((event->u).touchpad.x || (event->u).touchpad.y) {
        //g_printf("touchpad x:%d y:%d\n", (event->u).touchpad.x, (event->u).touchpad.y);
        mouse_report[1] = gradient_acceleration((event->u).touchpad.x);
        mouse_report[2] = gradient_acceleration((event->u).touchpad.y);
    }
    hid_report_send(MOUSE_POINT_REPORT_ID, mouse_report, 3);
}

void auto_shutdown_disable(void)
{
    if (g_auto_shutdown_timer) {
        sys_timeout_del(g_auto_shutdown_timer);
    }
}


static void bt_function_select_init()
{
    __set_user_ctrl_conn_num(TCFG_BD_NUM);
    __set_support_msbc_flag(1);

#if BT_SUPPORT_DISPLAY_BAT
    __bt_set_update_battery_time(60);
#else
    __bt_set_update_battery_time(0);
#endif

    __set_page_timeout_value(8000); /*回连搜索时间长度设置,可使用该函数注册使用，ms单位,u16*/
    __set_super_timeout_value(8000); /*回连时超时参数设置。ms单位。做主机有效*/

#if (TCFG_BD_NUM == 2)
    __set_auto_conn_device_num(2);
#endif

    //io_capabilities ; /*0: Display only 1: Display YesNo 2: KeyboardOnly 3: NoInputNoOutput*/
    //authentication_requirements: 0:not protect  1 :protect
#if SUPPORT_USER_PASSKEY
    __set_simple_pair_param(2, 0, 2);
#else
    __set_simple_pair_param(3, 0, 2);
#endif

    /* le_controller_set_mac((void*)"012345"); */
    if (syscfg_read(CFG_CUR_BT_IDX, &cur_bt_idx, 1) != 1) {
        cur_bt_idx = 0;
        syscfg_write(CFG_CUR_BT_IDX, &cur_bt_idx, 1);
    }
#if TCFG_USER_BLE_ENABLE
    {
        u8 tmp_ble_addr[6];
        printf("use cur_ble_idx:%d\n", cur_bt_idx);

        if (syscfg_read(CFG_BLE_ADDRESS_BEGIN + cur_bt_idx, tmp_ble_addr, 6) != 6) {       //read err
            get_random_number(tmp_ble_addr, 6);
            syscfg_write(CFG_BLE_ADDRESS_BEGIN + cur_bt_idx, tmp_ble_addr, 6);
        }
        le_controller_set_mac((void *)tmp_ble_addr);

        printf("\n-----edr + ble 's address-----");
        printf_buf((void *)bt_get_mac_addr(), 6);
        printf_buf((void *)tmp_ble_addr, 6);
    }
#endif
}

static void bredr_handle_register()
{
#if (USER_SUPPORT_PROFILE_HID==1)
    user_hid_set_icon(BD_CLASS_KEYBOARD);
    user_hid_set_ReportMap(hid_report_map, sizeof(hid_report_map));
    user_hid_init(edr_led_status_callback);
#endif

    /* bt_dut_test_handle_register(bt_dut_api); */
}

void hid_set_soft_poweroff(void)
{
    log_info("hid_set_soft_poweroff\n");
    is_hid_active = 1;
    //必须先主动断开蓝牙链路,否则要等链路超时断开
    if (bt_hid_mode == HID_MODE_EDR) {
        user_hid_enable(0);
    } else {
#if TCFG_USER_BLE_ENABLE
        ble_module_enable(0);
#endif
    }
    //延时300ms，确保BT退出链路断开
    sys_timeout_add(NULL, power_set_soft_poweroff, WAIT_DISCONN_TIME_MS);
}



static void app_start()
{
    log_info("=======================================");
    log_info("-------------HID DEMO-----------------");
    log_info("=======================================");

    putchar('0');

    clk_set("sys", BT_NORMAL_HZ);
    putchar('1');
    u32 sys_clk =  clk_get("sys");
    putchar('2');
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);
    putchar('3');

    u8 edr_ex_name[] = " 3.0";

    u8 cfg_name[LOCAL_NAME_LEN] = "JL_HID";
    u8 cfg_name_len = 0;
    int ret = syscfg_read(CFG_BT_NAME, cfg_name, 32);
    if (ret < 0) {
        log_info("read bt name err\n");
    } else {
        cfg_name_len = strlen(cfg_name);
        if (cfg_name_len + sizeof(edr_ex_name) <= LOCAL_NAME_LEN) {
            //增加后缀，区分名字
            memcpy(cfg_name + cfg_name_len, edr_ex_name, sizeof(edr_ex_name));
            bt_set_local_name(cfg_name, cfg_name_len + sizeof(edr_ex_name));
        }
    }
    putchar('4');

    bt_function_select_init();
    putchar('5');
#if TCFG_USER_EDR_ENABLE
    bredr_handle_register();
#endif
    putchar('6');

    btstack_init();

    putchar('7');
#if TCFG_USER_EDR_ENABLE
    sys_auto_sniff_controle(1, NULL);
#endif
    /* 按键消息使能 */
    sys_key_event_enable();

#if (TCFG_HID_AUTO_SHUTDOWN_TIME)
    //无操作定时软关机
    g_auto_shutdown_timer = sys_timeout_add(NULL, hid_set_soft_poweroff, TCFG_HID_AUTO_SHUTDOWN_TIME * 1000);
#endif
    /* sys_auto_shut_down_enable(); */
    /* sys_auto_sniff_controle(1, NULL); */
}

void keyboard_disconnect_deal(void)
{

}

static int state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        if (!it) {
            break;
        }
        switch (it->action) {
        case ACTION_HID_MAIN:
            app_start();
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

#define  SNIFF_CNT_TIME               5/////<空闲5S之后进入sniff模式

#define SNIFF_MAX_INTERVALSLOT        48
#define SNIFF_MIN_INTERVALSLOT        48
#define SNIFF_ATTEMPT_SLOT            2
#define SNIFF_TIMEOUT_SLOT            1

static u8 sniff_ready_status = 0; //0:sniff_ready 1:sniff_not_ready
static int exit_sniff_timer = 0;
static int sniff_timer = 0;


void bt_check_exit_sniff()
{
    if (exit_sniff_timer) {
        sys_timeout_del(exit_sniff_timer);
        exit_sniff_timer = 0;
    }
    user_send_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
}

static void bt_sniff_ready_clean(void)
{
    sniff_ready_status = 1;
}

void bt_check_enter_sniff()
{
    struct sniff_ctrl_config_t sniff_ctrl_config;
    u8 addr[12];
    u8 conn_cnt = 0;
    u8 i = 0;

    if (sniff_ready_status) {
        sniff_ready_status = 0;
        return;
    }

    /*putchar('H');*/
    conn_cnt = bt_api_enter_sniff_status_check(SNIFF_CNT_TIME, addr);

    ASSERT(conn_cnt <= 2);

    for (i = 0; i < conn_cnt; i++) {
        log_info("-----USER SEND SNIFF IN %d %d\n", i, conn_cnt);
        sniff_ctrl_config.sniff_max_interval = SNIFF_MAX_INTERVALSLOT;
        sniff_ctrl_config.sniff_mix_interval = SNIFF_MIN_INTERVALSLOT;
        sniff_ctrl_config.sniff_attemp = SNIFF_ATTEMPT_SLOT;
        sniff_ctrl_config.sniff_timeout  = SNIFF_TIMEOUT_SLOT;
        memcpy(sniff_ctrl_config.sniff_addr, addr + i * 6, 6);
        user_send_cmd_prepare(USER_CTRL_SNIFF_IN, sizeof(struct sniff_ctrl_config_t), (u8 *)&sniff_ctrl_config);
    }

}
static void sys_auto_sniff_controle(u8 enable, u8 *addr)
{
#if TCFG_USER_EDR_ENABLE
    if (addr) {
        if (bt_api_conn_mode_check(enable, addr) == 0) {
            log_info("sniff ctr not change\n");
            return;
        }
    }

    if (enable) {
        if (addr) {
            log_info("sniff cmd timer init\n");
            user_cmd_timer_init();
        }

        if (sniff_timer == 0) {
            log_info("check_sniff_enable\n");
            sniff_timer = sys_timer_add(NULL, bt_check_enter_sniff, 1000);
        }
    } else {

        if (addr) {
            log_info("sniff cmd timer remove\n");
            remove_user_cmd_timer();
        }

        if (sniff_timer) {
            log_info("check_sniff_disable\n");
            sys_timeout_del(sniff_timer);
            sniff_timer = 0;

            if (exit_sniff_timer == 0) {
                /* exit_sniff_timer = sys_timer_add(NULL, bt_check_exit_sniff, 5000); */
            }
        }
    }
#endif
}
/*开关可发现可连接的函数接口*/
static void bt_wait_phone_connect_control(u8 enable)
{
    int tmp;
    __asm__ volatile("%0 =rets" : "=r"(tmp));
    printf("%s %d rets:0x%x\n", __func__, enable, tmp);
    if (enable) {
        log_info("is_1t2_connection:%d \t total_conn_dev:%d, enable:%d\n", is_1t2_connection(), get_total_connect_dev(), enable);
        if (is_1t2_connection()) {
            /*达到最大连接数，可发现(0)可连接(0)*/
            user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
            user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
        } else {
            if (get_total_connect_dev() == 1) {
                /*支持连接2台，只连接一台的情况下，可发现(0)可连接(1)*/
                user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
                user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
            } else {
                /*可发现(1)可连接(1)*/
                user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
                user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
            }
        }
    } else {
        user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    }
    log_info("bt_end\n");
}

void bt_send_pair(u8 en)
{
    user_send_cmd_prepare(USER_CTRL_PAIR, 1, &en);
}


int bt_connect_phone_back_start(void)
{
    if (connect_last_device_from_vm()) {
        log_info("------bt_connect_phone_start------\n");
        return 1 ;
    }
    printf("vm no remote info...\n");
    return 0;
}




#define HCI_EVENT_INQUIRY_COMPLETE                            0x01
#define HCI_EVENT_CONNECTION_COMPLETE                         0x03
#define HCI_EVENT_DISCONNECTION_COMPLETE                      0x05
#define HCI_EVENT_PIN_CODE_REQUEST                            0x16
#define HCI_EVENT_IO_CAPABILITY_REQUEST                       0x31
#define HCI_EVENT_USER_CONFIRMATION_REQUEST                   0x33
#define HCI_EVENT_USER_PASSKEY_REQUEST                        0x34
#define HCI_EVENT_USER_PRESSKEY_NOTIFICATION			      0x3B
#define HCI_EVENT_VENDOR_NO_RECONN_ADDR                       0xF8
#define HCI_EVENT_VENDOR_REMOTE_TEST                          0xFE
#define BTSTACK_EVENT_HCI_CONNECTIONS_DELETE                  0x6D


#define ERROR_CODE_SUCCESS                                    0x00
#define ERROR_CODE_PAGE_TIMEOUT                               0x04
#define ERROR_CODE_AUTHENTICATION_FAILURE                     0x05
#define ERROR_CODE_PIN_OR_KEY_MISSING                         0x06
#define ERROR_CODE_CONNECTION_TIMEOUT                         0x08
#define ERROR_CODE_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED  0x0A
#define ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS                      0x0B
#define ERROR_CODE_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES       0x0D
#define ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR    0x0F
#define ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED         0x10
#define ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION          0x13
#define ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST        0x16

#define CUSTOM_BB_AUTO_CANCEL_PAGE                            0xFD  //// app cancle page
#define BB_CANCEL_PAGE                                        0xFE  //// bb cancle page



static void hogp_ble_status_callback(ble_state_e status, u8 reason)
{
    log_info("hogp_ble_status_callback==================== %d   reason:0x%x\n", status, reason);
    switch (status) {
    case BLE_ST_IDLE:
        break;
    case BLE_ST_ADV:
        break;
    case BLE_ST_CONNECT:
        printf("BLE_ST_CONNECT\n");
        bt_hid_mode = HID_MODE_BLE;
        syscfg_write(CFG_HID_MODE_BEGIN + cur_bt_idx, &bt_hid_mode, 1);         //保存当前HID模式
#if TCFG_USER_EDR_ENABLE
        edr_set_enable(0);
#endif      //TCFG_USER_EDR_MODE
        break;
    case BLE_ST_SEND_DISCONN:
        break;
    case BLE_ST_DISCONN:
        if (reason == ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST) {
            printf("BLE_ST_DISCONN BY LOCAL...\n");
#if TCFG_USER_EDR_ENABLE
            if (edr_operation == EDR_OPERATION_PAGESCAN_IRQUIRY_SCAN) {

            } else if (edr_operation == EDR_OPERATION_RECONN) {
                g_printf("reconn edr...\n");
                put_buf(remote_addr, 6);
                user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, remote_addr);     //回连设备
            }
#endif
        }
        edr_operation = EDR_OPERATION_NULL;
        break;
    case BLE_ST_NOTIFY_IDICATE:
        break;
    default:
        break;
    }
}

static void bt_hci_event_connection(struct bt_event *bt)
{
    bt_wait_phone_connect_control(0);
}

static void bt_hci_event_disconnect(struct bt_event *bt)
{
    printf("bt->value:0x%x\n", bt->value);
    if (edr_operation == EDR_OPERATION_PAGESCAN_IRQUIRY_SCAN) {
        printf("EDR_OPERATION_PAGESCAN_IRQUIRY_SCAN...\n");
        bt_wait_phone_connect_control(1);
    } else if (edr_operation == EDR_OPERATION_RECONN) {
        g_printf("reconn edr...\n");
        put_buf(remote_addr, 6);
        user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, remote_addr);     //回连设备
    } else {
        switch (bt->value) {
        case ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST:

            break;
        case ERROR_CODE_PAGE_TIMEOUT:           //回连失败需要重新按键发起回连或者长按清楚配对进入连接状态
        case ERROR_CODE_PIN_OR_KEY_MISSING:     //这种情况为手机取消配对或者某些电脑回连失败， 需要重新按键发起回连或者长按清除配对信息
            bt_wait_phone_connect_control(0);
            break;
        default:
            bt_wait_phone_connect_control(1);
            break;
        }
    }
    key_pass_enter = 0;
    edr_operation = EDR_OPERATION_NULL;
}

static void bt_hci_event_linkkey_missing(struct bt_event *bt)
{
    bt_wait_phone_connect_control(1);
}

static void bt_hci_event_page_timeout(struct bt_event *bt)
{
    bt_wait_phone_connect_control(1);
}

static void bt_hci_event_connection_timeout(struct bt_event *bt)
{
    bt_wait_phone_connect_control(1);
}

static void bt_hci_event_connection_exist(struct bt_event *bt)
{
    bt_wait_phone_connect_control(1);
}

static int bt_hci_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_info("------------------------bt_hci_event_handler reason %x %x", bt->event, bt->value);

    if (bt->event == HCI_EVENT_VENDOR_REMOTE_TEST) {
        if (0 == bt->value) {
            set_remote_test_flag(0);
            log_info("clear_test_box_flag");
            return 0;
        } else {

#if TCFG_USER_BLE_ENABLE
            //1:edr con;2:ble con;
            if (1 == bt->value) {
                extern void bt_ble_adv_enable(u8 enable);
                bt_ble_adv_enable(0);
            }
#endif
        }
    }

    if ((bt->event != HCI_EVENT_CONNECTION_COMPLETE) ||
        ((bt->event == HCI_EVENT_CONNECTION_COMPLETE) && (bt->value != ERROR_CODE_SUCCESS))) {
#if TCFG_TEST_BOX_ENABLE
        if (chargestore_get_testbox_status()) {
            if (get_remote_test_flag()) {
                chargestore_clear_connect_status();
            }
            //return 0;
        }
#endif
        if (get_remote_test_flag() \
            && !(HCI_EVENT_DISCONNECTION_COMPLETE == bt->event) \
            && !(HCI_EVENT_VENDOR_REMOTE_TEST == bt->event)) {
            log_info("cpu reset\n");
            cpu_reset();
        }
    }

    switch (bt->event) {
    case HCI_EVENT_INQUIRY_COMPLETE:
        log_info(" HCI_EVENT_INQUIRY_COMPLETE \n");
        /* bt_hci_event_inquiry(bt); */
        break;
    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
        log_info(" HCI_EVENT_USER_CONFIRMATION_REQUEST \n");
        ///<可通过按键来确认是否配对 1：配对   0：取消
        bt_send_pair(1);
        break;
    case HCI_EVENT_USER_PASSKEY_REQUEST:
        log_info(" HCI_EVENT_USER_PASSKEY_REQUEST \n");
        key_pass_enter = 1;
        ///<可以开始输入6位passkey
        break;
    case HCI_EVENT_USER_PRESSKEY_NOTIFICATION:
        log_info(" HCI_EVENT_USER_PRESSKEY_NOTIFICATION %x\n", bt->value);
        ///<可用于显示输入passkey位置 value 0:start  1:enrer  2:earse   3:clear  4:complete
        break;
    case HCI_EVENT_PIN_CODE_REQUEST :
        log_info("HCI_EVENT_PIN_CODE_REQUEST  \n");
        bt_send_pair(1);
        break;

    case HCI_EVENT_VENDOR_NO_RECONN_ADDR :
        log_info("HCI_EVENT_VENDOR_NO_RECONN_ADDR \n");
        bt_hci_event_disconnect(bt) ;
        break;

    case HCI_EVENT_DISCONNECTION_COMPLETE :
        log_info("HCI_EVENT_DISCONNECTION_COMPLETE \n");
        bt_hci_event_disconnect(bt) ;
        break;

    case BTSTACK_EVENT_HCI_CONNECTIONS_DELETE:
    case HCI_EVENT_CONNECTION_COMPLETE:
        log_info(" HCI_EVENT_CONNECTION_COMPLETE \n");
        switch (bt->value) {
        case ERROR_CODE_SUCCESS :
            log_info("ERROR_CODE_SUCCESS  \n");
            bt_hci_event_connection(bt);
            break;
        case ERROR_CODE_PIN_OR_KEY_MISSING:
            log_info(" ERROR_CODE_PIN_OR_KEY_MISSING \n");
            bt_hci_event_linkkey_missing(bt);

        case ERROR_CODE_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED :
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES:
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR:
        case ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED  :
        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION   :
        case ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST :
        case ERROR_CODE_AUTHENTICATION_FAILURE :
        case CUSTOM_BB_AUTO_CANCEL_PAGE:
            bt_hci_event_disconnect(bt) ;
            break;

        case ERROR_CODE_PAGE_TIMEOUT:
            log_info(" ERROR_CODE_PAGE_TIMEOUT \n");
            bt_hci_event_page_timeout(bt);
            break;

        case ERROR_CODE_CONNECTION_TIMEOUT:
            log_info(" ERROR_CODE_CONNECTION_TIMEOUT \n");
            bt_hci_event_connection_timeout(bt);
            break;

        case ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS  :
            log_info("ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS   \n");
            bt_hci_event_connection_exist(bt);
            break;
        default:
            break;

        }
        break;
    default:
        break;

    }
    return 0;
}

void ble_led_status_callback(u8 *buffer, u16 len)
{
    put_buf(buffer, len);
    if (buffer[0] & BIT(1)) {   //CAP灯的状态
        gpio_set_output_value(CAP_LED_PIN, CAP_LED_ON_VALUE);
    } else {
        gpio_set_output_value(CAP_LED_PIN, !CAP_LED_ON_VALUE);
    }
}

void edr_led_status_callback(u8 *buffer, u16 len)
{
    put_buf(buffer, len);
    if (buffer[0] == 0xA2) {    //SET_REPORT && Output
        if (buffer[1] ==  KEYBOARD_REPORT_ID) {
            if (buffer[2] & BIT(1)) {   //CAP灯的状态
                gpio_set_output_value(CAP_LED_PIN, CAP_LED_ON_VALUE);
            } else {
                gpio_set_output_value(CAP_LED_PIN, !CAP_LED_ON_VALUE);
            }
        }
    }
}
void le_hogp_set_output_callback(void *cb);
static void keyboard_mode_init(u8 hid_mode)
{
    u8 vm_hid_mode = HID_MODE_NULL;

    if ((!STACK_MODULES_IS_SUPPORT(BT_BTSTACK_LE) || !BT_MODULES_IS_SUPPORT(BT_MODULE_LE)) && (!STACK_MODULES_IS_SUPPORT(BT_BTSTACK_CLASSIC) || !BT_MODULES_IS_SUPPORT(BT_MODULE_CLASSIC))) {
        log_info("not surpport ble or edr,make sure config !!!\n");
        ASSERT(0);
    }

    if (syscfg_read(CFG_HID_MODE_BEGIN + cur_bt_idx, &vm_hid_mode, 1) == 1) {  //读取HID模式
        hid_mode = vm_hid_mode;
    }

#if TCFG_USER_EDR_ENABLE
    if (hid_mode & HID_MODE_EDR) {
        printf("HID EDR MODE INIT\n");
        user_hid_enable(1);
        if (!bt_connect_phone_back_start()) {
            bt_wait_phone_connect_control(1);
        }
    }
#endif

#if TCFG_USER_BLE_ENABLE
    le_hogp_set_output_callback(ble_led_status_callback);
    if (hid_mode & HID_MODE_BLE) {
        printf("HID BLE MODE INIT\n");
        u8 ble_ex_name[] = " 4.0";

        u8 cfg_name[LOCAL_NAME_LEN] = "JL_HID";
        u8 cfg_name_len = 0;
        int ret = syscfg_read(CFG_BT_NAME, cfg_name, 32);
        if (ret < 0) {
            log_info("read bt name err\n");
        } else {
            cfg_name_len = strlen(cfg_name);
            if (cfg_name_len + sizeof(ble_ex_name) <= LOCAL_NAME_LEN) {
                //增加后缀，区分名字
                memcpy(cfg_name + cfg_name_len, ble_ex_name, sizeof(ble_ex_name));
                modify_ble_name(cfg_name);
            }
        }
        ble_module_enable(1);
    }
#endif
}

static int bt_connction_status_event_handler(struct bt_event *bt)
{

    log_info("-----------------------bt_connction_status_event_handler %d", bt->event);

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        /*
         * 蓝牙初始化完成
         */
        log_info("BT_STATUS_INIT_OK\n");

#if TCFG_USER_BLE_ENABLE
        le_hogp_set_icon(BLE_APPEARANCE_HID_KEYBOARD);//keyboard
        le_hogp_set_ReportMap(hid_report_map, sizeof(hid_report_map));

        bt_ble_init();
#endif

        //hid_vm_deal(0);//bt_hid_mode read for VM
        keyboard_mode_init(HID_MODE_COMBO);
        break;

    case BT_STATUS_SECOND_CONNECTED:
    case BT_STATUS_FIRST_CONNECTED:
        log_info("BT_STATUS_CONNECTED: bt_idx:%d\n", cur_bt_idx);
        bt_hid_mode = HID_MODE_EDR;
        syscfg_write(CFG_HID_MODE_BEGIN + cur_bt_idx, &bt_hid_mode, 1);         //保存当前HID模式

        put_buf(bt->args, 6);
        syscfg_write(CFG_EDR_ADDRESS_BEGIN + cur_bt_idx, bt->args, 6);
#if TCFG_USER_BLE_ENABLE
        //close ble
        ble_module_enable(0);
#endif
        break;

    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
        log_info("BT_STATUS_DISCONNECT\n");
        break;

    case BT_STATUS_PHONE_INCOME:
        log_info("BT_STATUS_PHONE_INCOME\n");
        break;

    case BT_STATUS_PHONE_OUT:
        log_info("BT_STATUS_PHONE_OUT\n");
        break;

    case BT_STATUS_PHONE_ACTIVE:
        log_info("BT_STATUS_PHONE_ACTIVE\n");
        break;

    case BT_STATUS_PHONE_HANGUP:
        log_info("BT_STATUS_PHONE_HANGUP\n");
        break;

    case BT_STATUS_PHONE_NUMBER:
        log_info("BT_STATUS_PHONE_NUMBER\n");
        break;

    case BT_STATUS_INBAND_RINGTONE:
        log_info("BT_STATUS_INBAND_RINGTONE\n");
        break;

    case BT_STATUS_BEGIN_AUTO_CON:
        log_info("BT_STATUS_BEGIN_AUTO_CON\n");
        break;

    case BT_STATUS_A2DP_MEDIA_START:
        log_info(" BT_STATUS_A2DP_MEDIA_START");
        break;

    case BT_STATUS_SNIFF_STATE_UPDATE:
        log_info(" BT_STATUS_SNIFF_STATE_UPDATE %d\n", bt->value);    //0退出SNIFF
        if (bt->value == 0) {
            sys_auto_sniff_controle(1, bt->args);
        } else {
            sys_auto_sniff_controle(0, bt->args);
        }
        break;
    case  BT_STATUS_TRIM_OVER:
        log_info("BT STATUS TRIM OVER\n");
        break;
    default:
        log_info(" BT STATUS DEFAULT\n");
        break;
    }
    return 0;
}

static void app_key_event_handler(struct sys_event *event)
{
    /* u16 cpi = 0; */
    u8 event_type = 0;
    u8 key_value = 0;

    if (event->arg == (void *)DEVICE_EVENT_FROM_KEY) {
        event_type = event->u.key.event;
        key_value = event->u.key.value;
        printf("app_key_evnet: %d,%d\n", event_type, key_value);
    }
}


static int event_handler(struct application *app, struct sys_event *event)
{
    u8 i = 0;
#if (TCFG_HID_AUTO_SHUTDOWN_TIME)
    //重置无操作定时计数
    if (event->type != SYS_DEVICE_EVENT || DEVICE_EVENT_FROM_POWER != event->arg) { //过滤电源消息
        sys_timer_modify(g_auto_shutdown_timer, TCFG_HID_AUTO_SHUTDOWN_TIME * 1000);
    }
#endif

    bt_sniff_ready_clean();

    /* log_info("event: %s", event->arg); */
    switch (event->type) {
    case SYS_KEY_EVENT:
        /* log_info("Sys Key : %s", event->arg); */
        app_key_event_handler(event);
        return 0;

    case SYS_BT_EVENT:
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            bt_connction_status_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
            bt_hci_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_BLE_STATUS) {
            hogp_ble_status_callback(event->u.bt.event, event->u.bt.value);
        }
        return 0;

    case SYS_DEVICE_EVENT:
        if ((u32)event->arg == DEVICE_EVENT_FROM_POWER) {
            return app_power_event_handler(&event->u.dev);
        }
#if TCFG_CHARGE_ENABLE
        else if ((u32)event->arg == DEVICE_EVENT_FROM_CHARGE) {
            app_charge_event_handler(&event->u.dev);
        }
#endif
        return 0;


    case SYS_MATRIX_KEY_EVENT:
        matrix_key_map_deal((event->u).matrix_key.map);
        //send_matrix_key_report((event->u).matrix_key.args);
        break;

    case SYS_TOUCHPAD_EVENT:
        touch_pad_event_deal(event);
        break;

    default:
        return FALSE;
    }

    return FALSE;
}


//-----------------------
//system check go sleep is ok
static u8 app_hid_idle_query(void)
{
    return !is_hid_active;
}

REGISTER_LP_TARGET(app_hid_lp_target) = {
    .name = "app_hid_deal",
    .is_idle = app_hid_idle_query,
};


static const struct application_operation app_hid_ops = {
    .state_machine  = state_machine,
    .event_handler 	= event_handler,
};

/*
 * 注册模式
 */
REGISTER_APPLICATION(app_hid) = {
    .name 	= "hid_key",
    .action	= ACTION_HID_MAIN,
    .ops 	= &app_hid_ops,
    .state  = APP_STA_DESTROY,
};


#endif

