/*********************************************************************************************
    *   Filename        : app_mesh.c

    *   Description     :

    *   Author          : Bingquan

    *   Email           : caibingquan@zh-jieli.com

    *   Last modifiled  : 2019-07-22 14:01

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

#define LOG_TAG             "[app_mesh]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define WAIT_DISCONN_TIME_MS     (300)

extern const u8 *bt_get_mac_addr();
extern void lib_make_ble_address(u8 *ble_address, u8 *edr_address);
extern void ble_module_enable(u8 en);
extern void bt_pll_para(u32 osc, u32 sys, u8 low_power, u8 xosc);
extern void input_key_handler(u8 key_status, u8 key_number);
extern void bt_ble_init(void);
extern void bt_ble_adv_enable(u8 enable);

static u8 is_app_active = 0;

static void bt_function_select_init()
{
    u8 tmp_ble_addr[6];
    /* le_controller_set_mac((void*)"012345"); */
    lib_make_ble_address(tmp_ble_addr, (void *)bt_get_mac_addr());
    le_controller_set_mac((void *)tmp_ble_addr);

    printf("\n-----edr + ble 's address-----");
    printf_buf((void *)bt_get_mac_addr(), 6);
    printf_buf((void *)tmp_ble_addr, 6);
    /* bt_set_tx_power(9);//0~9 */
}

static void app_set_soft_poweroff(void)
{
    log_info("set_soft_poweroff\n");
    is_app_active = 1;

    //必须先主动断开蓝牙链路,否则要等链路超时断开
    ble_module_enable(0);

    //延时300ms，确保BT退出链路断开
    sys_timeout_add(NULL, power_set_soft_poweroff, WAIT_DISCONN_TIME_MS);
}

static void app_start()
{
    log_info("=======================================");
    log_info("-------------BLE MESH DEMO-------------");
    log_info("=======================================");

    is_app_active = 1;

    clk_set("sys", BT_NORMAL_HZ);
    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);

    bt_function_select_init();

    btstack_init();

    /* 按键消息使能 */
    sys_key_event_enable();
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
        case ACTION_AT_MAIN:
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

#define HCI_EVENT_VENDOR_REMOTE_TEST                          0xFE

static int bt_hci_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_info("------------------------bt_hci_event_handler reason %x %x", bt->event, bt->value);

    if (bt->event == HCI_EVENT_VENDOR_REMOTE_TEST) {
        if (0 == bt->value) {
            log_info("clear_test_box_flag");
            return 0;
        } else {
            //1:edr con;2:ble con;
            if (1 == bt->value) {
                bt_ble_adv_enable(0);
            }
        }
    }

    return 0;
}


static int bt_connction_status_event_handler(struct bt_event *bt)
{

    printf("-----------------------bt_connction_status_event_handler %d", bt->event);

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        /*
         * 蓝牙初始化完成
         */
        log_info("BT_STATUS_INIT_OK\n");

        if (BT_MODE_IS(BT_BQB)) {
            void ble_bqb_test_thread_init(void);
            ble_bqb_test_thread_init();
        } else {
#if TCFG_NORMAL_SET_DUT_MODE
            log_info("set dut mode\n");
            extern void ble_standard_dut_test_init(void);
            ble_standard_dut_test_init();
#else
            extern void bt_ble_init(void);
            bt_ble_init();
#endif
        }
        /* bt_ble_init(); */
        is_app_active = 0;

        break;

    default:
        log_info(" BT STATUS DEFAULT\n");
        break;
    }
    return 0;
}

static void app_key_event_handler(struct sys_event *event)
{
    u8 event_type = 0;
    u8 key_value = 0;

    if (event->arg == (void *)DEVICE_EVENT_FROM_KEY) {
        event_type = event->u.key.event;
        key_value = event->u.key.value;
        printf("app_key_evnet: %d,%d\n", event_type, key_value);
        input_key_handler(event_type, key_value);

        if (event_type == KEY_EVENT_LONG && key_value == TCFG_ADKEY_VALUE6) {
            app_set_soft_poweroff();
        }
    }
}


static int event_handler(struct application *app, struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
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
        return 0;

    default:
        return FALSE;
    }
    return FALSE;
}

static const struct application_operation app_at_ops = {
    .state_machine  = state_machine,
    .event_handler 	= event_handler,
};

/*
 * 注册AT Module模式
 */
REGISTER_APPLICATION(app_mesh) = {
    .name 	= "mesh",
    .action	= ACTION_AT_MAIN,
    .ops 	= &app_at_ops,
    .state  = APP_STA_DESTROY,
};

//-----------------------
//system check go sleep is ok
static u8 app_state_idle_query(void)
{
    return !is_app_active;
}

REGISTER_LP_TARGET(app_state_lp_target) = {
    .name = "app_state_deal",
    .is_idle = app_state_idle_query,
};
