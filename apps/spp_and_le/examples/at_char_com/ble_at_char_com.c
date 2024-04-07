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
#include "3th_profile_api.h"

#define   BLE_PROFILE_DATA
#include "le_common.h"
#include "at.h"
#include "ble_at_char_com.h"
#include "ble_at_char_profile.h"

#include "rcsp_bluetooth.h"
#include "JL_rcsp_api.h"
#include "custom_cfg.h"

#if  CONFIG_APP_AT_CHAR_COM

#if LE_DEBUG_PRINT_EN
extern void printf_buf(u8 *buf, u32 len);
//#define log_info          printf
#define log_info(x, ...)    printf("\n[LE_AT_CHAR_COM]" x " ", ## __VA_ARGS__)

#define log_info_hexdump  printf_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif


/* #define LOG_TAG_CONST       BT_BLE */
/* #define LOG_TAG             "[LE_S_DEMO]" */
/* #define LOG_ERROR_ENABLE */
/* #define LOG_DEBUG_ENABLE */
/* #define LOG_INFO_ENABLE */
/* #define LOG_DUMP_ENABLE */
/* #define LOG_CLI_ENABLE */
/* #include "debug.h" */

//------
#define ATT_LOCAL_MTU_SIZE        (247)                   //note: need >= 20
#define ATT_SEND_CBUF_SIZE        (512*2)                   //note: need >= 20,缓存大小，可修改
#define ATT_RAM_BUFSIZE           (ATT_CTRL_BLOCK_SIZE + ATT_LOCAL_MTU_SIZE + ATT_SEND_CBUF_SIZE)                   //note:
static u8 att_ram_buffer[ATT_RAM_BUFSIZE] __attribute__((aligned(4)));
//---------------

//---------------
#define ADV_INTERVAL_MIN          (2048)

#define HOLD_LATENCY_CNT_MIN  (3)  //(0~0xffff)
#define HOLD_LATENCY_CNT_MAX  (15) //(0~0xffff)
#define HOLD_LATENCY_CNT_ALL  (0xffff)

static volatile hci_con_handle_t con_handle;

//加密设置
/* static const uint8_t sm_min_key_size = 7; */
//1-just_works,2--passkey,3--yes or no
static uint8_t sm_pair_mode;

//连接参数设置
static const uint8_t connection_update_enable = 0; ///0--disable, 1--enable
static uint8_t connection_update_cnt = 0; //
void ble_test_auto_adv(u8 en);

static const struct conn_update_param_t connection_param_table[] = {
    {16, 24, 10, 600},//11
    {12, 28, 10, 600},//3.7
    {8,  20, 10, 600},
};
#define CONN_PARAM_TABLE_CNT      (sizeof(connection_param_table)/sizeof(struct conn_update_param_t))

#if (ATT_RAM_BUFSIZE < 64)
#error "adv_data & rsp_data buffer error!!!!!!!!!!!!"
#endif

//用户可配对的，这是样机跟客户开发的app配对的秘钥
/* const u8 link_key_data[16] = {0x06, 0x77, 0x5f, 0x87, 0x91, 0x8d, 0xd4, 0x23, 0x00, 0x5d, 0xf1, 0xd8, 0xcf, 0x0c, 0x14, 0x2b}; */
/* #define EIR_TAG_STRING   0xd6, 0x05, 0x08, 0x00, 'J', 'L', 'A', 'I', 'S', 'D','K' */
/* static const char user_tag_string[] = {EIR_TAG_STRING}; */

static u8 adv_data_len = 0;
static u8 adv_data[ADV_RSP_PACKET_MAX];//max is 31
static u8 scan_rsp_data_len = 0;
static u8 scan_rsp_data[ADV_RSP_PACKET_MAX];//max is 31

static const u8 cur_dev_cid = 8; //fixed

static char gap_device_name[BT_NAME_LEN_MAX] = "BR2262e-s";
static u8 gap_device_name_len = 0;
static u8 ble_work_state = 0;
static u8 adv_ctrl_en;
static u8 auto_adv_enable = 0;    //set 1,默认广播配置,上电&断开后开启广播
static u16 adv_interval_value = ADV_INTERVAL_MIN;

static void (*app_recieve_callback)(void *priv, void *buf, u16 len) = NULL;
static void (*app_ble_state_callback)(void *priv, ble_state_e state) = NULL;
static void (*ble_resume_send_wakeup)(void) = NULL;
static u32 channel_priv;
void (*at_send_event_callbak)(u8 event_type, const u8 *packet, int size);

static int app_send_user_data_check(u16 len);
//static int app_send_user_data_do(void *priv, u8 *data, u16 len);
static int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type);
static void ble_at_send_event(u8 opcode, const u8 *packet, int size);

// Complete Local Name  默认的蓝牙名字
//------------------------------------------------------
//广播参数设置
static void advertisements_setup_init();
static int set_adv_enable(void *priv, u32 en);
static int get_buffer_vaild_len(void *priv);
extern const char *bt_get_local_name();
extern void clr_wdt(void);
extern void sys_auto_shut_down_disable(void);
extern void sys_auto_shut_down_enable(void);
extern u8 get_total_connect_dev(void);
//------------------------------------------------------

static void send_request_connect_parameter(u8 table_index)
{
    struct conn_update_param_t *param = (void *)&connection_param_table[table_index];//static ram

    log_info("update_request:-%d-%d-%d-%d-\n", param->interval_min, param->interval_max, param->latency, param->timeout);
    if (con_handle) {
        ble_op_conn_param_request(con_handle, param);
    }
}

static void check_connetion_updata_deal(void)
{
    if (connection_update_enable) {
        if (connection_update_cnt < CONN_PARAM_TABLE_CNT) {
            send_request_connect_parameter(connection_update_cnt);
        }
    }
}

/***
  连接参数更新
 ***/

void slave_connect_param_update(u16 interval_min, u16 interval_max, u16 latency, u16 timeout)
{
    static struct conn_update_param_t param;
    param.interval_min = interval_min;
    param.interval_max = interval_max;
    param.latency = latency;
    param.timeout = timeout;

    log_info("client update param:-%d-%d-%d-%d-\n", param.interval_min, param.interval_max, param.latency, param.timeout);
    if (con_handle) {
        ble_op_conn_param_request(con_handle, &param);
    }
}

static void connection_update_complete_success(u8 *packet)
{
    int con_handle, conn_interval, conn_latency, conn_timeout;

    con_handle = hci_subevent_le_connection_update_complete_get_connection_handle(packet);
    conn_interval = hci_subevent_le_connection_update_complete_get_conn_interval(packet);
    conn_latency = hci_subevent_le_connection_update_complete_get_conn_latency(packet);
    conn_timeout = hci_subevent_le_connection_update_complete_get_supervision_timeout(packet);

    log_info("conn_interval = %d\n", conn_interval);
    log_info("conn_latency = %d\n", conn_latency);
    log_info("conn_timeout = %d\n", conn_timeout);
}

static void set_ble_work_state(ble_state_e state)
{
    if (state != ble_work_state) {
        log_info("ble_work_st:%x->%x\n", ble_work_state, state);
        ble_work_state = state;
        if (app_ble_state_callback) {
            app_ble_state_callback((void *)channel_priv, state);
        }
    }
}

static ble_state_e get_ble_work_state(void)
{
    return ble_work_state;
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
    /* putchar('E'); */
    if (ble_resume_send_wakeup) {
        ble_resume_send_wakeup();
    }
}

static void ble_auto_shut_down_enable(u8 enable)
{
#if TCFG_AUTO_SHUT_DOWN_TIME
    if (enable) {
        if (get_total_connect_dev() == 0) {    //已经没有设备连接
            sys_auto_shut_down_enable();
        }
    } else {
        sys_auto_shut_down_disable();
    }
#endif
}

static const char *const phy_result[] = {
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
    u16 phy_options = CONN_SET_PHY_OPTIONS_S8;

    ble_op_set_ext_phy(con_handle, all_phys, tx_phy, rx_phy, phy_options);
}

static void server_profile_start(u16 con_handle)
{
#if BT_FOR_APP_EN
    set_app_connect_type(TYPE_BLE);
#endif

    ble_op_multi_att_send_conn_handle(con_handle, cur_dev_cid - 8, 0);
    set_ble_work_state(BLE_ST_CONNECT);
    ble_auto_shut_down_enable(0);

    /* set_connection_data_phy(CONN_SET_CODED_PHY, CONN_SET_CODED_PHY); */
}

_WEAK_
u8 ble_update_get_ready_jump_flag(void)
{
    return 0;
}
/*
 * @section Packet Handler
 *
 * @text The packet handler is used to:
 *        - stop the counter after a disconnect
 *        - send a notification when the requested ATT_EVENT_CAN_SEND_NOW is received
 */

/* LISTING_START(packetHandler): Packet Handler */
int client_cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);;
static void cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    int mtu;
    u32 tmp;
    u8 status;
    u8 peer_addr[6] = {0};

#if CONFIG_BT_GATT_CLIENT_NUM
    if (client_cbk_packet_handler(packet_type, channel, packet, size)) {
        return;
    }
#endif

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
                if (0 == hci_subevent_le_connection_complete_get_role(packet)) {
                    //connect is master
                    break;
                }
                con_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                log_info("HCI_SUBEVENT_LE_CONNECTION_COMPLETE: %0x\n", con_handle);
                hci_subevent_le_connection_complete_get_peer_address(packet, peer_addr);
                multi_att_ccc_config_init();

                black_list_check(1, peer_addr);
                connection_update_complete_success(packet + 8);
                server_profile_start(con_handle);
#if RCSP_BTMATE_EN
#if (0 == BT_CONNECTION_VERIFY)
                JL_rcsp_auth_reset();
#endif
                //rcsp_dev_select(RCSP_BLE);
                rcsp_init();
#endif
                break;

            case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                if (con_handle == little_endian_read_16(packet, 4)) {
                    connection_update_complete_success(packet);
                }
                break;

            case HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE:
                log_info("APP HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE\n");
                /* set_connection_data_phy(CONN_SET_CODED_PHY, CONN_SET_CODED_PHY); */
                break;

            case HCI_SUBEVENT_LE_PHY_UPDATE_COMPLETE:
                log_info("APP HCI_SUBEVENT_LE_PHY_UPDATE %s\n", hci_event_le_meta_get_phy_update_complete_status(packet) ? "Fail" : "Succ");
                log_info("Tx PHY: %s\n", phy_result[hci_event_le_meta_get_phy_update_complete_tx_phy(packet)]);
                log_info("Rx PHY: %s\n", phy_result[hci_event_le_meta_get_phy_update_complete_rx_phy(packet)]);
                break;
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            put_buf(packet, size);
            if (con_handle != little_endian_read_16(packet, 3)) {
                break;
            }
            log_info("HCI_EVENT_DISCONNECTION_COMPLETE: %0x\n", packet[5]);
#if RCSP_BTMATE_EN
            rcsp_exit();
#endif
            con_handle = 0;
            ble_op_multi_att_send_conn_handle(con_handle, cur_dev_cid - 8, 0);
            set_ble_work_state(BLE_ST_DISCONN);

            if (auto_adv_enable) {
                if (!ble_update_get_ready_jump_flag()) {
                    bt_ble_adv_enable(1);
                }
            }

            connection_update_cnt = 0;
#if BT_FOR_APP_EN
            set_app_connect_type(TYPE_NULL);
#endif
            ble_auto_shut_down_enable(1);
            at_send_disconnect(cur_dev_cid);
            break;

        case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
            mtu = att_event_mtu_exchange_complete_get_MTU(packet) - 3;
            log_info("ATT MTU = %u\n", mtu);
            ble_op_multi_att_set_send_mtu(con_handle, mtu);
            /* set_connection_data_length(251, 2120); */
            break;

        case HCI_EVENT_VENDOR_REMOTE_TEST:
            log_info("--- HCI_EVENT_VENDOR_REMOTE_TEST\n");
            break;

        case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE:
            tmp = little_endian_read_16(packet, 4);
            log_info("-update_rsp: %02x\n", tmp);
            if (tmp) {
                connection_update_cnt++;
                log_info("remoter reject!!!\n");
                check_connetion_updata_deal();
            } else {
                connection_update_cnt = CONN_PARAM_TABLE_CNT;
            }
            break;

        case HCI_EVENT_ENCRYPTION_CHANGE:
            log_info("HCI_EVENT_ENCRYPTION_CHANGE= %d\n", packet[2]);
            break;
        }
        break;
    }
}


/* LISTING_END */

/*
 * @section ATT Read
 *
 * @text The ATT Server handles all reads to constant data. For dynamic data like the custom characteristic, the registered
 * att_read_callback is called. To handle long characteristics and long reads, the att_read_callback is first called
 * with buffer == NULL, to request the total value length. Then it will be called again requesting a chunk of the value.
 * See Listing attRead.
 */

/* LISTING_START(attRead): ATT Read */

// ATT Client Read Callback for Dynamic Data
// - if buffer == NULL, don't copy data, just return size of value
// - if buffer != NULL, copy data and return number bytes copied
// @param offset defines start of attribute value
static uint16_t att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{

    uint16_t  att_value_len = 0;
    uint16_t handle = att_handle;

    log_info("read_callback, handle= 0x%04x,buffer= %08x\n", handle, (u32)buffer);

    switch (handle) {

    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        att_value_len = gap_device_name_len;
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }

        if (buffer) {
            memcpy(buffer, &gap_device_name[offset], buffer_size);
            att_value_len = buffer_size;
            log_info("------read gap_name: %s \n", gap_device_name);
        }
        break;
    case ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae05_01_CLIENT_CONFIGURATION_HANDLE:
        if (buffer) {
            buffer[0] = multi_att_get_ccc_config(con_handle, handle);
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


/* LISTING_END */
/*
 * @section ATT Write
 *
 * @text The only valid ATT write in this example is to the Client Characteristic Configuration, which configures notification
 * and indication. If the ATT handle matches the client configuration handle, the new configuration value is stored and used
 * in the heartbeat handler to decide if a new value should be sent. See Listing attWrite.
 */

/* LISTING_START(attWrite): ATT Write */
static int att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    int result = 0;
    u16 tmp16;

    u16 handle = att_handle;
    /* log_info("write_callback, handle= 0x%04x,size = %d\n", handle, buffer_size); */

    switch (handle) {

    case ATT_CHARACTERISTIC_ae01_01_VALUE_HANDLE:
        /* printf("\n-ae_rx(%d):", buffer_size); */
        /* printf_buf(buffer, buffer_size); */

        at_send_rx_cid_data(cur_dev_cid, buffer, buffer_size);

        //收发测试，自动发送收到的数据;for test
        /* if (app_send_user_data_check(buffer_size)) { */
        /* app_send_user_data(ATT_CHARACTERISTIC_ae05_01_VALUE_HANDLE, buffer, buffer_size, ATT_OP_AUTO_READ_CCC); */
        /* } */
        break;

    case ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE:
        at_send_string("OK");
        at_send_connected(cur_dev_cid);
        set_ble_work_state(BLE_ST_NOTIFY_IDICATE);
        check_connetion_updata_deal();
    /* send_request_connect_parameter(3); */

    case ATT_CHARACTERISTIC_ae05_01_CLIENT_CONFIGURATION_HANDLE:
        log_info("------write ccc:%04x,%02x\n", handle, buffer[0]);
        multi_att_set_ccc_config(con_handle, handle, buffer[0]);
        break;

#if RCSP_BTMATE_EN
    case ATT_CHARACTERISTIC_ae02_02_CLIENT_CONFIGURATION_HANDLE:
#if (defined(BT_CONNECTION_VERIFY) && (0 == BT_CONNECTION_VERIFY))
        JL_rcsp_auth_reset();               //处理APP断开后台还连接的情况
#endif
        ble_op_latency_skip(con_handle, HOLD_LATENCY_CNT_ALL); //
        set_ble_work_state(BLE_ST_NOTIFY_IDICATE);
#endif
        /* if ((cur_conn_latency == 0) */
        /*     && (connection_update_cnt == CONN_PARAM_TABLE_CNT) */
        /*     && (Peripheral_Preferred_Connection_Parameters[0].latency != 0)) { */
        /*     connection_update_cnt = 0; */
        /* } */
        check_connetion_updata_deal();
        log_info("\n------write ccc:%04x,%02x\n", handle, buffer[0]);
        multi_att_set_ccc_config(con_handle, handle, buffer[0]);
        break;

#if RCSP_BTMATE_EN
    case ATT_CHARACTERISTIC_ae01_02_VALUE_HANDLE:
        printf("rcsp_read:%x\n", buffer_size);
        if (app_recieve_callback) {
            app_recieve_callback(0, buffer, buffer_size);
        }
        break;
#endif

    default:
        break;
    }

    return 0;
}

static int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type)
{
    u32 ret = APP_BLE_NO_ERROR;

    if (!con_handle) {
        return APP_BLE_OPERATION_ERROR;
    }

    if (!multi_att_get_ccc_config(con_handle, handle + 1)) {
        log_info("fail,no write ccc!!!,%04x\n", handle + 1);
        return APP_BLE_NO_WRITE_CCC;
    }

    ret = ble_op_multi_att_send_data(con_handle, handle, data, len, handle_type);
    if (ret == BLE_BUFFER_FULL) {
        ret = APP_BLE_BUFF_FULL;
    }

    if (ret) {
        log_info("app_send_fail:%d !!!!!!\n", ret);
    }
    return ret;
}

//------------------------------------------------------
static int make_set_adv_data(void)
{
    u8 offset = adv_data_len;
    u8 *buf = adv_data;

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***adv_data overflow!!!!!!\n");
        return -1;
    }
    log_info("adv_data(%d):", offset);
    log_info_hexdump(buf, offset);
    ble_op_set_adv_data(offset, buf);
    return 0;
}

static const u8 link_key_data[16] = {0x06, 0x77, 0x5f, 0x87, 0x91, 0x8d, 0xd4, 0x23, 0x00, 0x5d, 0xf1, 0xd8, 0xcf, 0x0c, 0x14, 0x2b};
#define EIR_TAG_STRING   0xd6, 0x05, 0x08, 0x00, 'J', 'L', 'A', 'I', 'S', 'D','K'
static const char user_tag_string[] = {EIR_TAG_STRING};
static int make_set_rsp_data(void)
{
    u8 offset = scan_rsp_data_len;
    u8 *buf = scan_rsp_data;

#if RCSP_BTMATE_EN
    u8  tag_len = sizeof(user_tag_string);
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA, (void *)user_tag_string, tag_len);
#endif

    //    u8 name_len = gap_device_name_len;
    //    u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
    //    if (name_len > vaild_len) {
    //        name_len = vaild_len;
    //    }
    //    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_device_name, name_len);

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***rsp_data overflow!!!!!!\n");
        return -1;
    }

    log_info("rsp_data(%d):", offset);
    log_info_hexdump(buf, offset);
    scan_rsp_data_len = offset;
    ble_op_set_rsp_data(offset, buf);
    return 0;
}

/* static int make_set_rsp_data(void) */
/* { */
/* u8 offset = scan_rsp_data_len; */
/* u8 *buf = scan_rsp_data; */

/* if (offset > ADV_RSP_PACKET_MAX) { */
/* puts("***rsp_data overflow!!!!!!\n"); */
/* return -1; */
/* } */

/* log_info("rsp_data(%d):", offset); */
/* log_info_hexdump(buf, offset); */
/* ble_op_set_rsp_data(offset, buf); */
/* return 0; */
/* } */

//广播参数设置
static void advertisements_setup_init()
{
    uint8_t adv_type = ADV_IND;
    uint8_t adv_channel = ADV_CHANNEL_ALL;
    int   ret = 0;

    log_info("interval=%d,adv_type=%d,adv_chl=%d\n", adv_interval_value, adv_type, adv_channel);

    ble_op_set_adv_param(adv_interval_value, adv_type, adv_channel);

    if (1) {
        ret |= make_set_adv_data();
        ret |= make_set_rsp_data();
    }

    if (ret) {
        puts("advertisements_setup_init fail !!!!!!\n");
        return;
    }

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
    printf("set new_key= %06u\n", *key);
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
    sm_set_request_security(security_en);
    sm_set_master_request_pair(security_en);
    sm_event_callback_set(&cbk_sm_packet_handler);

    if (io_type == IO_CAPABILITY_DISPLAY_ONLY) {
        reset_PK_cb_register(reset_passkey_cb);
    }
}

#define AT_CHAR_TCFG_BLE_SECURITY_REQUEST        0/*是否发请求加密命令*/
void ble_client_profile_init(void);
void ble_profile_init(void)
{
    printf("ble profile init\n");

    if (!config_le_gatt_server_num) {
        ASSERT(0, "server not enough!!!\n");
        while (1);
    }

    if (config_le_sm_support_enable) {
        le_device_db_init();
        sm_pair_mode = 1;

#if PASSKEY_ENTER_ENABLE
        ble_sm_setup_init(IO_CAPABILITY_DISPLAY_ONLY, SM_AUTHREQ_MITM_PROTECTION | SM_AUTHREQ_BONDING, 7, AT_CHAR_TCFG_BLE_SECURITY_REQUEST);
#else
        ble_sm_setup_init(IO_CAPABILITY_NO_INPUT_NO_OUTPUT, SM_AUTHREQ_MITM_PROTECTION | SM_AUTHREQ_BONDING, 7, AT_CHAR_TCFG_BLE_SECURITY_REQUEST);
#endif
    }

#if CONFIG_BT_GATT_SERVER_NUM
    /* setup ATT server */
    att_server_init(at_char_profile_data, att_read_callback, att_write_callback);
    att_server_register_packet_handler(cbk_packet_handler);
    /* gatt_client_register_packet_handler(packet_cbk); */
#endif
    // register for HCI events
    hci_event_callback_set(&cbk_packet_handler);
    /* ble_l2cap_register_packet_handler(packet_cbk); */
    /* sm_event_packet_handler_register(packet_cbk); */
    le_l2cap_register_packet_handler(&cbk_packet_handler);

#if CONFIG_BT_GATT_CLIENT_NUM
    ble_client_profile_init();
#endif

    ble_vendor_set_default_att_mtu(ATT_LOCAL_MTU_SIZE);
}



static int set_adv_enable(void *priv, u32 en)
{
    ble_state_e next_state, cur_state;

    if (!adv_ctrl_en && en) {
        return APP_BLE_OPERATION_ERROR;
    }

    if (con_handle) {
        return APP_BLE_OPERATION_ERROR;
    }

    if (en) {
        next_state = BLE_ST_ADV;
    } else {
        next_state = BLE_ST_IDLE;
    }

    cur_state =  get_ble_work_state();
    switch (cur_state) {
    case BLE_ST_ADV:
    case BLE_ST_IDLE:
    case BLE_ST_INIT_OK:
    case BLE_ST_NULL:
    case BLE_ST_DISCONN:
        break;
    default:
        return APP_BLE_OPERATION_ERROR;
        break;
    }

    if (cur_state == next_state) {
        return APP_BLE_NO_ERROR;
    }
    log_info("adv_en:%d\n", en);
    set_ble_work_state(next_state);

    if (en) {
        advertisements_setup_init();
    }
    ble_op_adv_enable(en);

    return APP_BLE_NO_ERROR;
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

static int get_buffer_vaild_len(void *priv)
{
    u32 vaild_len = 0;
    ble_op_multi_att_get_remain(con_handle, &vaild_len);
    return vaild_len;
}



static int app_send_user_data_check(u16 len)
{
    u32 buf_space = get_buffer_vaild_len(0);
    if (len <= buf_space) {
        return 1;
    }
    return 0;
}

//
static int regiest_wakeup_send(void *priv, void *cbk)
{
    ble_resume_send_wakeup = cbk;
    return APP_BLE_NO_ERROR;
}

static int regiest_recieve_cbk(void *priv, void *cbk)
{
    channel_priv = (u32)priv;
    app_recieve_callback = cbk;
    return APP_BLE_NO_ERROR;
}

static int regiest_state_cbk(void *priv, void *cbk)
{
    channel_priv = (u32)priv;
    app_ble_state_callback = cbk;
    return APP_BLE_NO_ERROR;
}

//------------------------------------------------------------------------
//------------------------------------------------------------------------

u8 *ble_get_scan_rsp_ptr(u16 *len)
{
    if (len) {
        *len = scan_rsp_data_len;
    }
    return scan_rsp_data;
}

u8 *ble_get_adv_data_ptr(u16 *len)
{
    if (len) {
        *len = adv_data_len;
    }
    return adv_data;
}

u8 *ble_get_gatt_profile_data(u16 *len)
{
    *len = sizeof(at_char_profile_data);
    return (u8 *)at_char_profile_data;
}


void bt_ble_adv_enable(u8 enable)
{
    set_adv_enable(0, enable);
}

u16 bt_ble_is_connected(void)
{
    return con_handle;
}

void ble_module_enable(u8 en)
{
    log_info("mode_en:%d\n", en);
    if (en) {
        adv_ctrl_en = 1;
        if (auto_adv_enable) {
            bt_ble_adv_enable(1);
        }
    } else {
        if (con_handle) {
            adv_ctrl_en = 0;
            ble_disconnect(NULL);
        } else {
            bt_ble_adv_enable(0);
            adv_ctrl_en = 0;
        }
    }
}

extern void bt_ble_client_init(void);
extern void bt_ble_client_exit(void);

//------------------------new
/* extern char const device_name_default[]; */
void bt_ble_init(void)
{
    log_info("***** ble_init******\n");
    log_info("ble_file: %s", __FILE__);

    ble_op_multi_att_send_init(att_ram_buffer, ATT_RAM_BUFSIZE, ATT_LOCAL_MTU_SIZE);

    /* gap_device_name_len = strlen(device_name_default); */
    /* memcpy(gap_device_name, device_name_default, gap_device_name_len + 1); */

#if CONFIG_BT_GATT_SERVER_NUM
    ble_test_auto_adv(0);
    set_ble_work_state(BLE_ST_INIT_OK);
    ble_module_enable(1);
#endif

    log_info("ble name(%d): %s \n", gap_device_name_len, gap_device_name);

#if CONFIG_BT_GATT_CLIENT_NUM
    bt_ble_client_init();
#endif
}

void bt_ble_exit(void)
{
    log_info("***** ble_exit******\n");

    ble_module_enable(0);
#if CONFIG_BT_GATT_CLIENT_NUM
    bt_ble_client_exit();
#endif
}

void ble_app_disconnect(void)
{
    ble_disconnect(NULL);
}

#if RCSP_BTMATE_EN
static int rcsp_send_user_data_do(void *priv, u8 *data, u16 len)
{
    printf("rcsp_tx:%x\n", len);
#if PRINT_DMA_DATA_EN
    if (len < 128) {
        log_info("-dma_tx(%d):");
        log_info_hexdump(data, len);
    } else {
        putchar('L');
    }
#endif
    return app_send_user_data(ATT_CHARACTERISTIC_ae02_02_VALUE_HANDLE, data, len, ATT_OP_AUTO_READ_CCC);
}
#endif

#if RCSP_BTMATE_EN
static const struct ble_server_operation_t mi_ble_operation = {
    .adv_enable = set_adv_enable,
    .disconnect = ble_disconnect,
    .get_buffer_vaild = get_buffer_vaild_len,
    .send_data = (void *)rcsp_send_user_data_do,
    .regist_wakeup_send = regiest_wakeup_send,
    .regist_recieve_cbk = regiest_recieve_cbk,
    .regist_state_cbk = regiest_state_cbk,
};

//#else
//static const struct ble_server_operation_t mi_ble_operation = {
//    .adv_enable = set_adv_enable,
//    .disconnect = ble_disconnect,
//    .get_buffer_vaild = get_buffer_vaild_len,
//    .send_data = (void *)app_send_user_data_do,
//    .regist_wakeup_send = regiest_wakeup_send,
//    .regist_recieve_cbk = regiest_recieve_cbk,
//    .regist_state_cbk = regiest_state_cbk,
//};
//#endif

void ble_get_server_operation_table(struct ble_server_operation_t **interface_pt)
{
    *interface_pt = (void *)&mi_ble_operation;
}
#endif

void ble_server_send_test_key_num(u8 key_num)
{
    ;
}
//-----------------------------------------------
int ble_at_set_address(u8 *addr)
{
    le_controller_set_mac(addr);
    return APP_BLE_NO_ERROR;
}

int ble_at_get_address(u8 *addr)
{
    le_controller_get_mac(addr);
    return APP_BLE_NO_ERROR;
}

static u8 name_vm_buf[BT_NAME_LEN_MAX + 1];
int ble_at_set_name(u8 *name, u8 len)
{
    u8 ret = 0;
    if (len > BT_NAME_LEN_MAX - 1) {
        len = BT_NAME_LEN_MAX - 1;
    }
    memcpy(gap_device_name, name, len);
    gap_device_name_len = len;

    name_vm_buf[0] = len;
    memcpy(&name_vm_buf[1], name, len);




    ret = syscfg_write(AT_CHAR_DEV_NAME, name_vm_buf, len + 1);
    if (ret == len + 1) {
        return APP_BLE_NO_ERROR;
    } else {
        return APP_BLE_BUFF_ERROR;
    }
}

int ble_at_get_name(u8 *name)
{
    u8 ret = 0;
    u8 len = 0;
    ret = syscfg_read(AT_CHAR_DEV_NAME, name_vm_buf, 1);
    len = name_vm_buf[0];
    if (len == 0) { //没有东西,使用默认名字
        goto get_name;
    }

    ret = syscfg_read(AT_CHAR_DEV_NAME, name_vm_buf, len + 1);
    put_buf(name_vm_buf, ret);
    if (ret != len + 1) {
        return 0;
    }
    memcpy(gap_device_name, &name_vm_buf[1], len);
    gap_device_name_len = len;

get_name:
    memcpy(name, gap_device_name, gap_device_name_len);
    return gap_device_name_len;
}

int ble_at_send_data(u8 *data, u8 len)
{
    return app_send_user_data(data[0] + (data[1] << 8), &data[2], len - 2, ATT_OP_AUTO_READ_CCC);
}

int ble_at_disconnect(void)
{
    return ble_disconnect(0);
}

int ble_at_set_adv_data(u8 *data, u8 len)
{
    log_info("set_adv_data(%d)", len);
    adv_data_len = len;
    if (len) {
        log_info_hexdump(data, len);
        if (len > ADV_RSP_PACKET_MAX) {
            log_info("len err!\n");
            return APP_BLE_OPERATION_ERROR;
        }
        memcpy(adv_data, data, len);
    }
    return APP_BLE_NO_ERROR;
}

u8 *ble_at_get_adv_data(u8 *len)
{
    *len = adv_data_len;
    return adv_data;
}

int ble_at_set_rsp_data(u8 *data, u8 len)
{
    log_info("set_rsp_data(%d)", len);
    scan_rsp_data_len = len;
    if (len) {
        log_info_hexdump(data, len);
        if (len > ADV_RSP_PACKET_MAX) {
            log_info("len err!\n");
            return APP_BLE_OPERATION_ERROR;
        }
        memcpy(scan_rsp_data, data, len);
    }
    return APP_BLE_NO_ERROR;
}

u8 *ble_at_get_rsp_data(u8 *len)
{
    *len = scan_rsp_data_len;
    return scan_rsp_data;
}

int ble_at_adv_enable(u8 enable)
{
    bt_ble_adv_enable(enable);
    return 0;
}

int ble_at_get_adv_state(void)
{
    if (get_ble_work_state() == BLE_ST_ADV) {
        return 1;
    }
    return 0;
}

int ble_at_set_adv_interval(u16 value)
{
    adv_interval_value = value;
    return 0;
}

int ble_at_get_adv_interval(void)
{
    return adv_interval_value;
}

void input_key_handler(u8 key_status, u8 key_number)
{
}


u8 le_att_server_state(void)
{
    return get_ble_work_state();
}

int le_att_server_send_data(u8 cid, u8 *packet, u16 size)
{
    if (cid == cur_dev_cid) {
        return app_send_user_data(ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE, packet, size, ATT_OP_AUTO_READ_CCC);
    }
    return 0;
}


int le_att_server_send_ctrl(u8 cid, u8 *packet, u16 size)
{
    if (cid == cur_dev_cid) {
        return app_send_user_data(ATT_CHARACTERISTIC_ae05_01_VALUE_HANDLE, packet, size, ATT_OP_AUTO_READ_CCC);
    }
    return 0;
}

static const u8 test_adv_data[] = {
    2, 0x01, 0x06,
};

static const u8 test_rsp_data[] = {
    12, 0x09, 'b', 'l', 'e', '_', 'a', 't', '_', 't', 'e', 's', 't'
};


void ble_test_auto_adv(u8 en)
{
    log_info("ble_test_auto_adv = %d", en);
    auto_adv_enable = en;
    adv_interval_value = 160;
    ble_at_set_adv_data(test_adv_data, sizeof(test_adv_data));
    ble_at_set_rsp_data(test_rsp_data, sizeof(test_rsp_data));
    /* ble_at_set_rsp_data(0,0); */
    ble_at_set_name(&test_rsp_data[2], 11);
    ble_at_adv_enable(en);
}



#endif


