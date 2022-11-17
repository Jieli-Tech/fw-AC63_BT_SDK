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


#include "hilink_demo.h"
#include "hilink_profile.h"

#include "hilink_protocol.h"
#include "hilink_task.h"

#if CONFIG_APP_HILINK

#define PRINT_HILINK_DATA_EN           0
#if LE_DEBUG_PRINT_EN
#define log_info(x, ...)  printf("[BLE_HILINK]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

//ATT发送的包长,    note: 23 <=need >= MTU
#define ATT_LOCAL_MTU_SIZE        (517)
//ATT缓存的buffer大小,  note: need >= 23,可修改
#define ATT_SEND_CBUF_SIZE        (512)
// 广播周期 (unit:0.625ms)
#define ADV_INTERVAL_MIN          (32)//


//---------------
//连接参数更新请求设置
//是否使能参数请求更新,0--disable, 1--enable
static uint8_t hilink_connection_update_enable = 1; ///0--disable, 1--enable
//当前请求的参数表index
//参数表
static const struct conn_update_param_t hilink_connection_param_table[] = {
    {16, 24, 10, 600},//11
    {12, 28, 10, 600},//3.7
    {8,  20, 10, 600},
};

//共可用的参数组数
#define CONN_PARAM_TABLE_CNT      (sizeof(hilink_connection_param_table)/sizeof(struct conn_update_param_t))

#define EIR_TAG_STRING   0xd6, 0x05, 0x08, 0x00, 'J', 'L', 'A', 'I', 'S', 'D','K'
static const char user_tag_string[] = {EIR_TAG_STRING};

static u8 hilink_adv_data[ADV_RSP_PACKET_MAX];//max is 31
static u8 hilink_scan_rsp_data[ADV_RSP_PACKET_MAX];//max is 31
static u16 hilink_con_handle;
static adv_cfg_t hilink_server_adv_config;
static const struct ble_server_operation_t hilink_ble_operation;
//-------------------------------------------------------------------------------------
static int ble_disconnect(void *priv);
static uint16_t hilink_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
static int hilink_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
static int hilink_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param);
static void hilink_protocol_disconnect();
void hilink_set_close_adv(uint8_t en);

// 用于hilink连接出错没走完连接流程但是没断开时超时断开
#define HILINK_CONN_ERROR_TIME  10000

// 设置的hilink断连超时时间MPP字段,单位:秒
#define HILINK_HEARTBEAT_TIME 0x000a

uint16_t hilink_mtu = ATT_LOCAL_MTU_SIZE - 3;
static uint8_t hilink_ble_conn_state = 0;
static uint16_t hilink_timer = 0;
static uint8_t hilink_conn_finish = 0;
static uint8_t hilink_close_adv_flag = 0;

// hilink 相关信息配置
static dev_info_t hilink_product_info = {
    .prodId = "2HRS",
    .sn = "1234567890AB",
    .dev_id = "",
    .model = "JL_01",
    .dev_t = "119",
    .manu = "JL",
    .hiv = "1.0.0",
    .fwv = "1.0.0",
    .hwv = "1.0.0",
    .swv = "1.0.0",
    .prot_t = "4",
};

// 广播最后两位要与sn/mac最后两位相同
static u8 hilink_service_data[20] = {
    0xEE, 0xFD, 0x01, 0x01, 0x07, 0x04, 0x00, 0x11, 0xE2, 0x12, 0x32, 0x48, 0x52, 0x53, 0xFF, 0x00,
    0x15, 0x02, 0x41, 0x42
};

// ble广播名称
static char gap_name_unbound[] = "Hi-jieli-12HRS0090AB";
static char gap_name_bound[] = "HI-jieli-12HRS0090AB";

//-------------------------------------------------------------------------------------
//输入passkey 加密
#define PASSKEY_ENABLE                     0

static const sm_cfg_t hilink_sm_init_config = {
    .slave_security_auto_req = 0,
    .slave_set_wait_security = 0,

#if PASSKEY_ENABLE
    .io_capabilities = IO_CAPABILITY_DISPLAY_ONLY,
#else
    .io_capabilities = IO_CAPABILITY_NO_INPUT_NO_OUTPUT,
#endif

    .authentication_req_flags = SM_AUTHREQ_MITM_PROTECTION | SM_AUTHREQ_BONDING,
    .min_key_size = 7,
    .max_key_size = 16,
    .sm_cb_packet_handler = NULL,
};

const gatt_server_cfg_t hilink_server_init_cfg = {
    .att_read_cb = &hilink_att_read_callback,
    .att_write_cb = &hilink_att_write_callback,
    .event_packet_handler = &hilink_event_packet_handler,
};

static gatt_ctrl_t hilink_gatt_control_block = {
    //public
    .mtu_size = ATT_LOCAL_MTU_SIZE,
    .cbuffer_size = ATT_SEND_CBUF_SIZE,
    .multi_dev_flag	= 0,

    //config
#if CONFIG_BT_GATT_SERVER_NUM
    .server_config = &hilink_server_init_cfg,
#else
    .server_config = NULL,
#endif

    .client_config = NULL,

#if CONFIG_BT_SM_SUPPORT_ENABLE
    .sm_config = &hilink_sm_init_config,
#else
    .sm_config = NULL,
#endif
    //cbk,event handle
    .hci_cb_packet_handler = NULL,
};
#define TEST_AUDIO_DATA_UPLOAD       0//测试文件上传

#if TEST_AUDIO_DATA_UPLOAD
static const u8 test_audio_data_file[1024] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9
};

#define AUDIO_ONE_PACKET_LEN  128
static void hilink_test_send_audio_data(int init_flag)
{
    static u32 send_pt = 0;
    static u32 start_flag = 0;

    if (!hilink_con_handle) {
        return;
    }

    if (init_flag) {
        log_info("audio send init\n");
        send_pt = 0;
        start_flag = 1;
    }

    if (!start_flag) {
        return;
    }

    u32 file_size = sizeof(test_audio_data_file);
    u8 *file_ptr = test_audio_data_file;

    if (send_pt >= file_size) {
        log_info("audio send Complete\n");
        start_flag = 0;
        return;
    }

    u32 send_len = file_size - send_pt;
    if (send_len > AUDIO_ONE_PACKET_LEN) {
        send_len = AUDIO_ONE_PACKET_LEN;
    }

    while (1) {
        if (ble_comm_cbuffer_vaild_len(hilink_con_handle) > send_len) {
            log_info("audio send %08x\n", send_pt);
            if (ble_comm_att_send_data(hilink_con_handle, ATT_CHARACTERISTIC_ae3c_01_VALUE_HANDLE, &file_ptr[send_pt], send_len, ATT_OP_AUTO_READ_CCC)) {
                log_info("audio send fail!\n");
                break;
            } else {
                send_pt += send_len;
            }
        } else {
            break;
        }
    }
}
#endif

void hilink_set_conn_finish(uint8_t value)
{
    log_info("hilink_set_conn_finish %d", value);
    hilink_conn_finish = value;
}

// 用于处理频繁切换导致没有完成连接流程但是ble还是连着
void hilink_check_conn_finish()
{
    if (!hilink_conn_finish) {
        log_info("conn get stuck,disconnect!!!");
        ble_disconnect(NULL);
    }
}

//-------------------------------------------------------------------------------------
static int hilink_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param)
{
    //log_info("event: %02x,size= %d\n", event, size);

    switch (event) {
    case GATT_COMM_EVENT_CONNECTION_COMPLETE:
        log_info("connection_handle:%04x\n", little_endian_read_16(packet, 0));
        log_info("peer_address_info:");
        put_buf(&ext_param[7], 7);
        hilink_con_handle = little_endian_read_16(packet, 0);
        hilink_connection_update_enable = 1;
        hilink_ble_conn_state = 1;
        hilink_set_close_adv(0);
        log_info("hilink_ble_connection_handler\n");
        hilink_timer = sys_timeout_add(NULL, hilink_check_conn_finish, HILINK_CONN_ERROR_TIME);
        break;

    case GATT_COMM_EVENT_DISCONNECT_COMPLETE:
        log_info("disconnect_handle:%04x,reason= %02x\n", little_endian_read_16(packet, 0), packet[2]);
        hilink_con_handle = 0;
        hilink_ble_conn_state = 0;
        hilink_conn_finish = 0;
        log_info("hilink_ble_disconnection_handler\n");
        hilink_protocol_disconnect();
        break;

    case GATT_COMM_EVENT_ENCRYPTION_CHANGE:
        log_info("ENCRYPTION_CHANGE:handle=%04x,state=%d,process =%d", little_endian_read_16(packet, 0), packet[2], packet[3]);

        break;

    case GATT_COMM_EVENT_CONNECTION_UPDATE_COMPLETE:
        log_info("conn_param update_complete:%04x\n", little_endian_read_16(packet, 0));
        break;

    case GATT_COMM_EVENT_CAN_SEND_NOW:
#if TEST_AUDIO_DATA_UPLOAD
        hilink_test_send_audio_data(0);
#endif
        break;

    case GATT_COMM_EVENT_CONNECTION_UPDATE_REQUEST_RESULT:
    case GATT_COMM_EVENT_MTU_EXCHANGE_COMPLETE:
        uint16_t mtu_tmp = little_endian_read_16(packet, 2) - 3;
        if (mtu_tmp >= 23 && mtu_tmp <= 517) {
            hilink_mtu = mtu_tmp;
            log_info("GATT_COMM_EVENT_MTU_EXCHANGE_COMPLETE, mtu:%d", hilink_mtu);
        } else {
            log_info("GATT_COMM_EVENT_MTU_EXCHANGE_ERROR, mtu:%d", mtu_tmp);
        }
        break;

    default:
        break;
    }
    return 0;
}

static void hilink_send_connetion_updata_deal(u16 conn_handle)
{
    if (hilink_connection_update_enable) {
        if (0 == ble_gatt_server_connetion_update_request(conn_handle, hilink_connection_param_table, CONN_PARAM_TABLE_CNT)) {
            hilink_connection_update_enable = 0;
        }
    }
}

// ATT Client Read Callback for Dynamic Data
// - if buffer == NULL, don't copy data, just return size of value
// - if buffer != NULL, copy data and return number bytes copied
// @param offset defines start of attribute value
static uint16_t hilink_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    uint16_t  att_value_len = 0;
    uint16_t handle = att_handle;

    log_info("read_callback,conn_handle =%04x, handle=%04x,buffer=%08x\n", connection_handle, handle, (u32)buffer);

    switch (handle) {
    case ATT_CHARACTERISTIC_2A00_01_VALUE_HANDLE: {
        att_value_len = strlen(gap_name_unbound);

        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }

        if (buffer) {
            memcpy(buffer, &gap_name_unbound[offset], buffer_size);
            log_info("\n------read gap_name: %s\n", gap_name_unbound);
            att_value_len = buffer_size;
        }
    }
    break;

    case ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_15F1E601_A277_43FC_A484_DD39EF8A9100_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_15f1e611_a277_43fc_a484_dd39ef8a9100_01_CLIENT_CONFIGURATION_HANDLE:
        if (buffer) {
            buffer[0] = att_get_ccc_config(handle);
            buffer[1] = 0;
        }
        att_value_len = 2;
        break;

    default:
        log_info("\n\nread unknow handle:%d\n\n", handle);
        break;
    }

    log_info("att_value_len= %d\n", att_value_len);
    return att_value_len;
}
/*
 * @section ATT Write
 */

/* LISTING_START(attWrite): ATT Write */
static int hilink_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    int result = 0;
    u16 tmp16;

    u8 *tmp_buf;
    u16 handle = att_handle;

    log_info("write_callback,conn_handle =0x%04x, handle =0x%04x,size =%d\n", connection_handle, handle, buffer_size);
    //put_buf(buffer, buffer_size);

    switch (handle) {
    // hilink ctl data msg 0x0d
    case ATT_CHARACTERISTIC_15F1E602_A277_43FC_A484_DD39EF8A9100_01_VALUE_HANDLE:
        tmp_buf = malloc(buffer_size +  HILINK_PACKET_HEAD_LEN);
        memcpy(tmp_buf + HILINK_PACKET_HEAD_LEN, buffer, buffer_size);
        ((HILINK_PACKET_HEAD_T *)tmp_buf)->packet_channel = HILINK_DEVICE_INFO_MSG_CH;
        ((HILINK_PACKET_HEAD_T *)tmp_buf)->len = buffer_size;
        hilink_packet_recieve(tmp_buf, buffer_size +  HILINK_PACKET_HEAD_LEN);
        free(tmp_buf);
        break;

    // OTA_CTL msg 0x10
    case ATT_CHARACTERISTIC_15f1e611_a277_43fc_a484_dd39ef8a9100_01_VALUE_HANDLE:
        tmp_buf = malloc(buffer_size +  HILINK_PACKET_HEAD_LEN);
        memcpy(tmp_buf + HILINK_PACKET_HEAD_LEN, buffer, buffer_size);
        ((HILINK_PACKET_HEAD_T *)tmp_buf)->packet_channel = HILINK_OTA_CTL_MSG_CH;
        ((HILINK_PACKET_HEAD_T *)tmp_buf)->len = buffer_size;
        hilink_packet_recieve(tmp_buf, buffer_size +  HILINK_PACKET_HEAD_LEN);
        free(tmp_buf);
        break;
    // OTA_DATA msg 0x13
    case ATT_CHARACTERISTIC_15f1e612_a277_43fc_a484_dd39ef8a9100_01_VALUE_HANDLE:
        tmp_buf = malloc(buffer_size +  HILINK_PACKET_HEAD_LEN);
        memcpy(tmp_buf + HILINK_PACKET_HEAD_LEN, buffer, buffer_size);
        ((HILINK_PACKET_HEAD_T *)tmp_buf)->packet_channel = HILINK_OTA_DATA_MSG_CH;
        ((HILINK_PACKET_HEAD_T *)tmp_buf)->len = buffer_size;
        hilink_packet_recieve(tmp_buf, buffer_size +  HILINK_PACKET_HEAD_LEN);
        free(tmp_buf);
        break;
    case ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_15F1E601_A277_43FC_A484_DD39EF8A9100_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_15f1e611_a277_43fc_a484_dd39ef8a9100_01_CLIENT_CONFIGURATION_HANDLE:
        hilink_send_connetion_updata_deal(connection_handle);
        log_info("\n------write ccc:%04x,%02x\n", handle, buffer[0]);
        ble_gatt_server_characteristic_ccc_set(connection_handle, handle, buffer[0]);
        break;

    default:
        log_info("\n\nwrite unknow handle:%d\n\n", handle);
        break;
    }
    return 0;
}

static void app_set_adv_data(u8 *adv_data, u8 adv_len)
{
    log_info("app_set_adv_data");
    hilink_server_adv_config.adv_data_len = adv_len;
    hilink_server_adv_config.adv_data = adv_data;
    put_buf(hilink_server_adv_config.adv_data, hilink_server_adv_config.adv_data_len);
}

static void app_set_rsp_data(u8 *rsp_data, u8 rsp_len)
{
    log_info("app_set_rsp_data");
    hilink_server_adv_config.rsp_data_len = rsp_len;
    hilink_server_adv_config.rsp_data = rsp_data;
    put_buf(hilink_server_adv_config.rsp_data, hilink_server_adv_config.rsp_data_len);
}

static u8  adv_name_ok = 0;//name 优先存放在ADV包

/*
 * 一靠广播
 */
static int hilink_oneclose_adv(void)
{
    u8 offset = 0;
    u8 *buf = hilink_adv_data;

    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, FLAGS_GENERAL_DISCOVERABLE_MODE | FLAGS_EDR_NOT_SUPPORTED, 1);

    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_SERVICE_DATA, hilink_service_data, sizeof(hilink_service_data));

    log_info("adv_type:one close adv");

    u8 name_len = strlen(gap_name_unbound);
    u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
    if (name_len < vaild_len) {
        offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_name_unbound, name_len);
        adv_name_ok = 1;
    } else {
        adv_name_ok = 0;
    }

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***hilink_adv_data overflow!!!!!!\n");
        return -1;
    }
    log_info("hilink_adv_data(%d):", offset);
    log_info_hexdump(buf, offset);
    hilink_server_adv_config.adv_data_len = offset;
    hilink_server_adv_config.adv_data = hilink_adv_data;
    return 0;
}

/*
 * 二靠广播
 */
static int hilink_twoclose_adv(void)
{
    u8 offset = 0;
    u8 *buf = hilink_adv_data;

    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, FLAGS_GENERAL_DISCOVERABLE_MODE | FLAGS_EDR_NOT_SUPPORTED, 1);

    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_SERVICE_DATA, hilink_service_data, sizeof(hilink_service_data));

    log_info("adv_type:two close adv");
    buf[21] = 0x00;

    u8 name_len = strlen(gap_name_bound);
    u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
    if (name_len < vaild_len) {
        offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_name_bound, name_len);
        adv_name_ok = 1;
    } else {
        adv_name_ok = 0;
    }

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***hilink_adv_data overflow!!!!!!\n");
        return -1;
    }
    log_info("hilink_adv_data(%d):", offset);
    log_info_hexdump(buf, offset);
    hilink_server_adv_config.adv_data_len = offset;
    hilink_server_adv_config.adv_data = hilink_adv_data;
    return 0;
}

/*
 * 未绑定常态广播
 */
static int hilink_unbound_normal_adv(void)
{
    u8 offset = 0;
    u8 *buf = hilink_adv_data;

    log_info("adv_type:hilink_unbound_normal_adv");

    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, FLAGS_GENERAL_DISCOVERABLE_MODE | FLAGS_EDR_NOT_SUPPORTED, 1);
    buf[21] = 0x00;

    u8 name_len = strlen(gap_name_unbound) + 3;
    u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
    if (name_len < vaild_len) {
        uint8_t *name_buf = malloc(name_len);
        memcpy(name_buf, gap_name_unbound, strlen(gap_name_unbound));
        name_buf[name_len - 3] = 0x02;
        name_buf[name_len - 2] = HILINK_HEARTBEAT_TIME & 0x0f;
        name_buf[name_len - 1] = HILINK_HEARTBEAT_TIME >> 8;

        offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)name_buf, name_len);
        free(name_buf);
        adv_name_ok = 1;
    } else {
        adv_name_ok = 0;
    }

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***hilink_adv_data overflow!!!!!!\n");
        return -1;
    }
    log_info("hilink_adv_data(%d):", offset);
    log_info_hexdump(buf, offset);
    hilink_server_adv_config.adv_data_len = offset;
    hilink_server_adv_config.adv_data = hilink_adv_data;
    return 0;
}

/*
 * 已绑定常态广播
 */
static int hilink_bound_normal_adv(void)
{
    u8 offset = 0;
    u8 *buf = hilink_adv_data;

    log_info("adv_type:hilink_bound_normal_adv");

    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, FLAGS_GENERAL_DISCOVERABLE_MODE | FLAGS_EDR_NOT_SUPPORTED, 1);

    buf[21] = 0x00;

    u8 name_len = strlen(gap_name_bound);
    u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
    if (name_len < vaild_len) {
        uint8_t *name_buf = malloc(name_len);
        memcpy(name_buf, gap_name_unbound, strlen(gap_name_unbound));
        name_buf[name_len - 3] = 0x02;
        name_buf[name_len - 2] = HILINK_HEARTBEAT_TIME & 0x0f;
        name_buf[name_len - 1] = HILINK_HEARTBEAT_TIME >> 8;
        offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)name_buf, name_len);
        free(name_buf);
        adv_name_ok = 1;
    } else {
        adv_name_ok = 0;
    }

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***hilink_adv_data overflow!!!!!!\n");
        return -1;
    }
    log_info("hilink_adv_data(%d):", offset);
    log_info_hexdump(buf, offset);
    hilink_server_adv_config.adv_data_len = offset;
    hilink_server_adv_config.adv_data = hilink_adv_data;
    return 0;
}

/*
 * 根据设备状态决定发什么广播
 */
static int hilink_make_set_adv_data(void)
{
    if (!hilink_get_pair_state() && hilink_close_adv_flag) {
        hilink_oneclose_adv();
    } else if (hilink_get_pair_state() && hilink_close_adv_flag) {
        hilink_twoclose_adv();
    } else if (!hilink_get_pair_state() && !hilink_close_adv_flag) {
        hilink_unbound_normal_adv();
    } else if (hilink_get_pair_state() && !hilink_close_adv_flag) {
        //hilink_bound_normal_adv();
        //已配对未连接时统一发二靠广播，IOS那边回连必须用二靠广播
        hilink_twoclose_adv();
    } else {
        log_info("adv set error happen!");
    }
    return 0;
}

static int hilink_make_set_rsp_data(void)
{
    u8 offset = 0;
    u8 *buf = hilink_scan_rsp_data;

    if (!adv_name_ok) {
        u8 name_len = strlen(gap_name_bound);
        u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
        if (name_len > vaild_len) {
            name_len = vaild_len;
        }
        if (hilink_get_pair_state()) {
            offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_name_bound, name_len);
        } else {
            offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_name_unbound, name_len);
        }
    }

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***rsp_data overflow!!!!!!\n");
        return -1;
    }

    log_info("rsp_data(%d):", offset);
    log_info_hexdump(buf, offset);
    hilink_server_adv_config.rsp_data_len = offset;
    hilink_server_adv_config.rsp_data = hilink_scan_rsp_data;
    return 0;
}

//广播参数设置
void hilink_adv_config_set(void)
{
    int ret = 0;
    ret |= hilink_make_set_adv_data();
    ret |= hilink_make_set_rsp_data();

    hilink_server_adv_config.adv_interval = ADV_INTERVAL_MIN;
    hilink_server_adv_config.adv_auto_do = 1;
    hilink_server_adv_config.adv_type = ADV_IND;
    hilink_server_adv_config.adv_channel = ADV_CHANNEL_ALL;
    memset(hilink_server_adv_config.direct_address_info, 0, 7);

    if (ret) {
        log_info("adv_setup_init fail!!!\n");
        return;
    }
    ble_gatt_server_set_adv_config(&hilink_server_adv_config);
}

void hilink_set_close_adv(uint8_t en)
{
    ble_gatt_server_adv_enable(0);
    hilink_close_adv_flag = en;
    hilink_adv_config_set();
    ble_gatt_server_adv_enable(1);
}

void hilink_server_init(void)
{
    log_info("%s", __FUNCTION__);
    ble_gatt_server_set_profile(hilink_profile_data, sizeof(hilink_profile_data));
    hilink_adv_config_set();
}

static int hilink_app_send_user_data_do(void *priv, u8 *data, u16 len)
{
#if PRINT_HILINK_DATA_EN
    log_info("-le_tx(%d):", len);
    log_info_hexdump(data, len);
#endif
    ble_gatt_server_characteristic_ccc_set(hilink_con_handle, ATT_CHARACTERISTIC_15F1E601_A277_43FC_A484_DD39EF8A9100_01_VALUE_HANDLE + 1, ATT_PROPERTY_READ);
    return ble_comm_att_send_data(hilink_con_handle, ATT_CHARACTERISTIC_15F1E601_A277_43FC_A484_DD39EF8A9100_01_VALUE_HANDLE, data, len, ATT_OP_AUTO_READ_CCC);
}

static int hilink_app_send_user_ota_data_do(void *priv, u8 *data, u16 len)
{
#if PRINT_HILINK_DATA_EN
    log_info("-le_tx(%d):", len);
    log_info_hexdump(data, len);
#endif
    ble_gatt_server_characteristic_ccc_set(hilink_con_handle, ATT_CHARACTERISTIC_15f1e611_a277_43fc_a484_dd39ef8a9100_01_VALUE_HANDLE + 1, ATT_PROPERTY_READ);
    return ble_comm_att_send_data(hilink_con_handle, ATT_CHARACTERISTIC_15f1e611_a277_43fc_a484_dd39ef8a9100_01_VALUE_HANDLE, data, len, ATT_OP_AUTO_READ_CCC);
}

void hilink_data_send(u8 *data, u16 len)
{
    hilink_app_send_user_data_do(NULL, data, len);
}

void hilink_ota_data_send(u8 *data, u16 len)
{
    hilink_app_send_user_ota_data_do(NULL, data, len);
}

void bt_ble_before_start_init(void)
{
    log_info("%s", __FUNCTION__);
    ble_comm_init(&hilink_gatt_control_block);
}

static void hilink_init()
{
    // 设置hilink的加解密信息存储的vm_id
    hilink_set_auth_info_store_vm_id(CFG_HILINK_AUTH_INFO_SETTING);
    hilink_set_attr_info_store_vm_id(CFG_HILINK_ATTR_INFO_SETTING);

    hilink_task_init();
    hilink_auth_info_read();
    hilink_attr_info_read();
}

void bt_ble_init(void)
{
    hilink_info_set(&hilink_product_info);
    log_info("%s\n", __FUNCTION__);
    log_info("ble_file: %s", __FILE__);
#if DOUBLE_BT_SAME_NAME
    ble_comm_set_config_name(bt_get_local_name(), 0);
#else
    ble_comm_set_config_name(bt_get_local_name(), 1);
#endif
    hilink_con_handle = 0;

    hilink_task_init();
    hilink_init();
    hilink_server_init();

    ble_module_enable(1);

}

static void hilink_protocol_disconnect()
{
    if (hilink_timer) {
        sys_timeout_del(hilink_timer);
        hilink_timer = 0;
    }
    hilink_ota_exit();
}

void bt_ble_exit(void)
{
    log_info("%s\n", __FUNCTION__);
    ble_module_enable(0);
    ble_comm_exit();
}

uint8_t hilink_get_ble_conn_state()
{
    return hilink_ble_conn_state;
}

void ble_module_enable(u8 en)
{
    ble_comm_module_enable(en);
}

static int ble_disconnect(void *priv)
{
    ble_comm_disconnect(hilink_con_handle);
}

_WEAK_ void hilink_auth_reset()
{

}

void hilink_reset()
{
    hilink_auth_reset();
    ble_disconnect(NULL);
    ble_gatt_server_adv_enable(0);
    hilink_adv_config_set();
    ble_gatt_server_adv_enable(1);
}

static const struct ble_server_operation_t hilink_ble_operation = {
    .disconnect = ble_disconnect,
    .send_data = (void *)hilink_app_send_user_data_do,
};

void hilink_ble_get_server_operation_table(struct ble_server_operation_t **interface_pt)
{
    *interface_pt = (void *)&hilink_ble_operation;
}

#endif


