/*********************************************************************************************
    *   Filename        : app_mouse.c

    *   Description     :鼠标单模切换

    *   Author          :

    *   Email           :

    *   Last modifiled  : 2020-03-24 09:52:50

    *   Copyright:(c)JIELI  2011-2020  @ , All Rights Reserved.
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
#include "le_common.h"

#include "OMSensor_manage.h"
#include "rcsp_bluetooth.h"
#include "rcsp_user_update.h"
#include "app_charge.h"
#include "app_power_manage.h"
#include "app_comm_bt.h"
#if(CONFIG_APP_MOUSE_SINGLE)

#define LOG_TAG_CONST       MOUSE
#define LOG_TAG             "[MOUSE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define trace_run_debug_val(x)   //log_info("\n## %s: %d,  0x%04x ##\n",__FUNCTION__,__LINE__,x)

//2.4G模式: 0---ble, 非0---2.4G配对码
#define CFG_RF_24G_CODE_ID         (0x239a239a) //32bits

//模式(edr,ble,24g)切换控制,可自己修改按键方式
#define MIDDLE_KEY_SWITCH          1   //左按键+中键,长按数秒切换 edr & ble , or 2.4g & ble, or ble & 2.4g & edr
#define MIDDLE_KEY_HOLD_CNT       (4)  //长按中键计算次数 >= 4s
//配置支持的切换模式
#define SWITCH_MODE_EDR_ENABLE     (1 & TCFG_USER_EDR_ENABLE)  //使能EDR模式
#define SWITCH_MODE_BLE_ENABLE     (1 & TCFG_USER_BLE_ENABLE)  //使能BLE模式
#define SWITCH_MODE_24G_ENABLE     (1 & (CFG_RF_24G_CODE_ID != 0) & TCFG_USER_BLE_ENABLE)//使能24G模式

//使能开配对管理,可自己修改按键方式
#define DOUBLE_KEY_HOLD_PAIR      (1)  //中键+右按键 长按数秒,进入当前蓝牙模块的配对模式
#define DOUBLE_KEY_HOLD_CNT       (4)  //长按中键计算次数 >= 4s

/*配置发送周期和sniff周期匹配*/
#define TEST_REPORT_DATA_PERIOD_MS (8)  //鼠标发数周期 >= 8ms
#define EDR_SET_SNIFF_SLOTS       (TEST_REPORT_DATA_PERIOD_MS * 8 / 5)
#define EDR_SET_TIMER_VALUE       (TEST_REPORT_DATA_PERIOD_MS)

//for io deubg
#define MO_IO_DEBUG_0(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT &= ~BIT(x);}
#define MO_IO_DEBUG_1(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT |= BIT(x);}
#define MO_IO_DEBUG_TOGGLE(i,x)  //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT ^= BIT(x);}

/*
   定时器模拟sensor发数使能
   测试时关掉TCFG_OMSENSOR_ENABLE 和 TCFG_CODE_SWITCH_ENABLE
 */
#define TEST_MOUSE_SIMULATION_ENABLE     0

static u8 switch_key_long_cnt = 0;
static u8 double_key_long_cnt = 0;

static u16 g_auto_shutdown_timer = 0;

int sniff_timer = 0;
void bt_check_exit_sniff();


/* static const u8 bt_address_default[] = {0x11, 0x22, 0x33, 0x66, 0x77, 0x88}; */
static void mouse_select_btmode(u8 mode);
static void mouse_vm_deal(u8 rw_flag);
static void app_optical_sensor_event_handler(struct sys_event *event);
static void mouse_test_ctrl(void);
extern int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type);

#define WAIT_DISCONN_TIME_MS     (300)

typedef enum {
    HID_MODE_NULL = 0,
    HID_MODE_EDR,
    HID_MODE_BLE,
    HID_MODE_24G,/*ble's 24g*/
    HID_MODE_INIT = 0xff
} bt_mode_e;
static bt_mode_e bt_hid_mode;
static volatile u8 is_hid_active = 0;//1-临界点,系统不允许进入低功耗，0-系统可以进入低功耗
static u32 ble_24g_code;//record 2.4G mode

//----------------------------------
#define BUTTONS_IDX							0
#define WHEEL_IDX               			(BUTTONS_IDX + 1)
#define SENSOR_XLSB_IDX         			0
#define SENSOR_YLSB_XMSB_IDX    			(SENSOR_XLSB_IDX + 1)
#define SENSOR_YMSB_IDX         			(SENSOR_YLSB_XMSB_IDX +1)

typedef struct {
    u8 data[3];
    u8 event_type;
    u8 *event_arg;
    u8 key_val;
} mouse_packet_data_t;

extern int ble_hid_is_connected(void);
extern int ble_hid_timer_handle;
extern int edr_hid_timer_handle;
static u8 button_send_flag = 0;
static u8 wheel_send_flag = 0;
static u8 sensor_send_flag = 0;

/* static u16 auto_shutdown_timer = 0; */
static volatile mouse_packet_data_t first_packet = {0};
static volatile mouse_packet_data_t second_packet = {0};

static const u8 mouse_report_map[] = {
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
#if SNIFF_MODE_RESET_ANCHOR
#define SNIFF_MODE_TYPE               SNIFF_MODE_ANCHOR
#define SNIFF_CNT_TIME                1/////<空闲?S之后进入sniff模式

#define SNIFF_MAX_INTERVALSLOT        EDR_SET_SNIFF_SLOTS
#define SNIFF_MIN_INTERVALSLOT        EDR_SET_SNIFF_SLOTS
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
static const edr_sniff_par_t mouse_sniff_param = {
    .sniff_mode = SNIFF_MODE_TYPE,
    .cnt_time = SNIFF_CNT_TIME,
    .max_interval_slots = SNIFF_MAX_INTERVALSLOT,
    .min_interval_slots = SNIFF_MIN_INTERVALSLOT,
    .attempt_slots = SNIFF_ATTEMPT_SLOT,
    .timeout_slots = SNIFF_TIMEOUT_SLOT,
    .check_timer_period = SNIFF_CHECK_TIMER_PERIOD,
};

static const edr_init_cfg_t mouse_edr_config = {
    .page_timeout = 8000,
    .super_timeout = 8000,
    .io_capabilities = 3,
    .passkey_enable = 0,
    .authentication_req = 2,
    .oob_data = 0,
    .sniff_param = &mouse_sniff_param,
    .class_type = BD_CLASS_MOUSE,
    .report_map = mouse_report_map,
    .report_map_size = sizeof(mouse_report_map),
};

//----------------------------------
static const ble_init_cfg_t mouse_ble_config = {
    .same_address = 0,
    .appearance = BLE_APPEARANCE_HID_MOUSE,
    .report_map = mouse_report_map,
    .report_map_size = sizeof(mouse_report_map),
};


void mouse_auto_shutdown_disable(void)
{
    if (g_auto_shutdown_timer) {
        sys_timeout_del(g_auto_shutdown_timer);
    }
}

extern int ble_hid_data_send(u8 report_id, u8 *data, u16 len);
static void ble_mouse_timer_handler(void)
{
#if TCFG_USER_BLE_ENABLE

    if (!ble_hid_is_connected()) {
        return;
    }

    if (button_send_flag == 0 || wheel_send_flag == 0) {
        /* log_info_hexdump(&first_packet.data, sizeof(first_packet.data)); */
        ble_hid_data_send(1, &first_packet.data, sizeof(first_packet.data));

        if (button_send_flag == 0) {
            button_send_flag = 1;
        }

        if (wheel_send_flag == 0) {
            wheel_send_flag = 1;
            first_packet.data[WHEEL_IDX] = 0;
        }
    }

    if (sensor_send_flag == 0) {
        ble_hid_data_send(2, &second_packet.data, sizeof(second_packet.data));
        memset(&second_packet.data, 0, sizeof(second_packet.data));
        sensor_send_flag = 1;
        MO_IO_DEBUG_TOGGLE(B, 1);
    }

#if TCFG_OMSENSOR_ENABLE
    optical_mouse_sensor_read_motion_handler();
#endif

#endif
}
extern int edr_hid_is_connected(void);
extern int edr_hid_tx_buff_is_ok(void);
extern int edr_hid_data_send(u8 report_id, u8 *data, u16 len);
static void edr_mouse_timer_handler(void)
{
#if TCFG_USER_EDR_ENABLE

    static u8 timer_exit_cnt = 0;
    if ((!edr_hid_is_connected()) || (!edr_hid_tx_buff_is_ok())) {
        return;
    }
    if (button_send_flag == 0 || wheel_send_flag == 0) {
        /* log_info_hexdump(&first_packet.data, sizeof(first_packet.data)); */
        edr_hid_data_send(1, &first_packet.data, sizeof(first_packet.data));

        if (button_send_flag == 0) {
            button_send_flag = 1;
        }

        if (wheel_send_flag == 0) {
            wheel_send_flag = 1;
            first_packet.data[WHEEL_IDX] = 0;
        }
#if TCFG_USER_EDR_ENABLE
        bt_comm_edr_sniff_clean();
#endif
    }

    if (sensor_send_flag == 0) {
        edr_hid_data_send(2, &second_packet.data, sizeof(second_packet.data));
        memset(&second_packet.data, 0, sizeof(second_packet.data));
        sensor_send_flag = 1;
#if TCFG_USER_EDR_ENABLE
        bt_comm_edr_sniff_clean();
#endif
        MO_IO_DEBUG_TOGGLE(B, 1);
    }

#if TCFG_OMSENSOR_ENABLE
    optical_mouse_sensor_read_motion_handler();
#endif

#endif
}



extern void p33_soft_reset(void);
static void power_set_soft_reset(void)
{
    p33_soft_reset();
    while (1);
}

/*根据配置好的模式切换*/
static void mode_switch_handler(void)
{
    log_info("mode_switch_handler");
    int switch_sucess = 0;

    /* if (edr_hid_is_connected() || ble_hid_is_connected()) { */
    /* log_info("fail, disconnect bt firstly!!!\n"); */
    /* return; */
    /* } */

    //切换的顺序loop:ble->edr->24g
    is_hid_active = 1;
    if (HID_MODE_BLE == bt_hid_mode) {
#if (SWITCH_MODE_EDR_ENABLE || SWITCH_MODE_24G_ENABLE)

#if TCFG_USER_BLE_ENABLE
        if (ble_hid_is_connected()) {
            log_info("disconnect ble\n");
            ble_module_enable(0);
            os_time_dly(10);
        }
#endif
        switch_sucess = 1;

#if SWITCH_MODE_EDR_ENABLE
        mouse_select_btmode(HID_MODE_EDR);
#else
        mouse_select_btmode(HID_MODE_24G);
#endif

#endif
    } else if (HID_MODE_EDR == bt_hid_mode) {

#if (SWITCH_MODE_BLE_ENABLE || SWITCH_MODE_24G_ENABLE)
#if TCFG_USER_EDR_ENABLE
        if (edr_hid_is_connected()) {
            log_info("disconnect edr\n");
            user_hid_disconnect();
            os_time_dly(20);
        }
#endif
        switch_sucess = 1;

        if (SWITCH_MODE_24G_ENABLE) { /**/
            mouse_select_btmode(HID_MODE_24G);
        } else {
            mouse_select_btmode(HID_MODE_BLE);
        }
#endif

    } else if (HID_MODE_24G == bt_hid_mode) {
#if (SWITCH_MODE_BLE_ENABLE || SWITCH_MODE_EDR_ENABLE)
#if TCFG_USER_BLE_ENABLE
        if (ble_hid_is_connected()) {
            log_info("disconnect 24g\n");
            ble_module_enable(0);
            os_time_dly(10);
        }
#endif
        switch_sucess = 1;

#if (SWITCH_MODE_BLE_ENABLE)
        mouse_select_btmode(HID_MODE_BLE);
#elif(SWITCH_MODE_EDR_ENABLE)
        mouse_select_btmode(HID_MODE_EDR);
#endif
#endif

    }

    if (switch_sucess) {
        log_info("switch success\n");
        sys_timeout_add(NULL, power_set_soft_reset, WAIT_DISCONN_TIME_MS);
    } else {
        log_info("switch fail\n");
        is_hid_active = 0;
    }

}

static void app_code_sw_event_handler(struct sys_event *event)
{
    static s8 sw_val = 0;

    if (wheel_send_flag) {
        sw_val = 0;
    }

    if (event->u.codesw.event == 0) {
        /* log_info("sw_val = %d\n", event->u.codesw.value); */
        sw_val += event->u.codesw.value;
        first_packet.data[WHEEL_IDX] = -sw_val;
    }

    wheel_send_flag = 0;
}

static void app_optical_sensor_event_handler(struct sys_event *event)
{
    static s16 delta_x = 0, delta_y = 0;
    if (sensor_send_flag) {
        delta_x = 0;
        delta_y = 0;
    }

    if (event->u.axis.event == 0) {
        if (((delta_x + event->u.axis.x) >= -2047) && ((delta_x + event->u.axis.x) <= 2047)) {
        } else {
            event->u.axis.x = 0;
        }

        if (((delta_y + event->u.axis.y) >= -2047) && ((delta_y + event->u.axis.y) <= 2047)) {
        } else {
            event->u.axis.y = 0;
        }


#if 1
        delta_x += event->u.axis.x;
        delta_y += event->u.axis.y;
#else
        //坐标调整
        delta_x += (-event->u.axis.y);
        delta_y += (event->u.axis.x);
        /* delta_x = (-event->u.axis.y); */
        /* delta_y = (event->u.axis.x); */
#endif

        second_packet.data[SENSOR_XLSB_IDX] = delta_x & 0xFF;
        second_packet.data[SENSOR_YLSB_XMSB_IDX] = ((delta_y << 4) & 0xF0) | ((delta_x >> 8) & 0x0F);
        second_packet.data[SENSOR_YMSB_IDX] = (delta_y >> 4) & 0xFF;
        second_packet.event_arg = event->arg;

        //log_info("x = %d.\ty = %d.\n", event->u.axis.x, event->u.axis.y);
    }

    sensor_send_flag = 0;
}



//----------------------------------

typedef struct {
    u16  head_tag;
    u8   mode;
    u8   res;
} hid_vm_cfg_t;

#define	HID_VM_HEAD_TAG (0x3AA3)
static void mouse_vm_deal(u8 rw_flag)
{
    hid_vm_cfg_t info;
    int ret;
    int vm_len = sizeof(hid_vm_cfg_t);

    log_info("-hid_info vm_do:%d\n", rw_flag);
    memset(&info, 0, vm_len);

    if (rw_flag == 0) {

        //default set
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
            /*第一次上电，默认模式优先顺序*/
#if SWITCH_MODE_BLE_ENABLE
            bt_hid_mode = HID_MODE_BLE;
#elif SWITCH_MODE_EDR_ENABLE
            bt_hid_mode = HID_MODE_EDR;
#else
            bt_hid_mode = HID_MODE_24G;
#endif
        } else {
            /*修改模式后，判断VM记录的模式是否还存在*/
            if (0 == SWITCH_MODE_24G_ENABLE && bt_hid_mode == HID_MODE_24G) {
                bt_hid_mode = HID_MODE_BLE;
            }

            if (0 == SWITCH_MODE_BLE_ENABLE && bt_hid_mode == HID_MODE_BLE) {
                bt_hid_mode = HID_MODE_EDR;
            }

            if (0 == SWITCH_MODE_EDR_ENABLE && bt_hid_mode == HID_MODE_EDR) {
#if SWITCH_MODE_BLE_ENABLE
                bt_hid_mode = HID_MODE_BLE;
#else
                bt_hid_mode = HID_MODE_24G;
#endif
            }
        }

        if (bt_hid_mode != info.mode) {
            log_info("-write00,mdy mode--\n");
            info.mode = bt_hid_mode;
            syscfg_write(CFG_AAP_MODE_INFO, (u8 *)&info, vm_len);
        }
    } else {
        //wm write
        info.mode = bt_hid_mode;
        info.head_tag = HID_VM_HEAD_TAG;
        info.res = 0;

        hid_vm_cfg_t tmp_info;
        syscfg_read(CFG_AAP_MODE_INFO, (u8 *)&tmp_info, vm_len);

        if (memcmp(&tmp_info, &info, vm_len)) {
            syscfg_write(CFG_AAP_MODE_INFO, (u8 *)&info, vm_len);
            log_info("-write11,new mode--\n");
            log_info_hexdump((u8 *)&info, vm_len);
        }

    }
}

void mouse_power_event_to_user(u8 event)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = (void *)DEVICE_EVENT_FROM_POWER;
    e.u.dev.event = event;
    e.u.dev.value = 0;
    sys_event_notify(&e);
}

static void mouse_set_soft_poweroff(void)
{
    log_info("mouse_set_soft_poweroff\n");
    is_hid_active = 1;
    //必须先主动断开蓝牙链路,否则要等链路超时断开
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

static void mouse_timer_handle_test(void)
{
    log_info("not_bt");
}

extern void bt_pll_para(u32 osc, u32 sys, u8 low_power, u8 xosc);
static void mouse_app_start()
{
    log_info("=======================================");
    log_info("-------------Single Mouse--------------");
    log_info("=======================================");
    log_info("app_file: %s", __FILE__);

    clk_set("sys", BT_NORMAL_HZ);
    //有蓝牙
#if (TCFG_USER_EDR_ENABLE || TCFG_USER_BLE_ENABLE)
    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);

#if TCFG_USER_EDR_ENABLE
    btstack_edr_start_before_init(&mouse_edr_config, 0);
#endif

#if TCFG_USER_BLE_ENABLE
    btstack_ble_start_before_init(&mouse_ble_config, 0);
    le_hogp_set_reconnect_adv_cfg(ADV_IND, 5000);
#endif

    btstack_init();

#else
    //no bt,to for test
    sys_timer_add(NULL, mouse_timer_handle_test, 1000);
#endif

#if TCFG_USER_EDR_ENABLE
    sys_auto_sniff_controle(1, NULL);
#endif

    /* 按键消息使能 */
    sys_key_event_enable();

#if (TCFG_HID_AUTO_SHUTDOWN_TIME)
    //无操作定时软关机
    g_auto_shutdown_timer = sys_timeout_add((void *)POWER_EVENT_POWER_SOFTOFF, mouse_power_event_to_user, TCFG_HID_AUTO_SHUTDOWN_TIME * 1000);
#endif
    /* sys_auto_shut_down_enable(); */
    /* sys_auto_sniff_controle(1, NULL); */
}

static int mouse_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        if (!it) {
            break;
        }
        switch (it->action) {
        case ACTION_MOUSE_MAIN:
            mouse_app_start();
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

extern void set_remote_test_flag(u8 own_remote_test);
static int mouse_bt_hci_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_info("------------------------mouse_bt_hci_event_handler reason %x %x", bt->event, bt->value);

#if TCFG_USER_EDR_ENABLE
    bt_comm_edr_hci_event_handler(bt);
#endif

#if TCFG_USER_BLE_ENABLE
    bt_comm_ble_hci_event_handler(bt);
#endif

    return 0;
}

extern void mouse_board_devices_init(void);
static int mouse_bt_connction_status_event_handler(struct bt_event *bt)
{

    log_info("-----------------------mouse_bt_connction_status_event_handler %d", bt->event);

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        /*
         * 蓝牙初始化完成
         */
        log_info("BT_STATUS_INIT_OK\n");
        mouse_board_devices_init();

        //read vm first
        mouse_vm_deal(0);//bt_hid_mode read for VM

#if TCFG_USER_BLE_ENABLE

#if DOUBLE_KEY_HOLD_PAIR
        //2.4g&ble 配对管理，默认绑定配对设备
        le_hogp_set_pair_config(10, 1);
#endif

        ble_24g_code = CFG_RF_24G_CODE_ID;
        log_info("##config_24g_code: %04x", CFG_RF_24G_CODE_ID);
        ble_set_fix_pwr(9);//range:0~9
#endif

        if (bt_hid_mode == HID_MODE_BLE || bt_hid_mode == HID_MODE_24G) {
            if (bt_hid_mode == HID_MODE_BLE) {
                rf_set_24g_hackable_coded(0);
            } else {
                log_info("##init_24g_code: %04x", ble_24g_code);
                rf_set_24g_hackable_coded(ble_24g_code);
            }

#if TCFG_USER_BLE_ENABLE
            btstack_ble_start_after_init(0);/*auto call ble_module_enable(1)*/
#endif

        } else {
#if TCFG_USER_EDR_ENABLE
            btstack_edr_start_after_init(0);
#endif
        }

        mouse_select_btmode(HID_MODE_INIT);//

        if (bt_hid_mode == HID_MODE_BLE || bt_hid_mode == HID_MODE_24G) {
#if TCFG_USER_BLE_ENABLE
            ble_hid_timer_handle = sys_s_hi_timer_add((void *)0, ble_mouse_timer_handler, 10);
            /* ble_module_enable(1); */
#endif
        } else {
#if TCFG_USER_EDR_ENABLE
            edr_hid_timer_handle = sys_s_hi_timer_add((void *)0, edr_mouse_timer_handler, EDR_SET_TIMER_VALUE);
            user_hid_enable(1);
            if (!bt_connect_phone_back_start()) {
                bt_wait_phone_connect_control(1);
            }
#endif
        }
        break;

    default: {
#if TCFG_USER_EDR_ENABLE
        bt_comm_edr_status_event_handler(bt);
#endif

#if TCFG_USER_BLE_ENABLE
        bt_comm_ble_status_event_handler(bt);
#endif
    }
    break;

    }
    return 0;

}


static int mouse_bt_common_event_handler(struct bt_event *bt)
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
        mouse_auto_shutdown_disable();
        break;

    default:
        break;

    }
    return 0;
}

/*清配对表，重新可以连接可发现*/
static void mouse_bt_pair_start(void)
{
    if (bt_hid_mode == HID_MODE_EDR) {
#if TCFG_USER_EDR_ENABLE
        if (edr_hid_is_connected()) {
            log_info("disconnect edr\n");
            user_hid_disconnect();
            user_send_cmd_prepare(USER_CTRL_DEL_ALL_REMOTE_INFO, 0, NULL);
        }
        log_info("#edr enter wait pair....");
#endif
    } else {
#if TCFG_USER_BLE_ENABLE
        if (ble_hid_is_connected()) {
            log_info("disconnect ble\n");
            ble_module_enable(0);
            os_time_dly(10);
        } else {
            ble_module_enable(0);
        }

        if (bt_hid_mode == HID_MODE_24G) {
            log_info("#2.4g enter wait pair....");
        } else {
            log_info("#ble enter wait pair....");
        }

        ble_set_fix_pwr(6);//range:0~9
        le_hogp_set_pair_allow();
        ble_module_enable(1);
#endif
    }
}

static void mouse_key_event_handler(struct sys_event *event)
{
    u16 cpi = 0;
    u8 event_type = 0;

    if (event->arg == (void *)DEVICE_EVENT_FROM_KEY) {
        log_info("key_value = %d.\tevent_type = %d.\n", event->u.key.value, event->u.key.event);

#if TEST_MOUSE_SIMULATION_ENABLE
        if (event->u.key.event == 1) {
            mouse_test_ctrl();
            return;
        }
        return;
#endif

        event_type = event->u.key.event;

//中键+left键切换模式,长按数秒进入
#if MIDDLE_KEY_SWITCH
        if (5 == event->u.key.value && event_type == KEY_EVENT_LONG) {
            log_info("switch:double_key5 hold:%d", switch_key_long_cnt);
            if (++switch_key_long_cnt >= MIDDLE_KEY_HOLD_CNT) {
                mode_switch_handler();
                switch_key_long_cnt = 0;
            }
            return;
        } else {
            switch_key_long_cnt = 0;
        }
#endif

//中键+right键, 长按数秒进入
#if (DOUBLE_KEY_HOLD_PAIR)
        if (6 == event->u.key.value && event_type == KEY_EVENT_LONG) {
            log_info("pair_start:double key6 hold:%d", double_key_long_cnt);
            if (++double_key_long_cnt >= DOUBLE_KEY_HOLD_CNT) {
                mouse_bt_pair_start();
                double_key_long_cnt = 0;
            }
            return;
        } else {
            double_key_long_cnt = 0;
        }
#endif

        first_packet.data[BUTTONS_IDX] = 0;
        if (event_type == KEY_EVENT_CLICK || \
            event_type == KEY_EVENT_LONG || \
            event_type == KEY_EVENT_HOLD) {
            first_packet.data[BUTTONS_IDX] |= event->u.key.value;
        }

    }

    if (button_send_flag) {
        button_send_flag = 0;
    }
}


static int mouse_event_handler(struct application *app, struct sys_event *event)
{
#if (TCFG_HID_AUTO_SHUTDOWN_TIME)
    //重置无操作定时计数
    if (event->type != SYS_DEVICE_EVENT || DEVICE_EVENT_FROM_POWER != event->arg) { //过滤电源消息
        sys_timer_modify(g_auto_shutdown_timer, TCFG_HID_AUTO_SHUTDOWN_TIME * 1000);
    }
#endif

#if TCFG_USER_EDR_ENABLE
    bt_comm_edr_sniff_clean();
    /* bt_sniff_ready_clean(); */
#endif
    /* log_info("event: %s", event->arg); */
    switch (event->type) {
    case SYS_KEY_EVENT:
        /* log_info("Sys Key : %s", event->arg); */
        mouse_key_event_handler(event);
        return 0;

    case SYS_BT_EVENT:
#if (TCFG_USER_EDR_ENABLE || TCFG_USER_BLE_ENABLE)
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            mouse_bt_connction_status_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
            mouse_bt_hci_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_FORM_COMMON) {
            return mouse_bt_common_event_handler(&event->u.dev);
        }
#endif
        return 0;

    case SYS_DEVICE_EVENT:
        if (event->arg == "code_switch") {
            app_code_sw_event_handler(event);
        } else if (event->arg == "omsensor_axis") {
            app_optical_sensor_event_handler(event);
        } else if ((u32)event->arg == DEVICE_EVENT_FROM_POWER) {
            return app_power_event_handler(&event->u.dev, mouse_set_soft_poweroff);
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


extern void bredr_power_get(void);
extern void bredr_power_put(void);
extern void radio_set_eninv(int v);
void ble_module_enable(u8 en);
static void mouse_select_btmode(u8 mode)
{
    if (mode != HID_MODE_INIT) {
        if (bt_hid_mode == mode) {
            return;
        }
        bt_hid_mode = mode;
    } else {
        //init start,上电初始化
    }

    log_info("###### %s: %d,%d\n", __FUNCTION__, mode, bt_hid_mode);

    if (bt_hid_mode == HID_MODE_BLE || bt_hid_mode == HID_MODE_24G) {

        if (bt_hid_mode == HID_MODE_BLE) {
            log_info("---------app select ble--------\n");
            rf_set_24g_hackable_coded(0);
        } else {
            log_info("---------app select 24g--------\n");
            log_info("set_24g_code: %04x", ble_24g_code);
            rf_set_24g_hackable_coded(ble_24g_code);
        }

        //ble
        if (!STACK_MODULES_IS_SUPPORT(BT_BTSTACK_LE) || !BT_MODULES_IS_SUPPORT(BT_MODULE_LE)) {
            log_info("not surpport ble,make sure config !!!\n");
            ASSERT(0);
        }

#if TCFG_USER_EDR_ENABLE
        user_hid_enable(0);
        bt_wait_phone_connect_control(0);
#endif

#if TCFG_USER_EDR_ENABLE
        //close edr
#ifndef CONFIG_NEW_BREDR_ENABLE
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

    }

    trace_run_debug_val(0);
    mouse_vm_deal(1);
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

static const struct application_operation app_mouse_ops = {
    .state_machine  = mouse_state_machine,
    .event_handler 	= mouse_event_handler,
};

/*
 * 注册AT Module模式
 */
REGISTER_APPLICATION(app_mouse) = {
    .name 	= "mouse_single",
    .action	= ACTION_MOUSE_MAIN,
    .ops 	= &app_mouse_ops,
    .state  = APP_STA_DESTROY,
};



#if TEST_MOUSE_SIMULATION_ENABLE
#if TCFG_OMSENSOR_ENABLE || TCFG_CODE_SWITCH_ENABLE
#error "please disable this enable!!!!"
#endif

static char RF_dcnt = 0;
static short int RF_dx = 20;
static short int RF_dy = 20;

typedef struct MOUSE_VARIABLE_T {
    unsigned char button;
    short int     sensor_x;
    short int     sensor_y;
    unsigned char sensor_cpi;
    unsigned char wheel;
} MOUSE_VARIABLE_T, *MOUSE_VARIABLE_P;

static MOUSE_VARIABLE_T mouse_val;

static void mouse_send_data_test(void)
{
    static int16_t deltaX, deltaY;
    static uint8_t dcount = 100;

    MO_IO_DEBUG_TOGGLE(B, 0);

    dcount++;
    if (dcount >= 100) {
        dcount = 0;
        if ((deltaX == 0) && (deltaY == 4)) {
            deltaX = 4;
            deltaY = 0;
        } else if ((deltaX == 4) && (deltaY == 0)) {
            deltaX = 0;
            deltaY = -4;
        } else if ((deltaX == 0) && (deltaY == -4)) {
            deltaX = -4;
            deltaY = 0;
        } else if ((deltaX == -4) && (deltaY == 0)) {
            deltaX = 0;
            deltaY = 4;
        } else {
            deltaX = 4;
            deltaY = 0;
        }
    }
    /* putchar('('); */
    //    log_info("(");
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg = "omsensor_axis";
    e.u.axis.event = 0;
    e.u.axis.x = deltaX;
    e.u.axis.y = deltaY;
    //sys_event_notify(&e); //线程调度最快只有10ms,回包率提不上去
    app_optical_sensor_event_handler(&e);

#if (TCFG_HID_AUTO_SHUTDOWN_TIME)
    sys_timer_modify(g_auto_shutdown_timer, TCFG_HID_AUTO_SHUTDOWN_TIME * 1000);
#endif
}

static void mouse_test_ctrl(void)
{
    static u16 loop = 0;
    if (loop) {
        sys_hi_timer_del(loop);
        loop = 0;
        is_hid_active = 0;
    } else {
        is_hid_active = 1;
        loop = sys_s_hi_timer_add(NULL, mouse_send_data_test, EDR_SET_TIMER_VALUE);//提高定时器精度到
    }
}
#endif

#endif

