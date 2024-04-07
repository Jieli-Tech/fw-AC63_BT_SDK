/*********************************************************************************************
    *   Filename        : .c

    *   Description     :

    *   Author          : JM

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2021-01-17 11:14

    *   Copyright:(c)JIELI  2011-2026  @ , All Rights Reserved.
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
#include "le_gatt_common.h"
#include "btstack_3th_protocol_user.h"

#define LOG_TAG_CONST       GATT_SERVER
#define LOG_TAG             "[GATT_SERVER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_USER_BLE_ENABLE && CONFIG_BT_GATT_COMMON_ENABLE

/* #ifdef log_info */
/* #undef log_info */
/* #define log_info(x, ...)  y_printf("[GATT_SERVER]" x " ", ## __VA_ARGS__) */
/* #define log_info_hexdump  put_buf */
/* #endif */

//----------------------------------------------------------------------------------------
typedef struct {
    gatt_server_cfg_t *server_config; //server 配置
    adv_cfg_t *adv_config;            //adv 配置
    const u8 *profile_data;           //profile
    u16 profile_data_len;             //profile 长度
    u8  server_work_state;            //未链接ble 状态变化
    u8  adv_ctrl_en: 4;               //广播控制
    u8  rcsp_ctrl_en: 4;              //ota 升级控制
    const struct conn_update_param_t *server_connection_param_table; //连接参数更新表
    u8  server_connection_update_index: 4; //连接参数表执行id
    u8  server_connection_update_count: 4; //连接参数表内个数
    u8  server_encrypt_process;            //配对执行流程
    u16 server_operation_handle;           //当前执行更新参数流程操作handle
    u16 update_conn_handle;                //ota 升级连接handle
    u16 update_send_att_handle;            //ota发送att handle
    u8  update_send_att_handle_type;       //ota发送att handle的类型
    void (*update_ble_state_callback)(void *priv, ble_state_e state); //ota 状态回调
    void (*update_recieve_callback)(void *priv, void *buf, u16 len);  //ota 接收数据回调
    void (*update_resume_send_wakeup)(void);                          //ota 发送唤醒
} server_ctl_t;

static server_ctl_t server_s_hdl;
#define __this    (&server_s_hdl)

//------------------------------------------------------
extern void clr_wdt(void);
static void __gatt_server_check_auto_adv(void);
//------------------------------------------------------
/*************************************************************************************************/
/*!
 *  \brief      事件回调输出
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
//event callback,handle
static int __gatt_server_event_callback_handler(int event, u8 *packet, u16 size, u8 *ext_param)
{
    if (__this->server_config->event_packet_handler) {

        return __this->server_config->event_packet_handler(event, packet, size, ext_param);
    }
    return GATT_OP_RET_SUCESS;
}

/*************************************************************************************************/
/*!
 *  \brief      执行连接参数更新命令发送
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int __gatt_server_connection_update_request_send(void)
{
    if (__this->server_operation_handle) {
        if (__this->server_connection_update_index < __this->server_connection_update_count) {
            const struct conn_update_param_t *param = &__this->server_connection_param_table[__this->server_connection_update_index];
            __this->server_connection_update_index++;
            log_info("connection_update_request: %04x: -%d-%d-%d-%d-\n", __this->server_operation_handle, \
                     param->interval_min, param->interval_max, param->latency, param->timeout);
            ble_op_conn_param_request(__this->server_operation_handle, param);
        } else {
            __this->server_operation_handle = 0;
            return GATT_CMD_OPT_FAIL;
        }
    }
    return GATT_OP_RET_SUCESS;
}

/*************************************************************************************************/
/*!
 *  \brief      设置 连接 or 未连接状态
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void __gatt_server_set_work_state(u16 conn_handle, ble_state_e state, u8 report_en)
{
    u8 packet_buf[4];

    if (__this->update_ble_state_callback) {
        __this->update_ble_state_callback(NULL, state);
    }

    //区分连接和未连接的两个状态维护
    if (conn_handle != INVAIL_CONN_HANDLE) {
        ;
    } else if (state != __this->server_work_state) {
        log_info("server_work_st:%x->%x\n", __this->server_work_state, state);
        __this->server_work_state = state;
        little_endian_store_16(packet_buf, 1, 0);
    } else {
        return;
    }

    if (report_en) {
        packet_buf[0] = state;
        little_endian_store_16(packet_buf, 1, conn_handle);
        __gatt_server_event_callback_handler(GATT_COMM_EVENT_SERVER_STATE, packet_buf, 3, 0);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      协议栈发送包完成回调
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note      可以用于触发上层往协议栈填入数据
 */
/*************************************************************************************************/
static void __gatt_server_can_send_now_wakeup(void)
{
    /* log_info("can_send"); */

#if RCSP_BTMATE_EN
    if (!__this->rcsp_ctrl_en) {
        return;
    }
#endif

    if (__this->update_resume_send_wakeup) {
        __this->update_resume_send_wakeup();
    }
    __gatt_server_event_callback_handler(GATT_COMM_EVENT_CAN_SEND_NOW, 0, 0, 0);
}

/*************************************************************************************************/
/*!
 *  \brief      获取未连接的状态机
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
ble_state_e ble_gatt_server_get_work_state(void)
{
    return __this->server_work_state;
}

/*************************************************************************************************/
/*!
 *  \brief      获取已连接的状态机
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
ble_state_e ble_gatt_server_get_connect_state(u16 conn_handle)
{
    if (!conn_handle) {
        return BLE_ST_NULL;
    }
    return ble_comm_dev_get_handle_state(conn_handle, GATT_ROLE_SERVER);
}

/*************************************************************************************************/
/*!
 *  \brief      输入passkey处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_gatt_server_passkey_input(u32 *key, u16 conn_handle)
{
    u16 tmp_val[3];
    *key = 123456; //default key
    tmp_val[0] = conn_handle;
    little_endian_store_32(&tmp_val, 2, (u32)key);
    __gatt_server_event_callback_handler(GATT_COMM_EVENT_SM_PASSKEY_INPUT, tmp_val, 6, 0);
    log_info("conn_handle= %04x,set new_key= %06u\n", conn_handle, *key);
}

/*************************************************************************************************/
/*!
 *  \brief      sm 配对事件处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_gatt_server_sm_packet(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    sm_just_event_t *event = (void *)packet;
    u32 tmp32, ret;
    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
        case SM_EVENT_JUST_WORKS_REQUEST:
            //发送接受配对命令sm_just_works_confirm,否则不发
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            __this->server_encrypt_process = LINK_ENCRYPTION_PAIR_JUST_WORKS;

            ret = __gatt_server_event_callback_handler(GATT_COMM_EVENT_ENCRYPTION_REQUEST, &event->con_handle, 2, &__this->server_encrypt_process);
            if (ret) {
                log_info("first pair, %04x->Just decline.\n", event->con_handle);
                sm_bonding_decline(sm_event_just_works_request_get_handle(packet));
            } else {
                log_info("first pair, %04x->Just Works Confirmed.\n", event->con_handle);
                sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            }
            break;

        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            log_info_hexdump(packet, size);
            memcpy(&tmp32, event->data, 4);
            log_info("%04x->Passkey display: %06u.\n", event->con_handle, tmp32);
            __gatt_server_event_callback_handler(GATT_COMM_EVENT_ENCRYPTION_REQUEST, &event->con_handle, 2, &__this->server_encrypt_process);
            break;

        case SM_EVENT_PASSKEY_INPUT_NUMBER:
            /*IO_CAPABILITY_KEYBOARD_ONLY 方式*/
            __gatt_server_event_callback_handler(GATT_COMM_EVENT_ENCRYPTION_REQUEST, &event->con_handle, 2, &__this->server_encrypt_process);
            ble_gatt_server_passkey_input(&tmp32, event->con_handle);
            log_info("%04x->Passkey input: %06u.\n", event->con_handle, tmp32);
            sm_passkey_input(event->con_handle, tmp32); /*update passkey*/
            break;

        case SM_EVENT_PAIR_PROCESS:
            log_info("%04x->===Pair_process,sub= %02x\n", event->con_handle, event->data[0]);
            put_buf(event->data, 4);
            switch (event->data[0]) {
            case SM_EVENT_PAIR_SUB_RECONNECT_START:
                __this->server_encrypt_process = LINK_ENCRYPTION_RECONNECT;
                __gatt_server_event_callback_handler(GATT_COMM_EVENT_ENCRYPTION_REQUEST, &event->con_handle, 2, &__this->server_encrypt_process);
                log_info("reconnect start\n");
                break;

            case SM_EVENT_PAIR_SUB_PIN_KEY_MISS:
                log_error("pin or keymiss\n");
                break;

            case SM_EVENT_PAIR_SUB_PAIR_FAIL:
                log_error("pair fail,reason=%02x,is_peer? %d\n", event->data[1], event->data[2]);
                break;

            case SM_EVENT_PAIR_SUB_PAIR_TIMEOUT:
                log_error("pair timeout\n");
                break;

            case SM_EVENT_PAIR_SUB_ADD_LIST_SUCCESS:
                log_info("first pair,add list success\n");
                break;

            case SM_EVENT_PAIR_SUB_ADD_LIST_FAILED:
                log_error("add list fail\n");
                break;

            case SM_EVENT_PAIR_SUB_SEND_DISCONN:
                log_error("local send disconnect,reason= %02x\n", event->data[1]);
                break;

            default:
                break;
            }
            break;
        }
        break;
    }

}


_WEAK_
u8 ble_update_get_ready_jump_flag(void)
{
    return 0;
}

static const char *const server_phy_result[] = {
    "None",
    "1M",
    "2M",
    "Coded",
};
/*************************************************************************************************/
/*!
 *  \brief      协议栈的hci & gatt 事件处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
/*
 * @section Packet Handler
 *
 * @text The packet handler is used to:
 *        - stop the counter after a disconnect
 *        - send a notification when the requested ATT_EVENT_CAN_SEND_NOW is received
 */
/* LISTING_START(packetHandler): Packet Handler */
void ble_gatt_server_cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    u16 tmp_val[4];

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {

        case ATT_EVENT_CAN_SEND_NOW:
            __gatt_server_can_send_now_wakeup();
            break;

        case ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE:
            log_info("ATT_INDICATION_COMPLETE:con_handle= %04x,att_handle= %04x\n", \
                     little_endian_read_16(packet, 3), little_endian_read_16(packet, 5));
            tmp_val[0] = little_endian_read_16(packet, 3);
            tmp_val[1] = little_endian_read_16(packet, 5);
            __gatt_server_event_callback_handler(GATT_COMM_EVENT_SERVER_INDICATION_COMPLETE, tmp_val, 4, packet);
            break;

        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {

#if EXT_ADV_MODE_EN
            case HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE: {
                switch (packet[3]) {
                case BT_OP_SUCCESS:
                    tmp_val[0] = hci_subevent_le_enhanced_connection_complete_get_connection_handle(packet);
                    log_info("HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE : %0x\n", tmp_val[0]);
                    log_info("conn_interval = %d\n", hci_subevent_le_enhanced_connection_complete_get_conn_interval(packet));
                    log_info("conn_latency = %d\n", hci_subevent_le_enhanced_connection_complete_get_conn_latency(packet));
                    log_info("conn_timeout = %d\n", hci_subevent_le_enhanced_connection_complete_get_supervision_timeout(packet));
                    __this->server_encrypt_process = LINK_ENCRYPTION_NULL;
                    __gatt_server_set_work_state(INVAIL_CONN_HANDLE, BLE_ST_IDLE, 0);
                    __gatt_server_event_callback_handler(GATT_COMM_EVENT_CONNECTION_COMPLETE, tmp_val, 2, packet);
                    break;

                case BT_ERR_ADVERTISING_TIMEOUT:
                    log_info("DIRECT_ADV TO!\n");
                    __gatt_server_set_work_state(INVAIL_CONN_HANDLE, BLE_ST_IDLE, 1);
                    __gatt_server_event_callback_handler(GATT_COMM_EVENT_DIRECT_ADV_TIMEOUT, tmp_val, 2, packet);
                    break;

                default:
                    log_info("CONNECTION FAIL!!! %0x\n", packet[3]);
                    __gatt_server_set_work_state(INVAIL_CONN_HANDLE, BLE_ST_IDLE, 1);
                    __gatt_server_event_callback_handler(GATT_COMM_EVENT_CONNECTION_COMPLETE_FAIL, tmp_val, 2, packet);
                    break;
                }

                if (BT_OP_SUCCESS != packet[3]) {
                    __gatt_server_check_auto_adv();
                } else {
                    __this->server_encrypt_process = LINK_ENCRYPTION_NULL;
                    __gatt_server_event_callback_handler(GATT_COMM_EVENT_CONNECTION_COMPLETE, tmp_val, 2, packet);
                }

            }
            break;
#endif

            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE: {
                if (BT_OP_SUCCESS != packet[3]) {
                    switch (packet[3]) {
                    case BT_ERR_ADVERTISING_TIMEOUT:
                        log_info("DIRECT_ADV TO!\n");
                        __gatt_server_set_work_state(INVAIL_CONN_HANDLE, BLE_ST_IDLE, 1);
                        __gatt_server_event_callback_handler(GATT_COMM_EVENT_DIRECT_ADV_TIMEOUT, tmp_val, 2, packet);
                        break;

                    default:
                        log_info("CONNECTION FAIL!!! %0x\n", packet[3]);
                        __gatt_server_set_work_state(INVAIL_CONN_HANDLE, BLE_ST_IDLE, 1);
                        __gatt_server_event_callback_handler(GATT_COMM_EVENT_CONNECTION_COMPLETE_FAIL, tmp_val, 2, packet);
                        break;
                    }
                    __gatt_server_check_auto_adv();
                    break;
                }

                tmp_val[0] = hci_subevent_le_connection_complete_get_connection_handle(packet);
                log_info("HCI_SUBEVENT_LE_CONNECTION_COMPLETE:conn_handle= %04x,rssi= %d\n", tmp_val[0], ble_vendor_get_peer_rssi(tmp_val[0]));
                log_info("conn_interval = %d\n", hci_subevent_le_connection_complete_get_conn_interval(packet));
                log_info("conn_latency = %d\n", hci_subevent_le_connection_complete_get_conn_latency(packet));
                log_info("conn_timeout = %d\n", hci_subevent_le_connection_complete_get_supervision_timeout(packet));

#if RCSP_BTMATE_EN
                if (!__this->rcsp_ctrl_en) {
                    __this->rcsp_ctrl_en = 1;
#if (defined(BT_CONNECTION_VERIFY) && (0 == BT_CONNECTION_VERIFY))
                    JL_rcsp_auth_reset();
#endif
#if CONFIG_APP_DONGLE
                    rcsp_dev_select_v1(RCSP_BLE);
#else
                    rcsp_init();
#endif
                }
#endif
                ble_comm_dev_set_handle_state(tmp_val[0], GATT_ROLE_SERVER, BLE_ST_CONNECT);
                __this->server_encrypt_process = LINK_ENCRYPTION_NULL;
                __gatt_server_set_work_state(tmp_val[0], BLE_ST_CONNECT, 1);
                __gatt_server_set_work_state(INVAIL_CONN_HANDLE, BLE_ST_IDLE, 0);
                __gatt_server_event_callback_handler(GATT_COMM_EVENT_CONNECTION_COMPLETE, tmp_val, 2, packet);
            }
            break;

            case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE: {
                tmp_val[0] = little_endian_read_16(packet, 4);
                log_info("conn_update_handle   = %04x\n", tmp_val[0]);
                log_info("conn_update_interval = %d\n", hci_subevent_le_connection_update_complete_get_conn_interval(packet));
                log_info("conn_update_latency  = %d\n", hci_subevent_le_connection_update_complete_get_conn_latency(packet));
                log_info("conn_update_timeout  = %d\n", hci_subevent_le_connection_update_complete_get_supervision_timeout(packet));
                __gatt_server_event_callback_handler(GATT_COMM_EVENT_CONNECTION_UPDATE_COMPLETE, tmp_val, 2, packet);
            }
            break;

            case HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE:
                tmp_val[0] = little_endian_read_16(packet, 3);
                log_info("conn_handle = %04x\n", tmp_val[0]);
                log_info("APP HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE\n");
                log_info("TX_Octets:%d, Time:%d\n", little_endian_read_16(packet, 5), little_endian_read_16(packet, 7));
                log_info("RX_Octets:%d, Time:%d\n", little_endian_read_16(packet, 9), little_endian_read_16(packet, 11));
                __gatt_server_event_callback_handler(GATT_COMM_EVENT_CONNECTION_DATA_LENGTH_CHANGE, tmp_val, 2, packet);
                break;

            case HCI_SUBEVENT_LE_PHY_UPDATE_COMPLETE:
                log_info("conn_handle = %04x\n", little_endian_read_16(packet, 3));
                log_info("APP HCI_SUBEVENT_LE_PHY_UPDATE %s\n", hci_event_le_meta_get_phy_update_complete_status(packet) ? "Fail" : "Succ");
                log_info("Tx PHY: %s\n", server_phy_result[hci_event_le_meta_get_phy_update_complete_tx_phy(packet)]);
                log_info("Rx PHY: %s\n", server_phy_result[hci_event_le_meta_get_phy_update_complete_rx_phy(packet)]);
                __gatt_server_event_callback_handler(GATT_COMM_EVENT_CONNECTION_PHY_UPDATE_COMPLETE, tmp_val, 2, packet);
                break;
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE: {
            tmp_val[0] = little_endian_read_16(packet, 3);
            tmp_val[1] = packet[5];
            if (__this->server_operation_handle == tmp_val[0]) {
                __this->server_operation_handle = 0;
            }
            log_info("HCI_EVENT_DISCONNECTION_COMPLETE:conn_handle= %04x, reason= %02x\n", tmp_val[0], packet[5]);

#if RCSP_BTMATE_EN
            if (__this->update_conn_handle == tmp_val[0]) {
                __this->update_conn_handle = 0;
                __this->rcsp_ctrl_en = 0;
#if (0 == CONFIG_APP_DONGLE)
                rcsp_exit();
#endif
            }
#endif

            multi_att_clear_ccc_config(tmp_val[0]);
            ble_comm_dev_set_handle_state(tmp_val[0], GATT_ROLE_SERVER, BLE_ST_DISCONN);
            __gatt_server_set_work_state(tmp_val[0], BLE_ST_DISCONN, 1);
            __gatt_server_event_callback_handler(GATT_COMM_EVENT_DISCONNECT_COMPLETE, tmp_val, 4, packet);
            //配设备开adv
            if (!ble_update_get_ready_jump_flag()) {
                __gatt_server_check_auto_adv();
            }
        }
        break;

        case ATT_EVENT_MTU_EXCHANGE_COMPLETE: {
            tmp_val[0] = little_endian_read_16(packet, 2);
            tmp_val[1] = att_event_mtu_exchange_complete_get_MTU(packet);
            log_info("handle= %02x, ATT_MTU= %u,payload= %u\n", tmp_val[0], tmp_val[1], tmp_val[1] - 3);
            __gatt_server_event_callback_handler(GATT_COMM_EVENT_MTU_EXCHANGE_COMPLETE, tmp_val, 4, 0);
        }
        break;

        case HCI_EVENT_VENDOR_REMOTE_TEST:
            log_info("---HCI_EVENT_VENDOR_REMOTE_TEST\n");
            break;

        case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE: {
            tmp_val[0] = little_endian_read_16(packet, 2);
            tmp_val[1] = little_endian_read_16(packet, 4);
            log_info("-update_rsp:%04x, %02x\n", tmp_val[0], tmp_val[1]);

            if (__this->server_operation_handle == tmp_val[0]) {
                if (tmp_val[1]) {
                    log_info("remoter reject\n");
                    if (__gatt_server_connection_update_request_send()) {
                        break;
                    }
                } else {
                    log_info("remoter accept \n");
                    __this->server_operation_handle = 0;
                }
                __gatt_server_event_callback_handler(GATT_COMM_EVENT_CONNECTION_UPDATE_REQUEST_RESULT, tmp_val, 4, 0);
            }
        }
        break;

        case HCI_EVENT_ENCRYPTION_CHANGE: {
            tmp_val[0] = little_endian_read_16(packet, 3);
            tmp_val[1] = packet[2] | (__this->server_encrypt_process << 8);
            log_info("HCI_EVENT_ENCRYPTION_CHANGE= %d, %04x\n", packet[2], tmp_val[0]);
            if (packet[2]) {
                log_info("Encryption fail!!!,%d,%04x\n", packet[2], tmp_val[0]);
            }
            __gatt_server_event_callback_handler(GATT_COMM_EVENT_ENCRYPTION_CHANGE, tmp_val, 4, 0);
        }
        break;
        }
        break;
    }
}


#if EXT_ADV_MODE_EN ||PERIODIC_ADV_MODE_EN

#define EXT_ADV_NAME                    'J', 'L', '_', 'E', 'X', 'T', '_', 'A', 'D', 'V'
/* #define EXT_ADV_NAME                    "JL_EXT_ADV" */
#define EXT_ADV_DATA                    \
    0x02, 0x01, 0x06, \
    0x03, 0x02, 0xF0, 0xFF, \
    BYTE_LEN(EXT_ADV_NAME) + 1, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, EXT_ADV_NAME

const le_set_ext_adv_param_t ext_adv_param = {
    .Advertising_Handle = 0,
#if PERIODIC_ADV_MODE_EN
    .Advertising_Event_Properties = 0,
#else
    .Advertising_Event_Properties = 1,
#endif /*PERIODIC_ADV_MODE_EN*/
    .Primary_Advertising_Interval_Min = {30, 0, 0},
    .Primary_Advertising_Interval_Max = {30, 0, 0},
    .Primary_Advertising_Channel_Map = 7,
    .Primary_Advertising_PHY = ADV_SET_1M_PHY,
    .Secondary_Advertising_PHY = ADV_SET_1M_PHY,
    .Advertising_SID  = CUR_ADVERTISING_SID,
};

const le_set_ext_adv_data_t ext_adv_data = {
    .Advertising_Handle = 0,
    .Operation = 3,
    .Fragment_Preference = 0,
    .Advertising_Data_Length = BYTE_LEN(EXT_ADV_DATA),
    .Advertising_Data = EXT_ADV_DATA,
};

const le_set_ext_adv_en_t ext_adv_enable = {
    .Enable = 1,
    .Number_of_Sets = 1,
    .Advertising_Handle = 0,
    .Duration = 0,
    .Max_Extended_Advertising_Events = 0,
};

const le_set_ext_adv_en_t ext_adv_disable = {
    .Enable = 0,
    .Number_of_Sets = 1,
    .Advertising_Handle = 0,
    .Duration = 0,
    .Max_Extended_Advertising_Events = 0,
};

#endif /* EXT_ADV_MODE_EN */

#if PERIODIC_ADV_MODE_EN

#if CHAIN_DATA_TEST_EN
#define PERIODIC_ADV_DATA                    \
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
#else
#define PERIODIC_ADV_DATA               EXT_ADV_DATA
#endif /* CHAIN_DATA_TEST_EN */

static const struct periodic_advertising_param periodic_adv_param = {
    .Advertising_Handle = CUR_ADV_HANDLE,
    .Periodic_Advertising_Interval_Min = 50,
    .Periodic_Advertising_Interval_Max = 50,
    .Periodic_Advertising_Properties = 0,
};

static const struct periodic_advertising_data periodic_adv_data = {
    .Advertising_Handle = CUR_ADV_HANDLE,
    .Operation = 3,
    .Advertising_Data_Length = BYTE_LEN(PERIODIC_ADV_DATA),
    .Advertising_Data = PERIODIC_ADV_DATA,
};

static const struct periodic_advertising_enable periodic_adv_enable = {
    .Enable = 1,
    .Advertising_Handle = CUR_ADV_HANDLE,
};

static const struct periodic_advertising_enable periodic_adv_disable = {
    .Enable = 0,
    .Advertising_Handle = CUR_ADV_HANDLE,
};

#endif 	/*PERIODIC_ADV_MODE_EN*/
/*************************************************************************************************/
/*!
 *  \brief      检查是否自动开广播
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void __gatt_server_check_auto_adv(void)
{
    if (__this->adv_config->adv_auto_do) {
        ble_gatt_server_adv_enable(1);
    }
}

/*************************************************************************************************/
/*!
 *  \brief       检查是否支持新设备open adv
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static bool __gatt_server_just_new_dev_adv(void)
{
    log_info("%s\n", __FUNCTION__);
    u8 state = ble_gatt_server_get_work_state();

#if RCSP_UPDATE_EN
    if (get_jl_update_flag()) {
        return false;
    }
#endif

    if (state != BLE_ST_IDLE && state != BLE_ST_DISCONN && state != BLE_ST_INIT_OK) {
        log_info("dev_doing,%02x\n", state);
        return false;
    }

    s8 tmp_cid = ble_comm_dev_get_idle_index(GATT_ROLE_SERVER);
    if (tmp_cid == INVAIL_INDEX) {
        log_info("no idle dev to do!!!\n");
        return false;
    }

    log_info("new_dev_adv\n");
    return true;
}

/*************************************************************************************************/
/*!
 *  \brief      调用开广播
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
int ble_gatt_server_adv_enable(u32 en)
{
    ble_state_e next_state, cur_state;

    if (!__this->adv_ctrl_en && en) {
        return GATT_CMD_OPT_FAIL;
    }

    if (en) {
        if (!__gatt_server_just_new_dev_adv()) {
            return GATT_CMD_OPT_FAIL;
        }
        next_state = BLE_ST_ADV;
    } else {
        next_state = BLE_ST_IDLE;
    }

    cur_state =  ble_gatt_server_get_work_state();

    switch (cur_state) {
    case BLE_ST_ADV:
    case BLE_ST_IDLE:
    case BLE_ST_INIT_OK:
    case BLE_ST_NULL:
    case BLE_ST_DISCONN:
        break;
    default:
        return GATT_CMD_OPT_FAIL;
        break;
    }

    if (cur_state == next_state) {
        return GATT_OP_RET_SUCESS;
    }
    log_info("adv_en:%d\n", en);
    __gatt_server_set_work_state(INVAIL_CONN_HANDLE, next_state, 1);

#if EXT_ADV_MODE_EN
    if (en) {
        ble_op_set_ext_adv_param(&ext_adv_param, sizeof(ext_adv_param));
        ble_op_set_ext_adv_data(&ext_adv_data, sizeof(ext_adv_data));
        ble_op_set_ext_adv_enable(&ext_adv_enable, sizeof(ext_adv_enable));
    } else {
        ble_op_set_ext_adv_enable(&ext_adv_disable, sizeof(ext_adv_disable));
    }
#elif PERIODIC_ADV_MODE_EN
    if (en) {
        ble_op_set_ext_adv_param(&ext_adv_param, sizeof(ext_adv_param));
        ble_user_cmd_prepare(BLE_CMD_PERIODIC_ADV_PARAM, 2, &periodic_adv_param, sizeof(periodic_adv_param));
        ble_user_cmd_prepare(BLE_CMD_PERIODIC_ADV_DATA, 2, &periodic_adv_data, sizeof(periodic_adv_data));
        ble_user_cmd_prepare(BLE_CMD_PERIODIC_ADV_ENABLE, 2, &periodic_adv_enable, sizeof(periodic_adv_enable));
        ble_op_set_ext_adv_enable(&ext_adv_enable, sizeof(ext_adv_enable));
    } else {
        ble_op_set_ext_adv_enable(&ext_adv_disable, sizeof(ext_adv_disable));
        ble_user_cmd_prepare(BLE_CMD_PERIODIC_ADV_ENABLE, 2, &periodic_adv_disable, sizeof(periodic_adv_disable));
    }
#else
    if (en) {
        if (__this->adv_config->set_local_addr_tag == USE_SET_LOCAL_ADDRESS_TAG) {
            le_controller_set_mac(&__this->adv_config->local_address_info[1]);
        }

        ble_op_set_adv_param_ext(__this->adv_config->adv_interval, __this->adv_config->adv_type, \
                                 __this->adv_config->adv_channel, __this->adv_config->direct_address_info);
        ble_op_set_adv_data(__this->adv_config->adv_data_len, __this->adv_config->adv_data);
        ble_op_set_rsp_data(__this->adv_config->rsp_data_len, __this->adv_config->rsp_data);
    }
    ble_op_adv_enable(en);
#endif /* EXT_ADV_MODE_EN */

    return GATT_OP_RET_SUCESS;
}

/*************************************************************************************************/
/*!
 *  \brief      断开server所有链路
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_gatt_server_disconnect_all(void)
{
    u8 i;
    u16 conn_handle;
    for (u8 i = 0; i < SUPPORT_MAX_GATT_SERVER; i++) {
        conn_handle = ble_comm_dev_get_handle(i, GATT_ROLE_SERVER);
        if (conn_handle) {
            ble_comm_disconnect(conn_handle);
            os_time_dly(1);
        }
    }
}


/*************************************************************************************************/
/*!
 *  \brief      断开OTA升级链路
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_gatt_server_disconnect_update_conn(void)
{
    if (__this->update_conn_handle) {
        ble_comm_disconnect(__this->update_conn_handle);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      sever 模块开关
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_gatt_server_module_enable(u8 en)
{
    log_info("mode_en:%d\n", en);
    if (en) {
        __this->adv_ctrl_en = 1;
        __gatt_server_check_auto_adv();
    } else {
        ble_gatt_server_adv_enable(0);
        __this->adv_ctrl_en = 0;
        ble_gatt_server_disconnect_all();
        __gatt_server_set_work_state(INVAIL_CONN_HANDLE, BLE_ST_IDLE, 0);
    }
}

/*************************************************************************************************/
/*!
 *  \brief       启动请求更新连接参数表
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
int ble_gatt_server_connetion_update_request(u16 conn_handle, const struct conn_update_param_t *update_table, u16 table_count)
{
    if (!conn_handle) {
        return GATT_CMD_PARAM_ERROR;
    }

    if (__this->server_operation_handle) {
        log_info("connection_update busy!!!\n");
        return GATT_CMD_RET_BUSY;
    }

    if (ble_comm_dev_get_handle_role(conn_handle) != GATT_ROLE_SERVER) {
        log_info("conn_handle error!!!\n");
        return GATT_OP_ROLE_ERR;
    }

    __this->server_operation_handle = conn_handle;
    __this->server_connection_param_table = update_table;
    __this->server_connection_update_count = table_count;
    __this->server_connection_update_index = 0;
    __gatt_server_connection_update_request_send();
    return GATT_OP_RET_SUCESS;
}

/*************************************************************************************************/
/*!
 *  \brief      更新 notify or indicate 通知功能使能
 *
 *  \param      [in]
 *
 *  \return     gatt_op_ret_e
 *
 *  \note       注意多机应用是，此接口会主动触发是否开新设备广播功能
 */
/*************************************************************************************************/
int ble_gatt_server_characteristic_ccc_set(u16 conn_handle, u16 att_ccc_handle, u16 ccc_config)
{
    multi_att_set_ccc_config(conn_handle, att_ccc_handle, ccc_config);
    if (BLE_ST_NOTIFY_IDICATE != ble_comm_dev_get_handle_state(conn_handle, GATT_ROLE_SERVER)) {
        ble_comm_dev_set_handle_state(conn_handle, GATT_ROLE_SERVER, BLE_ST_NOTIFY_IDICATE);
        __gatt_server_set_work_state(conn_handle, BLE_ST_NOTIFY_IDICATE, 1);
    }

    //多机等使能服务通知后才打开新广播
    if (SUPPORT_MAX_GATT_SERVER > 1) {
        __gatt_server_check_auto_adv();
    }
    return GATT_OP_RET_SUCESS;
}

/*************************************************************************************************/
/*!
 *  \brief      获取 notify or indicate 通知使能值
 *
 *  \param      [in]
 *
 *  \return     return
 GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NONE
 GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION
 GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_INDICATION
 *
 *  \note
 */
/*************************************************************************************************/
u16 ble_gatt_server_characteristic_ccc_get(u16 conn_handle, u16 att_ccc_handle)
{
    return multi_att_get_ccc_config(conn_handle, att_ccc_handle);
}

/*************************************************************************************************/
/*!
 *  \brief      发送完成唤醒
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
int ble_gatt_server_regiest_wakeup_send(void *priv, void *cbk)
{
    __this->update_resume_send_wakeup = cbk;
    return GATT_OP_RET_SUCESS;
}

/*************************************************************************************************/
/*!
 *  \brief      配置开广播参数
 *
 *  \param      [in]
 *
 *  \return     开广播前调用
 *
 *  \note
 */
/*************************************************************************************************/
void ble_gatt_server_set_adv_config(adv_cfg_t *adv_cfg)
{
    __this->adv_config = adv_cfg;
}

/*************************************************************************************************/
/*!
 *  \brief      配置profile
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note      未连接时，可配置
 */
/*************************************************************************************************/
void ble_gatt_server_set_profile(const u8 *profile_table, u16 size)
{
    __this->profile_data = profile_table;
    __this->profile_data_len = size;
    att_server_change_profile(profile_table);
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙协议栈初始化过程调用
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static const uint8_t default_profile_data[] = {
    0x0a, 0x00, 0x02, 0x00, 0x01, 0x00, 0x00, 0x28, 0x00, 0x18,
    0x00, 0x00,
};
void ble_gatt_server_profile_init(void)
{
    log_info("%s\n", __FUNCTION__);
    att_server_init(default_profile_data, __this->server_config->att_read_cb, \
                    __this->server_config->att_write_cb);
    __this->server_work_state = BLE_ST_INIT_OK;
}

/*************************************************************************************************/
/*!
 *  \brief     gatt server模块初始化,蓝牙协议栈初始化前调用
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_gatt_server_init(gatt_server_cfg_t *server_cfg)
{
    log_info("%s\n", __FUNCTION__);
    memset(__this, 0, sizeof(server_ctl_t));
    __this->server_config = server_cfg;
}

/*************************************************************************************************/
/*!
 *  \brief      gatt server模块退出
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note       协议栈退出前调用
 */
/*************************************************************************************************/
void ble_gatt_server_exit(void)
{
    log_info("%s\n", __FUNCTION__);
    ble_gatt_server_module_enable(0);
}


#if RCSP_BTMATE_EN  //OTA 升级功能
//------------------------------------------------------------------------------
/*************************************************************************************************/
/*!
 *  \brief      获取rsp包
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
u8 *ble_get_scan_rsp_ptr(u16 *len)
{
    if (__this && __this->adv_config) {
        if (len) {
            *len = __this->adv_config->rsp_data_len;
        }
        return __this->adv_config->rsp_data;
    } else {
        if (len) {
            *len = 0;
        }
        log_info("error: %s\n", __FUNCTION__);
        return 0;
    }
}

/*************************************************************************************************/
/*!
 *  \brief      获取adv包
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
u8 *ble_get_adv_data_ptr(u16 *len)
{
    if (__this && __this->adv_config) {
        if (len) {
            *len = __this->adv_config->adv_data_len;
        }
        return __this->adv_config->adv_data;
    } else {
        if (len) {
            *len = 0;
        }
        log_info("error: %s\n", __FUNCTION__);
        return 0;
    }
}

/*************************************************************************************************/
/*!
 *  \brief      获取profile
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
u8 *ble_get_gatt_profile_data(u16 *len)
{
    if (__this && __this->adv_config) {
        *len = __this->profile_data_len;
        return __this->profile_data;
    } else {
        *len = 0;
        log_info("error: %s\n", __FUNCTION__);
        return 0;
    }
}

/*************************************************************************************************/
/*!
 *  \brief      设置OTA升级使用的 连接handle 和 att handle
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_gatt_server_set_update_send(u16 conn_handle, u16 att_handle, u8 att_handle_type)
{
    __this->update_conn_handle = conn_handle;
    __this->update_send_att_handle = att_handle;
    __this->update_send_att_handle_type = att_handle_type;
}

/*************************************************************************************************/
/*!
 *  \brief      ota 接收数据处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_gatt_server_receive_update_data(void *priv, void *buf, u16 len)
{
    if (__this->update_recieve_callback) {
        __this->update_recieve_callback(priv, buf, len);
    }
}


/*************************************************************************************************/
/*!
 *  \brief      ota 发送数据
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int update_send_user_data_do(void *priv, u8 *data, u16 len)
{
    log_info("update_tx:%x\n", len);
    if (0 == __this->update_conn_handle || 0 == __this->update_send_att_handle) {
        return GATT_CMD_OPT_FAIL;
    }

    return ble_comm_att_send_data(__this->update_conn_handle, \
                                  __this->update_send_att_handle, data, len, __this->update_send_att_handle_type);
}

/*************************************************************************************************/
/*!
 *  \brief      ota 开关广播
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int update_adv_enable(void *priv, u32 en)
{
    return ble_gatt_server_adv_enable(en);
}

/*************************************************************************************************/
/*!
 *  \brief      ota 获取可用buff长度
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int update_get_buffer_len(void *priv)
{
    return ble_comm_cbuffer_vaild_len(__this->update_conn_handle);
}

/*************************************************************************************************/
/*!
 *  \brief      注册接收数据处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int update_regiest_recieve_cbk(void *priv, void *cbk)
{
    __this->update_recieve_callback = cbk;
    return GATT_OP_RET_SUCESS;
}

/*************************************************************************************************/
/*!
 *  \brief      注册状态回调
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int update_regiest_state_cbk(void *priv, void *cbk)
{
    __this->update_ble_state_callback = cbk;
    return GATT_OP_RET_SUCESS;
}

/*************************************************************************************************/
/*!
 *  \brief      ota升级使用的接口表
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static const struct ble_server_operation_t gatt_server_update_operation = {
    .adv_enable = update_adv_enable,
    .disconnect = ble_gatt_server_disconnect_update_conn,
    //.get_buffer_vaild = get_buffer_vaild_len,
    .send_data = update_send_user_data_do,
    .regist_wakeup_send = ble_gatt_server_regiest_wakeup_send,
    .regist_recieve_cbk = update_regiest_recieve_cbk,
    .regist_state_cbk = update_regiest_state_cbk,
};

/*************************************************************************************************/
/*!
 *  \brief      获取ota升级接口表
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_get_server_operation_table(struct ble_server_operation_t **interface_pt)
{
    *interface_pt = (void *)&gatt_server_update_operation;
}

#endif


#endif


