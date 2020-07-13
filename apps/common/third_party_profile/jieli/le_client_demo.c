/*********************************************************************************************
    *   Filename        : le_server_module.c

    *   Description     :

    *   Author          :

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2017-01-17 11:14

    *   Copyright:(c)JIELI  2011-2016  @ , All Rights Reserved.
*********************************************************************************************/

// *****************************************************************************
/* EXAMPLE_START(le_counter): LE Peripheral - Heartbeat Counter over GATT
 *
 * @text All newer operating systems provide GATT Client functionality.
 * The LE Counter examples demonstrates how to specify a minimal GATT Database
 * with a custom GATT Service and a custom Characteristic that sends periodic
 * notifications.
 */
// *****************************************************************************

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

#include "le_client_demo.h"
#include "le_common.h"
#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_CLIENT)

#define SUPPORT_TEST_BOX_BLE_MASTER_TEST_EN	1
#define TEST_SEND_DATA_RATE         0  //测试 send_data
#define SHOW_RX_DATA_RATE           0
#define EXT_ADV_MODE_EN             0

#if 1
#define log_info            printf
#define log_info_hexdump    put_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

//------
#define ATT_LOCAL_PAYLOAD_SIZE    (64*2)                    //note: need >= 20
#define ATT_SEND_CBUF_SIZE        (512)                   //note: need >= 20,缓存大小，可修改
#define ATT_RAM_BUFSIZE           (ATT_CTRL_BLOCK_SIZE + ATT_LOCAL_PAYLOAD_SIZE + ATT_SEND_CBUF_SIZE)                   //note:
static u8 att_ram_buffer[ATT_RAM_BUFSIZE] __attribute__((aligned(4)));

#define SEARCH_PROFILE_BUFSIZE    (512)                   //note:
static u8 search_ram_buffer[SEARCH_PROFILE_BUFSIZE] __attribute__((aligned(4)));
#define scan_buffer search_ram_buffer
//---------------

#define SET_SCAN_TYPE       SCAN_ACTIVE
#define SET_SCAN_INTERVAL   48
#define SET_SCAN_WINDOW     16

#define SET_CONN_INTERVAL   24 
#define SET_CONN_LATENCY    0
#define SET_CONN_TIMEOUT    400 
//----------------------------------------------------------------------------
static u8 scan_ctrl_en;
static u8 ble_work_state = 0;
static void (*app_recieve_callback)(void *priv, void *buf, u16 len) = NULL;
static void (*app_ble_state_callback)(void *priv, ble_state_e state) = NULL;
static void (*ble_resume_send_wakeup)(void) = NULL;
static u32 channel_priv;

static hci_con_handle_t con_handle;
static const char user_tag_string[] = {0xd6, 0x05, 'j', 'i', 'e', 'l', 'i' };
static const u8 create_conn_remoter[6] = {0x11, 0x22, 0x33, 0x88, 0x88, 0x88};

static u8 create_conn_mode = BIT(CLI_CREAT_BY_NAME);// BIT(CLI_CREAT_BY_ADDRESS) | BIT(CLI_CREAT_BY_NAME)

#if EXT_ADV_MODE_EN
static const u8 create_remoter_name[] = "JL_EXT_ADV";
#else
static const u8 create_remoter_name[] = "123456(BLE)";
/* static const u8 create_remoter_name[] = "AC630N_1(BLE)"; */
#endif

//-------------------------------------------------------------------------------
//指定搜索uuid
static const target_uuid_t  test_search_uuid_table[] = {

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
//----------------------------------------------------------------------------------------
//test target uuid
#define TEST_SEARCH_UUID_CNT  (sizeof(test_search_uuid_table)/sizeof(target_uuid_t))

typedef struct {
    uint16_t read_handle;
    uint16_t read_long_handle;
    uint16_t write_handle;
    uint16_t write_no_respond;
    uint16_t notify_handle;
    uint16_t indicate_handle;
} target_hdl_t;

//记录handle 使用
static target_hdl_t target_handle;
static opt_handle_t opt_handle_table[OPT_HANDLE_MAX];
static u8 opt_handle_used_cnt;

static const client_conn_cfg_t test_conn_config = 
{
	.create_conn_mode = BIT(CLI_CREAT_BY_NAME),	
	.compare_data_len = sizeof(create_remoter_name)-1,
	.compare_data = create_remoter_name,
	.report_data_callback = NULL,
	.search_uuid_cnt = TEST_SEARCH_UUID_CNT,
	.search_uuid_table = test_search_uuid_table,  
};

/* static const client_conn_cfg_t test_conn_config =  */
/* { */
	/* .create_conn_mode = BIT(CLI_CREAT_BY_TAG),	 */
	/* .compare_data_len = sizeof(user_tag_string), */
	/* .compare_data = user_tag_string, */
	/* .report_data_callback = NULL, */
	/* .search_uuid_cnt = TEST_SEARCH_UUID_CNT, */
	/* .search_uuid_table = test_search_uuid_table,   */
/* }; */


static  client_conn_cfg_t *client_config =  &test_conn_config;
#define CREAT_CONN_CHECK_FLAG(a)            (client_config->create_conn_mode & BIT(a))

//----------------------------------------------------------------------------
static void bt_ble_create_connection(u8 *conn_addr, u8 addr_type);
static int bt_ble_scan_enable(void *priv, u32 en);
static int client_write_send(void *priv, u8 *data, u16 len);
static int client_operation_send(u16 handle, u8 *data, u16 len, u8 att_op_type);

static const struct conn_update_param_t connection_param_table[] = {
    {16, 24, 0, 600},//11
    {12, 28, 0, 600},//3.7
    {8,  20, 0, 600},
    {50, 60, 0, 600},
};

static u8 send_param_index = 3;
void client_send_conn_param_update(void)
{
    struct conn_update_param_t *param = (void *)&connection_param_table[send_param_index];//for test
    log_info("client update param:-%d-%d-%d-%d-\n", param->interval_min, param->interval_max, param->latency, param->timeout);
    if (con_handle) {
        ble_user_cmd_prepare(BLE_CMD_ONNN_PARAM_UPDATA, 2, con_handle, param);
    }
}

/* static void test_send_conn_update(void) */
/* { */
/* client_send_conn_param_update(); */
/* send_param_index++; */
/* send_param_index &= 3; */
/* sys_timeout_add(0,test_send_conn_update,10000); */
/* } */


//------------------------------------------------------------
static void set_ble_work_state(ble_state_e state)
{
    if (state != ble_work_state) {
        log_info("ble_client_work_st:%x->%x\n", ble_work_state, state);
        ble_work_state = state;
        if (app_ble_state_callback) {
            app_ble_state_callback((void *)channel_priv, state);
        }
    }
}
//------------------------------------------------------------
static ble_state_e get_ble_work_state(void)
{
    return ble_work_state;
}

//-------------------------------------------------------------------------------
static void check_target_uuid_match(search_result_t *result_info)
{
    u32 i;
    target_uuid_t *t_uuid;

    for (i = 0; i < client_config->search_uuid_cnt; i++) {
        t_uuid = &client_config->search_uuid_table[i];
        if (result_info->services.uuid16) {
            if (result_info->services.uuid16 != t_uuid->services_uuid16) {
                /* log_info("b1"); */
                continue;
            }
        } else {
            if (memcmp(result_info->services.uuid128, t_uuid->services_uuid128, 16)) {
                /* log_info("b2"); */
                continue;
            }
        }

        if (result_info->characteristic.uuid16) {
            if (result_info->characteristic.uuid16 != t_uuid->characteristic_uuid16) {
                /* log_info("b3"); */
                /* log_info("%d: %04x--%04x",result_info->characteristic.uuid16,t_uuid->characteristic_uuid16); */
                continue;
            }
        } else {
            if (memcmp(result_info->characteristic.uuid128, t_uuid->characteristic_uuid128, 16)) {
                /* log_info("b4"); */
                continue;
            }
        }

        break;//match one
    }

    if (i >= client_config->search_uuid_cnt) {
        return;
    }

    if (opt_handle_used_cnt >= OPT_HANDLE_MAX) {
        log_info("opt_handle is full!!!\n");
        return;
    }

    if ((t_uuid->opt_type & result_info->characteristic.properties) != t_uuid->opt_type) {
        log_info("properties not match!!!\n");
        return;
    }

    log_info("match one uuid\n");

    opt_handle_t *opt_get = &opt_handle_table[opt_handle_used_cnt++];
    opt_get->value_handle = result_info->characteristic.value_handle;
    opt_get->search_uuid = t_uuid;

    switch (t_uuid->opt_type) {
    case ATT_PROPERTY_READ:
        target_handle.read_handle = result_info->characteristic.value_handle;
        break;

    case ATT_PROPERTY_WRITE_WITHOUT_RESPONSE:
        target_handle.write_no_respond = result_info->characteristic.value_handle;
        break;

    case ATT_PROPERTY_WRITE:
        target_handle.write_handle = result_info->characteristic.value_handle;
        break;

    case ATT_PROPERTY_NOTIFY:
        target_handle.notify_handle  = result_info->characteristic.value_handle;
        break;

    case ATT_PROPERTY_INDICATE:
        target_handle.indicate_handle = result_info->characteristic.value_handle;
        break;

    default:
        break;
    }
}

//操作handle，完成 write ccc
static void do_operate_search_handle(void)
{
    u16 tmp_16;
    u16 i, cur_opt_type;
    opt_handle_t *opt_hdl_pt;
    
	log_info("opt_handle_used_cnt= %d\n", opt_handle_used_cnt);

    log_info("find target_handle:");
    log_info_hexdump(&target_handle, sizeof(target_hdl_t));

    if (0 == opt_handle_used_cnt) {
        return;
    }

    /* test_send_conn_update();//for test */

    for (i = 0; i < opt_handle_used_cnt; i++) {
        opt_hdl_pt = &opt_handle_table[i];
        cur_opt_type = opt_hdl_pt->search_uuid->opt_type;
        switch ((u8)cur_opt_type) {
        case ATT_PROPERTY_READ:
            if (1) {
                tmp_16  = 0x55A2;//fixed
                log_info("read_long:%04x\n", opt_hdl_pt->value_handle);
                client_operation_send(opt_hdl_pt->value_handle, (u8 *)&tmp_16, 2, ATT_OP_READ_LONG);
            } else {
                tmp_16  = 0x55A1;//fixed
                log_info("read:%04x\n", opt_hdl_pt->value_handle);
                client_operation_send(opt_hdl_pt->value_handle, (u8 *)&tmp_16, 2, ATT_OP_READ);
            }
            break;

        case ATT_PROPERTY_NOTIFY:
            tmp_16  = 0x01;//fixed
            log_info("write_ntf_ccc:%04x\n", opt_hdl_pt->value_handle);
            client_operation_send(opt_hdl_pt->value_handle + 1, &tmp_16, 2, ATT_OP_WRITE);
            break;

        case ATT_PROPERTY_INDICATE:
            tmp_16  = 0x02;//fixed
            log_info("write_ind_ccc:%04x\n", opt_hdl_pt->value_handle);
            client_operation_send(opt_hdl_pt->value_handle + 1, &tmp_16, 2, ATT_OP_WRITE);
            break;

        default:
            break;
        }
    }

    set_ble_work_state(BLE_ST_SEARCH_COMPLETE);

}

//return: 0--accept,1--reject
int l2cap_connection_update_request_just(u8 *packet)
{
    log_info("slave request conn_update:\n-interval_min= %d,\n-interval_max= %d,\n-latency= %d,\n-timeout= %d\n",
             little_endian_read_16(packet, 0), little_endian_read_16(packet, 2),
             little_endian_read_16(packet, 4), little_endian_read_16(packet, 6));
    return 0;
    /* return 1; */
}

void user_client_report_search_result(search_result_t *result_info)
{
    if (result_info == (void *) - 1) {
        log_info("client_report_search_result finish!!!\n");
        do_operate_search_handle();
        return;
    }

    log_info("\n*** services, uuid16:%04x,index=%d ***\n", result_info->services.uuid16, result_info->service_index);
    log_info("{charactc, uuid16:%04x,index=%d,handle:%04x~%04x,value_handle=%04x}\n",
             result_info->characteristic.uuid16, result_info->characteristic_index,
             result_info->characteristic.start_handle, result_info->characteristic.end_handle,
             result_info->characteristic.value_handle
            );

    if (!result_info->services.uuid16) {
        log_info("######services_uuid128:");
        log_info_hexdump(result_info->services.uuid128, 16);
    }

    if (!result_info->characteristic.uuid16) {
        log_info("######charact_uuid128:");
        log_info_hexdump(result_info->characteristic.uuid128, 16);
    }

    check_target_uuid_match(result_info);
}

#if SHOW_RX_DATA_RATE

static u32 client_timer_handle = 0;
static u32 test_data_count;
extern s8 ble_vendor_get_peer_rssi(u16 conn_handle);
static void client_timer_handler(void)
{
    if (!con_handle) {
        test_data_count = 0;
        return;
    }
    log_info("peer_rssi = %d\n", ble_vendor_get_peer_rssi(con_handle));

    if (test_data_count) {
        /* log_info("\n-ble_data_rate: %d bps-\n", test_data_count * 8); */
        log_info("\n%d bytes receive: %d.%02d KB/s \n", test_data_count, test_data_count / 1000, test_data_count % 1000);
        test_data_count = 0;
    }
}

static void client_timer_start(void)
{
    client_timer_handle  = sys_timer_add(NULL, client_timer_handler, 1000);
}

#endif /* SHOW_RX_DATA_RATE */

static target_uuid_t *get_match_handle_target(u16 handle)
{
	for(int i = 0;i < opt_handle_used_cnt;i++){
		if(opt_handle_table[i].value_handle == handle){
			return opt_handle_table[i].search_uuid;
		}
	}
	return NULL;
}

void user_client_report_data_callback(att_data_report_t *report_data)
{
    /* log_info("\n-report_data:type %02x,handle %04x,offset %d,len %d:",report_data->packet_type, */
    /* 		report_data->value_handle,report_data->value_offset,report_data->blob_length); */
    /* log_info_hexdump(report_data->blob,report_data->blob_length); */

#if SHOW_RX_DATA_RATE
    test_data_count += report_data->blob_length;
#endif /* SHOW_RX_DATA_RATE */

	target_uuid_t *search_uuid = get_match_handle_target(report_data->value_handle);

	if(client_config->report_data_callback){
		client_config->report_data_callback(report_data,search_uuid);
		return;
	}

    switch (report_data->packet_type) {
    case GATT_EVENT_NOTIFICATION://notify
//        log_info("\n-notify_rx(%d):",report_data->blob_length);
    case GATT_EVENT_INDICATION://indicate
    case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT://read
    case GATT_EVENT_LONG_CHARACTERISTIC_VALUE_QUERY_RESULT://read long
        break;
    default:
        break;
    }
}

static void client_search_profile_start(void)
{
    opt_handle_used_cnt = 0;
    memset(&target_handle, 0, sizeof(target_hdl_t));
    user_client_init(con_handle, search_ram_buffer, SEARCH_PROFILE_BUFSIZE);
    ble_user_cmd_prepare(BLE_CMD_SEARCH_PROFILE, 2, PFL_SERVER_ALL, 0);
}

//------------------------------------------------------------
static bool resolve_adv_report(u8 *adv_address, u8 data_length, u8 *data)
{
    u8 i, lenght, ad_type;
    u8 *adv_data_pt;
    u8 find_remoter = 0;
    u8 tmp_addr[6];
    u32 tmp32;

	if (CREAT_CONN_CHECK_FLAG(CLI_CREAT_BY_ADDRESS)) {
		swapX(client_config->compare_data, tmp_addr, 6);
		if (0 == memcmp(tmp_addr, adv_address, 6)) {
			find_remoter = 1;
		}
	}

    adv_data_pt = data;
    for (i = 0; i < data_length;) {
        if (*adv_data_pt == 0) {
            /* log_info("analyze end\n"); */
            break;
        }

        lenght = *adv_data_pt++;
        ad_type = *adv_data_pt++;
        i += (lenght + 1);

        switch (ad_type) {
        case HCI_EIR_DATATYPE_FLAGS:
            /* log_info("flags:%02x\n",adv_data_pt[0]); */
            break;

        case HCI_EIR_DATATYPE_MORE_16BIT_SERVICE_UUIDS:
        case HCI_EIR_DATATYPE_COMPLETE_16BIT_SERVICE_UUIDS:
        case HCI_EIR_DATATYPE_MORE_32BIT_SERVICE_UUIDS:
        case HCI_EIR_DATATYPE_COMPLETE_32BIT_SERVICE_UUIDS:
        case HCI_EIR_DATATYPE_MORE_128BIT_SERVICE_UUIDS:
        case HCI_EIR_DATATYPE_COMPLETE_128BIT_SERVICE_UUIDS:
            /* log_info("service uuid:"); */
            /* log_info_hexdump(adv_data_pt, lenght - 1); */
            break;

        case HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME:
        case HCI_EIR_DATATYPE_SHORTENED_LOCAL_NAME:
            tmp32 = adv_data_pt[lenght - 1];
            adv_data_pt[lenght - 1] = 0;;
            log_info("remoter_name: %s\n", adv_data_pt);
            log_info_hexdump(adv_address, 6);
            adv_data_pt[lenght - 1] = tmp32;
            //-------
#if SUPPORT_TEST_BOX_BLE_MASTER_TEST_EN
#define TEST_BOX_BLE_NAME		"JLBT_TESTBOX"
#define TEST_BOX_BLE_NAME_LEN	0xc
            if (0 == memcmp(adv_data_pt, TEST_BOX_BLE_NAME, TEST_BOX_BLE_NAME_LEN)) {
                find_remoter = 1;
                break;
            }
#endif
            if (CREAT_CONN_CHECK_FLAG(CLI_CREAT_BY_NAME)) {
                if (0 == memcmp(adv_data_pt,client_config->compare_data,client_config->compare_data_len)) {
                    find_remoter = 1;
                    log_info("catch name ok\n");
                }
            }
            break;

        case HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA:
            if (CREAT_CONN_CHECK_FLAG(CLI_CREAT_BY_TAG)) {
                if (0 == memcmp(adv_data_pt,client_config->compare_data,client_config->compare_data_len)) {
                    log_info("get_tag_string!\n");
                    find_remoter = 1;
                }
            }
            break;

        case HCI_EIR_DATATYPE_APPEARANCE_DATA:
            /* log_info("get_class_type:%04x\n",little_endian_read_16(adv_data_pt,0)); */
            break;

        default:
            /* log_info("unknow ad_type:"); */
            break;
        }

        if (find_remoter) {
            log_info_hexdump(adv_data_pt, lenght - 1);
        }
        adv_data_pt += (lenght - 1);
    }

    return find_remoter;
}

static void client_report_adv_data(adv_report_t *report_pt, u16 len)
{
    bool find_remoter;

    /* log_info("event_type,addr_type: %x,%x; ",report_pt->event_type,report_pt->address_type); */
    /* log_info_hexdump(report_pt->address,6); */

    /* log_info("adv_data_display:"); */
    /* log_info_hexdump(report_pt->data,report_pt->length); */

//    log_info("rssi:%d\n",report_pt->rssi);

    find_remoter = resolve_adv_report(report_pt->address, report_pt->length, report_pt->data);

    if (find_remoter) {
        log_info("rssi:%d\n", report_pt->rssi);
        log_info("\n*********create_connection***********\n");
        log_info("***remote type %d,addr:", report_pt->address_type);
        log_info_hexdump(report_pt->address, 6);
        bt_ble_scan_enable(0, 0);
        bt_ble_create_connection(report_pt->address, report_pt->address_type);
    }
}

#if EXT_ADV_MODE_EN

#define GET_STRUCT_MEMBER_OFFSET(type, member) \
    (u32)&(((struct type*)0)->member)

struct __ext_adv_report_event {
    u8  Subevent_Code;
    u8  Num_Reports;
    u16 Event_Type;
    u8  Address_Type;
    u8  Address[6];
    u8  Primary_PHY;
    u8  Secondary_PHY;
    u8  Advertising_SID;
    u8  Tx_Power;
    u8  RSSI;
    u16 Periodic_Advertising_Interval;
    u8  Direct_Address_Type;
    u8  Direct_Address[6];
    u8  Data_Length;
    u8  Data[0];
} _GNU_PACKED_;

static void client_report_ext_adv_data(u8 *report_pt, u16 len)
{
    bool find_remoter;
    u8 address_type = report_pt[GET_STRUCT_MEMBER_OFFSET(__ext_adv_report_event, Address_Type)];
    u8 address[6];

    /* log_info("--func=%s", __FUNCTION__); */
    /* log_info_hexdump(report_pt, len); */

    memcpy(address, &report_pt[GET_STRUCT_MEMBER_OFFSET(__ext_adv_report_event, Address)], 6);

    find_remoter = resolve_adv_report(\
                                      address, \
                                      report_pt[GET_STRUCT_MEMBER_OFFSET(__ext_adv_report_event, Data_Length)], \
                                      &report_pt[GET_STRUCT_MEMBER_OFFSET(__ext_adv_report_event, Data)] \
                                     );

    if (find_remoter) {
        log_info("\n*********ext create_connection***********\n");
        log_info("***remote type %d, addr:", address_type);
        log_info_hexdump(address, 6);
        bt_ble_scan_enable(0, 0);
        bt_ble_create_connection(address, address_type);
    }
}

struct __ext_init {
    u8 Initiating_Filter_Policy;
    u8 Own_Address_Type;
    u8 Peer_Address_Type;
    u8 Peer_Address[6];
    u8 Initiating_PHYs;
    u16 Scan_Interval;
    u16 Scan_Window;
    u16 Conn_Interval_Min;
    u16 Conn_Interval_Max;
    u16 Conn_Latency;
    u16 Supervision_Timeout;
    u16 Minimum_CE_Length;
    u16 Maximum_CE_Length;
} _GNU_PACKED_;

static void client_ext_create_connection(u8 *conn_addr, u8 addr_type)
{
    struct __ext_init *create_conn_par = scan_buffer;

    if (get_ble_work_state() == BLE_ST_CREATE_CONN) {
        log_info("already create conn!!!\n");
        return;
    }

    memset(create_conn_par, 0, sizeof(*create_conn_par));
    create_conn_par->Conn_Interval_Min = SET_CONN_INTERVAL;
    create_conn_par->Conn_Interval_Max = SET_CONN_INTERVAL;
    create_conn_par->Conn_Latency = SET_CONN_LATENCY;
    create_conn_par->Supervision_Timeout = SET_CONN_TIMEOUT;
    create_conn_par->Peer_Address_Type = addr_type;
    create_conn_par->Initiating_PHYs = INIT_SET_1M_PHY;
    memcpy(create_conn_par->Peer_Address, conn_addr, 6);

    set_ble_work_state(BLE_ST_CREATE_CONN);

    log_info_hexdump(create_conn_par, sizeof(*create_conn_par));

    ble_user_cmd_prepare(BLE_CMD_EXT_CREATE_CONN, 2, create_conn_par, sizeof(*create_conn_par));
}

#endif /* EXT_ADV_MODE_EN */

static void client_create_connection(u8 *conn_addr, u8 addr_type)
{
    struct create_conn_param_t *create_conn_par = scan_buffer;
    if (get_ble_work_state() == BLE_ST_CREATE_CONN) {
        log_info("already create conn!!!\n");
        return;
    }
    create_conn_par->conn_interval = SET_CONN_INTERVAL;
    create_conn_par->conn_latency = SET_CONN_LATENCY;
    create_conn_par->supervision_timeout = SET_CONN_TIMEOUT;
    memcpy(create_conn_par->peer_address, conn_addr, 6);
    create_conn_par->peer_address_type = addr_type;

    set_ble_work_state(BLE_ST_CREATE_CONN);
    log_info_hexdump(create_conn_par, sizeof(struct create_conn_param_t));
    ble_user_cmd_prepare(BLE_CMD_CREATE_CONN, 1, create_conn_par);
}

static void bt_ble_create_connection(u8 *conn_addr, u8 addr_type)
{
#if EXT_ADV_MODE_EN
    client_ext_create_connection(conn_addr, addr_type);
#else
    client_create_connection(conn_addr, addr_type);
#endif /* EXT_ADV_MODE_EN */
}

static void client_create_connection_cannel(void)
{
    set_ble_work_state(BLE_ST_SEND_CREATE_CONN_CANNEL);
    ble_user_cmd_prepare(BLE_CMD_CREATE_CONN_CANCEL, 0);
}

static int client_disconnect(void *priv)
{
    if (con_handle) {
        if (BLE_ST_SEND_DISCONN != get_ble_work_state()) {
            log_info(">>>ble send disconnect\n");
            set_ble_work_state(BLE_ST_SEND_DISCONN);
            ble_user_cmd_prepare(BLE_CMD_DISCONNECT, 1, con_handle);
        } else {
            log_info(">>>ble wait disconnect...\n");
        }
        return APP_BLE_NO_ERROR;
    } else {
        return APP_BLE_OPERATION_ERROR;
    }
}

static void connection_update_complete_success(u8 *packet)
{
    int con_handle, conn_interval, conn_latency, conn_timeout;

    conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
    conn_latency = hci_subevent_le_connection_update_complete_get_conn_latency(packet);
    conn_timeout = hci_subevent_le_connection_update_complete_get_supervision_timeout(packet);

    log_info("conn_interval = %d\n", conn_interval);
    log_info("conn_latency = %d\n", conn_latency);
    log_info("conn_timeout = %d\n", conn_timeout);
}


static void cbk_sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    sm_just_event_t *event = (void *)packet;
    u32 tmp32;
    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
        case SM_EVENT_JUST_WORKS_REQUEST:
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            log_info("Just Works Confirmed.\n");
            break;
        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            log_info_hexdump(packet, size);
            memcpy(&tmp32, event->data, 4);
            log_info("Passkey display: %06u.\n", tmp32);
            break;
        }
        break;
    }
}

static void can_send_now_wakeup(void)
{
    putchar('E');
    if (ble_resume_send_wakeup) {
        ble_resume_send_wakeup();
    }
}

const char *const phy_result[] = {
    "None",
    "1M",
    "2M",
    "Coded",
};

static void set_connection_data_length(u16 tx_octets, u16 tx_time)
{
    if (con_handle) {
        ble_user_cmd_prepare(BLE_CMD_SET_DATA_LENGTH, 3, con_handle, tx_octets, tx_time);
    }
}

static void set_connection_data_phy(u8 tx_phy, u8 rx_phy)
{
    if (0 == con_handle) {
        return;
    }

    u8 all_phys = 0;
    u16 phy_options = 0;

    ble_user_cmd_prepare(BLE_CMD_SET_PHY, 5, con_handle, all_phys, tx_phy, rx_phy, phy_options);
}

static void client_profile_start(u16 con_handle)
{
    ble_user_cmd_prepare(BLE_CMD_ATT_SEND_INIT, 4, con_handle, att_ram_buffer, ATT_RAM_BUFSIZE, ATT_LOCAL_PAYLOAD_SIZE);
    set_ble_work_state(BLE_ST_CONNECT);

#if (TCFG_BLE_SECURITY_EN == 0)
    client_search_profile_start();
#endif
}

/* LISTING_START(packetHandler): Packet Handler */
static void cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    int mtu;
    u32 tmp;
    u8 status;

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {

        /* case DAEMON_EVENT_HCI_PACKET_SENT: */
        /* break; */
        case ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE:
            log_info("ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE\n");
        case ATT_EVENT_CAN_SEND_NOW:
            can_send_now_wakeup();
            break;

        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {

            case HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE:
                status = hci_subevent_le_enhanced_connection_complete_get_status(packet);
                if (status) {
                    log_info("LE_MASTER CREATE CONNECTION FAIL!!! %0x\n", status);
                    set_ble_work_state(BLE_ST_DISCONN);
                    break;
                }
                con_handle = hci_subevent_le_enhanced_connection_complete_get_connection_handle(packet);
                log_info("HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE : 0x%0x\n", con_handle);
                log_info("conn_interval = %d\n", hci_subevent_le_enhanced_connection_complete_get_conn_interval(packet));
                log_info("conn_latency = %d\n", hci_subevent_le_enhanced_connection_complete_get_conn_latency(packet));
                log_info("conn_timeout = %d\n", hci_subevent_le_enhanced_connection_complete_get_supervision_timeout(packet));
                client_profile_start(con_handle);
                break;

            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                if (packet[3]) {
                    log_info("LE_MASTER CREATE CONNECTION FAIL!!! %0x\n", packet[3]);
                    set_ble_work_state(BLE_ST_DISCONN);
                    break;
                }
                con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                log_info("HCI_SUBEVENT_LE_CONNECTION_COMPLETE : %0x\n", con_handle);
                connection_update_complete_success(packet + 8);
                client_profile_start(con_handle);
                break;

            case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                connection_update_complete_success(packet);
                break;

            case HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE:
                log_info("APP HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE");
                break;

            case HCI_SUBEVENT_LE_PHY_UPDATE_COMPLETE:
                log_info("APP HCI_SUBEVENT_LE_PHY_UPDATE %s\n", hci_event_le_meta_get_phy_update_complete_status(packet) ? "Fail" : "Succ");
                log_info("Tx PHY: %s\n", phy_result[hci_event_le_meta_get_phy_update_complete_tx_phy(packet)]);
                log_info("Rx PHY: %s\n", phy_result[hci_event_le_meta_get_phy_update_complete_rx_phy(packet)]);
                break;

#if EXT_ADV_MODE_EN
            case HCI_SUBEVENT_LE_EXTENDED_ADVERTISING_REPORT:
                /* log_info("APP HCI_SUBEVENT_LE_EXTENDED_ADVERTISING_REPORT"); */
                /* log_info_hexdump(packet, size); */
                client_report_ext_adv_data(&packet[2], packet[1]);
                break;
#endif /* EXT_ADV_MODE_EN */
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            log_info("HCI_EVENT_DISCONNECTION_COMPLETE: %0x\n", packet[5]);
            con_handle = 0;
            ble_user_cmd_prepare(BLE_CMD_ATT_SEND_INIT, 4, con_handle, 0, 0, 0);
            set_ble_work_state(BLE_ST_DISCONN);
            bt_ble_scan_enable(0, 1);
            break;

        case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
            mtu = att_event_mtu_exchange_complete_get_MTU(packet) - 3;
            log_info("ATT MTU = %u\n", mtu);
            ble_user_cmd_prepare(BLE_CMD_ATT_MTU_SIZE, 1, mtu);
            break;

        case HCI_EVENT_VENDOR_REMOTE_TEST:
            log_info("--- HCI_EVENT_VENDOR_REMOTE_TEST\n");
            break;

        case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE:
            tmp = little_endian_read_16(packet, 4);
            log_info("-update_rsp: %02x\n", tmp);
            break;

        case GAP_EVENT_ADVERTISING_REPORT:
            client_report_adv_data((void *)&packet[2], packet[1]);
            break;

        case HCI_EVENT_ENCRYPTION_CHANGE:
            log_info("HCI_EVENT_ENCRYPTION_CHANGE= %d\n", packet[2]);
#if TCFG_BLE_SECURITY_EN
            client_search_profile_start();
#endif
            break;
        }
        break;
    }
}


static int get_buffer_vaild_len(void *priv)
{
    u32 vaild_len = 0;
    ble_user_cmd_prepare(BLE_CMD_ATT_VAILD_LEN, 1, &vaild_len);
    return vaild_len;
}

static int client_operation_send(u16 handle, u8 *data, u16 len, u8 att_op_type)
{
    int ret = APP_BLE_NO_ERROR;
    if (!con_handle) {
        return APP_BLE_OPERATION_ERROR;
    }

    if (!handle) {
        log_info("handle is null\n");
        return APP_BLE_OPERATION_ERROR;
    }


    if (get_buffer_vaild_len(0) < len) {
        log_info("opt_buff_full!!!\n");
        return APP_BLE_BUFF_FULL;
    }

    ret = ble_user_cmd_prepare(BLE_CMD_ATT_SEND_DATA, 4, handle, data, len, att_op_type);
    if (ret == BLE_BUFFER_FULL) {
        ret = APP_BLE_BUFF_FULL;
    }
    return ret;
}


//-----------------------------------------------
static int client_write_send(void *priv, u8 *data, u16 len)
{
    return client_operation_send(target_handle.write_handle, data, len, ATT_OP_WRITE);
}

static int client_write_without_respond_send(void *priv, u8 *data, u16 len)
{
    return client_operation_send(target_handle.write_no_respond, data, len, ATT_OP_WRITE_WITHOUT_RESPOND);
}

static int client_read_value_send(void *priv)
{
    u16 tmp_flag = 0x55A1;
    return client_operation_send(target_handle.read_handle, (u8 *)&tmp_flag, 2, ATT_OP_READ);
}

static int client_read_long_value_send(void *priv)
{
    u16 tmp_flag = 0x55A2;
    return client_operation_send(target_handle.read_handle, (u8 *)&tmp_flag, 2, ATT_OP_READ_LONG);
}

#if EXT_ADV_MODE_EN

struct __ext_scan_param {
    u8 Own_Address_Type;
    u8 Scanning_Filter_Policy;
    u8 Scanning_PHYs;
    u8  Scan_Type;
    u16 Scan_Interval;
    u16 Scan_Window;
} _GNU_PACKED_;

struct __ext_scan_enable {
    u8  Enable;
    u8  Filter_Duplicates;
    u16 Duration;
    u16 Period;
} _GNU_PACKED_;

const struct __ext_scan_param ext_scan_param = {
    .Own_Address_Type = 0,
    .Scanning_Filter_Policy = 0,
    .Scanning_PHYs = SCAN_SET_1M_PHY,
    .Scan_Type = SET_SCAN_TYPE,
    .Scan_Interval = SET_SCAN_INTERVAL,
    .Scan_Window = SET_SCAN_WINDOW,
};

const struct __ext_scan_enable ext_scan_enable = {
    .Enable = 1,
    .Filter_Duplicates = 0,
    .Duration = 0,
    .Period = 0,
};

const struct __ext_scan_enable ext_scan_disable = {
    .Enable = 0,
    .Filter_Duplicates = 0,
    .Duration = 0,
    .Period = 0,
};

#endif /* EXT_ADV_MODE_EN */

//扫描数设置
static void scanning_setup_init(void)
{
    ble_user_cmd_prepare(BLE_CMD_SCAN_PARAM, 3, SET_SCAN_TYPE, SET_SCAN_INTERVAL, SET_SCAN_WINDOW);
}

static int bt_ble_scan_enable(void *priv, u32 en)
{
    ble_state_e next_state, cur_state;

    if (!scan_ctrl_en) {
        return 	APP_BLE_OPERATION_ERROR;
    }

    if (en) {
        next_state = BLE_ST_SCAN;
    } else {
        next_state = BLE_ST_IDLE;
    }

    cur_state =  get_ble_work_state();
    switch (cur_state) {
    case BLE_ST_SCAN:
    case BLE_ST_IDLE:
    case BLE_ST_INIT_OK:
    case BLE_ST_NULL:
    case BLE_ST_DISCONN:
    case BLE_ST_CONNECT_FAIL:
    case BLE_ST_SEND_CREATE_CONN_CANNEL:
        break;
    default:
        return APP_BLE_OPERATION_ERROR;
        break;
    }

    if (cur_state == next_state) {
        return APP_BLE_NO_ERROR;
    }
    log_info("scan_en:%d\n", en);
    set_ble_work_state(next_state);

#if EXT_ADV_MODE_EN
    if (en) {
        ble_user_cmd_prepare(BLE_CMD_EXT_SCAN_PARAM, 2, &ext_scan_param, sizeof(ext_scan_param));
        ble_user_cmd_prepare(BLE_CMD_EXT_SCAN_ENABLE, 2, &ext_scan_enable, sizeof(ext_scan_enable));
    } else {
        ble_user_cmd_prepare(BLE_CMD_EXT_SCAN_ENABLE, 2, &ext_scan_disable, sizeof(ext_scan_disable));
    }
#else
    if (en) {
        scanning_setup_init();
    }
    ble_user_cmd_prepare(BLE_CMD_SCAN_ENABLE, 1, en);
#endif /* EXT_ADV_MODE_EN */

    return APP_BLE_NO_ERROR;
}

static int client_regiest_wakeup_send(void *priv, void *cbk)
{
    /* att_regist_wakeup_send(cbk); */
    return APP_BLE_NO_ERROR;
}

static int client_regiest_recieve_cbk(void *priv, void *cbk)
{
    channel_priv = (u32)priv;
    app_recieve_callback = cbk;
    return APP_BLE_NO_ERROR;
}

static int client_regiest_state_cbk(void *priv, void *cbk)
{
    channel_priv = (u32)priv;
    app_ble_state_callback = cbk;
    return APP_BLE_NO_ERROR;
}

//该接口重新配置搜索的配置项
static int client_init_config(void *priv, const client_conn_cfg_t *cfg)
{
    log_info("client_init_config\n");
	client_config = cfg;//reset config
	return APP_BLE_NO_ERROR;
}

static const struct ble_client_operation_t client_operation = {
    .scan_enable = bt_ble_scan_enable,
    .disconnect = client_disconnect,
    .get_buffer_vaild = get_buffer_vaild_len,
    .write_data = (void *)client_write_without_respond_send,
    .read_do = (void *)client_read_value_send,
    .regist_wakeup_send = client_regiest_wakeup_send,
    .regist_recieve_cbk = client_regiest_recieve_cbk,
	.regist_state_cbk = client_regiest_state_cbk,
	.init_config = client_init_config,
	.opt_comm_send = client_operation_send,
};

void ble_get_client_operation_table(struct ble_client_operation_t **interface_pt)
{
    *interface_pt = (void *)&client_operation;
}

bool ble_msg_deal(u32 param)
{
    struct ble_client_operation_t *test_opt;
    ble_get_client_operation_table(&test_opt);
    return FALSE;
}

#define PASSKEY_ENTER_ENABLE      0 //输入passkey使能，可修改passkey
//重设passkey回调函数，在这里可以重新设置passkey
//passkey为6个数字组成，十万位、万位。。。。个位 各表示一个数字 高位不够为0
static void reset_passkey_cb(u32 *key)
{
#if 1
    u32 newkey = rand32();//获取随机数

    newkey &= 0xfffff;
    if (newkey > 999999) {
        newkey = newkey - 999999; //不能大于999999
    }
    *key = newkey; //小于或等于六位数
    log_info("set new_key= %06u\n", *key);
#else
    *key = 123456; //for debug
#endif
}


extern void reset_PK_cb_register(void (*reset_pk)(u32 *));
void ble_sm_setup_init(io_capability_t io_type, u8 auth_req, uint8_t min_key_size, u8 security_en)
{
    //setup SM: Display only
    sm_init();
    sm_set_io_capabilities(io_type);
    sm_set_authentication_requirements(auth_req);
    sm_set_encryption_key_size_range(min_key_size, 16);
    sm_set_request_security(security_en);
    sm_event_callback_set(&cbk_sm_packet_handler);

    if (io_type == IO_CAPABILITY_DISPLAY_ONLY) {
        reset_PK_cb_register(reset_passkey_cb);
    }
}


extern void gatt_client_init(void);
extern void gatt_client_register_packet_handler(btstack_packet_handler_t handler);
extern void le_device_db_init(void);
extern void ble_stack_gatt_role(u8 role);//0--server,1--client
void ble_profile_init(void)
{
    log_info("ble profile init\n");
    le_device_db_init();
    ble_stack_gatt_role(1);

#if PASSKEY_ENTER_ENABLE
    ble_sm_setup_init(IO_CAPABILITY_DISPLAY_ONLY, SM_AUTHREQ_MITM_PROTECTION, 7, TCFG_BLE_SECURITY_EN);
#else
    ble_sm_setup_init(IO_CAPABILITY_NO_INPUT_NO_OUTPUT, SM_AUTHREQ_BONDING, 7, TCFG_BLE_SECURITY_EN);
#endif

    /* setup ATT client */
    gatt_client_init();
    gatt_client_register_packet_handler(cbk_packet_handler);

    // register for HCI events
    hci_event_callback_set(&cbk_packet_handler);
    le_l2cap_register_packet_handler(&cbk_packet_handler);
}

void ble_module_enable(u8 en)
{
    log_info("mode_en:%d\n", en);
    if (en) {
        scan_ctrl_en = 1;
        bt_ble_scan_enable(0, 1);
    } else {
        if (con_handle) {
            scan_ctrl_en = 0;
            client_disconnect(NULL);
        } else {
            bt_ble_scan_enable(0, 0);
            scan_ctrl_en = 0;
        }
    }

}

#if TEST_SEND_DATA_RATE
static u32 test_data[2];
static u32 test_send_data_count;
static void client_timer_send_data(void)
{
    static u16 timer_cnt = 0;
    if (con_handle &&  get_ble_work_state() == BLE_ST_SEARCH_COMPLETE) {
        u16 remain_len = get_buffer_vaild_len(0);
        if (remain_len) {
            test_data[0]++;
            client_write_without_respond_send(0, test_data, remain_len);
        }
        /* client_write_without_respond_send(0, test_data, 64); */

        if (++timer_cnt == 20) {
            log_info("\n%d bytes send: %d.%02d KB/s \n", test_send_data_count, test_send_data_count / 1000, test_send_data_count % 1000);
            test_send_data_count = 0;
            timer_cnt = 0;
        }
        test_send_data_count += remain_len;
    }
}
#endif

void bt_ble_init(void)
{
    log_info("***** ble_init******\n");
    set_ble_work_state(BLE_ST_INIT_OK);
    ble_module_enable(1);
#if TEST_SEND_DATA_RATE
    sys_timer_add(0, client_timer_send_data, 50);
#endif

#if SHOW_RX_DATA_RATE
    client_timer_start();
#endif /* SHOW_RX_DATA_RATE */
}

void bt_ble_exit(void)
{
    log_info("***** ble_exit******\n");
}

void bt_ble_adv_enable(u8 enable)
{
    ble_module_enable(enable);
}

void input_key_handler(u8 key_status, u8 key_number)
{
}

static u8 client_idle_query(void)
{
    return 0;
}

REGISTER_LP_TARGET(client_user_target) = {
    .name = "client_user_demo",
    .is_idle = client_idle_query,
};


//----------------------------------------------------------------------------------



#endif


