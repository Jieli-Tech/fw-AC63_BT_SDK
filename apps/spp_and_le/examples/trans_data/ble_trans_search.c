/*********************************************************************************************
    *   Filename        : ble_trans_search.c

    *   Description     : 配置搜索指定的services

    *   Author          :JM

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2022-9-17 11:14

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
#include "custom_cfg.h"
#include "btstack/btstack_event.h"
#include "le_client_demo.h"
#include "gatt_common/le_gatt_common.h"

#if CONFIG_APP_SPP_LE && CONFIG_BT_GATT_CLIENT_NUM

#if LE_DEBUG_PRINT_EN
#define log_info(x, ...)  printf("[TRANS-SEARCH]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif


static int trans_client_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param);
static void trans_scan_conn_config_set(struct ctl_pair_info_t *pair_info);

const gatt_client_cfg_t trans_client_init_cfg = {
    .event_packet_handler = trans_client_event_packet_handler,
};
//---------------------------------------------------------------------------
#define UUID_HANDLE_MAX   16
typedef struct {
    u16 uuid16;
    /* u16 uuid128[16]; */
    u16 value_handle;
    u16 properties;
} match_uuid_t;
static match_uuid_t uuid_handle_table[UUID_HANDLE_MAX];
static u16 match_uuid_cnt;

static u16 trans_client_conn_handle;
static u16 trans_client_read_handle;
static u16 trans_client_write_handle;
static u16 trans_client_notify_handle;

#define TRANS_TEST_READ_SEND            1 //测试发数
//---------------------------------------------------------------------------
//指定搜索uuid
//指定搜索uuid
static const target_uuid_t  jl_trans_search_uuid_table[] = {

    {
        .services_uuid16 = 0x1800,         //Generic Access
        .characteristic_uuid16 = 0x2a00,   //Device Name
        .opt_type = ATT_PROPERTY_READ,
        .read_long_enable = 1,
    },

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

#if CONFIG_BT_SM_SUPPORT_ENABLE /*ios need pair*/
    {
        .services_uuid16 = 0x180f,         //Battery
        .characteristic_uuid16 = 0x2a19,   //Battery Level
        .opt_type = ATT_PROPERTY_NOTIFY | ATT_PROPERTY_READ,
    },

    {
        .services_uuid16 = 0x1805,         //Current Time
        .characteristic_uuid16 = 0x2a2b,   //Current Time
        .opt_type = ATT_PROPERTY_NOTIFY | ATT_PROPERTY_READ,
    },

    {
        .services_uuid16 = 0x1805,         //Current Time
        .characteristic_uuid16 = 0x2a0f,   //Local Time Information
        .opt_type = ATT_PROPERTY_READ,
    },

    {
        .services_uuid16 = 0x180a,         //Device Information
        .characteristic_uuid16 = 0x2a29,   //Manufacturer Name String
        .opt_type = ATT_PROPERTY_READ,
    },

    {
        .services_uuid16 = 0x180a,         //Device Information
        .characteristic_uuid16 = 0x2a24,   //Model Number String
        .opt_type = ATT_PROPERTY_READ,
    },
#endif

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

//测试write数据操作
static void trans_client_test_read(void)
{
#if TRANS_TEST_READ_SEND
    int ret = 0;
    u16 tmp_handle = trans_client_conn_handle;

    if (tmp_handle && trans_client_read_handle) {
        ret = ble_comm_att_send_data(tmp_handle, trans_client_read_handle, 0, 0, ATT_OP_READ);
        log_info("test_read:%04x, %04x, %d ", tmp_handle, trans_client_read_handle, ret);
    }

    static int write_count;
    if (tmp_handle && trans_client_write_handle) {
        ret = ble_comm_att_send_data(tmp_handle, trans_client_write_handle, &write_count, 4, ATT_OP_WRITE_WITHOUT_RESPOND);
        log_info("test_write:%04x, %04x, %d ", tmp_handle, trans_client_write_handle, ret);
        if (!ret) {
            write_count++;
        }
    } else {
        write_count = 0;
    }

#endif
}

//配置扫描匹配连接的设备，已经连上后搜索匹配的profile uuid
static const gatt_search_cfg_t trans_client_search_config = {
    .match_devices = NULL,
    .match_devices_count = 0,
    .match_rssi_enable = 0,

    .search_uuid_group = jl_trans_search_uuid_table,
    .search_uuid_count = (sizeof(jl_trans_search_uuid_table) / sizeof(target_uuid_t)),
    .auto_enable_ccc = 1,
};

/*handle 转换 对应的uuid*/
static u16 __handle_to_uuid(u16 value_handle)
{
    u8 i;
    match_uuid_t *t;
    for (i = 0; i < match_uuid_cnt; i++) {
        t = &uuid_handle_table[i];
        if (t->value_handle == value_handle) {
            return t->uuid16;
        }
    }
    return 0xFFFF;
}

/*解析对应handle的数据内容*/
static void __read_info_handle(u16 value_handle, u8 *data, u16 len)
{
    u16 uuid16 = __handle_to_uuid(value_handle);
    switch (uuid16) {
    case 0x2a00:
        log_info("0x2a00: Device Name: %s", data);
        break;

    case 0x2a19:
        log_info("0x2a19: Battery Level: %d", data[0]);
        break;

    case 0x2a2b:
        log_info("0x2a2b: Current Time: %d-%d-%d %d:%d:%d week:%d",
                 (data[1] << 8) | data[0], data[2], data[3],
                 data[4], data[5], data[6], data[7]);
        break;

    case 0x2a0f:
        char utc = data[0];
        log_info("0x2a0f: Local Time Information: UTC:%d", utc / 4);
        break;

    case 0x2a29:
        log_info("0x2a29: Manufacturer Name String: %s", data);
        break;

    case 0x2a24:
        log_info("0x2a24: Model Number String: %s", data);
        break;

    default:
        break;
    }
}

//-------------------------------------------------------------------------------------
//处理gatt 回调的事件，hci & gatt
static int trans_client_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param)
{
    /* log_info("event: %02x,size= %d\n",event,size); */
    switch (event) {
    case GATT_COMM_EVENT_GATT_DATA_REPORT: {
        att_data_report_t *report_data = (void *)packet;
        switch (report_data->packet_type) {
        case GATT_EVENT_NOTIFICATION://notify
            log_info("notify_report:");
            break;

        case GATT_EVENT_INDICATION://indicate
            log_info("indicate_report:");
            break;

        case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT://read
            log_info("read_report:");
            __read_info_handle(report_data->value_handle, report_data->blob, report_data->blob_length);
            break;

        case GATT_EVENT_LONG_CHARACTERISTIC_VALUE_QUERY_RESULT://read long
            log_info("read_long_report:");
            __read_info_handle(report_data->value_handle, report_data->blob, report_data->blob_length);
            break;

        case GATT_EVENT_QUERY_COMPLETE:
            /*ATT读写操作等结果返回,对应spec的att error*/
            log_info("QUERY_COMPLETE:att_error: %02x\n", report_data->blob[0]);
            break;

        default:
            log_info("unknow_report:");
            break;
        }

        log_info("conn_handle=%04x,value_handle= %04x,size=%d\n", report_data->conn_handle, report_data->value_handle, report_data->blob_length);
        put_buf(report_data->blob, report_data->blob_length);
    }
    break;

    case GATT_COMM_EVENT_GATT_SEARCH_MATCH_UUID: {
        opt_handle_t *opt_hdl = packet;
        log_info("match:server_uuid= %04x,charactc_uuid= %04x,value_handle= %04x\n", \
                 opt_hdl->search_uuid->services_uuid16, opt_hdl->search_uuid->characteristic_uuid16, opt_hdl->value_handle);

        if (match_uuid_cnt < UUID_HANDLE_MAX) {
            uuid_handle_table[match_uuid_cnt].uuid16 = opt_hdl->search_uuid->characteristic_uuid16;
            uuid_handle_table[match_uuid_cnt].properties = opt_hdl->search_uuid->opt_type;
            uuid_handle_table[match_uuid_cnt].value_handle = opt_hdl->value_handle;
            match_uuid_cnt++;
        }

#if TRANS_TEST_READ_SEND
        //for test
#if CONFIG_BT_SM_SUPPORT_ENABLE
        if (opt_hdl->search_uuid->characteristic_uuid16 == 0x2a2b) {
#else
        if (opt_hdl->search_uuid->characteristic_uuid16 == 0x2a00) {
#endif
            trans_client_read_handle = opt_hdl->value_handle;
            log_info("get read_handle= %04x\n", trans_client_read_handle);
        }

        if (opt_hdl->search_uuid->characteristic_uuid16 == 0xae01) {
            trans_client_write_handle = opt_hdl->value_handle;
            log_info("get write_handle= %04x\n", trans_client_write_handle);
        }

        if (opt_hdl->search_uuid->characteristic_uuid16 == 0xae02) {
            trans_client_notify_handle = opt_hdl->value_handle;
            log_info("get notify_handle= %04x\n", trans_client_notify_handle);
        }

#endif
    }
    break;

    case GATT_COMM_EVENT_GATT_SEARCH_DESCRIPTOR_RESULT: {
        log_info("GATT_COMM_EVENT_GATT_SEARCH_DESCRIPTOR_RESULT");
        charact_descriptor_t *result_descriptor = ext_param;
    }
    break;

    case GATT_COMM_EVENT_GATT_SEARCH_PROFILE_COMPLETE:
        log_info("GATT_SEARCH_PROFILE_COMPLETE:%04x\n", little_endian_read_16(packet, 0));
        break;

    case GATT_COMM_EVENT_CLIENT_STATE:
        log_info("client_state: handle=%02x,%02x\n", little_endian_read_16(packet, 1), packet[0]);
        break;

    default:
        break;
    }
    return 0;
}

/*启动搜索服务*/
int trans_client_search_remote_profile(u16 conn_handle)
{
#if TRANS_CLIENT_SEARCH_PROFILE_ENABLE
    log_info("%s", __FUNCTION__);
    ble_gatt_just_search_profile_start(conn_handle);
    trans_client_conn_handle = conn_handle;
    match_uuid_cnt = 0;
#endif
    return 0;
}

/*停止搜索服务*/
int trans_client_search_remote_stop(u16 conn_handle)
{
#if TRANS_CLIENT_SEARCH_PROFILE_ENABLE
    log_info("%s", __FUNCTION__);
    if (conn_handle == trans_client_conn_handle) {
        ble_gatt_just_search_profile_stop(conn_handle);
        trans_client_conn_handle = 0;
    }
#endif
    return 0;
}


/*固定默认配置,不做scan操作*/
static const scan_conn_cfg_t trans_client_scan_cfg = {
    .scan_auto_do = 0,
    .creat_auto_do = 0,
};

//trans client 初始化
void trans_client_init(void)
{
    log_info("%s", __FUNCTION__);
    ble_gatt_client_set_scan_config(&trans_client_scan_cfg);//默认要配置,但不开scan

#if TRANS_CLIENT_SEARCH_PROFILE_ENABLE
    ble_gatt_client_set_search_config(&trans_client_search_config);

#if TRANS_TEST_READ_SEND
    sys_timer_add(0, trans_client_test_read, 500);
#endif
#endif
}

//trans client exit
void trans_client_exit(void)
{
#if TRANS_CLIENT_SEARCH_PROFILE_ENABLE
    log_info("%s", __FUNCTION__);
#endif
}

#endif


