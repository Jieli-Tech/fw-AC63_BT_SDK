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
#include "ble_central.h"

#if CONFIG_APP_CENTRAL

#if LE_DEBUG_PRINT_EN
#define log_info(x, ...)  printf("[BLE_CENTRAL]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

//ATT发送的包长,    note: 23 <=need >= MTU
#define ATT_LOCAL_MTU_SIZE        (517)
//ATT缓存的buffer大小,  note: need >= 23,可修改
#define ATT_SEND_CBUF_SIZE        (512 *2)

//搜索类型
#define SET_SCAN_TYPE       SCAN_ACTIVE
//搜索 周期大小
#define SET_SCAN_INTERVAL   ADV_SCAN_MS(24) // unit: 0.625ms
//搜索 窗口大小
#define SET_SCAN_WINDOW     ADV_SCAN_MS(8)  // unit: 0.625ms, <= SET_SCAN_INTERVAL

//连接周期
#define BASE_INTERVAL_MIN   (6)//最小的interval
#define SET_CONN_INTERVAL   (BASE_INTERVAL_MIN*4) //(unit:1.25ms)
//连接latency
#define SET_CONN_LATENCY    0  //(unit:conn_interval)
//连接超时
#define SET_CONN_TIMEOUT    400 //(unit:10ms)

//建立连接超时
#define SET_CREAT_CONN_TIMEOUT    8000 //(unit:ms)

//------------------------------------------------------
static  u16 cetl_con_handle;
static  u8 search_profile_complete_flag[SUPPORT_MAX_GATT_CLIENT]; /*搜索完服务*/

#define CETL_TEST_WRITE_SEND_DATA             0//连上测试自动发送数据
#define CETL_TEST_SEND_EN                     1//

#if CONFIG_BLE_HIGH_SPEED
#define TEST_TIMER_MS                         5/*定时发包的时间*/
#else
#define TEST_TIMER_MS                         500/*定时发包的时间*/
#endif

#define CETL_TEST_DISPLAY_DATA                1//显示数据打印
#define CETL_TEST_PAYLOAD_LEN                 (244)//发送配PDU长度是251的包

static  u32 send_test_count[SUPPORT_MAX_GATT_CLIENT];
static  u32 recieve_test_count[SUPPORT_MAX_GATT_CLIENT];
//------------------------------------------------------
extern const char *bt_get_local_name();
extern void clr_wdt(void);
//------------------------------------------------------
//输入passkey 加密
#define PASSKEY_ENABLE                     0

static const sm_cfg_t cetl_sm_init_config = {
    .master_security_auto_req = 1,
    .master_set_wait_security = 1,

#if PASSKEY_ENABLE
    .io_capabilities = IO_CAPABILITY_KEYBOARD_ONLY,
#else
    .io_capabilities = IO_CAPABILITY_NO_INPUT_NO_OUTPUT,
#endif

    .authentication_req_flags = SM_AUTHREQ_MITM_PROTECTION,
    .min_key_size = 7,
    .max_key_size = 16,
    .sm_cb_packet_handler = NULL,
};

extern const gatt_client_cfg_t central_client_init_cfg;


static gatt_ctrl_t cetl_gatt_control_block = {
    //public
    .mtu_size = ATT_LOCAL_MTU_SIZE,
    .cbuffer_size = ATT_SEND_CBUF_SIZE,
    .multi_dev_flag	= 0,

#if CONFIG_BT_GATT_SERVER_NUM
    //config
    .server_config = &central_server_init_cfg,
#else
    .server_config = NULL,
#endif

#if CONFIG_BT_GATT_CLIENT_NUM
    .client_config = &central_client_init_cfg,
#else
    .client_config = NULL,
#endif

#if CONFIG_BT_SM_SUPPORT_ENABLE
    .sm_config = &cetl_sm_init_config,
#else
    .sm_config = NULL,
#endif
    //cbk,event handle
    .hci_cb_packet_handler = NULL,
};


static int cetl_client_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param);

static const gatt_client_cfg_t central_client_init_cfg = {
    .event_packet_handler = cetl_client_event_packet_handler,
};


//---------------------------------------------------------------------------

static scan_conn_cfg_t cetl_client_scan_cfg;
static u16 cetl_ble_client_write_handle;

//---------------------------------------------------------------------------
//指定搜索uuid
//指定搜索uuid
static const target_uuid_t  jl_cetl_search_uuid_table[] = {

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

#define   MATCH_CONFIG_NAME          0 //配置连接config的蓝牙名

//配置多个扫描匹配设备
static const u8 cetl_test_remoter_name1[] = "AC897N_MX(BLE)";//
static const u8 cetl_test_remoter_name2[] = "AC632N_MX(BLE)";//

static client_match_cfg_t cetl_match_device_table[] = {
#if MATCH_CONFIG_NAME
    {
        .create_conn_mode = BIT(CLI_CREAT_BY_NAME),
        .compare_data_len = 0, //去结束符
        .compare_data = 0,
        .filter_pdu_bitmap = 0,
    },
#endif

    {
        .create_conn_mode = BIT(CLI_CREAT_BY_NAME),
        .compare_data_len = sizeof(cetl_test_remoter_name1) - 1, //去结束符
        .compare_data = cetl_test_remoter_name1,
        .filter_pdu_bitmap = 0,
    },

    {
        .create_conn_mode = BIT(CLI_CREAT_BY_NAME),
        .compare_data_len = sizeof(cetl_test_remoter_name2) - 1, //去结束符
        .compare_data = cetl_test_remoter_name2,
        .filter_pdu_bitmap = 0,
    },

};

static void cetl_client_test_write(void)
{
#if CETL_TEST_WRITE_SEND_DATA
    static u32 count = 0;
    static u32 send_index[SUPPORT_MAX_GATT_CLIENT];

    int i, ret = 0;
    int send_len = CETL_TEST_PAYLOAD_LEN;
    u16 tmp_handle;
    u32 time_index_max = 1000 / TEST_TIMER_MS;

    count++;
    for (i = 0; i < SUPPORT_MAX_GATT_CLIENT; i++) {
        send_index[i]++;

        tmp_handle = ble_comm_dev_get_handle(i, GATT_ROLE_CLIENT);

#if CETL_TEST_SEND_EN
        if (tmp_handle && cetl_ble_client_write_handle && search_profile_complete_flag[i]) {
            do {
                if (ble_comm_att_check_send(tmp_handle, send_len)) {
                    ret = ble_comm_att_send_data(tmp_handle, cetl_ble_client_write_handle, &count, send_len, ATT_OP_WRITE_WITHOUT_RESPOND);

#if CETL_TEST_DISPLAY_DATA
                    log_info("test_write:%04x,%d", tmp_handle, ret);
#endif
                    if (!ret) {
                        /* putchar('T'); */
                        send_test_count[i] += send_len;
                    }
                } else {
                    break;
                }
            } while (ret == 0);
        }
#endif

        if (send_index[i] >= time_index_max) {
            if (send_test_count[i]) {
                log_info("conn_handle:%04x,send_rate= %d byte/s\n", tmp_handle, send_test_count[i]);
            }
            send_index[i] = 0;
            send_test_count[i] = 0;

            if (recieve_test_count[i]) {
                log_info("conn_handle:%04x,recieve_rate= %d byte/s\n", tmp_handle, recieve_test_count[i]);
                recieve_test_count[i] = 0;
            }
        }
    }
#endif
}

static const gatt_search_cfg_t cetl_client_search_config = {
    .match_devices = &cetl_match_device_table,
    .match_devices_count = (sizeof(cetl_match_device_table) / sizeof(client_match_cfg_t)),
    .match_rssi_enable = 0,

    .search_uuid_count = (sizeof(jl_cetl_search_uuid_table) / sizeof(target_uuid_t)),
    .search_uuid_group = jl_cetl_search_uuid_table,
    .auto_enable_ccc = 1,
};

//-------------------------------------------------------------------------------------
static int cetl_client_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param)
{
    int i;
    /* log_info("event: %02x,size= %d\n",event,size); */
    switch (event) {
    case GATT_COMM_EVENT_GATT_DATA_REPORT: {
        att_data_report_t *report_data = (void *)packet;

#if CETL_TEST_DISPLAY_DATA
        log_info("data_report:hdl=%04x,pk_type=%02x,size=%d\n", report_data->conn_handle, report_data->packet_type, report_data->blob_length);
        put_buf(report_data->blob, report_data->blob_length);
#else
        /* putchar('R'); */
#endif
        i = ble_comm_dev_get_index(report_data->conn_handle, SUPPORT_MAX_GATT_CLIENT);
        if (i != INVAIL_INDEX) {
            recieve_test_count[i] += report_data->blob_length;
        }

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

    case GATT_COMM_EVENT_SCAN_ADV_REPORT:
        putchar('V');
        /* log_info("adv_report_data(%d):",size); */
        /* put_buf(packet,size);  */
        break;

    case GATT_COMM_EVENT_CONNECTION_COMPLETE:
        if (!cetl_con_handle) {
            cetl_con_handle = little_endian_read_16(packet, 0);
#if CONFIG_BT_GATT_SERVER_NUM
            central_set_server_conn_handle(cetl_con_handle);
#endif
        }
        log_info("connection_handle:%04x, rssi= %d\n", cetl_con_handle, ble_vendor_get_peer_rssi(cetl_con_handle));
        log_info("peer_address_info:");
        put_buf(&ext_param[7], 7);
        break;

    case GATT_COMM_EVENT_DISCONNECT_COMPLETE:
        log_info("disconnect_handle:%04x,reason= %02x\n", little_endian_read_16(packet, 0), packet[2]);

        if (cetl_con_handle == little_endian_read_16(packet, 0)) {
            cetl_con_handle = 0;
#if CONFIG_BT_GATT_SERVER_NUM
            central_set_server_conn_handle(cetl_con_handle);
#endif
        }

        i = ble_comm_dev_get_index(little_endian_read_16(packet, 0), SUPPORT_MAX_GATT_CLIENT);
        if (i != INVAIL_INDEX) {
            search_profile_complete_flag[i] = 0;
        }
        break;

    case GATT_COMM_EVENT_ENCRYPTION_CHANGE:
        log_info("ENCRYPTION_CHANGE:handle=%04x,state=%d,process =%d", little_endian_read_16(packet, 0), packet[2], packet[3]);
        if (packet[3] == LINK_ENCRYPTION_RECONNECT) {
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

    case GATT_COMM_EVENT_GATT_SEARCH_MATCH_UUID: {
        opt_handle_t *opt_hdl = packet;
        log_info("match:server_uuid= %04x,charactc_uuid= %04x,value_handle= %04x\n", \
                 opt_hdl->search_uuid->services_uuid16, opt_hdl->search_uuid->characteristic_uuid16, opt_hdl->value_handle);
#if CETL_TEST_WRITE_SEND_DATA
        //for test
        if (opt_hdl->search_uuid->characteristic_uuid16 == 0xae01) {
            cetl_ble_client_write_handle = opt_hdl->value_handle;
        }
#endif
    }
    break;

    case GATT_COMM_EVENT_MTU_EXCHANGE_COMPLETE:
        log_info("con_handle= %02x, ATT MTU = %u\n", little_endian_read_16(packet, 0), little_endian_read_16(packet, 2));
        break;

    case GATT_COMM_EVENT_GATT_SEARCH_PROFILE_COMPLETE: {
        log_info("GATT_SEARCH_PROFILE_COMPLETE:%04x\n", little_endian_read_16(packet, 0));
        i = ble_comm_dev_get_index(little_endian_read_16(packet, 0), SUPPORT_MAX_GATT_CLIENT);
        if (i != INVAIL_INDEX) {
            search_profile_complete_flag[i] = 1;
        }
    }
    break;

    case GATT_COMM_EVENT_SM_PASSKEY_INPUT: {
        u32 *key = little_endian_read_32(packet, 2);
        *key = 888888;
        log_info("input_key:%6u\n", *key);
    }
    break;

    default:
        break;
    }
    return 0;
}

//scan参数设置
static void cetl_scan_conn_config_set(void)
{
    cetl_client_scan_cfg.scan_auto_do = 1;
    cetl_client_scan_cfg.creat_auto_do = 1;
    cetl_client_scan_cfg.scan_type = SET_SCAN_TYPE;
    cetl_client_scan_cfg.scan_filter = 1;
    cetl_client_scan_cfg.scan_interval = SET_SCAN_INTERVAL;
    cetl_client_scan_cfg.scan_window = SET_SCAN_WINDOW;

    cetl_client_scan_cfg.creat_conn_interval = SET_CONN_INTERVAL;
    cetl_client_scan_cfg.creat_conn_latency = SET_CONN_LATENCY;
    cetl_client_scan_cfg.creat_conn_super_timeout = SET_CONN_TIMEOUT;
    cetl_client_scan_cfg.conn_update_accept = 1;
    cetl_client_scan_cfg.creat_state_timeout_ms = SET_CREAT_CONN_TIMEOUT;

    ble_gatt_client_set_scan_config(&cetl_client_scan_cfg);
}

void cetl_client_init(void)
{
    log_info("%s", __FUNCTION__);

#if  MATCH_CONFIG_NAME
    log_info("set match config name\n");
    cetl_match_device_table[0].compare_data =  bt_get_local_name();
    cetl_match_device_table[0].compare_data_len = strlen(bt_get_local_name());
#endif

    ble_gatt_client_set_search_config(&cetl_client_search_config);
    //ble_gatt_client_set_search_config(NULL);
    cetl_scan_conn_config_set();

#if CETL_TEST_WRITE_SEND_DATA
    if (TEST_TIMER_MS < 10) {
        sys_s_hi_timer_add(0, cetl_client_test_write, TEST_TIMER_MS);
    } else {
        sys_timer_add(0, cetl_client_test_write, TEST_TIMER_MS);
    }
#endif
}


//-------------------------------------------------------------------------------------

void bt_ble_before_start_init(void)
{
    ble_comm_init(&cetl_gatt_control_block);
}

void bt_ble_init(void)
{
    log_info("%s\n", __FUNCTION__);
    cetl_con_handle = 0;
    ble_comm_set_config_name(bt_get_local_name(), 1);

#if CONFIG_BT_GATT_CLIENT_NUM
    cetl_client_init();
#endif

#if CONFIG_BT_GATT_SERVER_NUM
    central_server_init();
#endif

    ble_module_enable(1);
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

/*************************************************************************************************/
/*!
 *  \brief      断开连接
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void cetl_disconnect(void)
{
    log_info("%s", __FUNCTION__);
    if (cetl_con_handle) {
        ble_comm_disconnect(cetl_con_handle);
    }
}



#endif


