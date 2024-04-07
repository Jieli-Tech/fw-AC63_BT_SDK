/*********************************************************************************************
    *   Filename        : ble_hogp.c

    *   Description     :

    *   Author          :

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2020-09-01 11:14

    *   Copyright:(c)JIELI  2011-2020  @ , All Rights Reserved.
*********************************************************************************************/

// *****************************************************************************
/* EXAMPLE_START
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

#include "le_common.h"
#include "standard_hid.h"

#include "rcsp_bluetooth.h"
#include "JL_rcsp_api.h"
#include "update_loader_download.h"
#include "custom_cfg.h"
#include "ble_hogp_profile.h"
#include "gatt_common/le_gatt_common.h"
#include "app_comm_bt.h"

#if (TCFG_USER_BLE_ENABLE && CONFIG_HOGP_COMMON_ENABLE)
/* #if RCSP_BTMATE_EN */
/* #include "btstack/JL_rcsp_api.h" */
/* #include "rcsp_bluetooth.h" */
/* #endif */

#if LE_DEBUG_PRINT_EN
//#define log_info            y_printf
#define log_info(x, ...)  printf("[BLE_HOGP]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

static u8 remote_ble_addr[7] = {0};
static void hid_timer_mouse_handler(void);
extern get_cur_bt_idx();
extern u8 get_key_timeout_flag();

#define EIR_TAG_STRING   0xd6, 0x05, 0x08, 0x00, 'J', 'L', 'A', 'I', 'S', 'D','K'
static const char user_tag_string[] = {EIR_TAG_STRING};
//用户可配对的，这是样机跟客户开发的app配对的秘钥
//const u8 link_key_data[16] = {0x06, 0x77, 0x5f, 0x87, 0x91, 0x8d, 0xd4, 0x23, 0x00, 0x5d, 0xf1, 0xd8, 0xcf, 0x0c, 0x14, 0x2b};

#if CONFIG_BLE_HIGH_SPEED
//ATT发送的包长,    note: 23 <=need >= MTU
#define ATT_LOCAL_MTU_SIZE        (247)
#else
#define ATT_LOCAL_MTU_SIZE        (64)
#endif

//ATT缓存的buffer支持缓存数据包个数
#define ATT_PACKET_NUMS_MAX       (2)

//ATT缓存的buffer大小,  note: need >= 23,可修改
#define ATT_SEND_CBUF_SIZE        (ATT_PACKET_NUMS_MAX * (ATT_PACKET_HEAD_SIZE + ATT_LOCAL_MTU_SIZE))

static volatile hci_con_handle_t hogp_con_handle;
int ble_hid_timer_handle = 0;

//是否使能参数请求更新,0--disable, 1--enable
static uint8_t hogp_connection_update_enable = 1;

//连接参数表,按顺序优先请求,主机接受了就中止
static const struct conn_update_param_t Peripheral_Preferred_Connection_Parameters[] = {
    {6, 9,  100, 600}, //android
    /* {7, 7,  20, 300}, //mosue */
    /* {20, 20,  20, 300}, //kb */
    {12, 12, 30, 600}, //ios
    {6,  12, 30, 600},// ios fast
};
//共可用的参数组数
#define CONN_PARAM_TABLE_CNT      (sizeof(Peripheral_Preferred_Connection_Parameters)/sizeof(struct conn_update_param_t))

static u8 hogp_adv_data[ADV_RSP_PACKET_MAX];//max is 31
static u8 hogp_scan_rsp_data[ADV_RSP_PACKET_MAX];//max is 31

static u8 first_pair_flag;     //第一次配对标记
static u8 is_hogp_active = 0;  //不可进入sleep状态
static adv_cfg_t hogp_server_adv_config;

/*--------------------------------------------*/
//普通未连接广播周期 (unit:0.625ms)
#define ADV_INTERVAL_MIN                (160 * 5)

/*------------------------------
配置有配对绑定时,先回连广播快连,再普通广播;否则只作普通广播
定向广播只带对方的地址,不带adv和rsp包
*/
#define PAIR_RECONNECT_ADV_EN               1 /*是否上电使用定向广播，优先绑定回连*/

//可选两种定向广播类型,
//ADV_DIRECT_IND       (1.28s超时,interval 固定2ms)
//ADV_DIRECT_IND_LOW   (interval 和 channel 跟普通广播设置一样)
//ADV_IND              (无定向广播:interval 和 channel 跟普通广播设置一样)
static u8 pair_reconnect_adv_type = ADV_DIRECT_IND;

//等待回连状态下的广播周期 (unit:0.625ms)
#define PAIR_RECONNECT_ADV_INTERVAL     (32) /*>=32,  ADV_IND,ADV_DIRECT_IND_LOW,interval*/
static  u32 pair_reconnect_adv_timeout = 5000;/*回连广播持续时间,ms*/

//广播通道设置置
static  u8 le_adv_channel_sel = ADV_CHANNEL_ALL;

//--------------------------------------------
#define BLE_VM_HEAD_TAG           (0xB35C)
#define BLE_VM_TAIL_TAG           (0x5CB3)
struct pair_info_t {
    u16 head_tag;             //头标识
    u8  pair_flag: 2;         //配对信息是否有效
    u8  direct_adv_cnt: 6;    //定向广播次数
    u8 direct_adv_flag: 1;
    u8  peer_address_info[7]; //绑定的对方地址信息
    u16 tail_tag;//尾标识
};

//配对信息表
static struct pair_info_t  hogp_pair_info;
static u8 cur_peer_addr_info[7];//当前连接对方地址信息

#define REPEAT_DIRECT_ADV_COUNT  (2)// *1.28s

//为了及时响应手机数据包，某些流程会忽略进入latency的控制
//如下是定义忽略进入的interval个数
#define LATENCY_SKIP_INTERVAL_MIN     (3)
#define LATENCY_SKIP_INTERVAL_KEEP    (20)
#define LATENCY_SKIP_INTERVAL_LONG    (0xffff)

//hid device infomation
static u8 Appearance[] = {BLE_APPEARANCE_GENERIC_HID & 0xff, BLE_APPEARANCE_GENERIC_HID >> 8}; //
static const char Manufacturer_Name_String[] = "zhuhai_jieli";
static const char Model_Number_String[] = "hid_mouse";
static const char Serial_Number_String[] = "000000";
static const char Hardware_Revision_String[] = "0.0.1";
static const char Firmware_Revision_String[] = "0.0.1";
static const char Software_Revision_String[] = "0.0.1";
static const u8 System_ID[] = {0, 0, 0, 0, 0, 0, 0, 0};

//定义的产品信息,for test
#define  PNP_VID_SOURCE   0x02
#define  PNP_VID          0x05ac //0x05d6
#define  PNP_PID          0x022C //
#define  PNP_PID_VERSION  0x011b //1.1.11

static u8 PnP_ID[] = {PNP_VID_SOURCE, PNP_VID & 0xFF, PNP_VID >> 8, PNP_PID & 0xFF, PNP_PID >> 8, PNP_PID_VERSION & 0xFF, PNP_PID_VERSION >> 8};
/* static const u8 PnP_ID[] = {0x02, 0x17, 0x27, 0x40, 0x00, 0x23, 0x00}; */
/* static const u8 PnP_ID[] = {0x02, 0xac, 0x05, 0x2c, 0x02, 0x1b, 0x01}; */

/* static const u8 hid_information[] = {0x11, 0x01, 0x00, 0x01}; */
/* static const u8 hid_information[] = {0x01, 0x01, 0x00, 0x03}; */
static const u8 hid_information[] = {0x11, 0x01, 0x00, 0x03};
static const u8 hid_information1[] = {0x00, 0x00, 0x00, 0x00};

static  u8 *report_map; //描述符
static  u16 report_map_size;//描述符大小
static u16 hogp_adv_timeout_number = 0;
#define HID_REPORT_MAP_DATA    report_map
#define HID_REPORT_MAP_SIZE    report_map_size

//report 发生变化，通过service change 通知主机重新获取
static u8 hid_report_change = 0;
static u8 hid_battery_level = 88;
static u32 hid_battery_level_add_sum;/*电量采集累加*/
static u32 hid_battery_level_add_cnt;/*电量采集次数*/

#define HID_BATTERY_TIMER_SET     (60000) /*定时检测电量变化的时间30~60*/
static u16 hid_battery_notify_timer_id;

static u8(*get_vbat_percent_call)(void) = NULL;
static void (*le_hogp_output_callback)(u8 *buffer, u16 size) = NULL;
//------------------------------------------------------
static void __hogp_resume_all_ccc_enable(u8 update_request);
static void __check_report_map_change(void);
void ble_hid_transfer_channel_recieve(u8 *packet, u16 size);
void ble_hid_transfer_channel_recieve1(u8 *packet, u16 size);
static uint16_t hogp_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
static int hogp_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);
static int hogp_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param);
static void hogp_adv_config_set(void);
static u8 hid_get_vbat_handle(void);
//------------------------------------------------------

static u8 multi_dev_adv_flag = 0; /*多地址设备切换广播*/
void set_multi_devices_adv_flag(u8 adv_flag)
{
    multi_dev_adv_flag = adv_flag;
}

//输入passkey 加密
#define PASSKEY_ENABLE                     0

static const sm_cfg_t hogp_sm_init_config = {
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

const gatt_server_cfg_t hogp_server_init_cfg = {
    .att_read_cb = &hogp_att_read_callback,
    .att_write_cb = &hogp_att_write_callback,
    .event_packet_handler = &hogp_event_packet_handler,
};

static gatt_ctrl_t hogp_gatt_control_block = {
    //public
    .mtu_size = ATT_LOCAL_MTU_SIZE,
    .cbuffer_size = ATT_SEND_CBUF_SIZE,
    .multi_dev_flag	= 0,

    //config
#if CONFIG_BT_GATT_SERVER_NUM
    .server_config = &hogp_server_init_cfg,
#else
    .server_config = NULL,
#endif

    .client_config = NULL,

#if CONFIG_BT_SM_SUPPORT_ENABLE
    .sm_config = &hogp_sm_init_config,
#else
    .sm_config = NULL,
#endif
    //cbk,event handle
    .hci_cb_packet_handler = NULL,
};

/*************************************************************************************************/
/*!
 *  \brief      推送消息给app
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
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

/*************************************************************************************************/
/*!
 *  \brief      推送ble状态
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void __ble_state_to_user(u8 state, u8 reason)
{
    static u8 ble_state = 0xff;
    if (state != ble_state) {
        log_info("ble_state:%02x===>%02x\n", ble_state, state);
        ble_state = state;
        __ble_bt_evnet_post(SYS_BT_EVENT_BLE_STATUS, state, NULL, reason);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      配置发起连接参数更新
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void __hogp_send_connetion_update_deal(u16 conn_handle)
{
    if (hogp_connection_update_enable) {
        if (0 == ble_gatt_server_connetion_update_request(conn_handle, Peripheral_Preferred_Connection_Parameters, CONN_PARAM_TABLE_CNT)) {
            hogp_connection_update_enable = 0;
        }
    }
}

/*************************************************************************************************/
/*!
 *  \brief      vm 绑定对方信息 读写
 *
 *  \param      [in] rw_flag： 0--read， 1--write
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void __hogp_conn_pair_vm_do(struct pair_info_t *info, u8 rw_flag)
{
    int ret;
    int vm_len = sizeof(struct pair_info_t);

    log_info("-hogp_pair_info vm_do:%d\n", rw_flag);
    if (rw_flag == 0) {
        if (multi_dev_adv_flag) {
            ret = syscfg_read(CFG_BLE_BONDING_REMOTE_INFO + get_cur_bt_idx(), (u8 *)info, vm_len);
        } else {
            ret = syscfg_read(CFG_BLE_BONDING_REMOTE_INFO, (u8 *)info, vm_len);
        }
        if (vm_len != ret) {
            log_info("-null--\n");
            memset(info, 0, vm_len);
        }

        if ((BLE_VM_HEAD_TAG == info->head_tag) && (BLE_VM_TAIL_TAG == info->tail_tag)) {
            log_info("-exist--\n");
        } else {
            log_info("-reset vm--\n");
            memset(info, 0, vm_len);
            info->head_tag = BLE_VM_HEAD_TAG;
            info->tail_tag = BLE_VM_TAIL_TAG;
        }
    } else {
        if (multi_dev_adv_flag) {
            syscfg_write(CFG_BLE_BONDING_REMOTE_INFO + get_cur_bt_idx(), (u8 *)info, vm_len);
        } else {
            syscfg_write(CFG_BLE_BONDING_REMOTE_INFO, (u8 *)info, vm_len);
        }
    }
    log_info_hexdump(info, vm_len);
}

/*************************************************************************************************/
/*!
 *  \brief      配置hid描述符
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static const u16 change_handle_table[2] = {ATT_CHARACTERISTIC_2a4b_01_VALUE_HANDLE, ATT_CHARACTERISTIC_2a4b_01_VALUE_HANDLE};
static void __check_report_map_change(void)
{
#if 0 //部分手机不支持
    if (hid_report_change && first_pair_flag && ble_gatt_server_characteristic_ccc_get(hogp_con_handle, ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE)) {
        log_info("###send services changed\n");
        ble_comm_att_send_data(hogp_con_handle, ATT_CHARACTERISTIC_2a05_01_VALUE_HANDLE, change_handle_table, 4, ATT_OP_INDICATE);
        hid_report_change = 0;
    }
#endif
}

/*************************************************************************************************/
/*!
 *  \brief 回连状态，主动使能通知

 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void __hogp_resume_all_ccc_enable(u8 update_request)
{
    log_info("__hogp_resume_all_ccc_enable\n");
    ble_gatt_server_characteristic_ccc_set(hogp_con_handle, ATT_CHARACTERISTIC_2a4d_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    ble_gatt_server_characteristic_ccc_set(hogp_con_handle, ATT_CHARACTERISTIC_2a4d_02_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    ble_gatt_server_characteristic_ccc_set(hogp_con_handle, ATT_CHARACTERISTIC_2a4d_04_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    ble_gatt_server_characteristic_ccc_set(hogp_con_handle, ATT_CHARACTERISTIC_2a4d_05_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    ble_gatt_server_characteristic_ccc_set(hogp_con_handle, ATT_CHARACTERISTIC_2a4d_06_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    ble_gatt_server_characteristic_ccc_set(hogp_con_handle, ATT_CHARACTERISTIC_2a4d_07_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    ble_gatt_server_characteristic_ccc_set(hogp_con_handle, ATT_CHARACTERISTIC_ae42_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    ble_gatt_server_characteristic_ccc_set(hogp_con_handle, ATT_CHARACTERISTIC_2a19_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);

#if RCSP_BTMATE_EN
    /* ble_gatt_server_characteristic_ccc_set(hogp_con_handle, ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY); */
    /* ble_gatt_server_set_update_send(hogp_con_handle, ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE, ATT_OP_AUTO_READ_CCC); */
    /* set_rcsp_conn_handle(hogp_con_handle); */
#endif
    if (update_request) {
        __hogp_send_connetion_update_deal(hogp_con_handle);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      反馈检查对方的操作系统
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note 参考识别手机系统
 */
/*************************************************************************************************/
static void __att_check_remote_result(u16 con_handle, remote_type_e remote_type)
{
    log_info("le_hogp %02x:remote_type= %02x\n", con_handle, remote_type);
    __ble_bt_evnet_post(SYS_BT_EVENT_FORM_COMMON, COMMON_EVENT_BLE_REMOTE_TYPE, NULL, remote_type);
    //to do
}

/*************************************************************************************************/
/*!
 *  \brief       处理 gatt common 事件，hci & gatt
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int hogp_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param)
{
    /* log_info("event: %02x,size= %d\n",event,size); */

    switch (event) {

    case GATT_COMM_EVENT_CAN_SEND_NOW:
#if TEST_AUDIO_DATA_UPLOAD
        hogp_test_send_audio_data(0);
#endif
        break;

    case GATT_COMM_EVENT_SERVER_INDICATION_COMPLETE:
        log_info("INDICATION_COMPLETE:con_handle= %04x,att_handle= %04x\n", \
                 little_endian_read_16(packet, 0), little_endian_read_16(packet, 2));
        break;


    case GATT_COMM_EVENT_CONNECTION_COMPLETE:
        hogp_con_handle = little_endian_read_16(packet, 0);
        first_pair_flag = 0;
        memcpy(cur_peer_addr_info, ext_param + 7, 7);

        log_info("connection_handle:%04x, rssi= %d\n", hogp_con_handle, ble_vendor_get_peer_rssi(hogp_con_handle));
        log_info("peer_address_info:");
        put_buf(cur_peer_addr_info, 7);
        log_info("con_interval = %d\n", little_endian_read_16(ext_param, 14 + 0));
        log_info("con_latency = %d\n", little_endian_read_16(ext_param, 14 + 2));
        log_info("cnn_timeout = %d\n", little_endian_read_16(ext_param, 14 + 4));

        if (little_endian_read_16(ext_param, 14 + 2)) {
            //latency非0值
            hogp_connection_update_enable = 0;
            /* __hogp_resume_all_ccc_enable(0); */
        } else {
            hogp_connection_update_enable = 1;
        }

        if (ble_hid_timer_handle) {
            log_info("mdy_timer= %d\n", (u32)(little_endian_read_16(ext_param, 14 + 0) * 1.25));
            sys_s_hi_timer_modify(ble_hid_timer_handle, (u32)(little_endian_read_16(ext_param, 14 + 0) * 1.25));
        }

        if (little_endian_read_16(ext_param, 14 + 2)) {
            ble_op_latency_skip(hogp_con_handle, LATENCY_SKIP_INTERVAL_KEEP); //
        }
        break;

    case GATT_COMM_EVENT_DISCONNECT_COMPLETE:
        log_info("disconnect_handle:%04x,reason= %02x\n", little_endian_read_16(packet, 0), packet[2]);
        hogp_con_handle = 0;
        if (8 == packet[2] && hogp_pair_info.pair_flag) {
            //超时断开,有配对发定向广播
            hogp_pair_info.direct_adv_cnt = REPEAT_DIRECT_ADV_COUNT;
            hogp_adv_config_set();
        }
        break;

    case GATT_COMM_EVENT_ENCRYPTION_CHANGE:
        log_info("ENCRYPTION_CHANGE:handle=%04x,state=%d,process =%02x", little_endian_read_16(packet, 0), packet[2], packet[3]);
        if (packet[3] == LINK_ENCRYPTION_RECONNECT) {
            log_info("reconnect...\n");
            if ((!hogp_pair_info.pair_flag) || memcmp(cur_peer_addr_info, hogp_pair_info.peer_address_info, 7)) {
                log_info("update reconnect peer\n");
                put_buf(cur_peer_addr_info, 7);
                memcpy(hogp_pair_info.peer_address_info, cur_peer_addr_info, 7);
                hogp_pair_info.pair_flag = 1;
                __hogp_conn_pair_vm_do(&hogp_pair_info, 1);
            }

            __hogp_resume_all_ccc_enable(1);
            //回连时,从配对表中获取
            u8 tmp_buf[6];
            u8 remote_type = 0;
            swapX(&cur_peer_addr_info[1], tmp_buf, 6);
            ble_list_get_remote_type(tmp_buf, cur_peer_addr_info[0], &remote_type);
            log_info("list's remote_type:%d\n", remote_type);
            __att_check_remote_result(hogp_con_handle, remote_type);
        } else {
            //只在配对时启动检查
            log_info("first pair...\n");
            memcpy(hogp_pair_info.peer_address_info, cur_peer_addr_info, 7);
            att_server_set_check_remote(hogp_con_handle, __att_check_remote_result);
            hogp_pair_info.pair_flag = 1;
            __hogp_conn_pair_vm_do(&hogp_pair_info, 1);
        }
        __ble_state_to_user(BLE_PRIV_PAIR_ENCRYPTION_CHANGE, first_pair_flag);
        break;

    case GATT_COMM_EVENT_CONNECTION_UPDATE_COMPLETE:
        log_info("conn_param update_complete:%04x\n", little_endian_read_16(packet, 0));
        log_info("update_interval = %d\n", little_endian_read_16(ext_param, 6 + 0));
        log_info("update_latency = %d\n", little_endian_read_16(ext_param, 6 + 2));
        log_info("update_timeout = %d\n", little_endian_read_16(ext_param, 6 + 4));
        if (ble_hid_timer_handle) {
            log_info("mdy_timer= %d\n", (u32)(little_endian_read_16(ext_param, 6 + 0) * 1.25));
            sys_s_hi_timer_modify(ble_hid_timer_handle, (u32)(little_endian_read_16(ext_param, 6 + 0) * 1.25));
        }
        break;



    case GATT_COMM_EVENT_DIRECT_ADV_TIMEOUT:
        log_info("DIRECT_ADV_TIMEOUT\n");
        if (hogp_pair_info.direct_adv_cnt) {
            hogp_pair_info.direct_adv_cnt--;
        }
        hogp_adv_config_set();
        break;

    case GATT_COMM_EVENT_ENCRYPTION_REQUEST:
        first_pair_flag = 1;
        memcpy(hogp_pair_info.peer_address_info, &packet[4], 7);
        hogp_pair_info.pair_flag = 0;
        __ble_state_to_user(BLE_PRIV_MSG_PAIR_CONFIRM, 0);
        break;

    case GATT_COMM_EVENT_SERVER_STATE:
        log_info("server_state: %02x,%02x\n", little_endian_read_16(packet, 1), packet[0]);
        __ble_state_to_user(packet[0], hogp_con_handle);
        break;

    case GATT_COMM_EVENT_CONNECTION_UPDATE_REQUEST_RESULT:
    case GATT_COMM_EVENT_MTU_EXCHANGE_COMPLETE:
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

//主机操作ATT read,协议栈回调处理
static uint16_t hogp_att_read_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{

    uint16_t  att_value_len = 0;
    uint16_t handle = att_handle;

    log_info("read_callback, handle= 0x%04x,buffer= %08x,offset= %04x\n", handle, (u32)buffer, offset);

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

    case ATT_CHARACTERISTIC_2a01_01_VALUE_HANDLE:
        att_value_len = sizeof(Appearance);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &Appearance[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a04_01_VALUE_HANDLE:
        att_value_len = 8;//fixed
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            log_info("\n------get Peripheral_Preferred_Connection_Parameters\n");
            memcpy(buffer, &Peripheral_Preferred_Connection_Parameters[0], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a29_01_VALUE_HANDLE:
        att_value_len = strlen(Manufacturer_Name_String);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &Manufacturer_Name_String[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a24_01_VALUE_HANDLE:
        att_value_len = strlen(Model_Number_String);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &Model_Number_String[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a25_01_VALUE_HANDLE:
        att_value_len = strlen(Serial_Number_String);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &Serial_Number_String[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a27_01_VALUE_HANDLE:
        att_value_len = strlen(Hardware_Revision_String);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &Hardware_Revision_String[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a26_01_VALUE_HANDLE:
        att_value_len = strlen(Firmware_Revision_String);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &Firmware_Revision_String[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a28_01_VALUE_HANDLE:
        att_value_len = strlen(Software_Revision_String);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &Software_Revision_String[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a23_01_VALUE_HANDLE:
        att_value_len = sizeof(System_ID);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &System_ID[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a50_01_VALUE_HANDLE:
        log_info("read PnP_ID\n");
        att_value_len = sizeof(PnP_ID);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &PnP_ID[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a19_01_VALUE_HANDLE:
        att_value_len = 1;
        if (buffer) {
            if (get_vbat_percent_call) {
                hid_battery_level = hid_get_vbat_handle();
                log_info("read vbat:%d\n", hid_battery_level);
            }
            buffer[0] = hid_battery_level;
        }
        break;

    case ATT_CHARACTERISTIC_2a4b_01_VALUE_HANDLE:
        att_value_len = HID_REPORT_MAP_SIZE;
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &HID_REPORT_MAP_DATA[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_2a4a_01_VALUE_HANDLE:
        att_value_len = sizeof(hid_information);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            break;
        }
        if (buffer) {
            memcpy(buffer, &hid_information[offset], buffer_size);
            att_value_len = buffer_size;
        }
        break;

    case ATT_CHARACTERISTIC_ae10_01_VALUE_HANDLE:
        att_value_len = sizeof(hid_information1);
        if ((offset >= att_value_len) || (offset + buffer_size) > att_value_len) {
            log_info("len is err");
            break;
        }
        if (buffer) {
            memcpy(buffer, &hid_information1[offset], buffer_size);
            att_value_len = buffer_size;
            put_buf(buffer, att_value_len);
        }
        /* __hogp_send_connetion_update_deal(connection_handle); */
        break;


    case ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a19_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_02_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_04_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_05_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_06_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_07_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae42_01_CLIENT_CONFIGURATION_HANDLE:
        if (buffer) {
            buffer[0] = ble_gatt_server_characteristic_ccc_get(hogp_con_handle, handle);
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
 *  \brief      处理client 读操作
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note      profile的读属性uuid 有配置 DYNAMIC 关键字，就有read_callback 回调
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

//主机操作ATT write,协议栈回调处理
static int hogp_att_write_callback(hci_con_handle_t connection_handle, uint16_t att_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size)
{
    int result = 0;
    u16 tmp16;

    u16 handle = att_handle;

    log_info("write_callback, handle= 0x%04x,size = %d\n", handle, buffer_size);

    switch (handle) {

    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        break;

    case ATT_CHARACTERISTIC_2a4d_03_VALUE_HANDLE:
        put_buf(buffer, buffer_size);           //键盘led灯状态
        if (le_hogp_output_callback) {
            le_hogp_output_callback(buffer, buffer_size);
        }
        break;

    case ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE:
        ble_gatt_server_characteristic_ccc_set(connection_handle, handle, buffer[0]);
        if (buffer[0]) {
            __check_report_map_change();
        }
        break;

#if RCSP_BTMATE_EN
    case ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE:
        ble_gatt_server_set_update_send(connection_handle, ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE, ATT_OP_AUTO_READ_CCC);
        set_rcsp_conn_handle(connection_handle);
#if (0 == BT_CONNECTION_VERIFY)
        JL_rcsp_auth_reset();       //hid设备试能nofity的时候reset auth保证APP可以重新连接
#endif

#if (TCFG_HID_AUTO_SHUTDOWN_TIME)
        __ble_bt_evnet_post(SYS_BT_EVENT_FORM_COMMON, COMMON_EVENT_SHUTDOWN_DISABLE, NULL, 0);
        /* auto_shutdown_disable(); */
#endif
#endif

    case ATT_CHARACTERISTIC_ae42_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_02_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_04_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_05_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_06_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_07_CLIENT_CONFIGURATION_HANDLE:
        ble_op_latency_skip(hogp_con_handle, LATENCY_SKIP_INTERVAL_MIN); //
    case ATT_CHARACTERISTIC_2a19_01_CLIENT_CONFIGURATION_HANDLE:
        __hogp_send_connetion_update_deal(hogp_con_handle);
        log_info("\n------write ccc:%04x,%02x\n", handle, buffer[0]);
#if RCSP_BTMATE_EN
        //-----如果主机notify RCSP协议,从机告知主机存在此协议
        if (handle == ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE) {
            u8 data[1] = {0xff};
            ble_comm_att_send_data(connection_handle, ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE, &data, 1, ATT_OP_NOTIFY);
        }
#endif
        ble_gatt_server_characteristic_ccc_set(hogp_con_handle, handle, buffer[0]);
        break;

#if RCSP_BTMATE_EN
    case ATT_CHARACTERISTIC_ae01_01_VALUE_HANDLE:
        log_info("rcsp_read:%x\n", buffer_size);
        put_buf(buffer, (buffer_size > 30) ? 30 : buffer_size);
        hogp_connection_update_enable = 0;
        ble_gatt_server_receive_update_data(NULL, buffer, buffer_size);
        break;
#endif

    case ATT_CHARACTERISTIC_ae41_01_VALUE_HANDLE:
        ble_hid_transfer_channel_recieve(buffer, buffer_size);
        break;

    case ATT_CHARACTERISTIC_ae10_01_VALUE_HANDLE:
        tmp16 = sizeof(hid_information1);
        if ((offset >= tmp16) || (offset + buffer_size) > tmp16) {
            break;
        }

        log_info("cur_state:");
        put_buf(hid_information1, tmp16);

        memcpy(&hid_information1[offset], buffer, buffer_size);

        log_info("new_state:");
        put_buf(hid_information1, tmp16);
        ble_hid_transfer_channel_recieve1(hid_information1, tmp16);
        break;

    default:
        break;
    }
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      生成adv包，放入buff
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static u8  adv_name_ok;   //name 优先存放在ADV包
static int hogp_make_set_adv_data(u8 adv_reconnect)
{
    u8 offset = 0;
    u8 *buf = hogp_adv_data;

    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, FLAGS_GENERAL_DISCOVERABLE_MODE | FLAGS_EDR_NOT_SUPPORTED, 1);
    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_16BIT_SERVICE_UUIDS, HID_UUID_16, 2);
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_APPEARANCE_DATA, Appearance, 2);

    if (!adv_reconnect) {/*回连广播默认不填入名字*/
        char *gap_name = ble_comm_get_gap_name();
        u8 name_len = strlen(gap_name);
        u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
        if (name_len < vaild_len) {
            offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_name, name_len);
            adv_name_ok = 1;
        } else {
            adv_name_ok = 0;
        }
    }

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***hogp_adv_data overflow!!!!!!\n");
        return -1;
    }
    log_info("hogp_adv_data(%d):", offset);
    log_info_hexdump(buf, offset);
    hogp_server_adv_config.adv_data_len = offset;
    hogp_server_adv_config.adv_data = hogp_adv_data;
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      生成rsp包，放入buff
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int hogp_make_set_rsp_data(u8 adv_reconnect)
{
    u8 offset = 0;
    u8 *buf = hogp_scan_rsp_data;

#if RCSP_BTMATE_EN
    u8  tag_len = sizeof(user_tag_string);
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA, (void *)user_tag_string, tag_len);
#endif

    if (!adv_reconnect) {/*回连广播默认不填入名字*/
        if (!adv_name_ok) {
            char *gap_name = ble_comm_get_gap_name();
            u8 name_len = strlen(gap_name);
            u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
            if (name_len > vaild_len) {
                name_len = vaild_len;
            }
            offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_name, name_len);
        }
    }

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***rsp_data overflow!!!!!!\n");
        return -1;
    }

    log_info("rsp_data(%d):", offset);
    log_info_hexdump(buf, offset);
    hogp_server_adv_config.rsp_data_len = offset;
    hogp_server_adv_config.rsp_data = hogp_scan_rsp_data;
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      修改gatt name
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void modify_ble_name(const char *name)
{
    log_info(">>>modify_ble_name:%s\n", name);
    ble_comm_set_config_name(bt_get_local_name(), 1);
}

static void wait_to_open_adv(void)
{
    if (!hogp_con_handle) {
        log_info("allow open adv");
        ble_gatt_server_adv_enable(1);
    } else {
        log_info("no allow open adv");
    }
}

static void __hogp_reconnect_low_timeout_handle(void)
{
    if (0 == hogp_con_handle && ble_gatt_server_get_work_state() == BLE_ST_ADV) {
        log_info("ADV_RECONNECT timeout!!!\n");
        ble_gatt_server_adv_enable(0);
        hogp_pair_info.direct_adv_cnt = 0;
        hogp_adv_config_set();
#if RCSP_BTMATE_EN
        sys_timeout_add(NULL, wait_to_open_adv, 20);
#else
        ble_gatt_server_adv_enable(1);
#endif
    } else {
        /*其他情况，要取消定向广播*/
        log_info("Set Switch to ADV_IND Config\n");
        hogp_pair_info.direct_adv_cnt = 0;
        hogp_adv_config_set();
    }
}

/*************************************************************************************************/
/*!
 *  \brief   广播参数设置
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note  回连可默认用无定向广播，兼容某些主机不支持定向广播连接
 */
/*************************************************************************************************/
static void hogp_adv_config_set(void)
{
    int ret = 0;
    int adv_reconnect_flag = 0;

    hogp_server_adv_config.adv_interval = ADV_INTERVAL_MIN;
    hogp_server_adv_config.adv_auto_do = 1;
    hogp_server_adv_config.adv_channel = le_adv_channel_sel;

    if (PAIR_RECONNECT_ADV_EN && hogp_pair_info.pair_flag && hogp_pair_info.direct_adv_cnt) {
        hogp_server_adv_config.adv_type = pair_reconnect_adv_type;
        memcpy(hogp_server_adv_config.direct_address_info, hogp_pair_info.peer_address_info, 7);
        if (pair_reconnect_adv_type != ADV_DIRECT_IND) {
            hogp_server_adv_config.adv_interval = PAIR_RECONNECT_ADV_INTERVAL;
            if (pair_reconnect_adv_timeout) {
                if (hogp_adv_timeout_number) {
                    sys_timeout_del(hogp_adv_timeout_number);
                    hogp_adv_timeout_number = 0;
                }
                hogp_adv_timeout_number = sys_timeout_add(0, __hogp_reconnect_low_timeout_handle, pair_reconnect_adv_timeout);
            }
        }
        log_info("RECONNECT_ADV1= %02x, address:", pair_reconnect_adv_type);
        put_buf(hogp_server_adv_config.direct_address_info, 7);
        adv_reconnect_flag = 1;
    } else {
        hogp_server_adv_config.adv_type = ADV_IND;
        memset(hogp_server_adv_config.direct_address_info, 0, 7);
    }

    log_info("adv_type:%d,channel=%02x\n", hogp_server_adv_config.adv_type, hogp_server_adv_config.adv_channel);

    ret |= hogp_make_set_adv_data(adv_reconnect_flag);
    ret |= hogp_make_set_rsp_data(adv_reconnect_flag);

    if (ret) {
        log_info("adv_setup_init fail!!!\n");
        return;
    }
    ble_gatt_server_set_adv_config(&hogp_server_adv_config);

}

/*************************************************************************************************/
/*!
 *  \brief   定向广播参数设置
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note  回连可默认用无定向广播，兼容某些主机不支持定向广播连接
 */
/*************************************************************************************************/
void hogp_reconnect_adv_config_set(u8 adv_type, u32 adv_timeout)
{
    int ret = 0;
    int adv_reconnect_flag = 0;

    hogp_server_adv_config.adv_interval = PAIR_RECONNECT_ADV_INTERVAL;
    hogp_server_adv_config.adv_auto_do = 1;
    hogp_server_adv_config.adv_channel = ADV_CHANNEL_ALL;

    __hogp_conn_pair_vm_do(&hogp_pair_info, 0);

    if (hogp_pair_info.pair_flag) {
        adv_reconnect_flag = 1; /*已配对*/
        hogp_server_adv_config.adv_type = adv_type;
    } else {
        hogp_server_adv_config.adv_type = ADV_IND;/*强制切换可发现广播*/
    }

    if (PAIR_RECONNECT_ADV_EN && hogp_pair_info.pair_flag) {
        memcpy(hogp_server_adv_config.direct_address_info, &hogp_pair_info.peer_address_info, 7);
        log_info("RECONNECT_ADV2= %02x, address:", adv_type);
        put_buf(hogp_server_adv_config.direct_address_info, 7);

        if (adv_type == ADV_DIRECT_IND) {
            hogp_pair_info.direct_adv_cnt = (adv_timeout + 1279) / 1280; /*定向次数*/
        } else {
            if (adv_timeout) {
                /*设置超时切换到无定向广播*/
                if (hogp_adv_timeout_number) {
                    sys_timeout_del(hogp_adv_timeout_number);
                    hogp_adv_timeout_number = 0;
                }
                hogp_adv_timeout_number = sys_timeout_add(0, __hogp_reconnect_low_timeout_handle, adv_timeout);
            }
        }
    } else {
        memset(hogp_server_adv_config.direct_address_info, 0, 7);
        hogp_server_adv_config.adv_interval = ADV_INTERVAL_MIN;
    }

    log_info("adv_type:%d,channel=%02x\n", hogp_server_adv_config.adv_type, hogp_server_adv_config.adv_channel);

    ret |= hogp_make_set_adv_data(adv_reconnect_flag);
    ret |= hogp_make_set_rsp_data(adv_reconnect_flag);

    if (ret) {
        log_info("adv_setup_init fail!!!\n");
        return;
    }
    ble_gatt_server_set_adv_config(&hogp_server_adv_config);
}

/*************************************************************************************************/
/*!
 *  \brief      init
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void hogp_server_init(void)
{
    log_info("%s", __FUNCTION__);
    ble_gatt_server_set_profile(hogp_profile_data, sizeof(hogp_profile_data));
    hogp_adv_config_set();
}

/*************************************************************************************************/
/*!
 *  \brief      bt_ble_before_start_init
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
    ble_comm_init(&hogp_gatt_control_block);
}

/*************************************************************************************************/
/*!
 *  \brief      电量累计求平均计算方法
 *
 *  \param      [in]
 *
 *  \return vbat percent
 *
 *  \note
 */
/*************************************************************************************************/
static u8 hid_get_vbat_handle(void)
{
    if (!get_vbat_percent_call) {
        return 0;
    }

    u8 cur_val, avg_val, val;

    if (hid_battery_level_add_cnt > 10) {
        /*超过10次，取平均值*/
        hid_battery_level_add_sum = hid_battery_level_add_sum / hid_battery_level_add_cnt;
        hid_battery_level_add_cnt = 1;
    }

    cur_val = get_vbat_percent_call();

    if (hid_battery_level_add_cnt) {
        avg_val = hid_battery_level_add_sum / hid_battery_level_add_cnt;
        if (cur_val > (avg_val + 2) || (cur_val + 2) < avg_val) {
            /*变化较大*/
            hid_battery_level_add_sum = 0;
            hid_battery_level_add_cnt = 0;
        }
    }

    hid_battery_level_add_sum += cur_val;
    hid_battery_level_add_cnt++;

    /*简单的累加求平均值计算*/
    val = (u8)(hid_battery_level_add_sum / hid_battery_level_add_cnt);
    log_info("vbat: avg=%d,cur=%d,val=%d\n", avg_val, cur_val, val);
    return val;
}

/*************************************************************************************************/
/*!
 *  \brief      定时检测电量变化，推送通知给主机
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void hid_battery_timer_handler(void *priev)
{
#if TCFG_SYS_LVD_EN
    if (hogp_con_handle && get_vbat_percent_call) {
        u8 tmp_val = hid_get_vbat_handle();
        /* tmp_val = (tmp_val +10)%101; */
        if (hid_battery_level != tmp_val) {
            hid_battery_level = tmp_val;
            if (ble_gatt_server_characteristic_ccc_get(hogp_con_handle, ATT_CHARACTERISTIC_2a19_01_CLIENT_CONFIGURATION_HANDLE)) {
                log_info("notify battery: %d\n", hid_battery_level);
                ble_comm_att_send_data(hogp_con_handle, ATT_CHARACTERISTIC_2a19_01_VALUE_HANDLE, &hid_battery_level, 1, ATT_OP_AUTO_READ_CCC);
            }
        }
    }
#endif // TCFG_SYS_LVD_EN
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

    hogp_con_handle = 0;
    __hogp_conn_pair_vm_do(&hogp_pair_info, 0);
    if (hogp_pair_info.pair_flag) {
        hogp_pair_info.direct_adv_cnt = REPEAT_DIRECT_ADV_COUNT;
    }

#if TCFG_SYS_LVD_EN
    if (!hid_battery_notify_timer_id && HID_BATTERY_TIMER_SET) {
        log_info("add hid_battery_notify_timer_id\n");
        hid_battery_notify_timer_id = sys_timer_add((void *)0, hid_battery_timer_handler, HID_BATTERY_TIMER_SET);
    }
#endif // TCFG_SYS_LVD_EN

#if DOUBLE_BT_SAME_NAME
    ble_comm_set_config_name(bt_get_local_name(), 0);
#else
    ble_comm_set_config_name(bt_get_local_name(), 1);
#endif

    hogp_server_init();
#if CONFIG_APP_STANDARD_KEYBOARD
    ble_module_enable(0);
#else
    ble_module_enable(1);
#endif
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
    log_info("***** ble_exit******\n");
    if (ble_hid_timer_handle) {
        sys_s_hi_timer_del(ble_hid_timer_handle);
        ble_hid_timer_handle = 0;
    }

#if TCFG_SYS_LVD_EN
    if (hid_battery_notify_timer_id) {
        sys_timer_del(hid_battery_notify_timer_id);
        hid_battery_notify_timer_id = 0;
    }
#endif // TCFG_SYS_LVD_EN

    log_info("%s\n", __FUNCTION__);
    ble_module_enable(0);
    ble_comm_exit();
}

/*************************************************************************************************/
/*!
 *  \brief      模块开关使能
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
    log_info("mode_en:%d\n", en);
    ble_comm_module_enable(en);
}

/*************************************************************************************************/
/*!
 *  \brief      注册控制是否进入sleep
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static u8 hogp_idle_query(void)
{
    return !is_hogp_active;
}

REGISTER_LP_TARGET(le_hogp_target) = {
    .name = "ble_hogp",
    .is_idle = hogp_idle_query,
};

/*************************************************************************************************/
/*!
 *  \brief      设置设备的图标
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void le_hogp_set_icon(u16 class_type)
{
    memcpy(Appearance, &class_type, 2);
}

/*************************************************************************************************/
/*!
 *  \brief      设置hid描述符
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void le_hogp_set_ReportMap(u8 *map, u16 size)
{
    report_map = map;
    report_map_size = size;
    hid_report_change = 1;
}

/*************************************************************************************************/
/*!
 *  \brief      set callback
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void le_hogp_set_output_callback(void *cb)
{
    le_hogp_output_callback = cb;
}

/*************************************************************************************************/
/*!
 *  \brief       配对表绑定配置
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void le_hogp_set_pair_config(u8 pair_max, u8 is_allow_cover)
{
    ble_list_config_reset(pair_max, is_allow_cover); //开1对1配对绑定
}

/*************************************************************************************************/
/*!
 *  \brief       开可配对绑定允许
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void le_hogp_set_pair_allow(void)
{
    hogp_pair_info.pair_flag = 0;
    __hogp_conn_pair_vm_do(&hogp_pair_info, 1);
    ble_list_clear_all();
}

/*************************************************************************************************/
/*!
 *  \brief      获取电量
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void le_hogp_regiest_get_battery(u8(*get_battery_cbk)(void))
{
    get_vbat_percent_call = get_battery_cbk;
}


/*************************************************************************************************/
/*!
 *  \brief      配置回连广播使用的参数
 *
 *  \param     type [in] ADV_DIRECT_IND, ADV_DIRECT_IND_LOW,ADV_IND
 *
 *  ADV_DIRECT_IND       (密集定向广播:1.28s超时,interval 固定2ms)
 *  ADV_DIRECT_IND_LOW   (定向广播:interval 和 channel 跟普通广播设置一样)
 *  ADV_IND              (无定向广播:interval 和 channel 跟普通广播设置一样)
 *  \param     adv_timeout [in]

 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void le_hogp_set_reconnect_adv_cfg(u8 adv_type, u32 adv_timeout)
{
    pair_reconnect_adv_type = adv_type;
    pair_reconnect_adv_timeout = adv_timeout;
}


/*************************************************************************************************/
/*!
 *  \brief   广播通道配置
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void le_hogp_set_adv_channel(int channel)
{
    le_adv_channel_sel = channel;
}

/*************************************************************************************************/
/*!
 *  \brief      set PNP_PID
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void le_hogp_set_PNP_info(const u8 *info)
{
    memcpy(PnP_ID, info, sizeof(PnP_ID));
}


/*************************************************************************************************/
/*!
 *  \brief      is connect
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
int ble_hid_is_connected(void)
{
    return hogp_con_handle;
}

/*************************************************************************************************/
/*!
 *  \brief      disconnect
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void le_hogp_disconnect(void)
{
    if (hogp_con_handle) {
        ble_comm_disconnect(hogp_con_handle);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      获取当前连接对方的地址信息
 *
 *  \param      [in]
 *
 *  \return     addr_type + address
 *
 *  \note
 */
/*************************************************************************************************/
u8 *ble_cur_connect_addrinfo(void)
{
    return cur_peer_addr_info;
}

/*************************************************************************************************/
/*!
 *  \brief      设置绑定地址
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_set_pair_addrinfo(u8 *addr_info)
{
    log_info("ble_set_pair_addrinfo\n");

    if (addr_info) {
        put_buf(addr_info, 7);
        memcpy(hogp_pair_info.peer_address_info, addr_info, 7);
        hogp_pair_info.head_tag = BLE_VM_HEAD_TAG;
        hogp_pair_info.tail_tag = BLE_VM_TAIL_TAG;
        hogp_pair_info.pair_flag = 1;
        hogp_pair_info.direct_adv_flag = 1;
        hogp_pair_info.direct_adv_cnt = REPEAT_DIRECT_ADV_COUNT;
        __hogp_conn_pair_vm_do(&hogp_pair_info, 1);
    } else {
        memset(&hogp_pair_info, 0, sizeof(struct pair_info_t));
    }
}


//profile 支持的report id
static const u16 report_id_handle_table[] = {
    0,
    HID_REPORT_ID_01_SEND_HANDLE,
    HID_REPORT_ID_02_SEND_HANDLE,
    HID_REPORT_ID_03_SEND_HANDLE,
    HID_REPORT_ID_04_SEND_HANDLE,
    HID_REPORT_ID_05_SEND_HANDLE,
    HID_REPORT_ID_06_SEND_HANDLE,
};

/*************************************************************************************************/
/*!
 *  \brief      hid 数据发送
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
int ble_hid_data_send(u8 report_id, u8 *data, u16 len)
{
    if (report_id == 0 || report_id > 6) {
        log_info("report_id %d,err!!!\n", report_id);
        return -1;
    }

    putchar('@');
    return ble_comm_att_send_data(hogp_con_handle, report_id_handle_table[report_id], data, len, ATT_OP_AUTO_READ_CCC);
}

/*************************************************************************************************/
/*!
 *  \brief      hid 数据发送
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_hid_key_deal_test(u16 key_msg)
{
    if (key_msg) {
        ble_hid_data_send(1, &key_msg, 2);
        key_msg = 0;//key release
        ble_hid_data_send(1, &key_msg, 2);
    }
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
int ble_hid_transfer_channel_send(u8 *packet, u16 size)
{
    /* log_info("transfer_tx(%d):", size); */
    /* log_info_hexdump(packet, size); */
    return ble_comm_att_send_data(hogp_con_handle, ATT_CHARACTERISTIC_ae42_01_VALUE_HANDLE, packet, size, ATT_OP_AUTO_READ_CCC);
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
int ble_hid_transfer_channel_send1(u8 *packet, u16 size)
{
    /* log_info("transfer_tx(%d):", size); */
    /* log_info_hexdump(packet, size); */
    return ble_comm_att_send_data(hogp_con_handle, ATT_CHARACTERISTIC_ae10_01_VALUE_HANDLE, packet, size, ATT_OP_AUTO_READ_CCC);
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
void __attribute__((weak)) ble_hid_transfer_channel_recieve(u8 *packet, u16 size)
{
    log_info("transfer_rx(%d):", size);
    log_info_hexdump(packet, size);
    //ble_hid_transfer_channel_send(packet,size);//for test
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
void __attribute__((weak)) ble_hid_transfer_channel_recieve1(u8 *packet, u16 size)
{
    log_info("transfer_rx1(%d):", size);
    log_info_hexdump(packet, size);
}

/*************************************************************************************************/
/*!
 *  \brief     控制配对模式
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_set_pair_list_control(u8 mode)
{
    bool ret = 0;
    u8 connect_address[6];
    switch (mode) {
    case 0:
        //close pair,关配对
        ret = ble_list_pair_accept(0);
        break;

    case 1:
        //open pair,开配对
        ret = ble_list_pair_accept(1);
        break;

    case 2:
        //绑定已配对设备，不再接受新的配对
        swapX(&cur_peer_addr_info[1], connect_address, 6);
        //bond current's device
        ret = ble_list_bonding_remote(connect_address, cur_peer_addr_info[0]);
        //close pair
        ble_list_pair_accept(0);
        break;

    default:
        break;
    }
    log_info("%s: %02x,ret=%02x\n", __FUNCTION__, mode, ret);
}

/*************************************************************************************************/
/*!
 *  \brief      testbox,key test
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
    if (hogp_con_handle) {
        if (get_remote_test_flag()) {
            ble_op_test_key_num(hogp_con_handle, key_num);
        } else {
            log_info("-not conn testbox\n");
        }
    }
}

void call_hogp_adv_config_set()
{
    return hogp_adv_config_set();
}

#endif


