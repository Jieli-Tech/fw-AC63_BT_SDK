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

#include "ll_sync_profile.h"
#include "ble_qiot_export.h"
#include "ble_qiot_service.h"
#include "ll_task.h"


#if CONFIG_APP_LL_SYNC

#if 1
#define log_info(x, ...)  printf("[BLE_llsync]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

//ATT发送的包长,    note: 23 <=need >= MTU
#define ATT_LOCAL_MTU_SIZE        (200)
//ATT缓存的buffer大小,  note: need >= 23,可修改
#define ATT_SEND_CBUF_SIZE        (512)
// 广播周期 (unit:0.625ms)
#define ADV_INTERVAL_MIN          (160 * 5)//

#define LL_TECENT_SUPPORT_TASK       1

//---------------
//连接参数更新请求设置
//是否使能参数请求更新,0--disable, 1--enable
static uint8_t llsync_connection_update_enable = 1; ///0--disable, 1--enable
//当前请求的参数表index
//参数表
static const struct conn_update_param_t llsync_connection_param_table[] = {
    {16, 24, 10, 600},//11
    {12, 28, 10, 600},//3.7
    {8,  20, 10, 600},
};

//共可用的参数组数
#define CONN_PARAM_TABLE_CNT      (sizeof(llsync_connection_param_table)/sizeof(struct conn_update_param_t))

#define EIR_TAG_STRING   0xd6, 0x05, 0x08, 0x00, 'J', 'L', 'A', 'I', 'S', 'D','K'
static const char user_tag_string[] = {EIR_TAG_STRING};

static u8 llsync_adv_data[ADV_RSP_PACKET_MAX];//max is 31
static u8 llsync_scan_rsp_data[ADV_RSP_PACKET_MAX];//max is 31
static llsync_con_handle;
static adv_cfg_t llsync_server_adv_config;
//-------------------------------------------------------------------------------------
static uint16_t llsync_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
static int llsync_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
static int llsync_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param);

//-------------------------------------------------------------------------------------
//输入passkey 加密
#define PASSKEY_ENABLE                     0

static const sm_cfg_t llsync_sm_init_config = {
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

const gatt_server_cfg_t llsync_server_init_cfg = {
    .att_read_cb = &llsync_att_read_callback,
    .att_write_cb = &llsync_att_write_callback,
    .event_packet_handler = &llsync_event_packet_handler,
};

static gatt_ctrl_t llsync_gatt_control_block = {
    //public
    .mtu_size = 517,
    .cbuffer_size = 1024,
    .multi_dev_flag	= 0,

    //config
#if CONFIG_BT_GATT_SERVER_NUM
    .server_config = &llsync_server_init_cfg,
#else
    .server_config = NULL,
#endif

    .client_config = NULL,

#if CONFIG_BT_SM_SUPPORT_ENABLE
    .sm_config = &llsync_sm_init_config,
#else
    .sm_config = NULL,
#endif
    //cbk,event handle
    .hci_cb_packet_handler = NULL,
};

//-------------------------------------------------------------------------------------
static int llsync_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param)
{
    log_info("event: %02x,size= %d\n", event, size);

    switch (event) {
    case GATT_COMM_EVENT_CONNECTION_COMPLETE:
        log_info("connection_handle:%04x\n", little_endian_read_16(packet, 0));
        log_info("peer_address_info:");
        put_buf(&ext_param[7], 7);

        llsync_con_handle = little_endian_read_16(packet, 0);
        llsync_connection_update_enable = 1;

        log_info("llsync_ble_connection_handler\n");
        break;

    case GATT_COMM_EVENT_DISCONNECT_COMPLETE:
        log_info("disconnect_handle:%04x,reason= %02x\n", little_endian_read_16(packet, 0), packet[2]);
        llsync_con_handle = 0;
        ble_qiot_advertising_start();
        ble_module_enable(1);
        log_info("llsync_ble_disconnection_handler\n");
        break;

    case GATT_COMM_EVENT_ENCRYPTION_CHANGE:
        log_info("ENCRYPTION_CHANGE:handle=%04x,state=%d,process =%d", little_endian_read_16(packet, 0), packet[2], packet[3]);

        break;

    case GATT_COMM_EVENT_CONNECTION_UPDATE_COMPLETE:
        log_info("conn_param update_complete:%04x\n", little_endian_read_16(packet, 0));
        break;

    case GATT_COMM_EVENT_CAN_SEND_NOW:
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
static uint16_t llsync_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
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

static void llsync_send_connetion_updata_deal(u16 conn_handle)
{
    if (llsync_connection_update_enable) {
        if (0 == ble_gatt_server_connetion_update_request(conn_handle, llsync_connection_param_table, CONN_PARAM_TABLE_CNT)) {
            llsync_connection_update_enable = 0;
        }
    }
}

/*
 * @section ATT Write
 */

/* LISTING_START(attWrite): ATT Write */
static int llsync_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    int result = 0;
    u16 tmp16;
    u8 *tmp_buf = NULL;
    u16 handle = att_handle;

    log_info("write_callback,conn_handle =%04x, handle =%04x,size =%d\n", connection_handle, handle, buffer_size);

    switch (handle) {
    case ATT_CHARACTERISTIC_0000FFE3_65D0_4E20_B56A_E493541BA4E2_01_CLIENT_CONFIGURATION_HANDLE:
        llsync_send_connetion_updata_deal(connection_handle);
        log_info("\n------write ccc:%04x,%02x\n", handle, buffer[0]);
        ble_gatt_server_characteristic_ccc_set(connection_handle, handle, buffer[0]);
        break;

    case ATT_CHARACTERISTIC_0000FFE1_65D0_4E20_B56A_E493541BA4E2_01_VALUE_HANDLE:
        log_info("llsync device info handler\n");
#if LL_TECENT_SUPPORT_TASK
        tmp_buf = malloc(buffer_size +  LL_PACKET_HEAD_LEN);
        memcpy(tmp_buf + LL_PACKET_HEAD_LEN, buffer, buffer_size);
        ((LL_PACKET_HEAD_T *)tmp_buf)->packet_channel = LL_DEVICE_INFO_MSG_CH;
        ((LL_PACKET_HEAD_T *)tmp_buf)->len = buffer_size;
        tecent_ll_packet_recieve(tmp_buf, buffer_size +  LL_PACKET_HEAD_LEN);
        free(tmp_buf);
#else
        result = ble_device_info_msg_handle(buffer, buffer_size);
        if (result) {
            log_info("llsync device info handler error:%d\n", result);
        }
#endif
        break;

    case ATT_CHARACTERISTIC_0000FFE2_65D0_4E20_B56A_E493541BA4E2_01_VALUE_HANDLE:
        log_info("llsync data msg handler\n");
#if LL_TECENT_SUPPORT_TASK
        tmp_buf = malloc(buffer_size +  LL_PACKET_HEAD_LEN);
        memcpy(tmp_buf + LL_PACKET_HEAD_LEN, buffer, buffer_size);
        ((LL_PACKET_HEAD_T *)tmp_buf)->packet_channel = LL_DATA_MSG_CH;
        ((LL_PACKET_HEAD_T *)tmp_buf)->len = buffer_size;
        tecent_ll_packet_recieve(tmp_buf, buffer_size +  LL_PACKET_HEAD_LEN);
        free(tmp_buf);
#else
        result = ble_lldata_msg_handle(buffer, buffer_size);
        if (result) {
            log_info("llsync data msg handler error:%d\n", result);
        }
#endif
        break;

    case ATT_CHARACTERISTIC_0000FFE3_65D0_4E20_B56A_E493541BA4E2_01_VALUE_HANDLE:
        log_info("llsync notify msg \n");
        break;

    case ATT_CHARACTERISTIC_0000FFE4_65D0_4E20_B56A_E493541BA4E2_01_VALUE_HANDLE:
        log_info("llsync ota msg handler");
#if LL_TECENT_SUPPORT_TASK
        tmp_buf = malloc(buffer_size +  LL_PACKET_HEAD_LEN);
        memcpy(tmp_buf + LL_PACKET_HEAD_LEN, buffer, buffer_size);
        ((LL_PACKET_HEAD_T *)tmp_buf)->packet_channel = LL_OTA_MSG_CH;
        ((LL_PACKET_HEAD_T *)tmp_buf)->len = buffer_size;
        tecent_ll_packet_recieve(tmp_buf, buffer_size +  LL_PACKET_HEAD_LEN);
        free(tmp_buf);
#else
        result = ble_ota_msg_handle(buffer, buffer_size);
        if (result) {
            log_info("llsync ota msg handler error:%d\n", result);
        }
#endif
        break;

    default:
        log_info("\n\nunknow handle:%d\n\n", handle);
        break;
    }
    return 0;
}

static void app_set_adv_data(u8 *adv_data, u8 adv_len)
{
    log_info("app_set_adv_data");
    llsync_server_adv_config.adv_data_len = adv_len;
    llsync_server_adv_config.adv_data = adv_data;
    put_buf(llsync_server_adv_config.adv_data, llsync_server_adv_config.adv_data_len);
}

static void app_set_rsp_data(u8 *rsp_data, u8 rsp_len)
{
    log_info("app_set_rsp_data");
    llsync_server_adv_config.rsp_data_len = rsp_len;
    llsync_server_adv_config.rsp_data = rsp_data;
    put_buf(llsync_server_adv_config.rsp_data, llsync_server_adv_config.rsp_data_len);
}

//广播参数设置
void llsync_adv_config_set(void)
{
    int ret = 0;

    llsync_server_adv_config.adv_interval = 800;
    llsync_server_adv_config.adv_auto_do = 1;
    llsync_server_adv_config.adv_type = ADV_IND;
    llsync_server_adv_config.adv_channel = ADV_CHANNEL_ALL;
    memset(llsync_server_adv_config.direct_address_info, 0, 7);

    if (ret) {
        log_info("adv_setup_init fail!!!\n");
        return;
    }
    ble_gatt_server_set_adv_config(&llsync_server_adv_config);
}

void llsync_server_init(void)
{
    log_info("%s", __FUNCTION__);
    ble_gatt_server_set_profile(llsync_profile_data, sizeof(llsync_profile_data));
    llsync_adv_config_set();
}

int llsync_app_send_user_data_do(void *priv, u8 *data, u16 len)
{
    printf("llsync_app_send_user_data_do");
#if PRINT_DMA_DATA_EN
    if (len < 128) {
        log_info("-le_tx(%d):");
        log_info_hexdump(data, len);
    } else {
        putchar('L');
    }
#endif
    return ble_comm_att_send_data(llsync_con_handle, ATT_CHARACTERISTIC_0000FFE3_65D0_4E20_B56A_E493541BA4E2_01_VALUE_HANDLE, data, len, ATT_OP_AUTO_READ_CCC);
}

void llsync_func_register()
{
    llsync_ble_module_enable_register(ble_module_enable);
    llsync_send_data_register(llsync_app_send_user_data_do);
    app_set_adv_data_register(app_set_adv_data);
    app_set_rsp_data_register(app_set_rsp_data);
}

void bt_ble_before_start_init(void)
{
    log_info("%s", __FUNCTION__);
    ble_comm_init(&llsync_gatt_control_block);
}

void tecent_ll_init()
{
#if LL_TECENT_SUPPORT_TASK
    tecent_ll_task_init();
#endif
    llsync_dev_info_get();
    ll_sync_init();
}

void bt_ble_init(void)
{
    log_info("%s\n", __FUNCTION__);
    log_info("ble_file: %s", __FILE__);
    llsync_func_register();
#if DOUBLE_BT_SAME_NAME
    ble_comm_set_config_name(bt_get_local_name(), 0);
#else
    ble_comm_set_config_name(bt_get_local_name(), 1);
#endif
    llsync_con_handle = 0;
    tecent_ll_init();
    llsync_server_init();
    ble_module_enable(1);
    ble_qiot_advertising_start();
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
    ble_comm_disconnect(llsync_con_handle);
}

#if 0
static const struct ble_server_operation_t llsync_ble_operation = {
    .disconnect = ble_disconnect,
    .send_data = (void *)llsync_app_send_user_data_do,
};

void llsync_ble_get_server_operation_table(struct ble_server_operation_t **interface_pt)
{
    *interface_pt = (void *)&llsync_ble_operation;
}
#endif

#endif


