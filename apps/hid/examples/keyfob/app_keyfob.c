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
#include "asm/pwm_led.h"
#include "app_charge.h"
#include "app_power_manage.h"
#include "app_comm_bt.h"

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

/* static const u8 bt_address_default[] = {0x11, 0x22, 0x33, 0x66, 0x77, 0x88}; */
static void keyfob_app_select_btmode(u8 mode);

extern int edr_hid_is_connected(void);
extern int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type);

typedef enum {
    HID_MODE_NULL = 0,
    HID_MODE_EDR,
    HID_MODE_BLE,
    HID_MODE_INIT = 0xff
} bt_mode_e;
static bt_mode_e bt_hid_mode;
static volatile u8 is_hid_active = 0;//1-临界点,系统不允许进入低功耗，0-系统可以进入低功耗

//----------------------------------
static const u8 keyfob_report_map[] = {
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
static const edr_sniff_par_t keyfob_sniff_param = {
    .sniff_mode = SNIFF_MODE_TYPE,
    .cnt_time = SNIFF_CNT_TIME,
    .max_interval_slots = SNIFF_MAX_INTERVALSLOT,
    .min_interval_slots = SNIFF_MIN_INTERVALSLOT,
    .attempt_slots = SNIFF_ATTEMPT_SLOT,
    .timeout_slots = SNIFF_TIMEOUT_SLOT,
    .check_timer_period = SNIFF_CHECK_TIMER_PERIOD,
};


//----------------------------------
static const edr_init_cfg_t keyfob_edr_config = {
    .page_timeout = 8000,
    .super_timeout = 8000,
    .io_capabilities = 3,
    .passkey_enable = 0,
    .authentication_req = 2,
    .oob_data = 0,
    .sniff_param = &keyfob_sniff_param,
    .class_type = BD_CLASS_KEYBOARD,
    .report_map = keyfob_report_map,
    .report_map_size = sizeof(keyfob_report_map),
};

//----------------------------------
static const ble_init_cfg_t keyfob_ble_config = {
    .same_address = 0,
    .appearance = BLE_APPEARANCE_HID_KEYBOARD,
    .report_map = keyfob_report_map,
    .report_map_size = sizeof(keyfob_report_map),
};

//-------------------SNIFF参数配置完成-------------------
static void keyfob_auto_shutdown_disable(void)
{
    log_info("----%s", __FUNCTION__);
    if (g_auto_shutdown_timer) {
        sys_timeout_del(g_auto_shutdown_timer);
    }
}


extern void ble_module_enable(u8 en);
extern void p33_soft_reset(void);
static void keyfob_set_soft_poweroff(void);
void keyfob_power_event_to_user(u8 event);

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
static u8 ble_connect = 0;
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

//bd29 没有pwm模块,用io反转推灯
#define KEYF_LED_ON()    gpio_direction_output(TCFG_PWMLED_PIN, 1)
#define KEYF_LED_OFF()   gpio_direction_output(TCFG_PWMLED_PIN, 0)
/* #define KEYF_LED_FLASH() //NULL */
#define KEYF_LED_INIT()  {gpio_set_dieh(TCFG_PWMLED_PIN, 0);gpio_set_die(TCFG_PWMLED_PIN, 1);gpio_set_pull_down(TCFG_PWMLED_PIN, 0);gpio_set_pull_up(TCFG_PWMLED_PIN, 0);} //

#if(LED_FLASH_1S)
#define KEYF_LED_FLASH() pwm_led_one_flash_display(1,200,200,1000,-1,500)
#else
#define KEYF_LED_FLASH() pwm_led_one_flash_display(1,200,200,500,-1,250)
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

#if(LED_FLASH_1S)
            led_timeout_count = (SOFT_OFF_TIME_MS / 1000) * 2;
            led_timer_start(500);//<
#else
            led_timeout_count = (SOFT_OFF_TIME_MS / 1000) * 4;
            led_timer_start(250);//<
#endif

            /* KEYF_LED_ON(); */
            led_io_flash = BIT(7) | BIT(0);
            KEYF_LED_FLASH();
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
            led_timer_start(300);
#if TCFG_USER_EDR_ENABLE
            if (edr_hid_is_connected()) {
                led_next_state = LED_CLOSE;
            } else {
                led_next_state = LED_WAIT_CONNECT;
            }
#else
            if ((ble_connect != 0)) {
                led_next_state = LED_CLOSE;
            } else {
                led_next_state = LED_WAIT_CONNECT;
            }
#endif
            KEYF_LED_ON();
            break;

        case LED_KEY_IO_VAILD:
            led_timeout_count = 1;//
            led_timer_start(650);
#if TCFG_USER_EDR_ENABLE
            if (edr_hid_is_connected()) {
                led_next_state = LED_CLOSE;
            } else {
                led_next_state = LED_WAIT_CONNECT;
            }
#endif
            KEYF_LED_ON();
            break;


        case LED_KEY_HOLD:
            led_timeout_count = 4;//2s
            led_timer_start(500);
#if TCFG_USER_EDR_ENABLE
            if (edr_hid_is_connected()) {
                led_next_state = LED_CLOSE;
            } else {
                led_next_state = LED_WAIT_CONNECT;
            }
#endif
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
            keyfob_power_event_to_user(POWER_EVENT_POWER_SOFTOFF);
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
#if TCFG_USER_EDR_ENABLE
            bt_wait_phone_connect_control(1);
#endif
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


//----------------------------------------
/* static const u8 key_a_big_press[3] = {0x00,0x0a,0x00}; */
static const u8 key_a_big_press[3] = {0x00, 0x02, 0x00};
static const u8 key_a_big_null[3] =  {0x00, 0x00, 0x00};

static const u8 key_a_small_press[3] = {0x00, 0x01, 0x00};
static const u8 key_a_small_null[3] =  {0x00, 0x00, 0x00};

extern int ble_hid_data_send(u8 report_id, u8 *data, u16 len);
extern int edr_hid_data_send(u8 report_id, u8 *data, u16 len);

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
            hid_data_send_pt(3, key_a_small_press, 3);
        }
        if (mode == 0 || mode == 2) {
            /* hid_data_send_pt(1,key_b_small_null,8); */
            hid_data_send_pt(3, key_a_small_null, 3);
        }
    }

}


static void app_keyfob_deal_test(u8 key_type, u8 key_value)
{
    u16 key_msg = 0;

#if TCFG_USER_EDR_ENABLE
    if (bt_hid_mode == HID_MODE_EDR && !edr_hid_is_connected()) {
        if (bt_connect_phone_back_start()) { //回连
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
    /* keyfob_app_select_btmode(HID_MODE_EDR); */
    /* } else { */
    /* keyfob_app_select_btmode(HID_MODE_BLE); */
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
static void keyfob_vm_deal(u8 rw_flag)
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

void keyfob_power_event_to_user(u8 event)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = (void *)DEVICE_EVENT_FROM_POWER;
    e.u.dev.event = event;
    e.u.dev.value = 0;
    sys_event_notify(&e);
}

static void keyfob_set_soft_poweroff(void)
{
    log_info("keyfob_set_soft_poweroff\n");
    is_hid_active = 1;
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



static void keyfob_timer_handle_test(void)
{
    log_info("not_bt");
}

void bredr_set_fix_pwr(u8 fix);
extern void bt_pll_para(u32 osc, u32 sys, u8 low_power, u8 xosc);
static void keyfob_app_start()
{
    log_info("=======================================");
    log_info("-------------keyfob DEMO-----------------");
    log_info("=======================================");
    log_info("app_file: %s", __FILE__);
    clk_set("sys", BT_NORMAL_HZ);

#if (TCFG_USER_EDR_ENABLE || TCFG_USER_BLE_ENABLE)
    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);
    led_on_off(LED_INIT, 0);


#if TCFG_USER_EDR_ENABLE
    btstack_edr_start_before_init(&keyfob_edr_config, 0);
#endif

#if TCFG_USER_BLE_ENABLE
    btstack_ble_start_before_init(&keyfob_ble_config, 0);
#endif

    btstack_init();

#else
    //no bt,to for test
    sys_timer_add(NULL, keyfob_timer_handle_test, 1000);
#endif

    /* 按键消息使能 */
    sys_key_event_enable();

#if (TCFG_HID_AUTO_SHUTDOWN_TIME)
    //无操作定时软关机
    g_auto_shutdown_timer = sys_timeout_add((void *)POWER_EVENT_POWER_SOFTOFF, keyfob_power_event_to_user, TCFG_HID_AUTO_SHUTDOWN_TIME * 1000);
#endif
    bredr_set_fix_pwr(7);
}

static int keyfob_state_machine(struct application *app, enum app_state state, struct intent *it)
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
            keyfob_app_start();
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


u8 connect_last_device_from_vm();


static void bt_hci_event_linkkey_missing(struct bt_event *bt)
{
}
static void keyfob_hogp_ble_status_callback(ble_state_e status, u8 reason)
{
    log_info("keyfob_hogp_ble_status_callback==================== %d   reason:0x%x\n", status, reason);
    switch (status) {
    case BLE_ST_IDLE:
        break;
    case BLE_ST_ADV:
        break;
    case BLE_ST_CONNECT:
        ble_connect++;
        led_on_off(LED_CLOSE, 0);
        bt_hid_mode = HID_MODE_BLE;
        break;
    case BLE_ST_SEND_DISCONN:
        break;
    case BLE_ST_DISCONN:
        if (reason == ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST) {
            led_on_off(LED_CLOSE, 0);
            printf("BLE_ST_DISCONN BY LOCAL...\n");
        } else {
            led_on_off(LED_WAIT_CONNECT, 0);
        }
        break;
    case BLE_ST_NOTIFY_IDICATE:
        break;
    default:
        break;
    }
}

extern void set_remote_test_flag(u8 own_remote_test);
static int keyfob_bt_hci_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_info("------------------------keyfob_bt_hci_event_handler reason %x %x", bt->event, bt->value);
#if TCFG_USER_EDR_ENABLE
    bt_comm_edr_hci_event_handler(bt);
#endif

#if TCFG_USER_BLE_ENABLE
    bt_comm_ble_hci_event_handler(bt);
#endif

    return 0;
}


static int keyfob_bt_connction_status_event_handler(struct bt_event *bt)
{

    log_info("--------keyfob_bt_connction_status_event_handler %d", bt->event);

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        /*
         * 蓝牙初始化完成
         */
        log_info("BT_STATUS_INIT_OK\n");

        keyfob_vm_deal(0);

        if (bt_hid_mode == HID_MODE_BLE) {
#if TCFG_USER_BLE_ENABLE
            btstack_ble_start_after_init(0);
#endif
        } else {
#if TCFG_USER_EDR_ENABLE
            btstack_edr_start_after_init(0);
#endif
        }

        keyfob_app_select_btmode(HID_MODE_INIT);
#if HID_TEST_KEEP_SEND_EN
        hidkey_test_keep_send_init();
#endif
        break;

    default:
#if TCFG_USER_EDR_ENABLE
        if (bt->event == BT_STATUS_FIRST_CONNECTED) {
            led_on_off(LED_CLOSE, 0);
        } else if (bt->event == BT_STATUS_FIRST_DISCONNECT) {
            led_on_off(LED_WAIT_CONNECT, 0);
        }
        bt_comm_edr_status_event_handler(bt);
#endif

#if TCFG_USER_BLE_ENABLE
        bt_comm_ble_status_event_handler(bt);
#endif
        break;
    }

    return 0;
}
static int keyfob_bt_common_event_handler(struct bt_event *bt)
{
    log_info("----%s reason %x %x", __FUNCTION__, bt->event, bt->value);

    switch (bt->event) {
    case COMMON_EVENT_EDR_REMOTE_TYPE:
        log_info(" COMMON_EVENT_EDR_REMOTE_TYPE \n");
        break;

    case COMMON_EVENT_BLE_REMOTE_TYPE:
        log_info(" COMMON_EVENT_BLE_REMOTE_TYPE \n");
        break;

    case COMMON_EVENT_SHUTDOWN_DISABLE:
        keyfob_auto_shutdown_disable();
        break;

    default:
        break;

    }
    return 0;
}

static void app_keyfob_event_handler(struct sys_event *event)
{
    /* u16 cpi = 0; */
    u8 event_type = 0;
    u8 key_value = 0;

    if (event->arg == (void *)DEVICE_EVENT_FROM_KEY) {
        event_type = event->u.key.event;
        key_value = event->u.key.value;
        printf("app_key_evnet: %d,%d\n", event_type, key_value);
        app_keyfob_deal_test(event_type, key_value);
    }
}


static int keyfob_event_handler(struct application *app, struct sys_event *event)
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
    /* log_info("event: %s", event->arg); */
    switch (event->type) {
    case SYS_KEY_EVENT:
        /* log_info("Sys Key : %s", event->arg); */
        app_keyfob_event_handler(event);
        return 0;

    case SYS_BT_EVENT:
#if (TCFG_USER_EDR_ENABLE || TCFG_USER_BLE_ENABLE)
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            keyfob_bt_connction_status_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
            keyfob_bt_hci_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_BLE_STATUS) {
            keyfob_hogp_ble_status_callback(event->u.bt.event, event->u.bt.value);
        } else if ((u32)event->arg == SYS_BT_EVENT_FORM_COMMON) {
            return keyfob_bt_common_event_handler(&event->u.dev);
        }
#endif
        return 0;

    case SYS_DEVICE_EVENT:
        if ((u32)event->arg == DEVICE_EVENT_FROM_POWER) {
            return app_power_event_handler(&event->u.dev, keyfob_set_soft_poweroff);
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


static void keyfob_app_select_btmode(u8 mode)
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
        /* user_hid_enable(0); */
        /* bt_wait_phone_connect_control(0); */
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
            /* user_hid_enable(1); */
            if (!bt_connect_phone_back_start()) {
                bt_wait_phone_connect_control(1);
            }
        }
#endif

    }

    /* trace_run_debug_val(0); */
    keyfob_vm_deal(1);
}


//-----------------------
//system check go sleep is ok
static u8 app_keyfob_idle_query(void)
{
    return !is_hid_active;
}

REGISTER_LP_TARGET(app_hid_lp_target) = {
    .name = "app_keyfob_deal",
    .is_idle = app_keyfob_idle_query,
};

static const struct application_operation app_keyfob_ops = {
    .state_machine  = keyfob_state_machine,
    .event_handler 	= keyfob_event_handler,
};

/*
 * 注册AT Module模式
 */
REGISTER_APPLICATION(app_hid) = {
    .name 	= "keyfob",
    .action	= ACTION_KEYFOB,
    .ops 	= &app_keyfob_ops,
    .state  = APP_STA_DESTROY,
};

#endif



