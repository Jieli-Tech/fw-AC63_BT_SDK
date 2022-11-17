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
#include "electrocar.h"
#include "electrocar_le_trans.h"
#include "electrocar_le_trans_profile.h"

#if CONFIG_APP_ELECTROCAR

#if LE_DEBUG_PRINT_EN
#define log_info(x, ...)  printf("[ELECTROCAR_LE_TRANS]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif


//测试NRF连接,工具不会主动发起交换流程,需要手动操作; 但设备可配置主动发起MTU长度交换请求
#define ATT_MTU_REQUEST_ENALBE     0    /*配置1,就是设备端主动发起交换*/

//ATT发送的包长,    note: 23 <=need >= MTU
#define ATT_LOCAL_MTU_SIZE        (512) /*一般是主机发起交换,如果主机没有发起,设备端也可以主动发起(ATT_MTU_REQUEST_ENALBE set 1)*/

//ATT缓存的buffer支持缓存数据包个数
#define ATT_PACKET_NUMS_MAX       (2)

//ATT缓存的buffer大小,  note: need >= 23,可修改
#define ATT_SEND_CBUF_SIZE        (ATT_PACKET_NUMS_MAX * (ATT_PACKET_HEAD_SIZE + ATT_LOCAL_MTU_SIZE))

// 广播周期 (unit:0.625ms)
#define ADV_INTERVAL_MIN          48//(160 * 5)//

#define TEST_TRANS_CHANNEL_DATA      0 /*测试记录收发数据速度*/
#define TEST_TRANS_NOTIFY_HANDLE     ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE /*主动发送hanlde,为空则不测试发数*/
#define TEST_TRANS_TIMER_MS          500
#define TEST_PAYLOAD_LEN            (256)

static u32 trans_recieve_test_count;
static u32 trans_send_test_count;

//---------------
//连接参数更新请求设置
//是否使能参数请求更新,0--disable, 1--enable
static uint8_t trans_connection_update_enable = 1; ///0--disable, 1--enable
//当前请求的参数表index
//参数表
static const struct conn_update_param_t trans_connection_param_table[] = {
#if 1
    {16, 24, 0, 600},//11
    {12, 28, 0, 600},//3.7
    {8,  20, 0, 600},
#else
    {16, 24, 10, 600},//11
    {12, 28, 10, 600},//3.7
    {8,  20, 10, 600},
#endif
};

//共可用的参数组数
#define CONN_PARAM_TABLE_CNT      (sizeof(trans_connection_param_table)/sizeof(struct conn_update_param_t))

#define EIR_TAG_STRING   0xd6, 0x05, 0x08, 0x00, 'J', 'L', 'A', 'I', 'S', 'D','K'
static const char user_tag_string[] = {EIR_TAG_STRING};

static u8  trans_adv_data[ADV_RSP_PACKET_MAX];//max is 31
static u8  trans_scan_rsp_data[ADV_RSP_PACKET_MAX];//max is 31
static u8  trans_test_read_write_buf[ELECTROCAR_DATA_LONG];
static u16 trans_con_handle;
static adv_cfg_t trans_server_adv_config;
//-------------------------------------------------------------------------------------
static uint16_t trans_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
static int trans_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
static int trans_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param);
extern void uart_db_regiest_recieve_callback(void *rx_cb);
//-------------------------------------------------------------------------------------
//输入passkey 加密
#define PASSKEY_ENABLE                     0

static const sm_cfg_t trans_sm_init_config = {
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

const gatt_server_cfg_t trans_server_init_cfg = {
    .att_read_cb = &trans_att_read_callback,
    .att_write_cb = &trans_att_write_callback,
    .event_packet_handler = &trans_event_packet_handler,
};

static gatt_ctrl_t trans_gatt_control_block = {
    //public
    .mtu_size = ATT_LOCAL_MTU_SIZE,
    .cbuffer_size = ATT_SEND_CBUF_SIZE,
    .multi_dev_flag	= 0,

    //config
#if CONFIG_BT_GATT_SERVER_NUM
    .server_config = &trans_server_init_cfg,
#else
    .server_config = NULL,
#endif

    .client_config = NULL,

#if CONFIG_BT_SM_SUPPORT_ENABLE
    .sm_config = &trans_sm_init_config,
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

/*************************************************************************************************/
/*!
 *  \brief      测试上传文件
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
#define AUDIO_ONE_PACKET_LEN  128
static void trans_test_send_audio_data(int init_flag)
{
    static u32 send_pt = 0;
    static u32 start_flag = 0;

    if (!trans_con_handle) {
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
        if (ble_comm_cbuffer_vaild_len(trans_con_handle) > send_len) {
            log_info("audio send %08x\n", send_pt);
            if (ble_comm_att_send_data(trans_con_handle, ATT_CHARACTERISTIC_ae3c_01_VALUE_HANDLE, &file_ptr[send_pt], send_len, ATT_OP_AUTO_READ_CCC)) {
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

/* static void led_test_reflash_handle(void) */
/* { */
/*     if(trans_test_read_write_buf[0]) */
/*     { */
/*         gpio_set_output_value(LOCK_MOTOR_PIN,1); */
/*         log_info("===led1 on"); */
/*     } */
/*     else */
/*     { */
/*         gpio_set_output_value(LOCK_MOTOR_PIN,0); */
/*         log_info("===led1 off"); */
/*     } */
/*  */
/*     if(trans_test_read_write_buf[1]) */
/*     { */
/*         gpio_set_output_value(LOCK_LED_PIN,1); */
/*         log_info("===led2 on"); */
/*     } */
/*     else */
/*     { */
/*         gpio_set_output_value(LOCK_LED_PIN,0); */
/*         log_info("===led2 off"); */
/*     } */
/* } */
/*************************************************************************************************/
/*!
 *  \brief      串口接收转发到BLE
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void trans_uart_rx_to_ble(u8 *packet, u32 size)
{
    if (trans_con_handle && ble_comm_att_check_send(trans_con_handle, size) &&
        ble_gatt_server_characteristic_ccc_get(trans_con_handle, ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE)) {
        ble_comm_att_send_data(trans_con_handle, ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE, packet, size, ATT_OP_AUTO_READ_CCC);
    } else {
        log_info("drop uart data!!!\n");
    }
}

/*************************************************************************************************/
/*!
 *  \brief      发送请求连接参数表
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void trans_send_connetion_updata_deal(u16 conn_handle)
{
    if (trans_connection_update_enable) {
        if (0 == ble_gatt_server_connetion_update_request(conn_handle, trans_connection_param_table, CONN_PARAM_TABLE_CNT)) {
            trans_connection_update_enable = 0;
        }
    }
}

/*************************************************************************************************/
/*!
 *  \brief      回连状态，使能所有profile
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note      配对绑定的方式，主机回连不是在使能server的通知开关，需要自己打开
 */
/*************************************************************************************************/
static void trans_resume_all_ccc_enable(u16 conn_handle, u8 update_request)
{
    log_info("resume_all_ccc_enable\n");

#if RCSP_BTMATE_EN
    ble_gatt_server_characteristic_ccc_set(conn_handle, ATT_CHARACTERISTIC_ae02_02_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
#endif
    ble_gatt_server_characteristic_ccc_set(conn_handle, ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    ble_gatt_server_characteristic_ccc_set(conn_handle, ATT_CHARACTERISTIC_ae04_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    ble_gatt_server_characteristic_ccc_set(conn_handle, ATT_CHARACTERISTIC_ae05_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_INDICATE);
    ble_gatt_server_characteristic_ccc_set(conn_handle, ATT_CHARACTERISTIC_ae3c_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);

    if (update_request) {
        trans_send_connetion_updata_deal(conn_handle);
    }
}


/*************************************************************************************************/
/*!
 *  \brief      处理gatt 返回的事件（hci && gatt）
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int trans_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param)
{
    /* log_info("event: %02x,size= %d\n",event,size); */

    switch (event) {

    case GATT_COMM_EVENT_CAN_SEND_NOW:
#if TEST_AUDIO_DATA_UPLOAD
        trans_test_send_audio_data(0);
#endif
        break;

    case GATT_COMM_EVENT_SERVER_INDICATION_COMPLETE:
        log_info("INDICATION_COMPLETE:con_handle= %04x,att_handle= %04x\n", \
                 little_endian_read_16(packet, 0), little_endian_read_16(packet, 2));
        break;


    case GATT_COMM_EVENT_CONNECTION_COMPLETE:
        trans_con_handle = little_endian_read_16(packet, 0);
        trans_connection_update_enable = 1;

        log_info("connection_handle:%04x\n", little_endian_read_16(packet, 0));
        log_info("connection_handle:%04x, rssi= %d\n", trans_con_handle, ble_vendor_get_peer_rssi(trans_con_handle));
        log_info("peer_address_info:");
        put_buf(&ext_param[7], 7);

        log_info("con_interval = %d\n", little_endian_read_16(ext_param, 14 + 0));
        log_info("con_latency = %d\n", little_endian_read_16(ext_param, 14 + 2));
        log_info("cnn_timeout = %d\n", little_endian_read_16(ext_param, 14 + 4));

#if ATT_MTU_REQUEST_ENALBE
        att_server_set_exchange_mtu(trans_con_handle);/*主动请求MTU长度交换*/
#endif

#if TCFG_UART0_RX_PORT != NO_CONFIG_PORT
        //for test 串口数据直通到蓝牙
        uart_db_regiest_recieve_callback(trans_uart_rx_to_ble);
#endif
        break;

    case GATT_COMM_EVENT_DISCONNECT_COMPLETE:
        log_info("disconnect_handle:%04x,reason= %02x\n", little_endian_read_16(packet, 0), packet[2]);
        if (trans_con_handle == little_endian_read_16(packet, 0)) {
            trans_con_handle = 0;
        }
        break;

    case GATT_COMM_EVENT_ENCRYPTION_CHANGE:
        log_info("ENCRYPTION_CHANGE:handle=%04x,state=%d,process =%d", little_endian_read_16(packet, 0), packet[2], packet[3]);
        if (packet[3] == LINK_ENCRYPTION_RECONNECT) {
            trans_resume_all_ccc_enable(little_endian_read_16(packet, 0), 1);
        }
        break;

    case GATT_COMM_EVENT_CONNECTION_UPDATE_COMPLETE:
        log_info("conn_param update_complete:%04x\n", little_endian_read_16(packet, 0));
        log_info("update_interval = %d\n", little_endian_read_16(ext_param, 6 + 0));
        log_info("update_latency = %d\n", little_endian_read_16(ext_param, 6 + 2));
        log_info("update_timeout = %d\n", little_endian_read_16(ext_param, 6 + 4));
        break;

    case GATT_COMM_EVENT_CONNECTION_UPDATE_REQUEST_RESULT:
    case GATT_COMM_EVENT_MTU_EXCHANGE_COMPLETE:
        break;

    case GATT_COMM_EVENT_SERVER_STATE:
        log_info("server_state: handle=%02x,%02x\n", little_endian_read_16(packet, 1), packet[0]);
        break;

    case GATT_COMM_EVENT_SM_PASSKEY_INPUT: {
        u32 *key = little_endian_read_32(packet, 2);
        *key = 888888;
        r_printf("input_key:%6u\n", *key);
    }
    break;

    default:
        break;
    }
    return 0;
}


/*************************************************************************************************/
/*!
 *  \brief      处理client 读操作
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note      profile的读属性uuid 有配置 DYNAMIC 关键字，就有read_callback 回调
 */
/*************************************************************************************************/
// ATT Client Read Callback for Dynamic Data
// - if buffer == NULL, don't copy data, just return size of value
// - if buffer != NULL, copy data and return number bytes copied
// @param con_handle of hci le connection
// @param attribute_handle to be read
// @param offset defines start of attribute value
// @param buffer
// @param buffer_size
static uint16_t trans_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    uint16_t  att_value_len = 0;
    uint16_t handle = att_handle;

    log_info("read_callback,conn_handle =%04x, handle=%04x,buffer=%08x\n", connection_handle, handle, (u32)buffer);

    switch (handle) {
    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE: {
        char *gap_name = ble_comm_get_gap_name();
        att_value_len = strlen(gap_name);

        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }

        if (buffer) {
            memcpy(buffer, &gap_name[offset], buffer_size);
            att_value_len = buffer_size;
            log_info("\n------read gap_name: %s\n", gap_name);
        }
    }
    break;

    case ATT_CHARACTERISTIC_ae10_01_VALUE_HANDLE:
        att_value_len = sizeof(trans_test_read_write_buf);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }

        if (buffer) {
            memcpy(buffer, &trans_test_read_write_buf[offset], buffer_size);
            att_value_len = buffer_size;
        }
        trans_send_connetion_updata_deal(connection_handle);
        break;

    case ATT_CHARACTERISTIC_ae04_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae05_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae3c_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE:
        if (buffer) {
            buffer[0] = ble_gatt_server_characteristic_ccc_get(connection_handle, handle);
            buffer[1] = 0;
        }
        att_value_len = 2;
        break;

    default:
        break;
    }

    log_info("att_value_len= %d\n", att_value_len);
    return att_value_len;
}


/*************************************************************************************************/
/*!
 *  \brief      处理client write操作
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note      profile的写属性uuid 有配置 DYNAMIC 关键字，就有write_callback 回调
 */
/*************************************************************************************************/
// ATT Client Write Callback for Dynamic Data
// @param con_handle of hci le connection
// @param attribute_handle to be written
// @param transaction - ATT_TRANSACTION_MODE_NONE for regular writes, ATT_TRANSACTION_MODE_ACTIVE for prepared writes and ATT_TRANSACTION_MODE_EXECUTE
// @param offset into the value - used for queued writes and long attributes
// @param buffer
// @param buffer_size
// @param signature used for signed write commmands
// @returns 0 if write was ok, ATT_ERROR_PREPARE_QUEUE_FULL if no space in queue, ATT_ERROR_INVALID_OFFSET if offset is larger than max buffer

static int trans_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    int result = 0;
    u16 tmp16;

    u16 handle = att_handle;

#if !TEST_TRANS_CHANNEL_DATA
    log_info("write_callback,conn_handle =%04x, handle =%04x,size =%d\n", connection_handle, handle, buffer_size);
#endif

    switch (handle) {

    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        break;

    case ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae04_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae05_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae3c_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE:
        trans_send_connetion_updata_deal(connection_handle);
        log_info("\n------write ccc:%04x,%02x\n", handle, buffer[0]);
        ble_gatt_server_characteristic_ccc_set(connection_handle, handle, buffer[0]);
        break;

    case ATT_CHARACTERISTIC_ae10_01_VALUE_HANDLE:
        tmp16 = sizeof(trans_test_read_write_buf);
        if ((offset >= tmp16) || (offset + buffer_size) > tmp16) {
            break;
        }

        log_info("cur_state:");
        put_buf(trans_test_read_write_buf, tmp16);

        memcpy(&trans_test_read_write_buf[offset], buffer, buffer_size);

        log_info("new_state:");
        put_buf(trans_test_read_write_buf, tmp16);

        /* led_test_reflash_handle(); */
        ble_transfer_channel_recieve(trans_test_read_write_buf, tmp16);
        trans_send_connetion_updata_deal(connection_handle);
        break;

    case ATT_CHARACTERISTIC_ae01_01_VALUE_HANDLE:

#if TEST_TRANS_CHANNEL_DATA
        /* putchar('R'); */
        trans_recieve_test_count += buffer_size;
        break;
#endif

        log_info("\n-ae01_rx(%d):", buffer_size);
        put_buf(buffer, buffer_size);

        //收发测试，自动发送收到的数据;for test
        if (ble_comm_att_check_send(connection_handle, buffer_size) &&
            ble_gatt_server_characteristic_ccc_get(trans_con_handle, ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE)) {
            log_info("-loop send1\n");
            ble_comm_att_send_data(connection_handle, ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE, buffer, buffer_size, ATT_OP_AUTO_READ_CCC);
        }
        break;

    case ATT_CHARACTERISTIC_ae03_01_VALUE_HANDLE:
        log_info("\n-ae03_rx(%d):", buffer_size);
        put_buf(buffer, buffer_size);

        //收发测试，自动发送收到的数据;for test
        if (ble_comm_att_check_send(connection_handle, buffer_size) && \
            ble_gatt_server_characteristic_ccc_get(trans_con_handle, ATT_CHARACTERISTIC_ae05_01_CLIENT_CONFIGURATION_HANDLE)) {
            log_info("-loop send2\n");
            ble_comm_att_send_data(connection_handle, ATT_CHARACTERISTIC_ae05_01_VALUE_HANDLE, buffer, buffer_size, ATT_OP_AUTO_READ_CCC);
        }
        break;

#if RCSP_BTMATE_EN
    case ATT_CHARACTERISTIC_ae02_02_CLIENT_CONFIGURATION_HANDLE:
        ble_op_latency_skip(connection_handle, 0xffff); //
        ble_gatt_server_set_update_send(connection_handle, ATT_CHARACTERISTIC_ae02_02_VALUE_HANDLE, ATT_OP_AUTO_READ_CCC);
#endif
        /* trans_send_connetion_updata_deal(connection_handle); */
        log_info("------write ccc:%04x,%02x\n", handle, buffer[0]);
        ble_gatt_server_characteristic_ccc_set(connection_handle, handle, buffer[0]);
        break;

#if RCSP_BTMATE_EN
    case ATT_CHARACTERISTIC_ae01_02_VALUE_HANDLE:
        log_info("rcsp_read:%x\n", buffer_size);
        ble_gatt_server_receive_update_data(NULL, buffer, buffer_size);
        break;
#endif

    case ATT_CHARACTERISTIC_ae3b_01_VALUE_HANDLE:
        log_info("\n-ae3b_rx(%d):", buffer_size);
        put_buf(buffer, buffer_size);

#if TEST_AUDIO_DATA_UPLOAD
        if (0 == memcmp(buffer, "start", 5)) {
            trans_test_send_audio_data(1);
        }
#endif
        break;

    default:
        break;
    }
    return 0;
}


/*************************************************************************************************/
/*!
 *  \brief      组织adv包数据，放入buff
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static u8  adv_name_ok = 0;//name 优先存放在ADV包
static int trans_make_set_adv_data(void)
{
    u8 offset = 0;
    u8 *buf = trans_adv_data;

#if DOUBLE_BT_SAME_MAC
    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, FLAGS_GENERAL_DISCOVERABLE_MODE | FLAGS_LE_AND_EDR_SAME_CONTROLLER, 1);
#else
    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, FLAGS_GENERAL_DISCOVERABLE_MODE | FLAGS_EDR_NOT_SUPPORTED, 1);
#endif

    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_16BIT_SERVICE_UUIDS, 0xAF30, 2);

    char *gap_name = ble_comm_get_gap_name();
    u8 name_len = strlen(gap_name);
    u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
    if (name_len < vaild_len) {
        offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_name, name_len);
        adv_name_ok = 1;
    } else {
        adv_name_ok = 0;
    }

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***trans_adv_data overflow!!!!!!\n");
        return -1;
    }
    log_info("trans_adv_data(%d):", offset);
    log_info_hexdump(buf, offset);
    trans_server_adv_config.adv_data_len = offset;
    trans_server_adv_config.adv_data = trans_adv_data;
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      组织rsp包数据，放入buff
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int trans_make_set_rsp_data(void)
{
    u8 offset = 0;
    u8 *buf = trans_scan_rsp_data;

#if RCSP_BTMATE_EN
    u8  tag_len = sizeof(user_tag_string);
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA, (void *)user_tag_string, tag_len);
#endif

    if (!adv_name_ok) {
        char *gap_name = ble_comm_get_gap_name();
        u8 name_len = strlen(gap_name);
        u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
        if (name_len > vaild_len) {
            name_len = vaild_len;
        }
        offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_name, name_len);
    }

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***rsp_data overflow!!!!!!\n");
        return -1;
    }

    log_info("rsp_data(%d):", offset);
    log_info_hexdump(buf, offset);
    trans_server_adv_config.rsp_data_len = offset;
    trans_server_adv_config.rsp_data = trans_scan_rsp_data;
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      配置广播参数
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note      开广播前配置都有效
 */
/*************************************************************************************************/
static void trans_adv_config_set(void)
{
    int ret = 0;
    ret |= trans_make_set_adv_data();
    ret |= trans_make_set_rsp_data();

    trans_server_adv_config.adv_interval = ADV_INTERVAL_MIN;
    trans_server_adv_config.adv_auto_do = 1;
    trans_server_adv_config.adv_type = ADV_IND;
    trans_server_adv_config.adv_channel = ADV_CHANNEL_ALL;
    memset(trans_server_adv_config.direct_address_info, 0, 7);

    if (ret) {
        log_info("adv_setup_init fail!!!\n");
        return;
    }
    ble_gatt_server_set_adv_config(&trans_server_adv_config);
}

/*************************************************************************************************/
/*!
 *  \brief      server init初始化
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void trans_server_init(void)
{
    log_info("%s", __FUNCTION__);
    ble_gatt_server_set_profile(trans_profile_data, sizeof(trans_profile_data));
    trans_adv_config_set();
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
void trans_disconnect(void)
{
    log_info("%s", __FUNCTION__);
    if (trans_con_handle) {
        ble_comm_disconnect(trans_con_handle);
    }
}


/*************************************************************************************************/
/*!
 *  \brief      协议栈初始化前调用
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/

void bt_ble_before_start_init(void)
{
    log_info("%s", __FUNCTION__);
    ble_comm_init(&trans_gatt_control_block);
}


/*************************************************************************************************/
/*!
 *  \brief
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void trans_test_send_data(void)
{
#if TEST_TRANS_CHANNEL_DATA
    static u32 count = 0;
    static u32 send_index;

    int i, ret = 0;
    int send_len = TEST_PAYLOAD_LEN;
    u32 time_index_max = 1000 / TEST_TRANS_TIMER_MS;

    if (!trans_con_handle) {
        return;
    }

    send_index++;

#if TEST_TRANS_NOTIFY_HANDLE
    count++;
    if (ble_comm_att_check_send(trans_con_handle, send_len) && ble_gatt_server_characteristic_ccc_get(trans_con_handle, TEST_TRANS_NOTIFY_HANDLE + 1)) {
        ret = ble_comm_att_send_data(trans_con_handle, TEST_TRANS_NOTIFY_HANDLE, &count, send_len, ATT_OP_AUTO_READ_CCC);
        if (!ret) {
            /* putchar('T'); */
            trans_send_test_count += send_len;
        }
    }
#endif

    if (send_index >= time_index_max) {
        if (trans_send_test_count) {
            log_info(">>>>>> send_rate= %d byte/s\n", trans_send_test_count);
        }
        send_index = 0;
        trans_send_test_count = 0;

        if (trans_recieve_test_count) {
            log_info("<<<<<<< recieve_rate= %d byte/s\n", trans_recieve_test_count);
            trans_recieve_test_count = 0;
        }
    }
#endif
}



/*************************************************************************************************/
/*!
 *  \brief      模块初始化
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void bt_ble_init(void)
{
    log_info("%s\n", __FUNCTION__);
    log_info("ble_file: %s", __FILE__);

#if DOUBLE_BT_SAME_NAME
    ble_comm_set_config_name(bt_get_local_name(), 0);
#else
    //ble_comm_set_config_name(bt_get_local_name(), 1);
    ble_comm_set_config_name("Car-Demo", 0);
#endif
    trans_con_handle = 0;
    trans_server_init();
    ble_module_enable(1);

#if TEST_TRANS_CHANNEL_DATA
    if (TEST_TRANS_TIMER_MS < 10) {
        sys_hi_timer_add(0, trans_test_send_data, TEST_TRANS_TIMER_MS);
    } else {
        sys_timer_add(0, trans_test_send_data, TEST_TRANS_TIMER_MS);
    }
#endif

    gpio_set_direction(LOCK_MOTOR_PIN, 0);
    gpio_set_direction(LOCK_LED_PIN, 0);
    /* led_test_reflash_handle(); */

}

/*************************************************************************************************/
/*!
 *  \brief      模块退出
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void bt_ble_exit(void)
{
    log_info("%s\n", __FUNCTION__);
    ble_module_enable(0);
    ble_comm_exit();
}

/*************************************************************************************************/
/*!
 *  \brief      模块开发使能
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_module_enable(u8 en)
{
    ble_comm_module_enable(en);
}

/*************************************************************************************************/
/*!
 *  \brief      testbox 按键测试
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_server_send_test_key_num(u8 key_num)
{
    if (trans_con_handle) {
        if (get_remote_test_flag()) {
            ble_op_test_key_num(trans_con_handle, key_num);
        } else {
            log_info("-not conn testbox\n");
        }
    }
}


#endif



