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
#include "usb/usb_phy.h"

#if CONFIG_APP_DONGLE && TCFG_USER_BLE_ENABLE

#if LE_DEBUG_PRINT_EN
#define log_info(x, ...)  printf("[BLE_DG]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

#if CONFIG_BLE_HIGH_SPEED
//ATT发送的包长,    note: 23 <=need >= MTU
#define ATT_LOCAL_MTU_SIZE        (247)
#else
#define ATT_LOCAL_MTU_SIZE        (64)
#endif

//ATT缓存的buffer支持缓存数据包个数
#if RCSP_BTMATE_EN
#if CONFIG_BLE_HIGH_SPEED
//ATT发送的包长,    note: 23 <=need >= MTU
#define ATT_LOCAL_MTU_SIZE        (247)
#define ATT_PACKET_NUMS_MAX       (5)
#else
#define ATT_LOCAL_MTU_SIZE        (64)
#define ATT_PACKET_NUMS_MAX       (2 * 8 * CONFIG_BT_GATT_CLIENT_NUM)
#endif
#else
#define ATT_LOCAL_MTU_SIZE        (64)
#define ATT_PACKET_NUMS_MAX       (10)
#endif

//ATT缓存的buffer大小,  note: need >= 23,可修改
#define ATT_SEND_CBUF_SIZE        (ATT_PACKET_NUMS_MAX * (ATT_PACKET_HEAD_SIZE + ATT_LOCAL_MTU_SIZE))

//搜索类型
#define SET_SCAN_TYPE       SCAN_ACTIVE
//搜索 周期大小
#define SET_SCAN_INTERVAL   ADV_SCAN_MS(24) // unit: 0.625ms
//搜索 窗口大小
#define SET_SCAN_WINDOW     ADV_SCAN_MS(8)  // unit: 0.625ms ,<= SET_SCAN_INTERVAL

//连接周期
#define BASE_INTERVAL_MIN   (6)//最小的interval
#define SET_CONN_INTERVAL   (BASE_INTERVAL_MIN*3) //(unit:1.25ms)
//连接latency
#define SET_CONN_LATENCY    0  //(unit:conn_interval)
//连接超时
#define SET_CONN_TIMEOUT    100 //(unit:10ms)

//建立连接超时
#define SET_CREAT_CONN_TIMEOUT    0 //(unit:ms)

static u8 dg_pair_reconnect_search_profile = 0; /*配对回连是否搜索profile*/
//---------------------------------------------------------------------------
static scan_conn_cfg_t dg_central_scan_cfg;
static u16 dg_ble_central_write_handle;
static u16 dg_conn_handle, dg_conn_handle2;
static u8  cur_connect_pair_process;

//配对信息
#define   CLIENT_PAIR_BOND_ENABLE    CONFIG_BT_SM_SUPPORT_ENABLE
#define   PAIR_BOND_TAG              0x53
struct ctl_pair_info_t {
    u8 head_tag;
    u8 match_dev_id; //client_match_cfg_t 搜索表，设备的顺序0~
    u8 pair_flag;
    u8 peer_address_info[7];
    u16 conn_handle;
    u16 conn_interval;
    u16 conn_latency;
    u16 conn_timeout;
    u16 write_handle;
};
static u8 bt_connected = 0;
static u8 ota_is_support = 0;//是否支持ota升级
/* static u8 pair_bond_info[8]; //tag + addr_type + address */
/* static u8 cur_peer_addr_info[7];//当前连接对方地址信息 */
static struct ctl_pair_info_t cur_conn_info;
static struct ctl_pair_info_t record_bond_info[SUPPORT_MAX_GATT_CLIENT];

static u16 dg_timer_id = 0;

//dongle 上电开配对管理,若配对失败,没有配对设备，停止搜索
#define POWER_ON_RECONNECT_START   (1)   // 上电先回连
#define POWER_ON_SWITCH_TIME       (3500)//unit ms,切换搜索回连周期
#define MATCH_DEVICE_RSSI_LEVEL    (-50)  //RSSI 阈值

//------------------------------------------------------
extern const char *bt_get_local_name();
extern void clr_wdt(void);
u8 dg_central_get_match_id(u16 conn_handle);
u8 dg_central_get_ota_is_support(u16 conn_handle);
static int dg_pair_vm_do(struct ctl_pair_info_t *info, u8 rw_flag);
static void dg_scan_conn_config_set(struct ctl_pair_info_t *pair_info);
extern void check_is_reconn_succ(u8 state, u16 con_handle);
extern int dongle_ble_hid_input_handler(u8 *packet, u16 size);
extern int dongle_second_ble_hid_input_handler(u8 *packet, u16 size);
//------------------------------------------------------
//输入passkey 加密
#define PASSKEY_ENABLE                     0

static const sm_cfg_t dg_sm_init_config = {
    .master_security_auto_req = 1,
    .master_set_wait_security = 1,

#if PASSKEY_ENABLE
    .io_capabilities = IO_CAPABILITY_KEYBOARD_ONLY,
#else
    .io_capabilities = IO_CAPABILITY_NO_INPUT_NO_OUTPUT,
#endif

    .authentication_req_flags = SM_AUTHREQ_BONDING | SM_AUTHREQ_MITM_PROTECTION,
    .min_key_size = 7,
    .max_key_size = 16,
    .sm_cb_packet_handler = NULL,
};

extern const gatt_client_cfg_t central_client_init_cfg;

static gatt_ctrl_t dg_gatt_control_block = {
    //public
    .mtu_size = ATT_LOCAL_MTU_SIZE,
    .cbuffer_size = ATT_SEND_CBUF_SIZE,
    .multi_dev_flag	= 0,

    //config
    .server_config = NULL,

#if CONFIG_BT_GATT_CLIENT_NUM
    .client_config = &central_client_init_cfg,
#else
    .client_config = NULL,
#endif

#if CONFIG_BT_SM_SUPPORT_ENABLE
    .sm_config = &dg_sm_init_config,
#else
    .sm_config = NULL,
#endif
    //cbk,event handle
    .hci_cb_packet_handler = NULL,
};


static int dg_central_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param);

static const gatt_client_cfg_t central_client_init_cfg = {
    .event_packet_handler = dg_central_event_packet_handler,
};



//---------------------------------------------------------------------------
//指定搜索uuid
static const target_uuid_t  dongle_search_ble_uuid_table[] = {
    {
        .services_uuid16 = 0x1800,
        .characteristic_uuid16 = 0x2a00,
        .opt_type = ATT_PROPERTY_READ,
    },

    {
        .services_uuid16 = 0x180a,
        .characteristic_uuid16 = 0x2a50,
        .opt_type = ATT_PROPERTY_READ,
    },

    /* { */
    /* .services_uuid16 = 0x1812, */
    /* .characteristic_uuid16 = 0x2a4b, */
    /* .opt_type = ATT_PROPERTY_READ, */
    /* }, */

    {
        .services_uuid16 = 0x1812,
        .characteristic_uuid16 = 0x2a4b,
        .opt_type = ATT_PROPERTY_READ,
        .read_long_enable = 1,
    },

    {
        .services_uuid16 = 0x1812,
        .characteristic_uuid16 = 0x2a4d,
        .opt_type = ATT_PROPERTY_NOTIFY,
        .read_report_reference = 1,
    },

#if RCSP_BTMATE_EN //确保远端设备是否支持ota
    {
        .services_uuid16 = 0xae00,
        .characteristic_uuid16 = 0xae01,
        .opt_type = ATT_PROPERTY_WRITE_WITHOUT_RESPONSE,
    },

    {
        .services_uuid16 = 0xae00,
        .characteristic_uuid16 = 0xae02,
        .opt_type = ATT_PROPERTY_NOTIFY,
    },
#endif

    /* { */
    /* .services_uuid16 = 0x1812, */
    /* .characteristic_uuid16 = 0x2a33, */
    /* .opt_type = ATT_PROPERTY_NOTIFY, */
    /* }, */

    /* { */
    /* .services_uuid16 = 0x1801, */
    /* .characteristic_uuid16 = 0x2a05, */
    /* .opt_type = ATT_PROPERTY_INDICATE, */
    /* }, */

};

/*设备match id 对应搜索表 dg_match_device_table的 顺序位置*/
#define NAME1_DEV_ID                 (SUPPORT_MAX_GATT_CLIENT + 0)
#define NAME2_DEV_ID                 (SUPPORT_MAX_GATT_CLIENT + 1)
#define USER_CONFIG_DEV_ID           (SUPPORT_MAX_GATT_CLIENT + 2)

//------!!!!注意:如果是OTA升级的话,这里名字修改还需要去ota_dg_central.c修改dg_ana_remoter_name1/2
static const u8 dg_test_remoter_name1[] = "AC695X_1(BLE)";//键盘
static const u8 dg_test_remoter_name2[] = "AC897N_MX(BLE)";//鼠标
static const u8 user_config_tag_string[] = "abc123";
static const client_match_cfg_t dg_match_device_table[] = {
    {
        .create_conn_mode = BIT(CLI_CREAT_BY_NAME),
        .compare_data_len = sizeof(dg_test_remoter_name1) - 1, //去结束符
        .compare_data = dg_test_remoter_name1,
        .filter_pdu_bitmap = 0,
    },
    {
        .create_conn_mode = BIT(CLI_CREAT_BY_NAME),
        .compare_data_len = sizeof(dg_test_remoter_name2) - 1, //去结束符
        .compare_data = dg_test_remoter_name2,
        .filter_pdu_bitmap = 0,
    },
    {
        .create_conn_mode = BIT(CLI_CREAT_BY_TAG),
        .compare_data_len = sizeof(user_config_tag_string) - 1, //去结束符
        .compare_data = user_config_tag_string,
        .filter_pdu_bitmap = 0,
    },
};


static const gatt_search_cfg_t dg_central_search_config = {
    .match_devices = &dg_match_device_table,
    .match_devices_count = (sizeof(dg_match_device_table) / sizeof(client_match_cfg_t)),
    .match_rssi_enable = 0,
    .match_rssi_value = MATCH_DEVICE_RSSI_LEVEL,

    .search_uuid_count = (sizeof(dongle_search_ble_uuid_table) / sizeof(target_uuid_t)),
    .search_uuid_group = dongle_search_ble_uuid_table,
    .auto_enable_ccc = 1,

};

//配置扫描匹配连接绑定后的设备
static gatt_search_cfg_t dg_client_bond_config = {
    .match_devices = NULL,
    .match_devices_count = 0,
    .match_rssi_enable = 0,

    .search_uuid_group = dongle_search_ble_uuid_table,
    .search_uuid_count = (sizeof(dongle_search_ble_uuid_table) / sizeof(target_uuid_t)),
    .auto_enable_ccc = 1,
};

//-------------------------------------------------------------------------------------
//带绑定的设备搜索
static client_match_cfg_t *dg_bond_device_table;/*配对表(匹配地址) + 默认匹配表dg_match_device_table*/
static u16  bond_device_table_cnt;
/*更新连接设备的匹配配置*/
static void dg_client_reflash_bond_search_config(void)
{
    int i;

    if (!dg_bond_device_table) {
        log_info("device tabl is null!");
        return;
    }

    /*建立对应关系 配对表(匹配地址) + 默认匹配表 dg_match_device_table*/
    for (i = 0; i < SUPPORT_MAX_GATT_CLIENT; i++) {
        dg_bond_device_table[i].create_conn_mode = BIT(CLI_CREAT_BY_ADDRESS);
        dg_bond_device_table[i].compare_data_len = 6;
        dg_bond_device_table[i].compare_data = &record_bond_info[i].peer_address_info[1];

        if (PAIR_BOND_TAG == record_bond_info[i].head_tag) {
            r_printf("set bond search: %d\n", i);
            dg_bond_device_table[i].filter_pdu_bitmap = BIT(EVENT_ADV_SCAN_IND) | BIT(EVENT_ADV_NONCONN_IND);
        } else {
#if RCSP_BTMATE_EN
            dg_bond_device_table[i].filter_pdu_bitmap = 0;
#else
            dg_bond_device_table[i].filter_pdu_bitmap = EVENT_DEFAULT_REPORT_BITMAP;
#endif
        }
    }
    memcpy(&dg_bond_device_table[SUPPORT_MAX_GATT_CLIENT], dg_match_device_table, sizeof(dg_match_device_table));

    dg_client_bond_config.match_devices = dg_bond_device_table;
    dg_client_bond_config.match_devices_count = bond_device_table_cnt;
    ble_gatt_client_set_search_config(&dg_client_bond_config);
}

//input priv
static void dg_timer_handle(u32 priv)
{
    putchar('%');
    static u8 connected_tag = 0;//上电连接配对过标识

    if (bt_connected) {
        if (bt_connected == 2) {
            connected_tag = 1;
        }
        return;
    }

    if (priv == 0) {
        ///init
        if (record_bond_info[0].head_tag == PAIR_BOND_TAG && \
            0 == ble_gatt_client_create_connection_request(&record_bond_info[0].peer_address_info[1], record_bond_info[0].peer_address_info[0], 0)) {
            log_info("pair is exist");
            log_info("reconnect start0");
        } else {
            log_info("pair new start0");
            ble_gatt_client_scan_enable(1);
        }
    } else {

        if (connected_tag) {
            //上电连接配对过，就不执行搜索配对;默认创建连接
            putchar('^');
#if !RCSP_BTMATE_EN
            if (bt_connected == 0 && ble_gatt_client_get_work_state() != BLE_ST_CREATE_CONN) {
                dg_scan_conn_config_set(&record_bond_info[0]);/*record's config*/
                if (ble_gatt_client_create_connection_request(&record_bond_info[0].peer_address_info[1], record_bond_info[0].peer_address_info[0], 0)) {
                    log_info("recreate_conn fail!!!");
                }
            }
#endif
            return;
        }
        switch (ble_gatt_client_get_work_state()) {
        case BLE_ST_CREATE_CONN:
            ble_gatt_client_create_connection_cannel();
            dg_scan_conn_config_set(NULL);/*default config*/
            ble_gatt_client_scan_enable(1);
            log_info("pair new start1");
            break;

        case BLE_ST_SCAN:
            if (record_bond_info[0].head_tag == PAIR_BOND_TAG) {
                ble_gatt_client_scan_enable(0);
                dg_scan_conn_config_set(&record_bond_info[0]);/*record's config*/
                if (0 == ble_gatt_client_create_connection_request(&record_bond_info[0].peer_address_info[1], record_bond_info[0].peer_address_info[0], 0)) {
                    log_info("reconnect start1");
                } else {
                    log_info("keep pair new start1");
                    dg_scan_conn_config_set(NULL);/*default config*/
                    ble_gatt_client_scan_enable(1);
                }
            }
            break;

        case BLE_ST_INIT_OK:
        case BLE_ST_IDLE:
        case BLE_ST_DISCONN:
            ble_gatt_client_scan_enable(1);
            log_info("pair new start000");
            break;

        default:
            break;
        }
    }
}

/*预设知道连接hid设备 nofify发送的handle,通过handle分发数据上报到usb端*/
static const u16 mouse_ccc_value = 0x0001;
static const u16 ota_notify_handle[2] = {0x0084, 0x0082};//---后面这些ota handle看怎么优化
static const u16 mouse_notify_handle[3] = {0x0027, 0x002b, 0x002f};
static const u16 standard_keyboard_notify_handle[2] = {0x0036, 0x003a};

static void dg_enable_notify_ccc(u16 conn_handle)
{
    log_info("%s\n", __FUNCTION__);
    /* ble_comm_att_send_data(conn_handle, mouse_notify_handle[0] + 1, &mouse_ccc_value, 2, ATT_OP_WRITE); */
    /* ble_comm_att_send_data(conn_handle, mouse_notify_handle[1] + 1, &mouse_ccc_value, 2, ATT_OP_WRITE); */
    /* ble_comm_att_send_data(conn_handle, mouse_notify_handle[2] + 1, &mouse_ccc_value, 2, ATT_OP_WRITE); */
    /* ble_comm_att_send_data(conn_handle, standard_keyboard_notify_handle[0] + 1, &mouse_ccc_value, 2, ATT_OP_WRITE); */
    /* ble_comm_att_send_data(conn_handle, standard_keyboard_notify_handle[1] + 1, &mouse_ccc_value, 2, ATT_OP_WRITE); */
#if RCSP_BTMATE_EN
    ble_comm_att_send_data(conn_handle, ota_notify_handle[0] + 1, &mouse_ccc_value, 2, ATT_OP_WRITE);
#endif
}


static struct ctl_pair_info_t *dg_get_pair_info(u16 conn_handle)
{
    if (!conn_handle) {
        return NULL;
    }

    for (int i = 0; i < SUPPORT_MAX_GATT_CLIENT; i++) {
        if (record_bond_info[i].conn_handle == conn_handle) {
            return &record_bond_info[i];
        }
    }
    return NULL;
}

static int dg_pair_vm_do(struct ctl_pair_info_t *info, u8 rw_flag)
{
    int ret = 0;

#if CLIENT_PAIR_BOND_ENABLE
    int i;
    int uint_len = sizeof(struct ctl_pair_info_t);

    log_info("-conn_pair_info vm_do:%d\n", rw_flag);

    if (rw_flag == 0) {
        ret = syscfg_read(CFG_BLE_BONDING_REMOTE_INFO2, (u8 *)&record_bond_info, uint_len * SUPPORT_MAX_GATT_CLIENT);
        if (!ret) {
            log_info("-null--\n");
            memset((u8 *)&record_bond_info, 0xff, uint_len * SUPPORT_MAX_GATT_CLIENT);
            ret = 1;
        }

        /*检查合法性*/
        for (i = 0; i < SUPPORT_MAX_GATT_CLIENT; i++) {
            if (PAIR_BOND_TAG != record_bond_info[i].head_tag || record_bond_info[i].pair_flag != 1) {
                memset((u8 *)&record_bond_info[i], 0xff, uint_len);
            }
        }

    } else {
        int fill_index = -1;

        if (info == NULL) {
            log_info("vm clear\n");
            memset((u8 *)&record_bond_info, 0xff, uint_len * SUPPORT_MAX_GATT_CLIENT);
        } else {

            for (i = 0; i < SUPPORT_MAX_GATT_CLIENT; i++) {
                if (0 == memcmp(info, &record_bond_info[i], uint_len)) {
                    log_info("dev in table\n");
                    return ret;
                }
            }

            put_buf((u8 *)&record_bond_info, uint_len * SUPPORT_MAX_GATT_CLIENT);
            put_buf((u8 *)info, uint_len);
            log_info("write vm start\n");
            log_info("find table\n");

            /*find same*/
            if (fill_index == -1) {
                for (i = 0; i < SUPPORT_MAX_GATT_CLIENT; i++) {
                    if (0 == memcmp(info->peer_address_info, &record_bond_info[i].peer_address_info, 7)) {
                        log_info("replace old,match_dev_id: %d,%d\n", info->match_dev_id, record_bond_info[i].match_dev_id);
                        fill_index = i;
                        if (info->match_dev_id < SUPPORT_MAX_GATT_CLIENT) {
                            /*地址回连方式,获取原来的search id*/
                            info->match_dev_id = record_bond_info[i].match_dev_id;
                            /*地址回连方式,获取原来的write_handle*/
                            info->write_handle = record_bond_info[i].write_handle;
                        } else if ((info->match_dev_id >= SUPPORT_MAX_GATT_CLIENT) && (info->match_dev_id != record_bond_info[i].match_dev_id)) {
                            /*遇到连接多设备match_dev_id为别的通道的值时,不更改*/
                            info->match_dev_id = record_bond_info[i].match_dev_id;
                        }
                        break;
                    }
                }
            }

            /*find idle*/
            if (fill_index == -1) {
                for (i = 0; i < SUPPORT_MAX_GATT_CLIENT; i++) {
                    if (PAIR_BOND_TAG != record_bond_info[i].head_tag) {
                        log_info("find idle\n");
                        fill_index = i;
                        break;
                    }
                }
            }

            /*find first*/
            if (fill_index == -1) {
                for (i = 1; i < SUPPORT_MAX_GATT_CLIENT; i++) {
                    memcpy(&record_bond_info[i - 1], &record_bond_info[i], uint_len);
                }
                log_info("replace first\n");
                fill_index = SUPPORT_MAX_GATT_CLIENT - 1;
            }

            /*连接顺序不同，handle是不一样,防止重复相同*/
            for (i = 0; i < SUPPORT_MAX_GATT_CLIENT; i++) {
                if (info->conn_handle == record_bond_info[i].conn_handle) {
                    record_bond_info[i].conn_handle = 0;
                    log_info("clear repeat handle %d\n", info->conn_handle);
                }
            }

            memcpy(&record_bond_info[fill_index], info, uint_len);
            log_info("new record,match_dev_id= %d\n", info->match_dev_id);

        }

        syscfg_write(CFG_BLE_BONDING_REMOTE_INFO2, (u8 *)&record_bond_info, uint_len * SUPPORT_MAX_GATT_CLIENT);
    }

    put_buf((u8 *)&record_bond_info, uint_len * SUPPORT_MAX_GATT_CLIENT);
    dg_client_reflash_bond_search_config();/*配对表发生变化，更新scan设备匹配*/
#endif
    return ret;
}

//清配对信息
int dg_central_clear_pair(void)
{
#if CLIENT_PAIR_BOND_ENABLE
    ble_gatt_client_disconnect_all();
    memset(&cur_conn_info, 0, sizeof(cur_conn_info));
    dg_pair_vm_do(NULL, 1);
    if (BLE_ST_SCAN == ble_gatt_client_get_work_state()) {
        ble_gatt_client_scan_enable(0);
        ble_gatt_client_scan_enable(1);
    }
#endif
    return 0;
}

int ble_hid_data_send_ext(u8 report_type, u8 report_id, u8 *data, u16 len)
{
    if (dg_conn_handle && dg_ble_central_write_handle) {
        return ble_comm_att_send_data(dg_conn_handle, dg_ble_central_write_handle, data, len, ATT_OP_WRITE);
    }
    return APP_BLE_OPERATION_ERROR;
}

int ble_dongle_send_data(u16 con_handle, u8 *data, u16 len)
{
    u8 i, j;
    /* put_buf(data, len); */
    /* log_info("ble_comm_dev_get_handle: %d", ble_comm_dev_get_handle(0, GATT_ROLE_CLIENT)); */
    for (i = 0; i < SUPPORT_MAX_GATT_CLIENT; i++) {
        if (ble_comm_dev_get_handle(i, GATT_ROLE_CLIENT) == con_handle) {
            break;
        }

        if (i == SUPPORT_MAX_GATT_CLIENT - 1) {
            log_info("No this handle to send!!");
            return 0;
        }
    }

    if (con_handle) {
        if (record_bond_info[i].conn_handle == con_handle) {
            log_info("Send usb data to trans: %x, %x", con_handle, record_bond_info[i].write_handle);
        } else {
            for (j = 0; j < SUPPORT_MAX_GATT_CLIENT; j++) {
                if (record_bond_info[j].conn_handle == con_handle) {
                    i = j;
                }
            }
            log_info("Send usb data to trans: %x, %x", con_handle, record_bond_info[i].write_handle);
        }
        return ble_comm_att_send_data(con_handle, record_bond_info[i].write_handle, data, len, ATT_OP_WRITE_WITHOUT_RESPOND);
    }
    return APP_BLE_OPERATION_ERROR;
}

u8 dg_central_get_match_id(u16 conn_handle)
{
    struct ctl_pair_info_t *dg_handle_info =  dg_get_pair_info(conn_handle);
    if (dg_handle_info) {
        return dg_handle_info->match_dev_id;
    }
    return 0;
}

u8 dg_central_get_ota_is_support(u16 conn_handle)
{
    log_info("ota_is_support :%d", ota_is_support);
    return ota_is_support;
}

u8 is_succ_connection(void)
{
    log_info("is_support :%d", bt_connected);
    return bt_connected;
}

u8 err_address[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
u8 *dg_central_get_conn_address(u16 conn_handle)
{
    struct ctl_pair_info_t *dg_handle_info =  dg_get_pair_info(conn_handle);

    /* if (conn_handle == cur_conn_info.conn_handle) { //避免连接上过早的拿地址导致地址出错 */
    /*     log_info("peer address is now:"); */
    /*     return cur_conn_info.peer_address_info; */
    /* } else { */
    if (dg_handle_info) {
        log_info("peer address:");
        /* put_buf(dg_handle_info->peer_address_info, 7); */
        return dg_handle_info->peer_address_info;
    } else {
        log_info("err address:");
        return err_address;
    }

}

/*断开后,清PC按键;根据设备描述符的长度,清0*/
static void dg_central_disable_clear_key(u16 conn_handle)
{
    u8 match_id = dg_central_get_match_id(conn_handle);
    u8 tmp_buf[16];
    memset(tmp_buf, 0, 16);
    if (match_id == NAME1_DEV_ID) {
        log_info("clear dev_id1 key\n");
        tmp_buf[0] = 1;
        dongle_ble_hid_input_handler(tmp_buf, 10);
        tmp_buf[0] = 2;
        dongle_ble_hid_input_handler(tmp_buf, 10);

    } else {
        log_info("clear dev_id2 key\n");
        tmp_buf[0] = 4;
        dongle_second_ble_hid_input_handler(tmp_buf, 9);
        tmp_buf[0] = 5;
        dongle_second_ble_hid_input_handler(tmp_buf, 9);
    }
}

#if RCSP_BTMATE_EN
#define RCSP_RX_HANDLER_TAIL_TAG         0xEF
#define RCSP_RX_HANDLER_HEAD_TAG         0xFEDCBA
#define RCSP_RX_DEVICE_AUTH_TAG1         0x00
#define RCSP_RX_DEVICE_AUTH_TAG2         0x01
#define RCSP_RX_DEVICE_AUTH_TAG3         0x02
static u8 buf_total[128];//RCSP透传接收buffer
#endif

static u8 wait_usb_wakeup = 1;//0:发送空包  1:正常发送数据
static u8 usb_send_packet[MAXP_SIZE_HIDIN];/*usb发送缓存*/

static int dg_central_event_packet_handler(int event, u8 *packet, u16 size, u8 *ext_param)
{
    att_data_report_t *report_data = (void *)packet;
    static u16 mtu_size = 0;

    /* log_info("event: %02x,size= %d\n",event,size); */
    switch (event) {
    case GATT_COMM_EVENT_GATT_DATA_REPORT: {
        /* log_info("dg_central_event_packet_handler: %x", report_data->value_handle); */
        /* log_info("data_report:hdl=%04x,pk_type=%02x,size=%d,val_hdl=%04x\n", \ */
        /* report_data->conn_handle, report_data->packet_type, report_data->blob_length, report_data->value_handle); */
        /* put_buf(report_data->blob, report_data->blob_length); */

        /*u8 match_id = dg_central_get_match_id(report_data->conn_handle);*/
        /* log_info("data_match_id= %02x\n",match_id); */
        /* printf("{%d}", match_id); */

        switch (report_data->packet_type) {
        case GATT_EVENT_NOTIFICATION: { //notify
            if (report_data->value_handle != ota_notify_handle[0]) {
                /*预设知道连接hid设备 nofify发送的handle,通过handle分发数据上报到usb端*/
                //u8 packet[16];
                if (report_data->blob_length >= sizeof(usb_send_packet)) {
                    log_info("err:usb ep over");
                    return 0;
                }
                u8 second_devices = 0;
                if (report_data->value_handle == mouse_notify_handle[0]) {
                    usb_send_packet[0] = 1; //report_id
                } else if (report_data->value_handle == mouse_notify_handle[1]) {
                    usb_send_packet[0] = 2;//report_id
                } else if (report_data->value_handle == standard_keyboard_notify_handle[0]) {
                    usb_send_packet[0] = 4;//report_id
                    second_devices = 1;
                } else if (report_data->value_handle == standard_keyboard_notify_handle[1]) {
                    usb_send_packet[0] = 5;//report_id
                    second_devices = 1;
                } else {
                    log_info("err notify");
                    break;
                    /* usb_send_packet[0] = 1;//report_id */
                }

                u8 usb_status_ret = usb_get_suspend_resume_status(0);
                if (usb_status_ret == USB_READY) {
                    wdt_clear();
                    if (wait_usb_wakeup) {
                        memcpy(&usb_send_packet[1], report_data->blob, report_data->blob_length);
                    } else {
                        printf("clear length: %d", report_data->blob_length);
                        memset(&usb_send_packet[1], 0, report_data->blob_length); //发空包
                    }
                } else if (usb_status_ret == USB_SUSPEND) {
                    wait_usb_wakeup = 0;
                    log_info("send remote_wakeup\n");
                    usb_remote_wakeup(0);
                    break;
                } else if (usb_status_ret == USB_RESUME_WAIT) {
                    putchar('W');
                    break;
                } else if (usb_status_ret == USB_RESUME_OK) {
                    putchar('M');
                }

                if (report_data->value_handle == mouse_notify_handle[0]) {
                    usb_send_packet[0] = 1; //report_id
                } else if (report_data->value_handle == mouse_notify_handle[1]) {
                    usb_send_packet[0] = 2;//report_id
                } else {
                    usb_send_packet[0] = 1;//report_id
                }

                int (*dongle_input_handler)(u8 * packet, u16 size);
                if (second_devices == 0) {
                    /* dongle_ble_hid_input_handler(usb_send_packet, report_data->blob_length + 1); */
                    /* putchar('&'); */
                    dongle_input_handler = dongle_ble_hid_input_handler;
                } else {
                    /* dongle_second_ble_hid_input_handler(usb_send_packet, report_data->blob_length + 1); */
                    /* putchar('&'); */
                    dongle_input_handler = dongle_second_ble_hid_input_handler;
                }

                /* put_buf(usb_send_packet, report_data->blob_length + 1); */
                int ret = dongle_input_handler(usb_send_packet, report_data->blob_length + 1);
                if (ret && wait_usb_wakeup == 0) {
                    wait_usb_wakeup = 1;
                    log_info("send 0packet success!\n");
                }
            } else {
#if RCSP_BTMATE_EN
                if (!ota_is_support) {
                    log_info("This device support OTA updata!!!");
                    ota_is_support = 1;
                }

                /* gatt_client_get_mtu(report_data->conn_handle, &mtu_size); */
                /* mtu_size -= 3; */
                /* log_info("mut_size is %d", mtu_size); */

                if (0xff == report_data->blob[0]) {
                    putchar('E');//dorp
                    return 0;
                }

                /* server_receive = 0; */
                put_buf(report_data->blob, report_data->blob_length);
                dongle_send_data_to_pc_2(report_data->conn_handle, report_data->blob, report_data->blob_length);
#endif
            }
        }
        break;

        case GATT_EVENT_INDICATION://indicate
            break;
        case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT://read
            break;
        case GATT_EVENT_LONG_CHARACTERISTIC_VALUE_QUERY_RESULT://read long
            break;
        default:
            break;
        }
    }
    break;

    case GATT_COMM_EVENT_CAN_SEND_NOW:
        break;

    case GATT_COMM_EVENT_CONNECTION_COMPLETE:
        log_info("connection_handle:%04x\n", little_endian_read_16(packet, 0));
        log_info("peer_address_info:");
        put_buf(&ext_param[7], 7);
        memcpy(cur_conn_info.peer_address_info, &ext_param[7], 7);
        cur_conn_info.conn_handle =   little_endian_read_16(packet, 0);
        cur_conn_info.conn_interval = little_endian_read_16(ext_param, 14 + 0);
        cur_conn_info.conn_latency =  little_endian_read_16(ext_param, 14 + 2);
        cur_conn_info.conn_timeout =  little_endian_read_16(ext_param, 14 + 4);
        cur_conn_info.pair_flag = 0;
        cur_conn_info.head_tag = 0;
        bt_connected = 1;
        cur_connect_pair_process = 0;

        if (!dg_conn_handle) {
            dg_conn_handle = little_endian_read_16(packet, 0);
        } else {
            dg_conn_handle2 = little_endian_read_16(packet, 0);
        }

#if RCSP_BTMATE_EN
        check_is_reconn_succ(1, cur_conn_info.conn_handle);//返回重连接成功,未放到encry处,避免不走加密情况
#endif
        break;

    case GATT_COMM_EVENT_DISCONNECT_COMPLETE: {
        log_info("disconnect_handle:%04x,reason= %02x\n", little_endian_read_16(packet, 0), packet[2]);
        if (little_endian_read_16(packet, 0) == dg_conn_handle) {
            dg_conn_handle = 0;
            bt_connected = 0;
        } else if (little_endian_read_16(packet, 0) == dg_conn_handle2) {
            dg_conn_handle2 = 0;
        }

        if (packet[2] == 0x08) {
            /*超时断开,清按键行为操作*/
            /* dg_central_disable_clear_key(little_endian_read_16(packet, 0)); */
        }
#if RCSP_BTMATE_EN

        if (!get_reonn_param()) {
            dongle_return_online_list();
        }
        if (reonn_channel_is_change()) {
            dongle_ota_init();
        }
        extern void clear_auth(u16 channel);
        extern void judge_is_reonn(u16 channel);
        clear_auth(little_endian_read_16(packet, 0));
        ble_gatt_client_scan_enable(0);//ota升级过程中,从机主动断开连接需要等上位机下发回连接命令
        //当回连接出现偶现0x3e同步报错问题,为避免直接卡死.重新打开scan
        if (packet[2] == 0x3e) {
            ble_gatt_client_scan_enable(1);
            break;
        }

        sys_timeout_add(little_endian_read_16(packet, 0), judge_is_reonn, 1000);//判断是否是ota回连接
#endif
    }
    break;

    case GATT_COMM_EVENT_ENCRYPTION_CHANGE:
        log_info("ENCRYPTION_CHANGE:handle=%04x,state=%d,process =%d", little_endian_read_16(packet, 0), packet[2], packet[3]);
        if (packet[3] == LINK_ENCRYPTION_RECONNECT) {
            log_info("reconnect...\n");
            cur_connect_pair_process = 2;
            dg_enable_notify_ccc(little_endian_read_16(packet, 0));
            if (!dg_pair_reconnect_search_profile) {
                //RCSP升级,每次连接均使用搜索协议升级
                /* #if RCSP_BTMATE_EN */
                /*                 dg_client_bond_config.search_uuid_count = (sizeof(dongle_search_ble_uuid_table) / sizeof(target_uuid_t));//set search profile */
                /* #else */
                dg_client_bond_config.search_uuid_count = 0;//set no search
                /* #endif */
            }
        } else {
            log_info("first pair...\n");
            cur_connect_pair_process = 1;
            dg_client_bond_config.search_uuid_count = (sizeof(dongle_search_ble_uuid_table) / sizeof(target_uuid_t));//set search profile
        }

        if (packet[2]) {
            log_info("error:encryption fail=%02x,disconnect,%04x\n", packet[2], little_endian_read_16(packet, 0));
#if RCSP_BTMATE_EN
            //ota升级失败之后,由于是不加密连接,会触发26的加密失败,做一个不断开的操作
            if (packet[2] == 26) {
                log_info("OTA no encryption conn!!");
            } else {
                ble_comm_disconnect(little_endian_read_16(packet, 0));
                break;
            }
#else
            ble_comm_disconnect(little_endian_read_16(packet, 0));
            break;
#endif
        }

#if CLIENT_PAIR_BOND_ENABLE
#if RCSP_BTMATE_EN
        ota_is_support = 0;
        for (u8 i = 0; i < SUPPORT_MAX_GATT_CLIENT; i++) {
            if (record_bond_info[i].write_handle != 0xffff) {
                log_info("open ota :%d", i);
                cur_conn_info.write_handle = record_bond_info[i].write_handle;
                ota_is_support = 1;
            }
        }
#endif
        cur_conn_info.head_tag = PAIR_BOND_TAG;
        cur_conn_info.pair_flag = 1;
        dg_pair_vm_do(&cur_conn_info, 1);
#endif
        break;

    case GATT_COMM_EVENT_CONNECTION_UPDATE_COMPLETE:
        log_info("conn_param update_complete:%04x\n", little_endian_read_16(packet, 0));
        log_info("update_interval = %d\n", little_endian_read_16(ext_param, 6 + 0));
        log_info("update_latency = %d\n",  little_endian_read_16(ext_param, 6 + 2));
        log_info("update_timeout = %d\n",  little_endian_read_16(ext_param, 6 + 4));

        if (cur_conn_info.conn_handle == little_endian_read_16(packet, 0)) {
            cur_conn_info.conn_interval = little_endian_read_16(ext_param, 6 + 0);
            cur_conn_info.conn_latency =  little_endian_read_16(ext_param, 6 + 2);
            cur_conn_info.conn_timeout =  little_endian_read_16(ext_param, 6 + 4);
        }

#if CLIENT_PAIR_BOND_ENABLE
        if (cur_conn_info.conn_handle == little_endian_read_16(packet, 0)) {
            if (cur_conn_info.pair_flag) {
                log_info("update_cur_conn\n");
                dg_pair_vm_do(&cur_conn_info, 1);
            }
        } else {
            struct ctl_pair_info_t tmp_conn_info;
            for (int i = 0; i < SUPPORT_MAX_GATT_CLIENT; i++) {
                if (record_bond_info[i].pair_flag && record_bond_info[i].conn_handle == little_endian_read_16(packet, 0)) {
                    log_info("update_record_conn\n");
                    memcpy(&tmp_conn_info, &record_bond_info[i], sizeof(struct ctl_pair_info_t));
                    tmp_conn_info.conn_interval = little_endian_read_16(ext_param, 6 + 0);
                    tmp_conn_info.conn_latency =  little_endian_read_16(ext_param, 6 + 2);
                    tmp_conn_info.conn_timeout =  little_endian_read_16(ext_param, 6 + 4);
                    dg_pair_vm_do(&tmp_conn_info, 1);
                    break;
                }
            }
        }
#endif

        break;


    case GATT_COMM_EVENT_SCAN_DEV_MATCH: {
        log_info("match_dev:addr_type= %d,rssi= %d\n", packet[0], packet[7]);
        put_buf(&packet[1], 6);
        if (packet[8] == 2) {
            log_info("is TEST_BOX\n");
            break;
        }
        client_match_cfg_t *match_cfg = ext_param;
        if (match_cfg) {
            log_info("match_mode: %d\n", match_cfg->create_conn_mode);
            if (match_cfg->compare_data_len) {
                put_buf(match_cfg->compare_data, match_cfg->compare_data_len);
            }
        }
        //update info
        cur_conn_info.conn_handle = 0;
        cur_conn_info.pair_flag = 0;
        cur_conn_info.match_dev_id = packet[9];
        dg_client_bond_config.search_uuid_count = (sizeof(dongle_search_ble_uuid_table) / sizeof(target_uuid_t));//set search profile

#if CLIENT_PAIR_BOND_ENABLE
        if (packet[9] < SUPPORT_MAX_GATT_CLIENT) {
            /*记录表地址回连，使用记录的连接参数建立*/
            r_printf("match bond,reconnect\n");
            dg_scan_conn_config_set(&record_bond_info[packet[9]]);
        } else {
            /*搜索匹配方式连接*/
            r_printf("match search_config\n");
            dg_scan_conn_config_set(NULL);
        }
#endif
        log_info("search match_dev_id: %d\n", cur_conn_info.match_dev_id);

    }
    break;

    case GATT_COMM_EVENT_CREAT_CONN_TIMEOUT:
        break;

    case GATT_COMM_EVENT_GATT_SEARCH_MATCH_UUID: {
        opt_handle_t *opt_hdl = packet;
        log_info("match:server_uuid= %04x,charactc_uuid= %04x,value_handle= %04x\n", \
                 opt_hdl->search_uuid->services_uuid16, opt_hdl->search_uuid->characteristic_uuid16, opt_hdl->value_handle);

#if RCSP_BTMATE_EN
        if (opt_hdl->value_handle == ota_notify_handle[1]) {
            log_info("This device support OTA updata!!");
            cur_conn_info.write_handle = opt_hdl->value_handle;

            if (get_reonn_param() != 1) {
                cur_conn_info.head_tag = PAIR_BOND_TAG;
                cur_conn_info.pair_flag = 1;
                dg_pair_vm_do(&cur_conn_info, 1);
            }
            ota_is_support = 1;
        } else {
            /* log_info("This device No support OTA updata!!!"); */
            /* ota_is_support = 0; */
        }
#endif
    }
    break;


    case GATT_COMM_EVENT_MTU_EXCHANGE_COMPLETE:
        log_info("con_handle= %02x, ATT MTU = %u\n", little_endian_read_16(packet, 0), little_endian_read_16(packet, 2));
        break;

    case GATT_COMM_EVENT_GATT_SEARCH_PROFILE_COMPLETE:
        log_info("GATT_COMM_EVENT_GATT_SEARCH_PROFILE_COMPLETE\n");
        bt_connected = 2;
#if RCSP_BTMATE_EN
        dongle_return_online_list();
        ble_gatt_client_scan_enable(1);//ota升级流程连接完成后继续开启scan
        if (get_reonn_param()) {
            //如果开了提速,ota回连接也申请del
            if (config_btctler_le_features & LE_DATA_PACKET_LENGTH_EXTENSION) {
                log_info(">>>>>>>>s2--request DLE, %04x\n", cur_conn_info.conn_handle);
                ble_comm_set_connection_data_length(cur_conn_info.conn_handle, config_btctler_le_acl_packet_length, 2120);
            }

        }
#endif

        break;

    default:
        break;
    }
    return 0;
}

//scan参数设置
static void dg_scan_conn_config_set(struct ctl_pair_info_t *pair_info)
{
#if RCSP_BTMATE_EN
    dg_central_scan_cfg.scan_auto_do = 0;
#else
    dg_central_scan_cfg.scan_auto_do = CONFIG_BT_GATT_CLIENT_NUM > 1 ? 1 : 0;
#endif
    dg_central_scan_cfg.creat_auto_do = 1;
    dg_central_scan_cfg.scan_type = SET_SCAN_TYPE;
    dg_central_scan_cfg.scan_filter = 1;
    dg_central_scan_cfg.scan_interval = SET_SCAN_INTERVAL;
    dg_central_scan_cfg.scan_window = SET_SCAN_WINDOW;

    if (pair_info) {
        log_info("pair_to_creat:%d,%d,%d\n", pair_info->conn_interval, pair_info->conn_latency, pair_info->conn_timeout);
        dg_central_scan_cfg.creat_conn_interval = pair_info->conn_interval;
        dg_central_scan_cfg.creat_conn_latency = pair_info->conn_latency;
        dg_central_scan_cfg.creat_conn_super_timeout = pair_info->conn_timeout;
    } else {
        dg_central_scan_cfg.creat_conn_interval = SET_CONN_INTERVAL;
        dg_central_scan_cfg.creat_conn_latency = SET_CONN_LATENCY;
        dg_central_scan_cfg.creat_conn_super_timeout = SET_CONN_TIMEOUT;
    }

    dg_central_scan_cfg.conn_update_accept = 1;
    dg_central_scan_cfg.creat_state_timeout_ms = SET_CREAT_CONN_TIMEOUT;

    ble_gatt_client_set_scan_config(&dg_central_scan_cfg);
}

static void dg_central_init(void)
{
    log_info("%s", __FUNCTION__);

    if (!dg_bond_device_table) {
        int table_size = sizeof(dg_match_device_table) + sizeof(client_match_cfg_t) * SUPPORT_MAX_GATT_CLIENT;//设置名字匹配
        bond_device_table_cnt = dg_central_search_config.match_devices_count + SUPPORT_MAX_GATT_CLIENT;
        dg_bond_device_table = malloc(table_size);
        ASSERT(dg_bond_device_table != NULL, "%s,malloc fail!", __func__);
        memset(dg_bond_device_table, 0, table_size);
    }

    if (0 == dg_pair_vm_do(NULL, 0)) {
        log_info("client already bond dev");
    }

    dg_scan_conn_config_set(NULL);
}

//-------------------------------------------------------------------------------------

void uuid_count_set(void)
{
    dg_client_bond_config.search_uuid_count = (sizeof(dongle_search_ble_uuid_table) / sizeof(target_uuid_t));//set search profile
}

void bt_ble_before_start_init(void)
{
    ble_comm_init(&dg_gatt_control_block);
}

void bt_ble_init(void)
{
    log_info("%s\n", __FUNCTION__);
    ble_comm_set_config_name(bt_get_local_name(), 1);
    dg_conn_handle = 0;
    bt_connected = 0;

    dg_central_init();

    ble_module_enable(1);

    /* if (record_bond_info[0].head_tag == PAIR_BOND_TAG && CONFIG_BT_GATT_CLIENT_NUM == 1) { */
    if (CONFIG_BT_GATT_CLIENT_NUM == 1) {
        log_info("connect + scan");
#if	POWER_ON_RECONNECT_START
        dg_timer_handle(0);
#endif
        dg_timer_id = sys_timer_add((void *)1, dg_timer_handle, POWER_ON_SWITCH_TIME);
    } else {
        ble_gatt_client_scan_enable(1);
        log_info("just scan");
    }
}

void bt_ble_exit(void)
{
    log_info("%s\n", __FUNCTION__);
    ble_module_enable(0);
    ble_comm_exit();

    if (dg_bond_device_table) {
        free(dg_bond_device_table);
        dg_bond_device_table = NULL;
    }
}

void ble_module_enable(u8 en)
{
    ble_comm_module_enable(en);
}

#endif


