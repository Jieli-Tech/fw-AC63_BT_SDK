/*********************************************************************************************
    *   Filename        : .c

    *   Description     :

    *   Author          : JM

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2017-01-17 11:14

    *   Copyright:(c)JIELI  2011-2016  @ , All Rights Reserved.
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

#if CONFIG_APP_DONGLE && TCFG_USER_BLE_ENABLE

#if LE_DEBUG_PRINT_EN
#define log_info(x, ...)  printf("[BLE_DG]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

//ATT发送的包长,    note: 23 <=need >= MTU
#define ATT_LOCAL_MTU_SIZE        (64)
//ATT缓存的buffer大小,  note: need >= 23,可修改
#define ATT_SEND_CBUF_SIZE        (128)

//搜索类型
#define SET_SCAN_TYPE       SCAN_ACTIVE
//搜索 周期大小
#define SET_SCAN_INTERVAL   ADV_SCAN_MS(10) // unit: ms
//搜索 窗口大小
#define SET_SCAN_WINDOW     ADV_SCAN_MS(10) // unit: ms

//连接周期
#define BASE_INTERVAL_MIN         (6)//最小的interval
#define SET_CONN_INTERVAL   (BASE_INTERVAL_MIN*8) //(unit:1.25ms)
//连接latency
#define SET_CONN_LATENCY    0  //(unit:conn_interval)
//连接超时
#define SET_CONN_TIMEOUT    400 //(unit:10ms)

//建立连接超时
#define SET_CREAT_CONN_TIMEOUT    0 //(unit:ms)


#define SEARCH_PROFILE_EN       0 //search profile
//---------------------------------------------------------------------------
static scan_conn_cfg_t dg_central_scan_cfg;
static u16 dg_ble_central_write_handle;
static u16 dg_conn_handle;

//配对信息
#define   PAIR_BOND_TAG       0x53
static u8 pair_bond_info[8]; //tag + addr_type + address
static u8 cur_peer_addr_info[7];//当前连接对方地址信息

static u8  bt_connected = 0;
static u16 dg_timer_id = 0;

//dongle 上电开配对管理,若配对失败,没有配对设备，停止搜索
#define POWER_ON_RECONNECT_START   (1)   // 上电先回连
#define POWER_ON_SWITCH_TIME       (3500)//unit ms,切换搜索回连周期
#define MATCH_DEVICE_RSSI_LEVEL    (-50)  //RSSI 阈值

//------------------------------------------------------
extern const char *bt_get_local_name();
extern void clr_wdt(void);
//------------------------------------------------------
//输入passkey 加密
#define PASSKEY_ENABLE                     0

static const sm_cfg_t dg_sm_init_config = {
    .master_security_auto_req = 1,
    .master_set_wait_security = 0,

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

extern const gatt_client_cfg_t central_client_init_cfg;

static gatt_ctrl_t dg_gatt_control_block = {
    //public
    .mtu_size = ATT_LOCAL_MTU_SIZE,
    .cbuffer_size = ATT_SEND_CBUF_SIZE,
    .multi_dev_flag	= 0,

    //config
    .server_config = NULL,

#if CONFIG_BT_GATT_CLIENT_NUM
    .client_config = &central_client_init_cfg,
#else
    .client_config = NULL,
#endif

#if CONFIG_BT_SM_SUPPORT_ENABLE
    .sm_config = &dg_sm_init_config,
#else
    .sm_config = NULL,
#endif
    //cbk,event handle
    .hci_cb_packet_handler = NULL,
};


static int dg_central_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param);

static const gatt_client_cfg_t central_client_init_cfg = {
    .event_packet_handler = dg_central_event_packet_handler,
};



//---------------------------------------------------------------------------
//指定搜索uuid
static const target_uuid_t  dongle_search_ble_uuid_table[] = {
    /* { */
    /* .services_uuid16 = 0x1800, */
    /* .characteristic_uuid16 = 0x2a00, */
    /* .opt_type = ATT_PROPERTY_READ, */
    /* }, */

    /* { */
    /* .services_uuid16 = 0x1812, */
    /* .characteristic_uuid16 = 0x2a4b, */
    /* .opt_type = ATT_PROPERTY_READ, */
    /* }, */

    {
        .services_uuid16 = 0x1812,
        .characteristic_uuid16 = 0x2a4d,
        .opt_type = ATT_PROPERTY_NOTIFY,
    },

    {
        .services_uuid16 = 0x1812,
        .characteristic_uuid16 = 0x2a33,
        .opt_type = ATT_PROPERTY_NOTIFY,
    },

    {
        .services_uuid16 = 0x1801,
        .characteristic_uuid16 = 0x2a05,
        .opt_type = ATT_PROPERTY_INDICATE,
    },

};


//配置多个扫描匹配设备
static const u8 dg_test_remoter_name1[] = "BlueTooth_Keyboard 5.0";//
static const u8 dg_test_remoter_name2[] = "AC630N_mx(BLE)";//
static const u8 user_config_tag_string[] = "abc123";
static const client_match_cfg_t dg_match_device_table[] = {
    {
        .create_conn_mode = BIT(CLI_CREAT_BY_NAME),
        .compare_data_len = sizeof(dg_test_remoter_name1) - 1, //去结束符
        .compare_data = dg_test_remoter_name1,
    },

    {
        .create_conn_mode = BIT(CLI_CREAT_BY_TAG),
        .compare_data_len = sizeof(user_config_tag_string) - 1, //去结束符
        .compare_data = user_config_tag_string,
    },

    {
        .create_conn_mode = BIT(CLI_CREAT_BY_NAME),
        .compare_data_len = sizeof(dg_test_remoter_name2) - 1, //去结束符
        .compare_data = dg_test_remoter_name2,
    },
};


static const gatt_search_cfg_t dg_central_search_config = {
    .match_devices = &dg_match_device_table,
    .match_devices_count = (sizeof(dg_match_device_table) / sizeof(client_match_cfg_t)),
    .match_rssi_enable = 1,
    .match_rssi_value = MATCH_DEVICE_RSSI_LEVEL,

#if SEARCH_PROFILE_EN
    .search_uuid_count = (sizeof(dongle_search_ble_uuid_table) / sizeof(target_uuid_t)),
    .search_uuid_group = dongle_search_ble_uuid_table,
    .auto_enable_ccc = 1,
#else
    .search_uuid_count = 0,
    .search_uuid_group = NULL,
    .auto_enable_ccc = 0,
#endif

};

//-------------------------------------------------------------------------------------

//input priv
static void dg_timer_handle(u32 priv)
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
        if (0 == ble_gatt_client_create_connection_request(&pair_bond_info[2], pair_bond_info[1], 0)) {
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
                if (ble_gatt_client_create_connection_request(&pair_bond_info[2], pair_bond_info[1], 0)) {
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
            if (0 == ble_gatt_client_create_connection_request(&pair_bond_info[2], pair_bond_info[1], 0)) {
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


static const u16 mouse_ccc_value = 0x0001;
static const u16 mouse_notify_handle[3] = {0x0027, 0x002b, 0x002f};
static void dg_enable_notify_ccc(void)
{
    log_info("%s\n", __FUNCTION__);
    ble_comm_att_send_data(dg_conn_handle, mouse_notify_handle[0] + 1, &mouse_ccc_value, 2, ATT_OP_WRITE);
    ble_comm_att_send_data(dg_conn_handle, mouse_notify_handle[1] + 1, &mouse_ccc_value, 2, ATT_OP_WRITE);
    ble_comm_att_send_data(dg_conn_handle, mouse_notify_handle[2] + 1, &mouse_ccc_value, 2, ATT_OP_WRITE);
}

static int dg_pair_vm_do(u8 *info, u8 info_len, u8 rw_flag)
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


int ble_hid_data_send_ext(u8 report_type, u8 report_id, u8 *data, u16 len)
{
    if (dg_conn_handle && dg_ble_central_write_handle) {
        return ble_comm_att_send_data(dg_conn_handle, dg_ble_central_write_handle, data, len, ATT_OP_WRITE);
    }
    return APP_BLE_OPERATION_ERROR;
}



extern void dongle_ble_hid_input_handler(u8 *packet, u16 size);
static int dg_central_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param)
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
            if (report_data->value_handle == mouse_notify_handle[0]) {
                packet[0] = 1; //report_id
            } else if (report_data->value_handle == mouse_notify_handle[1]) {
                packet[0] = 2;//report_id
            } else {
                packet[0] = 1;//report_id
            }
            memcpy(&packet[1], report_data->blob, report_data->blob_length);
            dongle_ble_hid_input_handler(packet, report_data->blob_length + 1);
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
        dg_conn_handle = little_endian_read_16(packet, 0);
        put_buf(&ext_param[7], 7);
        memcpy(cur_peer_addr_info, &ext_param[7], 7);
        bt_connected = 1;
        break;

    case GATT_COMM_EVENT_DISCONNECT_COMPLETE:
        log_info("disconnect_handle:%04x,reason= %02x\n", little_endian_read_16(packet, 0), packet[2]);
        dg_conn_handle = 0;
        bt_connected = 0;
        break;

    case GATT_COMM_EVENT_ENCRYPTION_CHANGE:
        log_info("ENCRYPTION_CHANGE:handle=%04x,state=%d,process =%d", little_endian_read_16(packet, 0), packet[2], packet[3]);
        log_info("ENCRYPTION_CHANGE:handle=%04x,state=%d,process =%d", little_endian_read_16(packet, 0), packet[2], packet[3]);
        if (packet[3] == LINK_ENCRYPTION_RECONNECT) {
            dg_enable_notify_ccc();
        } else {
            memcpy(&pair_bond_info[1], cur_peer_addr_info, 7);
            pair_bond_info[0] = PAIR_BOND_TAG;
            dg_pair_vm_do(pair_bond_info, sizeof(pair_bond_info), 1);
            log_info("bonding remote");
            put_buf(pair_bond_info, sizeof(pair_bond_info));
        }
        break;

    case GATT_COMM_EVENT_CONNECTION_UPDATE_COMPLETE:
        log_info("conn_param update_complete:%04x\n", little_endian_read_16(packet, 0));
        break;


    case GATT_COMM_EVENT_SCAN_DEV_MATCH: {
        log_info("match_dev:addr_type= %d,rssi= %d\n", packet[0], packet[7]);
        put_buf(&packet[1], 6);
        if (packet[8] == 2) {
            log_info("is TEST_BOX\n");
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
        break;

    case GATT_COMM_EVENT_GATT_SEARCH_PROFILE_COMPLETE:
        bt_connected = 2;
        dg_enable_notify_ccc();
        break;

    default:
        break;
    }
    return 0;
}

//广播参数设置
static void dg_scan_conn_config_set(void)
{
    dg_central_scan_cfg.scan_auto_do = 1;
    dg_central_scan_cfg.creat_auto_do = 1;
    dg_central_scan_cfg.scan_type = SET_SCAN_TYPE;
    dg_central_scan_cfg.scan_filter = 1;
    dg_central_scan_cfg.scan_interval = SET_SCAN_INTERVAL;
    dg_central_scan_cfg.scan_window = SET_SCAN_WINDOW;

    dg_central_scan_cfg.creat_conn_interval = SET_CONN_INTERVAL;
    dg_central_scan_cfg.creat_conn_latency = SET_CONN_LATENCY;
    dg_central_scan_cfg.creat_conn_super_timeout = SET_CONN_TIMEOUT;
    dg_central_scan_cfg.conn_update_accept = 1;
    dg_central_scan_cfg.creat_state_timeout_ms = SET_CREAT_CONN_TIMEOUT;

    ble_gatt_client_set_scan_config(&dg_central_scan_cfg);
}

static void dg_central_init(void)
{
    log_info("%s", __FUNCTION__);
    dg_pair_vm_do(pair_bond_info, sizeof(pair_bond_info), 0);
    ble_gatt_client_set_search_config(&dg_central_search_config);
    dg_scan_conn_config_set();
}

//-------------------------------------------------------------------------------------

void bt_ble_before_start_init(void)
{
    ble_comm_init(&dg_gatt_control_block);
}

void bt_ble_init(void)
{
    log_info("%s\n", __FUNCTION__);
    ble_comm_set_config_name(bt_get_local_name(), 1);
    dg_conn_handle = 0;
    bt_connected = 0;

    dg_central_init();

    ble_module_enable(1);

    if (pair_bond_info[0] == PAIR_BOND_TAG) {
        log_info("connect + scan");
#if	POWER_ON_RECONNECT_START
        dg_timer_handle(0);
#endif
        dg_timer_id = sys_timer_add((void *)1, dg_timer_handle, POWER_ON_SWITCH_TIME);
    } else {
        ble_gatt_client_scan_enable(1);
        log_info("just scan");
    }
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


