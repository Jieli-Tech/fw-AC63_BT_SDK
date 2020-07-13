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

#include "le_common.h"
#include "le_hogp.h"
#include "standard_hid.h"

#include "rcsp_bluetooth.h"
#include "JL_rcsp_api.h"

#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_HOGP)
/* #if RCSP_BTMATE_EN */
/* #include "btstack/JL_rcsp_api.h" */
/* #include "rcsp_bluetooth.h" */
/* #endif */

#define TEST_SEND_DATA_RATE          0  //测试 music play control
#define TEST_SEND_HANDLE_VAL         HID_REPORT_ID_01_SEND_HANDLE

#if 1
extern void printf_buf(u8 *buf, u32 len);
#define log_info          printf
#define log_info_hexdump  printf_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

static void hid_timer_mouse_handler(void);

#define EIR_TAG_STRING   0xd6, 0x05, 0x08, 0x00, 'J', 'L', 'A', 'I', 'S', 'D','K'
static const char user_tag_string[] = {EIR_TAG_STRING};
//用户可配对的，这是样机跟客户开发的app配对的秘钥
const u8 link_key_data[16] = {0x06, 0x77, 0x5f, 0x87, 0x91, 0x8d, 0xd4, 0x23, 0x00, 0x5d, 0xf1, 0xd8, 0xcf, 0x0c, 0x14, 0x2b};
/* #define LOG_TAG_CONST       BT_BLE */
/* #define LOG_TAG             "[LE_S_DEMO]" */
/* #define LOG_ERROR_ENABLE */
/* #define LOG_DEBUG_ENABLE */
/* #define LOG_INFO_ENABLE */
/* #define LOG_DUMP_ENABLE */
/* #define LOG_CLI_ENABLE */
/* #include "debug.h" */

//------
#define ATT_LOCAL_PAYLOAD_SIZE    (64)                   //note: need >= 20
#define ATT_SEND_CBUF_SIZE        (256)                   //note: need >= 20,缓存大小，可修改
#define ATT_RAM_BUFSIZE           (ATT_CTRL_BLOCK_SIZE + ATT_LOCAL_PAYLOAD_SIZE + ATT_SEND_CBUF_SIZE)                   //note:
static u8 att_ram_buffer[ATT_RAM_BUFSIZE] __attribute__((aligned(4)));
//---------------

#if TEST_SEND_DATA_RATE
static u32 test_data_count;
static u32 server_timer_handle = 0;
#endif
int ble_hid_timer_handle = 0;

//---------------
#define ADV_INTERVAL_MIN          (160 * 1)

static volatile hci_con_handle_t con_handle;
static u16 cur_conn_latency;

//加密设置
static const uint8_t sm_min_key_size = 7;

//连接参数设置
static const uint8_t connection_update_enable = 1; ///0--disable, 1--enable
static uint8_t connection_update_cnt = 0; //
static uint8_t connection_update_waiting = 0; //
static const struct conn_update_param_t Peripheral_Preferred_Connection_Parameters[] = {
    {6, 9,  100, 600}, //android
    {12, 12, 30, 400}, //ios
};
#define CONN_PARAM_TABLE_CNT      (sizeof(Peripheral_Preferred_Connection_Parameters)/sizeof(struct conn_update_param_t))

#if (ATT_RAM_BUFSIZE < 64)
#error "adv_data & rsp_data buffer error!!!!!!!!!!!!"
#endif

/* #define adv_data       &att_ram_buffer[0] */
/* #define scan_rsp_data  &att_ram_buffer[32] */

static u8 adv_data_len;
static u8 adv_data[ADV_RSP_PACKET_MAX];//max is 31
static u8 scan_rsp_data_len;
static u8 scan_rsp_data[ADV_RSP_PACKET_MAX];//max is 31

static char gap_device_name[BT_NAME_LEN_MAX] = "jl_ble_test";
static u8 gap_device_name_len = 0;
static u8 ble_work_state = 0;
static u8 first_pair_flag = 0;
static u8 handle_ccc_enable = 0;
static u8 is_hogp_active = 0;
static u8 adv_ctrl_en;
static u16 adv_interval_val;

//--------------------------------------------
#define PAIR_DIREDT_ADV_EN         1//有配对，先定向广播，再普通广播
#define BLE_VM_HEAD_TAG           (0xB35C)
#define BLE_VM_TAIL_TAG           (0x5CB3)
struct pair_info_t {
    u16 head_tag;
    u8  pair_flag: 2;
    u8  direct_adv_flag: 1;
    u8  direct_adv_cnt: 5;
    u8  peer_address_info[7];
    u16 tail_tag;
};
static struct pair_info_t  conn_pair_info;
static u8 cur_peer_addr_info[7];

#define REPEAT_DIRECT_ADV_COUNT  (2)// *1.28s
#define REPEAT_DIRECT_ADV_TIMER  (100)//

#define HOLD_LATENCY_CNT_MIN  (3)  //(0~0xffff)
#define HOLD_LATENCY_CNT_MAX  (15) //(0~0xffff)
#define HOLD_LATENCY_CNT_ALL  (0xffff)

//hid device infomation
static u8 Appearance[] = {0x00, 0x00};//
static const char Manufacturer_Name_String[] = "zhuhai_jieli";
static const char Model_Number_String[] = "hid_mouse";
static const char Serial_Number_String[] = "000000";
static const char Hardware_Revision_String[] = "0.0.1";
static const char Firmware_Revision_String[] = "0.0.1";
static const char Software_Revision_String[] = "0.0.1";
static const u8 System_ID[] = {0, 0, 0, 0, 0, 0, 0, 0};
static const u8 PnP_ID[] = {0x02, 0x17, 0x27, 0x40, 0x00, 0x23, 0x00};
static const u8 hid_information[] = {0x01, 0x01, 0x00, 0x02};

static  u8 *report_map;
static  u16 report_map_size;
#define HID_REPORT_MAP_DATA    report_map
#define HID_REPORT_MAP_SIZE    report_map_size

static u8 hid_battery_level = 88;
//------------------------------------------------------
static void (*app_recieve_callback)(void *priv, void *buf, u16 len) = NULL;
static void (*app_ble_state_callback)(void *priv, ble_state_e state) = NULL;
static void (*ble_resume_send_wakeup)(void) = NULL;
static u32 channel_priv;

static int app_send_user_data_check(u16 len);
static int app_send_user_data_do(void *priv, u8 *data, u16 len);
/* static int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type); */
int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type);
static void hid_consumer_send_start(u16 key);
static void hid_consumer_send_end(void);

void auto_shutdown_disable(void);
// Complete Local Name  默认的蓝牙名字

//------------------------------------------------------
//广播参数设置
static void advertisements_setup_init();
extern const char *bt_get_local_name();
static int set_adv_enable(void *priv, u32 en);
static int get_buffer_vaild_len(void *priv);
extern void watchdog_clear(void);

static void bt_ble_adv_fast_to_normal(void);
void ble_hid_transfer_channel_recieve(u8 *packet, u16 size);
//------------------------------------------------------
//------------------------------------------------------
void le_hogp_set_icon(u16 class_type)
{
    memcpy(Appearance, &class_type, 2);
}

void le_hogp_set_ReportMap(u8 *map, u16 size)
{
    report_map = map;
    report_map_size = size;
}

static void send_request_connect_parameter(u8 table_index)
{
    static struct conn_update_param_t param;

    memcpy(&param, &Peripheral_Preferred_Connection_Parameters[table_index], sizeof(struct conn_update_param_t)); //static ram

    if (cur_conn_latency == 0xffff) {
        log_info("disable latency!!!\n");
        param.latency = 0;
    }

    log_info("update_request:-%d-%d-%d-%d-\n", param.interval_min, param.interval_max, param.latency, param.timeout);
    if (con_handle) {
        ble_user_cmd_prepare(BLE_CMD_REQ_CONN_PARAM_UPDATE, 2, con_handle, (u32)&param);
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

    if (cur_conn_latency != 0xffff) {
        //not creat connection,judge
        if ((conn_interval == 12) && (conn_latency == 4)) {
            //ios&hid
            log_info("is ios,do update_again\n");
            connection_update_cnt = 1;
            check_connetion_updata_deal();
        } else if ((conn_latency == 0)
                   && (connection_update_cnt == CONN_PARAM_TABLE_CNT)
                   && (Peripheral_Preferred_Connection_Parameters[0].latency != 0)) {
            log_info("latency is 0,update_again\n");
            connection_update_cnt = 0;
            check_connetion_updata_deal();
        }
    }

    cur_conn_latency = conn_latency;

    if (ble_hid_timer_handle) {
        sys_s_hi_timer_modify(ble_hid_timer_handle, (u32)(conn_interval * 1.25));
    }
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

static void conn_pair_vm_do(struct pair_info_t *info, u8 rw_flag)
{
#if PAIR_DIREDT_ADV_EN
    int ret;
    int vm_len = sizeof(struct pair_info_t);

    log_info("-conn_pair_info vm_do:%d\n", rw_flag);
    if (rw_flag == 0) {
        ret = syscfg_read(CFG_BLE_MODE_INFO, (u8 *)info, vm_len);
        if (!ret) {
            log_info("-null--\n");
        }

        if ((BLE_VM_HEAD_TAG == info->head_tag) && (BLE_VM_TAIL_TAG == info->tail_tag)) {
            log_info("-exist--\n");
            log_info_hexdump((u8 *)info, vm_len);
            info->direct_adv_flag = 1;
        } else {
            memset(info, 0, vm_len);
            info->head_tag = BLE_VM_HEAD_TAG;
            info->tail_tag = BLE_VM_TAIL_TAG;
        }
    } else {
        info->direct_adv_flag = 1;
        syscfg_write(CFG_BLE_MODE_INFO, (u8 *)info, vm_len);
    }
#endif
}

static void timer_adv_deal(void *priv)
{
    log_info("%s,%d\n", __FUNCTION__, (u32)priv);

    if (con_handle) {
        return;
    }

    if (priv) {
        //high
        bt_ble_adv_enable(1);
    } else {
        //normal
        bt_ble_adv_enable(0);
        adv_interval_val = ADV_INTERVAL_MIN;
        bt_ble_adv_enable(1);
    }
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
            first_pair_flag = 1;
            memcpy(conn_pair_info.peer_address_info, &packet[4], 7);
            conn_pair_info.pair_flag = 0;
            ble_user_cmd_prepare(BLE_CMD_LATENCY_HOLD_CNT, 2, con_handle, HOLD_LATENCY_CNT_MAX); //
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


#if TEST_SEND_DATA_RATE
static void server_timer_handler(void)
{
    static u8 send_count = 0;

    if (!con_handle) {
        test_data_count = 0;
        return;
    }

    if (++send_count > 8) {
        putchar('%');
        hid_consumer_send_start(CONSUMER_PLAY_PAUSE);
        hid_consumer_send_end();
        send_count = 0;
    }

    if (test_data_count) {
        log_info("\n-data_rate: %d bps-\n", test_data_count * 8);
        test_data_count = 0;
    }
}

static void server_timer_start(void)
{
    server_timer_handle  = sys_timer_add(NULL, server_timer_handler, 1000);
}

static void server_timer_stop(void)
{
    if (server_timer_handle) {
        sys_timeout_del(server_timer_handle);
        server_timer_handle = 0;
    }
}

void test_data_send_packet(void)
{
    u32 vaild_len = get_buffer_vaild_len(0);
    if (vaild_len) {
        /* printf("\n---test_data_len = %d---\n",vaild_len); */
        app_send_user_data(TEST_SEND_HANDLE_VAL, &test_data_send_packet, vaild_len, ATT_OP_AUTO_READ_CCC);
        test_data_count += vaild_len;
    }
    watchdog_clear();
}
#endif


static void can_send_now_wakeup(void)
{
    /* putchar('E'); */
    if (ble_resume_send_wakeup) {
        ble_resume_send_wakeup();
    }

#if TEST_SEND_DATA_RATE
    /* test_data_send_packet(); */
#endif
}

static void resume_all_ccc_enable(void)
{
    log_info("resume_all_ccc_enable\n");
    att_set_ccc_config(ATT_CHARACTERISTIC_2a4d_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    att_set_ccc_config(ATT_CHARACTERISTIC_2a4d_02_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    att_set_ccc_config(ATT_CHARACTERISTIC_2a4d_03_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);
    att_set_ccc_config(ATT_CHARACTERISTIC_ae42_01_CLIENT_CONFIGURATION_HANDLE, ATT_OP_NOTIFY);

    set_ble_work_state(BLE_ST_NOTIFY_IDICATE);
    check_connetion_updata_deal();
    ble_user_cmd_prepare(BLE_CMD_LATENCY_HOLD_CNT, 2, con_handle, HOLD_LATENCY_CNT_MIN); //
}

extern u8 ble_update_get_ready_jump_flag(void);
/*
 * @section Packet Handler
 *
 * @text The packet handler is used to:
 *        - stop the counter after a disconnect
 *        - send a notification when the requested ATT_EVENT_CAN_SEND_NOW is received
 */

/* LISTING_START(packetHandler): Packet Handler */
static void cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    int mtu;
    u32 tmp;

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
            switch (hci_event_le_meta_get_subevent_code(packet)) {
            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE: {
                switch (packet[3]) {
                case BT_OP_SUCCESS:
                    con_handle = little_endian_read_16(packet, 4);
                    first_pair_flag = 0;
                    connection_update_cnt = 0;
                    connection_update_waiting = 0;
					cur_conn_latency = 0xffff;
                    log_info("HCI_SUBEVENT_LE_CONNECTION_COMPLETE: %0x\n", con_handle);
                    connection_update_complete_success(packet + 8);
                    ble_user_cmd_prepare(BLE_CMD_ATT_SEND_INIT, 4, con_handle, att_ram_buffer, ATT_RAM_BUFSIZE, ATT_LOCAL_PAYLOAD_SIZE);
                    set_ble_work_state(BLE_ST_CONNECT);
                    log_info_hexdump(packet + 7, 7);
                    memcpy(cur_peer_addr_info, packet + 7, 7);

                    ble_user_cmd_prepare(BLE_CMD_LATENCY_HOLD_CNT, 2, con_handle, HOLD_LATENCY_CNT_MAX); //

                    //清除PC端历史键值
                    /* extern void clear_mouse_packet_data(void); */
                    /* sys_timeout_add(NULL, clear_mouse_packet_data, 50); */
#if RCSP_BTMATE_EN
                    JL_rcsp_auth_reset();
                    /* rcsp_dev_select(RCSP_BLE); */
                    rcsp_init();
#endif
                    break;

                case BT_ERR_ADVERTISING_TIMEOUT:
                    //定向广播超时结束
                    log_info("BT_ERR_ADVERTISING_TIMEOUT\n");
                    set_ble_work_state(BLE_ST_IDLE);

                    if (conn_pair_info.direct_adv_cnt) {
                        //high duty adv
                        sys_timeout_add((void *)1, timer_adv_deal, REPEAT_DIRECT_ADV_TIMER);
                    } else {
#if 1
                        //快速广播,调快广播周期
                        adv_interval_val = 0x20;
                        bt_ble_adv_enable(1);
                        sys_timeout_add(0, timer_adv_deal, 3000); //
#else
                        //normal adv
                        bt_ble_adv_enable(1);
#endif
                    }
                    break;

                default:
                    break;
                }
            }
            break;

			case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE:
			connection_update_waiting = 0;
			connection_update_complete_success(packet);
			break;
			}
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE:
            log_info("HCI_EVENT_DISCONNECTION_COMPLETE: %0x\n", packet[5]);
#if RCSP_BTMATE_EN
            rcsp_exit();
#endif
            con_handle = 0;
            ble_user_cmd_prepare(BLE_CMD_ATT_SEND_INIT, 4, con_handle, 0, 0, 0);
            set_ble_work_state(BLE_ST_DISCONN);
            /* bt_ble_adv_enable(1); */
			if (!ble_update_get_ready_jump_flag()) {
				if(packet[5] == 0x08){
					//超时断开,快速广播
					bt_ble_adv_fast_to_normal();
				}
				else{
					//其他断开的，正常广播
					adv_interval_val = ADV_INTERVAL_MIN;
					bt_ble_adv_enable(1);
				}
			} else {
				log_info("no open adv\n");
			}
            break;

        case ATT_EVENT_MTU_EXCHANGE_COMPLETE:
            mtu = att_event_mtu_exchange_complete_get_MTU(packet) - 3;
            log_info("ATT MTU = %u\n", mtu);
            ble_user_cmd_prepare(BLE_CMD_ATT_MTU_SIZE, 1, mtu);
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
                connection_update_waiting = 1;
				connection_update_cnt = CONN_PARAM_TABLE_CNT;
            }
            break;

        case HCI_EVENT_ENCRYPTION_CHANGE:
            log_info("HCI_EVENT_ENCRYPTION_CHANGE= %d\n", packet[2]);
            if (!packet[2]) {
                if (first_pair_flag) {
#if PAIR_DIREDT_ADV_EN
                    conn_pair_info.pair_flag = 1;
                    conn_pair_vm_do(&conn_pair_info, 1);
#endif
                } else {
                    if ((!conn_pair_info.pair_flag) || memcmp(cur_peer_addr_info, conn_pair_info.peer_address_info, 7)) {
                        log_info("record reconnect peer\n");
                        put_buf(cur_peer_addr_info, 7);
                        put_buf(conn_pair_info.peer_address_info, 7);
                        memcpy(conn_pair_info.peer_address_info, cur_peer_addr_info, 7);
                        conn_pair_info.pair_flag = 1;
                        conn_pair_vm_do(&conn_pair_info, 1);
                    }
                    resume_all_ccc_enable();
                }
            }
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
            log_info("\n------read gap_name: %s \n", gap_device_name);
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

    case ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a19_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_02_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_03_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_ae42_01_CLIENT_CONFIGURATION_HANDLE:
        if (buffer) {
            buffer[0] = att_get_ccc_config(handle);
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

    log_info("write_callback, handle= 0x%04x,size = %d\n", handle, buffer_size);

    switch (handle) {

    case ATT_CHARACTERISTIC_2a00_01_VALUE_HANDLE:
        break;

#if RCSP_BTMATE_EN
    case ATT_CHARACTERISTIC_ae02_01_CLIENT_CONFIGURATION_HANDLE:
#if (TCFG_HID_AUTO_SHUTDOWN_TIME)
        auto_shutdown_disable();
#endif
#endif
    case ATT_CHARACTERISTIC_ae42_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_02_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_01_CLIENT_CONFIGURATION_HANDLE:
    case ATT_CHARACTERISTIC_2a4d_03_CLIENT_CONFIGURATION_HANDLE:
        set_ble_work_state(BLE_ST_NOTIFY_IDICATE);
        ble_user_cmd_prepare(BLE_CMD_LATENCY_HOLD_CNT, 2, con_handle, HOLD_LATENCY_CNT_MIN); //
	case ATT_CHARACTERISTIC_2a05_01_CLIENT_CONFIGURATION_HANDLE:
	case ATT_CHARACTERISTIC_2a19_01_CLIENT_CONFIGURATION_HANDLE:
		if ((cur_conn_latency == 0)
				&& (connection_update_waiting == 0)
				&& (connection_update_cnt == CONN_PARAM_TABLE_CNT)
				&& (Peripheral_Preferred_Connection_Parameters[0].latency != 0)) {
			connection_update_cnt = 0;//update again
		}
		check_connetion_updata_deal();
		log_info("\n------write ccc:%04x,%02x\n", handle, buffer[0]);
		att_set_ccc_config(handle, buffer[0]);
		break;

#if RCSP_BTMATE_EN
    case ATT_CHARACTERISTIC_ae01_01_VALUE_HANDLE:
        printf("rcsp_read:%x 0x%x\n", buffer_size, app_recieve_callback);
        ble_user_cmd_prepare(BLE_CMD_LATENCY_HOLD_CNT, 2, con_handle, HOLD_LATENCY_CNT_ALL); //
        if (app_recieve_callback) {
            app_recieve_callback(0, buffer, buffer_size);
        }
        break;
#endif

    case ATT_CHARACTERISTIC_ae41_01_VALUE_HANDLE:
        ble_hid_transfer_channel_recieve(buffer, buffer_size);
        break;

    default:
        break;
    }
    return 0;
}

/* static int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type) */
int app_send_user_data(u16 handle, u8 *data, u16 len, u8 handle_type)
{
    u32 ret = APP_BLE_NO_ERROR;

    if (!con_handle) {
        return APP_BLE_OPERATION_ERROR;
    }

#if 1
    if (conn_pair_info.pair_flag && cur_conn_latency) { //connect is latency
        //回连带latency,不需要等write CCC,响应快
        handle_type = ATT_OP_NOTIFY;
    }
#endif

    putchar('$');
    /* put_buf(data, len); */

    ret = ble_user_cmd_prepare(BLE_CMD_ATT_SEND_DATA, 4, handle, data, len, handle_type);
    if (ret == BLE_BUFFER_FULL) {
        ret = APP_BLE_BUFF_FULL;
    }

    if (ret) {
        log_info("app_send_fail:%d !!!!!!\n", ret);
    }
    return ret;
}

//------------------------------------------------------
static u8 adv_name_ok = 0;

static int make_set_adv_data(void)
{
    u8 offset = 0;
    u8 *buf = adv_data;

    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_FLAGS, 0x06, 1);
    offset += make_eir_packet_val(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_16BIT_SERVICE_UUIDS, HID_UUID_16, 2);
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_APPEARANCE_DATA, Appearance, 2);

    //check set name is ok?
    u8 name_len = gap_device_name_len;
    u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);

    if (name_len <= vaild_len) {
        adv_name_ok = 1;
        offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_device_name, name_len);
        log_info("name in adv_data\n");
    } else {
        adv_name_ok = 0;
    }

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***adv_data overflow!!!!!!\n");
        return -1;
    }
    log_info("adv_data(%d):", offset);
    log_info_hexdump(buf, offset);
    adv_data_len = offset;
    ble_user_cmd_prepare(BLE_CMD_ADV_DATA, 2, offset, buf);
    return 0;
}

static int make_set_rsp_data(void)
{
    u8 offset = 0;
    u8 *buf = scan_rsp_data;

#if RCSP_BTMATE_EN
    u8  tag_len = sizeof(user_tag_string);
    offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA, (void *)user_tag_string, tag_len);
#endif

    if (!adv_name_ok) {
        u8 name_len = gap_device_name_len;
        u8 vaild_len = ADV_RSP_PACKET_MAX - (offset + 2);
        if (name_len > vaild_len) {
            name_len = vaild_len;
        }
        offset += make_eir_packet_data(&buf[offset], offset, HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME, (void *)gap_device_name, name_len);
        log_info("name in rsp_data\n");
    }

    if (offset > ADV_RSP_PACKET_MAX) {
        puts("***rsp_data overflow!!!!!!\n");
        return -1;
    }

    log_info("rsp_data(%d):", offset);
    log_info_hexdump(buf, offset);
    scan_rsp_data_len = offset;
    ble_user_cmd_prepare(BLE_CMD_RSP_DATA, 2, offset, buf);
    return 0;
}

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
    *len = sizeof(profile_data);
    return (u8 *)profile_data;
}

//广播参数设置
static void advertisements_setup_init()
{
    uint8_t adv_type = ADV_IND;
    uint8_t adv_channel = ADV_CHANNEL_ALL;
    int   ret = 0;

    ret |= make_set_adv_data();
    ret |= make_set_rsp_data();

    if (ret) {
        puts("adv_setup_init fail !!!!!!\n");
        ASSERT(0);
    }

    if (conn_pair_info.pair_flag && conn_pair_info.direct_adv_flag && conn_pair_info.direct_adv_cnt) {
        log_info(">>>direct_adv......\n");
        adv_type = ADV_DIRECT_IND;
        conn_pair_info.direct_adv_cnt--;
        ble_user_cmd_prepare(BLE_CMD_ADV_PARAM_EXT, 4, adv_interval_val, adv_type, adv_channel, conn_pair_info.peer_address_info);
    } else {
        ble_user_cmd_prepare(BLE_CMD_ADV_PARAM, 3, adv_interval_val, adv_type, adv_channel);
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


extern void reset_PK_cb_register(void (*reset_pk)(u32 *));
void ble_sm_setup_init(io_capability_t io_type, u8 auth_req, uint8_t min_key_size, u8 security_en)
{
    //setup SM: Display only
    sm_init();
    sm_set_io_capabilities(io_type);
    sm_set_authentication_requirements(auth_req);
    sm_set_encryption_key_size_range(min_key_size, 16);
    /* sm_set_request_security(security_en); */
    sm_event_callback_set(&cbk_sm_packet_handler);

    if (io_type == IO_CAPABILITY_DISPLAY_ONLY) {
        reset_PK_cb_register(reset_passkey_cb);
    }
}


extern void att_server_flow_enable(u8 enable);
extern void le_device_db_init(void);
void ble_profile_init(void)
{
    printf("ble profile init\n");
    le_device_db_init();

#if PASSKEY_ENTER_ENABLE
    ble_sm_setup_init(IO_CAPABILITY_DISPLAY_ONLY, SM_AUTHREQ_MITM_PROTECTION, 7, TCFG_BLE_SECURITY_EN);
#else
    ble_sm_setup_init(IO_CAPABILITY_NO_INPUT_NO_OUTPUT, SM_AUTHREQ_BONDING, 7, TCFG_BLE_SECURITY_EN);
#endif

    /* setup ATT server */
    att_server_init(profile_data, att_read_callback, att_write_callback);
    att_server_register_packet_handler(cbk_packet_handler);
    /* gatt_client_register_packet_handler(packet_cbk); */

    // register for HCI events
    hci_event_callback_set(&cbk_packet_handler);
    /* ble_l2cap_register_packet_handler(packet_cbk); */
    /* sm_event_packet_handler_register(packet_cbk); */
    le_l2cap_register_packet_handler(&cbk_packet_handler);
    /* att_server_flow_enable(1); */
}



static int set_adv_enable(void *priv, u32 en)
{
    ble_state_e next_state, cur_state;

    if (!adv_ctrl_en) {
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
    ble_user_cmd_prepare(BLE_CMD_ADV_ENABLE, 1, en);
    return APP_BLE_NO_ERROR;
}

static int ble_disconnect(void *priv)
{
    if (con_handle) {
        if (BLE_ST_SEND_DISCONN != get_ble_work_state()) {
            log_info(">>>ble send disconnect\n");
            set_ble_work_state(BLE_ST_SEND_DISCONN);
            ble_user_cmd_prepare(BLE_CMD_DISCONNECT, 1, con_handle);
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
    ble_user_cmd_prepare(BLE_CMD_ATT_VAILD_LEN, 1, &vaild_len);
    return vaild_len;
}

static int app_send_user_data_do(void *priv, u8 *data, u16 len)
{
#if PRINT_DMA_DATA_EN
    if (len < 128) {
        log_info("-le_tx(%d):");
        log_info_hexdump(data, len);
    } else {
        putchar('L');
    }
#endif
    return app_send_user_data(HID_REPORT_ID_01_SEND_HANDLE, data, len, ATT_OP_AUTO_READ_CCC);
}

#if RCSP_BTMATE_EN
static int rcsp_send_user_data_do(void *priv, u8 *data, u16 len)
{
    log_info("rcsp_tx:%x\n", len);
#if PRINT_DMA_DATA_EN
    if (len < 128) {
        log_info("-dma_tx(%d):");
        log_info_hexdump(data, len);
    } else {
        putchar('L');
    }
#endif
    return app_send_user_data(ATT_CHARACTERISTIC_ae02_01_VALUE_HANDLE, data, len, ATT_OP_AUTO_READ_CCC);
}
#endif
static int app_send_user_data_check(u16 len)
{
    u32 buf_space = get_buffer_vaild_len(0);
    if (len <= buf_space) {
        return 1;
    }
    return 0;
}


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

void bt_ble_adv_enable(u8 enable)
{
    set_adv_enable(0, enable);
}

/* typedef struct { */
    /* u8 button; */
    /* s8 x; */
    /* s8 y; */
/* } hid_ctl_info_t; */
/* #define M_STEP   50 */
/* static hid_ctl_info_t test_hid_point_table[] = { */
    /* {0, 0, M_STEP}, */
    /* {0, M_STEP, 0}, */
    /* {0, 0, -M_STEP}, */
    /* {0, -M_STEP, 0}, */
/* }; */

/* static void hid_timer_handler(void *param) */
/* { */
    /* static u8 test_id = 0; */
    /* if (!con_handle) { */
        /* return; */
    /* } */

    /* if (get_ble_work_state() != BLE_ST_NOTIFY_IDICATE) { */
        /* return; */
    /* } */

    /* int ret; */
    /* if (test_id < sizeof(test_hid_point_table) / sizeof(hid_ctl_info_t)) { */
        /* int ret = app_send_user_data_do(0, &test_hid_point_table[test_id], 3); */
        /* if (ret) { */
            /* printf("s_fail!\n"); */
        /* } else { */
            /* test_id++; */
        /* } */
    /* } else { */
        /* test_id = 0; */
    /* } */
/* } */

static u8 hogp_idle_query(void)
{
    return !is_hogp_active;
}

REGISTER_LP_TARGET(le_hogp_target) = {
    .name = "le_hogp",
    .is_idle = hogp_idle_query,
};

static const char ble_ext_name[] = "(BLE)";
void bt_ble_init(void)
{
    log_info("***** ble_init******\n");
#if 1
    char *name_p;
    u8 ext_name_len = sizeof(ble_ext_name) - 1;

    name_p = bt_get_local_name();
    gap_device_name_len = strlen(name_p);
    if (gap_device_name_len > (BT_NAME_LEN_MAX - ext_name_len)) {
        gap_device_name_len = BT_NAME_LEN_MAX - ext_name_len;
    }

    //增加后缀，区分名字
    memcpy(gap_device_name, name_p, gap_device_name_len);
    memcpy(&gap_device_name[gap_device_name_len], ble_ext_name, ext_name_len);
    gap_device_name_len += ext_name_len;
#else
    const char name_p[] = "std_mouse";
    u8 ext_name_len = sizeof(ble_ext_name);

    gap_device_name_len = sizeof(name_p) - 1;
    memcpy(gap_device_name, name_p, gap_device_name_len);
    memcpy(&gap_device_name[gap_device_name_len], ble_ext_name, ext_name_len);
    gap_device_name_len += ext_name_len;
#endif

    log_info("ble name(%d): %s \n", gap_device_name_len, gap_device_name);

    memset(&conn_pair_info, 0, sizeof(struct pair_info_t));
    conn_pair_vm_do(&conn_pair_info, 0);
    adv_interval_val = ADV_INTERVAL_MIN;
    set_ble_work_state(BLE_ST_INIT_OK);

#if TEST_SEND_DATA_RATE
    server_timer_start();
#endif
}

void bt_ble_exit(void)
{
    log_info("***** ble_exit******\n");
    bt_ble_adv_enable(0);

    if (ble_hid_timer_handle) {
        sys_s_hi_timer_del(ble_hid_timer_handle);
        ble_hid_timer_handle = 0;
    }

#if TEST_SEND_DATA_RATE
    server_timer_stop();
#endif

}

static void bt_ble_adv_fast_to_normal(void)
{
    log_info("%s\n", __FUNCTION__);

    if (!adv_ctrl_en) {
        return;
    }

#if PAIR_DIREDT_ADV_EN
    if (conn_pair_info.pair_flag && conn_pair_info.direct_adv_flag) {
        conn_pair_info.direct_adv_cnt = REPEAT_DIRECT_ADV_COUNT;
        bt_ble_adv_enable(1);
    } else
#endif
    {
        //快速广播,调快广播周期
        adv_interval_val = 0x20;
        bt_ble_adv_enable(1);
        sys_timeout_add(0, timer_adv_deal, 3000); //
    }
}

static void timer_disconn_deal(void *priv)
{
    if (con_handle) {
        ble_user_cmd_prepare(BLE_CMD_LATENCY_HOLD_CNT, 2, con_handle, HOLD_LATENCY_CNT_MAX); //
        ble_disconnect(NULL);
    }
}

void ble_module_enable(u8 en)
{
    log_info("mode_en:%d\n", en);
    if (en) {
        adv_ctrl_en = 1;
        /* bt_ble_adv_enable(1); */
        bt_ble_adv_fast_to_normal();
    } else {
        if (con_handle) {
            adv_ctrl_en = 0;
            ble_user_cmd_prepare(BLE_CMD_LATENCY_HOLD_CNT, 2, con_handle, HOLD_LATENCY_CNT_MAX); //
            ble_disconnect(NULL);
        } else {
            bt_ble_adv_enable(0);
            adv_ctrl_en = 0;
        }
    }
}

void ble_app_disconnect(void)
{
    ble_disconnect(NULL);
}

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
#else
static const struct ble_server_operation_t mi_ble_operation = {
    .adv_enable = set_adv_enable,
    .disconnect = ble_disconnect,
    .get_buffer_vaild = get_buffer_vaild_len,
    .send_data = (void *)app_send_user_data_do,
    .regist_wakeup_send = regiest_wakeup_send,
    .regist_recieve_cbk = regiest_recieve_cbk,
    .regist_state_cbk = regiest_state_cbk,
};
#endif

void ble_get_server_operation_table(struct ble_server_operation_t **interface_pt)
{
    *interface_pt = (void *)&mi_ble_operation;
}

extern bool get_remote_test_flag();
void ble_server_send_test_key_num(u8 key_num)
{
    if (con_handle) {
        if (get_remote_test_flag()) {
            ble_user_cmd_prepare(BLE_CMD_SEND_TEST_KEY_NUM, 2, con_handle, key_num);
        } else {
            log_info("-not conn testbox\n");
        }
    }
}

bool ble_msg_deal(u32 param)
{
    struct ble_server_operation_t *test_opt;
    ble_get_server_operation_table(&test_opt);

#if 0//for test key
    switch (param) {
    case MSG_BT_PP:
        break;

    case MSG_BT_NEXT_FILE:
        break;

    case MSG_HALF_SECOND:
        /* putchar('H'); */
        break;

    default:
        break;
    }
#endif

    return FALSE;
}

void input_key_handler(u8 key_status, u8 key_number)
{
}

//for test
static void hid_consumer_send_start(u16 key)
{
    int ret = app_send_user_data_do(0, &key, 2);
    if (ret) {
        printf("send start fail!!!\n");
    }
}

//for test
static void hid_consumer_send_end(void)
{
    u16 key = 0;
    int ret = app_send_user_data_do(0, &key, 2);
    if (ret) {
        printf("send end fail!!!\n");
    }
}

void ble_hid_key_deal_test(u16 key_msg)
{
    if (key_msg) {
        hid_consumer_send_start(key_msg);
        hid_consumer_send_end();
    }
}

int ble_hid_is_connected(void)
{
    return con_handle;
}

static const u16 report_id_handle_table[4] = {
    0,
    HID_REPORT_ID_01_SEND_HANDLE,
    HID_REPORT_ID_02_SEND_HANDLE,
    HID_REPORT_ID_03_SEND_HANDLE,
};

int ble_hid_data_send(u8 report_id, u8 *data, u16 len)
{
    if (report_id == 0 || report_id > 3) {
        log_info("report_id %d,err!!!\n", report_id);
        return -1;
    }
    return app_send_user_data(report_id_handle_table[report_id], data, len, ATT_OP_AUTO_READ_CCC);
}


int ble_hid_transfer_channel_send(u8 *packet, u16 size)
{
    log_info("transfer_tx(%d):", size);
    log_info_hexdump(packet, size);
    return app_send_user_data(ATT_CHARACTERISTIC_ae42_01_VALUE_HANDLE, packet, size, ATT_OP_AUTO_READ_CCC);
}

void __attribute__((weak)) ble_hid_transfer_channel_recieve(u8 *packet, u16 size)
{
    log_info("transfer_rx(%d):", size);
    log_info_hexdump(packet, size);
    //ble_hid_transfer_channel_send(packet,size);//for test
}


#endif


