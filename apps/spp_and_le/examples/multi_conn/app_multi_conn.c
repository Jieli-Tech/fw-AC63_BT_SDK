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
#include "app_chargestore.h"
#include "app_power_manage.h"
#include "le_client_demo.h"
#include "app_comm_bt.h"

#define LOG_TAG_CONST       MULTI_CONN
#define LOG_TAG             "[MULTI_CONN]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


#if CONFIG_APP_MULTI

static u8 is_app_multi_active = 0;
//---------------------------------------------------------------------

//----------------------------------------------------------------------------
void multi_set_soft_poweroff(void)
{
    log_info("set_soft_poweroff\n");
    is_app_multi_active = 1;
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

static void multi_app_start()
{
    log_info("=======================================");
    log_info("-----------multi_conn demo-------------");
    log_info("=======================================");
    log_info("app_file: %s", __FILE__);

    clk_set("sys", BT_NORMAL_HZ);

#if (TCFG_USER_EDR_ENABLE || TCFG_USER_BLE_ENABLE)
    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);

#if TCFG_USER_EDR_ENABLE
    btstack_edr_start_before_init(NULL, 0);
#endif

#if TCFG_USER_BLE_ENABLE
    btstack_ble_start_before_init(NULL, 0);
#endif

    btstack_init();

#endif
    /* 按键消息使能 */
    sys_key_event_enable();
}

static int multi_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        if (!it) {
            break;
        }
        switch (it->action) {
        case ACTION_MULTI_MAIN:
            multi_app_start();
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

static int multi_bt_hci_event_handler(struct bt_event *bt)
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

static int multi_bt_connction_status_event_handler(struct bt_event *bt)
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

static void multi_key_event_handler(struct sys_event *event)
{
    /* u16 cpi = 0; */
    u8 event_type = 0;
    u8 key_value = 0;

    if (event->arg == (void *)DEVICE_EVENT_FROM_KEY) {
        event_type = event->u.key.event;
        key_value = event->u.key.value;
        log_info("app_key_evnet: %d,%d\n", event_type, key_value);


        if (event_type == KEY_EVENT_TRIPLE_CLICK
            && (key_value == TCFG_ADKEY_VALUE3 || key_value == TCFG_ADKEY_VALUE0)) {
            //for test
            multi_set_soft_poweroff();
            return;
        }

        if (event_type == KEY_EVENT_CLICK && key_value == TCFG_ADKEY_VALUE0) {

#if TCFG_USER_BLE_ENABLE
#if CONFIG_BT_GATT_CLIENT_NUM
            multi_client_clear_pair();
#endif
#if CONFIG_BT_GATT_SERVER_NUM
            multi_server_clear_pair();
#endif
#endif
        }


        if (event_type == KEY_EVENT_DOUBLE_CLICK && key_value == TCFG_ADKEY_VALUE0) {
#if TCFG_USER_EDR_ENABLE
            //for test
            static u8 edr_en = 1;
            edr_en = !edr_en;
            bt_comm_edr_mode_enable(edr_en);
#endif
        }

        if (event_type == KEY_EVENT_LONG && key_value == TCFG_ADKEY_VALUE0) {
            //for test
            //r_printf("ble_list_clear_all\n");
            //ble_list_clear_all();
#if TCFG_USER_BLE_ENABLE
            static u8 en_value = 1;
            en_value = !en_value;
            ble_module_enable(en_value);
#endif
            return;
        }

    }
}

static int multi_event_handler(struct application *app, struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        multi_key_event_handler(event);
        return 0;

    case SYS_BT_EVENT:
#if (TCFG_USER_EDR_ENABLE || TCFG_USER_BLE_ENABLE)
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            multi_bt_connction_status_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
            multi_bt_hci_event_handler(&event->u.bt);
        }
#endif
        return 0;

    case SYS_DEVICE_EVENT:
        if ((u32)event->arg == DEVICE_EVENT_FROM_POWER) {
            return app_power_event_handler(&event->u.dev, multi_set_soft_poweroff);
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

static const struct application_operation app_multi_ops = {
    .state_machine  = multi_state_machine,
    .event_handler 	= multi_event_handler,
};

/*
 * 注册AT Module模式
 */
REGISTER_APPLICATION(app_multi) = {
    .name 	= "multi_conn",
    .action	= ACTION_MULTI_MAIN,
    .ops 	= &app_multi_ops,
    .state  = APP_STA_DESTROY,
};

//-----------------------
//system check go sleep is ok
static u8 multi_state_idle_query(void)
{
    return !is_app_multi_active;
}

REGISTER_LP_TARGET(multi_state_lp_target) = {
    .name = "multi_state_deal",
    .is_idle = multi_state_idle_query,
};

#endif


