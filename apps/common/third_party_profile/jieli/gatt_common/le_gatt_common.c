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
#include "le_common.h"
#include "rcsp_bluetooth.h"
#include "JL_rcsp_api.h"
#include "custom_cfg.h"
#include "btstack/btstack_event.h"
#include "le_gatt_common.h"


#define LOG_TAG_CONST       GATT_COMM
#define LOG_TAG             "[GATT_COMM]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_USER_BLE_ENABLE && CONFIG_BT_GATT_COMMON_ENABLE

/* #ifdef log_info */
/* #undef log_info */
/* #define log_info(x, ...)  r_printf("[GATT_COMM]" x " ", ## __VA_ARGS__) */
/* #define log_info_hexdump  put_buf */
/* #endif */

static u8 *gatt_ram_buffer;
static u8 cbk_event_type;
static gatt_ctrl_t *gatt_control_block;

#define CLR_HANDLER_ROLE()      cbk_event_type  = 0
#define SET_HANDLER_ROLE(a)     cbk_event_type  = (1<<a)
#define ADD_HANDLER_ROLE(a)     cbk_event_type |= (1<<a)
#define CHECK_HANDLER_ROLE(a)   (cbk_event_type & (1<<a))

static u16 gatt_server_conn_handle[SUPPORT_MAX_GATT_SERVER];
static u16 gatt_client_conn_handle[SUPPORT_MAX_GATT_CLIENT];
static u8 gatt_server_conn_handle_state[SUPPORT_MAX_GATT_SERVER];//BLE_ST_CONNECT,BLE_ST_SEND_DISCONN,BLE_ST_NOTIFY_IDICATE
static u8 gatt_client_conn_handle_state[SUPPORT_MAX_GATT_CLIENT];//BLE_ST_CONNECT,BLE_ST_SEND_DISCONN,BLE_ST_SEARCH_COMPLETE

//----------------------------------------------------------------------------------------
u32 att_need_ctrl_ramsize(void);
void ble_comm_server_profile_init(void);
void ble_gatt_server_passkey_input(u32 *key, u16 conn_handle);
void ble_gatt_client_passkey_input(u32 *key, u16 conn_handle);
void ble_comm_client_profile_init(void);
void ble_comm_server_init(gatt_server_cfg_t *server_cfg);
void ble_comm_server_exit(void);
void ble_gatt_server_sm_packet(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
void ble_gatt_server_cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
void ble_gatt_server_profile_init(void);
void ble_gatt_client_profile_init(void);
void ble_gatt_client_sm_packet(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
void ble_gatt_client_cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
//----------------------------------------------------------------------------------------
/*************************************************************************************************/
/*!
 *  \brief      获取连接 handle 分配的index
 *
 *  \param      [in] handle
 *  \param      [in] role
 *
 *  \return     index
 *
 *  \note
 */
/*************************************************************************************************/
s8 ble_comm_dev_get_index(u16 handle, u8 role)
{
    s8 i;
    u16 *group_handle;
    u8 count;

    if (GATT_ROLE_SERVER == role) {
        group_handle = gatt_server_conn_handle;
        count = SUPPORT_MAX_GATT_SERVER;
    } else {
        group_handle = gatt_client_conn_handle;
        count = SUPPORT_MAX_GATT_CLIENT;
    }

    for (i = 0; i < count; i++) {
        if (handle == group_handle[i]) {
            return i;
        }
    }
    return INVAIL_INDEX;
}

/*************************************************************************************************/
/*!
 *  \brief      获取可用分配连接 的index
 *
 *  \param      [in]
 *
 *  \return     role
 *
 *  \note
 */
/*************************************************************************************************/
s8 ble_comm_dev_get_idle_index(u8 role)
{
    s8 i;
    u16 *group_handle;
    u8 count;

    if (GATT_ROLE_SERVER == role) {
        group_handle = gatt_server_conn_handle;
        count = SUPPORT_MAX_GATT_SERVER;
    } else {
        group_handle = gatt_client_conn_handle;
        count = SUPPORT_MAX_GATT_CLIENT;
    }

    for (i = 0; i < count; i++) {
        if (0 == group_handle[i]) {
            return i;
        }
    }
    return INVAIL_INDEX;
}

/*************************************************************************************************/
/*!
 *  \brief      断开，释放连接 handle的index
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static s8 ble_comm_del_dev_index(u16 handle, u8 role)
{
    s8 i;
    u8 count;
    u16 *group_handle;

    if (GATT_ROLE_SERVER == role) {
        group_handle = gatt_server_conn_handle;
        count = SUPPORT_MAX_GATT_SERVER;
    } else {
        group_handle = gatt_client_conn_handle;
        count = SUPPORT_MAX_GATT_CLIENT;
    }


    for (i = 0; i < count; i++) {
        if (handle == group_handle[i]) {
            group_handle[i] = 0;
            return i;
        }
    }
    return INVAIL_INDEX;
}


/*************************************************************************************************/
/*!
 *  \brief      检查gatt 角色 是否已有链路连接
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
bool ble_comm_dev_is_connected(u8 role)
{
    s8 i;
    u16 *group_handle;
    u8 count;

    if (GATT_ROLE_SERVER == role) {
        group_handle = gatt_server_conn_handle;
        count = SUPPORT_MAX_GATT_SERVER;
    } else {
        group_handle = gatt_client_conn_handle;
        count = SUPPORT_MAX_GATT_CLIENT;
    }

    for (i = 0; i < count; i++) {
        if (group_handle[i]) {
            return true;
        }
    }
    return false;
}

/*************************************************************************************************/
/*!
 *  \brief      获取gatt 角色 已有链路个数
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
u8 ble_comm_dev_get_connected_nums(u8 role)
{
    s8 i, nums = 0;
    u16 *group_handle;
    u8 count;

    if (GATT_ROLE_SERVER == role) {
        group_handle = gatt_server_conn_handle;
        count = SUPPORT_MAX_GATT_SERVER;
    } else {
        group_handle = gatt_client_conn_handle;
        count = SUPPORT_MAX_GATT_CLIENT;
    }

    for (i = 0; i < count; i++) {
        if (group_handle[i]) {
            nums++;
        }
    }
    return nums;
}

/*************************************************************************************************/
/*!
 *  \brief      获取index对应的连接 handle
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
u16 ble_comm_dev_get_handle(u8 index, u8 role)
{
    u16 *group_handle;
    u8 count;

    if (GATT_ROLE_SERVER == role) {
        group_handle = gatt_server_conn_handle;
        count = SUPPORT_MAX_GATT_SERVER;
    } else {
        group_handle = gatt_client_conn_handle;
        count = SUPPORT_MAX_GATT_CLIENT;
    }

    if (index < count) {
        return group_handle[index];
    } else {
        return 0;
    }
}

/*************************************************************************************************/
/*!
 *  \brief      获取连接 handle对应的gatt 角色
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
u8 ble_comm_dev_get_handle_role(u16 handle)
{
    u16 *group_handle;
    u8 count;
    u8 i;

    for (i = 0; i < SUPPORT_MAX_GATT_CLIENT; i++) {
        if (handle == gatt_client_conn_handle[i]) {
            return GATT_ROLE_CLIENT;
        }
    }
    return GATT_ROLE_SERVER;
}

/*************************************************************************************************/
/*!
 *  \brief      设置已连接handle的状态
 *
 *  \param      [in] state-- ble_state_e
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_comm_dev_set_handle_state(u16 handle, u8 role, u8 state)
{
    s8 index = ble_comm_dev_get_index(handle, role);

    if (index == INVAIL_INDEX) {
        return;
    }

    if (GATT_ROLE_SERVER == role) {
        gatt_server_conn_handle_state[index] = state;
    } else {
        gatt_client_conn_handle_state[index] = state;
    }
}

/*************************************************************************************************/
/*!
 *  \brief       获取已连接handle的状态
 *
 *  \param      [in]
 *
 *  \return      ble_state_e
 *
 *  \note
 */
/*************************************************************************************************/
u8 ble_comm_dev_get_handle_state(u16 handle, u8 role)
{
    s8 index = ble_comm_dev_get_index(handle, role);

    if (index == INVAIL_INDEX) {
        return 0;
    }

    if (GATT_ROLE_SERVER == role) {
        return gatt_server_conn_handle_state[index];
    } else {
        return gatt_client_conn_handle_state[index];
    }
}

/*************************************************************************************************/
/*!
 *  \brief      记录连接handle值
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void __comm_set_dev_handle_value(u16 handle, u8 role, u8 index)
{
    if (role == GATT_ROLE_CLIENT) {
        gatt_client_conn_handle[index] = handle;
    } else {
        gatt_server_conn_handle[index] = handle;
    }
}


/*************************************************************************************************/
/*!
 *  \brief      推送消息到app_core 任务，app处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_comm_bt_evnet_post(u32 arg_type, u8 priv_event, u8 *args, u32 value)
{
    struct sys_event e;
    e.type = SYS_BT_EVENT;
    e.arg  = (void *)arg_type;
    e.u.bt.event = priv_event;
    if (args) {
        memcpy(e.u.bt.args, args, 7);
    }
    e.u.bt.value = value;
    sys_event_notify(&e);
}

/*************************************************************************************************/
/*!
 *  \brief      检查是否需要等待配对连接成功操作
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
bool ble_comm_need_wait_encryption(u8 role)
{
    if (!STACK_IS_SUPPORT_SM_PAIR()) {
        return false;
    }

    if (!gatt_control_block->sm_config) {
        return false;
    }

    if (!sm_return_master_reconn_encryption()) {
        return false;
    }

    if (GATT_ROLE_CLIENT == role) {
        return gatt_control_block->sm_config->master_set_wait_security;
    }
    return gatt_control_block->sm_config->slave_set_wait_security;
}

/*************************************************************************************************/
/*!
 *  \brief      获取角色的handle类型
 *
 *  \param      [in]
 *
 *  \return     role
 *
 *  \note
 */
/*************************************************************************************************/
static u8 __just_conn_handle_role(u16 conn_handle)
{
    u8 role = ble_comm_dev_get_handle_role(conn_handle);
    log_info("just_handle=%04x,type= %d\n", conn_handle, role);
    return role;
}

/*************************************************************************************************/
/*!
 *  \brief      接受协议栈的sm消息，分发 server or client
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
//处理协议栈sm事件消息
void __ble_comm_cbk_sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    if (STACK_IS_SUPPORT_SM_PAIR()) {

        /* log_info("%s\n", __FUNCTION__); */
        CLR_HANDLER_ROLE();

        /* if(gatt_control_block->sm_config->sm_cb_packet_handler){ */
        /* if(0 == gatt_control_block->sm_config->sm_cb_packet_handler(packet_type,channel,packet,size)){ */
        /* return; */
        /* } */
        /* } */

        switch (packet_type) {
        case HCI_EVENT_PACKET:
            switch (hci_event_packet_get_type(packet)) {
            case SM_EVENT_JUST_WORKS_REQUEST:
            case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            case SM_EVENT_PASSKEY_INPUT_NUMBER:
            case SM_EVENT_PAIR_PROCESS: {
                log_info("Event_Type:%02x,packet_address:%08x", hci_event_packet_get_type(packet), packet);
                put_buf(packet, 16);
                sm_just_event_t *event = (void *)packet;
                ADD_HANDLER_ROLE(__just_conn_handle_role(event->con_handle));
            }
            break;
            }
            break;
        }

        if (STACK_IS_SUPPORT_GATT_SERVER()) {
            if (CHECK_HANDLER_ROLE(GATT_ROLE_SERVER)) {
                ble_gatt_server_sm_packet(packet_type, channel, packet, size);
            }
        }

        if (STACK_IS_SUPPORT_GATT_CLIENT()) {
            if (CHECK_HANDLER_ROLE(GATT_ROLE_CLIENT)) {
                ble_gatt_client_sm_packet(packet_type, channel, packet, size);
            }
        }
    }
}

/*************************************************************************************************/
/*!
 *  \brief      接受协议栈的hci & gatt消息，分发 server or client
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
//处理协议栈HCI,GATT事件消息
static void __ble_comm_cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    u16 tmp_handle;
    u8  role;
    s8  tmp_index;

    CLR_HANDLER_ROLE();

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
        case ATT_EVENT_CAN_SEND_NOW:
            if (0 == packet[1]) {
                ADD_HANDLER_ROLE(GATT_ROLE_SERVER);
            } else {
                ADD_HANDLER_ROLE(GATT_ROLE_CLIENT);
            }
            break;

        case ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE:
            ADD_HANDLER_ROLE(GATT_ROLE_SERVER);
            break;

        case ATT_EVENT_MTU_EXCHANGE_COMPLETE: {
            tmp_handle = little_endian_read_16(packet, 2);
            role = ble_comm_dev_get_handle_role(tmp_handle);
            u16 mtu = att_event_mtu_exchange_complete_get_MTU(packet) - 3;
            ADD_HANDLER_ROLE(__just_conn_handle_role(tmp_handle));
            /* log_info("ATT_MTU = %u\n", mtu); */
            ble_op_multi_att_set_send_mtu(tmp_handle, mtu);
        }
        break;

        case GAP_EVENT_ADVERTISING_REPORT:
            ADD_HANDLER_ROLE(GATT_ROLE_CLIENT);
            break;

        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {
#if EXT_ADV_MODE_EN
            case HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE: {
                tmp_handle = hci_subevent_le_enhanced_connection_complete_get_connection_handle(packet);
                if (BT_OP_SUCCESS == packet[3]) {
                    if (hci_subevent_le_enhanced_connection_complete_get_role(packet)) {
                        ADD_HANDLER_ROLE(GATT_ROLE_SERVER);
                        role = GATT_ROLE_SERVER;
                    } else {
                        ADD_HANDLER_ROLE(GATT_ROLE_CLIENT);
                        role = GATT_ROLE_CLIENT;
                    }

                    tmp_index = ble_comm_dev_get_idle_index(role);
                    if (tmp_index == INVAIL_INDEX) {
                        log_info("err_ext_connection:%d\n", role);
                        /* ble_op_disconnect(tmp_handle); */
                        ble_op_disconnect_ext(tmp_handle, 0x13);
                        CLR_HANDLER_ROLE();
                    } else {
                        s8 cur_cid =  ble_comm_dev_get_idle_index(role);
                        __comm_set_dev_handle_value(tmp_handle, role, cur_cid);
                        ble_op_multi_att_send_conn_handle(tmp_handle, cur_cid, role);
                    }
                } else if (BT_ERR_ADVERTISING_TIMEOUT == packet[3]) {
                    ADD_HANDLER_ROLE(GATT_ROLE_SERVER);
                } else {
                    if (hci_subevent_le_enhanced_connection_complete_get_role(packet)) {
                        ADD_HANDLER_ROLE(GATT_ROLE_SERVER);
                    } else {
                        ADD_HANDLER_ROLE(GATT_ROLE_CLIENT);
                    }
                }
            }
            break;
#endif
#if EXT_ADV_MODE_EN || PERIODIC_ADV_MODE_EN
            case HCI_SUBEVENT_LE_EXTENDED_ADVERTISING_REPORT:
                ADD_HANDLER_ROLE(GATT_ROLE_CLIENT);
                break;
#endif

            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE:
                if (BT_OP_SUCCESS == packet[3]) {
                    tmp_handle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                    if (hci_subevent_le_connection_complete_get_role(packet)) {
                        ADD_HANDLER_ROLE(GATT_ROLE_SERVER);
                        role = GATT_ROLE_SERVER;
                    } else {
                        ADD_HANDLER_ROLE(GATT_ROLE_CLIENT);
                        role = GATT_ROLE_CLIENT;
                    }

                    tmp_index = ble_comm_dev_get_idle_index(role);
                    if (tmp_index == INVAIL_INDEX) {
                        log_info("err_connection:%d\n", role);
                        /* ble_op_disconnect(tmp_handle); */
                        ble_op_disconnect_ext(tmp_handle, 0x13);
                        CLR_HANDLER_ROLE();
                    } else {
                        __comm_set_dev_handle_value(tmp_handle, role, tmp_index);
                        ble_op_multi_att_send_conn_handle(tmp_handle, tmp_index, role);
                    }
                } else if (BT_ERR_ADVERTISING_TIMEOUT == packet[3]) {
                    ADD_HANDLER_ROLE(GATT_ROLE_SERVER);
                    role = GATT_ROLE_SERVER;
                } else {
                    if (hci_subevent_le_connection_complete_get_role(packet)) {
                        ADD_HANDLER_ROLE(GATT_ROLE_SERVER);
                    } else {
                        ADD_HANDLER_ROLE(GATT_ROLE_CLIENT);
                    }
                }
                break;

            //default:
            //    break;

            //参数更新完成
            case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
                ADD_HANDLER_ROLE(__just_conn_handle_role(little_endian_read_16(packet, 4)));
                break;

            case HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE:
                ADD_HANDLER_ROLE(__just_conn_handle_role(little_endian_read_16(packet, 3)));
                break;

            case HCI_SUBEVENT_LE_PHY_UPDATE_COMPLETE:
                ADD_HANDLER_ROLE(__just_conn_handle_role(little_endian_read_16(packet, 4)));
                break;
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            tmp_handle = little_endian_read_16(packet, 3);
            ADD_HANDLER_ROLE(__just_conn_handle_role(tmp_handle));
            role = ble_comm_dev_get_handle_role(tmp_handle);
            tmp_index = ble_comm_del_dev_index(tmp_handle, role);
            /* log_info("HCI_EVENT_DISCONNECTION_COMPLETE(%04x): %0x\n",tmp_handle, packet[5]); */
            if (tmp_index != INVAIL_INDEX) {
                __comm_set_dev_handle_value(0, role, tmp_index);
                ble_op_multi_att_send_conn_handle(0, tmp_index, role);
            } else {
                log_info("error handle:%04x", tmp_handle);
            }
            break;

        case HCI_EVENT_VENDOR_REMOTE_TEST:
            log_info("HCI_EVENT_VENDOR_REMOTE_TEST: %d,%d\n", packet[1], packet[2]);
            break;

        case L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE:
            tmp_handle = little_endian_read_16(packet, 2);
            ADD_HANDLER_ROLE(__just_conn_handle_role(tmp_handle));
            break;

        case HCI_EVENT_ENCRYPTION_CHANGE:
            tmp_handle = little_endian_read_16(packet, 3);
            ADD_HANDLER_ROLE(__just_conn_handle_role(tmp_handle));
            tmp_index = ble_comm_dev_get_index(tmp_handle, GATT_ROLE_SERVER);
            if (tmp_index != INVAIL_INDEX) {
                log_info("HCI_EVENT_ENCRYPTION_CHANGE= %d,handle= %04x\n", packet[2], tmp_handle);
            }
            break;
        }
        break;

    default:
        break;

    }

    if (STACK_IS_SUPPORT_GATT_SERVER()) {
        if (CHECK_HANDLER_ROLE(GATT_ROLE_SERVER)) {
            ble_gatt_server_cbk_packet_handler(packet_type, channel, packet, size);
        }
    }

    if (STACK_IS_SUPPORT_GATT_CLIENT()) {
        if (CHECK_HANDLER_ROLE(GATT_ROLE_CLIENT)) {
            ble_gatt_client_cbk_packet_handler(packet_type, channel, packet, size);
        }
    }
}
/*************************************************************************************************/
/*!
 *  \brief      接受协议栈的输入key消息，分发 server or client
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
//重设passkey回调函数，在这里可以重新设置passkey
//passkey为6个数字组成，十万位、万位。。。。个位 各表示一个数字 高位不够为0
static void __ble_comm_cbk_passkey_input(u32 *key, u16 conn_handle)
{
    ADD_HANDLER_ROLE(__just_conn_handle_role(conn_handle));

    if (STACK_IS_SUPPORT_GATT_SERVER()) {
        if (CHECK_HANDLER_ROLE(GATT_ROLE_SERVER)) {
            ble_gatt_server_passkey_input(key, conn_handle);
        }
    }

    if (STACK_IS_SUPPORT_GATT_CLIENT()) {
        if (CHECK_HANDLER_ROLE(GATT_ROLE_CLIENT)) {
            ble_gatt_client_passkey_input(key, conn_handle);
        }
    }
}

/*************************************************************************************************/
/*!
 *  \brief       协议栈初始化过程回调,初始 sever 和 client
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
//协议栈回调初始化
void ble_profile_init(void)
{
    log_info("%s\n", __FUNCTION__);
    CLR_HANDLER_ROLE();

    if (!gatt_control_block) {
        log_error("gatt_control_block is null!!!\n");
        ASSERT(0);
    }

    if (STACK_IS_SUPPORT_GATT_SERVER() && STACK_IS_SUPPORT_GATT_CLIENT()) {
        ble_stack_gatt_role(2);
    } else if (STACK_IS_SUPPORT_GATT_CLIENT()) {
        ble_stack_gatt_role(1);
    } else {
        ble_stack_gatt_role(0);
    }

    if (STACK_IS_SUPPORT_SM_PAIR()) {
        if (gatt_control_block->sm_config) {
            le_device_db_init();
            sm_init();

            sm_set_io_capabilities(gatt_control_block->sm_config->io_capabilities);

            if (STACK_IS_SUPPORT_GATT_CLIENT()) {
                if (gatt_control_block->sm_config->io_capabilities == IO_CAPABILITY_DISPLAY_ONLY) {
                    sm_set_master_io_capabilities(IO_CAPABILITY_KEYBOARD_ONLY);
                }
            }
            if (STACK_IS_SUPPORT_SM_SUB_SC()) {
                log_info("enable SC_CONNECT");
                sm_set_authentication_requirements(gatt_control_block->sm_config->authentication_req_flags | SM_AUTHREQ_SECURE_CONNECTION);
            } else {
                sm_set_authentication_requirements(gatt_control_block->sm_config->authentication_req_flags);
            }
            sm_set_encryption_key_size_range(gatt_control_block->sm_config->min_key_size, gatt_control_block->sm_config->max_key_size);
            sm_set_master_request_pair(gatt_control_block->sm_config->master_security_auto_req);
            sm_set_request_security(gatt_control_block->sm_config->slave_security_auto_req);
            sm_event_callback_set(&__ble_comm_cbk_sm_packet_handler);

            if (gatt_control_block->sm_config->io_capabilities == IO_CAPABILITY_DISPLAY_ONLY ||
                gatt_control_block->sm_config->io_capabilities == IO_CAPABILITY_KEYBOARD_DISPLAY) {
                reset_PK_cb_register_ext(__ble_comm_cbk_passkey_input);
            }
        }
    }

    if (STACK_IS_SUPPORT_GATT_SERVER()) {
        att_server_register_packet_handler(__ble_comm_cbk_packet_handler);
        ble_gatt_server_profile_init();
    }

    if (STACK_IS_SUPPORT_GATT_CLIENT()) {
        gatt_client_register_packet_handler(__ble_comm_cbk_packet_handler);
        ble_gatt_client_profile_init();
    }

    // register for HCI events
    hci_event_callback_set(&__ble_comm_cbk_packet_handler);
    le_l2cap_register_packet_handler(&__ble_comm_cbk_packet_handler);

    if (STACK_IS_SUPPORT_GATT_CONNECT()) {
        u32 need_ram = att_need_ctrl_ramsize();
        need_ram += (gatt_control_block->mtu_size + gatt_control_block->cbuffer_size);
        if (!gatt_ram_buffer) {
            gatt_ram_buffer = malloc(need_ram);
            if (!gatt_ram_buffer) {
                log_info("att malloc size=%d fail!!!\n", need_ram);
                ASSERT(0);
                while (1);
            }
            memset(gatt_ram_buffer, 0, need_ram);
        }

        if (gatt_control_block->mtu_size > ble_vendor_set_default_att_mtu(gatt_control_block->mtu_size)) {
            log_info("mtu_size= %d,set fail!!!", gatt_control_block->mtu_size);
        }

        ble_op_multi_att_send_init(gatt_ram_buffer, need_ram, gatt_control_block->mtu_size);
        if (STACK_IS_SUPPORT_GATT_SERVER()) {
            multi_att_ccc_config_init(); //做1次就行
        }
    }
    log_info("%s end\n", __FUNCTION__);

}

/*************************************************************************************************/
/*!
 *  \brief       获取配置的cbuffer 可以填入数据长度
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
u32 ble_comm_cbuffer_vaild_len(u16 conn_handle)
{
    u32 vaild_len = 0;
    ble_op_multi_att_get_remain(conn_handle, &vaild_len);
    return vaild_len;
}

/*************************************************************************************************/
/*!
 *  \brief      gatt_common 模块开关，会调server & client 的模块开关
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_comm_module_enable(u8 en)
{
    log_info("mode_en:%d\n", en);

    if (STACK_IS_SUPPORT_GATT_SERVER()) {
        ble_gatt_server_module_enable(en);
    }

    if (STACK_IS_SUPPORT_GATT_CLIENT()) {
        ble_gatt_client_module_enable(en);
    }

}

/*************************************************************************************************/
/*!
 *  \brief      通过连接handle 断开主 or 从链路
 *
 *  \param      [in]
 *
 *  \return     gatt_op_ret_e
 *
 *  \note
 */
/*************************************************************************************************/
int ble_comm_disconnect_extend(u16 conn_handle, u8 reason)
{
    if (conn_handle) {
        u8 role = ble_comm_dev_get_handle_role(conn_handle);
        if (BLE_ST_SEND_DISCONN != ble_comm_dev_get_handle_state(conn_handle, role)) {
            ble_comm_dev_set_handle_state(conn_handle, role, BLE_ST_SEND_DISCONN);
            log_info(">>>send disconnect= %04x\n", conn_handle);
            /* ble_op_disconnect(conn_handle); */
            ble_op_disconnect_ext(conn_handle, reason);
        } else {
            log_info(">>>busy,wait disconnect= %04x\n", conn_handle);
        }
    }
    return GATT_OP_RET_SUCESS;
}

int ble_comm_disconnect(u16 conn_handle)
{
    return ble_comm_disconnect_extend(conn_handle, 0x13);
}

/*************************************************************************************************/
/*!
 *  \brief      gatt 模块初始化
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note       蓝牙协议栈初始化前调用
 */
/*************************************************************************************************/
void ble_comm_init(const gatt_ctrl_t *control_blk)
{
    int error_reset = 0;
    log_info("%s,%d\n", __FUNCTION__, __LINE__);
    log_info("ble_file: %s", __FILE__);

    log_info("gatt_config: %d,%d\n", SUPPORT_MAX_GATT_SERVER, SUPPORT_MAX_GATT_CLIENT);
    log_info("btstack_config: %d-%d-%d-%d\n", config_le_hci_connection_num, config_le_sm_support_enable, config_le_gatt_server_num, config_le_gatt_client_num);

    if (!control_blk) {
        printf("control_blk is null!!!\n");
        error_reset = 1;
    }

    if (error_reset) {
        printf("======error config!!!\n");
        ASSERT(0);
        while (1);
    }

    gatt_control_block = control_blk;

    if (SUPPORT_MAX_GATT_SERVER && STACK_IS_SUPPORT_GATT_SERVER()) {
        memset(gatt_server_conn_handle, 0, sizeof(gatt_server_conn_handle));
        memset(gatt_server_conn_handle_state, 0, sizeof(gatt_server_conn_handle_state));
        ble_gatt_server_init(gatt_control_block->server_config);
    }

    if (SUPPORT_MAX_GATT_CLIENT && STACK_IS_SUPPORT_GATT_CLIENT()) {
        memset(gatt_client_conn_handle, 0, sizeof(gatt_client_conn_handle));
        memset(gatt_client_conn_handle_state, 0, sizeof(gatt_client_conn_handle_state));
        ble_gatt_client_init(gatt_control_block->client_config);
    }

    log_info("%s end\n", __FUNCTION__);
}

/*************************************************************************************************/
/*!
 *  \brief      gatt common 模块退出
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note       蓝牙协议栈退出前调用
 */
/*************************************************************************************************/
void ble_comm_exit(void)
{
    log_info("%s\n", __FUNCTION__);

    if (STACK_IS_SUPPORT_GATT_SERVER()) {
        ble_gatt_server_exit();
    }

    if (STACK_IS_SUPPORT_GATT_CLIENT()) {
        ble_gatt_client_exit();
    }

    if (gatt_ram_buffer) {
        ble_op_multi_att_send_init(0, 0, 0);//set disable firstly
        free(gatt_ram_buffer);
        gatt_ram_buffer = 0;
    }
}

/*************************************************************************************************/
/*!
 *  \brief      配置gatt name
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note      没开 scan & adv 状态下可修改
 */
/*************************************************************************************************/
static char ble_device_name[BT_NAME_LEN_MAX + 1] = "JL_BT(BLE)";
static u8 ble_device_name_len = 10; //名字长度，不包含结束符
static const char ble_ext_name[] = "(BLE)";
void ble_comm_set_config_name(const char *name_p, u8 add_ext_name)
{
    log_info("%s\n", __FUNCTION__);

    if (!name_p) {
        log_info("ble default name(%d): %s\n", ble_device_name_len, ble_device_name);
        return;
    }

    ble_device_name_len = strlen(name_p);
    if (ble_device_name_len > BT_NAME_LEN_MAX) {
        log_error("cfg_name overflow!!!\n");
        ble_device_name_len = BT_NAME_LEN_MAX;
    }
    memcpy(ble_device_name, name_p, ble_device_name_len);

    if (add_ext_name) {
        u8 ext_name_len = strlen(ble_ext_name);
        if (ext_name_len + ble_device_name_len > BT_NAME_LEN_MAX) {
            log_error("add_name overflow!!!\n");
        } else {
            memcpy(&ble_device_name[ble_device_name_len], ble_ext_name, ext_name_len);
            ble_device_name_len += ext_name_len;
        }

    }
    ble_device_name[ble_device_name_len] = 0;//结束符
    log_info("ble name(%d): %s \n", ble_device_name_len, ble_device_name);
}

/*************************************************************************************************/
/*!
 *  \brief      获取 gap 的gatt name
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
const char *ble_comm_get_gap_name(void)
{
    return ble_device_name;
}

/*************************************************************************************************/
/*!
 *  \brief      配置dle 参数
 *
 *  \param      [in]
 *
 *  \return     gatt_op_ret_e
 *
 *  \note
 */
/*************************************************************************************************/
int ble_comm_set_connection_data_length(u16 conn_handle, u16 tx_octets, u16 tx_time)
{
    log_info("%s,%04x\n", __FUNCTION__, conn_handle);
    if (conn_handle) {
        return ble_op_set_data_length(conn_handle, tx_octets, tx_time);
    }
    return GATT_CMD_PARAM_ERROR;
}

/*************************************************************************************************/
/*!
 *  \brief      配置物理链路带宽
 *
 *  \param      [in]tx_phy,rx_phy,phy_options:  define in le_common_define.h
 *
 *  \return    gatt_op_ret_e
 *
 *  \note
 */
/*************************************************************************************************/
/* ble_comm_set_connection_data_phy(conn_handle,CONN_SET_CODED_PHY, CONN_SET_CODED_PHY,CONN_SET_PHY_OPTIONS_S8); */

int ble_comm_set_connection_data_phy(u16 conn_handle, u8 tx_phy, u8 rx_phy, u16 phy_options)
{
    u8 all_phys = 0;

    log_info("%s,%04x\n", __FUNCTION__, conn_handle);
    if (conn_handle) {
        return ble_op_set_ext_phy(conn_handle, all_phys, tx_phy, rx_phy, phy_options);
    }
    return GATT_CMD_PARAM_ERROR;
}

/*************************************************************************************************/
/*!
 *  \brief      检查预备发送的数据包长度是否能填入
 *
 *  \param      [in]
 *
 *  \return     true or false
 *
 *  \note
 */
/*************************************************************************************************/
bool ble_comm_att_check_send(u16 conn_handle, u16 pre_send_len)
{
    if (ble_comm_cbuffer_vaild_len(conn_handle) >= pre_send_len) {
        return true;
    } else {
        return false;
    }
}

/*************************************************************************************************/
/*!
 *  \brief      gatt (server or client) 数据发送接口
 *
 *  \param      [in]
 *
 *  \return     gatt_op_ret_e
 *
 *  \note
 */
/*************************************************************************************************/
static const char *gatt_op_error_string[] = {
    "GATT_CMD_RET_BUSY", //命令处理忙
    "GATT_CMD_PARAM_OVERFLOW",  //传参数溢出
    "GATT_CMD_OPT_FAIL",        //操作失败
    "GATT_BUFFER_FULL",         //缓存满了
    "GATT_BUFFER_ERROR",        //缓存出错
    "GATT_CMD_PARAM_ERROR",     //传参出错
    "GATT_CMD_STACK_NOT_RUN",   //协议栈没有运行
    "GATT_CMD_USE_CCC_FAIL",    //没有使能通知，导致NOTIFY或INDICATE发送失败，
};

int ble_comm_att_send_data(u16 conn_handle, u16 att_handle, u8 *data, u16 len, att_op_type_e op_type)
{
    gatt_op_ret_e ret = GATT_OP_RET_SUCESS;
    u16 tmp_16;

    if (!conn_handle) {
        return GATT_CMD_PARAM_ERROR;
    }

    switch (op_type) {
    case ATT_OP_READ:
        tmp_16  = 0x55A1;//fixed
        ret = ble_op_multi_att_send_data(conn_handle, att_handle, &tmp_16, 2, op_type);
        break;

    case ATT_OP_READ_LONG:
        tmp_16  = 0x55A2;//fixed
        ret = ble_op_multi_att_send_data(conn_handle, att_handle, &tmp_16, 2, op_type);
        break;

    default:
        ret = ble_op_multi_att_send_data(conn_handle, att_handle, data, len, op_type);
        break;
    }

    if (ret) {
        const char *err_string;

        int error_id = (int)0 - (int)GATT_CMD_RET_BUSY + (int)ret;
        if (error_id >= 0 && error_id < sizeof(gatt_op_error_string) / sizeof(char *)) {
            err_string = gatt_op_error_string[error_id];
        } else {
            err_string = "UNKNOW GATT_ERROR";
        }

        log_error("att_send_fail: %d!!!,%s", ret, err_string);
        log_error("param:%04x, %04x, %02x,len= %d", conn_handle, att_handle, op_type, len);
        /* put_buf(data,len); */
    }
    return ret;
}

#endif


