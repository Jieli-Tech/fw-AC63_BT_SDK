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
#include "user_cfg.h"
#include "usb/host/usb_hid_keys.h"
#include "matrix_keyboard.h"
#include "user_cfg.h"
#include "app_charge.h"
#include "app_power_manage.h"
#include "gpio.h"
#include "app_comm_bt.h"
#include "usb/device/hid.h"
#include "gatt_common/le_gatt_common.h"
#if(CONFIG_APP_STANDARD_KEYBOARD)

#define LOG_TAG_CONST       STD_KEYB
#define LOG_TAG             "[STD_KEYB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define SUPPORT_RECONN_ADV_OR_DIRECT        0    //0:ADV回连接 1:DIRECT回连接
#define SUPPORT_KEYBOARD_NO_CONFLICT        0    //无冲按键支持
#define SUPPORT_USER_PASSKEY                0
#define CAP_LED_ON_VALUE                    1

//2.4G模式： 0---ble ,非0 2.4G配对码
#define CFG_RF_24G_CODE_ID  (0)//32bits
/* #define CFG_RF_24G_CODE_ID  (0x5555AAAA) */
#define CFG_RF_24G_CODE_CHANNEL 0x4//2.4g对应的通道

typedef enum {
    EDR_OPERATION_NULL = 0,
    EDR_OPERATION_RECONN,
    EDR_OPERATION_PAGESCAN_IRQUIRY_SCAN,
} edr_operation_t;

typedef enum {
    SYSTEM_IOS,
    SYSTEM_WIN,
    SYSTEM_ARD,
};


static u16 g_auto_shutdown_timer = 0;
static volatile u8 cur_bt_idx = 0;
static volatile u8 paired_flag  = 0;
static reconnect_flag = 0;
static vm_address_change_flag = 0;
static edr_operation_t edr_operation = EDR_OPERATION_NULL;
static u8 key_pass_enter  = 0;
static u8 remote_addr[6] = {0};
static int bt_close_pair_handler = 0;
static u8 keyboard_system = SYSTEM_WIN;

static u8 usb_connect_ok = 0;
static u8 btstack_init_ok;

static void hid_report_send(u8 report_id, u8 *data, u16 len);
static void stdkb_set_soft_poweroff(void);
static void sys_auto_sniff_controle(u8 enable, u8 *addr);
static void bt_wait_phone_connect_control(u8 enable);
static void stdkb_edr_led_status_callback(u8 *buffer, u16 len, u16 channel);

extern void ble_module_enable(u8 en);
extern int ble_hid_data_send(u8 report_id, u8 *data, u16 len);
extern int ble_hid_is_connected(void);
extern void delete_link_key(bd_addr_t *bd_addr, u8 id);
extern void le_hogp_set_output_callback(void *cb);
extern void modify_ble_name(const char *name);
extern void lmp_sniff_t_slot_attemp_reset(u16 slot, u16 attemp);
extern const int sniff_support_reset_anchor_point;   //sniff状态下是否支持reset到最近一次通信点，用于HID
void usb_hid_register_state_callback(void *callback);
static void stdkb_usb_state_callback(u8 state);
static void stdkb_usb_led_status_callback(u8 *buffer, u16 len);
/* extern void hogp_direct_adv_config_set(u8 type); */
extern void hogp_reconnect_adv_config_set(u8 adv_type, u32 adv_timeout);
static key_timeout_flag = 0;
extern void set_multi_devices_adv_flag(u8 adv_flag);
u8 get_key_timeout_flag();
static void stdkb_keyboard_irk_init(u8 bt_idx);
static void set_key_timeout_flag(u8 kt_flag);

#define KEYBOARD_ENTER_PAIR0    0x0
#define KEYBOARD_ENTER_PAIR1    0x1
#define KEYBOARD_ENTER_PAIR2    0x2
#define KEYBOARD_ENTER_PAIR3    0x3

#define KEYBOARD_SYSTEM_IOS     0x4
#define KEYBOARD_SYSTEM_WIN     0x5
#define KEYBOARD_SYSTEM_ARD     0x6
#define KEYBOARD_COMPOSE_KEY    0x7
#define KEYBOARD_F6FUNC         0x8
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

const u16 matrix_key_table[ROW_MAX][COL_MAX] = {                //高八位用来标识是否特殊键
//   0                         1         2               3        4        5        6                7          8               9                      10             11         12       13     14      15                     16
    /**/{0,                     _KEY_Q,   _KEY_W,         _KEY_E,  _KEY_R,  _KEY_U,  _KEY_I,          _KEY_O,    _KEY_P,             0,                     0,             0,         0,       0,    0,      0},

    /**/{0,                     _KEY_TAB, _KEY_CAPSLOCK,  _KEY_F3, _KEY_T,  _KEY_Y,  _KEY_RIGHTBRACE, _KEY_F7,   _KEY_LEFTBRACE,     0,                  _KEY_BACKSPACE,   0,         0,       0,    0,  S_KEY(_KEY_MOD_LSHIFT), S_KEY(_KEY_MOD_LALT)},

    /**/{0,                     _KEY_A,   _KEY_S,         _KEY_D,  _KEY_F,  _KEY_J,  _KEY_K,          _KEY_L,    _KEY_SEMICOLON,  S_KEY(_KEY_MOD_LCTRL), _KEY_BACKSLASH,   0,         0,       0,    0,  S_KEY(_KEY_MOD_RSHIFT)},

    /**/{0,                     _KEY_ESC,    0,           _KEY_F4, _KEY_G,  _KEY_H,  _KEY_F6,            0,      _KEY_APOSTROPHE, _KEY_LEFTMETA,            0,          _KEY_SPACE,   0,       0, _KEY_UP},

    /**/{S_KEY(_KEY_MOD_RALT),  _KEY_Z,   _KEY_X,         _KEY_C,  _KEY_V,  _KEY_M,  _KEY_COMMA,      _KEY_DOT,     0,               0,                  _KEY_ENTER,    _KEY_F11,  _KEY_MINUS},

    /**/{0,                       0,         0,             0,     _KEY_B,  _KEY_N,    0,                0,      _KEY_SLASH,      _KEY_RIGHTMETA,           0,          _KEY_DOWN, _KEY_RIGHT, 0, _KEY_LEFT},

    /**/{0,                     _KEY_GRAVE, _KEY_F1,      _KEY_F2, _KEY_5,  _KEY_6,  _KEY_EQUAL,      _KEY_F8,   _KEY_MINUS,         0,                  _KEY_F9,       _KEY_DELETE},

    {_KEY_F5,               _KEY_1,   _KEY_2,         _KEY_3,  _KEY_4,  _KEY_7,  _KEY_8,          _KEY_9,    _KEY_0,          _KEY_F12,              _KEY_F10},
};
//bd19+ad15键值对应----打开之后需要将fn_remap_key中的F3和F7 col设置为2 && 同时将matrix_keyboard.h中的COL_MAX 设置为16
/* const u16 matrix_key_table[ROW_MAX][COL_MAX] = {                //高八位用来标识是否特殊键 */
/* //  0                        1         2           3        4       5         6                7              8               9                    10              11         12        13    14  15                            16 */
/*     {0,                    _KEY_Q,   _KEY_W,       _KEY_E,  _KEY_R,  _KEY_U,  _KEY_I,          _KEY_O,        _KEY_P,         0,                    0,             0,         0,        0,    0,   0       },      //0 */
/*     {0,                    _KEY_TAB, _KEY_CAPSLOCK, _KEY_F3, _KEY_T,  _KEY_Y,  _KEY_RIGHTBRACE, _KEY_F7,       _KEY_LEFTBRACE, S_KEY(_KEY_MOD_LCTRL), _KEY_BACKSPACE, 0,         0,        S_KEY(_KEY_MOD_LALT), 0, S_KEY(_KEY_MOD_LSHIFT)},  //1 */
/*     {0,                    _KEY_A,   _KEY_S,       _KEY_D,  _KEY_F,  _KEY_J,  _KEY_K, _KEY_L,  _KEY_SEMICOLON, _KEY_LEFTCTRL,  _KEY_BACKSLASH,       0,             0,         0,        0,   S_KEY(_KEY_MOD_RSHIFT), 0}, */
/*     {0,                    _KEY_ESC, 0,            _KEY_F4, _KEY_G,  _KEY_H,  _KEY_F6,         0,             _KEY_APOSTROPHE, _KEY_LEFTMETA, 0,     _KEY_SPACE, 0, 0, _KEY_UP,  0             }, */
/*     {S_KEY(_KEY_MOD_RALT), _KEY_Z,   _KEY_X,       _KEY_C,  _KEY_V,  _KEY_M,  _KEY_COMMA,      _KEY_DOT,      0,              0,                    _KEY_ENTER,    _KEY_F11,  0 }, */
/*     {0,                    0,        0,            0,       _KEY_B,  _KEY_N,  0,               0,             _KEY_SLASH,     _KEY_RIGHTMETA,       0,             _KEY_DOWN, _KEY_RIGHT, 0,   _KEY_LEFT}, */
/*     {0,                    _KEY_GRAVE, _KEY_F1,     _KEY_F2, _KEY_5,  _KEY_6,  _KEY_EQUAL,      _KEY_F8,       _KEY_MINUS,     0,                    _KEY_F9,       _KEY_DELETE}, */
/*     {_KEY_F5,              _KEY_1,   _KEY_2,       _KEY_3,  _KEY_4,  _KEY_7,  _KEY_8,          _KEY_9,        _KEY_0,         _KEY_F12,             _KEY_F10}, */
/* }; */

/*
const u16 matrix_key_table[ROW_MAX][COL_MAX] = {                //高八位用来标识是否为特殊键
//  0                        1         2               3         4       5         6                7        8                9                         10            11 12 13    14     15                            16
    {0,                     _KEY_Q,   _KEY_W,         _KEY_E,  _KEY_R,  _KEY_U,  _KEY_I,          _KEY_O,    _KEY_P,             0,                       0,          0, 0, 0,    0,   0},      //0
    {0,                     _KEY_TAB, _KEY_CAPSLOCK, _KEY_F3, _KEY_T,  _KEY_Y,  _KEY_RIGHTBRACE, _KEY_F7,   _KEY_LEFTBRACE,     0,                 _KEY_BACKSPACE,   0, 0, 0,    0,   S_KEY(_KEY_MOD_LSHIFT), S_KEY(_KEY_MOD_LALT)},
    {0,                     _KEY_A,   _KEY_S,         _KEY_D,  _KEY_F,  _KEY_J,  _KEY_K,          _KEY_L,    _KEY_SEMICOLON,  S_KEY(_KEY_MOD_LCTRL), _KEY_BACKSLASH,  0, 0, 0,    0,   S_KEY(_KEY_MOD_RSHIFT)},     //1
    {0,                     _KEY_ESC,    0,           _KEY_F4, _KEY_G,  _KEY_H,  _KEY_F6,            0,      _KEY_APOSTROPHE, _KEY_LEFTMETA,
        0,          _KEY_SPACE, 0, 0, _KEY_UP},
    {S_KEY(_KEY_MOD_RALT),  _KEY_Z,   _KEY_X,         _KEY_C,  _KEY_V,  _KEY_M,  _KEY_COMMA,      _KEY_DOT,   0,                 0,                 _KEY_ENTER,      _KEY_F11, _KEY_MINUS},
    {0,                       0,         0,             0,     _KEY_B,  _KEY_N,    0,                0,      _KEY_SLASH,     _KEY_RIGHTMETA,               0,          _KEY_DOWN,  _KEY_RIGHT, 0,   _KEY_LEFT},
    {0,                     _KEY_GRAVE, _KEY_F1,       _KEY_F2, _KEY_5,  _KEY_6,  _KEY_EQUAL,      _KEY_F8,   _KEY_MINUS,         0,                _KEY_F9,       _KEY_DELETE},
    {_KEY_F5,               _KEY_1,   _KEY_2,        _KEY_3,  _KEY_4,  _KEY_7,  _KEY_8,          _KEY_9,    _KEY_0,          _KEY_F12,             _KEY_F10},
};
*/
/*
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
};*/

#if TCFG_MATRIX_KEY_CORE == 1
special_key fn_remap_key[15 + 4 + 4 + 4] = {
    {.row = 3, .col = 1},       //ESC
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

    {.row = 2, .col = 9},      //Control 虚拟键盘
    {.row = 3, .col = 11},     //Key_Space 语言切换

    {.row = 3, .col = 14},      //↑PageUp
    {.row = 5, .col = 11},      //↓PageDn
    {.row = 5, .col = 14},      //←home
    {.row = 5, .col = 12},      //→end

    {.row = 0, .col = 1, .is_user_key = 1},       //q
    {.row = 0, .col = 2, .is_user_key = 1},       //w
    {.row = 0, .col = 3, .is_user_key = 1},       //e
    {.row = 7, .col = 1, .is_user_key = 1},       //1
    {.row = 7, .col = 2, .is_user_key = 1},       //2
    {.row = 7, .col = 3, .is_user_key = 1},       //3
    {.row = 7, .col = 4, .is_user_key = 1},       //4
};

const u16 fn_remap_event[15 + 4 + 4 + 4] = {_KEY_CUSTOM_ESC, _KEY_CUSTOM_CTRL_RETURN, _KEY_CUSTOM_CTRL_SEARCH, _KEY_CUSTOM_SELECT_ALL_TEST, _KEY_CUSTOM_COPY_TEST, \
                                            _KEY_CUSTOM_PASTE_TEST, _KEY_CUSTOM_CUT_TEST,  _KEY_CUSTOM_CTRL_BACK,  _KEY_CUSTOM_CTRL_STOP, \
                                            _KEY_CUSTOM_CTRL_FORWARD, _KEY_CUSTOM_CTRL_VOL_DOWN, _KEY_CUSTOM_CTRL_VOL_UP,  \
                                            _KEY_CUSTOM_LOCK,
                                            _KEY_VIRTUAL_KEYBBOARD, _KEY_CHANGE_LANGUAGE_TEST,
                                            _KEY_PAGEUP, _KEY_PAGEDOWN, _KEY_HOME, _KEY_END,
                                            KEYBOARD_SYSTEM_ARD, KEYBOARD_SYSTEM_WIN, KEYBOARD_SYSTEM_IOS,
                                            KEYBOARD_ENTER_PAIR0, KEYBOARD_ENTER_PAIR1, KEYBOARD_ENTER_PAIR2, KEYBOARD_ENTER_PAIR3,
                                           };
const u16 fn_remap_ios_event[15 + 4 + 4 + 4] = {_KEY_CUSTOM_ESC, _KEY_CUSTOM_CTRL_RETURN, _KEY_CUSTOM_CTRL_SEARCH, _KEY_CUSTOM_SELECT_ALL_TEST, _KEY_CUSTOM_COPY_TEST, \
                                                _KEY_CUSTOM_PASTE_TEST, _KEY_CUSTOM_CUT_TEST,  _KEY_CUSTOM_CTRL_BACK,  _KEY_CUSTOM_CTRL_STOP, \
                                                _KEY_CUSTOM_CTRL_FORWARD, _KEY_CUSTOM_CTRL_VOL_DOWN, _KEY_CUSTOM_CTRL_VOL_UP,  \
                                                _KEY_CUSTOM_LOCK,
                                                _KEY_VIRTUAL_KEYBBOARD, _KEY_CHANGE_LANGUAGE_TEST,
                                                _KEY_PAGEUP, _KEY_PAGEDOWN, _KEY_HOME, _KEY_END,
                                                KEYBOARD_SYSTEM_ARD, KEYBOARD_SYSTEM_WIN, KEYBOARD_SYSTEM_IOS,
                                                KEYBOARD_ENTER_PAIR0, KEYBOARD_ENTER_PAIR1, KEYBOARD_ENTER_PAIR2, KEYBOARD_ENTER_PAIR3,
                                               };
#elif TCFG_MATRIX_KEY_CORE == 0
special_key fn_remap_key[14 + 4 + 4] = {
    {.row = 3, .col = 1},       //ESC
    {.row = 6, .col = 2},       //F1
    {.row = 6, .col = 3},       //F2
    {.row = 1, .col = 3, .is_user_key = 1},     //F3
    {.row = 3, .col = 3},       //F4
    {.row = 7, .col = 0},       //F5
    {.row = 3, .col = 6},       //F6
    {.row = 1, .col = 7},       //F7
    {.row = 6, .col = 7},       //F8
    {.row = 6, .col = 10},      //F9
    {.row = 7, .col = 10},      //F10
    {.row = 4, .col = 11},      //F11
    {.row = 7, .col = 9},       //F12
    {.row = 6, .col = 11},       //F12
    {.row = 0, .col = 1, .is_user_key = 1},       //q
    {.row = 0, .col = 2, .is_user_key = 1},       //w
    {.row = 0, .col = 3, .is_user_key = 1},       //e
    {.row = 7, .col = 1, .is_user_key = 1},       //1
    {.row = 7, .col = 2, .is_user_key = 1},       //2
    {.row = 7, .col = 3, .is_user_key = 1},       //3
    {.row = 7, .col = 4, .is_user_key = 1},       //4
};


const u16 fn_remap_event[14 + 4 + 4] = {_KEY_CUSTOM_ESC, _KEY_CUSTOM_CTRL_HOME, _KEY_CUSTOM_CTRL_EMAIL, KEYBOARD_COMPOSE_KEY,  _KEY_CUSTOM_CTRL_CALCULATOR, \
                                        _KEY_CUSTOM_CTRL_SEARCH, _KEY_CUSTOM_SPLIT_SCREEN,    _KEY_CUSTOM_CTRL_FORWARD,  _KEY_CUSTOM_CTRL_STOP, \
                                        _KEY_CUSTOM_CTRL_BACK, _KEY_CUSTOM_CTRL_MUTE, _KEY_CUSTOM_CTRL_VOL_DOWN, _KEY_CUSTOM_CTRL_VOL_UP,                                      _KEY_CUSTOM_LOCK,
                                        KEYBOARD_SYSTEM_IOS,  KEYBOARD_SYSTEM_ARD, KEYBOARD_SYSTEM_WIN,
                                        KEYBOARD_ENTER_PAIR0, KEYBOARD_ENTER_PAIR1, KEYBOARD_ENTER_PAIR2, KEYBOARD_ENTER_PAIR3,
                                       };
const u16 fn_remap_ios_event[14 + 4 + 4] = {_KEY_CUSTOM_ESC, _KEY_BRIGHTNESS_REDUCTION,  _KEY_BRIGHTNESS_INCREASE, KEYBOARD_COMPOSE_KEY, _KEY_CUSTOM_CTRL_CALCULATOR, \
                                            _KEY_CUSTOM_CTRL_SEARCH, _KEY_CUSTOM_SPLIT_SCREEN,  _KEY_CUSTOM_CTRL_FORWARD,  _KEY_CUSTOM_CTRL_STOP, \
                                            _KEY_CUSTOM_CTRL_BACK, _KEY_CUSTOM_CTRL_MUTE, _KEY_CUSTOM_CTRL_VOL_DOWN, _KEY_CUSTOM_CTRL_VOL_UP,  \
                                            _KEY_CUSTOM_LOCK,
                                            KEYBOARD_SYSTEM_IOS,  KEYBOARD_SYSTEM_ARD, KEYBOARD_SYSTEM_WIN,
                                            KEYBOARD_ENTER_PAIR0, KEYBOARD_ENTER_PAIR1, KEYBOARD_ENTER_PAIR2, KEYBOARD_ENTER_PAIR3,
                                           };
/*
const u16 fn_remap_event[12 + 4] = {_KEY_CUSTOM_CTRL_HOME, _KEY_CUSTOM_CTRL_EMAIL, _KEY_CUSTOM_CTRL_SEARCH, _KEY_CUSTOM_CTRL_CALCULATOR, \
                                    _KEY_CUSTOM_CTRL_MUSIC, _KEY_CUSTOM_CTRL_BACK, _KEY_CUSTOM_CTRL_STOP, _KEY_CUSTOM_CTRL_FORWARD, \
                                    _KEY_CUSTOM_CTRL_MUTE, _KEY_CUSTOM_CTRL_VOL_DOWN, _KEY_CUSTOM_CTRL_VOL_UP, 0, \
                                    KEYBOARD_ENTER_PAIR0, KEYBOARD_ENTER_PAIR1, KEYBOARD_ENTER_PAIR2, KEYBOARD_ENTER_PAIR3,
                                   };*/
#endif
#endif


static special_key other_key[] = {
    {.row = 0, .col = 18, .is_user_key = 1},
};

static const u16 other_key_map[] = {KEYBOARD_ENTER_PAIR0};
static u16 key_status_array[6] = {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff};
static u16 remap_event = 0;

static int user_key_timer_hdl = 0;

typedef enum {
    HID_MODE_NULL = 0,
    HID_MODE_EDR = BIT(0),
    HID_MODE_BLE = BIT(1),
    HID_MODE_COMBO = (BIT(0) | BIT(1)),
    HID_MODE_BLE_24G = BIT(2),
    HID_MODE_INIT = 0xff
} bt_mode_e;
static bt_mode_e bt_hid_mode = HID_MODE_NULL;

static volatile u8 is_stdkb_active = 0;//1-临界点,系统不允许进入低功耗，0-系统可以进入低功耗

#define KEYBOARD_REPORT_ID          0x4//默认report_data修改为4/5/6
#define COUSTOM_CONTROL_REPORT_ID   0x5
#define MOUSE_POINT_REPORT_ID       0x6


const static u8  kb_hid_report_map[] = {
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


static void kb_24g_mode_set(u32 code_id)
{
#if TCFG_USER_BLE_ENABLE
    log_info("%s:%02x", __FUNCTION__, code_id);
    r_printf("%s:%02x", __FUNCTION__, code_id);
    rf_set_24g_hackable_coded(code_id);
#endif
}


//==========================================================
#define SNIFF_MODE_TYPE               SNIFF_MODE_ANCHOR
#define SNIFF_CNT_TIME                1/////<空闲5S之后进入sniff模式

#define SNIFF_MAX_INTERVALSLOT        48
#define SNIFF_MIN_INTERVALSLOT        48
#define SNIFF_ATTEMPT_SLOT            2
#define SNIFF_TIMEOUT_SLOT            1
#define SNIFF_CHECK_TIMER_PERIOD      1000


//默认配置
static const edr_sniff_par_t hidkey_sniff_param = {
    .sniff_mode = SNIFF_MODE_TYPE,
    .cnt_time = SNIFF_CNT_TIME,
    .max_interval_slots = SNIFF_MAX_INTERVALSLOT,
    .min_interval_slots = SNIFF_MIN_INTERVALSLOT,
    .attempt_slots = SNIFF_ATTEMPT_SLOT,
    .timeout_slots = SNIFF_TIMEOUT_SLOT,
    .check_timer_period = SNIFF_CHECK_TIMER_PERIOD,
};


//----------------------------------
static const edr_init_cfg_t hidkey_edr_config = {
    .page_timeout = 8000,
    .super_timeout = 8000,

#if SUPPORT_USER_PASSKEY
    .io_capabilities = 2,
    .passkey_enable = 1,
#else
    .io_capabilities = 3,
    .passkey_enable = 0,
#endif

    .authentication_req = 2,
    .oob_data = 0,
    .sniff_param = &hidkey_sniff_param,
    .class_type = BD_CLASS_KEYBOARD,
    .report_map = kb_hid_report_map,
    .report_map_size = sizeof(kb_hid_report_map),
};

//----------------------------------
static const ble_init_cfg_t standard_keyboard_ble_config = {
    .same_address = 0,
    .appearance = BLE_APPEARANCE_HID_KEYBOARD,
    .report_map = kb_hid_report_map,
    .report_map_size = sizeof(kb_hid_report_map),
};

//==========================================================

#define LED_ON()  gpio_direction_output(CONNECT_LED_PIN,1);
#define LED_OFF() gpio_direction_output(CONNECT_LED_PIN,0);
//==========================================================
//上电亮灯
extern int rtc_port_pr_out(u8 port, bool on);

static void power_led_on_handler(void *arg)
{
    rtc_port_pr_out(1, 0);
}

static void power_led_set(u8 en)
{
    rtc_port_pr_out(1, en);
    if (en) {

        sys_timeout_add(NULL, power_led_on_handler, 3000);
    }
}
//================================================================
//连接闪灯
enum {
    LED_WAIT_CONNECT,
    LED_CLOSE,
};

static u8 led_io_flash;
static u32 led_timer_id = 0;
static u32 led_timer_ms;
static u32 led_timeout_count;


static void led_timer_stop(void);
static void led_timer_start(u32 time_ms);
static void led_on_off(u8 state, u8 res);

#define SOFT_OFF_TIME_MS  (90000L)

static void led_on_off(u8 state, u8 res)
{
    led_io_flash = 0;

    switch (state) {
    case LED_WAIT_CONNECT:
        led_timeout_count = (SOFT_OFF_TIME_MS / 1000) * 2;
        led_timer_start(500);
        LED_ON();
        led_io_flash = BIT(7) | BIT(0);
        break;
    case LED_CLOSE:
        LED_OFF();
        led_timer_stop();
        break;
    }
}

static void led_timer_handle(void)
{
    if (led_io_flash & BIT(7)) {
        led_io_flash ^= BIT(0);
        if (led_io_flash & BIT(0)) {
            LED_ON();
        } else {
            LED_OFF();
        }
    }
}


static void led_timer_stop(void)
{
    if (led_timer_id) {
        sys_timer_del(led_timer_id);
        led_timer_id = 0;
    }
}

static void led_timer_start(u32 time_ms)
{
    led_timer_stop(); //stop firstly
    led_timer_ms = time_ms;
    led_timer_id = sys_timer_add(0, led_timer_handle, led_timer_ms);
}


static void bt_close_pair(void *arg)
{
    led_on_off(LED_CLOSE, 0);
}

static void bt_keyboard_enter_pair_deal(void)
{
    if (bt_close_pair_handler) {
        sys_timeout_del(bt_close_pair_handler);
    }
    bt_close_pair_handler = sys_timeout_add(NULL, bt_close_pair, 1000 * 60 * 3);
    led_on_off(LED_WAIT_CONNECT, 0);
}



//+============================================================

static void edr_set_enable(u8 en)
{
    log_info("edr enable :%d\n", en);
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

void set_edr_reconnect_flag(u8 recon_flag)
{
    reconnect_flag = recon_flag;
}

u8 get_edr_reconnect_flag()
{
    return reconnect_flag;
}
void edr_reconnect()
{
    if (get_curr_channel_state()) {
        log_info("no reconn have channel state");
        edr_operation = EDR_OPERATION_RECONN;
    } else {
        log_info("reconnect edr at once... %d \n", cur_bt_idx);
        put_buf(remote_addr, 6);
        set_edr_reconnect_flag(0);
        user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, remote_addr);
    }
}

static void ble_reset_addr(u8 *new_addr)
{
#if TCFG_USER_BLE_ENABLE
    uint8_t irk[16];

    syscfg_read(CFG_BLE_IRK_NUMBER + cur_bt_idx, irk, 16);//设置对应通道的irk
    sm_test_set_irk(irk);
    put_buf(irk, 16);

    ble_module_enable(0);
    log_info("new ble address:");
    put_buf(new_addr, 6);
    le_controller_set_mac((void *)new_addr);
    if (get_key_timeout_flag()) {
        call_hogp_adv_config_set();
        set_key_timeout_flag(0);
    } else {
#if SUPPORT_RECONN_ADV_OR_DIRECT
        hogp_reconnect_adv_config_set(ADV_DIRECT_IND, 5000);
#else
        hogp_reconnect_adv_config_set(ADV_IND, 5000);
#endif
    }
    ble_module_enable(1);
#endif
}

static u8 keyboard_report[8] = {0};

u8 get_cur_bt_idx()
{
    return cur_bt_idx ;
}
u8 get_key_timeout_flag()
{
    return key_timeout_flag ;
}
static void set_key_timeout_flag(u8 kt_flag)
{
    key_timeout_flag = kt_flag ;
}

static void user_key_deal(u16 user_key, u8 timeout)
{
    int ret = 0;
    int switch_flag = 0;
    u8 tmp_ble_addr[6];
    u8 zero_addr[6] = {0};
    u8 vm_hid_mode = HID_MODE_NULL;
    u8 user_event = user_key & 0xff;

    switch (user_event) {

    case KEYBOARD_COMPOSE_KEY:
        keyboard_report[2] = 0x65;
        hid_report_send(KEYBOARD_REPORT_ID, keyboard_report, 8);
        break;
    case KEYBOARD_SYSTEM_IOS:
        keyboard_system = SYSTEM_IOS;
        log_info("sys select ios\n");
        break;
    case KEYBOARD_SYSTEM_WIN:
        log_info("sys select win\n");
        keyboard_system = SYSTEM_WIN;
        break;
    case KEYBOARD_SYSTEM_ARD:
        keyboard_system = SYSTEM_ARD;
        log_info("sys select ard\n");
        break;
    case KEYBOARD_ENTER_PAIR0:
    case KEYBOARD_ENTER_PAIR1:
    case KEYBOARD_ENTER_PAIR2:
    case KEYBOARD_ENTER_PAIR3:
#if TCFG_USER_BLE_ENABLE
        ble_state_e state = ble_gatt_server_get_work_state();
#endif
        log_info("user_event:0x%x\n", user_event);
        user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);//开启page之前关闭page
        syscfg_read(CFG_CUR_BT_IDX, &cur_bt_idx, 1);
        if ((cur_bt_idx == user_event) && !timeout) {
#if TCFG_USER_EDR_ENABLE
            if (edr_hid_is_connected()) {                   //短按且HID连接不响应按键
                printf("is_connect_so_to_return");
                return;
            }
#endif

#if TCFG_USER_BLE_ENABLE
            if (ble_hid_is_connected()) {
                return;
            }
#endif
        }

        log_info(">>>bt_hid_mode:%d\n", bt_hid_mode);
        switch (bt_hid_mode) {                                                  //根据连接模式断开当前连接
#if TCFG_USER_EDR_ENABLE
        case HID_MODE_EDR:
            if (edr_hid_is_connected()) {
                switch_flag = 1;//确定是断链切换还是非短链切换
                user_send_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL);
            }
            break;
#endif
#if TCFG_USER_BLE_ENABLE
        case HID_MODE_BLE:
        case HID_MODE_BLE_24G:
            if (ble_hid_is_connected()) {
                //安卓手机在update instant区间内发送disconnect命令不响应,提前设置timeout
                ll_vendor_change_local_supervision_timeout(ble_hid_is_connected(), 500);
                ble_module_enable(0);
                kb_24g_mode_set(0);
            }
            break;
#endif
        case HID_MODE_NULL:
            break;
        }
        /* log_info("finish disconnected.....\n"); */
        bt_keyboard_enter_pair_deal();
        cur_bt_idx = user_event;
        syscfg_write(CFG_CUR_BT_IDX, &cur_bt_idx, 1);                           //记录当前使用哪组蓝牙

        if (timeout) {                                                          //如果是长按超时直接打开3.0 和 4.0 并清除VM信息
            log_info("timeout\n");
            vm_hid_mode = HID_MODE_COMBO;
            syscfg_write(CFG_HID_MODE_BEGIN + cur_bt_idx, &vm_hid_mode, 1);
#if TCFG_USER_EDR_ENABLE
            if (syscfg_read(CFG_EDR_ADDRESS_BEGIN + cur_bt_idx, remote_addr, 6) == 6) {
                delete_link_key(remote_addr, get_remote_dev_info_index());
                syscfg_write(CFG_EDR_ADDRESS_BEGIN + cur_bt_idx, zero_addr, 6); //清空EDR回连地址
            }
            vm_address_change_flag = 0;//清除配对信息允许覆盖vm
            edr_set_enable(1);
            if (get_curr_channel_state()) {
                /* log_info("edr operation pagscan...\n"); */
                edr_operation = EDR_OPERATION_PAGESCAN_IRQUIRY_SCAN;
            }
#endif
#if TCFG_USER_BLE_ENABLE
            get_random_number(tmp_ble_addr, 6);                             //重新生成BLE地址
            syscfg_write(CFG_BLE_ADDRESS_BEGIN + cur_bt_idx, tmp_ble_addr, 6);
            vm_hid_mode = HID_MODE_NULL;
            syscfg_write(CFG_HID_MODE_BEGIN + cur_bt_idx, &vm_hid_mode, 1);

            if (cur_bt_idx == CFG_RF_24G_CODE_CHANNEL) {
                kb_24g_mode_set(CFG_RF_24G_CODE_ID);
            }

            if (state != BLE_ST_IDLE && state != BLE_ST_DISCONN && state != BLE_ST_INIT_OK) {
                ble_gatt_server_adv_enable(0);
            }
            ble_reset_addr(tmp_ble_addr);
#endif
            return;
        }

        ret = syscfg_read(CFG_HID_MODE_BEGIN + cur_bt_idx, &vm_hid_mode, 1);    //读取HID模式
        log_info("ret = %d vm_hid_mode:%d\n", ret, vm_hid_mode);

#if  TCFG_USER_EDR_ENABLE
        user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);//开启page之前关闭page
        if ((ret == 1) && (vm_hid_mode == HID_MODE_EDR)) {
            //last conn is edr
            log_info("last connected is edr .... :%d\n", switch_flag);
            edr_set_enable(1);
            ret = syscfg_read(CFG_EDR_ADDRESS_BEGIN + cur_bt_idx, remote_addr, 6);
            if ((ret == 6) && memcmp(zero_addr, remote_addr, 6)) {              //read remote addr error
                log_info("remote_addr:0x%x:");
                put_buf(remote_addr, 6);
                if (switch_flag == 1) {
                    set_edr_reconnect_flag(1);//断链切换使用
                } else {
                    edr_reconnect();//非短链切换使用(避免切换别的通道不连接切换回来不回连)
                    set_edr_reconnect_flag(0);
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
            if (state != BLE_ST_IDLE && state != BLE_ST_DISCONN && state != BLE_ST_INIT_OK) {
                ble_gatt_server_adv_enable(0);
            }

            ble_reset_addr(tmp_ble_addr);
            return;
        } else if ((ret == 1) && (vm_hid_mode == HID_MODE_BLE_24G)) {
            g_printf("sw ble_24g...\n");
            kb_24g_mode_set(CFG_RF_24G_CODE_ID);
            if (syscfg_read(CFG_BLE_ADDRESS_BEGIN + cur_bt_idx, tmp_ble_addr, 6) != 6) {       //read current index ble address error
                get_random_number(tmp_ble_addr, 6);
                syscfg_write(CFG_BLE_ADDRESS_BEGIN + cur_bt_idx, tmp_ble_addr, 6);
            }
            if (state != BLE_ST_IDLE && state != BLE_ST_DISCONN && state != BLE_ST_INIT_OK) {
                ble_gatt_server_adv_enable(0);
            }
            ble_reset_addr(tmp_ble_addr);
            return;
        }
#endif

        bt_keyboard_enter_pair_deal();
        //last conn is null
        /* g_printf("last conn is null\n"); */
#if TCFG_USER_EDR_ENABLE
        edr_set_enable(1);
        if (get_curr_channel_state()) {
            log_info("edr operation pagscan...\n");
            edr_operation = EDR_OPERATION_PAGESCAN_IRQUIRY_SCAN;
        }
#endif

#if TCFG_USER_BLE_ENABLE
        if (syscfg_read(CFG_BLE_ADDRESS_BEGIN + cur_bt_idx, tmp_ble_addr, 6) != 6) {       //read current index ble address error
            get_random_number(tmp_ble_addr, 6);
            syscfg_write(CFG_BLE_ADDRESS_BEGIN + cur_bt_idx, tmp_ble_addr, 6);
        }
        if (cur_bt_idx == CFG_RF_24G_CODE_CHANNEL) {

            r_printf("24G_mode_set\n");
            kb_24g_mode_set(CFG_RF_24G_CODE_ID);
            /* ble_module_enable(1); */
        } else {
            r_printf("24G_mode_reset\n");
            kb_24g_mode_set(0);
        }
        set_key_timeout_flag(1);
        ble_reset_addr(tmp_ble_addr);
#endif
        break;
    }
}

static void hid_report_send(u8 report_id, u8 *data, u16 len)
{
    if (bt_hid_mode == HID_MODE_EDR) {
#if TCFG_USER_EDR_ENABLE
        if (edr_hid_is_connected()) {
            edr_hid_data_send(report_id, data, len);
        }
#endif
    } else {
#if TCFG_USER_BLE_ENABLE
        if (ble_hid_is_connected()) {
            ble_hid_data_send(report_id, data, len);
        }
#endif

#if (TCFG_PC_ENABLE )
        u8 packet[9];
        packet[0] = KEYBOARD_REPORT_ID;
        memcpy(&packet[1], data, len);
        put_buf(packet, sizeof(packet));
        hid_send_data(packet, sizeof(packet));
#endif
    }
}

static void bt_send_keypress(u8 key)
{
    user_send_cmd_prepare(USER_CTRL_KEYPRESS, 1, &key);
}

static void send_matrix_key_report(u16 *key)
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
                log_info("enter key:%d\n", key_value - _KEY_1 + 1);
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

static void Phantomkey_process(void)
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
            //log_info("%x %x\n", key_status_array[i], key_status_array[i + 1]);
            for (j = 0; j < sizeof(key_status_array) / sizeof(u16); j++) {
                //寻找是否有相邻row的按键，如果有就是幻影键
                if ((j != i) && (j != i + 1) && ((key_status_array[i] & 0xff) == (key_status_array[j] & 0xff))) {           //寻找与这两个键row相同的按键
                    //log_info("Phantomkey mark0...\n");
                    memset(key_status_array, 0xff, sizeof(key_status_array));
                    return;
                }
                if ((j != i) && (j != i + 1) && ((key_status_array[i + 1] & 0xff) == (key_status_array[j] & 0xff))) {
                    //log_info("Phantomkey mark1...\n");
                    memset(key_status_array, 0xff, sizeof(key_status_array));
                    return;
                }
            }
        }
    }
}

#include "asm/power/p33.h"
//void p33_tx_1byte(u16 addr, u8 data0);
static void full_key_array(u8 row, u8 col, u8 st)
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

static void user_key_timeout(void *priv)
{
    user_key_deal(remap_event, 1);
    user_key_timer_hdl = 0;
}

static u8 select_send_report_id = 0;
static u8 special_key_deal(u8 *map, special_key *key_tab, u8 key_tab_size, u16 *remap_event_tab, u8 is_fn_remap_deal)
{
    static u8 special_key_press = 0;
    static u16 last_user_key = 0xff;
    static u8 fn_contral_key[] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
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
                    log_info("last_event:0x%x\n", remap_event);
                    if (remap_event == _KEY_CUSTOM_SELECT_ALL_TEST || remap_event == _KEY_CUSTOM_COPY_TEST || remap_event == _KEY_CUSTOM_PASTE_TEST || \
                        remap_event == _KEY_CUSTOM_CUT_TEST || remap_event == _KEY_CHANGE_LANGUAGE_TEST || remap_event == _KEY_PAGEUP || remap_event == _KEY_PAGEDOWN \
                        || remap_event == _KEY_HOME || remap_event == _KEY_END) {
                        fn_contral_key[2] = remap_event / 256;
                        fn_contral_key[3] = remap_event % 256;
                        hid_report_send(KEYBOARD_REPORT_ID, &fn_contral_key, 8);
                        select_send_report_id = 1;
                    } else {
                        log_info("last_event_test:%d\n", select_send_report_id);
                        if (select_send_report_id == 1) {
                            hid_report_send(KEYBOARD_REPORT_ID, &remap_event, 8);
                            select_send_report_id = 0;
                        } else {
                            hid_report_send(COUSTOM_CONTROL_REPORT_ID, &remap_event, 2);
                        }
                    }
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

        log_info("last_event_test:%d\n", select_send_report_id);
        if (select_send_report_id == 1) {
            memset(&fn_contral_key[1], 0, 7);
            hid_report_send(KEYBOARD_REPORT_ID, &fn_contral_key, 8);
            select_send_report_id = 0;
        } else {
            hid_report_send(COUSTOM_CONTROL_REPORT_ID, &remap_event, 2);
        }
        last_user_key = 0xff;
        return 1;
    }
    return 0;
}


static void matrix_key_map_deal(u8 *map)
{
    u8 row, col, i = 0;
    static u8 fn_press = 0;

    if (keyboard_system == SYSTEM_ARD) {
        if (special_key_deal(map, fn_remap_key, sizeof(fn_remap_key) / sizeof(special_key), fn_remap_event, 1)) {
            /* log_info("fn mark...\n"); */
            return;
        }
    }
    if (keyboard_system == SYSTEM_IOS || keyboard_system == SYSTEM_WIN) {
        if (special_key_deal(map, fn_remap_key, sizeof(fn_remap_key) / sizeof(special_key), fn_remap_ios_event, 1)) {
            /* log_info("fn mark..ios.\n"); */
            return;
        }
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

static void stdkb_touch_pad_event_deal(struct sys_event *event)
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

static void stdkb_auto_shutdown_disable(void)
{
    log_info("----%s", __FUNCTION__);
    if (g_auto_shutdown_timer) {
        sys_timeout_del(g_auto_shutdown_timer);
    }
}

void stdkb_power_event_to_user(u8 event)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = (void *)DEVICE_EVENT_FROM_POWER;
    e.u.dev.event = event;
    e.u.dev.value = 0;
    sys_event_notify(&e);
}

static void stdkb_set_soft_poweroff(void)
{
    log_info("%s\n", __FUNCTION__);

    led_on_off(LED_CLOSE, 0);//关闭led闪烁定时器
    if (bt_close_pair_handler) {
        sys_timeout_del(bt_close_pair_handler);//关闭配对定时器
        bt_close_pair_handler = 0;
    }

    EX_MCU_ENTER_POWEROFF();//通知从机进入poweroff
    is_stdkb_active = 1;

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


static void stdkb_app_start()
{
    log_info("=======================================");
    log_info("--------standard keyboard--------------");
    log_info("=======================================");
    log_info("app_file: %s", __FILE__);

    clk_set("sys", BT_NORMAL_HZ);

#if(TCFG_PC_ENABLE)
    r_printf("usb_start\n");
    is_stdkb_active = 1;
    usb_connect_ok = 0;
    void usb_start();
    void usb_hid_set_repport_map(const u8 * map, int size);
    void usb_hid_set_output_callback(void *cb);
    usb_hid_register_state_callback(stdkb_usb_state_callback);
    usb_hid_set_repport_map(kb_hid_report_map, sizeof(kb_hid_report_map));
    usb_hid_set_output_callback(stdkb_usb_led_status_callback);
    usb_start();
#endif

#if(TCFG_USER_BLE_ENABLE)
    set_multi_devices_adv_flag(1);
#endif
    //有蓝牙
#if (TCFG_USER_EDR_ENABLE || TCFG_USER_BLE_ENABLE)
    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);

    btstack_init_ok = 0;
    u8 edr_ex_name[] = "";
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

#if TCFG_USER_EDR_ENABLE
    btstack_edr_start_before_init(&hidkey_edr_config, 0);
    //change init
    user_hid_init(stdkb_edr_led_status_callback);
#endif

    if (syscfg_read(CFG_CUR_BT_IDX, &cur_bt_idx, 1) != 1) {
        cur_bt_idx = 0;
        syscfg_write(CFG_CUR_BT_IDX, &cur_bt_idx, 1);
    }
    log_info("use cur_ble_idx:%d\n", cur_bt_idx);

#if TCFG_USER_BLE_ENABLE
    btstack_ble_start_before_init(&standard_keyboard_ble_config, 0);
    {
        u8 tmp_ble_addr[6];

        if (syscfg_read(CFG_BLE_ADDRESS_BEGIN + cur_bt_idx, tmp_ble_addr, 6) != 6) {       //read err
            get_random_number(tmp_ble_addr, 6);
            syscfg_write(CFG_BLE_ADDRESS_BEGIN + cur_bt_idx, tmp_ble_addr, 6);
        }
        le_controller_set_mac((void *)tmp_ble_addr);

        log_info("\n-----change ble 's address-----");
        printf_buf((void *)tmp_ble_addr, 6);
    }
#endif
    btstack_init();

#else
    //no bt,to for test
    log_info("no bt!!!!!!");
#endif

    /* 按键消息使能 */
    sys_key_event_enable();

#if (TCFG_HID_AUTO_SHUTDOWN_TIME)
    //无操作定时软关机
    g_auto_shutdown_timer = sys_timeout_add((void *)POWER_EVENT_POWER_SOFTOFF, stdkb_power_event_to_user, TCFG_HID_AUTO_SHUTDOWN_TIME * 1000);
#endif
}

static int stdkb_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        if (!it) {
            break;
        }
        switch (it->action) {
        case ACTION_STANDARD_KEYBOARD:
            stdkb_app_start();
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


static void stdkb_usb_state_callback(u8 state)
{
    log_info("usb_state= %d\n", state);
    if (state == 1) {
        log_info("usb connect sucesss");
        usb_connect_ok = 1;

        if (btstack_init_ok) {
#if TCFG_USER_BLE_ENABLE
            ble_module_enable(0);
#endif
#if TCFG_USER_EDR_ENABLE
            edr_set_enable(0);
#endif
        }
    }
}

static void stdkb_hogp_ble_status_callback(ble_state_e status, u8 reason)
{
    log_info("----%s reason %x %x", __FUNCTION__, status, reason);

    switch (status) {
    case BLE_ST_IDLE:
        break;
    case BLE_ST_ADV:
        break;
    case BLE_ST_CONNECT:
        is_stdkb_active = 0;
        if (cur_bt_idx == CFG_RF_24G_CODE_CHANNEL) {
            r_printf("write 24g mode\n");
            bt_hid_mode = HID_MODE_BLE_24G;
            syscfg_write(CFG_HID_MODE_BEGIN + cur_bt_idx, &bt_hid_mode, 1);
            stdkb_keyboard_irk_init(cur_bt_idx);//如果是第一次上电把所有通道irk设置完毕并将通道0的irk地址设置好

        } else {
            r_printf("write ble mode\n");
            bt_hid_mode = HID_MODE_BLE;
            syscfg_write(CFG_HID_MODE_BEGIN + cur_bt_idx, &bt_hid_mode, 1);         //保存当前HID模式
            stdkb_keyboard_irk_init(cur_bt_idx);//如果是第一次上电把所有通道irk设置完毕并将通道0的irk地址设置好

        }

#if TCFG_USER_EDR_ENABLE
        edr_set_enable(0);
#endif      //TCFG_USER_EDR_MODE
        if (bt_close_pair_handler) {
            led_on_off(LED_CLOSE, 0);
            sys_timeout_del(bt_close_pair_handler);
            bt_close_pair_handler = 0;
        }
        break;
    case BLE_ST_SEND_DISCONN:
        break;
    case BLE_ST_DISCONN:
        if (reason == ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST) {
            log_info("BLE_ST_DISCONN BY LOCAL...\n");
#if TCFG_USER_EDR_ENABLE
            if (edr_operation == EDR_OPERATION_PAGESCAN_IRQUIRY_SCAN) {

            } else if (edr_operation == EDR_OPERATION_RECONN) {
                log_info("reconn edr...\n");
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

static void stdkb_bt_hci_event_disconnect(struct bt_event *bt)
{
#if TCFG_USER_EDR_ENABLE
    log_info("bt->value:0x%x\n", bt->value);
    if (edr_operation == EDR_OPERATION_PAGESCAN_IRQUIRY_SCAN) {
        log_info("EDR_OPERATION_PAGESCAN_IRQUIRY_SCAN...\n");
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
#endif
}


static int stdkb_bt_hci_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_info("----%s reason %x %x", __FUNCTION__, bt->event, bt->value);

#if TCFG_USER_EDR_ENABLE
    //键盘过滤掉app_common.c中的error_code_connection_timeout
    if (bt->event == HCI_EVENT_CONNECTION_COMPLETE && bt->value == ERROR_CODE_CONNECTION_TIMEOUT) {
        if (!edr_hid_is_connected()) {
            edr_reconnect();
        }
    } else {
        bt_comm_edr_hci_event_handler(bt);
    }
#endif

#if TCFG_USER_BLE_ENABLE
    bt_comm_ble_hci_event_handler(bt);
#endif

    switch (bt->event) {
    case HCI_EVENT_USER_PASSKEY_REQUEST:
        log_info(" HCI_EVENT_USER_PASSKEY_REQUEST \n");
        key_pass_enter = 1;
        ///<可以开始输入6位passkey
        break;
    case HCI_EVENT_VENDOR_NO_RECONN_ADDR :
        log_info("HCI_EVENT_VENDOR_NO_RECONN_ADDR \n");
        stdkb_bt_hci_event_disconnect(bt) ;
        break;

    case HCI_EVENT_DISCONNECTION_COMPLETE :
        log_info("HCI_EVENT_DISCONNECTION_COMPLETE \n");
        if (get_edr_reconnect_flag()) {
            edr_reconnect();
            set_edr_reconnect_flag(0);
        }
        break;

    case BTSTACK_EVENT_HCI_CONNECTIONS_DELETE:
        log_info(" BTSTACK_EVENT_HCI_CONNECTIONS_DELETE \n");
        if (bt->value == ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION) {
            vm_address_change_flag = 0;//手机主动断开此链路允许覆盖vm地址
        }
        break;

    case HCI_EVENT_CONNECTION_COMPLETE:
        log_info(" HCI_EVENT_CONNECTION_COMPLETE \n");
        /* led_on_off(LED_CLOSE,0);  */
        if (bt->value == ERROR_CODE_PAGE_TIMEOUT) {
            vm_address_change_flag = 0;//page超时允许vm覆盖
        }
        is_stdkb_active = 0;
        if (bt_close_pair_handler) {
            led_on_off(LED_CLOSE, 0);
            sys_timeout_del(bt_close_pair_handler);
            bt_close_pair_handler = 0;
        }
        break;

    default:
        break;
    }

    return 0;
}

static void stdkb_usb_led_status_callback(u8 *buffer, u16 len)
{
    log_info("%s,len=%d", __FUNCTION__, len);
    put_buf(buffer, len);
    if (buffer[1] & BIT(1)) {   //CAP灯的状态
        log_info("enter usb led callback......on");
        /* gpio_set_output_value(CAP_LED_PIN, CAP_LED_ON_VALUE); */
        gpio_direction_output(CAP_LED_PIN, CAP_LED_ON_VALUE);
    } else {
        log_info("enter usb led callback......off");
        /* gpio_set_output_value(CAP_LED_PIN, !CAP_LED_ON_VALUE); */
        gpio_direction_output(CAP_LED_PIN, !CAP_LED_ON_VALUE);
    }
}

static void stdkb_ble_led_status_callback(u8 *buffer, u16 len)
{
    log_info("%s,len=%d", __FUNCTION__, len);
    put_buf(buffer, len);
    if (buffer[0] & BIT(1)) {   //CAP灯的状态
        r_printf("enter bble callback......\n");
        /* gpio_set_output_value(CAP_LED_PIN, CAP_LED_ON_VALUE); */
        gpio_direction_output(CAP_LED_PIN, CAP_LED_ON_VALUE);
    } else {
        /* gpio_set_output_value(CAP_LED_PIN, !CAP_LED_ON_VALUE); */
        gpio_direction_output(CAP_LED_PIN, !CAP_LED_ON_VALUE);
    }
}

static void stdkb_edr_led_status_callback(u8 *buffer, u16 len, u16 channel)
{
    log_info("%s,chl=%d,len=%d", __FUNCTION__, channel, len);
    put_buf(buffer, len);

    if (buffer[0] == 0xA2 || buffer[0] == 0x52) {    //SET_REPORT && Output
        if (buffer[1] ==  KEYBOARD_REPORT_ID) {
            if (buffer[2] & BIT(1)) {   //CAP灯的状态
                /* gpio_set_output_value(CAP_LED_PIN, CAP_LED_ON_VALUE); */
                log_info("CAP_LED ON");
                gpio_direction_output(CAP_LED_PIN, CAP_LED_ON_VALUE);
            } else {
                /* gpio_set_output_value(CAP_LED_PIN, !CAP_LED_ON_VALUE); */
                log_info("CAP_LED OFF");
                gpio_direction_output(CAP_LED_PIN, !CAP_LED_ON_VALUE);
            }
        }
    }
}

#define STAND_KEYBOARD_CHANNEL      4
static void stdkb_keyboard_irk_init(u8 bt_idx)
{
    uint8_t irk[16];
    u8 i;

    int ret = syscfg_read(CFG_BLE_IRK_NUMBER + i, irk, 16);//检查第一个通道irk是否存在

    if (ret < 0) {

        for (i = 0; i < STAND_KEYBOARD_CHANNEL; i++) {
            get_random_number(irk, 16);
            syscfg_write(CFG_BLE_IRK_NUMBER + i, irk, 16);
        }

        syscfg_read(CFG_BLE_IRK_NUMBER + bt_idx, irk, 16);
        sm_test_set_irk(irk);
        log_info("stdkb_keyboard_irk_init: %d", bt_idx);
        put_buf(irk, 16);

    } else {

        syscfg_read(CFG_BLE_IRK_NUMBER + bt_idx, irk, 16);
        sm_test_set_irk(irk);
        log_info("stdkb_keyboard_irk_init: %d", bt_idx);
        put_buf(irk, 16);

    }
}

void le_hogp_set_output_callback(void *cb);
static void stdkb_keyboard_mode_init(u8 hid_mode)
{
    power_led_set(1);

    /* le_hogp_set_output_callback(stdkb_ble_led_status_callback); */
    u8 vm_hid_mode = HID_MODE_NULL;

#if TCFG_USER_BLE_ENABLE
    le_hogp_set_output_callback(stdkb_ble_led_status_callback);
    if (ble_hid_is_connected()) {
    }
#endif

#if TCFG_USER_EDR_ENABLE
    user_hid_init(stdkb_edr_led_status_callback);
#endif

    if ((!STACK_MODULES_IS_SUPPORT(BT_BTSTACK_LE) || !BT_MODULES_IS_SUPPORT(BT_MODULE_LE)) && (!STACK_MODULES_IS_SUPPORT(BT_BTSTACK_CLASSIC) || !BT_MODULES_IS_SUPPORT(BT_MODULE_CLASSIC))) {
        log_info("not surpport ble or edr,make sure config !!!\n");
        ASSERT(0);
    }

    if (syscfg_read(CFG_HID_MODE_BEGIN + cur_bt_idx, &vm_hid_mode, 1) == 1) {  //读取HID模式
        hid_mode = vm_hid_mode;
    }

    if (hid_mode == HID_MODE_NULL) {
        r_printf("first power on mode is null\n");
#if TCFG_USER_EDR_ENABLE
        edr_set_enable(0);
#endif

#if TCFG_USER_BLE_ENABLE
        ble_module_enable(0);
#endif
        return ;
    }


#if TCFG_USER_EDR_ENABLE
    if (hid_mode & HID_MODE_EDR) {
        log_info("HID EDR MODE INIT\n");
        user_hid_enable(1);
        //读取对应通道是否存在地址
        syscfg_read(CFG_CUR_BT_IDX, &cur_bt_idx, 1);
        if (syscfg_read(CFG_EDR_ADDRESS_BEGIN + cur_bt_idx, remote_addr, 6) == 6) {
            edr_reconnect();
        } else {
            if (!bt_connect_phone_back_start()) {
                bt_wait_phone_connect_control(1);
            }
        }

    }
#endif

#if TCFG_USER_BLE_ENABLE
    if (hid_mode & HID_MODE_BLE || (hid_mode & HID_MODE_BLE_24G)) {
        log_info("HID BLE MODE INIT\n");
        u8 ble_ex_name[] = " 5.0";

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
        if (hid_mode & HID_MODE_BLE_24G) {
            kb_24g_mode_set(CFG_RF_24G_CODE_ID);
        }

        ble_module_enable(1);
    }
#endif
}

static int stdkb_bt_connction_status_event_handler(struct bt_event *bt)
{
    int ret = 0;
    log_info("----%s %d", __FUNCTION__, bt->event);


    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        /*
         * 蓝牙初始化完成
         */
        log_info("BT_STATUS_INIT_OK\n");

        if (usb_connect_ok != 1) {
            btstack_init_ok = 1;
#if TCFG_USER_BLE_ENABLE
#if !SUPPORT_RECONN_ADV_OR_DIRECT
            le_hogp_set_reconnect_adv_cfg(ADV_IND, 5000);//第一次上电使用无定向回连
#endif
            btstack_ble_start_after_init(0);
#endif

#if TCFG_USER_EDR_ENABLE
            btstack_edr_start_after_init(0);
#endif
            //hid_vm_deal(0);//bt_hid_mode read for VM
            stdkb_keyboard_mode_init(HID_MODE_NULL);
        }
        break;

    case BT_STATUS_SECOND_CONNECTED://这个通道暂时未用,加入需考虑这里
    case BT_STATUS_FIRST_CONNECTED:
        log_info("BT_STATUS_CONNECTED: bt_idx:%d\n", cur_bt_idx);
        bt_hid_mode = HID_MODE_EDR;
        syscfg_write(CFG_HID_MODE_BEGIN + cur_bt_idx, &bt_hid_mode, 1);         //保存当前HID模式
#if TCFG_USER_EDR_ENABLE
        //to judge remote_addr == bt->args ?
        ret = syscfg_read(CFG_EDR_ADDRESS_BEGIN + cur_bt_idx, remote_addr, 6);
        if ((memcmp(remote_addr, bt->args, 6) != 0) && (ret == 6) && vm_address_change_flag == 1) {
            log_info("address is not match!!");
            user_send_cmd_prepare(USER_CTRL_DISCONNECTION_HCI, 0, NULL);
            set_edr_reconnect_flag(1);//当前通道重新page
            ret = 6;//避免覆盖vm
        }

        log_info("vm_address_change_flag: %d", vm_address_change_flag);
        if ((ret != 6) || vm_address_change_flag == 0) {
            put_buf(bt->args, 6);
            syscfg_write(CFG_EDR_ADDRESS_BEGIN + cur_bt_idx, bt->args, 6);
        }
        vm_address_change_flag = 1;
#endif
#if TCFG_USER_BLE_ENABLE
        //close ble
        ble_module_enable(0);
#endif
        break;

    default:
#if TCFG_USER_EDR_ENABLE
        bt_comm_edr_status_event_handler(bt);
#endif

#if TCFG_USER_BLE_ENABLE
        bt_comm_ble_status_event_handler(bt);
#endif
        break;
    }
    return 0;
}

static void stdkb_key_event_handler(struct sys_event *event)
{
    u8 event_type = 0;
    u8 key_value = 0;

    if (event->arg == (void *)DEVICE_EVENT_FROM_KEY) {
        event_type = event->u.key.event;
        key_value = event->u.key.value;
        log_info("app_key_evnet: %d,%d\n", event_type, key_value);
    }
}

static int stdkb_bt_common_event_handler(struct bt_event *bt)
{
    log_info("----%s reason %x %x", __FUNCTION__, bt->event, bt->value);

    switch (bt->event) {
    case COMMON_EVENT_EDR_REMOTE_TYPE:
        log_info(" COMMON_EVENT_EDR_REMOTE_TYPE,%d \n", bt->value);
        break;

    case COMMON_EVENT_BLE_REMOTE_TYPE:
        log_info(" COMMON_EVENT_BLE_REMOTE_TYPE,%d \n", bt->value);
        break;

    case COMMON_EVENT_SHUTDOWN_DISABLE:
        stdkb_auto_shutdown_disable();
        break;

    default:
        break;

    }
    return 0;
}

#define DEVICE_EVENT_FROM_CON		(('C' << 24) | ('O' << 16) | ('N' << 8) | '\0')
static int stdkb_event_handler(struct application *app, struct sys_event *event)
{
    u8 i = 0;
#if (TCFG_HID_AUTO_SHUTDOWN_TIME)
    //重置无操作定时计数
    if (event->type != SYS_DEVICE_EVENT || DEVICE_EVENT_FROM_POWER != event->arg) { //过滤电源消息
        //过滤掉sniff_enable or sniff_disable带来电源消息
        if (!(event->type == SYS_BT_EVENT && event->arg == DEVICE_EVENT_FROM_CON)) {
            sys_timer_modify(g_auto_shutdown_timer, TCFG_HID_AUTO_SHUTDOWN_TIME * 1000);
        }
    }
#endif

#if TCFG_USER_EDR_ENABLE
    bt_comm_edr_sniff_clean();
#endif

    /* log_info("event: %s", event->arg); */
    switch (event->type) {
    case SYS_KEY_EVENT:
        /* log_info("Sys Key : %s", event->arg); */
        stdkb_key_event_handler(event);
        return 0;

    case SYS_BT_EVENT:
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            stdkb_bt_connction_status_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
            stdkb_bt_hci_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_BLE_STATUS) {
            stdkb_hogp_ble_status_callback(event->u.bt.event, event->u.bt.value);
        } else if ((u32)event->arg == SYS_BT_EVENT_FORM_COMMON) {
            return stdkb_bt_common_event_handler(&event->u.dev);
        }
        return 0;

    case SYS_DEVICE_EVENT:
        if ((u32)event->arg == DEVICE_EVENT_FROM_POWER) {
            return app_power_event_handler(&event->u.dev, stdkb_set_soft_poweroff);
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
        stdkb_touch_pad_event_deal(event);
        break;

    default:
        return FALSE;
    }

    return FALSE;
}


//-----------------------
//system check go sleep is ok
static u8 stdkb_idle_query(void)
{
    return !is_stdkb_active;
}

REGISTER_LP_TARGET(app_stdkb_lp_target) = {
    .name = "app_stdkb_deal",
    .is_idle = stdkb_idle_query,
};

static const struct application_operation app_stdkb_ops = {
    .state_machine  = stdkb_state_machine,
    .event_handler 	= stdkb_event_handler,
};

/*
 * 注册模式
 */
REGISTER_APPLICATION(app_hid) = {
    .name 	= "standard_keyboard",
    .action	= ACTION_STANDARD_KEYBOARD,
    .ops 	= &app_stdkb_ops,
    .state  = APP_STA_DESTROY,
};


#endif



