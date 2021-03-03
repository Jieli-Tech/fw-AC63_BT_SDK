/*********************************************************************************************
    *   Filename        : app_mouse.c

    *   Description     :

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
#include "hid_user.h"
/* #include "code_switch.h" */
/* #include "omsensor/OMSensor_manage.h" */
#include "le_common.h"
/* #include <stdlib.h>  */
#include "rcsp_bluetooth.h"
#include "rcsp_user_update.h"
#include "app_charge.h"
#include "app_power_manage.h"
#include "app_config.h"

#if(CONFIG_APP_MOUSE)

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
#define CFG_RF_24G_CODE_ID       (0) //<=24bits
/* #define CFG_RF_24G_CODE_ID       (0x23) //<=24bits */

//切换控制,可自己修改按键方式
#define SWITCH_MODE_BLE_24G_EDR    1   //BLE,2.4G,EDR ; 3个模式切换, 要使能 EDR BLE 模块
#define MIDDLE_KEY_SWITCH          1   //中键长按数秒切换 edr & ble , or 2.4g & ble, or ble & 2.4g & edr
#define MIDDLE_KEY_HOLD_CNT       (4)  //长按中键计算次数 >= 4s


//使能开配对管理，BLE & 2.4g 鼠标只绑定1个主机,可自己修改按键方式
#define DOUBLE_KEY_HOLD_PAIR      (0 & CFG_RF_24G_CODE_ID)  //中键+右键 长按数秒,进入2.4G配对模式
#define DOUBLE_KEY_HOLD_CNT       (4)  //长按中键计算次数 >= 4s

static u8 switch_key_long_cnt = 0;
static u8 double_key_long_cnt = 0;

static u16 g_auto_shutdown_timer = 0;

int sniff_timer = 0;
void bt_check_exit_sniff();

extern void bt_set_osc_cap(u8 sel_l, u8 sel_r);
extern const u8 *bt_get_mac_addr();
extern void lib_make_ble_address(u8 *ble_address, u8 *edr_address);


/* static const u8 bt_address_default[] = {0x11, 0x22, 0x33, 0x66, 0x77, 0x88}; */
static void app_select_btmode(u8 mode);
void sys_auto_sniff_controle(u8 enable, u8 *addr);
void bt_sniff_ready_clean(void);
static void hid_vm_deal(u8 rw_flag);

extern int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type);

extern int edr_hid_is_connected(void);
extern int ble_hid_is_connected(void);
void ble_set_fix_pwr(u8 fix);//set tx power
void le_hogp_set_pair_config(u8 pair_max, u8 is_allow_cover);
void le_hogp_set_pair_allow(void);

#define WAIT_DISCONN_TIME_MS     (300)

typedef enum {
    HID_MODE_NULL = 0,
    HID_MODE_EDR,
    HID_MODE_BLE,
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


extern int ble_hid_timer_handle;
extern int edr_hid_timer_handle;
static u8 button_send_flag = 0;
static u8 wheel_send_flag = 0;
static u8 sensor_send_flag = 0;

/* static u16 auto_shutdown_timer = 0; */
static volatile mouse_packet_data_t first_packet = {0};
static volatile mouse_packet_data_t second_packet = {0};

static const u8 hid_report_map[] = {
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

void auto_shutdown_disable(void)
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
        /* memset(&second_packet.data, 0, sizeof(second_packet.data)); */
        sensor_send_flag = 1;
    }
#endif
}

extern int  edr_hid_tx_buff_is_ok(void);
extern void edr_hid_data_send(u8 report_id, u8 *data, u16 len);
static void edr_mouse_timer_handler(void)
{
#if TCFG_USER_EDR_ENABLE

    static u8 timer_exit_cnt = 0;

    if ((!edr_hid_is_connected()) || (!edr_hid_tx_buff_is_ok())) {
        return;
    }

    if (!sniff_timer) {
        if (!(button_send_flag == 0 || wheel_send_flag == 0 || sensor_send_flag == 0)) {
            //no data
            return;
        }

        if (timer_exit_cnt) {
            timer_exit_cnt--;
        } else {
            timer_exit_cnt = 15; //150ms
            //send exit sniff
            bt_check_exit_sniff();
        }
        return;
    }

    timer_exit_cnt = 0;

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
        bt_sniff_ready_clean();
    }

    if (sensor_send_flag == 0) {
        edr_hid_data_send(2, &second_packet.data, sizeof(second_packet.data));
        memset(&second_packet.data, 0, sizeof(second_packet.data));
        sensor_send_flag = 1;
        bt_sniff_ready_clean();
    }
#endif
}



extern void p33_soft_reset(void);
static void power_set_soft_reset(void)
{
    p33_soft_reset();
    while (1);
}

static void mode_switch_handler(void)
{
    log_info("mode_switch_handler");
#if (TCFG_USER_EDR_ENABLE && TCFG_USER_BLE_ENABLE)

    if (edr_hid_is_connected() || ble_hid_is_connected()) {
        log_info("fail, disconnect bt firstly!!!\n");
        return;
    }

    is_hid_active = 1;
    if (HID_MODE_BLE == bt_hid_mode) {
        app_select_btmode(HID_MODE_EDR);
    } else {
        app_select_btmode(HID_MODE_BLE);
    }
    sys_timeout_add(NULL, power_set_soft_reset, WAIT_DISCONN_TIME_MS);
#endif

}

static void mode_switch_24g(void)
{
    /* log_info("mode_switch_24g"); */

#if TCFG_USER_BLE_ENABLE
    ble_module_enable(0);
    if (ble_24g_code) {
        ble_24g_code = 0;
        log_info("##switch to ble_mode: %04x", ble_24g_code);
    } else {
        ble_24g_code = CFG_RF_24G_CODE_ID;
        log_info("##switch to 24g_mode: %04x", ble_24g_code);
    }
    rf_set_24g_hackable_coded(ble_24g_code);
    hid_vm_deal(1);
    ble_module_enable(1);
#endif
}

static void mode_switch_ble_24g_edr(void)
{
    /* log_info("mode_switch_ble_24g_edr"); */

    if (bt_hid_mode == HID_MODE_EDR) {
        ble_24g_code = 0;
        rf_set_24g_hackable_coded(ble_24g_code);
        hid_vm_deal(1);
        log_info("##switch to ble_mode\n");
        mode_switch_handler();//switch to ble
    } else {
        if (CFG_RF_24G_CODE_ID && ble_24g_code == 0) {
            log_info("##switch to 24g_mode\n");
            mode_switch_24g();//switch to 2.4g
        } else {
            log_info("##switch to edr_mode\n");
            mode_switch_handler();//switch to edr
        }
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
        first_packet.data[WHEEL_IDX] = sw_val;
    }

    wheel_send_flag = 0;
}

static s16 gradient_acceleration(s16 src)
{
    //不需要加速,由CPI决定速度
    /* #define GRADIENT_1              3 */
    /* #define GRADIENT_2              10 */
    /* #define ACCELERATION_1(x)       (x * 3 / 2) // 1.5 */
    /* #define ACCELERATION_2(x)       (x * 2) // 2 */

    /* if ((src > GRADIENT_2) || (src < -GRADIENT_2)) { */
    /* src = ACCELERATION_2(src); */
    /* } else if ((src > GRADIENT_1) || (src < -GRADIENT_1)) { */
    /* src = ACCELERATION_1(src); */
    /* } */

    return src;
}

static void app_optical_sensor_event_handler(struct sys_event *event)
{
    static s16 delta_x = 0, delta_y = 0;

    if (sensor_send_flag) {
        delta_x = 0;
        delta_y = 0;
    }

    /* if (event->u.axis.event == 0) { */
    if (((delta_x + event->u.axis.x) >= -2047) && ((delta_x + event->u.axis.x) <= 2047)) {
        event->u.axis.x = gradient_acceleration(event->u.axis.x);
        delta_x += event->u.axis.x;
    }

    if (((delta_y + event->u.axis.y) >= -2047) && ((delta_y + event->u.axis.y) <= 2047)) {
        event->u.axis.y = gradient_acceleration(event->u.axis.y);
        delta_y += event->u.axis.y;
    }

    if (sensor_send_flag) {
        second_packet.data[SENSOR_XLSB_IDX] = delta_x & 0xFF;
        second_packet.data[SENSOR_YLSB_XMSB_IDX] = ((delta_y << 4) & 0xF0) | ((delta_x >> 8) & 0x0F);
        second_packet.data[SENSOR_YMSB_IDX] = (delta_y >> 4) & 0xFF;
        second_packet.event_arg = event->arg;
    }

    /* log_info("x = %d.\ty = %d.\n", event->u.axis.x, event->u.axis.y); */
    /* } */

    sensor_send_flag = 0;
}



//----------------------------------

typedef struct {
    u16  head_tag;
    u8   mode;
    u8   res;
    u32  ble_24g_code;
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
                ble_24g_code = info.ble_24g_code;
            }
        }

        if (HID_MODE_NULL == bt_hid_mode) {
#if TCFG_USER_BLE_ENABLE
            bt_hid_mode = HID_MODE_BLE;
            ble_24g_code = CFG_RF_24G_CODE_ID;
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
                info.ble_24g_code = ble_24g_code;
                syscfg_write(CFG_AAP_MODE_INFO, (u8 *)&info, vm_len);
            }
        }
    } else {

        info.mode = bt_hid_mode;
        info.ble_24g_code = ble_24g_code;
        info.head_tag = HID_VM_HEAD_TAG;
        info.res = 0;

        hid_vm_cfg_t tmp_info;
        syscfg_read(CFG_AAP_MODE_INFO, (u8 *)&tmp_info, vm_len);

        if (memcmp(&tmp_info, &info, vm_len)) {
            syscfg_write(CFG_AAP_MODE_INFO, (u8 *)&info, vm_len);
            log_info("-write11--\n");
            log_info_hexdump((u8 *)&info, vm_len);
        }

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
    user_hid_set_icon(BD_CLASS_MOUSE);
    user_hid_set_ReportMap(hid_report_map, sizeof(hid_report_map));
    user_hid_init(NULL);
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


extern void bt_pll_para(u32 osc, u32 sys, u8 low_power, u8 xosc);
static void app_start()
{
    log_info("=======================================");
    log_info("-------------HID Mouse-----------------");
    log_info("=======================================");

    clk_set("sys", BT_NORMAL_HZ);
    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);


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
        case ACTION_MOUSE_MAIN:
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

#define SNIFF_MAX_INTERVALSLOT        800
#define SNIFF_MIN_INTERVALSLOT        100
#define SNIFF_ATTEMPT_SLOT            4
#define SNIFF_TIMEOUT_SLOT            1

static u8 sniff_ready_status = 0; //0:sniff_ready 1:sniff_not_ready
static int exit_sniff_timer = 0;


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
int bt_connect_phone_back_start(void)
{
    if (bt_hid_mode != HID_MODE_EDR) {
        return 0;
    }

    if (connect_last_device_from_vm()) {
        log_info("------bt_connect_phone_start------\n");
        return 1 ;
    }
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



static void bt_hci_event_connection(struct bt_event *bt)
{
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

extern void mouse_board_devices_init(void);
static int bt_connction_status_event_handler(struct bt_event *bt)
{

    log_info("-----------------------bt_connction_status_event_handler %d", bt->event);

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        /*
         * 蓝牙初始化完成
         */
        log_info("BT_STATUS_INIT_OK\n");
        mouse_board_devices_init();

        //read vm first
        hid_vm_deal(0);//bt_hid_mode read for VM

#if TCFG_USER_BLE_ENABLE

#if DOUBLE_KEY_HOLD_PAIR
        //2.4g 配对管理，默认只绑定1个配对设备
        le_hogp_set_pair_config(1, 0);
#endif

        ble_set_fix_pwr(9);//range:0~9
        le_hogp_set_icon(BLE_APPEARANCE_HID_MOUSE);//mouse
        le_hogp_set_ReportMap(hid_report_map, sizeof(hid_report_map));

        log_info("##init_24g_code: %04x", ble_24g_code);
        rf_set_24g_hackable_coded(ble_24g_code);

        bt_ble_init();
#endif

        app_select_btmode(HID_MODE_INIT);//

        if (bt_hid_mode == HID_MODE_BLE) {
#if TCFG_USER_BLE_ENABLE
            ble_hid_timer_handle = sys_s_hi_timer_add((void *)0, ble_mouse_timer_handler, 10);
#endif
        } else {
#if TCFG_USER_EDR_ENABLE
            edr_hid_timer_handle = sys_s_hi_timer_add((void *)0, edr_mouse_timer_handler, 5);
#endif
        }
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

/* static void app_key_event_handler(struct sys_event *event) */
/* { */
/* u8 event_type = 0; */
/* u8 key_value = 0; */

/* if (event->arg == (void *)DEVICE_EVENT_FROM_KEY) {	 */
/* event_type = event->u.key.event; */
/* key_value = event->u.key.value; */
/* printf("app_key_evnet: %d,%d\n",event_type,key_value); */
/* app_key_deal_test(event_type,key_value); */
/* } */
/* } */


static void app_key_event_handler(struct sys_event *event)
{
    u16 cpi = 0;
    u8 event_type = 0;

    if (event->arg == (void *)DEVICE_EVENT_FROM_KEY) {
        log_info("key_value = %d.\tevent_type = %d.\n", event->u.key.value, event->u.key.event);
        event_type = event->u.key.event;

        first_packet.data[BUTTONS_IDX] = 0;

        if (event_type == KEY_EVENT_CLICK || \
            event_type == KEY_EVENT_LONG || \
            event_type == KEY_EVENT_HOLD) {
            first_packet.data[BUTTONS_IDX] |= event->u.key.value;
        }

//中键+右键 长按数秒进入
#if (TCFG_USER_BLE_ENABLE && DOUBLE_KEY_HOLD_PAIR)
        if (bt_hid_mode == HID_MODE_BLE && 2 == event->u.key.value && event_type == KEY_EVENT_LONG) {
            if (ONE_PORT_TO_LOW == gpio_read(TCFG_IOKEY_MOUSE_HK_PORT)) {
                log_info("double key hold1:%d", double_key_long_cnt);
                if (++double_key_long_cnt >= DOUBLE_KEY_HOLD_CNT) {
                    if (ble_hid_is_connected()) {
                        log_info("device disconnect firstly!!!");
                        return;
                    }

                    if (ble_24g_code) {
                        log_info("#2.4g enter wait pair....");
                    } else {
                        log_info("#ble enter wait pair....");
                    }

                    ble_module_enable(0);
                    ble_set_fix_pwr(6);//range:0~9
                    le_hogp_set_pair_allow();
                    ble_module_enable(1);
                    double_key_long_cnt = 0;
                }
                return;
            }
        } else {
            if (bt_hid_mode == HID_MODE_BLE && 1 == event->u.key.value && event_type == KEY_EVENT_LONG) {
                if (ONE_PORT_TO_LOW == gpio_read(TCFG_IOKEY_MOUSE_RK_PORT)) {
                    log_info("double key hold2:%d", double_key_long_cnt);
                    if (++double_key_long_cnt >= DOUBLE_KEY_HOLD_CNT) {
                        if (ble_hid_is_connected()) {
                            log_info("device disconnect firstly!!!");
                            return;
                        }

                        if (ble_24g_code) {
                            log_info("#2.4g enter wait pair....");
                        } else {
                            log_info("#ble enter wait pair....");
                        }

                        ble_module_enable(0);
                        ble_set_fix_pwr(6);//range:0~9
                        le_hogp_set_pair_allow();
                        ble_module_enable(1);
                        double_key_long_cnt = 0;
                    }
                    return;
                }
            }
        }
        double_key_long_cnt = 0;
#endif


//中键长按数秒切换 edr & ble , or 2.4g & ble, or ble & 2.4g & edr
#if MIDDLE_KEY_SWITCH
        if (4 == event->u.key.value && event_type == KEY_EVENT_LONG) {
            log_info("key_value4 long hold:%d", switch_key_long_cnt);
            if (++switch_key_long_cnt >= MIDDLE_KEY_HOLD_CNT) {
                if (TCFG_USER_BLE_ENABLE && CFG_RF_24G_CODE_ID) {

#if SWITCH_MODE_BLE_24G_EDR && TCFG_USER_EDR_ENABLE
                    //定义2.4g,ble 2.4g和ble切换
                    mode_switch_ble_24g_edr();
#else
                    //定义2.4g,默认2.4g和ble切换
                    mode_switch_24g();
#endif
                } else {
                    //edr和ble切换
                    mode_switch_handler();
                }
                switch_key_long_cnt = 0;
            }
        } else {
            switch_key_long_cnt = 0;
        }
#endif

    }

    if (button_send_flag) {
        button_send_flag = 0;
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
        }
        return 0;

    case SYS_DEVICE_EVENT:
        if (event->arg == "code_switch") {
            app_code_sw_event_handler(event);
        } else if (event->arg == "omsensor_axis") {
            app_optical_sensor_event_handler(event);
        } else if ((u32)event->arg == DEVICE_EVENT_FROM_POWER) {
            return app_power_event_handler(&event->u.dev);
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

static const struct application_operation app_mouse_ops = {
    .state_machine  = state_machine,
    .event_handler 	= event_handler,
};

/*
 * 注册AT Module模式
 */
REGISTER_APPLICATION(app_mouse) = {
    .name 	= "mouse",
    .action	= ACTION_MOUSE_MAIN,
    .ops 	= &app_mouse_ops,
    .state  = APP_STA_DESTROY,
};


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
            if (!bt_connect_phone_back_start()) {
                bt_wait_phone_connect_control(1);
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


#endif

