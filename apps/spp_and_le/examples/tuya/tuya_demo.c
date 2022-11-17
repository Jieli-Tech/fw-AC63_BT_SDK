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

#include "tuya_demo.h"
#include "tuya_ble_api.h"
#include "tuya_profile.h"

#if CONFIG_APP_TUYA

#if LE_DEBUG_PRINT_EN
#define log_info(x, ...)  printf("[BLE_TUYA]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

#if (TUYA_BLE_PROTOCOL_VERSION_HIGN == 0x04)
#define ATT_LOCAL_MTU_SIZE    (246)                   //
//ATT缓存的buffer大小,  note: need >= 23,可修改
#define ATT_SEND_CBUF_SIZE        (246)                   //
#else
#define ATT_LOCAL_MTU_SIZE    (23)                   //
//ATT缓存的buffer大小,  note: need >= 23,可修改
#define ATT_SEND_CBUF_SIZE        (246)                   //
#endif

// 广播周期 (unit:0.625ms)
#define ADV_INTERVAL_MIN          (160 * 5)//

//---------------
//连接参数更新请求设置
//是否使能参数请求更新,0--disable, 1--enable
static uint8_t tuya_connection_update_enable = 1; ///0--disable, 1--enable
//当前请求的参数表index
//参数表
static const struct conn_update_param_t tuya_connection_param_table[] = {
    {16, 24, 10, 600},//11
    {12, 28, 10, 600},//3.7
    {8,  20, 10, 600},
};

//共可用的参数组数
#define CONN_PARAM_TABLE_CNT      (sizeof(tuya_connection_param_table)/sizeof(struct conn_update_param_t))

#define EIR_TAG_STRING   0xd6, 0x05, 0x08, 0x00, 'J', 'L', 'A', 'I', 'S', 'D','K'
static const char user_tag_string[] = {EIR_TAG_STRING};

static u8 tuya_adv_data[ADV_RSP_PACKET_MAX];//max is 31
static u8 tuya_scan_rsp_data[ADV_RSP_PACKET_MAX];//max is 31
static u16 tuya_con_handle;
static adv_cfg_t tuya_server_adv_config;
static const struct ble_server_operation_t tuya_ble_operation;
//-------------------------------------------------------------------------------------
static uint16_t tuya_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
static int tuya_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
static int tuya_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param);

//-------------------------------------------------------------------------------------
//输入passkey 加密
#define PASSKEY_ENABLE                     0

static const sm_cfg_t tuya_sm_init_config = {
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

const gatt_server_cfg_t tuya_server_init_cfg = {
    .att_read_cb = &tuya_att_read_callback,
    .att_write_cb = &tuya_att_write_callback,
    .event_packet_handler = &tuya_event_packet_handler,
};

static gatt_ctrl_t tuya_gatt_control_block = {
    //public
    .mtu_size = ATT_LOCAL_MTU_SIZE,
    .cbuffer_size = ATT_SEND_CBUF_SIZE,
    .multi_dev_flag	= 0,

    //config
#if CONFIG_BT_GATT_SERVER_NUM
    .server_config = &tuya_server_init_cfg,
#else
    .server_config = NULL,
#endif

    .client_config = NULL,

#if CONFIG_BT_SM_SUPPORT_ENABLE
    .sm_config = &tuya_sm_init_config,
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
static void tuya_test_send_audio_data(int init_flag)
{
    static u32 send_pt = 0;
    static u32 start_flag = 0;

    if (!tuya_con_handle) {
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
        if (ble_comm_cbuffer_vaild_len(tuya_con_handle) > send_len) {
            log_info("audio send %08x\n", send_pt);
            if (ble_comm_att_send_data(tuya_con_handle, ATT_CHARACTERISTIC_ae3c_01_VALUE_HANDLE, &file_ptr[send_pt], send_len, ATT_OP_AUTO_READ_CCC)) {
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

//-------------------------------------------------------------------------------------
static int tuya_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param)
{
    log_info("event: %02x,size= %d\n", event, size);

    switch (event) {
    case GATT_COMM_EVENT_CONNECTION_COMPLETE:
        log_info("connection_handle:%04x\n", little_endian_read_16(packet, 0));
        log_info("peer_address_info:");
        put_buf(&ext_param[7], 7);
        tuya_con_handle = little_endian_read_16(packet, 0);
        tuya_connection_update_enable = 1;
        log_info("tuya_ble_connection_handler\n");
        tuya_ble_connected_handler();
        break;

    case GATT_COMM_EVENT_DISCONNECT_COMPLETE:
        log_info("disconnect_handle:%04x,reason= %02x\n", little_endian_read_16(packet, 0), packet[2]);
        tuya_con_handle = 0;
        log_info("tuya_ble_disconnection_handler\n");
        tuya_ble_disconnected_handler();
        break;

    case GATT_COMM_EVENT_ENCRYPTION_CHANGE:
        log_info("ENCRYPTION_CHANGE:handle=%04x,state=%d,process =%d", little_endian_read_16(packet, 0), packet[2], packet[3]);

        break;

    case GATT_COMM_EVENT_CONNECTION_UPDATE_COMPLETE:
        log_info("conn_param update_complete:%04x\n", little_endian_read_16(packet, 0));
        break;

    case GATT_COMM_EVENT_CAN_SEND_NOW:
#if TEST_AUDIO_DATA_UPLOAD
        tuya_test_send_audio_data(0);
#endif
        break;

    case GATT_COMM_EVENT_CONNECTION_UPDATE_REQUEST_RESULT:
        break;
    case GATT_COMM_EVENT_MTU_EXCHANGE_COMPLETE:
        log_info("con_handle= %02x, ATT MTU = %u\n", little_endian_read_16(packet, 0), little_endian_read_16(packet, 2));
        break;

    default:
        break;
    }
    return 0;
}


// ATT Client Read Callback for Dynamic Data
// - if buffer == NULL, don't copy data, just return size of value
// - if buffer != NULL, copy data and return number bytes copied
// @param offset defines start of attribute value
static uint16_t tuya_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    uint16_t  att_value_len = 0;
    uint16_t handle = att_handle;

    log_info("read_callback,conn_handle =%04x, handle=%04x,buffer=%08x\n", connection_handle, handle, (u32)buffer);

    switch (handle) {
    default:
        break;
    }

    log_info("att_value_len= %d\n", att_value_len);
    return att_value_len;
}

static void tuya_send_connetion_updata_deal(u16 conn_handle)
{
    if (tuya_connection_update_enable) {
        if (0 == ble_gatt_server_connetion_update_request(conn_handle, tuya_connection_param_table, CONN_PARAM_TABLE_CNT)) {
            tuya_connection_update_enable = 0;
        }
    }
}

/*
 * @section ATT Write
 */

/* LISTING_START(attWrite): ATT Write */
static int tuya_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    int result = 0;
    u16 tmp16;

    u16 handle = att_handle;

    log_info("write_callback,conn_handle =%04x, handle =%04x,size =%d\n", connection_handle, handle, buffer_size);

    switch (handle) {
#if (TUYA_BLE_PROTOCOL_VERSION_HIGN == 0x03)
    case ATT_CHARACTERISTIC_2b10_01_CLIENT_CONFIGURATION_HANDLE:
        tuya_send_connetion_updata_deal(connection_handle);
        printf("\n------write ccc:%04x,%02x\n", handle, buffer[0]);
        ble_gatt_server_characteristic_ccc_set(connection_handle, handle, buffer[0]);
        break;

    case ATT_CHARACTERISTIC_2b11_01_VALUE_HANDLE:
        printf("TUYA msg receive, handle = 0x%x, size = %d\n", handle, buffer_size);
        JL_tuya_ble_gatt_receive_data(buffer, buffer_size);
        break;
#endif

#if (TUYA_BLE_PROTOCOL_VERSION_HIGN == 0x04)
    case ATT_CHARACTERISTIC_00000002_0000_1001_8001_00805F9B07D0_01_CLIENT_CONFIGURATION_HANDLE:
        tuya_send_connetion_updata_deal(connection_handle);
        printf("\n------write ccc:%04x,%02x\n", handle, buffer[0]);
        ble_gatt_server_characteristic_ccc_set(connection_handle, handle, buffer[0]);
        break;

    case ATT_CHARACTERISTIC_00000001_0000_1001_8001_00805F9B07D0_01_VALUE_HANDLE:
        printf("\nTUYA msg receive, handle = 0x%x, size = %d\n", handle, buffer_size);
        JL_tuya_ble_gatt_receive_data(buffer, buffer_size);
        break;

    case ATT_CHARACTERISTIC_00000003_0000_1001_8001_00805F9B07D0_01_VALUE_HANDLE:
        printf("\nTUYA read msg, handle = 0x%x, size = %d\n", handle, buffer_size);
        put_buf(buffer, buffer_size);
        break;

#endif
        break;

    default:
        printf("\n\nunknow handle:%d\n\n", handle);
        break;
    }
    return 0;
}

static void app_set_adv_data(u8 *adv_data, u8 adv_len)
{
    log_info("app_set_adv_data");
    tuya_server_adv_config.adv_data_len = adv_len;
    tuya_server_adv_config.adv_data = adv_data;
    put_buf(tuya_server_adv_config.adv_data, tuya_server_adv_config.adv_data_len);
}

static void app_set_rsp_data(u8 *rsp_data, u8 rsp_len)
{
    log_info("app_set_rsp_data");
    tuya_server_adv_config.rsp_data_len = rsp_len;
    tuya_server_adv_config.rsp_data = rsp_data;
    put_buf(tuya_server_adv_config.rsp_data, tuya_server_adv_config.rsp_data_len);
}

//广播参数设置
void tuya_adv_config_set(void)
{
    int ret = 0;

    //tuya_ble_adv_get(&tuya_server_adv_config.adv_data, &tuya_server_adv_config.adv_data_len, &tuya_server_adv_config.rsp_data, &tuya_server_adv_config.rsp_data_len);

    tuya_server_adv_config.adv_interval = ADV_INTERVAL_MIN;
    tuya_server_adv_config.adv_auto_do = 1;
    tuya_server_adv_config.adv_type = ADV_IND;
    tuya_server_adv_config.adv_channel = ADV_CHANNEL_ALL;
    memset(tuya_server_adv_config.direct_address_info, 0, 7);

    if (ret) {
        log_info("adv_setup_init fail!!!\n");
        return;
    }
    ble_gatt_server_set_adv_config(&tuya_server_adv_config);
}

void tuya_server_init(void)
{
    log_info("%s", __FUNCTION__);
    ble_gatt_server_set_profile(tuya_profile_data, sizeof(tuya_profile_data));
    tuya_adv_config_set();
}

static int tuya_app_send_user_data_do(void *priv, u8 *data, u16 len)
{
#if PRINT_DMA_DATA_EN
    if (len < 128) {
        log_info("-le_tx(%d):");
        log_info_hexdump(data, len);
    } else {
        putchar('L');
    }
#endif
    ble_gatt_server_characteristic_ccc_set(tuya_con_handle, ATT_CHARACTERISTIC_00000002_0000_1001_8001_00805F9B07D0_01_VALUE_HANDLE + 1, ATT_OP_NOTIFY);
    return ble_comm_att_send_data(tuya_con_handle, ATT_CHARACTERISTIC_00000002_0000_1001_8001_00805F9B07D0_01_VALUE_HANDLE, data, len, ATT_OP_AUTO_READ_CCC);
}

void bt_ble_before_start_init(void)
{
    log_info("%s", __FUNCTION__);
    ble_comm_init(&tuya_gatt_control_block);
}

static void tuya_func_register()
{
    tuya_app_set_adv_data_register(app_set_adv_data);
    tuya_app_set_rsp_data_register(app_set_rsp_data);
}

void bt_ble_init(void)
{
    log_info("%s\n", __FUNCTION__);
    log_info("ble_file: %s", __FILE__);
    tuya_func_register();
    tuya_ble_operation_register(&tuya_ble_operation);
#if DOUBLE_BT_SAME_NAME
    ble_comm_set_config_name(bt_get_local_name(), 0);
#else
    ble_comm_set_config_name(bt_get_local_name(), 1);
#endif
    tuya_con_handle = 0;

    extern void tuya_ble_app_init();
    tuya_server_init();
    tuya_ble_app_init();
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

static int ble_disconnect(void *priv)
{
    ble_comm_disconnect(tuya_con_handle);
}

static const struct ble_server_operation_t tuya_ble_operation = {
    .disconnect = ble_disconnect,
    .send_data = (void *)tuya_app_send_user_data_do,
};

void tuya_ble_get_server_operation_table(struct ble_server_operation_t **interface_pt)
{
    *interface_pt = (void *)&tuya_ble_operation;
}

#endif


