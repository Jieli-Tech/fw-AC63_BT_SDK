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
#include "le_common.h"

#include "ble_user.h"
#include "le_client_demo.h"
#include "ble_at_client.h"

#if CONFIG_APP_AT_COM && TRANS_AT_CLIENT

#if LE_DEBUG_PRINT_EN
/* #define log_info            printf */
#define log_info(x, ...)    printf("[LE_AT_CLI]" x " ", ## __VA_ARGS__)
#define log_info_hexdump    put_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

//------
#define ATT_LOCAL_MTU_SIZE        (64*2)                  //note: need >= 20
#define ATT_SEND_CBUF_SIZE        (512)                   //note: need >= 20,缓存大小，可修改
#define ATT_RAM_BUFSIZE           (ATT_CTRL_BLOCK_SIZE + ATT_LOCAL_MTU_SIZE + ATT_SEND_CBUF_SIZE)                   //note:
static u8 att_ram_buffer[ATT_RAM_BUFSIZE] __attribute__((aligned(4)));

#define SEARCH_PROFILE_BUFSIZE    (512)                   //note:
static u8 search_ram_buffer[SEARCH_PROFILE_BUFSIZE] __attribute__((aligned(4)));
#define scan_buffer search_ram_buffer
//---------------
//搜索类型
#define SET_SCAN_TYPE       SCAN_ACTIVE
//搜索 周期大小
#define SET_SCAN_INTERVAL   40 //(unit:0.625ms)
//搜索 窗口大小
#define SET_SCAN_WINDOW     8 //(unit:0.625ms)

//连接周期
#define SET_CONN_INTERVAL   24 //(unit:1.25ms)
//连接latency
#define SET_CONN_LATENCY    0  //(unit:conn_interval)
//连接超时
#define SET_CONN_TIMEOUT    400 //(unit:10ms)
//----------------------------------------------------------------------------
static u8 scan_ctrl_en;
static u8 ble_work_state = 0;
static void (*app_recieve_callback)(void *priv, void *buf, u16 len) = NULL;
static void (*app_ble_state_callback)(void *priv, ble_state_e state) = NULL;
static void (*ble_resume_send_wakeup)(void) = NULL;
static u32 channel_priv;

//搜索 周期大小,(unit:0.625ms)
static u16 scan_interval_value =  SET_SCAN_INTERVAL;
//搜索 窗口大小,(unit:0.625ms)
static u16 scan_window_value =  SET_SCAN_WINDOW;

static hci_con_handle_t con_handle;

#define BLE_VM_HEAD_TAG           (0xB95C)
#define BLE_VM_TAIL_TAG           (0x5CB9)
struct pair_info_t {
    u16 head_tag;
    u8  pair_flag;
    u8  peer_address_info[7];
    u16 tail_tag;
};
static struct pair_info_t  conn_pair_info;
static u8 pair_bond_enable = 0;

//-------------------------------------------------------------------------------
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

static  client_conn_cfg_t *client_config = NULL ;

//----------------------------------------------------------------------------
static void bt_ble_create_connection(u8 *conn_addr, u8 addr_type);
static int client_write_send(void *priv, u8 *data, u16 len);
static int client_operation_send(u16 handle, u8 *data, u16 len, u8 att_op_type);


static void at_send_event_ble_connected()
{
    //at_send_event参数: 宏, 对应命令的数据包,数据包长度
    at_send_event(AT_EVT_BLE_CONNECTED, NULL, 0);
}

static void at_send_event_ble_disconnected()
{
    at_send_event(AT_EVT_BLE_DISCONNECTED, NULL, 0);
}

static u8 buf_ble_receive[256]__attribute__((aligned(4)));
static void at_send_event_ble_data_receive(u16 handle, u8 *dat, u8 size)
{
    if (size > 64) {
        log_info("EVT payload overflow");
        return;
    }

    buf_ble_receive[0] = handle;  //先低位,再高位
    buf_ble_receive[1] = handle >> 8;

    memcpy(buf_ble_receive + 2, dat, size);

    at_send_event(AT_EVT_BLE_DATA_RECEIVED, buf_ble_receive, 2 + size);
}

static void at_send_event_ble_conn_param_update_complete(u16 interval, u16 latency, u16 time_out)
{
    u16 conn_param_buf[3];
    conn_param_buf[0] = interval;   //先低位,再高位
    conn_param_buf[1] = latency;
    conn_param_buf[2] = time_out;
    at_send_event(AT_EVT_CONN_PARAM_UPDATE_COMPLETE, conn_param_buf, 6);
}

static void at_send_event_ble_adv_report(adv_report_t *report_pt)
{
    u8 *adv_buf = buf_ble_receive;
    adv_buf[0] = report_pt->event_type;
    adv_buf[1] = report_pt->address_type;

    memcpy(adv_buf + 2, report_pt->address, 6);
    memcpy(adv_buf + 8, report_pt->data, report_pt->length);
    at_send_event(AT_EVT_ADV_REPORT, adv_buf, report_pt->length + 8);
}


typedef struct { //profile 信息结构
    u16 services_uuid16 ; //非 0：16bit uuid，为 0：128bit uuid
    u8 services_uuid128[16]; // 128 bit uuid
    u16 characteristic_uuid16; //非 0：16bit uuid，为 0：128bit uuid
    u8 characteristic_uuid128[16]; // 128 bit uuid
    u16 value_handle; //操作 handle
    u8 properties; //属性 bits
} handle_info_t;

static handle_info_t *handle_info = NULL;
static void at_send_event_ble_profile_report(search_result_t const *result_info)
{
    // 为保证机构体字节对齐, 舍弃前3个字节
    u8 *profile_buf = buf_ble_receive;
    handle_info = profile_buf + 4;
    int i = 0;

    if (result_info->services.uuid16) {
        profile_buf[3] = 1;
        handle_info->services_uuid16 = result_info->services.uuid16;
        for (i = 0; i < 16; i++) {
            handle_info->services_uuid128[i] = 0;
        }
        handle_info->characteristic_uuid16 = result_info->characteristic.uuid16;
        for (i = 0; i < 16; i++) {
            handle_info->characteristic_uuid128[i] = 0;
        }
        handle_info->value_handle = result_info->characteristic.value_handle;

        handle_info->properties = result_info->characteristic.properties;
    }

    else {
        profile_buf[3] = 2;

        handle_info->services_uuid16 = 0;

        memcpy(handle_info->services_uuid128, result_info->services.uuid128, 16);

        handle_info->characteristic_uuid16 = 0;

        memcpy(handle_info->characteristic_uuid128, result_info->characteristic.uuid128, 16);

        handle_info->value_handle = result_info->characteristic.value_handle;

        handle_info->properties = result_info->characteristic.properties;

    }
    at_send_event(AT_EVT_PROFILE_REPOFT, profile_buf + 3, 1 + sizeof(handle_info_t));
}

static void at_send_event_ble_profile_search_end()
{
    at_send_event(AT_EVT_PROFILE_SEARCH_END, 0, 0);
}


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
        ble_op_conn_param_update(con_handle, param);
    }
}

/* static void test_send_conn_update(void) */
/* { */
/* client_send_conn_param_update(); */
/* send_param_index++; */
/* send_param_index &= 3; */
/* sys_timeout_add(0,test_send_conn_update,10000); */
/* } */

static void client_event_report(le_client_event_e event, u8 *packet, int size)
{
    if (client_config->event_callback) {
        client_config->event_callback(event, packet, size);
    }
}

static bool check_device_is_match(u8 info_type, u8 *data, int size)
{
    return false;
}

static void conn_pair_vm_do(struct pair_info_t *info, u8 rw_flag)
{
    /* return; */

    int ret;
    int vm_len = sizeof(struct pair_info_t);

    log_info("-conn_pair_info vm_do:%d\n", rw_flag);
    if (rw_flag == 0) {
        ret = syscfg_read(CFG_BLE_MODE_INFO, (u8 *)info, vm_len);
        if (!ret) {
            log_info("-null--\n");
        }
        if ((BLE_VM_HEAD_TAG == info->head_tag) && (BLE_VM_TAIL_TAG == info->tail_tag)) {
            log_info("-exist--\n");
            log_info_hexdump((u8 *)info, vm_len);
        } else {
            memset(info, 0, vm_len);
            info->head_tag = BLE_VM_HEAD_TAG;
            info->tail_tag = BLE_VM_TAIL_TAG;
        }
    } else {
        syscfg_write(CFG_BLE_MODE_INFO, (u8 *)info, vm_len);
    }
}

void client_clear_bonding_info(void)
{
    log_info("client_clear_bonding_info\n");
    conn_pair_vm_do(&conn_pair_info, 0);
    if (conn_pair_info.pair_flag) {
        //del pair bond
        memset(&conn_pair_info, 0, sizeof(struct pair_info_t));
        conn_pair_vm_do(&conn_pair_info, 1);
    }
}

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
    return;
}

//操作handle，完成 write ccc
static void do_operate_search_handle(void)
{
    set_ble_work_state(BLE_ST_SEARCH_COMPLETE);
}

//return: 0--accept,1--reject
int l2cap_connection_update_request_just(u8 *packet, hci_con_handle_t handle)
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
        at_send_event_ble_profile_search_end();
        client_event_report(CLI_EVENT_SEARCH_PROFILE_COMPLETE, 0, 0);
        return;
    }

    log_info("\n*** services, uuid16:%04x,index=%d ***\n", result_info->services.uuid16, result_info->service_index);
    log_info("{charactc, uuid16:%04x,index=%d,handle:%04x~%04x,value_handle=%04x}\n",
             result_info->characteristic.uuid16, result_info->characteristic_index,
             result_info->characteristic.start_handle, result_info->characteristic.end_handle,
             result_info->characteristic.value_handle
            );

    at_send_event_ble_profile_report(result_info);

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


/* static target_uuid_t *get_match_handle_target(u16 handle) */
/* { */
/* for (int i = 0; i < opt_handle_used_cnt; i++) { */
/* if (opt_handle_table[i].value_handle == handle) { */
/* return opt_handle_table[i].search_uuid; */
/* } */
/* } */
/* return NULL; */
/* } */

void user_client_report_data_callback(att_data_report_t *report_data)
{
    /* log_info("\n-report_data:type %02x,handle %04x,offset %d,len %d:",report_data->packet_type, */
    /* 		report_data->value_handle,report_data->value_offset,report_data->blob_length); */
    /* log_info_hexdump(report_data->blob,report_data->blob_length); */

    /* target_uuid_t *search_uuid = get_match_handle_target(report_data->value_handle); */

    /* if(client_config->report_data_callback){ */
    /* client_config->report_data_callback(report_data,search_uuid); */
    /* return; */
    /* } */

    at_send_event_ble_data_receive(report_data->value_handle, report_data->blob, report_data->blob_length);

    switch (report_data->packet_type) {
    case GATT_EVENT_NOTIFICATION://notify
        log_info("\n-notify_rx(%d):  %c    %c  \n ", report_data->blob_length, report_data->blob[0], report_data->blob[1]);
        break;
    case GATT_EVENT_INDICATION://indicate
        log_info("\n-indicate_rx(%d):  %c    %c  \n ", report_data->blob_length, report_data->blob[0], report_data->blob[1]);
        break;
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
}

//------------------------------------------------------------
static bool resolve_adv_report(u8 *adv_address, u8 data_length, u8 *data)
{
    u8 i, lenght, ad_type;
    u8 *adv_data_pt;
    u8 find_remoter = 0;
    /* u8 tmp_addr[6]; */
    u32 tmp32;

    if (check_device_is_match(CLI_CREAT_BY_ADDRESS, adv_address, 6)) {
        find_remoter = 1;
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
            /* log_info("target name:%s", client_config->compare_data); */
            if (check_device_is_match(CLI_CREAT_BY_NAME, adv_data_pt, lenght - 1)) {
                find_remoter = 1;
                log_info("catch name ok\n");
            }
            break;

        case HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA:
            if (check_device_is_match(CLI_CREAT_BY_TAG, adv_data_pt, lenght - 1)) {
                log_info("get_tag_string!\n");
                find_remoter = 1;
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

    /* log_info("event_type,addr_type: %x,%x; ",report_pt->event_type,report_pt->address_type); */
    /* log_info_hexdump(report_pt->address,6); */

    /* log_info("adv_data_display:"); */
    /* log_info_hexdump(report_pt->data,report_pt->length); */

    log_info("rssi:%d\n", report_pt->rssi);
    resolve_adv_report(report_pt->address, report_pt->length, report_pt->data);

    at_send_event_ble_adv_report(report_pt);
}


static void bt_ble_create_connection(u8 *conn_addr, u8 addr_type)
{
    client_create_connection(conn_addr, addr_type);
}

static void client_create_connection_cannel(void)
{
    set_ble_work_state(BLE_ST_SEND_CREATE_CONN_CANNEL);
    ble_op_create_connection_cancel();
}

static int client_disconnect(void *priv)
{
    if (con_handle) {
        if (BLE_ST_SEND_DISCONN != get_ble_work_state()) {
            log_info(">>>ble send disconnect\n");
            set_ble_work_state(BLE_ST_SEND_DISCONN);
            ble_op_disconnect(con_handle);
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

    at_send_event_ble_conn_param_update_complete(conn_interval, conn_latency, conn_timeout);

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
        ble_op_set_data_length(con_handle, tx_octets, tx_time);
    }
}

static void set_connection_data_phy(u8 tx_phy, u8 rx_phy)
{
    if (0 == con_handle) {
        return;
    }

    u8 all_phys = 0;
    u16 phy_options = 0;

    ble_op_set_ext_phy(con_handle, all_phys, tx_phy, rx_phy, phy_options);
}

static void client_profile_start(u16 con_handle)
{
    ble_op_att_send_init(con_handle, att_ram_buffer, ATT_RAM_BUFSIZE, ATT_LOCAL_MTU_SIZE);
    set_ble_work_state(BLE_ST_CONNECT);

    if (0 == client_config->security_en) {
        client_search_profile_start();
    }
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

                at_send_event_ble_connected();

//                connection_update_complete_success(packet + 8);
                client_profile_start(con_handle);
                client_event_report(CLI_EVENT_CONNECTED, packet, size);

                if (pair_bond_enable) {
                    conn_pair_info.pair_flag = 1;
                    memcpy(&conn_pair_info.peer_address_info, &packet[7], 7);
                    conn_pair_vm_do(&conn_pair_info, 1);
                    pair_bond_enable = 0;
                }
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
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            log_info("HCI_EVENT_DISCONNECTION_COMPLETE: %0x\n", packet[5]);

            at_send_event_ble_disconnected();

            con_handle = 0;
            ble_op_att_send_init(con_handle, 0, 0, 0);
            set_ble_work_state(BLE_ST_DISCONN);
            client_event_report(CLI_EVENT_DISCONNECT, packet, size);
            //bt_ble_scan_enable(0, 1);
            break;

        case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
            mtu = att_event_mtu_exchange_complete_get_MTU(packet);
            log_info("ATT MTU = %u\n", mtu);
            ble_op_att_set_send_mtu(mtu - 3);
            break;

        case HCI_EVENT_VENDOR_REMOTE_TEST:
            log_info("--- HCI_EVENT_VENDOR_REMOTE_TEST\n");
            break;

        case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE:
            tmp = little_endian_read_16(packet, 4);
            log_info("-update_rsp: %02x\n", tmp);
            break;

        case GAP_EVENT_ADVERTISING_REPORT:
            /* putchar('@'); */
            client_report_adv_data((void *)&packet[2], packet[1]);
            break;

        case HCI_EVENT_ENCRYPTION_CHANGE:
            log_info("HCI_EVENT_ENCRYPTION_CHANGE= %d\n", packet[2]);
            if (client_config->security_en) {
                client_search_profile_start();
            }
            break;
        }
        break;
    }
}


static int get_buffer_vaild_len(void *priv)
{
    u32 vaild_len = 0;
    ble_op_att_get_remain(&vaild_len);
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

    ret = ble_op_att_send_data(handle, data, len, att_op_type);
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


//扫描数设置
static void scanning_setup_init(void)
{
    ble_op_set_scan_param(SET_SCAN_TYPE, scan_interval_value, scan_window_value);
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

struct ble_client_operation_t *ble_get_client_operation_table(void)
{
    return &client_operation;
}

bool ble_msg_deal(u32 param)
{
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


extern void sm_set_master_request_pair(int enable);
void ble_sm_setup_init(io_capability_t io_type, u8 auth_req, uint8_t min_key_size, u8 security_en)
{
    //setup SM: Display only
    sm_init();
    sm_set_io_capabilities(io_type);
    sm_set_authentication_requirements(auth_req);
    sm_set_encryption_key_size_range(min_key_size, 16);
    sm_set_master_request_pair(security_en);
    sm_event_callback_set(&cbk_sm_packet_handler);

    if (io_type == IO_CAPABILITY_DISPLAY_ONLY) {
        reset_PK_cb_register(reset_passkey_cb);
    }
}


#define AT_CLIENT_TCFG_BLE_SECURITY_REQUEST        0
void ble_profile_init(void)
{
    log_info("ble profile init\n");
    ble_stack_gatt_role(1);

    if (config_le_sm_support_enable) {
        le_device_db_init();
#if PASSKEY_ENTER_ENABLE
        ble_sm_setup_init(IO_CAPABILITY_DISPLAY_ONLY, SM_AUTHREQ_MITM_PROTECTION | SM_AUTHREQ_BONDING, 7, AT_CLIENT_TCFG_BLE_SECURITY_REQUEST);
#else
        ble_sm_setup_init(IO_CAPABILITY_NO_INPUT_NO_OUTPUT, SM_AUTHREQ_MITM_PROTECTION | SM_AUTHREQ_BONDING, 7, AT_CLIENT_TCFG_BLE_SECURITY_REQUEST);
#endif
    }

    /* setup ATT client */
    gatt_client_init();
    gatt_client_register_packet_handler(cbk_packet_handler);

    // register for HCI events
    hci_event_callback_set(&cbk_packet_handler);
    le_l2cap_register_packet_handler(&cbk_packet_handler);

    ble_vendor_set_default_att_mtu(ATT_LOCAL_MTU_SIZE);
}

void ble_module_enable(u8 en)
{
    log_info("mode_en:%d\n", en);
    if (en) {
        scan_ctrl_en = 1;
//        bt_ble_scan_enable(0, 1);
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


void bt_ble_init(void)
{
    log_info("***** ble_init******\n");
    log_info("ble_file: %s", __FILE__);
    set_ble_work_state(BLE_ST_INIT_OK);
    conn_pair_vm_do(&conn_pair_info, 0);
    ble_module_enable(1);

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
    return 1;
}

REGISTER_LP_TARGET(client_user_target) = {
    .name = "client_user_demo",
    .is_idle = client_idle_query,
};


//---------------------------------AT命令解析用到-------


/***
扫描参数配置
***/
void ble_at_scan_param(u16 scan_interval, u16 scan_window)
{
    scan_interval_value = scan_interval;
    scan_window_value = scan_window;
}

/***
连接参数更新
***/
static struct conn_update_param_t param;

void client_connect_param_update(u16 interval_min, u16 interval_max, u16 latency, u16 timeout)
{
    param.interval_min = interval_min;
    param.interval_max = interval_max;
    param.latency = latency;
    param.timeout = timeout;

    log_info("client update param:-%d-%d-%d-%d-\n", param.interval_min, param.interval_max, param.latency, param.timeout);
    if (con_handle) {
        ble_op_conn_param_update(con_handle, &param);
    }
}


/**
设置BLE地址,
*/
int ble_at_set_address(u8 *addr)
{
    le_controller_set_mac(addr);
    return APP_BLE_NO_ERROR;
}


/**
设置BLE名称
**/
static char gap_device_name[BT_NAME_LEN_MAX] = "br22_ble_test";
static u8 gap_device_name_len = 0;
int ble_at_set_name(u8 *name, u8 len)
{
    if (len > BT_NAME_LEN_MAX - 1) {
        len = BT_NAME_LEN_MAX - 1;
    }
    memcpy(gap_device_name, name, len);
    gap_device_name_len = len;
    return APP_BLE_NO_ERROR;
}


/**
获取BLE状态
*/
u8 ble_at_get_staus(void)
{
    u8 staus = 0;

    if (con_handle) {
        staus |= BIT(ST_BIT_BLE_CONN);
    }

    if (BLE_ST_ADV == get_ble_work_state()) {
        staus |= BIT(ST_BIT_BLE_ADV);
    }

    return staus;
}


static int ble_disconnect(void *priv)
{
    if (con_handle) {
        if (BLE_ST_SEND_DISCONN != get_ble_work_state()) {
            log_info(">>>ble send disconnect\n");
            set_ble_work_state(BLE_ST_SEND_DISCONN);
            ble_op_disconnect(con_handle);
        } else {
            log_info(">>>ble wait disconnect...\n");
        }
        return APP_BLE_NO_ERROR;
    } else {
        return APP_BLE_OPERATION_ERROR;
    }
}

/**
断开BLE连接
*/
int ble_at_disconnect(void)
{
    return ble_disconnect(0);
}



int ble_at_get_address(u8 *addr)
{
    le_controller_get_mac(addr);
    return APP_BLE_NO_ERROR;
}


int ble_at_get_name(u8 *name)
{
    memcpy(name, gap_device_name, gap_device_name_len);
    return gap_device_name_len;
}


int bt_ble_scan_enable(void *priv, u32 en)
{
    ble_state_e next_state, cur_state;

    if (!scan_ctrl_en && en) {
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

    if (en) {
        scanning_setup_init();
    }
    ble_op_scan_enable(en);
    return APP_BLE_NO_ERROR;
}

void client_create_connection(u8 *conn_addr, u8 addr_type)
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
    ble_op_create_connection(create_conn_par);
}


void client_create_connection_cancel(void)
{
    set_ble_work_state(BLE_ST_SEND_CREATE_CONN_CANNEL);
    ble_op_create_connection_cancel();
}

void client_search_profile(u8 search_type, u8 *UUID)
{
    u16 UUID16 = UUID[1] * 0X100 + UUID[0];

    switch (search_type) {

    case 1:
        ble_op_search_profile_uuid16(UUID16);
        break;
    case 2:
        ble_op_search_profile_uuid128(UUID);
        break;
    case 3:
        ble_op_search_profile_all();
        break;
    default:
        log_info("Search_type_error");
    }
}


void client_receive_ccc(u16 value_handle, u8 ccc_type)
{
    u16 tmp_16;
    switch (ccc_type) {
    case 0:
        tmp_16  = 0x00;//fixed
        log_info("close_ntf_ccc:%04x\n", value_handle);
        client_operation_send(value_handle + 1, &tmp_16, 2, ATT_OP_WRITE);      // 0是关
        break;

    case 1:
        tmp_16  = 0x01;//fixed
        log_info("write_ntf_ccc:%04x\n", value_handle);
        client_operation_send(value_handle + 1, &tmp_16, 2, ATT_OP_WRITE);
        break;

    case 2:
        tmp_16  = 0x02;//fixed
        log_info("write_ind_ccc:%04x\n", value_handle);
        client_operation_send(value_handle + 1, &tmp_16, 2, ATT_OP_WRITE);
        break;
    }
    set_ble_work_state(BLE_ST_SEARCH_COMPLETE);
}


int client_read_long_value(u8 *read_long_handle)
{
    u16 tmp_flag = 0x55A2;
    return client_operation_send(*read_long_handle, (u8 *)&tmp_flag, 2, ATT_OP_READ_LONG);
}



int client_write(u16 write_handle, u8 *data, u8 len)
{
    return client_operation_send(write_handle, data, len, ATT_OP_WRITE);
}


int client_write_without_respond(u16 write_no_respond_handle, u8 *data, u16 len)
{
    return client_operation_send(write_no_respond_handle, data, len, ATT_OP_WRITE_WITHOUT_RESPOND);
}


//----------------------------------------------------------------------------------



#endif



