/*********************************************************************************************
    *   Filename        : .c

    *   Description     :

    *   Author          :JM

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
#include "ble_multi.h"
#include "le_client_demo.h"
#include "gatt_common/le_gatt_common.h"

#if CONFIG_APP_MULTI && CONFIG_BT_GATT_CLIENT_NUM

#if LE_DEBUG_PRINT_EN
#define log_info(x, ...)  printf("[MUL-CEN]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

//搜索类型
#define SET_SCAN_TYPE       SCAN_ACTIVE
//搜索 周期大小
#define SET_SCAN_INTERVAL   ADV_SCAN_MS(10) // unit: ms
//搜索 窗口大小
#define SET_SCAN_WINDOW     ADV_SCAN_MS(10) // unit: ms

//连接周期
#define BASE_INTERVAL_MIN   (6)//最小的interval
#define SET_CONN_INTERVAL   (BASE_INTERVAL_MIN*8) //(unit:1.25ms)
//连接latency
#define SET_CONN_LATENCY    0  //(unit:conn_interval)
//连接超时
#define SET_CONN_TIMEOUT    400 //(unit:10ms)

//建立连接超时
#define SET_CREAT_CONN_TIMEOUT    8000 //(unit:ms)

//配对信息表
#define   CLIENT_PAIR_BOND_ENABLE    1
#define   CLIENT_PAIR_BOND_TAG       0x56
static u8 client_pair_bond_info[8]; //tag + addr_type + address
static u8 cur_peer_addr_info[7];//当前连接对方地址信息

static int multi_client_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param);

const gatt_client_cfg_t mul_client_init_cfg = {
    .event_packet_handler = multi_client_event_packet_handler,
};
//---------------------------------------------------------------------------

static scan_conn_cfg_t multi_client_scan_cfg;
static u16 multi_ble_client_write_handle;

#define MULTI_TEST_WRITE_SEND_DATA            1 //测试发数
//---------------------------------------------------------------------------
//指定搜索uuid
//指定搜索uuid
static const target_uuid_t  jl_multi_search_uuid_table[] = {

    // for uuid16
    // PRIMARY_SERVICE, ae30
    // CHARACTERISTIC,  ae01, WRITE_WITHOUT_RESPONSE | DYNAMIC,
    // CHARACTERISTIC,  ae02, NOTIFY,

    {
        .services_uuid16 = 0xae30,
        .characteristic_uuid16 = 0xae01,
        .opt_type = ATT_PROPERTY_WRITE_WITHOUT_RESPONSE,
    },

    {
        .services_uuid16 = 0xae30,
        .characteristic_uuid16 = 0xae02,
        .opt_type = ATT_PROPERTY_NOTIFY,
    },

    //for uuid128,sample
    //	PRIMARY_SERVICE, 0000F530-1212-EFDE-1523-785FEABCD123
    //	CHARACTERISTIC,  0000F531-1212-EFDE-1523-785FEABCD123, NOTIFY,
    //	CHARACTERISTIC,  0000F532-1212-EFDE-1523-785FEABCD123, WRITE_WITHOUT_RESPONSE | DYNAMIC,
    /*
    	{
    		.services_uuid16 = 0,
    		.services_uuid128 =       {0x00,0x00,0xF5,0x30 ,0x12,0x12 ,0xEF, 0xDE ,0x15,0x23 ,0x78,0x5F,0xEA ,0xBC,0xD1,0x23} ,
    		.characteristic_uuid16 = 0,
    		.characteristic_uuid128 = {0x00,0x00,0xF5,0x31 ,0x12,0x12 ,0xEF, 0xDE ,0x15,0x23 ,0x78,0x5F,0xEA ,0xBC,0xD1,0x23},
    		.opt_type = ATT_PROPERTY_NOTIFY,
    	},

    	{
    		.services_uuid16 = 0,
    		.services_uuid128 =       {0x00,0x00,0xF5,0x30 ,0x12,0x12 ,0xEF, 0xDE ,0x15,0x23 ,0x78,0x5F,0xEA ,0xBC,0xD1,0x23} ,
    		.characteristic_uuid16 = 0,
    		.characteristic_uuid128 = {0x00,0x00,0xF5,0x32 ,0x12,0x12 ,0xEF, 0xDE ,0x15,0x23 ,0x78,0x5F,0xEA ,0xBC,0xD1,0x23},
    		.opt_type = ATT_PROPERTY_WRITE_WITHOUT_RESPONSE,
    	},
    */

};

//配置多个扫描匹配设备
static const u8 cetl_test_remoter_name1[] = "AC897N_MX(BLE)";//
static const client_match_cfg_t multi_match_device_table[] = {
    {
        .create_conn_mode = BIT(CLI_CREAT_BY_NAME),
        .compare_data_len = sizeof(cetl_test_remoter_name1) - 1, //去结束符
        .compare_data = cetl_test_remoter_name1,
    },
};


//绑定后的设备配置地址匹配
static client_match_cfg_t multi_bond_device_table[1] = {
    {
        .create_conn_mode = BIT(CLI_CREAT_BY_ADDRESS),
        .compare_data_len = 6, //
        .compare_data = &client_pair_bond_info[2],
    },
};


//测试write数据操作
static void multi_client_test_write(void)
{
#if MULTI_TEST_WRITE_SEND_DATA
    static u32 count = 0;
    int i, ret = 0;
    u16 tmp_handle;

    count++;
    for (i = 0; i < SUPPORT_MAX_GATT_CLIENT; i++) {
        tmp_handle = ble_comm_dev_get_handle(i, GATT_ROLE_CLIENT);
        if (tmp_handle && multi_ble_client_write_handle) {
            ret = ble_comm_att_send_data(tmp_handle, multi_ble_client_write_handle, &count, 16, ATT_OP_WRITE_WITHOUT_RESPOND);
            log_info("test_write:%04x,%d", tmp_handle, ret);
        }
    }
#endif
}

//配置扫描匹配连接的设备，已经连上后搜索匹配的profile uuid
static const gatt_search_cfg_t multil_client_search_config = {
    .match_devices = multi_match_device_table,
    .match_devices_count = (sizeof(multi_match_device_table) / sizeof(client_match_cfg_t)),
    .match_rssi_enable = 0,

    .search_uuid_group = jl_multi_search_uuid_table,
    .search_uuid_count = (sizeof(jl_multi_search_uuid_table) / sizeof(target_uuid_t)),
    .auto_enable_ccc = 1,
};

//配置扫描匹配连接绑定后的设备
static const gatt_search_cfg_t multil_client_bond_config = {
    .match_devices = multi_bond_device_table,
    .match_devices_count = (sizeof(multi_bond_device_table) / sizeof(client_match_cfg_t)),
    .match_rssi_enable = 0,

    .search_uuid_group = jl_multi_search_uuid_table,
    .search_uuid_count = (sizeof(jl_multi_search_uuid_table) / sizeof(target_uuid_t)),
    .auto_enable_ccc = 1,
};

//-------------------------------------------------------------------------------------
//vm 绑定对方信息读写
static int multi_client_pair_vm_do(u8 *info, u8 info_len, u8 rw_flag)
{
    int ret;
    int vm_len = info_len;

    log_info("-conn_pair_info vm_do:%d\n", rw_flag);
    if (rw_flag == 0) {
        ret = syscfg_read(CFG_BLE_BONDING_REMOTE_INFO2, (u8 *)info, vm_len);
        if (!ret) {
            log_info("-null--\n");
            memset(info, 0xff, info_len);
        }
        if (info[0] != CLIENT_PAIR_BOND_TAG) {
            return -1;
        }
    } else {
        syscfg_write(CFG_BLE_BONDING_REMOTE_INFO2, (u8 *)info, vm_len);
    }
    return 0;
}

//清配对信息
static int multi_client_clear_pair(void)
{
    memset(client_pair_bond_info, 0, sizeof(client_pair_bond_info));
    multi_client_pair_vm_do(client_pair_bond_info, sizeof(client_pair_bond_info), 1);
    if (BLE_ST_SCAN == ble_gatt_client_get_work_state()) {
        ble_gatt_client_scan_enable(0);
        ble_gatt_client_set_search_config(&multil_client_search_config);
        ble_gatt_client_scan_enable(1);
    }
}

//处理gatt 回调的事件，hci & gatt
static int multi_client_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param)
{
    /* log_info("event: %02x,size= %d\n",event,size); */
    switch (event) {
    case GATT_COMM_EVENT_GATT_DATA_REPORT: {
        att_data_report_t *report_data = (void *)packet;
        log_info("data_report:hdl=%04x,pk_type=%02x,size=%d\n", report_data->conn_handle, report_data->packet_type, report_data->blob_length);
        put_buf(report_data->blob, report_data->blob_length);

        switch (report_data->packet_type) {
        case GATT_EVENT_NOTIFICATION://notify
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
        put_buf(&ext_param[7], 7);
        memcpy(cur_peer_addr_info, &ext_param[7], 7);
        break;

    case GATT_COMM_EVENT_DISCONNECT_COMPLETE:
        log_info("disconnect_handle:%04x,reason= %02x\n", little_endian_read_16(packet, 0), packet[2]);
        break;

    case GATT_COMM_EVENT_ENCRYPTION_CHANGE:
        log_info("ENCRYPTION_CHANGE:handle=%04x,state=%d,process =%d", little_endian_read_16(packet, 0), packet[2], packet[3]);

        if (packet[3] == LINK_ENCRYPTION_RECONNECT) {
            log_info("reconnect...\n");
            if (memcmp(&client_pair_bond_info[1], cur_peer_addr_info, 7)) {
                log_info("update peer\n");
                memcpy(&client_pair_bond_info[1], cur_peer_addr_info, 7);
                client_pair_bond_info[0] = CLIENT_PAIR_BOND_TAG;
                multi_client_pair_vm_do(client_pair_bond_info, sizeof(client_pair_bond_info), 1);
            }
        } else {
            log_info("first pair...\n");
#if CLIENT_PAIR_BOND_ENABLE
            memcpy(&client_pair_bond_info[1], cur_peer_addr_info, 7);
            client_pair_bond_info[0] = CLIENT_PAIR_BOND_TAG;
            multi_client_pair_vm_do(client_pair_bond_info, sizeof(client_pair_bond_info), 1);
#endif
        }
        break;

    case GATT_COMM_EVENT_CONNECTION_UPDATE_COMPLETE:
        log_info("conn_param update_complete:%04x\n", little_endian_read_16(packet, 0));
        break;


    case GATT_COMM_EVENT_SCAN_DEV_MATCH: {
        log_info("match_dev:addr_type= %d\n", packet[0]);
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

    case GATT_COMM_EVENT_GATT_SEARCH_MATCH_UUID: {
        opt_handle_t *opt_hdl = packet;
        log_info("match:server_uuid= %04x,charactc_uuid= %04x,value_handle= %04x\n", \
                 opt_hdl->search_uuid->services_uuid16, opt_hdl->search_uuid->characteristic_uuid16, opt_hdl->value_handle);
#if MULTI_TEST_WRITE_SEND_DATA
        //for test
        if (opt_hdl->search_uuid->characteristic_uuid16 == 0xae01) {
            multi_ble_client_write_handle = opt_hdl->value_handle;
        }
#endif
    }
    break;


    case GATT_COMM_EVENT_MTU_EXCHANGE_COMPLETE:
        break;

    case GATT_COMM_EVENT_GATT_SEARCH_PROFILE_COMPLETE:
        break;

    case GATT_COMM_EVENT_CLIENT_STATE:
        log_info("client_state: handle=%02x,%02x\n", little_endian_read_16(packet, 1), packet[0]);
        break;

    default:
        break;
    }
    return 0;
}

//广播参数设置
static void multi_scan_conn_config_set(void)
{
    multi_client_scan_cfg.scan_auto_do = 1;
    multi_client_scan_cfg.creat_auto_do = 1;
    multi_client_scan_cfg.scan_type = SET_SCAN_TYPE;
    multi_client_scan_cfg.scan_filter = 1;
    multi_client_scan_cfg.scan_interval = SET_SCAN_INTERVAL;
    multi_client_scan_cfg.scan_window = SET_SCAN_WINDOW;

    multi_client_scan_cfg.creat_conn_interval = SET_CONN_INTERVAL;
    multi_client_scan_cfg.creat_conn_latency = SET_CONN_LATENCY;
    multi_client_scan_cfg.creat_conn_super_timeout = SET_CONN_TIMEOUT;
    multi_client_scan_cfg.conn_update_accept = 1;
    multi_client_scan_cfg.creat_state_timeout_ms = SET_CREAT_CONN_TIMEOUT;

    ble_gatt_client_set_scan_config(&multi_client_scan_cfg);
}

//multi client 初始化
void multi_client_init(void)
{
    log_info("%s", __FUNCTION__);

#if CONFIG_BT_SM_SUPPORT_ENABLE && CLIENT_PAIR_BOND_ENABLE
    if (0 == multi_client_pair_vm_do(client_pair_bond_info, sizeof(client_pair_bond_info), 0)) {
        log_info("client already bond dev");
        put_buf(client_pair_bond_info, sizeof(client_pair_bond_info));
        ble_gatt_client_set_search_config(&multil_client_bond_config);
    } else {
        ble_gatt_client_set_search_config(&multil_client_search_config);
    }
#else
    ble_gatt_client_set_search_config(&multil_client_search_config);
#endif

    multi_scan_conn_config_set();

#if MULTI_TEST_WRITE_SEND_DATA
    sys_timer_add(0, multi_client_test_write, 500);
#endif
}


#endif


