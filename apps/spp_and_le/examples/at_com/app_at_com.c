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
#include "le_client_demo.h"
#include "app_charge.h"
#include "app_power_manage.h"
#include "app_comm_bt.h"
#include "ble_at_client.h"

#define LOG_TAG_CONST       AT_COM
#define LOG_TAG             "[AT_COM]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if CONFIG_APP_AT_COM && USER_SUPPORT_PROFILE_SPP

#if !(TCFG_USER_BLE_ENABLE && TCFG_USER_EDR_ENABLE)
#error "board config error, confirm!!!!!!"
#endif

#define TEST_AUTO_BT_OPEN          0//for test

static u8 is_app_atcom_active = 1;
static struct ble_client_operation_t *atcom_ble_client_api;
//--------------------------------------

#if TRANS_AT_CLIENT
static const client_conn_cfg_t client_at_conn_config = {
    .report_data_callback = NULL,
    .event_callback = NULL,
    .search_uuid_cnt = 0,
    .security_en = 0,
};

static void ble_at_client_config_init(void)
{
    atcom_ble_client_api = ble_get_client_operation_table();
    atcom_ble_client_api->init_config(0, &client_at_conn_config);
}

#endif


void atcom_power_event_to_user(u8 event)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = (void *)DEVICE_EVENT_FROM_POWER;
    e.u.dev.event = event;
    e.u.dev.value = 0;
    sys_event_notify(&e);
}

extern void set_at_uart_wakeup(void);
static void atcom_set_soft_poweroff(void)
{
    log_info("set_soft_poweroff\n");
    is_app_atcom_active = 1;
    //必须先主动断开蓝牙链路,否则要等链路超时断开

#if TCFG_USER_BLE_ENABLE
    btstack_ble_exit(0);
    set_at_uart_wakeup();
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

static void at_set_soft_poweroff(void)
{
    atcom_set_soft_poweroff();
}

void at_set_atcom_low_power_mode(u8 enable)
{
    is_app_atcom_active = !enable;
}


static void atcom_app_start()
{
    log_info("=======================================");
    log_info("-------------at_com demo---------------");
    log_info("=======================================");

    log_info("app_file: %s", __FILE__);

    clk_set("sys", BT_NORMAL_HZ);
    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);

#if TCFG_USER_EDR_ENABLE
    btstack_edr_start_before_init(NULL, 0);
#endif

#if TCFG_USER_BLE_ENABLE
    btstack_ble_start_before_init(NULL, 0);

#if TRANS_AT_CLIENT
    /* atcom_client_config_init(); */
#endif

#endif

    btstack_init();
    /* 按键消息使能 */
    sys_key_event_enable();

}

static int atcom_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        if (!it) {
            break;
        }
        switch (it->action) {
        case ACTION_AT_COM:
            atcom_app_start();
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

static int atcom_bt_hci_event_handler(struct bt_event *bt)
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


extern void ble_test_auto_adv(u8 en);
extern void transport_spp_init(void);
extern void at_cmd_init(void);
static int atcom_bt_connction_status_event_handler(struct bt_event *bt)
{
    log_info("----%s %d", __FUNCTION__, bt->event);
    {
        log_info("-----------------------bt_connction_status_event_handler %d", bt->event);

        switch (bt->event) {
        case BT_STATUS_INIT_OK:
            /*
             * 蓝牙初始化完成
             */
            log_info("BT_STATUS_INIT_OK\n");


#if TCFG_USER_BLE_ENABLE
            extern void bt_ble_init(void);
            bt_ble_init();
#endif

#if TRANS_AT_COM &&  USER_SUPPORT_PROFILE_SPP
            transport_spp_init();
            sys_auto_sniff_controle(1, NULL);
#endif
            at_cmd_init();

#if TEST_AUTO_BT_OPEN
            ble_test_auto_adv(1);
            bt_wait_phone_connect_control_ext(1, 1);
#endif
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

}


static void atcom_key_event_handler(struct sys_event *event)
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
            atcom_power_event_to_user(POWER_EVENT_POWER_SOFTOFF);
            return;
        }

        if (event_type == KEY_EVENT_DOUBLE_CLICK && key_value == TCFG_ADKEY_VALUE0) {
        }
    }
}

extern cbuffer_t at_to_uart_cbuf;
static u8 at_uart_sent_buf[AT_UART_FIFIO_BUFFER_SIZE];
extern void at_cmd_rx_handler(void);
static int atcom_event_handler(struct application *app, struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        atcom_key_event_handler(event);
        return 0;

    case SYS_BT_EVENT:
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            atcom_bt_connction_status_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
            atcom_bt_hci_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_FORM_AT) {
            int cbuf_len = cbuf_get_data_size(&at_to_uart_cbuf);
            if (cbuf_len) {
                cbuf_len = cbuf_read(&at_to_uart_cbuf, at_uart_sent_buf, cbuf_len);
                ct_uart_send_packet(at_uart_sent_buf, cbuf_len);
            }
            /* put_buf(at_uart_sent_buf, cbuf_len);  */
        }
        return 0;

    case SYS_DEVICE_EVENT:
        if ((u32)event->arg == DEVICE_EVENT_FROM_AT_UART) {
            at_cmd_rx_handler();
        } else if ((u32)event->arg == DEVICE_EVENT_FROM_POWER) {
            return app_power_event_handler(&event->u.dev, atcom_set_soft_poweroff);
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



static const struct application_operation app_at_com_ops = {
    .state_machine  = atcom_state_machine,
    .event_handler 	= atcom_event_handler,
};

/*
 * 注册AT Module模式
 */
REGISTER_APPLICATION(app_app_com) = {
    .name 	= "at_com",
    .action	= ACTION_AT_COM,
    .ops 	= &app_at_com_ops,
    .state  = APP_STA_DESTROY,
};

//-----------------------
//system check go sleep is ok
static u8 app_atcom_state_idle_query(void)
{
    return !is_app_atcom_active;
}

REGISTER_LP_TARGET(app_atcom_lp_target) = {
    .name = "app_state_deal",
    .is_idle = app_atcom_state_idle_query,
};

#endif


