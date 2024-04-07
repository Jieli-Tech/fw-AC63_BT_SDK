/*********************************************************************************************
    *   Filename        : app_gamebox.c

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
#include "le_common.h"
#include "rcsp_bluetooth.h"
#include "rcsp_user_update.h"
#include "app_charge.h"
#include "app_power_manage.h"
#include "app_config.h"
#include "usb_hid_keys.h"
#include "gamebox.h"
#include "app_comm_bt.h"

#if(CONFIG_APP_GAMEBOX)

#if TCFG_USER_EDR_ENABLE
//配置需要一致
#error "need disable TCFG_USER_EDR_ENABLE,no support!!!!!!"
#endif

#define LOG_TAG_CONST       GAMEBOX
#define LOG_TAG             "[GAMBOX]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

static u16 g_auto_shutdown_timer = 0;
static u8 is_gamebox_active = 0;//1-临界点,系统不允许进入低功耗，0-系统可以进入低功耗

extern void gamebox_init();
extern int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type);
extern int ble_hid_data_send(u8 report_id, u8 *data, u16 len);
//----------------------------------
static const ble_init_cfg_t gamebox_ble_config = {
    .same_address = 0,
    .appearance = BLE_APPEARANCE_HID_GAMEPAD,
    .report_map = hid_report_desc,
    .report_map_size = sizeof(hid_report_desc),
};

//----------------------------------
#define BUTTONS_IDX							0
#define WHEEL_IDX               			(BUTTONS_IDX + 1)
#define SENSOR_XLSB_IDX         			0
#define SENSOR_YLSB_XMSB_IDX    			(SENSOR_XLSB_IDX + 1)
#define SENSOR_YMSB_IDX         			(SENSOR_YLSB_XMSB_IDX +1)

extern int ble_hid_timer_handle;
static u8 button_send_flag = 0;
static u8 wheel_send_flag = 0;
static u8 sensor_send_flag = 0;

int ble_hid_is_connected(void);
extern int ble_hid_transfer_channel_send(u8 *packet, u16 size);


void send2phone(u32 type, const void *_p)
{
    if (type == 0x32 /*USB_CLASS_HID_MOUSE*/) {
        const struct mouse_data_t *p = _p;
        //usb isr 和ble(timer) 发送放在同一个中断优先级，所以不加互斥
        mouse_data.btn = p->btn;
        mouse_data.wheel += p->wheel;
        mouse_data.x += p->x;
        mouse_data.y += p->y;
        mouse_data_send = 1;
    }

    if (!ble_hid_is_connected()) {
        return ;
    }

    if (type == 0x37) {
        ble_hid_transfer_channel_send(_p, 8);
    } else if (type == 0x33) {
        struct mouse_data_t pp ;
        struct mouse_data_t *p = _p;
        pp.btn =  p->btn;
        p = &pp;
        p->x = -1;
        p->y = -1;
        p->wheel = -1;
        p->ac_pan = -1;
        p->btn &= ~BIT(2);
        if (p->btn) {
            ble_hid_transfer_channel_send(p, 8);
        }
    }
}

void gamebox_auto_shutdown_disable(void)
{
    log_info("----%s", __FUNCTION__);
    if (g_auto_shutdown_timer) {
        sys_timeout_del(g_auto_shutdown_timer);
    }
}

extern void p33_soft_reset(void);
static void gamebox_power_set_soft_reset(void)
{
    p33_soft_reset();
    while (1);
}

static void gamebox_ble_mouse_timer_handler(void)
{
    if (!ble_hid_is_connected()) {
        if (get_phone_connect_status() == BT_MODE) {
            set_phone_connect_status(0);
        }
        return;
    }

    set_phone_connect_status(BT_MODE);

    if (mouse_data_send == 1) {
        int r = ble_hid_data_send(2, &mouse_data, sizeof(mouse_data));
        memset(&mouse_data, 0, sizeof(mouse_data)) ;
        mouse_data_send = 0;
    }

    struct touch_screen_t t;
    memset(&t, 0, sizeof(t));
    if (point_list_pop(&t)) {
        ble_hid_data_send(1, &t, sizeof(t));
    }
}

void gamebox_power_event_to_user(u8 event)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = (void *)DEVICE_EVENT_FROM_POWER;
    e.u.dev.event = event;
    e.u.dev.value = 0;
    sys_event_notify(&e);
}

static void gamebox_set_soft_poweroff(void)
{
    log_info("gamebox_set_soft_poweroff\n");
    is_gamebox_active = 1;
    //必须先主动断开蓝牙链路,否则要等链路超时断开
#if TCFG_USER_BLE_ENABLE
    ble_module_enable(0);
#endif
    sys_timeout_add(NULL, power_set_soft_poweroff, WAIT_DISCONN_TIME_MS);
}

extern void bt_pll_para(u32 osc, u32 sys, u8 low_power, u8 xosc);
static void gamebox_app_start()
{
    gamebox_init();

    log_info("=======================================");
    log_info("------------- gamebox -----------------");
    log_info("=======================================");
    log_info("app_file: %s", __FILE__);

    clk_set("sys", BT_NORMAL_HZ);

    //有蓝牙
#if (TCFG_USER_EDR_ENABLE || TCFG_USER_BLE_ENABLE)
    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);

#if TCFG_USER_BLE_ENABLE
    btstack_ble_start_before_init(&gamebox_ble_config, 0);
    btstack_init();
#endif

#else
    //no bt,to for test
    log_info("not bt!!!!!!");
#endif

#if (TCFG_HID_AUTO_SHUTDOWN_TIME)
    //无操作定时软关机
    g_auto_shutdown_timer = sys_timeout_add(POWER_EVENT_POWER_SOFTOFF, gamebox_power_event_to_user, TCFG_HID_AUTO_SHUTDOWN_TIME * 1000);
#endif
}

static int gamebox_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        if (!it) {
            break;
        }
        switch (it->action) {
        case ACTION_GAMEBOX:
            gamebox_app_start();
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

static int gamebox_bt_hci_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_info("----%s reason %x %x", __FUNCTION__, bt->event, bt->value);

#if TCFG_USER_BLE_ENABLE
    bt_comm_ble_hci_event_handler(bt);
#endif

    return 0;
}

extern void le_hogp_set_PNP_info(const u8 *info);
static const u8 gambox_PnP_ID[] = {0x02, 0x17, 0x27, 0x40, 0x00, 0x23, 0x00};
static int gamebox_bt_connction_status_event_handler(struct bt_event *bt)
{
    log_info("----%s %d", __FUNCTION__, bt->event);

#if TCFG_USER_BLE_ENABLE
    bt_comm_ble_status_event_handler(bt);

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        /*
         * 蓝牙初始化完成
         */
        log_info("BT_STATUS_INIT_OK\n");

        log_info("set gamebox pnp\n");
        le_hogp_set_PNP_info(gambox_PnP_ID);

        ble_module_enable(1);
        ble_hid_timer_handle = sys_s_hi_timer_add((void *)0, gamebox_ble_mouse_timer_handler, 10);
        break;

    default:
        break;
    }

#endif
    return 0;
}

static int gamebox_bt_common_event_handler(struct bt_event *bt)
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
        gamebox_auto_shutdown_disable();
        break;

    default:
        break;

    }
    return 0;
}

static int gamebox_event_handler(struct application *app, struct sys_event *event)
{
#if (TCFG_HID_AUTO_SHUTDOWN_TIME)
    //重置无操作定时计数
    if (event->type != SYS_DEVICE_EVENT || DEVICE_EVENT_FROM_POWER != event->arg) { //过滤电源消息
        sys_timer_modify(g_auto_shutdown_timer, TCFG_HID_AUTO_SHUTDOWN_TIME * 1000);
    }
#endif

    /* log_info("event: %s", event->arg); */
    switch (event->type) {
    case SYS_KEY_EVENT:
        log_info("Sys Key : %s", event->arg);
        /* app_key_event_handler(event); */
        return 0;

    case SYS_BT_EVENT:
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            gamebox_bt_connction_status_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
            gamebox_bt_hci_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_FORM_COMMON) {
            return gamebox_bt_common_event_handler(&event->u.dev);
        }
        return 0;

    case SYS_DEVICE_EVENT:
        if ((u32)event->arg == DEVICE_EVENT_FROM_POWER) {
            return app_power_event_handler(&event->u.dev, gamebox_power_set_soft_reset);
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

//-----------------------
//system check go sleep is ok
static u8 gamebox_idle_query(void)
{
    return !is_gamebox_active;
}

REGISTER_LP_TARGET(app_gamebox_lp_target) = {
    .name = "app_gamebox_deal",
    .is_idle = gamebox_idle_query,
};

static const struct application_operation app_gamebox_ops = {
    .state_machine  = gamebox_state_machine,
    .event_handler 	= gamebox_event_handler,
};

/*
 * 注册AT Module模式
 */
REGISTER_APPLICATION(app_gamebox) = {
    .name 	= "gamebox",
    .action	= ACTION_GAMEBOX,
    .ops 	= &app_gamebox_ops,
    .state  = APP_STA_DESTROY,
};

#endif

