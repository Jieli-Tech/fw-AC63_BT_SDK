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
#include "ble_at_char_client.h"

#define LOG_TAG_CONST       AT_COM
/* #define LOG_TAG             "[AT_COM]" */
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if  CONFIG_APP_AT_CHAR_COM

#if !(TCFG_USER_BLE_ENABLE && (!TCFG_USER_EDR_ENABLE))
#error "board config error, confirm!!!!!!"
#endif

#define TEST_ATCHAR_AUTO_BT_OPEN        0//for test


#define     APP_IO_OUTPUT_0(i,x)      // {JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT &= ~BIT(x);}
#define     APP_IO_OUTPUT_1(i,x)      // {JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT |= BIT(x);}

static u8 is_app_atchar_active = 1;
//---------------------------------------------------------------------
extern void ble_test_auto_scan(u8 en);
extern void ble_test_auto_adv(u8 en);
extern void at_cmd_init(void);
extern void set_at_uart_wakeup(void);
//---------------------------------------------------------------------

void atchar_power_event_to_user(u8 event)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = (void *)DEVICE_EVENT_FROM_POWER;
    e.u.dev.event = event;
    e.u.dev.value = 0;
    sys_event_notify(&e);
}

static void atchar_set_soft_poweroff(void)
{
    log_info("set_soft_poweroff\n");
    is_app_atchar_active = 1;
    //必须先主动断开蓝牙链路,否则要等链路超时断开

#if TCFG_USER_BLE_ENABLE
    btstack_ble_exit(0);
    set_at_uart_wakeup();
    //延时300ms，确保BT退出链路断开
    sys_timeout_add(NULL, power_set_soft_poweroff, WAIT_DISCONN_TIME_MS);
#else
    power_set_soft_poweroff();
#endif
}

static void at_set_soft_poweroff(void)
{
    atchar_set_soft_poweroff();
}

void at_set_low_power_mode(u8 enable)
{
    is_app_atchar_active = !enable;
}

u8 at_get_low_power_mode(void)
{
    return !is_app_atchar_active;
}

#define BLE_AT_TEST_SEND_DATA    0
void ble_at_client_test_senddata(void);
static void atchar_timer_handler(void)
{
    ble_at_client_test_senddata();
}


static void atchar_app_start()
{
    log_info("=======================================");
    log_info("-------------atchar_com demo---------------");
    log_info("=======================================");

    log_info("app_file: %s", __FILE__);

    clk_set("sys", BT_NORMAL_HZ);
#if TCFG_USER_BLE_ENABLE
    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);

    APP_IO_OUTPUT_0(B, 0);


    btstack_ble_start_before_init(NULL, 0);
    btstack_init();
#endif


    /* 按键消息使能 */
    sys_key_event_enable();

#if BLE_AT_TEST_SEND_DATA
    sys_timer_add(NULL, atchar_timer_handler, 1000);
#endif
}

static int atchar_state_machine(struct application *app, enum app_state state, struct intent *it)
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
            atchar_app_start();
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

extern u8 le_att_server_state(void);
static void atchar_timer_to_check_adv_state(void)
{
    static u8 adv_state = 0;

    putchar('H');
    switch (le_att_server_state()) {

    case BLE_ST_ADV:
        adv_state = !adv_state;
        if (adv_state) {
            APP_IO_OUTPUT_1(B, 1);
        } else {
            APP_IO_OUTPUT_0(B, 1);
        }
        break;

    case BLE_ST_NOTIFY_IDICATE:
        APP_IO_OUTPUT_0(B, 1);
        break;

    default:
        APP_IO_OUTPUT_1(B, 1);
        break;
    }
}


static int atchar_bt_hci_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_info("----%s reason %x %x", __FUNCTION__, bt->event, bt->value);

#if TCFG_USER_BLE_ENABLE
    bt_comm_ble_hci_event_handler(bt);
#endif
    return 0;
}

static int atchar_bt_connction_status_event_handler(struct bt_event *bt)
{
    log_info("----%s %d", __FUNCTION__, bt->event);

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        /*
         * 蓝牙初始化完成
         */
        log_info("BT_STATUS_INIT_OK\n");

        at_cmd_init();

        bt_ble_init();

#if TEST_ATCHAR_AUTO_BT_OPEN
        ble_test_auto_adv(1);
        /* ble_test_auto_scan(1); */
#endif
        sys_timer_add(0, atchar_timer_to_check_adv_state, 500);
        break;

    default:
#if TCFG_USER_BLE_ENABLE
        bt_comm_ble_status_event_handler(bt);
#endif
        break;
    }
    return 0;
}

static void atchar_key_event_handler(struct sys_event *event)
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
            atchar_power_event_to_user(POWER_EVENT_POWER_SOFTOFF);
            return;
        }

        if (event_type == KEY_EVENT_DOUBLE_CLICK && key_value == TCFG_ADKEY_VALUE0) {
        }
    }
}

extern void at_cmd_rx_handler(void);
extern cbuffer_t bt_to_uart_cbuf;
static u8 uart_sent_buf[BT_UART_FIFIO_BUFFER_SIZE];
static int atchar_event_handler(struct application *app, struct sys_event *event)
{
    u32 cbuf_len = 0;
    switch (event->type) {
    case SYS_KEY_EVENT:
        atchar_key_event_handler(event);
        return 0;

    case SYS_BT_EVENT:
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            atchar_bt_connction_status_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
            atchar_bt_hci_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_FORM_AT) {
            cbuf_len = cbuf_get_data_size(&bt_to_uart_cbuf);
            if (cbuf_len) {
                cbuf_len = cbuf_read(&bt_to_uart_cbuf, uart_sent_buf, cbuf_len);
                ct_uart_send_packet(uart_sent_buf, cbuf_len);
            }
            /* put_buf(uart_sent_buf, cbuf_len);  */
        }
        return 0;

    case SYS_DEVICE_EVENT:
        if ((u32)event->arg == DEVICE_EVENT_FROM_AT_UART) {
            at_cmd_rx_handler();
        } else if ((u32)event->arg == DEVICE_EVENT_FROM_POWER) {
            return app_power_event_handler(&event->u.dev, atchar_set_soft_poweroff);
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
    .state_machine  = atchar_state_machine,
    .event_handler 	= atchar_event_handler,
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
static u8 app_atchar_state_idle_query(void)
{
    return !is_app_atchar_active;
}

REGISTER_LP_TARGET(app_atcom_lp_target) = {
    .name = "app_state_deal",
    .is_idle = app_atchar_state_idle_query,
};

#endif


