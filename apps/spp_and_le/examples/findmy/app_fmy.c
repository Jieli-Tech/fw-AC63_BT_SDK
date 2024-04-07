/*********************************************************************************************
    *   Filename        : app_fmy.c

    *   Description     :

    *   Author          : JM

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2024-03-05 15:14

    *   Copyright:(c)JIELI  2011-2026  @ , All Rights Reserved.
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
#include "rcsp_bluetooth.h"
#include "rcsp_user_update.h"
#include "app_charge.h"
#include "app_chargestore.h"
#include "app_power_manage.h"
#include "app_comm_bt.h"
#include "ble_fmy_sensor_uart.h"
#include "ble_fmy.h"

#define LOG_TAG_CONST       FINDMY
#define LOG_TAG             "[APP_FMY]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


#if CONFIG_APP_FINDMY

//---------------------------------------------------------------------
void fmy_state_idle_set_active(uint8_t active)
{
    __fydata->is_app_fmy_active = active;
}

void fmy_power_event_to_user(uint8_t event)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = (void *)DEVICE_EVENT_FROM_POWER;
    e.u.dev.event = event;
    e.u.dev.value = 0;
    sys_event_notify(&e);
}

/*************************************************************************************************/
/*!
 *  \brief      进入软关机
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void fmy_set_soft_poweroff(void)
{
    log_info("set_soft_poweroff\n");
    fmy_state_idle_set_active(true);
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

static void fmy_timer_handle_test(void)
{
    log_info("not_bt");
    //	mem_stats();//see memory
}

static const ble_init_cfg_t fmy_data_ble_config = {
    .same_address = 0,
    .appearance = 0,
};

/*************************************************************************************************/
/*!
 *  \brief      app start
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void fmy_app_start()
{
    log_info("=======================================");
    log_info("-----------findmy demo-------------");
    log_info("=======================================");
    log_info("app_file: %s", __FILE__);

    wdt_init(WDT_32S);//need set long time
    clk_set("sys", 96 * 1000000L);//need run fast for AL
    log_info("vm start address: 0x%8x", CONFIG_VM_ADDR);

    if (__fydata->enter_btstack_num == 0) {
        __fydata->enter_btstack_num = 1;

//有蓝牙
#if (TCFG_USER_EDR_ENABLE || TCFG_USER_BLE_ENABLE)
        uint32_t sys_clk =  clk_get("sys");
        bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);

#if TCFG_USER_EDR_ENABLE
        btstack_edr_start_before_init(NULL, 0);
#if DOUBLE_BT_SAME_MAC
        //手机自带搜索界面，默认搜索到EDR
        __change_hci_class_type(BD_CLASS_TRANSFER_HEALTH);//
#endif
#endif

#if TCFG_USER_BLE_ENABLE
        btstack_ble_start_before_init(&fmy_data_ble_config, 0);
#endif

        btstack_init();

#else
//no bt,to for test
        sys_timer_add(NULL, fmy_timer_handle_test, 1000);
#endif
    }
    /* 按键消息使能 */
    sys_key_event_enable();

    /* SENSOR UART使能 */
#if FMY_DEBUG_SENSOR_TO_UART_ENBALE
    sensor_uart_init();
#endif

    fmy_state_idle_set_active(false);
}
/*************************************************************************************************/
/*!
 *  \brief      app 状态机处理
 *
 *  \param      [in] app ,state ,it
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int fmy_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        if (!it) {
            break;
        }
        switch (it->action) {
        case ACTION_FINDMY:
            fmy_app_start();
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

/*************************************************************************************************/
/*!
 *  \brief      hci 事件处理
 *
 *  \param      [in] bt
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int fmy_bt_hci_event_handler(struct bt_event *bt)
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

/*************************************************************************************************/
/*!
 *  \brief      bt 连接状态处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int fmy_bt_connction_status_event_handler(struct bt_event *bt)
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

/*************************************************************************************************/
/*!
 *  \brief      按键事件处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void fmy_key_event_handler(struct sys_event *event)
{
    /* uint16_t cpi = 0; */
    uint8_t event_type = 0;
    uint8_t key_value = 0;

    if (event->arg == (void *)DEVICE_EVENT_FROM_KEY) {
        event_type = event->u.key.event;
        key_value = event->u.key.value;
        log_info("app_key_evnet: %d,%d\n", event_type, key_value);
        /*Change Case To Idle Demo*/
#if CONFIG_APP_SPP_LE_TO_IDLE
        if (event_type == KEY_EVENT_CLICK && key_value == TCFG_ADKEY_VALUE1) {
            log_info(">>>hold key1:chage case to idle\n");
            //change case to idle test
#if TCFG_USER_BLE_ENABLE
            btstack_ble_exit(0);
#endif

#if TCFG_USER_EDR_ENABLE
            btstack_edr_exit(0);
#endif
            app_switch("idle", ACTION_IDLE_MAIN);//切换case
            return;
        }
#endif  /*Btctrler Task Del Enable */

        if (event_type == KEY_EVENT_TRIPLE_CLICK
            && (key_value == TCFG_ADKEY_VALUE3 || key_value == TCFG_ADKEY_VALUE0)) {
            //for test
            fmy_power_event_to_user(POWER_EVENT_POWER_SOFTOFF);
            return;
        }

        fmy_ble_key_event_handler(event_type, key_value);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      app 事件处理
 *
 *  \param      [in] app, event
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int fmy_event_handler(struct application *app, struct sys_event *event)
{
    switch (event->type) {
    case SYS_FMNA_EVENT:
//            log_info("======SYS_FMNA_EVENT: %02x",event->u.fmna.event);
        clr_wdt();
        fmna_state_machine_event_handle(&event->u.fmna);
        clr_wdt();
        break;

    case SYS_KEY_EVENT:
        fmy_key_event_handler(event);
        break;

    case SYS_BT_EVENT:
#if (TCFG_USER_EDR_ENABLE || TCFG_USER_BLE_ENABLE)
        if ((uint32_t)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            fmy_bt_connction_status_event_handler(&event->u.bt);
        } else if ((uint32_t)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
            fmy_bt_hci_event_handler(&event->u.bt);
        }
#endif
        break;

    case SYS_DEVICE_EVENT:
        if ((uint32_t)event->arg == DEVICE_EVENT_FROM_POWER) {
            return app_power_event_handler(&event->u.dev, fmy_set_soft_poweroff);
        }

#if TCFG_CHARGE_ENABLE
        else if ((uint32_t)event->arg == DEVICE_EVENT_FROM_CHARGE) {
            app_charge_event_handler(&event->u.dev);
        }
#endif
        break;

    default:
        break;
    }
    return 0;
}

static const struct application_operation app_fmy_ops = {
    .state_machine  = fmy_state_machine,
    .event_handler 	= fmy_event_handler,
};

/*
 * 注册AT Module模式
 */
REGISTER_APPLICATION(app_fmy_action) = {
    .name 	= "findmy",
    .action	= ACTION_FINDMY,
    .ops 	= &app_fmy_ops,
    .state  = APP_STA_DESTROY,
};

/*************************************************************************************************/
/*!
 *  \brief      注册控制是否允许系统进入sleep状态
 *
 *  \param      [in]
 *
 *  \return     1--可以进入sleep  0--不允许
 *
 *  \note
 */
/*************************************************************************************************/
static uint8_t fmy_state_idle_query(void)
{
    return !__fydata->is_app_fmy_active;
}

REGISTER_LP_TARGET(fmy_state_lp_target) = {
    .name = "fmy_state_deal",
    .is_idle = fmy_state_idle_query,
};

#endif


