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
#include "asm/pwm_led.h"
#include "app_charge.h"
#include "app_power_manage.h"

#if(CONFIG_APP_KEYFOB)

#define LOG_TAG_CONST      KEYFOB
#define LOG_TAG             "[KEYFOB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define trace_run_debug_val(x)   //log_info("\n## %s: %d,  0x%04x ##\n",__FUNCTION__,__LINE__,x)


static u16 g_auto_shutdown_timer = 0;

extern void bt_set_osc_cap(u8 sel_l, u8 sel_r);
extern const u8 *bt_get_mac_addr();
extern void lib_make_ble_address(u8 *ble_address, u8 *edr_address);


/* static const u8 bt_address_default[] = {0x11, 0x22, 0x33, 0x66, 0x77, 0x88}; */
static void app_select_btmode(u8 mode);
void sys_auto_sniff_controle(u8 enable, u8 *addr);
void bt_sniff_ready_clean(void);
static void bt_wait_phone_connect_control(u8 enable);
static void auto_reconnect_handle(void);
static void auto_reconnect_start(void);
static void auto_reconnect_stop(void);

extern int edr_hid_is_connected(void);
extern int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type);

#define WAIT_DISCONN_TIME_MS     (300)

typedef enum {
    HID_MODE_NULL = 0,
    HID_MODE_EDR,
    HID_MODE_BLE,
    HID_MODE_INIT = 0xff
} bt_mode_e;
static bt_mode_e bt_hid_mode;
static volatile u8 is_hid_active = 0;//1-临界点,系统不允许进入低功耗，0-系统可以进入低功耗

//----------------------------------
static const u8 hid_report_map[] = {
#if 0
    // keyboard 描述符
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0,        //   Usage Minimum (0xE0)
    0x29, 0xE7,        //   Usage Maximum (0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x05,        //   Usage Maximum (Kana)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x03,        //   Report Size (3)
    0x91, 0x03,        //   Output (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x29, 0xFF,        //   Usage Maximum (0xFF)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
#endif

    //通用按键
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x03,        //   Report ID (3)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x0B,        //   Report Count (11)
    0x0A, 0x23, 0x02,  //   Usage (AC Home)
    0x0A, 0x21, 0x02,  //   Usage (AC Search)
    0x0A, 0xB1, 0x01,  //   Usage (AL Screen Saver)
    0x09, 0xB8,        //   Usage (Eject)
    0x09, 0xB6,        //   Usage (Scan Previous Track)
    0x09, 0xCD,        //   Usage (Play/Pause)
    0x09, 0xB5,        //   Usage (Scan Next Track)
    0x09, 0xE2,        //   Usage (Mute)
    0x09, 0xEA,        //   Usage (Volume Decrement)
    0x09, 0xE9,        //   Usage (Volume Increment)
    0x09, 0x30,        //   Usage (Power)
    0x0A, 0xAE, 0x01,  //   Usage (AL Keyboard Layout)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x0D,        //   Report Size (13)
    0x81, 0x03,        //   Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection

    // 119 bytes
};

// consumer key

//----------------------------------
extern void p33_soft_reset(void);
void hid_set_soft_poweroff(void);
int bt_connect_phone_back_start(u8 mode);

enum {
    LED_NULL = 0,
    LED_INIT,
    LED_WAIT_CONNECT,
    LED_AUTO_CONNECT,
    LED_KEY_UP,
    LED_KEY_HOLD,
    LED_KEY_IO_VAILD,
    LED_CLOSE,
    LED_POWER_OFF,
};

static u8  led_state;
static u8  led_next_state;
static u8  led_io_flash;
static u32 led_state_count = 0;
static u32 led_timer_ms;
static u32 led_timer_id = 0;
static u32 led_timeout_count;
static u8 reconnect_mode = 0;

static u32 auto_reconnect_timer = 0;

static void led_timer_stop(void);
static void led_timer_start(u32 time_ms);
static void led_on_off(u8 state, u8 res);

#define LED_FLASH_1S           1 //未连接闪灯时间，1秒 or 0.5s

#define SOFT_OFF_TIME_MS      (160000L)//
#define AUTO_CONNECT_TIME_MS  (8000L)//
#define POWER_LED_ON_TIME_MS  (2000L)//

#ifndef VBAT_LOW_POWER_LEVEL
#define VBAT_LOW_POWER_LEVEL    (250)
#endif

#if TCFG_PWMLED_ENABLE

#if TCFG_PWMLED_IO_PUSH
//bd29 没有pwm模块,用io反转推灯
#define KEYF_LED_ON()    gpio_direction_output(TCFG_PWMLED_PIN, 1)
#define KEYF_LED_OFF()   gpio_direction_output(TCFG_PWMLED_PIN, 0)
#define KEYF_LED_FLASH() //NULL
#define KEYF_LED_INIT()  {gpio_set_dieh(TCFG_PWMLED_PIN, 0);gpio_set_die(TCFG_PWMLED_PIN, 1);gpio_set_pull_down(TCFG_PWMLED_PIN, 0);gpio_set_pull_up(TCFG_PWMLED_PIN, 0);} //

#else
#define KEYF_LED_ON()    _pwm_led_on_display(1,200,200)
#define KEYF_LED_OFF()   pwm_led_set_off()

#if(LED_FLASH_1S)
#define KEYF_LED_FLASH() pwm_led_one_flash_display(1,200,200,1000,-1,500)
#else
#define KEYF_LED_FLASH() pwm_led_one_flash_display(1,200,200,500,-1,250)
#endif

#define KEYF_LED_INIT() //NULL
#endif

#else
#define KEYF_LED_ON()
#define KEYF_LED_OFF()
#define KEYF_LED_FLASH()
#define KEYF_LED_INIT() //NULL
#endif


void set_change_vbg_value_flag(void);
static int check_vbat_level(void)
{
    static u8 lpwr_cnt = 0;
    u32 value = (adc_get_voltage(AD_CH_VBAT) * 4 / 10);

    log_info("{%d}", value);
    if (value < VBAT_LOW_POWER_LEVEL) {
        log_info("{%d,%d}", value, VBAT_LOW_POWER_LEVEL);
        if (++lpwr_cnt > 1) {
            return 1;
        }
    } else {
        lpwr_cnt = 0;
    }
    return 0;
}


static u16 io_check_timer = 0;
int io_key_is_vaild(void);
static void io_check_timer_handle(void)
{
    static u32 vaild_cnt = 0;
    static u8 check_time = 0;

#if 0 //低电关机检测,佳宝确认再打开
    if (led_state != LED_AUTO_CONNECT) {
        ++check_time;
        if (check_time % 35 == 0) {
            check_time = 0;
            if (check_vbat_level()) {
                led_on_off(LED_POWER_OFF, 0);
                return;
            }
        }
    }
#endif

    if (led_state != LED_KEY_IO_VAILD && led_state != LED_CLOSE && led_state != LED_WAIT_CONNECT) {
        vaild_cnt = 0;
        return;
    }

    if (io_key_is_vaild()) {
        if (++vaild_cnt > 1) {
            led_on_off(LED_KEY_IO_VAILD, 0);
        }
    } else {
        vaild_cnt = 0;
    }

}

static void led_on_off(u8 state, u8 res)
{
    /* if(led_state != state || (state == LED_KEY_HOLD)){ */
    if (1) { //相同状态也要更新时间
        u8 prev_state = led_state;
        log_info("led_state: %d>>>%d", led_state, state);
        led_state = state;
        led_io_flash = 0;

        switch (state) {
        case LED_INIT:
            KEYF_LED_INIT();
            led_timeout_count = POWER_LED_ON_TIME_MS / 1000; //
            led_timer_start(1000);
            led_next_state = LED_WAIT_CONNECT;
            KEYF_LED_ON();
            if (!io_check_timer) {
                io_check_timer = sys_s_hi_timer_add((void *)0, io_check_timer_handle, 15);
            }
            break;

        case LED_WAIT_CONNECT:
#if TCFG_PWMLED_IO_PUSH

#if(LED_FLASH_1S)
            led_timeout_count = (SOFT_OFF_TIME_MS / 1000) * 2;
            led_timer_start(500);//<
#else
            led_timeout_count = (SOFT_OFF_TIME_MS / 1000) * 4;
            led_timer_start(250);//<
#endif

            KEYF_LED_ON();
            led_io_flash = BIT(7) | BIT(0);
#else
            led_timeout_count = (SOFT_OFF_TIME_MS / 1000) / 5;
            led_timer_start(5000);//<
            KEYF_LED_FLASH();
#endif
            led_next_state = LED_POWER_OFF;
            break;

        case LED_AUTO_CONNECT:
            led_timeout_count = 1;//
            led_timer_start(AUTO_CONNECT_TIME_MS);
            led_next_state = LED_WAIT_CONNECT;
            KEYF_LED_ON();
            break;

        case LED_KEY_UP:
            led_timeout_count = 1;//
            led_timer_start(100);
            if (edr_hid_is_connected()) {
                led_next_state = LED_CLOSE;
            } else {
                led_next_state = LED_WAIT_CONNECT;
            }
            KEYF_LED_ON();
            break;

        case LED_KEY_IO_VAILD:
            led_timeout_count = 1;//
            led_timer_start(650);
            if (edr_hid_is_connected()) {
                led_next_state = LED_CLOSE;
            } else {
                led_next_state = LED_WAIT_CONNECT;
            }
            KEYF_LED_ON();
            break;


        case LED_KEY_HOLD:
            led_timeout_count = 4;//2s
            led_timer_start(500);
            if (edr_hid_is_connected()) {
                led_next_state = LED_CLOSE;
            } else {
                led_next_state = LED_WAIT_CONNECT;
            }
            KEYF_LED_ON();
            break;

        case LED_CLOSE:
            KEYF_LED_OFF();
            led_timeout_count = 0;
            led_timer_stop();
            break;

        case LED_POWER_OFF:
            KEYF_LED_OFF();
            led_timeout_count = 0;
            led_timer_stop();
            hid_set_soft_poweroff();
            if (io_check_timer) {
                sys_s_hi_timer_del(io_check_timer);
                io_check_timer = 0;
            }
            break;

        default:
            log_error("Unknow led_state:%d", led_state);
            break;
        }
    }

}

static void led_timer_handle(void)
{
    static u8 onoff = 0;

    /* putchar('H'); */
    //printf("{%d}",led_timeout_count);

    if (led_timeout_count < 2) {
        if (LED_INIT == led_state) {
            bt_wait_phone_connect_control(1);
        }
        led_on_off(led_next_state, 0);
        return;
    }
    led_timeout_count--;

    //io 反转推灯才需要
    if (led_io_flash & BIT(7)) {
        led_io_flash ^= BIT(0);
        if (led_io_flash & BIT(0)) {
            KEYF_LED_ON();
        } else {
            KEYF_LED_OFF();
        }
    }

}

static void led_timer_stop(void)
{
    if (led_timer_id) {
        log_info("led_timer stop");
        sys_timer_del(led_timer_id);
        led_timer_id = 0;
    }
}

static void led_timer_start(u32 time_ms)
{
    led_timer_stop(); //stop firstly
    log_info("led_timer start %d ms", time_ms);
    led_timer_ms = time_ms;
    led_timer_id = sys_timer_add(0, led_timer_handle, led_timer_ms);
}

void auto_shutdown_disable(void)
{
    if (g_auto_shutdown_timer) {
        sys_timeout_del(g_auto_shutdown_timer);
    }
}


//----------------------------------------
/* static const u8 key_a_big_press[3] = {0x00,0x0a,0x00}; */
static const u8 key_a_big_press[3] = {0x00, 0x02, 0x00};
static const u8 key_a_big_null[3] =  {0x00, 0x00, 0x00};

/* static const u8 key_b_small_press[4] = {0x28,0x00,0x00,0x00}; */
/* static const u8 key_b_small_null[4] =  {0x00,0x00,0x00,0x00}; */

/* static const u8 key_b_small_press[8] = {0x00,0x00,0x28,0x00,0x00,0x00,0x00,0x00}; */
/* static const u8 key_b_small_null[8] =  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}; */

extern int ble_hid_data_send(u8 report_id, u8 *data, u16 len);
extern void edr_hid_data_send(u8 report_id, u8 *data, u16 len);

//按键ID
#define KEY_BIG_ID    1
#define KEY_SMALL_ID  2

//mode >> 0：press + up，1：press，2：up
static void key_value_send(u8 key_value, u8 mode)
{
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
        return;
    }

    if (key_value == KEY_BIG_ID) {
        if (mode == 0 || mode == 1) {
            hid_data_send_pt(3, key_a_big_press, 3);
        }
        if (mode == 0 || mode == 2) {
            hid_data_send_pt(3, key_a_big_null, 3);
        }
    } else if (key_value == KEY_SMALL_ID) {
        if (mode == 0 || mode == 1) {
            /* hid_data_send_pt(1,key_b_small_press,8); */
            hid_data_send_pt(3, key_a_big_press, 3);
        }
        if (mode == 0 || mode == 2) {
            /* hid_data_send_pt(1,key_b_small_null,8); */
            hid_data_send_pt(3, key_a_big_null, 3);
        }
    }

}

/* static u16 key_timer_id = 0; */
/* static void key_timer_handle(void *priv) */
/* { */
/* key_value_send((u32)priv,0); */
/* } */

/* static void key_timer_start(u32 priv) */
/* { */
/* if(!key_timer_id){ */
/* log_info("key_timer start"); */
/* key_timer_id = sys_timer_add((void*)priv,key_timer_handle,30); */
/* } */
/* }	 */

/* static void key_timer_stop(void) */
/* { */
/* if(key_timer_id){ */
/* log_info("key_timer stop"); */
/* sys_timer_del(key_timer_id); */
/* key_timer_id = 0; */
/* } */
/* } */

static void app_key_deal_test(u8 key_type, u8 key_value)
{
    u16 key_msg = 0;

#if TCFG_USER_EDR_ENABLE
    if (bt_hid_mode == HID_MODE_EDR && !edr_hid_is_connected()) {
        if (bt_connect_phone_back_start(1)) { //回连
            return;
        }
    }
#endif

    switch (key_type) {
    case KEY_EVENT_CLICK:
        key_value_send(key_value, 0);
        led_on_off(LED_KEY_UP, 0);
        break;

    case KEY_EVENT_LONG:
        key_value_send(key_value, 0);
        if (io_key_is_vaild) {
            led_on_off(LED_KEY_HOLD, 0);
        } else {
            led_on_off(LED_KEY_UP, 0);
        }
        break;

    case KEY_EVENT_HOLD:
        if (key_value == KEY_BIG_ID) {
            //启动连拍
            key_value_send(key_value, 1);
        }
        led_on_off(LED_KEY_HOLD, 0);
        break;

    case KEY_EVENT_UP:
        if (key_value == KEY_BIG_ID) {
            //停止连拍
            key_value_send(key_value, 2);
        }
        led_on_off(LED_KEY_UP, 0);
        break;

    default:
        break;
    }


    /* if (key_type == KEY_EVENT_DOUBLE_CLICK && key_value == TCFG_ADKEY_VALUE0) { */
    /* #if (TCFG_USER_EDR_ENABLE && TCFG_USER_BLE_ENABLE) */
    /* is_hid_active = 1; */
    /* if (HID_MODE_BLE == bt_hid_mode) { */
    /* app_select_btmode(HID_MODE_EDR); */
    /* } else { */
    /* app_select_btmode(HID_MODE_BLE); */
    /* } */
    /* os_time_dly(WAIT_DISCONN_TIME_MS / 10); //for disconnect ok */
    /* p33_soft_reset(); */
    /* while (1); */
    /* #endif */
    /* } */
}
//----------------------------------

typedef struct {
    u16 head_tag;
    u8  mode;
} hid_vm_cfg_t;

#define	HID_VM_HEAD_TAG (0x3AA3)
static void hid_vm_deal(u8 rw_flag)
{
    hid_vm_cfg_t info;
    int ret;
    int vm_len = sizeof(hid_vm_cfg_t);

    log_info("-hid_info vm_do:%d\n", rw_flag);
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
    __set_simple_pair_param(3, 0, 2);

#if TCFG_USER_BLE_ENABLE
    {
        u8 tmp_ble_addr[6];
        /* le_controller_set_mac((void*)"012345"); */
        lib_make_ble_address(tmp_ble_addr, (void *)bt_get_mac_addr());
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
    user_hid_init(NULL);
#endif

    /* bt_dut_test_handle_register(bt_dut_api); */
}

extern void ble_module_enable(u8 en);
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

void bredr_set_fix_pwr(u8 fix);
extern void bt_pll_para(u32 osc, u32 sys, u8 low_power, u8 xosc);
static void app_start()
{
    log_info("=======================================");
    log_info("-------------keyfob DEMO-----------------");
    log_info("=======================================");

    clk_set("sys", BT_NORMAL_HZ);
    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);

    led_on_off(LED_INIT, 0);

    bt_function_select_init();
    bredr_handle_register();
    btstack_init();

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
    bredr_set_fix_pwr(7);
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
        case ACTION_KEYFOB:
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

#define SNIFF_MAX_INTERVALSLOT        800//(800 *2)
#define SNIFF_MIN_INTERVALSLOT        100
#define SNIFF_ATTEMPT_SLOT            4
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

void bt_sniff_ready_clean(void)
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
void sys_auto_sniff_controle(u8 enable, u8 *addr)
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
    if (enable) {
        if (bt_hid_mode != HID_MODE_EDR) {
            return;
        }
        if (LED_INIT != led_state) {
            led_on_off(LED_WAIT_CONNECT, 0);
        }
        reconnect_mode = 0;
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

/* void bt_wait_phone_connect_control_ext(u8 inquiry_en, u8 page_scan_en) */
/* { */
/* if (inquiry_en) { */
/* user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL); */
/* } else { */
/* user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL); */
/* } */

/* if (page_scan_en) { */
/* user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL); */
/* } else { */
/* user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL); */
/* } */
/* } */

void bt_send_pair(u8 en)
{
    user_send_cmd_prepare(USER_CTRL_PAIR, 1, &en);
}


u8 connect_last_device_from_vm();
int bt_connect_phone_back_start(u8 mode)
{
    log_info("bt_hid_mode= %d", bt_hid_mode);
    if (bt_hid_mode != HID_MODE_EDR) {
        return 0;
    }

    if (led_state == LED_AUTO_CONNECT) {
        log_info("doing1");
        auto_reconnect_stop();
        return 1;
    }

    log_info("old:%d,new:%d\n", reconnect_mode, mode);

    if (reconnect_mode == 2 && mode == 1) {
        reconnect_mode = mode;
        led_on_off(LED_AUTO_CONNECT, 0);
        log_info("doing2");
        return 1;
    }

    reconnect_mode = mode;

    if (connect_last_device_from_vm()) {
        log_info("------bt_connect_phone_start------");
        if (reconnect_mode == 1) {
            led_on_off(LED_AUTO_CONNECT, 0);
        }
        return 1;
    } else {
        log_info("no pair info");
    }
    return 0;
}

static void auto_reconnect_stop(void)
{
    if (auto_reconnect_timer) {
        log_info("auto_reconnect stop");
        sys_timer_del(auto_reconnect_timer);
        auto_reconnect_timer = 0;
    }
}

static void auto_reconnect_start(void)
{
    if (!auto_reconnect_timer) {
        log_info("auto_reconnect start");
        auto_reconnect_handle();//do firstly
        auto_reconnect_timer = sys_timer_add(0, auto_reconnect_handle, 15000);
    }
}


static void auto_reconnect_handle(void)
{
    if (reconnect_mode == 2) {
        reconnect_mode = 0;
    }
    if (!bt_connect_phone_back_start(2)) {
        auto_reconnect_stop();
    }
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
#define ERROR_CODE_REMOTE_USER_TERMINATED_LOW_RESOURCES       0x14
#define ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST        0x16

#define CUSTOM_BB_AUTO_CANCEL_PAGE                            0xFD  //// app cancle page
#define BB_CANCEL_PAGE                                        0xFE  //// bb cancle page


static void bt_hci_event_connection(struct bt_event *bt)
{
    auto_reconnect_stop();
    led_on_off(LED_CLOSE, 0);
    bt_wait_phone_connect_control(0);
}

static void bt_hci_event_disconnect(struct bt_event *bt)
{
    /* #if (RCSP_BTMATE_EN && RCSP_UPDATE_EN) */
    /*     if (get_jl_update_flag()) { */
    /*         JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_UPDATE_START, NULL, 0); */
    /*     } */
    /* #endif */
    bt_wait_phone_connect_control(1);
}

static void bt_hci_event_linkkey_missing(struct bt_event *bt)
{
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

extern void set_remote_test_flag(u8 own_remote_test);
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

        case ERROR_CODE_REMOTE_USER_TERMINATED_LOW_RESOURCES:
        case ERROR_CODE_CONNECTION_TIMEOUT:
            log_info(" ERROR_CODE_CONNECTION_TIMEOUT \n");
            bt_hci_event_connection_timeout(bt);
            auto_reconnect_start();
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

        hid_vm_deal(0);//bt_hid_mode read for VM
        app_select_btmode(HID_MODE_INIT);//
        break;

    case BT_STATUS_SECOND_CONNECTED:
    case BT_STATUS_FIRST_CONNECTED:
        log_info("BT_STATUS_CONNECTED\n");
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
        app_key_deal_test(event_type, key_value);
    }
}


static int event_handler(struct application *app, struct sys_event *event)
{

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
        } else if ((u32)event->arg == DEVICE_EVENT_FROM_POWER) {
            return app_power_event_handler(&event->u.dev);
        }
#if TCFG_CHARGE_ENABLE
        else if ((u32)event->arg == DEVICE_EVENT_FROM_CHARGE) {
            app_charge_event_handler(&event->u.dev);
        }
#endif
        return 0;

    case SYS_DEVICE_EVENT:
        return 0;

    default:
        return FALSE;
    }

    return FALSE;
}


extern void bredr_power_get(void);
extern void bredr_power_put(void);
extern void radio_set_eninv(int v);
void ble_module_enable(u8 en);
static void app_select_btmode(u8 mode)
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
        user_hid_enable(0);
        bt_wait_phone_connect_control(0);
#endif

#if TCFG_USER_BLE_ENABLE
        if (mode == HID_MODE_INIT) {
            ble_module_enable(1);
        }
#endif

#if TCFG_USER_EDR_ENABLE
        //close edr

#ifndef CONFIG_CPU_BR30
        radio_set_eninv(0);
#endif
        bredr_power_put();
        sys_auto_sniff_controle(0, NULL);
#endif
    } else {
        //edr
        log_info("---------app select edr--------\n");
        if (!STACK_MODULES_IS_SUPPORT(BT_BTSTACK_CLASSIC) || !BT_MODULES_IS_SUPPORT(BT_MODULE_CLASSIC)) {
            log_info("not surpport edr,make sure config !!!\n");
            ASSERT(0);
        }

        /* bredr_power_get(); */
        /* radio_set_eninv(1); */

#if TCFG_USER_BLE_ENABLE
        //close ble
        ble_module_enable(0);
#endif

#if TCFG_USER_EDR_ENABLE
        if (mode == HID_MODE_INIT) {
            user_hid_enable(1);
            if (!bt_connect_phone_back_start(1)) {
                /* bt_wait_phone_connect_control(1); */
            }
        }
#endif

    }

    trace_run_debug_val(0);
    hid_vm_deal(1);
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
 * 注册AT Module模式
 */
REGISTER_APPLICATION(app_hid) = {
    .name 	= "keyfob",
    .action	= ACTION_KEYFOB,
    .ops 	= &app_hid_ops,
    .state  = APP_STA_DESTROY,
};

#endif

