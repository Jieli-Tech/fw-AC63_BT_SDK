/*********************************************************************************************
    *   Filename        : ble_24g_server.c

    *   Description     :

    *   Author          :LW

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2021-09-29 11:16

    *   Copyright:(c)JIELI  2021-2026  @ , All Rights Reserved.
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
#include "ble_24g_profile.h"

#if CONFIG_APP_CONN_24G && CONFIG_BT_GATT_SERVER_NUM

#if LE_DEBUG_PRINT_EN
//#define log_info            y_printf
#define log_info(x, ...)  printf("[24G_SER]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

//ATT发送的包长,    note: 23 <=need >= MTU
#define ATT_LOCAL_MTU_SIZE        (64)
//ATT缓存的buffer支持缓存数据包个数
#define ATT_PACKET_NUMS_MAX       (2)

//ATT缓存的buffer大小,  note: need >= 23,可修改
#define ATT_SEND_CBUF_SIZE        (ATT_PACKET_NUMS_MAX * (ATT_PACKET_HEAD_SIZE + ATT_LOCAL_MTU_SIZE))

// 广播周期 (unit:0.625ms)
#define ADV_INTERVAL_MIN               (16 * 1)//10ms
//ADV_DIRECT_IND_LOW,定义interval 和 超时切换广播

static  u16 direct_adv_timeout_number  = 0;
static  u32 pair_reconnect_adv_timeout = 5000;/*回连广播持续时间,ms*/
#define PAIR_DIRECT_LOW_ADV_TIME       (5000) //ADV_DIRECT_IND_LOW,低频率定向广播时间,5s
#define PAIR_DIRECT_LOW_ADV_INTERVAL   (0x30) //ADV_DIRECT_IND_LOW,interval 0x30

//输入passkey 加密
#define PASSKEY_ENABLE            0//是否使用passkey加密
//是否使能从机绑定流程
#define PEER_INFO_BOND            1

static volatile hci_con_handle_t conn_24g_handle;
//---------------
//连接参数更新请求设置
//是否使能参数请求更新,0--disable, 1--enable
static uint8_t conn_24g_connection_update_enable = 0; ///0--disable, 1--enable
//加密初始化配置
static const sm_cfg_t sm_init_config = {
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

//gatt 初始化gatt_server配置
extern const gatt_server_cfg_t conn_24g_server_init_cfg;
//gatt 控制块初始化
gatt_ctrl_t conn_24g_gatt_server_control_block = {
    //public
    .mtu_size = ATT_LOCAL_MTU_SIZE,
    .cbuffer_size = ATT_SEND_CBUF_SIZE,
    .multi_dev_flag	= 1,

    //config
#if CONFIG_BT_GATT_SERVER_NUM
    .server_config = &conn_24g_server_init_cfg,
#else
    .server_config = NULL,
#endif

#if CONFIG_BT_SM_SUPPORT_ENABLE
    .sm_config = &sm_init_config,
#else
    .sm_config = NULL,
#endif
    //cbk,event handle
    .hci_cb_packet_handler = NULL,
};

//当前请求的参数表index
//参数表
static const struct conn_update_param_t conn_24g_connection_param_table[] = {
    {6, 9, 0, 600},//android
};

//可用的参数组数
#define CONN_PARAM_TABLE_CNT      (sizeof(conn_24g_connection_param_table)/sizeof(struct conn_update_param_t))

#define EIR_TAG_STRING   0xd6, 0x05, 0x08, 0x00, 'J', 'L', 'A', 'I', 'S', 'D','K'
static const char user_tag_string[] = {EIR_TAG_STRING};

static u8 conn_24g_adv_data[ADV_RSP_PACKET_MAX];//max is 31
static u8 conn_24g_scan_rsp_data[ADV_RSP_PACKET_MAX];//max is 31
static u8 conn_24g_test_read_write_buf[4];

static adv_cfg_t conn_24g_server_adv_config;
static u8 pair_bond_enalbe;

//配对信息表
#define   PAIR_BOND_ENABLE    1
#define   PAIR_BOND_TAG       0x53
#define   DIRECT_ADV_MAX_CNT   0

u8 pair_bond_info[8]; //tag + addr_type + address
static u8 direct_adv_count;  //定向广播次数
static u8 cur_peer_addr_info[7];//当前连接对方地址信息

//------------------------------------------------------
//广播参数设置
extern const char *bt_get_local_name();
extern void clr_wdt(void);
static void conn_24g_adv_config_set(void);
//------------------------------------------------------
//for ANCS
static uint16_t conn_24g_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
static int conn_24g_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
static int conn_24g_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param);

const gatt_server_cfg_t conn_24g_server_init_cfg = {
    .att_read_cb = &conn_24g_att_read_callback,
    .att_write_cb = &conn_24g_att_write_callback,
    .event_packet_handler = &conn_24g_event_packet_handler,
};

//-------------------------------------------------------------------------------------
//推送消息给app
static void __ble_bt_evnet_post(u32 arg_type, u8 priv_event, u8 *args, u32 value)
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

//推送ble状态
static void __ble_state_to_user(u8 state, u8 reason)
{
    static u8 ble_state = 0xff;
    if (state != ble_state) {
        log_info("ble_state:%02x===>%02x\n", ble_state, state);
        ble_state = state;
        __ble_bt_evnet_post(SYS_BT_EVENT_BLE_STATUS, state, NULL, reason);
    }
}

//vm 绑定对方信息读写
int conn_24g_server_pair_vm_do(u8 *info, u8 info_len, u8 rw_flag)
{
    int ret;
    int vm_len = info_len;

    log_info("-conn_pair_info vm_do:%d\n", rw_flag);
    if (rw_flag == 0) {
        ret = syscfg_read(CFG_BLE_BONDING_REMOTE_INFO, (u8 *)info, vm_len);
        if (!ret) {
            log_info("-null--\n");
            memset(info, 0xff, info_len);
        }
        if (info[0] != PAIR_BOND_TAG) {
            return -1;
        }
    } else {
        syscfg_write(CFG_BLE_BONDING_REMOTE_INFO, (u8 *)info, vm_len);
    }
    return 0;
}

//配置连接参数更新
static void conn_24g_send_connetion_update_deal(u16 conn_handle)
{
    if (conn_24g_connection_update_enable) {
        if (0 == ble_gatt_server_connetion_update_request(conn_handle, conn_24g_connection_param_table, CONN_PARAM_TABLE_CNT)) {
            conn_24g_connection_update_enable = 0;
        }
    }
}

//回连状态，主动使能通知
static void conn_24g_resume_all_ccc_enable(u16 conn_handle, u8 update_request)
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
        conn_24g_send_connetion_update_deal(conn_handle);
    }
}

//-------------------------------------------------------------------------------------
//处理gatt_common 模块返回的事件，hci & gatt
static int conn_24g_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param)
{
    /* log_info("event: %02x,size= %d\n",event,size); */

    switch (event) {
    case GATT_COMM_EVENT_CONNECTION_COMPLETE:
        conn_24g_handle = little_endian_read_16(packet, 0);
        log_info("connection_handle:%04x\n", little_endian_read_16(packet, 0));
        log_info("peer_address_info:");
        put_buf(&ext_param[7], 7);
        memcpy(cur_peer_addr_info, &ext_param[7], 7);
        memcpy(&pair_bond_info[1], &ext_param[7], 7);
        pair_bond_info[0] = PAIR_BOND_TAG;
        put_buf(&pair_bond_info[0], 8);
        conn_24g_server_pair_vm_do(pair_bond_info, sizeof(pair_bond_info), 1);
        pair_bond_enalbe = 0;
        __ble_state_to_user(BLE_ST_CONNECT, conn_24g_handle);
        break;

    case GATT_COMM_EVENT_DISCONNECT_COMPLETE:
        log_info("disconnect_handle:%04x,reason= %02x\n", little_endian_read_16(packet, 0), packet[2]);
        if (packet[2] == 8) {
            if (pair_bond_info[0] == PAIR_BOND_TAG) {
                direct_adv_count = DIRECT_ADV_MAX_CNT;
                conn_24g_adv_config_set();
            }
        } else if (packet[2] == 19) {
            if (pair_bond_info[0] == PAIR_BOND_TAG) {
                conn_24g_adv_config_set();
            }
        }
        break;

    case GATT_COMM_EVENT_ENCRYPTION_CHANGE:
        log_info("ENCRYPTION_CHANGE:handle=%04x,state=%d,process =%02x", little_endian_read_16(packet, 0), packet[2], packet[3]);
        if (packet[3] == LINK_ENCRYPTION_RECONNECT) {
            log_info("reconnect...\n");
            conn_24g_resume_all_ccc_enable(little_endian_read_16(packet, 0), 1);
            if (memcmp(&pair_bond_info[1], cur_peer_addr_info, 7)) {
                log_info("update peer\n");
                memcpy(&pair_bond_info[1], cur_peer_addr_info, 7);
                pair_bond_info[0] = PAIR_BOND_TAG;
                conn_24g_server_pair_vm_do(pair_bond_info, sizeof(pair_bond_info), 1);
            }
        } else {
            log_info("first pair...\n");
#if PAIR_BOND_ENABLE
            memcpy(&pair_bond_info[1], cur_peer_addr_info, 7);
            pair_bond_info[0] = PAIR_BOND_TAG;
            conn_24g_server_pair_vm_do(pair_bond_info, sizeof(pair_bond_info), 1);
            log_info("bonding remote");
            put_buf(pair_bond_info, sizeof(pair_bond_info));
#endif
        }
        break;

    case GATT_COMM_EVENT_CONNECTION_UPDATE_COMPLETE:
        log_info("conn_param update_complete:%04x\n", little_endian_read_16(packet, 0));
        break;

    case GATT_COMM_EVENT_CONNECTION_PHY_UPDATE_COMPLETE:
        log_info("PHY update_complete:%04x\n", conn_24g_handle);

        break;

    case GATT_COMM_EVENT_DIRECT_ADV_TIMEOUT:
        log_info("DIRECT_ADV_TIMEOUT:%d", direct_adv_count);
#if PEER_INFO_BOND
        if (direct_adv_count) {
            direct_adv_count--;
        } else {
            conn_24g_adv_config_set();
        }
#endif
        break;

    case GATT_COMM_EVENT_SERVER_STATE:
        log_info("server_state: handle=%02x,%02x\n", little_endian_read_16(packet, 1), packet[0]);
        /* __ble_state_to_user(packet[0], conn_24g_handle); */
        break;

    case GATT_COMM_EVENT_CAN_SEND_NOW:
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
// @param offset defines start of attribute value
static uint16_t conn_24g_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
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
        att_value_len = sizeof(conn_24g_test_read_write_buf);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }

        if (buffer) {
            memcpy(buffer, &conn_24g_test_read_write_buf[offset], buffer_size);
            att_value_len = buffer_size;
        }
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
static int conn_24g_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    int result = 0;
    u16 tmp16;

    u16 handle = att_handle;

    log_info("write_callback,conn_handle =%04x, handle =%04x,size =%d\n", connection_handle, handle, buffer_size);

    switch (handle) {
    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        break;

    case ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae04_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae05_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae3c_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE:
        conn_24g_send_connetion_update_deal(connection_handle);
        log_info("\n------write ccc:%04x,%02x\n", handle, buffer[0]);
        ble_gatt_server_characteristic_ccc_set(connection_handle, handle, buffer[0]);
        break;

    case ATT_CHARACTERISTIC_ae10_01_VALUE_HANDLE:
        tmp16 = sizeof(conn_24g_test_read_write_buf);
        if ((offset >= tmp16) || (offset + buffer_size) > tmp16) {
            break;
        }
        memcpy(&conn_24g_test_read_write_buf[offset], buffer, buffer_size);
        break;

    case ATT_CHARACTERISTIC_ae01_01_VALUE_HANDLE:
        log_info("\n-ae01_rx(%d):", buffer_size);
        put_buf(buffer, buffer_size);

        /* //收发测试，自动发送收到的数据;for test */
        /* if (ble_comm_att_check_send(connection_handle, buffer_size)) { */
        /*     log_info("-loop send1\n"); */
        /*     ble_comm_att_send_data(connection_handle, ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE, buffer, buffer_size, ATT_OP_AUTO_READ_CCC); */
        /* } */
        break;

    case ATT_CHARACTERISTIC_ae03_01_VALUE_HANDLE:
        log_info("\n-ae_rx(%d):", buffer_size);
        put_buf(buffer, buffer_size);

        //收发测试，自动发送收到的数据;for test
        if (ble_comm_att_check_send(connection_handle, buffer_size)) {
            log_info("-loop send2\n");
            ble_comm_att_send_data(connection_handle, ATT_CHARACTERISTIC_ae05_01_VALUE_HANDLE, buffer, buffer_size, ATT_OP_AUTO_READ_CCC);
        }
        break;

#if RCSP_BTMATE_EN
    case ATT_CHARACTERISTIC_ae02_02_CLIENT_CONFIGURATION_HANDLE:
        ble_op_latency_skip(connection_handle, 0xffff); //
        ble_gatt_server_set_update_send(connection_handle, ATT_CHARACTERISTIC_ae02_02_VALUE_HANDLE, ATT_OP_AUTO_READ_CCC);
#endif
        /* conn_24g_send_connetion_update_deal(connection_handle); */
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
        break;

    default:
        break;
    }
    return 0;
}

//------------------------------------------------------
//生成adv包数据，写入buff
static u8  adv_name_ok = 0;//name 优先存放在ADV包
static int conn_24g_make_set_adv_data(void)
{
    u8 offset = 0;
    u8 *buf = conn_24g_adv_data;

    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, FLAGS_GENERAL_DISCOVERABLE_MODE | FLAGS_EDR_NOT_SUPPORTED, 1);
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
        puts("***conn_24g_adv_data overflow!!!!!!\n");
        return -1;
    }
    /* log_info("conn_24g_adv_data(%d):", offset); */
    /* log_info_hexdump(buf, offset); */
    conn_24g_server_adv_config.adv_data_len = offset;
    conn_24g_server_adv_config.adv_data = conn_24g_adv_data;
    return 0;
}

//生成rsp包数据，写入buff
static int conn_24g_make_set_rsp_data(void)
{
    u8 offset = 0;
    u8 *buf = conn_24g_scan_rsp_data;

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
    conn_24g_server_adv_config.rsp_data_len = offset;
    conn_24g_server_adv_config.rsp_data = conn_24g_scan_rsp_data;
    return 0;
}

static void __conn_24g_direct_low_timeout_handle(void)
{
    if (ble_gatt_server_get_work_state() == BLE_ST_ADV) {
        log_info("ADV_DIRECT_IND_LOW timeout!!!\n");
        ble_gatt_server_adv_enable(0);
        conn_24g_adv_config_set();
        ble_gatt_server_adv_enable(1);
    }
}
//广播参数设置
static void conn_24g_adv_config_set(void)
{
    u8 direct_address_flag = 0;
    int ret = 0;
    ret |= conn_24g_make_set_adv_data();
    ret |= conn_24g_make_set_rsp_data();

    conn_24g_server_adv_config.adv_interval = ADV_INTERVAL_MIN;
    conn_24g_server_adv_config.adv_auto_do = 1;
    conn_24g_server_adv_config.adv_channel = ADV_CHANNEL_ALL;

    conn_24g_server_pair_vm_do(pair_bond_info, sizeof(pair_bond_info), 0);//从vm中读取出配对绑定信息
    memcpy(conn_24g_server_adv_config.direct_address_info, &pair_bond_info[1], 7);

    for (unsigned i = 0; i < 7 ; ++i) {
        if (conn_24g_server_adv_config.direct_address_info[i] == 0) {
            direct_address_flag++;
        }
    }

    if (direct_adv_count) {
        conn_24g_server_adv_config.adv_type = ADV_DIRECT_IND;
        direct_adv_count--;
    } else {
        if (direct_address_flag == 7) {
            conn_24g_server_adv_config.adv_type = ADV_IND;
            conn_24g_server_adv_config.adv_interval = ADV_INTERVAL_MIN;
            memset(conn_24g_server_adv_config.direct_address_info, 0, 7);
            log_info("===ADV_IND");
            put_buf(conn_24g_server_adv_config.direct_address_info, 7);
        } else {
            //配置定向广播信息
            conn_24g_server_adv_config.adv_type = ADV_DIRECT_IND_LOW;
            conn_24g_server_adv_config.adv_interval = PAIR_DIRECT_LOW_ADV_INTERVAL;
            log_info("===DIRECT_ADV address:");
            put_buf(conn_24g_server_adv_config.direct_address_info, 7);
        }
    }

    log_info("conn_24g_adv_data(%d):", 31);
    log_info_hexdump(conn_24g_server_adv_config.adv_data, 31);

    /* ble_op_adv_enable(1); */

    ble_gatt_server_set_adv_config(&conn_24g_server_adv_config);
}

//server init
void conn_24g_server_init(void)
{
    log_info("%s", __FUNCTION__);

#if CONFIG_BT_SM_SUPPORT_ENABLE && PAIR_BOND_ENABLE
    if (0 == conn_24g_server_pair_vm_do(pair_bond_info, sizeof(pair_bond_info), 0)) {
        log_info("server already bond dev");
        put_buf(pair_bond_info, sizeof(pair_bond_info));
        direct_adv_count = DIRECT_ADV_MAX_CNT;
    }
#endif

    ble_gatt_server_set_profile(conn_24g_profile_data, sizeof(conn_24g_profile_data));
    conn_24g_adv_config_set();

}

#endif



