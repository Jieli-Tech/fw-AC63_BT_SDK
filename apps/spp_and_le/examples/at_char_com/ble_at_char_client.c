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
#include "ble_at_char_client.h"

#if  CONFIG_APP_AT_CHAR_COM && CONFIG_BT_GATT_CLIENT_NUM

#if LE_DEBUG_PRINT_EN
//#define log_info            printf
#define log_info(x, ...)    printf("[LE_AT_CHAR_CLI]" x " ", ## __VA_ARGS__)

#define log_info_hexdump    put_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

#define JL_AT_TEST_MODE            1//连接杰理默认的透传从机

#define SEARCH_PROFILE_BUFSIZE    (512)                   //note:
static u8 search_ram_buffer[SEARCH_PROFILE_BUFSIZE] __attribute__((aligned(4)));
#define scan_buffer search_ram_buffer
//---------------
#define   SUPPORT_MAX_DEV        (CONFIG_BT_GATT_CLIENT_NUM) //最大支持的设备
#define   HANDLE_INVAIL_INDEX    ((s8)-1)

//搜索类型
#define SET_SCAN_TYPE           SCAN_ACTIVE
//搜索 周期大小
#define SET_SCAN_INTERVAL       48 //(unit:0.625ms)
//搜索 窗口大小
#define SET_SCAN_WINDOW         8 //(unit:0.625ms)

//连接周期
#define SET_CONN_INTERVAL_MIN   24 //(unit:1.25ms)
#define SET_CONN_INTERVAL_MAX   48 //(unit:1.25ms)
//连接latency
#define SET_CONN_LATENCY        0  //(unit:conn_interval)
//连接超时
#define SET_CONN_TIMEOUT        400 //(unit:10ms)
//----------------------------------------------------------------------------
static u8 scan_ctrl_en;
static u8 ble_work_state = 0;
static void (*app_recieve_callback)(void *priv, void *buf, u16 len) = NULL;
static void (*app_ble_state_callback)(void *priv, ble_state_e state) = NULL;
static void (*ble_resume_send_wakeup)(void) = NULL;
static u32 channel_priv;
static u32 auto_scan_enable = 0;

//搜索 周期大小,(unit:0.625ms)
static u16 scan_interval_value =  SET_SCAN_INTERVAL;
//搜索 窗口大小,(unit:0.625ms)
static u16 scan_window_value =  SET_SCAN_WINDOW;

static hci_con_handle_t con_handle[SUPPORT_MAX_DEV];
static s8 cur_dev_cid = 0;

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

struct conn_update_param_t client_conn_param = {
    .interval_min = SET_CONN_INTERVAL_MIN,  //(unit:0.625ms)
    .interval_max = SET_CONN_INTERVAL_MAX,  //(unit:0.625ms)
    .latency = SET_CONN_LATENCY,       //(unit: interval)
    .timeout = SET_CONN_TIMEOUT,      //(unit:10ms)
};

//-------------------------------------------------------------------------------
typedef struct {
    uint16_t write_type;
    //ATT_PROPERTY_WRITE_WITHOUT_RESPONSE or ATT_PROPERTY_WRITE
    uint16_t write_handle;
    uint16_t write_no_respond;
    uint16_t notify_handle;
    uint16_t indicate_handle;
    uint16_t read_handle;
    uint16_t read_long_handle;
} target_hdl_t;

typedef struct {
    //可操作的handle
    u16 value_handle;
    u16 opt_type; //属性
} opt_handle_record_t;

static const client_conn_cfg_t at_client_config_default = {
    .report_data_callback = NULL,
    .search_uuid_cnt = 0,
    .security_en = 0,
    .event_callback = NULL,
};



//记录handle 使用
static u16 search_target_uuid16 = 0xAE30;
static target_hdl_t target_handle[SUPPORT_MAX_DEV];
static opt_handle_record_t opt_handle_table[OPT_HANDLE_MAX];
static u8 opt_handle_used_cnt;
static  client_conn_cfg_t *client_config = &at_client_config_default;

//----------------------------------------------------------------------------
static int bt_ble_create_connection(u8 *conn_addr, u8 addr_type);
static int client_write_send(void *priv, u8 *data, u16 len);
static int client_operation_send(u16 conn_handle, u16 handle, u8 *data, u16 len, u8 att_op_type);
static void ble_client_module_enable(u8 en);
//----------------------------------------------------------------------------
static s8 get_dev_index(u16 handle)
{
    s8 i;
    for (i = 0; i < SUPPORT_MAX_DEV; i++) {
        if (handle == con_handle[i]) {
            return i;
        }
    }
    return HANDLE_INVAIL_INDEX;
}

static s8 get_idle_dev_index(void)
{
    s8 i;
    for (i = 0; i < SUPPORT_MAX_DEV; i++) {
        if (0 == con_handle[i]) {
            return i;
        }
    }
    return HANDLE_INVAIL_INDEX;
}


static s8 del_dev_index(u16 handle)
{
    s8 i;
    for (i = 0; i < SUPPORT_MAX_DEV; i++) {
        if (handle == con_handle[i]) {
            con_handle[i] = 0;
            return i;
        }
    }
    return HANDLE_INVAIL_INDEX;
}

static bool dev_have_connected(void)
{
    s8 i;
    for (i = 0; i < SUPPORT_MAX_DEV; i++) {
        if (con_handle[i]) {
            return true;
        }
    }
    return false;
}
//----------------------------------------------------------------------------


static const struct conn_update_param_t connection_param_table[] = {
    {16, 24, 0, 500},//11
    {12, 28, 0, 500},//3.7
    {8,  20, 0, 500},
    {50, 60, 0, 500},
};

static u8 send_param_index = 3;
void client_send_conn_param_update(void)
{
    struct conn_update_param_t *param = (void *)&connection_param_table[send_param_index];//for test
    log_info("client update param:-%d-%d-%d-%d-\n", param->interval_min, param->interval_max, param->latency, param->timeout);
    if (con_handle[cur_dev_cid]) {
        ble_op_conn_param_update(con_handle[cur_dev_cid], param);
    }
}


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
    u8 opt_type = result_info->characteristic.properties;

#if JL_AT_TEST_MODE
    if (result_info->characteristic.uuid16 != 0xae01
        && result_info->characteristic.uuid16 != 0xae02
        && result_info->characteristic.uuid16 != 0xae05) {
        log_info("======drop not jieli uuid\n");
        return;
    }
#endif

    if (opt_type & ATT_PROPERTY_WRITE_WITHOUT_RESPONSE) {
        target_handle[cur_dev_cid].write_no_respond = result_info->characteristic.value_handle;
        target_handle[cur_dev_cid].write_type = ATT_PROPERTY_WRITE_WITHOUT_RESPONSE;
    }

    if (opt_type & ATT_PROPERTY_WRITE) {
        target_handle[cur_dev_cid].write_handle = result_info->characteristic.value_handle;
        target_handle[cur_dev_cid].write_type = ATT_PROPERTY_WRITE;
    }

    if (opt_type & ATT_PROPERTY_NOTIFY) {
        target_handle[cur_dev_cid].notify_handle  = result_info->characteristic.value_handle;
        opt_handle_record_t *opt_get = &opt_handle_table[opt_handle_used_cnt++];
        opt_get->value_handle = result_info->characteristic.value_handle;
        opt_get->opt_type = opt_type;

    } else if (opt_type & ATT_PROPERTY_INDICATE) {
        target_handle[cur_dev_cid].indicate_handle  = result_info->characteristic.value_handle;
        opt_handle_record_t *opt_get = &opt_handle_table[opt_handle_used_cnt++];
        opt_get->value_handle = result_info->characteristic.value_handle;
        opt_get->opt_type = opt_type;
    }

    return;
}

//操作handle，完成 write ccc
static void do_operate_search_handle(void)
{
    u16 tmp_16;
    u16 i, cur_opt_type;
    opt_handle_record_t *opt_hdl_pt;

    log_info("opt_handle_used_cnt= %d\n", opt_handle_used_cnt);

    log_info("find target_handle:");
    log_info_hexdump(&target_handle, sizeof(target_hdl_t));

    if (0 == opt_handle_used_cnt) {
        goto opt_end;
    }

    for (i = 0; i < opt_handle_used_cnt; i++) {
        opt_hdl_pt = &opt_handle_table[i];
        cur_opt_type = opt_hdl_pt->opt_type;
        if ((cur_opt_type & ATT_PROPERTY_NOTIFY) == ATT_PROPERTY_NOTIFY) {
            tmp_16  = 0x01;//fixed
            log_info("write_ntf_ccc:%04x\n", opt_hdl_pt->value_handle);
            client_operation_send(con_handle[cur_dev_cid], opt_hdl_pt->value_handle + 1, &tmp_16, 2, ATT_OP_WRITE);
        } else if ((cur_opt_type & ATT_PROPERTY_INDICATE) == ATT_PROPERTY_INDICATE) {
            tmp_16  = 0x02;//fixed
            log_info("write_ind_ccc:%04x\n", opt_hdl_pt->value_handle);
            client_operation_send(con_handle[cur_dev_cid], opt_hdl_pt->value_handle + 1, &tmp_16, 2, ATT_OP_WRITE);
        }
    }

opt_end:
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
        client_event_report(CLI_EVENT_SEARCH_PROFILE_COMPLETE, 0, 0);
        at_send_string("OK");
        at_send_connected(cur_dev_cid);
        set_ble_work_state(BLE_ST_IDLE);
        return;
    }

    log_info("*** services, uuid16:%04x,index=%d ***\n", result_info->services.uuid16, result_info->service_index);
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

//    at_send_event_ble_data_receive(report_data->value_handle, report_data->blob, report_data->blob_length);

    s8 tmp_cid = get_dev_index(report_data->conn_handle);
    if (tmp_cid == HANDLE_INVAIL_INDEX) {
        log_info("report err handle: %04x\n", report_data->conn_handle);
        return;
    }

    switch (report_data->packet_type) {
    case GATT_EVENT_NOTIFICATION://notify
        log_info("-notify_rx(%d):  %c    %c  \n ", report_data->blob_length, report_data->blob[0], report_data->blob[1]);
        at_send_rx_cid_data(tmp_cid, report_data->blob, report_data->blob_length);
        /* le_att_client_send_data(tmp_cid,report_data->blob,4); */
        break;
    case GATT_EVENT_INDICATION://indicate
        log_info("-indicate_rx(%d):  %c    %c  \n ", report_data->blob_length, report_data->blob[0], report_data->blob[1]);
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
    memset(&target_handle[cur_dev_cid], 0, sizeof(target_hdl_t));
    user_client_init(con_handle[cur_dev_cid], search_ram_buffer, SEARCH_PROFILE_BUFSIZE);
    if (!search_target_uuid16) {
        log_info("search all server\n");
        ble_op_search_profile_all();
    } else {
        log_info("search uuid:%04x\n", search_target_uuid16);
        ble_op_search_profile_uuid16(search_target_uuid16);
    }
}

//------------------------------------------------------------
extern u32 hex_2_str(u8 *hex, u32 hex_len, u8 *str);
extern u32 str_2_hex(u8 *str, u32 str_len, u8 *hex);

static u8 at_send_adv_buf[128] = {0};
static bool resolve_adv_report(u8 *adv_address, u8 data_length, u8 *data, s8 rssi, u8 addr_type)
{
    u8 i, lenght, ad_type;
    u8 *adv_data_pt;
    u8 find_remoter = 0;
    /* u8 tmp_addr[6]; */
    u32 tmp32;

    u8 ret = 0;

    adv_data_pt = data;



    /* log_info("----->\n"); */
    /* put_buf(data,data_length); */

    //-----------------new
    //memset(&name_buf[5], 0, 35);
    ret = hex_2_str(adv_address, 6, at_send_adv_buf); //dev_addr
    at_send_adv_buf[ret++] = ',';
    at_send_adv_buf[ret++] = addr_type + '0';     //addr_type
    at_send_adv_buf[ret++] = ',';
    sprintf(&at_send_adv_buf[ret], "%d", rssi);  //rssi
    ret = strlen(at_send_adv_buf);
    at_cmd_send_no_end(at_send_adv_buf, ret);
    //--------------------

    for (i = 0; i < data_length;) {
        if (*adv_data_pt == 0) {
            /* log_info("analyze end\n"); */
            break;
        }

        lenght = *adv_data_pt++;
        if (lenght >= data_length || (lenght + i) >= data_length) {
            /*过滤非标准包格式*/
            printf("!!!error_adv_packet:");
            put_buf(data, data_length);
            break;
        }

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

            //-----------------new
            sprintf(at_send_adv_buf, "UUID:");
            ret = hex_2_str(adv_data_pt, lenght - 1, &at_send_adv_buf[5]);
            at_cmd_send_no_end(at_send_adv_buf, ret + 5);
            //--------------------

            /* log_info("service uuid:"); */
            /* log_info_hexdump(adv_data_pt, lenght - 1); */
            break;

        case HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME:
        case HCI_EIR_DATATYPE_SHORTENED_LOCAL_NAME:
            tmp32 = adv_data_pt[lenght - 1];
            adv_data_pt[lenght - 1] = 0;;
            log_info("remoter_name: %s\n", adv_data_pt);
            //---------------------new
            sprintf(at_send_adv_buf, "NAME:%s", adv_data_pt);  //rssi
            at_cmd_send_no_end(at_send_adv_buf, strlen(at_send_adv_buf));
            //------------------------
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
            //---------------------new
            sprintf(at_send_adv_buf, "MANU:");
            ret = hex_2_str(adv_data_pt, lenght - 1, &at_send_adv_buf[5]);
            at_cmd_send_no_end(at_send_adv_buf, ret + 5);
            //---------------------

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

//    log_info("rssi:%d\n", report_pt->rssi);
    resolve_adv_report(report_pt->address, report_pt->length, report_pt->data, report_pt->rssi, report_pt->address_type);

}

static int client_create_connection_cannel(void)
{
    if (get_ble_work_state() == BLE_ST_CREATE_CONN) {
        set_ble_work_state(BLE_ST_SEND_CREATE_CONN_CANNEL);
        return ble_op_create_connection_cancel();
    } else {
        return 1;
    }
}

static void connection_update_complete_success(u16 conn_handle, u8 *packet)
{
    int conn_interval, conn_latency, conn_timeout;

    conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
    conn_latency = hci_subevent_le_connection_update_complete_get_conn_latency(packet);
    conn_timeout = hci_subevent_le_connection_update_complete_get_supervision_timeout(packet);

    log_info("conn_handle = %d\n", conn_handle);
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

static const char *const phy_result[] = {
    "None",
    "1M",
    "2M",
    "Coded",
};

static void set_connection_data_length(u16 tx_octets, u16 tx_time)
{
    if (con_handle[cur_dev_cid]) {
        ble_op_set_data_length(con_handle[cur_dev_cid], tx_octets, tx_time);
    }
}

static void set_connection_data_phy(u8 tx_phy, u8 rx_phy)
{
    if (0 == con_handle[cur_dev_cid]) {
        return;
    }

    u8 all_phys = 0;
    u16 phy_options = 0;

    ble_op_set_ext_phy(con_handle[cur_dev_cid], all_phys, tx_phy, rx_phy, phy_options);
}

static void client_profile_start(u16 conn_handle)
{
    ble_op_multi_att_send_conn_handle(conn_handle, cur_dev_cid, 1);
    set_ble_work_state(BLE_ST_CONNECT);

    if (0 == client_config->security_en) {
        client_search_profile_start();
    }
}

/* LISTING_START(packetHandler): Packet Handler */
int client_cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    int tmp;
    u16 tmp_handle;
    u8 status;
    int ret = 0;

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
            put_buf(packet, size);
            switch (hci_event_le_meta_get_subevent_code(packet)) {

            case HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE:
                break;

            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                if (1 == hci_subevent_le_connection_complete_get_role(packet)) {
                    //connect is slave
                    break;
                }

                if (packet[3]) {
                    log_info("LE_MASTER CREATE CONNECTION FAIL!!! %0x\n", packet[3]);
                    set_ble_work_state(BLE_ST_DISCONN);
                    at_respond_send_err(ERR_AT_CMD);
                    break;
                }

                tmp_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                cur_dev_cid = get_idle_dev_index();

                if (cur_dev_cid < 0) {
                    log_info("dev_cid full!!!\n");
                    ble_op_disconnect(tmp_handle);
                    break;
                }

                con_handle[cur_dev_cid] = tmp_handle;

                log_info("HCI_SUBEVENT_LE_CONNECTION_COMPLETE : %0x\n", con_handle[cur_dev_cid]);

                connection_update_complete_success(con_handle[cur_dev_cid], packet + 8);
                client_profile_start(con_handle[cur_dev_cid]);
                client_event_report(CLI_EVENT_CONNECTED, packet, size);

                if (pair_bond_enable) {
                    conn_pair_info.pair_flag = 1;
                    memcpy(&conn_pair_info.peer_address_info, &packet[7], 7);
                    conn_pair_vm_do(&conn_pair_info, 1);
                    pair_bond_enable = 0;
                }
                break;

            case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE: {
                tmp_handle = little_endian_read_16(packet, 4);
                if (get_dev_index(tmp_handle) != HANDLE_INVAIL_INDEX) {
                    connection_update_complete_success(tmp_handle, packet);
                }
            }
            break;

            case HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE:
                tmp_handle = little_endian_read_16(packet, 3);
                if (get_dev_index(tmp_handle) != HANDLE_INVAIL_INDEX) {
                    log_info("APP HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE");
                }
                break;

            case HCI_SUBEVENT_LE_PHY_UPDATE_COMPLETE:
                tmp_handle = little_endian_read_16(packet, 4);
                if (get_dev_index(tmp_handle) != HANDLE_INVAIL_INDEX) {
                    log_info("APP HCI_SUBEVENT_LE_PHY_UPDATE %s\n", hci_event_le_meta_get_phy_update_complete_status(packet) ? "Fail" : "Succ");
                    log_info("Tx PHY: %s\n", phy_result[hci_event_le_meta_get_phy_update_complete_tx_phy(packet)]);
                    log_info("Rx PHY: %s\n", phy_result[hci_event_le_meta_get_phy_update_complete_rx_phy(packet)]);
                }
                break;
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE: {
            tmp_handle = little_endian_read_16(packet, 3);
            s8 tmp_index = get_dev_index(tmp_handle);
            if (tmp_index == HANDLE_INVAIL_INDEX) {
                log_info("unknown_handle:%04x\n", tmp_handle);
                break;
            }

            log_info("HCI_EVENT_DISCONNECTION_COMPLETE:%04x, %0x\n", tmp_handle, packet[5]);
            con_handle[tmp_index] = 0;
            ble_op_multi_att_send_conn_handle(0, tmp_index, 1);
            //set_ble_work_state(BLE_ST_DISCONN);
            //client_event_report(CLI_EVENT_DISCONNECT, packet, size);
            at_send_disconnect(tmp_index);
            //bt_ble_scan_enable(0, 1);
        }
        break;

        case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
            tmp_handle = little_endian_read_16(packet, 2);
            if (get_dev_index(tmp_handle) != HANDLE_INVAIL_INDEX) {
                tmp = att_event_mtu_exchange_complete_get_MTU(packet);
                log_info("ATT MTU = %u\n", tmp);
                ble_op_multi_att_set_send_mtu(con_handle[cur_dev_cid], tmp - 3);
            }
            break;

        case HCI_EVENT_VENDOR_REMOTE_TEST:
            log_info("--- HCI_EVENT_VENDOR_REMOTE_TEST\n");
            break;

        case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE:
            tmp_handle = little_endian_read_16(packet, 2);
            tmp = little_endian_read_16(packet, 4);
            if (get_dev_index(tmp_handle) != HANDLE_INVAIL_INDEX) {
                log_info("-update_rsp:%04x, %02x\n", tmp_handle, tmp);
            }
            ret = 0;//accept
            break;

        case GAP_EVENT_ADVERTISING_REPORT:
            /* putchar('@'); */
            client_report_adv_data((void *)&packet[2], packet[1]);
            break;

        case HCI_EVENT_ENCRYPTION_CHANGE:
            tmp_handle = little_endian_read_16(packet, 3);
            if (get_dev_index(tmp_handle) != HANDLE_INVAIL_INDEX) {
                log_info("HCI_EVENT_ENCRYPTION_CHANGE= %d\n", packet[2]);
                if (client_config->security_en) {
                    client_search_profile_start();
                }
            }
            break;
        }
        break;
    }

    return ret;
}

static int client_operation_send(u16 conn_handle, u16 handle, u8 *data, u16 len, u8 att_op_type)
{
    int ret = APP_BLE_NO_ERROR;
    u32 vaild_len = 0;

    if (!conn_handle || !handle) {
        log_info("handle is null\n");
        return APP_BLE_OPERATION_ERROR;
    }

    ble_op_multi_att_get_remain(conn_handle, &vaild_len);

    if (vaild_len < len) {
        log_info("opt_buff_full!!!\n");
        return APP_BLE_BUFF_FULL;
    }

    ret = ble_op_multi_att_send_data(conn_handle, handle, data, len, att_op_type);
    if (ret == BLE_BUFFER_FULL) {
        ret = APP_BLE_BUFF_FULL;
    }
    return ret;
}

static int client_disconnect(u8 cid)
{
    if (con_handle[cid]) {
        if (1) { //(BLE_ST_SEND_DISCONN != get_ble_work_state()) {
            log_info(">>>ble send disconnect\n");
            //set_ble_work_state(BLE_ST_SEND_DISCONN);
            ble_op_disconnect(con_handle[cid]);
        } else {
            log_info(">>>ble wait disconnect...\n");
        }
        return APP_BLE_NO_ERROR;
    } else {
        return APP_BLE_OPERATION_ERROR;
    }
}
//-----------------------------------------------
//扫描数设置
static void scanning_setup_init(void)
{
    ble_op_set_scan_param(SET_SCAN_TYPE, scan_interval_value, scan_window_value);
}

void ble_client_profile_init(void)
{
    log_info("ble client_profile init\n");

    if (SUPPORT_MAX_DEV > config_le_gatt_client_num) {
        ASSERT(0, "client not enough!!!\n");
        while (1);
    }

    /* setup ATT client */
    gatt_client_init();
    gatt_client_register_packet_handler(client_cbk_packet_handler);

}

static void ble_client_module_enable(u8 en)
{
    log_info("mode_en:%d\n", en);
    if (en) {
        scan_ctrl_en = 1;
    } else {
        bt_ble_scan_enable(0, 0);
        scan_ctrl_en = 0;
    }

    for (u8 i = 0; i < SUPPORT_MAX_DEV; i++) {
        if (con_handle[i]) {
            client_disconnect(i);
        }
    }

}

void bt_ble_client_init(void)
{
    log_info("***** ble_client_init******\n");
    set_ble_work_state(BLE_ST_INIT_OK);
    conn_pair_vm_do(&conn_pair_info, 0);
    ble_client_module_enable(1);

}

void bt_ble_client_exit(void)
{
    log_info("***** ble_client_exit******\n");
    ble_client_module_enable(0);
}

void bt_ble_client_adv_enable(u8 enable)
{
    ble_client_module_enable(enable);
}

//void input_key_handler(u8 key_status, u8 key_number)
//{
//}

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
    if (con_handle[cur_dev_cid]) {
        ble_op_conn_param_update(con_handle[cur_dev_cid], &param);
    }
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
    ble_op_scan_enable2(en, 1);
    return APP_BLE_NO_ERROR;
}

static int client_create_connection(u8 *conn_addr, u8 addr_type)
{
    struct create_conn_param_t *create_conn_par = scan_buffer;
    if (get_ble_work_state() == BLE_ST_CREATE_CONN) {
        log_info("already create conn!!!\n");
        return 2;
    }

    create_conn_par->conn_interval = client_conn_param.interval_max;
    create_conn_par->conn_latency = client_conn_param.latency;
    create_conn_par->supervision_timeout = client_conn_param.timeout;

    memcpy(create_conn_par->peer_address, conn_addr, 6);
    create_conn_par->peer_address_type = addr_type;

    set_ble_work_state(BLE_ST_CREATE_CONN);
    log_info_hexdump(create_conn_par, sizeof(struct create_conn_param_t));
    log_info("------------------------------");
    return ble_op_create_connection(create_conn_par);
}


static int client_create_connection_cancel(void)
{
    if (BLE_ST_CREATE_CONN == get_ble_work_state()) {
        set_ble_work_state(BLE_ST_SEND_CREATE_CONN_CANNEL);
        return ble_op_create_connection_cancel();
    } else {
        return 1;
    }
}

int le_at_client_scan_enable(u8 enable)
{
    if (get_idle_dev_index() == -1) {
        log_info("not idle device!!!\n");
        return 1;
    }
    return bt_ble_scan_enable(0, enable);
}

int le_at_client_creat_connection(u8 *conn_addr, u8 addr_type)
{
    return client_create_connection(conn_addr, addr_type);
}

int le_at_client_creat_cannel(void)
{
    return client_create_connection_cannel();
}

int le_at_client_disconnect(u8 id)
{
    return client_disconnect(id);
}

int le_at_client_set_conn_param(u16 *conn_param)
{
    client_conn_param.interval_min = conn_param[0];  //(unit:0.625ms)
    client_conn_param.interval_max = conn_param[1];  //(unit:0.625ms)
    client_conn_param.latency = conn_param[2];      //(unit: interval)
    client_conn_param.timeout = conn_param[3];      //(unit:10ms)
    return 0;
}

int le_at_client_get_conn_param(u16 *conn_param)
{
    conn_param[0] = client_conn_param.interval_min;  //(unit:0.625ms)
    conn_param[1] = client_conn_param.interval_max;  //(unit:0.625ms)
    conn_param[2] = client_conn_param.latency;       //(unit: interval)
    conn_param[3] = client_conn_param.timeout;      //(unit:10ms)
    return 0;
};

int le_at_client_set_target_uuid16(u16 uuid16)
{
    search_target_uuid16 = uuid16;
    return 0;
}

int le_at_client_get_target_uuid16(void)
{
    return search_target_uuid16;
}

int le_at_client_set_channel(u8 cid)
{
    cur_dev_cid = cid;
    return 0;
}

/**************************************************************************
扫描参数配置
**************************************************************************/
int ble_at_client_scan_param(u16 scan_interval, u16 scan_window)
{
    scan_interval_value = scan_interval;
    scan_window_value = scan_window;
    return 0;
}

int le_att_client_send_data(u8 cid, u8 *packet, u16 size)
{
    log_info("client cid=%d,send data= %d\n", cid, size);

    if (cid < SUPPORT_MAX_DEV && con_handle[cid]) {
        if (target_handle[cid].write_type == ATT_PROPERTY_WRITE_WITHOUT_RESPONSE) {
            return client_operation_send(con_handle[cid], target_handle[cid].write_no_respond, packet, size, ATT_OP_WRITE_WITHOUT_RESPOND);
        } else if (target_handle[cid].write_type == ATT_PROPERTY_WRITE) {
            return client_operation_send(con_handle[cid], target_handle[cid].write_handle, packet, size, ATT_OP_WRITE);
        } else {
            log_info("no write handle ,drop data!!!\n");
            return -1;
        }
        return 0;
    }
}

//----------------------------------------------------------------------------------

void ble_test_auto_scan(u8 en)
{
    log_info("ble_test_auto_scan = %d", en);
    auto_scan_enable = en;
    le_at_client_scan_enable(en);
}

void ble_at_client_test_senddata(void)
{
    s8 i;

    for (i = 0; i < SUPPORT_MAX_DEV; i++) {
        if (con_handle[i]) {
            log_info("%s:%04x\n", __FUNCTION__, con_handle[i]);
            le_att_client_send_data(i, &con_handle[i], 16);
        }
    }
}


#endif




