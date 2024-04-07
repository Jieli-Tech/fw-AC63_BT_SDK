/*********************************************************************************************
    *   Filename        : ble_24g_client.c

    *   Description     :

    *   Author          : LW

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2021-09-29 11:14

    *   Copyright:(c)JIELI  2021-2026  @ , All Rights Reserved.
*********************************************************************************************/
#include "system/app_core.h"
#include "system/includes.h"

#include "app_config.h"
#include "app_action.h"

#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "user_cfg.h"
#include "vm.h"
#include "btcontroller_modules.h"
#include "bt_common.h"
#include "3th_profile_api.h"
#include "le_common.h"
#include "rcsp_bluetooth.h"
#include "JL_rcsp_api.h"
#include "custom_cfg.h"
#include "btstack/btstack_event.h"
#include "gatt_common/le_gatt_common.h"
#include "ble_24g_profile.h"

#if CONFIG_APP_CONN_24G && CONFIG_BT_GATT_CLIENT_NUM

#if LE_DEBUG_PRINT_EN
#define log_info(x, ...)  printf("[24G_CEN]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

//ATT发送的包长,    note: 23 <=need >= MTU
#define ATT_LOCAL_MTU_SIZE        (64)
//ATT缓存的buffer支持缓存数据包个数
#define ATT_PACKET_NUMS_MAX       (2)

//ATT缓存的buffer大小,  note: need >= 23,可修改
#define ATT_SEND_CBUF_SIZE        (ATT_PACKET_NUMS_MAX * (ATT_PACKET_HEAD_SIZE + ATT_LOCAL_MTU_SIZE))

//搜索类型
#define SET_SCAN_TYPE       SCAN_ACTIVE
//搜索 周期大小
#define SET_SCAN_INTERVAL   ADV_SCAN_MS(24) // unit: 0.625ms
//搜索 窗口大小
#define SET_SCAN_WINDOW     ADV_SCAN_MS(8)  // unit: 0.625ms, <= SET_SCAN_INTERVAL

//连接周期
#define BASE_INTERVAL_MIN   (6)//最小的interval
#define SET_CONN_INTERVAL   (BASE_INTERVAL_MIN*1) //(unit:1.25ms)
//连接latency
#define SET_CONN_LATENCY    0  //(unit:conn_interval)
//连接超时
#define SET_CONN_TIMEOUT    100 //(unit:10ms)

//建立连接超时
#define SET_CREAT_CONN_TIMEOUT    0 //(unit:ms)

#define SEARCH_PROFILE_EN         1 //search profile enable
//---------------------------------------------------------------------------
static scan_conn_cfg_t conn_24g_central_scan_cfg;//扫描连接配置
static u16 conn_24g_handle = 0;//2.4g连接handle

//配对信息
//#define   CLIENT_PAIR_BOND_ENABLE    1 //可增加主机配对使能
#define   PAIR_BOND_TAG       0x53
u8 client_pair_bond_info[8];    //tag + addr_type + address
static u8 cur_peer_addr_info[7];//当前连接对方地址信息

static u8  bt_connected = 0;
static u16 conn_timer_id = 0;

//conn 上电开配对管理,若配对失败,没有配对设备，停止搜索
#define POWER_ON_RECONNECT_START   (1)   //上电回连使能
#define POWER_ON_SWITCH_TIME       (3500)//unit ms,切换搜索回连周期
#define MATCH_DEVICE_RSSI_LEVEL    (-50) //RSSI 阈值

//------------------------------------------------------
extern const char *bt_get_local_name();
void conn_24g_central_init(void);
//------------------------------------------------------
//输入passkey 加密
#define PASSKEY_ENABLE             0

static const sm_cfg_t sm_init_config = {
    .master_security_auto_req = 1,
    .master_set_wait_security = 1,

#if PASSKEY_ENABLE
    .io_capabilities = IO_CAPABILITY_KEYBOARD_ONLY,
#else
    .io_capabilities = IO_CAPABILITY_NO_INPUT_NO_OUTPUT,
#endif

    .authentication_req_flags = SM_AUTHREQ_BONDING | SM_AUTHREQ_MITM_PROTECTION,
    .min_key_size = 7,
    .max_key_size = 16,
    .sm_cb_packet_handler = NULL,
};

//gatt 初始化gatt_client配置
extern const gatt_client_cfg_t conn_24g_client_init_cfg;
//gatt 控制模块初始化
gatt_ctrl_t conn_24g_gatt_client_control_block = {
    //public
    .mtu_size = ATT_LOCAL_MTU_SIZE,
    .cbuffer_size = ATT_SEND_CBUF_SIZE,
    .multi_dev_flag	= 0,

    //config
    .server_config = NULL,

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

static int conn_24g_central_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param);

const gatt_client_cfg_t conn_24g_client_init_cfg = {
    .event_packet_handler = conn_24g_central_event_packet_handler,
};

//---------------------------------------------------------------------------
//推送消息给app
static void __ble_bt_evnet_post(u32 arg_type, u8 priv_event, u8 *args, u32 value)
{
    struct sys_event e;
    e.type = SYS_BT_EVENT;
    e.arg  = (void *)arg_type;
    e.u.bt.event = priv_event;
    if (args) {
        memcpy(e.u.bt.args, args, 7);
    }
    e.u.bt.value = value;
    sys_event_notify(&e);
}
//推送ble状态
static void __ble_state_to_user(u8 state, u8 reason)
{
    static u8 ble_state = 0xff;
    if (state != ble_state) {
        log_info("ble_state:%02x===>%02x\n", ble_state, state);
        ble_state = state;
        __ble_bt_evnet_post(SYS_BT_EVENT_BLE_STATUS, state, NULL, reason);
    }
}

//连接之后,搜索指定uuid操作handle交互
static const target_uuid_t  conn_24g_search_ble_uuid_table[] = {
    {
        .services_uuid16 = 0x1800,
        .characteristic_uuid16 = 0x2a00,
        .opt_type = ATT_PROPERTY_READ,
    },

    {
        .services_uuid16 = 0xae30,
        .characteristic_uuid16 = 0xae02,//charge service
        .opt_type = ATT_PROPERTY_NOTIFY,
    },
    {
        .services_uuid16 = 0xae30,
        .characteristic_uuid16 = 0xae04,//charge service
        .opt_type = ATT_PROPERTY_NOTIFY,
    },

};

//配置多个扫描匹配设备
static const u8 conn_24g_test_remoter_name1[] = "AC696X_1(BLE)";//"CONN_24G(BLE)";//
static const u8 conn_24g_test_remoter_name2[] = "AC630N_mx(BLE)";//
static const u8 user_config_tag_string[] = "abc123";
static const client_match_cfg_t conn_24g_match_device_table[] = {
    {
        .create_conn_mode = BIT(CLI_CREAT_BY_NAME),
        .compare_data_len = sizeof(conn_24g_test_remoter_name1) - 1, //去结束符
        .compare_data = conn_24g_test_remoter_name1,
        .filter_pdu_bitmap = BIT(EVENT_ADV_SCAN_IND) | BIT(EVENT_ADV_DIRECT_IND) | BIT(EVENT_ADV_NONCONN_IND),//过滤掉scan direct noconn广播
    },

    {
        .create_conn_mode = BIT(CLI_CREAT_BY_TAG),
        .compare_data_len = sizeof(user_config_tag_string) - 1, //去结束符
        .compare_data = user_config_tag_string,
        .filter_pdu_bitmap = 0,
    },

    {
        .create_conn_mode = BIT(CLI_CREAT_BY_NAME),
        .compare_data_len = sizeof(conn_24g_test_remoter_name2) - 1, //去结束符
        .compare_data = conn_24g_test_remoter_name2,
        .filter_pdu_bitmap = 0,
    },
};

//绑定后的设备配置地址匹配
static client_match_cfg_t conn_24g_bond_device_table[1] = {
    {
        .create_conn_mode = BIT(CLI_CREAT_BY_ADDRESS),
        .compare_data_len = 6, //
        .compare_data = &client_pair_bond_info[2],
        .filter_pdu_bitmap = BIT(EVENT_ADV_SCAN_IND) | BIT(EVENT_ADV_IND) | BIT(EVENT_ADV_NONCONN_IND) | BIT(EVENT_SCAN_RSP),//过滤掉scan adv noconn scan_rsp广播
    },
};

static const gatt_search_cfg_t conn_24g_central_search_config = {
    .match_devices = &conn_24g_match_device_table,
    .match_devices_count = (sizeof(conn_24g_match_device_table) / sizeof(client_match_cfg_t)),
    .match_rssi_enable = 0, //若需距离较近才能进行搜索连接，关闭此使能
    .match_rssi_value = MATCH_DEVICE_RSSI_LEVEL,

#if SEARCH_PROFILE_EN
    .search_uuid_count = (sizeof(conn_24g_search_ble_uuid_table) / sizeof(target_uuid_t)),
    .search_uuid_group = conn_24g_search_ble_uuid_table,
    .auto_enable_ccc = 1,
#else
    .search_uuid_count = 0,
    .search_uuid_group = NULL,
    .auto_enable_ccc = 0,
#endif

};

static const gatt_search_cfg_t conn_24g_central_bond_config = {
    .match_devices = &conn_24g_bond_device_table,
    .match_devices_count = (sizeof(conn_24g_bond_device_table) / sizeof(client_match_cfg_t)),
    .match_rssi_enable = 0, //若需距离较近才能进行搜索连接，关闭此使能
    .match_rssi_value = MATCH_DEVICE_RSSI_LEVEL,

#if SEARCH_PROFILE_EN
    .search_uuid_count = (sizeof(conn_24g_search_ble_uuid_table) / sizeof(target_uuid_t)),
    .search_uuid_group = conn_24g_search_ble_uuid_table,
    .auto_enable_ccc = 1,
#else
    .search_uuid_count = 0,
    .search_uuid_group = NULL,
    .auto_enable_ccc = 0,
#endif

};
//-------------------------------------------------------------------------------------

//input priv
static void conn_24g_timer_handle(u32 priv)
{
    putchar('%');
    static u8 connected_tag = 0;//上电连接配对过标识

    if (bt_connected) {
        if (bt_connected == 2) {
            connected_tag = 1;
        }
        return;
    }

    if (priv == 0) {
        ///init
        if (0 == ble_gatt_client_create_connection_request(&client_pair_bond_info[2], client_pair_bond_info[1], 0)) {
            log_info("pair is exist");
            log_info("reconnect start0");
        } else {
            log_info("pair new start0");
            ble_gatt_client_scan_enable(1);
        }
    } else {

        if (connected_tag) {
            //上电连接配对过，就不执行搜索配对;默认创建连接
            putchar('^');
            if (bt_connected == 0 && ble_gatt_client_get_work_state() != BLE_ST_CREATE_CONN) {
                if (ble_gatt_client_create_connection_request(&client_pair_bond_info[2], client_pair_bond_info[1], 0)) {
                    log_info("recreate_conn fail!!!");
                }
            }
            return;
        }

        switch (ble_gatt_client_get_work_state()) {
        case BLE_ST_CREATE_CONN:
            ble_gatt_client_create_connection_cannel();
            ble_gatt_client_scan_enable(1);
            log_info("pair new start1");
            break;

        case BLE_ST_SCAN:
            ble_gatt_client_scan_enable(0);
            if (0 == ble_gatt_client_create_connection_request(&client_pair_bond_info[2], client_pair_bond_info[1], 0)) {
                log_info("reconnect start1");
            } else {
                log_info("keep pair new start1");
                ble_gatt_client_scan_enable(1);
            }
            break;

        case BLE_ST_INIT_OK:
        case BLE_ST_IDLE:
        case BLE_ST_DISCONN:
            ble_gatt_client_scan_enable(1);
            log_info("pair new start000");
            break;

        default:
            break;
        }
    }
}


static const u16 conn_ccc_value = 0x0001;
static const u16 conn_24g_notify_handle[2] = {0x0008, 0x000d};
static void conn_24g_enable_notify_ccc(void)
{
    log_info("%s:con_hanle:0x%x\n", __FUNCTION__, conn_24g_handle);
    ble_comm_att_send_data(conn_24g_handle, conn_24g_notify_handle[0] + 1, &conn_ccc_value, 2, ATT_OP_WRITE);
    ble_comm_att_send_data(conn_24g_handle, conn_24g_notify_handle[1] + 1, &conn_ccc_value, 2, ATT_OP_WRITE);
    /* ble_comm_att_send_data(conn_24g_handle, conn_24g_notify_handle[2] + 1, &conn_ccc_value, 2, ATT_OP_WRITE); */
}

//-------------------------------------------------------------------------------------
//vm 绑定对方信息读写
int conn_24g_client_pair_vm_do(u8 *info, u8 info_len, u8 rw_flag)
{
    int ret;
    int vm_len = info_len;

    log_info("-conn_pair_info vm_do:%d\n", rw_flag);
    if (rw_flag == 0) {
        ret = syscfg_read(CFG_BLE_BONDING_REMOTE_INFO, (u8 *)info, vm_len);
        if (!ret) {
            log_info("-null--\n");
            memset(info, 0xff, info_len);
        }
        if (info[0] != PAIR_BOND_TAG) {
            return -1;
        }
    } else {
        syscfg_write(CFG_BLE_BONDING_REMOTE_INFO, (u8 *)info, vm_len);
    }
    return 0;
}

static int conn_24g_central_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param)
{
    /* log_info("event: %02x,size= %d\n",event,size); */
    switch (event) {
    case GATT_COMM_EVENT_GATT_DATA_REPORT: {
        att_data_report_t *report_data = (void *)packet;
        //log_info("data_report:hdl=%04x,pk_type=%02x,size=%d\n", report_data->conn_handle, report_data->packet_type, report_data->blob_length);
        //log_info("value_handle=%04x\n", report_data->value_handle);
        //put_buf(report_data->blob, report_data->blob_length);

        switch (report_data->packet_type) {
        case GATT_EVENT_NOTIFICATION: { //notify
            u8 packet[16];
            if (report_data->value_handle == conn_24g_notify_handle[0]) {
                packet[0] = 1; //report_id
            } else if (report_data->value_handle == conn_24g_notify_handle[1]) {
                packet[0] = 2;//report_id
            } else {
                packet[0] = 1;//report_id
            }
            memcpy(&packet[1], report_data->blob, report_data->blob_length);
            putchar('&');
        }
        break;

        case GATT_EVENT_INDICATION://indicate
            break;
        case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT://read
            break;
        case GATT_EVENT_LONG_CHARACTERISTIC_VALUE_QUERY_RESULT://read long
            break;
        default:
            break;
        }
    }
    break;

    case GATT_COMM_EVENT_CAN_SEND_NOW:
        break;

    case GATT_COMM_EVENT_CONNECTION_COMPLETE:
        log_info("connection_handle:%04x\n", little_endian_read_16(packet, 0));
        log_info("peer_address_info:");
        conn_24g_handle = little_endian_read_16(packet, 0);
        put_buf(&ext_param[7], 7);
        //保存配对信息
        memcpy(cur_peer_addr_info, &ext_param[7], 7);
        memcpy(&client_pair_bond_info[1], cur_peer_addr_info, 7);
        client_pair_bond_info[0] = PAIR_BOND_TAG;
        put_buf(&ext_param[7], 7);
        conn_24g_client_pair_vm_do(client_pair_bond_info, sizeof(client_pair_bond_info), 1);
        bt_connected = 1;
        //连接后识别定向广播,不然会导致从机一断开连接,主机马上又连接
        ble_gatt_client_set_search_config(&conn_24g_central_bond_config);
        __ble_bt_evnet_post(SYS_BT_EVENT_BLE_STATUS, BLE_ST_CONNECT, NULL, conn_24g_handle);
        break;

    case GATT_COMM_EVENT_DISCONNECT_COMPLETE:
        log_info("disconnect_handle:%04x,reason= %02x\n", little_endian_read_16(packet, 0), packet[2]);
        conn_24g_handle = 0;
        bt_connected = 0;
        /* __ble_state_to_user(BLE_ST_DISCONN, conn_24g_handle); */
        break;

    case GATT_COMM_EVENT_ENCRYPTION_CHANGE:
        log_info("ENCRYPTION_CHANGE:handle=%04x,state=%d,process =%d", little_endian_read_16(packet, 0), packet[2], packet[3]);
        log_info("ENCRYPTION_CHANGE:handle=%04x,state=%d,process =%d", little_endian_read_16(packet, 0), packet[2], packet[3]);
        if (packet[3] == LINK_ENCRYPTION_RECONNECT) {
            conn_24g_enable_notify_ccc();
        } else {
            memcpy(&client_pair_bond_info[1], cur_peer_addr_info, 7);
            client_pair_bond_info[0] = PAIR_BOND_TAG;
            conn_24g_client_pair_vm_do(client_pair_bond_info, sizeof(client_pair_bond_info), 1);
            log_info("bonding remoter");
            put_buf(client_pair_bond_info, sizeof(client_pair_bond_info));
        }
        /* __ble_state_to_user(BLE_ST_NOTIFY_IDICATE, conn_24g_handle); */
        break;

    case GATT_COMM_EVENT_CONNECTION_UPDATE_COMPLETE:
        log_info("conn_param update_complete:%04x\n", little_endian_read_16(packet, 0));
        break;

    case GATT_COMM_EVENT_SCAN_DEV_MATCH: {
        log_info("match_dev:addr_type= %d,rssi= %d\n", packet[0], packet[7]);
        put_buf(&packet[1], 6);//打印的是指针不是地址
        if (packet[8] == 2) {
            log_info("is TEST_BOX\n");
            break;
        }

        client_match_cfg_t *match_cfg = ext_param;
        if (match_cfg) {
            log_info("match_mode: %d\n", match_cfg->create_conn_mode);
            if (match_cfg->compare_data_len) {
                put_buf(match_cfg->compare_data, match_cfg->compare_data_len);
            }
        }
    }
    break;

    case GATT_COMM_EVENT_CREAT_CONN_TIMEOUT:
        break;

    case GATT_COMM_EVENT_GATT_SEARCH_MATCH_UUID: {
        opt_handle_t *opt_hdl = packet;
        log_info("match:server_uuid= %04x,charactc_uuid= %04x,value_handle= %04x\n", \
                 opt_hdl->search_uuid->services_uuid16, opt_hdl->search_uuid->characteristic_uuid16, opt_hdl->value_handle);
    }
    break;


    case GATT_COMM_EVENT_MTU_EXCHANGE_COMPLETE:
        log_info("con_handle= %02x, ATT MTU = %u\n", little_endian_read_16(packet, 0), little_endian_read_16(packet, 2));
        break;

    case GATT_COMM_EVENT_GATT_SEARCH_PROFILE_COMPLETE:
        bt_connected = 2;
#if !SEARCH_PROFILE_EN
        conn_24g_enable_notify_ccc();
#endif
        break;

    case GATT_COMM_EVENT_CLIENT_STATE:
        log_info("client_state: handle=%02x,%02x\n", little_endian_read_16(packet, 1), packet[0]);
        __ble_state_to_user(packet[0], conn_24g_handle);
        break;


    default:
        break;
    }
    return 0;
}

//scan参数设置
static void conn_24g_scan_conn_config_set(void)
{
    conn_24g_central_scan_cfg.scan_auto_do = 1;
    conn_24g_central_scan_cfg.creat_auto_do = 1;
    conn_24g_central_scan_cfg.scan_type = SET_SCAN_TYPE;
    conn_24g_central_scan_cfg.scan_filter = 1;
    conn_24g_central_scan_cfg.scan_interval = SET_SCAN_INTERVAL;
    conn_24g_central_scan_cfg.scan_window = SET_SCAN_WINDOW;

    conn_24g_central_scan_cfg.creat_conn_interval = SET_CONN_INTERVAL;
    conn_24g_central_scan_cfg.creat_conn_latency = SET_CONN_LATENCY;
    conn_24g_central_scan_cfg.creat_conn_super_timeout = SET_CONN_TIMEOUT;
    conn_24g_central_scan_cfg.conn_update_accept = 1;
    conn_24g_central_scan_cfg.creat_state_timeout_ms = SET_CREAT_CONN_TIMEOUT;

    ble_gatt_client_set_scan_config(&conn_24g_central_scan_cfg);
}

void conn_24g_central_init(void)
{
    log_info("%s", __FUNCTION__);

    if (0 == conn_24g_client_pair_vm_do(client_pair_bond_info, sizeof(client_pair_bond_info), 0)) {
        log_info("set connect set is bond!!");
        ble_gatt_client_set_search_config(&conn_24g_central_bond_config);
    } else {
        log_info("set connect set is search!!");
        ble_gatt_client_set_search_config(&conn_24g_central_search_config);
    }

    conn_24g_scan_conn_config_set();
}

#endif



