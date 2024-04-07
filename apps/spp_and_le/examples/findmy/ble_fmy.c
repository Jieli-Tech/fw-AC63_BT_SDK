/*********************************************************************************************
    *   Filename        : ble_fmy.c

    *   Description     :

    *   Author          : JM

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2024-03-05 15:14

    *   Copyright:(c)JIELI  2011-2026  @ , All Rights Reserved.
*********************************************************************************************/
#include "system/app_core.h"
#include "system/includes.h"

#include "app_config.h"
#include "app_action.h"
#include "app_main.h"

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
#include "gSensor/fmy/gsensor_api.h"
#include "ble_fmy.h"
#include "ble_fmy_profile.h"
#include "system/malloc.h"
#include "ble_fmy_cfg.h"
#include "ble_fmy_ota.h"
#include "ble_fmy_fmna.h"

#if CONFIG_APP_FINDMY

#if LE_DEBUG_PRINT_EN
#define log_info(x, ...)  printf("[BLE_FMY]" x "\r\n", ## __VA_ARGS__)
/* #define log_info(x, ...)  r_printf("[BLE_FMY]" x "\r\n", ## __VA_ARGS__) */
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

//配对绑定输入passkey 加密
#define PASSKEY_ENABLE             0

//测试NRF连接,工具不会主动发起交换流程,需要手动操作; 但设备可配置主动发起MTU长度交换请求
#define ATT_MTU_REQUEST_ENALBE     0    /*配置1,就是设备端主动发起交换*/

//ATT发送的包长,    note: 23 <=need >= MTU
#define ATT_LOCAL_MTU_SIZE        (247) /*一般是主机发起交换,如果主机没有发起,设备端也可以主动发起(ATT_MTU_REQUEST_ENALBE set 1)*/

//ATT缓存的buffer支持缓存数据包个数
#define ATT_PACKET_NUMS_MAX       (4)  //need 1k

//ATT缓存的buffer大小,  note: need >= 23,可修改
#define ATT_SEND_CBUF_SIZE        (ATT_PACKET_NUMS_MAX * (ATT_PACKET_HEAD_SIZE + ATT_LOCAL_MTU_SIZE))

// 广播周期 (unit:0.625ms)
#define ADV_INTERVAL_MIN          (160 * 5)//
// FMY最大连接数，不可修改
#define FMY_MAX_CONNECTIONS       2

//******************************************************************************************
fmy_glb_t  fmy_global_data;
fmy_vm_t fmy_vm_info;
//******************************************************************************************

static const char the_original_Name[] = "Find My Accessory";//"Accessory- Find My.";
static const char the_suffix_Name[] = "Acce- Find My.";
static const char FMY_ManufacturerName[64] = "Zhuhai Jieli Technology Co.,Ltd.";
static const char FMY_ModelName[64] = "JLtag";
static const uint8_t FMY_AccessoryCategory[8] = {FMY_CATEGORY_Finder, 0, 0, 0, 0, 0, 0, 0};
static const uint8_t read_tx_power_level = 4;//The Bluetooth LE transmit power level of the accessory shall be fixed at ≥ +4dBm

static const uint8_t fmy_battery_type = FMNA_BAT_NON_RECHARGEABLE;

//set capability
static const uint8_t FMY_AccessoryCapabilities[4] = {
    (FMY_CAPABILITY_SUPPORTS_PLAY_SOUND

#if FMY_OTA_SUPPORT_CONFIG
    | FMY_CAPABILITY_SUPPORTS_FW_UPDATE_SERVICE
#endif

#if TCFG_GSENSOR_ENABLE
    | FMY_CAPABILITY_SUPPORTS_MOTION_DETECTOR_UT
#endif

    | FMY_CAPABILITY_SUPPORTS_SN_LOOKUP_BY_BLE),
    0x00, 0x00, 0x00
};

#define FMY_CHECK_CAPABILITIES(bit)   ((FMY_AccessoryCapabilities[0] & bit) != 0)
//-------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------
static uint16_t fmy_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
static int fmy_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t fmyaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
static int fmy_event_packet_handler(int event, uint8_t *packet, uint16_t size, uint8_t *ext_param);
static int fmy_set_adv_enable(uint8_t enable);
static int fmy_set_adv_mode(uint8_t mode);
static int fmy_get_static_mac(uint8_t *mac);
static int fmy_set_static_mac(uint8_t *mac);
static int fmy_disconnect(uint16_t conn_handle, uint8_t reason);
//-------------------------------------------------------------------------------------

static const fmna_att_handle_t fmna_att_handle_table = {
    .pairing_control_point_handle = ATT_CHARACTERISTIC_4F860001_943B_49EF_BED4_2F730304427A_01_VALUE_HANDLE,
    .owner_cfg_control_point_handle = ATT_CHARACTERISTIC_4F860002_943B_49EF_BED4_2F730304427A_01_VALUE_HANDLE,
    .owner_info_porint_handle = ATT_CHARACTERISTIC_4F860004_943B_49EF_BED4_2F730304427A_01_VALUE_HANDLE,
    .non_owner_control_point_handle = ATT_CHARACTERISTIC_4F860003_943B_49EF_BED4_2F730304427A_01_VALUE_HANDLE,
    .debug_control_point_handle = ATT_CHARACTERISTIC_4F860005_943B_49EF_BED4_2F730304427A_01_VALUE_HANDLE,
    .firmware_update_handle = ATT_CHARACTERISTIC_94110001_6D9B_4225_A4F1_6A4A7F01B0DE_01_VALUE_HANDLE,
};

//-------------------------------------------------------------------------------------
static const sm_cfg_t fmy_sm_init_config = {
    .slave_security_auto_req = 0,
    .slave_set_wait_security = 0,

#if PASSKEY_ENABLE
    .io_capabilities = IO_CAPABILITY_DISPLAY_ONLY,
#else
    .io_capabilities = IO_CAPABILITY_NO_INPUT_NO_OUTPUT,
#endif

    .authentication_req_flags = SM_AUTHREQ_BONDING,// | SM_AUTHREQ_MITM_PROTECTION,
    .min_key_size = 7,
    .max_key_size = 16,
    .sm_cb_packet_handler = NULL,
};

static const gatt_server_cfg_t fmy_server_init_cfg = {
    .att_read_cb = &fmy_att_read_callback,
    .att_write_cb = &fmy_att_write_callback,
    .event_packet_handler = &fmy_event_packet_handler,
};

static gatt_ctrl_t fmy_gatt_control_block = {
    //public
    .mtu_size = ATT_LOCAL_MTU_SIZE,
    .cbuffer_size = ATT_SEND_CBUF_SIZE,
    .multi_dev_flag	= 0,

#if CONFIG_BT_GATT_SERVER_NUM
    .server_config = &fmy_server_init_cfg,
#else
    .server_config = NULL,
#endif

    .client_config = NULL,

#if CONFIG_BT_SM_SUPPORT_ENABLE
    .sm_config = &fmy_sm_init_config,
#else
    .sm_config = NULL,
#endif

    .hci_cb_packet_handler = NULL,
};

//============================================================================================
bool fmy_check_capabilities_is_enalbe(uint8_t cap)
{
    if (FMY_CHECK_CAPABILITIES(cap)) {
        return true;
    } else {
        return false;
    }
}

/*************************************************************************************************/
/*!
 *  \brief      vm 信息存储
 *
 *  \param      [in] info, rw_flag:0-read,1-write
 *
 *  \return     true or false
 *
 *  \note
 */
/*************************************************************************************************/
#define	FMY_VM_HEAD_TAG (0xA1)
bool fmy_vm_deal(fmy_vm_t *info, uint8_t rw_flag)
{
    int ret;
    int vm_len = sizeof(fmy_vm_t);

    log_info("-fmy_info vm_do:%d", rw_flag);

    if (rw_flag == 0) {
        ret = syscfg_read(CFG_FMY_INFO, (uint8_t *)info, vm_len);
        if (!ret) {
            log_info("-null--");
        } else {
            if (FMY_VM_HEAD_TAG == info->head_tag) {
                log_info("-exist--");
                log_info_hexdump((uint8_t *)info, vm_len);
                return true;
            }
        }
        memset(info, 0, vm_len);
        info->head_tag = FMY_VM_HEAD_TAG;
        return false;

    } else {
        syscfg_write(CFG_FMY_INFO, (uint8_t *)info, vm_len);
        log_info("-write--");
        log_info_hexdump((uint8_t *)info, vm_len);
        return true;
    }
}


/*************************************************************************************************/
/*!
 *  \brief      控制profile 的character的可见性模式
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void fmy_set_profile_switch(uint16_t conn_handle, uint8_t mode)
{
    log_info("%s: %d", __FUNCTION__, mode);
    __fydata->profile_mode = mode;
    uint8_t fw_update_enable = 0;

    if (FMY_CHECK_CAPABILITIES(FMY_CAPABILITY_SUPPORTS_FW_UPDATE_SERVICE)) {
        fw_update_enable = 1;
    }

    switch (mode) {
    case PROFILE_MODE_UNPAIR:
        log_info("profile unpair");
        att_set_handle_enable(conn_handle, UNPAIR_CONTROL_START_HANDLE, UNPAIR_CONTROL_END_HANDLE, true);
#if FMY_FMCA_TEST_MODE
        att_set_handle_enable(conn_handle, CFG_CONTROL_START_HANDLE, CFG_CONTROL_END_HANDLE, false);
#else
        att_set_handle_enable(conn_handle, CFG_CONTROL_START_HANDLE, CFG_CONTROL_END_HANDLE, true);//打开cfg_control,fix接收配对过程可以取消配对
#endif
        att_set_handle_enable(conn_handle, NON_OWNER_CONTROL_START_HANDLE, NON_OWNER_CONTROL_END_HANDLE, false);
        att_set_handle_enable(conn_handle, PAIRED_OWNER_INFO_START_HANDLE, PAIRED_OWNER_INFO_END_HANDLE, false);
        att_set_handle_enable(conn_handle, DEBUG_CONTROL_START_HANDLE, DEBUG_CONTROL_END_HANDLE, FMY_DEBUG_SERVICE_ENABLE);
        att_set_handle_enable(conn_handle, FW_UPDATE_START_HANDLE, FW_UPDATE_END_HANDLE, fw_update_enable);
        break;

    case PROFILE_MODE_OWNER:
        log_info("profile owner");
        att_set_handle_enable(conn_handle, UNPAIR_CONTROL_START_HANDLE, UNPAIR_CONTROL_END_HANDLE, false);
        att_set_handle_enable(conn_handle, CFG_CONTROL_START_HANDLE, CFG_CONTROL_END_HANDLE, true);
        //att_set_handle_enable(conn_handle, NON_OWNER_CONTROL_START_HANDLE,NON_OWNER_CONTROL_END_HANDLE,false);
        att_set_handle_enable(conn_handle, NON_OWNER_CONTROL_START_HANDLE, NON_OWNER_CONTROL_END_HANDLE, true);
        att_set_handle_enable(conn_handle, PAIRED_OWNER_INFO_START_HANDLE, PAIRED_OWNER_INFO_END_HANDLE, true);
        att_set_handle_enable(conn_handle, DEBUG_CONTROL_START_HANDLE, DEBUG_CONTROL_END_HANDLE, FMY_DEBUG_SERVICE_ENABLE);
        att_set_handle_enable(conn_handle, FW_UPDATE_START_HANDLE, FW_UPDATE_END_HANDLE, fw_update_enable);
        break;

    case PROFILE_MODE_NON_OWNER:
        log_info("profile no owner");
        att_set_handle_enable(conn_handle, UNPAIR_CONTROL_START_HANDLE, UNPAIR_CONTROL_END_HANDLE, false);
        att_set_handle_enable(conn_handle, CFG_CONTROL_START_HANDLE, CFG_CONTROL_END_HANDLE, false);
        att_set_handle_enable(conn_handle, NON_OWNER_CONTROL_START_HANDLE, NON_OWNER_CONTROL_END_HANDLE, true);
        att_set_handle_enable(conn_handle, PAIRED_OWNER_INFO_START_HANDLE, PAIRED_OWNER_INFO_END_HANDLE, true);
        att_set_handle_enable(conn_handle, DEBUG_CONTROL_START_HANDLE, DEBUG_CONTROL_END_HANDLE, FMY_DEBUG_SERVICE_ENABLE);
        att_set_handle_enable(conn_handle, FW_UPDATE_START_HANDLE, FW_UPDATE_END_HANDLE, false);
        break;

    case PROFILE_MODE_SEPARATED:
        log_info("profile separated");
        att_set_handle_enable(conn_handle, UNPAIR_CONTROL_START_HANDLE, UNPAIR_CONTROL_END_HANDLE, false);
        att_set_handle_enable(conn_handle, CFG_CONTROL_START_HANDLE, CFG_CONTROL_END_HANDLE, false);
        att_set_handle_enable(conn_handle, NON_OWNER_CONTROL_START_HANDLE, NON_OWNER_CONTROL_END_HANDLE, true);
        att_set_handle_enable(conn_handle, PAIRED_OWNER_INFO_START_HANDLE, PAIRED_OWNER_INFO_END_HANDLE, false);
        att_set_handle_enable(conn_handle, DEBUG_CONTROL_START_HANDLE, DEBUG_CONTROL_END_HANDLE, FMY_DEBUG_SERVICE_ENABLE);
        att_set_handle_enable(conn_handle, FW_UPDATE_START_HANDLE, FW_UPDATE_END_HANDLE, fw_update_enable);
        break;

    case PROFILE_MODE_NEARBY:
    default:
        log_info("profile nearby");
        att_set_handle_enable(conn_handle, UNPAIR_CONTROL_START_HANDLE, UNPAIR_CONTROL_END_HANDLE, false);
        att_set_handle_enable(conn_handle, CFG_CONTROL_START_HANDLE, CFG_CONTROL_END_HANDLE, false);
        att_set_handle_enable(conn_handle, NON_OWNER_CONTROL_START_HANDLE, NON_OWNER_CONTROL_END_HANDLE, false);
        att_set_handle_enable(conn_handle, PAIRED_OWNER_INFO_START_HANDLE, PAIRED_OWNER_INFO_END_HANDLE, false);
        att_set_handle_enable(conn_handle, DEBUG_CONTROL_START_HANDLE, DEBUG_CONTROL_END_HANDLE, FMY_DEBUG_SERVICE_ENABLE);
        att_set_handle_enable(conn_handle, FW_UPDATE_START_HANDLE, FW_UPDATE_END_HANDLE, fw_update_enable);
        break;

    }
}

/*************************************************************************************************/
/*!
 *  \brief      更新电量
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void fmy_update_battery_level(void)
{

#if TCFG_SYS_LVD_EN
    uint8_t  bat_level = get_cur_battery_level();//0~9
    log_info("read vbat:%d\n", bat_level);

    if (bat_level > 8) {
        __fydata->battery_level = BAT_STATE_FULL;
    } else if (bat_level < 2) {
        __fydata->battery_level = BAT_STATE_CRITICALLY_LOW;
    } else if (bat_level < 4) {
        __fydata->battery_level = BAT_STATE_LOW;
    } else {
        __fydata->battery_level = BAT_STATE_MEDIUM;
    }
#else
    __fydata->battery_level = BAT_STATE_FULL;
#endif

    log_info("%s,bat_lev= %d", __FUNCTION__, __fydata->battery_level);
}


//检查连接的收集地址是否已配对绑定
static void fmy_check_connect_remote(uint16_t conn_handle, uint8_t addr_type, uint8_t *addr)
{
    uint8_t temp_addr[6];
    swapX(addr, temp_addr, 6);
    if (fmna_pm_peer_count() && fmna_connection_is_fmna_paired()) {
        if (ble_list_check_addr_is_exist(temp_addr, addr_type)) {
            log_info("remote in list");
        } else {
            log_info("remote not in list");
        }
        fmy_set_profile_switch(conn_handle, PROFILE_MODE_NON_OWNER);
    } else {
        log_info("device no pair");
        fmy_set_profile_switch(conn_handle, PROFILE_MODE_UNPAIR);
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
static int fmy_event_packet_handler(int event, uint8_t *packet, uint16_t size, uint8_t *ext_param)
{
    /* log_info("event: %02x,size= %d\n",event,size); */
    int ret = 0;
    uint16_t tmp16;

    switch (event) {

    case GATT_COMM_EVENT_CAN_SEND_NOW:
        break;

    case GATT_COMM_EVENT_SERVER_INDICATION_COMPLETE:
        log_info("INDICATION_COMPLETE:con_handle= %04x,att_handle= %04x", \
                 little_endian_read_16(packet, 0), little_endian_read_16(packet, 2));
        fmna_gatt_platform_recieve_indication_response(little_endian_read_16(packet, 0), little_endian_read_16(packet, 2));
        break;


    case GATT_COMM_EVENT_CONNECTION_COMPLETE:
        __fydata->cur_con_handle = little_endian_read_16(packet, 0);
        tmp16 = little_endian_read_16(ext_param, 14 + 0);//interval
        __fydata->adv_fmna_state = FMNA_SM_CONNECTING;

        log_info("connection_handle:%04x", little_endian_read_16(packet, 0));
        log_info("connection_handle:%04x, rssi= %d", __fydata->cur_con_handle, ble_vendor_get_peer_rssi(__fydata->cur_con_handle));
        log_info("peer_address_info:");
        log_info_hexdump(&ext_param[7], 7);

        log_info("con_interval = %d", little_endian_read_16(ext_param, 14 + 0));
        log_info("con_latency = %d", little_endian_read_16(ext_param, 14 + 2));
        log_info("cnn_timeout = %d", little_endian_read_16(ext_param, 14 + 4));

#if FMY_SUPPORT_TEST_BOX_MODE
        if (__fydata->testbox_mode_enable) {
            log_info("testbox connect,timeout to reset!!!");
            sys_timeout_add(0, fmy_systerm_reset, 4000);
            break;
        }
#endif

        ble_comm_set_config_name(the_suffix_Name, 0);
        /* memcpy(__fydata->cur_remote_address_info, &ext_param[7], 7); */
        fmy_check_connect_remote(__fydata->cur_con_handle, ext_param[7], &ext_param[8]);
        fmna_connection_connected_handler(__fydata->cur_con_handle, tmp16);


#if ATT_MTU_REQUEST_ENALBE
        att_server_set_exchange_mtu(__fydata->cur_con_handle);/*主动请求MTU长度交换*/
#endif

#if TCFG_SYS_LVD_EN && FMY_FMCA_TEST_MODE == 0
        fmy_update_battery_level();
#endif
        break;

    case GATT_COMM_EVENT_DISCONNECT_COMPLETE:
        log_info("disconnect_handle:%04x,reason= %02x", little_endian_read_16(packet, 0), packet[2]);
        if (__fydata->cur_con_handle == little_endian_read_16(packet, 0)) {
            __fydata->cur_con_handle = 0;
        }
        fmna_connection_disconnected_handler(little_endian_read_16(packet, 0), packet[2]);
        break;

    case GATT_COMM_EVENT_ENCRYPTION_CHANGE:
        log_info("ENCRYPTION_CHANGE:handle=%04x,state=%d,process =%d", little_endian_read_16(packet, 0), packet[2], packet[3]);
        if (packet[2] == 0) {
            log_info("BOND OK");
            ble_comm_set_config_name(the_original_Name, 0);
            if (fmna_connection_is_fmna_paired()) {
                log_info("the same apple_id connect");
                //绑定后的配置，默认进入owner访问,不同手机用绑定的apple id，也可以加密成功的
                fmy_set_profile_switch(little_endian_read_16(packet, 0), PROFILE_MODE_OWNER);
            }
            fmna_connection_encryption_change_complete(little_endian_read_16(packet, 0), true);
            fmy_pairing_timeout_stop();
        } else {
            log_info("BOND fail!!!");
            fmy_set_profile_switch(little_endian_read_16(packet, 0), PROFILE_MODE_NON_OWNER);
        }
        break;

    case GATT_COMM_EVENT_CONNECTION_UPDATE_COMPLETE:
        log_info("conn_param update_complete:%04x", little_endian_read_16(packet, 0));
        log_info("update_interval = %d", little_endian_read_16(ext_param, 6 + 0));
        log_info("update_latency = %d", little_endian_read_16(ext_param, 6 + 2));
        log_info("update_timeout = %d", little_endian_read_16(ext_param, 6 + 4));
        fmna_connection_conn_param_update_handler(little_endian_read_16(packet, 0), little_endian_read_16(ext_param, 6 + 0));
        break;

    case GATT_COMM_EVENT_CONNECTION_UPDATE_REQUEST_RESULT:
        break;

    case GATT_COMM_EVENT_MTU_EXCHANGE_COMPLETE:
        log_info("con_handle= %02x, ATT MTU = %u", little_endian_read_16(packet, 0), little_endian_read_16(packet, 2));
        fmna_gatt_set_mtu_size(little_endian_read_16(packet, 0), little_endian_read_16(packet, 2));
        break;

    case GATT_COMM_EVENT_SERVER_STATE:
        log_info("server_state: handle=%02x,%02x", little_endian_read_16(packet, 1), packet[0]);
        break;

    case GATT_COMM_EVENT_SM_PASSKEY_INPUT: {
        uint32_t *key = (uint32_t *)little_endian_read_32(packet, 2);
        *key = 888888;
        log_info("input_key:%6u", *key);
    }
    break;

    case GATT_COMM_EVENT_ENCRYPTION_REQUEST:
        log_info("GATT_COMM_EVENT_ENCRYPTION_REQUEST:%02x", ext_param[0]);

        if (packet[3] == LINK_ENCRYPTION_RECONNECT) {
            log_info("PAIR_RECONNECT START");
        } else if (packet[3] == LINK_ENCRYPTION_PAIR_JUST_WORKS) {
            log_info("PAIR_JUST_WORKS START");
            if (!fmna_connection_pair_request_check(little_endian_read_16(packet, 0))) {
                //设备已被绑定，则不允许再次走配对绑定流程
                log_info("reject new pair request!!!");
                ret = -1;
            }
        }
        break;

    default:
        break;
    }
    return ret;
}

//公共处理read的行为回复
static uint16_t fmy_att_read_callback_ack_handle(uint16_t offset, uint8_t *buffer, uint16_t buffer_size, uint8_t *ack_data, uint16_t ack_data_len)
{
    uint16_t ret_att_value_len = ack_data_len;

    if ((offset >= ret_att_value_len) || (offset + buffer_size) > ret_att_value_len) {
        return 0;
    }

    if (buffer && ack_data_len) {
        memcpy(buffer, &ack_data[offset], buffer_size);
        ret_att_value_len = buffer_size;
    }
    return ret_att_value_len;
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
static uint16_t fmy_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    uint16_t  att_value_len = 0;
    uint16_t handle = att_handle;

    log_info("read_callback,conn_handle =%04x, handle=%04x,buffer=%08x", connection_handle, handle, (uint32_t)buffer);

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
            log_info("\n------read gap_name: %s", gap_name);
        }
    }
    break;

    case ATT_CHARACTERISTIC_2a07_01_VALUE_HANDLE:
        log_info("read_tx_power = %d", read_tx_power_level);
        att_value_len = fmy_att_read_callback_ack_handle(offset, buffer, buffer_size, &read_tx_power_level, sizeof(read_tx_power_level));
        break;


    case ATT_CHARACTERISTIC_6AA50001_6352_4D57_A7B4_003A416FBB0B_01_VALUE_HANDLE:
        att_value_len = fmy_att_read_callback_ack_handle(offset, buffer, buffer_size, fmna_get_product_data(), 8);
        break;

    case ATT_CHARACTERISTIC_6AA50002_6352_4D57_A7B4_003A416FBB0B_01_VALUE_HANDLE:
        att_value_len = fmy_att_read_callback_ack_handle(offset, buffer, buffer_size, FMY_ManufacturerName, sizeof(FMY_ManufacturerName));
        break;

    case ATT_CHARACTERISTIC_6AA50003_6352_4D57_A7B4_003A416FBB0B_01_VALUE_HANDLE:
        att_value_len = fmy_att_read_callback_ack_handle(offset, buffer, buffer_size, FMY_ModelName, sizeof(FMY_ModelName));
        break;

    case ATT_CHARACTERISTIC_6AA50005_6352_4D57_A7B4_003A416FBB0B_01_VALUE_HANDLE:
        att_value_len = fmy_att_read_callback_ack_handle(offset, buffer, buffer_size, fmna_get_accessory_category(), 8);
        break;

    case ATT_CHARACTERISTIC_6AA50006_6352_4D57_A7B4_003A416FBB0B_01_VALUE_HANDLE:
        att_value_len = fmy_att_read_callback_ack_handle(offset, buffer, buffer_size, FMY_AccessoryCapabilities, sizeof(FMY_AccessoryCapabilities));
        break;

    case ATT_CHARACTERISTIC_6AA50007_6352_4D57_A7B4_003A416FBB0B_01_VALUE_HANDLE: {
        uint32_t tmp_version = fmna_version_get_fw_version();
        att_value_len = fmy_att_read_callback_ack_handle(offset, buffer, buffer_size, &tmp_version, 4);
    }
    break;

    case ATT_CHARACTERISTIC_6AA50008_6352_4D57_A7B4_003A416FBB0B_01_VALUE_HANDLE:
        att_value_len = fmy_att_read_callback_ack_handle(offset, buffer, buffer_size, fmna_version_get_network_version(), 4);
        break;

    case ATT_CHARACTERISTIC_6AA50009_6352_4D57_A7B4_003A416FBB0B_01_VALUE_HANDLE:
        att_value_len = fmy_att_read_callback_ack_handle(offset, buffer, buffer_size, &fmy_battery_type, sizeof(fmy_battery_type));
        break;

    case ATT_CHARACTERISTIC_6AA5000A_6352_4D57_A7B4_003A416FBB0B_01_VALUE_HANDLE:
        att_value_len = fmy_att_read_callback_ack_handle(offset, buffer, buffer_size, &__fydata->battery_level, sizeof(fmy_battery_type));
        break;

    case ATT_CHARACTERISTIC_4F860001_943B_49EF_BED4_2F730304427A_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_4F860002_943B_49EF_BED4_2F730304427A_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_4F860003_943B_49EF_BED4_2F730304427A_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_4F860004_943B_49EF_BED4_2F730304427A_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_4F860005_943B_49EF_BED4_2F730304427A_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_94110001_6D9B_4225_A4F1_6A4A7F01B0DE_01_CLIENT_CONFIGURATION_HANDLE:
        if (buffer) {
            buffer[0] = ble_gatt_server_characteristic_ccc_get(connection_handle, handle);
            buffer[1] = 0;
        }
        att_value_len = 2;
        break;

    default:
        break;
    }

    log_info("att_value_len= %d", att_value_len);
    if (att_value_len && buffer) {
        log_info("read handle= %04x respond data(%d)", handle, att_value_len);
        log_info_hexdump(buffer, att_value_len);
    }
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
// @param fmyaction - ATT_TRANSACTION_MODE_NONE for regular writes, ATT_TRANSACTION_MODE_ACTIVE for prepared writes and ATT_TRANSACTION_MODE_EXECUTE
// @param offset into the value - used for queued writes and long attributes
// @param buffer
// @param buffer_size
// @param signature used for signed write commmands
// @returns 0 if write was ok, ATT_ERROR_PREPARE_QUEUE_FULL if no space in queue, ATT_ERROR_INVALID_OFFSET if offset is larger than max buffer

static int fmy_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t fmyaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    int result = 0;
    uint16_t handle = att_handle;

#if FMY_SUPPORT_TEST_BOX_MODE
    if (__fydata->testbox_mode_enable) {
        log_info("testbox mode write!");
        return 0;
    }
#endif

    log_info("write_callback,conn_handle =%04x, handle =%04x,size =%d", connection_handle, handle, buffer_size);

    switch (handle) {

    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        break;

    case ATT_CHARACTERISTIC_4F860001_943B_49EF_BED4_2F730304427A_01_VALUE_HANDLE:
        log_info("---Pairing Control Point write data:%04x,%d", handle, buffer_size);
        log_info_hexdump(buffer, buffer_size);
        fmna_gatt_pairing_char_authorized_write_handler(connection_handle, 0, buffer_size, buffer);
        break;
    case ATT_CHARACTERISTIC_4F860002_943B_49EF_BED4_2F730304427A_01_VALUE_HANDLE:
        log_info("---Configuration Control Point write data:%04x,%d", handle, buffer_size);
        log_info_hexdump(buffer, buffer_size);
        fmna_gatt_config_char_write_handler(connection_handle, 0, buffer_size, buffer);
        break;

    case ATT_CHARACTERISTIC_4F860003_943B_49EF_BED4_2F730304427A_01_VALUE_HANDLE:
        log_info("---Non Owner Control Point:%04x,%d", handle, buffer_size);
        log_info_hexdump(buffer, buffer_size);
        fmna_gatt_nonown_char_write_handler(connection_handle, 0, buffer_size, buffer);
        break;

    case ATT_CHARACTERISTIC_94110001_6D9B_4225_A4F1_6A4A7F01B0DE_01_VALUE_HANDLE:
        log_info("---firmware update write data:%04x,%d", handle, buffer_size);
        log_info_hexdump(buffer, buffer_size);
        if (FMY_CHECK_CAPABILITIES(FMY_CAPABILITY_SUPPORTS_FW_UPDATE_SERVICE)) {
            fmna_gatt_uarp_char_write_handler(connection_handle, 0, buffer_size, buffer);
        }
        break;

    case ATT_CHARACTERISTIC_4F860004_943B_49EF_BED4_2F730304427A_01_VALUE_HANDLE:
        log_info("---Ower Information Control Point write data:%04x,%d", handle, buffer_size);
        log_info_hexdump(buffer, buffer_size);
        fmna_gatt_paired_owner_char_write_handler(connection_handle, 0, buffer_size, buffer);
        break;

    case ATT_CHARACTERISTIC_4F860005_943B_49EF_BED4_2F730304427A_01_VALUE_HANDLE:
        log_info("---Debug Control Point rx data:%04x,%d", handle, buffer_size);
        log_info_hexdump(buffer, buffer_size);
        fmna_debug_control_point_rx_handler(connection_handle, buffer, buffer_size);
        break;


    case ATT_CHARACTERISTIC_4F860001_943B_49EF_BED4_2F730304427A_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_4F860002_943B_49EF_BED4_2F730304427A_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_4F860003_943B_49EF_BED4_2F730304427A_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_4F860004_943B_49EF_BED4_2F730304427A_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_4F860005_943B_49EF_BED4_2F730304427A_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_94110001_6D9B_4225_A4F1_6A4A7F01B0DE_01_CLIENT_CONFIGURATION_HANDLE:
        /* fmy_send_connetion_updata_deal(connection_handle); */
        log_info("\n------write ccc:%04x,%02x", handle, buffer[0]);
        ble_gatt_server_characteristic_ccc_set(connection_handle, handle, buffer[0]);
        break;

    default:
        break;
    }

    return 0;
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
    ble_comm_init(&fmy_gatt_control_block);
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
    log_info("%s", __FUNCTION__);
    log_info("ble_file: %s", __FILE__);

    __fydata->pairing_mode_enable = 0;//power on control pairing state
    __fydata->testbox_mode_enable = 0;
    fmy_vm_deal(__fy_vm, 0);

    if (__fy_vm->reset_config) {
        __fydata->pairing_mode_enable = 1;
        __fy_vm->reset_config = 0;
        fmy_vm_deal(__fy_vm, 1);
        log_info("reset config to pair adv ");
    }

    ble_comm_set_config_name(the_suffix_Name, 0);
    __fydata->cur_con_handle = 0;

    fmy_fmna_adv_cofig_init();
    ble_gatt_server_set_profile(findmy_profile_data, sizeof(findmy_profile_data));
    ble_module_enable(1);

    log_info("capability= %02x, ota= %d", FMY_AccessoryCapabilities[0], FMY_OTA_SUPPORT_CONFIG);
    fmna_set_accessory_category(FMY_AccessoryCategory);
    fmy_fmna_init();

    // set findmy connect link
    if (config_le_hci_connection_num < FMY_MAX_CONNECTIONS) {
        log_info("config_le_hci_connection_num has:%d, fmy_max_connections need:%d", config_le_hci_connection_num, FMY_MAX_CONNECTIONS);
        ASSERT(0, "no enough ble conect links for findmy!!");
    }
    fmna_connection_set_sys_max_connections(FMY_MAX_CONNECTIONS);
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
void ble_module_enable(uint8_t en)
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
void ble_server_send_test_key_num(uint8_t key_num)
{
    if (__fydata->cur_con_handle) {
        if (get_remote_test_flag()) {
            ble_op_test_key_num(__fydata->cur_con_handle, key_num);
        } else {
            log_info("-not conn testbox\n");
        }
    }
}

#endif



