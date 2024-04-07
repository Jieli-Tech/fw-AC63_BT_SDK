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
#include "app_power_manage.h"
#include "le_client_demo.h"
#include "usb/device/hid.h"
#include "app_comm_bt.h"
#include "ble_24g_profile.h"
#include "le_gatt_common.h"
#include "le_common.h"

#define LOG_TAG_CONST       CONN_24G
#define LOG_TAG             "[CONN_24G]"  //暂时不修改
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if CONFIG_APP_CONN_24G

#if TCFG_USER_EDR_ENABLE
//只支持BLE
#error " only support ble !!!!!!"
#endif

//是否打开2.4G持续发送数据
#define  CONN_24G_KEEP_SEND_EN      1   //just for 2.4gtest keep data
//选择物理层
//配置主机需要连接的广播类型,绑定

extern u8  client_pair_bond_info[8];        //主机配对信息
extern u8  pair_bond_info[8];               //从机配对信息
static u8  con_handle = 0;                  //连接handle
static u8  is_app_conn_active = 0;          //app软关机标志
static u8  conn_24g_phy_test_timer_id = 0;  //2.4g数据发送定时器id号
static u8  conn_24g_coded_test = CFG_RF_24G_CODE_ID_ADV;

#if 1
//-----------------------
//bt初始化配置
#define PASSKEY_ENABLE             0
//ATT发送的包长,    note: 23 <=need >= MTU
#define ATT_LOCAL_MTU_SIZE             (64)
//ATT缓存的buffer大小,  note: need >= 23,可修改
#define ATT_SEND_CBUF_SIZE             (30)
static const sm_cfg_t sm_init_config = {
    .master_security_auto_req = 1,
    .master_set_wait_security = 1,
    .slave_security_auto_req = 0,
    .slave_set_wait_security = 0,

#if PASSKEY_ENABLE
    .io_capabilities = IO_CAPABILITY_DISPLAY_ONLY,
#else
    .io_capabilities = IO_CAPABILITY_NO_INPUT_NO_OUTPUT,
#endif

    .authentication_req_flags = SM_AUTHREQ_BONDING | SM_AUTHREQ_MITM_PROTECTION,
    .min_key_size = 7,
    .max_key_size = 16,
    .sm_cb_packet_handler = NULL,
};

//gatt 初始化gatt配置
extern const gatt_server_cfg_t conn_24g_server_init_cfg;
extern const gatt_client_cfg_t conn_24g_client_init_cfg;
//gatt 控制块初始化
static gatt_ctrl_t conn_24g_gatt_control_block = {
    //public
    .mtu_size = ATT_LOCAL_MTU_SIZE,
    .cbuffer_size = ATT_SEND_CBUF_SIZE,
    .multi_dev_flag	= 0,

    //config
#if CONFIG_BT_GATT_SERVER_NUM
    .server_config = &conn_24g_server_init_cfg,
#else
    .server_config = NULL,
#endif

#if CONFIG_BT_GATT_CLIENT_NUM
    .client_config = &conn_24g_client_init_cfg,
#else
    .client_config = NULL,
#endif

#if CONFIG_BT_SM_SUPPORT_ENABLE
    .sm_config = &sm_init_config,
#else
    .sm_config = NULL,
#endif
    //cbk,event handle
    .hci_cb_packet_handler = NULL,
};
#endif

//----------------------------------------------------------------------------
//2.4g主机发送数据函数
static void conn_24g_phy_test(void)
{
    static u32 count = 0;
    count++;
    ble_comm_att_send_data(con_handle, ATT_CHARACTERISTIC_ae01_01_VALUE_HANDLE, &count, 16, ATT_OP_WRITE_WITHOUT_RESPOND);
    log_info("con_handle %d send data: %d", con_handle, count);
}

void conn_24g_power_event_to_user(u8 event)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = (void *)DEVICE_EVENT_FROM_POWER;
    e.u.dev.event = event;
    e.u.dev.value = 0;
    sys_event_notify(&e);
}

static void conn_24g_set_soft_poweroff(void)
{
    log_info("set_soft_poweroff\n");
    is_app_conn_active = 1;
    //必须先主动断开蓝牙链路,否则要等链路超时断开

#if TCFG_USER_BLE_ENABLE
    btstack_ble_exit(0);
#endif

    //延时300ms，确保BT退出链路断开
    sys_timeout_add(NULL, power_set_soft_poweroff, WAIT_DISCONN_TIME_MS);
}

extern const int config_btctler_coded_type;
static void conn_24g_app_start()
{
    log_info("=======================================");
    log_info("-------------conn 24g demo-------------");
    log_info("=======================================");

    log_info("app_file: %s", __FILE__);

    clk_set("sys", BT_NORMAL_HZ);

    //配置CODED类型
    if (config_btctler_coded_type == CONN_SET_PHY_OPTIONS_S2) {
        log_info("set coded is s2");
        ll_vendor_set_code_type(CONN_SET_PHY_OPTIONS_S2);
    } else if (config_btctler_coded_type == CONN_SET_PHY_OPTIONS_S8) {
        log_info("set coded is s8");
        ll_vendor_set_code_type(CONN_SET_PHY_OPTIONS_S8);
    }
//有BLE
#if (TCFG_USER_BLE_ENABLE)
    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);
    btstack_ble_start_before_init(NULL, 0);//ble开始之前初始化
    btstack_init();//蓝牙协议栈初始化
#endif

    /* 按键消息使能 */
    sys_key_event_enable();//按键事件使能
}

/*************************************************************************************************/
/*!
 *  \brief      修改2.4G CODED码
 *  \param      [in] coded         设置coded码,输入32bits，0101分布需要相对均匀.
 *  \param      [in] channel       设置广播(GATT_ROLE_SERVER) or 扫描(GATT_ROLE_CLIENT)
 *
 *  \note       在初始化完成后任意非连接时刻修改CODED码
 */
/*************************************************************************************************/
void rf_set_conn_24g_coded(u32 coded, u8 channel)
{
    if (channel == GATT_ROLE_CLIENT) {
        ble_gatt_client_module_enable(0);
        rf_set_scan_24g_hackable_coded(coded);
        ble_gatt_client_module_enable(1);
    } else {
        ble_gatt_server_module_enable(0);
        rf_set_adv_24g_hackable_coded(coded);
        ble_gatt_server_module_enable(1);
    }
}

static int conn_24g_state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        if (!it) {
            break;
        }
        switch (it->action) {
        case ACTION_CONN_24G_MAIN:
            conn_24g_app_start();
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

static int conn_24g_bt_hci_event_handler(struct bt_event *bt)
{
    log_info("----%s reason %x %x", __FUNCTION__, bt->event, bt->value);

#if TCFG_USER_BLE_ENABLE
    bt_comm_ble_hci_event_handler(bt);//hci 事件消息处理
#endif
    return 0;
}

static int conn_24g_bt_connction_status_event_handler(struct bt_event *bt)
{
    log_info("----%s %d", __FUNCTION__, bt->event);

#if TCFG_USER_BLE_ENABLE
    bt_comm_ble_status_event_handler(bt);//公共事件消息处理
#endif
    return 0;
}

//BLE连接状态消息回调
static void conn_24g_ble_status_callback(ble_state_e status, u8 reason)
{
    log_info("----%s reason %x %x", __FUNCTION__, status, reason);
    con_handle = reason;

    switch (status) {
    case BLE_ST_CONNECT:
        break;
    case BLE_ST_DISCONN:
        /*
         * 蓝牙断开连接,主机设置关闭数据发送
         */
#if CONN_24G_KEEP_SEND_EN && CONFIG_BT_GATT_CLIENT_NUM
        log_info("CLOSE CONN_24G_KEEP_SEND_EN\n");
        sys_timeout_del(conn_24g_phy_test_timer_id);
        conn_24g_phy_test_timer_id = 0;
#endif
        break;
    case BLE_ST_NOTIFY_IDICATE:
    case BLE_ST_SEARCH_COMPLETE:
#if CONN_24G_KEEP_SEND_EN && CONFIG_BT_GATT_CLIENT_NUM
        if (conn_24g_phy_test_timer_id == 0) {
            log_info("OPEN CONN_24G_KEEP_SEND_EN\n");
            conn_24g_phy_test_timer_id = sys_timer_add(NULL, conn_24g_phy_test, 200);
        }
#endif
        break;
    case BLE_ST_CREATE_CONN:
        break;

    default:
        break;
    }
}

static void conn_24g_key_event_handler(struct sys_event *event)
{
    u8 event_type = 0;
    u8 key_value = 0;

    if (event->arg == (void *)DEVICE_EVENT_FROM_KEY) {
        event_type = event->u.key.event;
        key_value = event->u.key.value;
        log_info("app_key_evnet: %d,%d\n", event_type, key_value);

        if (event_type == KEY_EVENT_TRIPLE_CLICK
            && (key_value == TCFG_ADKEY_VALUE3 || key_value == TCFG_ADKEY_VALUE0)) {
            conn_24g_power_event_to_user(POWER_EVENT_POWER_SOFTOFF);
            return;
        }
        //按键0长按抬起解绑
        if (event_type == KEY_EVENT_UP && (key_value == TCFG_ADKEY_VALUE0)) {
#if CONFIG_BT_GATT_CLIENT_NUM
            memset(&client_pair_bond_info[0], 0, 8);
            conn_24g_client_pair_vm_do(client_pair_bond_info, sizeof(client_pair_bond_info), 1);
            log_info("clear client pear info!");
            put_buf(&client_pair_bond_info, 8);
            ble_gatt_client_scan_enable(0);
            ble_gatt_client_disconnect_all();
            conn_24g_central_init();
            ble_gatt_client_scan_enable(1);
#elif CONFIG_BT_GATT_SERVER_NUM
            memset(&pair_bond_info[0], 0, 8);
            conn_24g_server_pair_vm_do(pair_bond_info, sizeof(pair_bond_info), 1);
            log_info("clear server pear info!");
            put_buf(&pair_bond_info, 8);
            //关闭模块重新打开
            ble_gatt_server_adv_enable(0);
            ble_gatt_server_disconnect_all();
            /* ble_comm_disconnect(0x50); */
            conn_24g_server_init();
            ble_gatt_server_adv_enable(1);
#endif
            return;
        }

        //按键2按下切换2.4 CODED码,(需要在非连接state)
        if (event_type == KEY_EVENT_CLICK && (key_value == TCFG_ADKEY_VALUE2)) {
            if (ble_comm_dev_is_connected(SUPPORT_MAX_GATT_SERVER) || ble_comm_dev_is_connected(SUPPORT_MAX_GATT_CLIENT)) {
                log_info("Device is connection, no set 2.4coded!!");
            } else {
                if (conn_24g_coded_test) {
                    conn_24g_coded_test = 0x00;
                } else {
                    conn_24g_coded_test = CFG_RF_24G_CODE_ID_ADV;
                }
                /*//for test
                 *           / ADV(CODED:0x00)    ----------BT-----------      <-B-> SCAN(CODED:0x00)
                 *  1:<-A->--|
                 *           \ SCAN(CODED:0x01)   ----------BT-----------      <-C-> ADV(CODED:0x01)
                 *                                          ||
                 *                                          ||
                 *           / ADV(CODED:0x23)    ----------BT-----------      <-B-> SCAN(CODED:0x23)
                 *  2:<-A->--|
                 *           \ SCAN(CODED:0x24)   ----------BT-----------      <-C-> ADV(CODED:0x24)
                 *  已知: A为开主从的设备,B为主机,C为从机; 在1中A与B/C均保持连接
                    验证: 从1到2过程中若任然能回连上(A与B/C均保持连接),可证明ADV和SCAN可以分别切换CODED码并正常
                */
                log_info("rf_set_conn_24g_coded: %d", conn_24g_coded_test);
                rf_set_conn_24g_coded(conn_24g_coded_test, GATT_ROLE_SERVER);
                rf_set_conn_24g_coded(conn_24g_coded_test + 1, GATT_ROLE_CLIENT);
            }
        }
    }
}

static int conn_24g_event_handler(struct application *app, struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        //按键消息
        conn_24g_key_event_handler(event);
        return 0;

    case SYS_BT_EVENT:
#if (TCFG_USER_BLE_ENABLE)
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            //2.4g连接过程消息处理
            conn_24g_bt_connction_status_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
            //2.4g HCI消息处理
            conn_24g_bt_hci_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_BLE_STATUS) {
            //2.4g ble状态变化消息处理
            conn_24g_ble_status_callback(event->u.bt.event, event->u.bt.value);
        }
#endif
        return 0;

    case SYS_DEVICE_EVENT:
        if ((u32)event->arg == DEVICE_EVENT_FROM_POWER) {
            //电源事件处理
            return app_power_event_handler(&event->u.dev, conn_24g_set_soft_poweroff);
        }

#if TCFG_CHARGE_ENABLE
        else if ((u32)event->arg == DEVICE_EVENT_FROM_CHARGE) {
            //电量消息处理
            app_charge_event_handler(&event->u.dev);
        }
#endif
        return 0;

    default:
        return FALSE;
    }
    return FALSE;
}

//注册状态机和事件机
static const struct application_operation app_conn_24g_ops = {
    .state_machine  = conn_24g_state_machine,
    .event_handler 	= conn_24g_event_handler,
};

/*
 * 注册AT Module模式
 */
REGISTER_APPLICATION(app_conn_24g) = {
    .name 	= "conn_24g",
    .action	= ACTION_CONN_24G_MAIN,
    .ops 	= &app_conn_24g_ops,
    .state  = APP_STA_DESTROY,
};

//-----------------------
//system check go sleep is ok
static u8 conn_24g_state_idle_query(void)
{
    return !is_app_conn_active;
}

REGISTER_LP_TARGET(conn_24g_state_lp_target) = {
    .name = "conn_24g_state_deal",
    .is_idle = conn_24g_state_idle_query,
};

//-----------------------
//bt comm:蓝牙公共部分
void bt_ble_before_start_init(void)
{
    log_info("%s", __FUNCTION__);
    ble_comm_init(&conn_24g_gatt_control_block);
}

void bt_ble_init(void)
{
    log_info("%s\n", __FUNCTION__);
    ble_comm_set_config_name(bt_get_local_name(), 1);

#if CONFIG_BT_GATT_SERVER_NUM
    conn_24g_server_init();//24g从机初始化
    rf_set_adv_24g_hackable_coded(CFG_RF_24G_CODE_ID_ADV);//设置为2.4g模式
    ble_gatt_server_module_enable(1);
#endif

#if CONFIG_BT_GATT_CLIENT_NUM
    conn_24g_central_init();//24g主机初始化
    rf_set_scan_24g_hackable_coded(CFG_RF_24G_CODE_ID_SCAN);//设置为2.4g模式
    ble_gatt_client_module_enable(1);
#endif

}

void bt_ble_exit(void)
{
    log_info("%s\n", __FUNCTION__);
    ble_module_enable(0);
    ble_comm_exit();
}

void ble_module_enable(u8 en)
{
    ble_comm_module_enable(en);
}

#endif


