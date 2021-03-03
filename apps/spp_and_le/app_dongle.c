/*********************************************************************************************
    *   Filename        : app_spp_and_le.c

    *   Description     :

    *   Author          : Bingquan

    *   Email           : caibingquan@zh-jieli.com

    *   Last modifiled  : 2019-07-22 14:01

    *   Copyright:(c)JIELI  2011-2019  @ , All Rights Reserved.
*********************************************************************************************/
#include "system/app_core.h"
#include "system/includes.h"
#include "server/server_core.h"
#include "app_config.h"
#include "app_action.h"
#include "os/os_api.h"
#include "btcontroller_config.h"
#include "btctrler/btctrler_task.h"
#include "config/config_transport.h"
#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "bt_common.h"
#include "rcsp_bluetooth.h"
#include "rcsp_user_update.h"
#include "app_charge.h"
#include "app_power_manage.h"
#include "le_client_demo.h"
#include "usb/device/hid.h"

#define LOG_TAG_CONST       DONGLE
#define LOG_TAG             "[DONGLE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if CONFIG_APP_DONGLE

//2.4G模式: 0---ble, 非0---2.4G配对码
#define CFG_RF_24G_CODE_ID       (0) //<=24bits
/* #define CFG_RF_24G_CODE_ID       (0x23) //<=24bits */

//dongle 上电开配对管理,若配对失败，没有配对设备，停止搜索
#define POWER_ON_PAIR_START      (1)//
#define POWER_ON_PAIR_TIME       (3500)//unit ms,切换搜索回连周期
#define DEVICE_RSSI_LEVEL        (-50)
#define POWER_ON_KEEP_SCAN       (0)//配对失败，保持一直搜索配对

//等待连接断开时间
#define WAIT_DISCONN_TIME_MS     (300)

void sys_auto_sniff_controle(u8 enable, u8 *addr);
void bt_wait_phone_connect_control_ext(u8 inquiry_en, u8 page_scan_en);

static u8 is_app_active = 0;
static u8 bt_connected = 0;
static u16  dongle_timer_id = 0;

static void dongle_timer_handle(u32 priv);

//---------------------------------------------------------------------
static const u8 sHIDReportDesc[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x95, 0x05,        //     Report Count (5)
    0x75, 0x01,        //     Report Size (1)
    0x05, 0x09,        //     Usage Page (Button)
    0x19, 0x01,        //     Usage Minimum (0x01)
    0x29, 0x05,        //     Usage Maximum (0x05)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //     Report Count (1)
    0x75, 0x03,        //     Report Size (3)
    0x81, 0x01,        //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x01,        //     Report Count (1)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x38,        //     Usage (Wheel)
    0x15, 0x81,        //     Logical Minimum (-127)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x0C,        //     Usage Page (Consumer)
    0x0A, 0x38, 0x02,  //     Usage (AC Pan)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0x85, 0x02,        //   Report ID (2)
    0x09, 0x01,        //   Usage (Consumer Control)
    0xA1, 0x00,        //   Collection (Physical)
    0x75, 0x0C,        //     Report Size (12)
    0x95, 0x02,        //     Report Count (2)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x16, 0x01, 0xF8,  //     Logical Minimum (-2047)
    0x26, 0xFF, 0x07,  //     Logical Maximum (2047)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0xC0,              // End Collection
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x03,        //   Report ID (3)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x01,        //   Report Count (1)
    0x09, 0xCD,        //   Usage (Play/Pause)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x0A, 0x83, 0x01,  //   Usage (AL Consumer Control Configuration)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xB5,        //   Usage (Scan Next Track)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xB6,        //   Usage (Scan Previous Track)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xEA,        //   Usage (Volume Decrement)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0xE9,        //   Usage (Volume Increment)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x0A, 0x25, 0x02,  //   Usage (AC Forward)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x0A, 0x24, 0x02,  //   Usage (AC Back)
    0x81, 0x06,        //   Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x05,        //   Usage (Headphone)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x02,        //   Report Count (2)
    0xB1, 0x02,        //   Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
// 149 bytes
};


//---------------------------------------------------------------------
//指定搜索uuid
static const target_uuid_t  dongle_search_ble_uuid_table[] = {
    /* { */
    /* .services_uuid16 = 0x1800, */
    /* .characteristic_uuid16 = 0x2a00, */
    /* .opt_type = ATT_PROPERTY_READ, */
    /* }, */

    /* { */
    /* .services_uuid16 = 0x1812, */
    /* .characteristic_uuid16 = 0x2a4b, */
    /* .opt_type = ATT_PROPERTY_READ, */
    /* }, */

    {
        .services_uuid16 = 0x1812,
        .characteristic_uuid16 = 0x2a4d,
        .opt_type = ATT_PROPERTY_NOTIFY,
    },

    {
        .services_uuid16 = 0x1812,
        .characteristic_uuid16 = 0x2a33,
        .opt_type = ATT_PROPERTY_NOTIFY,
    },

    {
        .services_uuid16 = 0x1801,
        .characteristic_uuid16 = 0x2a05,
        .opt_type = ATT_PROPERTY_INDICATE,
    },

};

static const u16 mouse_notify_handle[3] = {0x0027, 0x002b, 0x002f};
/* static u8 get_report_map[256]; */
/* static u8 get_report_map_size = 0; */
static void ble_report_data_deal(att_data_report_t *report_data, target_uuid_t *search_uuid)
{
    /* log_info("report_data:type %02x,handle %04x,offset %d,len %d", report_data->packet_type, */
    /* report_data->value_handle, report_data->value_offset, report_data->blob_length); */

    /* log_info_hexdump(report_data->blob, report_data->blob_length); */

    /* if (search_uuid == NULL) { */
    /* log_info("not_match handle"); */
    /* return; */
    /* } */

    switch (report_data->packet_type) {
    case GATT_EVENT_NOTIFICATION: { //notify
        u8 packet[4];
        if (report_data->value_handle == mouse_notify_handle[0]) {
            packet[0] = 1;
        } else if (report_data->value_handle == mouse_notify_handle[1]) {
            packet[0] = 2;
        }
        memcpy(&packet[1], report_data->blob, report_data->blob_length);
        hid_send_data(packet, sizeof(packet));
        putchar('&');
    }
    break;

    case GATT_EVENT_INDICATION://indicate
    case GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT://read
        break;

    case GATT_EVENT_LONG_CHARACTERISTIC_VALUE_QUERY_RESULT://read long
        /* if (search_uuid->characteristic_uuid16 == 0x2a4b) { */
        /* log_info("report_map"); */
        /* if (0 == report_data->value_offset) { */
        /* get_report_map_size = report_data->blob_length; */
        /* } else { */
        /* get_report_map_size += report_data->blob_length; */
        /* } */
        /* memcpy(&get_report_map[report_data->value_offset], report_data->blob, report_data->blob_length); */
        /* } */
        break;

    default:
        break;
    }
}

static struct ble_client_operation_t *ble_client_api;
static const u8 dongle_remoter_name1[] = "JL_MOUSE(BLE)";//

//匹配配置的名字
static const client_match_cfg_t match_dev01 = {
    .create_conn_mode = BIT(CLI_CREAT_BY_NAME),
    .compare_data_len = sizeof(dongle_remoter_name1) - 1, //去结束符
    .compare_data = dongle_remoter_name1,
    .bonding_flag = 1,//
};

//匹配配置的名字
static client_match_cfg_t match_config_name = {
    .create_conn_mode = BIT(CLI_CREAT_BY_NAME),
    .compare_data_len = 0, //去结束符
    .compare_data = 0,
    .bonding_flag = 1,//绑定
};

//匹配配置的厂家信息
static const u8 user_config_tag_string[] = "abc123";
static const client_match_cfg_t match_user_tag = {
    .create_conn_mode = BIT(CLI_CREAT_BY_TAG),
    .compare_data_len = sizeof(user_config_tag_string) - 1, //去结束符
    .compare_data = user_config_tag_string,
    .bonding_flag = 1,//绑定
};

static const u16 mouse_ccc_value = 0x0001;
static void dongle_enable_notify_ccc(void)
{
    ble_client_api->opt_comm_send(mouse_notify_handle[0] + 1, &mouse_ccc_value, 2, ATT_OP_WRITE);
    ble_client_api->opt_comm_send(mouse_notify_handle[1] + 1, &mouse_ccc_value, 2, ATT_OP_WRITE);
    ble_client_api->opt_comm_send(mouse_notify_handle[2] + 1, &mouse_ccc_value, 2, ATT_OP_WRITE);
}

static void dongle_event_callback(le_client_event_e event, u8 *packet, int size)
{
    switch (event) {
    case CLI_EVENT_MATCH_DEV:
        client_match_cfg_t *match_dev = packet;
        log_info("match_name:%s\n", match_dev->compare_data);
        break;

    case CLI_EVENT_SEARCH_PROFILE_COMPLETE:
        log_info("search profile commplete");
        dongle_enable_notify_ccc();
        bt_connected = 2;
        break;

    case CLI_EVENT_CONNECTED:
        log_info("bt connected");
        bt_connected = 1;
        break;

    case CLI_EVENT_DISCONNECT:
        bt_connected = 0;
        log_info("bt disconnec");
        break;

    default:
        break;
    }
}

//配置 client 信息
static const client_conn_cfg_t dongle_conn_config = {
    .match_dev_cfg[0] = &match_dev01,      //匹配指定的名字
    /* .match_dev_cfg[1] = &match_config_name,//匹配配置的名字 */
    /* .match_dev_cfg[2] = &match_user_tag, */
    .match_dev_cfg[1] = NULL,
    .match_dev_cfg[2] = NULL,
    .report_data_callback = ble_report_data_deal,
    .search_uuid_cnt = 0, //配置不搜索profile，加快回连速度
    /* .search_uuid_cnt = (sizeof(dongle_search_ble_uuid_table) / sizeof(target_uuid_t)), */
    /* .search_uuid_table = dongle_search_ble_uuid_table, */
    .security_en = 1, //加密配对
    .event_callback = dongle_event_callback,
};

void power_on_pair_timeout(void *priv)
{
#if TCFG_USER_BLE_ENABLE && POWER_ON_PAIR_START
    //关强制搜索
    ble_client_api->scan_enable(0, 0);
    ble_client_api->set_force_search(0, 0);

    if (!bt_connected) {
        log_info("pair-new-timeout");
        //开scan，若有pair，直接创建连接监听
        if (ble_client_api->create_connect(0, 0, 1)) {
#if POWER_ON_KEEP_SCAN
            //pair fail,open scan
            ble_client_api->scan_enable(0, 1);
#endif
        }

    } else {
        log_info("pair-new-sucess");
    }
#endif
}

//input priv
static void dongle_timer_handle(u32 priv)
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
        if (0 == ble_client_api->create_connect(0, 0, 1)) {
            log_info("pair is exist");
            log_info("reconnect start0");
        } else {
            ble_client_api->set_force_search(1, DEVICE_RSSI_LEVEL);
            ble_client_api->scan_enable(0, 1);
            log_info("pair new start0");
        }
    } else {

        if (connected_tag) {
            //上电连接配对过，就不执行搜索配对;默认创建连接
            return;
        }

        switch (ble_client_api->get_work_state()) {
        case BLE_ST_CREATE_CONN:
            ble_client_api->create_connect_cannel();
            ble_client_api->set_force_search(1, DEVICE_RSSI_LEVEL);
            ble_client_api->scan_enable(0, 1);
            log_info("pair new start1");
            break;

        case BLE_ST_SCAN:
            ble_client_api->scan_enable(0, 0);
            ble_client_api->set_force_search(0, 0);
            if (0 == ble_client_api->create_connect(0, 0, 1)) {
                log_info("reconnect start1");
            } else {
                ble_client_api->set_force_search(1, DEVICE_RSSI_LEVEL);
                ble_client_api->scan_enable(0, 1);
                log_info("keep pair new start1");
            }
            break;

        default:
            break;
        }
    }
}


static void dongle_ble_config_init(void)
{
    u8 *cfg_name_p = bt_get_local_name();
    u8 cfg_name_len = strlen(cfg_name_p);

    match_config_name.compare_data_len = cfg_name_len; //
    match_config_name.compare_data = cfg_name_p;

    ble_client_api = ble_get_client_operation_table();
    ble_client_api->init_config(0, &dongle_conn_config);
    /* client_clear_bonding_info();//for test */
}

//---------------------------------------------------------------------


static void bt_function_select_init()
{
    __set_user_ctrl_conn_num(TCFG_BD_NUM);
    __set_support_msbc_flag(1);
#if BT_SUPPORT_DISPLAY_BAT
    __bt_set_update_battery_time(60);
#else
    __bt_set_update_battery_time(0);
#endif
    __set_page_timeout_value(8000); /*回连搜索时间长度设置,可使用该函数注册使用，ms单位,u16*/
    __set_super_timeout_value(8000); /*回连时超时参数设置。ms单位。做主机有效*/
#if (TCFG_BD_NUM == 2)
    __set_auto_conn_device_num(2);
#endif
#if BT_SUPPORT_MUSIC_VOL_SYNC
    vol_sys_tab_init();
#endif

    //io_capabilities ; /*0: Display only 1: Display YesNo 2: KeyboardOnly 3: NoInputNoOutput*/
    //authentication_requirements: 0:not protect  1 :protect
    __set_simple_pair_param(3, 0, 2);

#if TCFG_USER_BLE_ENABLE
    {
        u8 tmp_ble_addr[6];
        /* le_controller_set_mac((void*)"012345"); */
        lib_make_ble_address(tmp_ble_addr, (void *)bt_get_mac_addr());
        le_controller_set_mac((void *)tmp_ble_addr);

        printf("\n-----edr + ble 's address-----");
        printf_buf((void *)bt_get_mac_addr(), 6);
        printf_buf((void *)tmp_ble_addr, 6);
        dongle_ble_config_init();
    }
#endif
}


static void bredr_handle_register()
{

#if (USER_SUPPORT_PROFILE_SPP==1)
    spp_data_deal_handle_register(user_spp_data_handler);
#endif

    /* bt_fast_test_handle_register(bt_fast_test_api);//测试盒快速测试接口 */
#if BT_SUPPORT_MUSIC_VOL_SYNC
    music_vol_change_handle_register(set_music_device_volume, phone_get_device_vol);
#endif
#if BT_SUPPORT_DISPLAY_BAT
    get_battery_value_register(bt_get_battery_value);   /*电量显示获取电量的接口*/
#endif

    /* bt_dut_test_handle_register(bt_dut_api); */
}

void app_set_soft_poweroff(void)
{
    log_info("set_soft_poweroff\n");
    is_app_active = 1;
    //必须先主动断开蓝牙链路,否则要等链路超时断开

#if TCFG_USER_BLE_ENABLE
    ble_module_enable(0);
#endif

    //延时300ms，确保BT退出链路断开
    sys_timeout_add(NULL, power_set_soft_poweroff, WAIT_DISCONN_TIME_MS);
}

static void app_start()
{
    log_info("=======================================");
    log_info("-------------dongle demo---------------");
    log_info("=======================================");

    clk_set("sys", BT_NORMAL_HZ);
    u32 sys_clk =  clk_get("sys");
    bt_pll_para(TCFG_CLOCK_OSC_HZ, sys_clk, 0, 0);

    bt_function_select_init();
    bredr_handle_register();
    __change_hci_class_type(0);//default icon
    btstack_init();

#if TCFG_USER_EDR_ENABLE
    sys_auto_sniff_controle(1, NULL);
#endif /* SIG_MESH_EN */

    /* 按键消息使能 */
    sys_key_event_enable();

#if (TCFG_PC_ENABLE)
    void usb_start();
    void usb_hid_set_repport_map(const u8 * map, int size);
    usb_hid_set_repport_map(sHIDReportDesc, sizeof(sHIDReportDesc));
    usb_start();
#endif

    /* sys_auto_shut_down_enable(); */
    /* sys_auto_sniff_controle(1, NULL); */
    /* app_timer_handle  = sys_timer_add(NULL, app_timer_handler, 500); */
}

static int state_machine(struct application *app, enum app_state state, struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        if (!it) {
            break;
        }
        switch (it->action) {
        case ACTION_DONGLE_MAIN:
            app_start();
            break;
        }
        break;
    case APP_STA_PAUSE:
        break;
    case APP_STA_RESUME:
        break;
    case APP_STA_STOP:
        break;
    case APP_STA_DESTROY:
        log_info("APP_STA_DESTROY\n");
        break;
    }

    return 0;
}

#define SNIFF_CNT_TIME                5/////<空闲5S之后进入sniff模式

#define SNIFF_MAX_INTERVALSLOT        800
#define SNIFF_MIN_INTERVALSLOT        100
#define SNIFF_ATTEMPT_SLOT            4
#define SNIFF_TIMEOUT_SLOT            1

u8 sniff_ready_status = 0; //0:sniff_ready 1:sniff_not_ready
int exit_sniff_timer = 0;
int sniff_timer = 0;


void bt_check_exit_sniff()
{
    if (exit_sniff_timer) {
        sys_timeout_del(exit_sniff_timer);
        exit_sniff_timer = 0;
    }
    user_send_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
}

void bt_sniff_ready_clean(void)
{
    sniff_ready_status = 1;
}

void bt_check_enter_sniff()
{
    struct sniff_ctrl_config_t sniff_ctrl_config;
    u8 addr[12];
    u8 conn_cnt = 0;
    u8 i = 0;

    if (sniff_ready_status) {
        sniff_ready_status = 0;
        return;
    }

    /*putchar('H');*/
    conn_cnt = bt_api_enter_sniff_status_check(SNIFF_CNT_TIME, addr);

    ASSERT(conn_cnt <= 2);

    for (i = 0; i < conn_cnt; i++) {
        log_info("-----USER SEND SNIFF IN %d %d\n", i, conn_cnt);
        sniff_ctrl_config.sniff_max_interval = SNIFF_MAX_INTERVALSLOT;
        sniff_ctrl_config.sniff_mix_interval = SNIFF_MIN_INTERVALSLOT;
        sniff_ctrl_config.sniff_attemp = SNIFF_ATTEMPT_SLOT;
        sniff_ctrl_config.sniff_timeout  = SNIFF_TIMEOUT_SLOT;
        memcpy(sniff_ctrl_config.sniff_addr, addr + i * 6, 6);
        user_send_cmd_prepare(USER_CTRL_SNIFF_IN, sizeof(struct sniff_ctrl_config_t), (u8 *)&sniff_ctrl_config);
    }

}
void sys_auto_sniff_controle(u8 enable, u8 *addr)
{
#if TCFG_USER_EDR_ENABLE
    if (addr) {
        if (bt_api_conn_mode_check(enable, addr) == 0) {
            log_info("sniff ctr not change\n");
            return;
        }
    }

    if (enable) {
        if (addr) {
            log_info("sniff cmd timer init\n");
            user_cmd_timer_init();
        }

        if (sniff_timer == 0) {
            log_info("check_sniff_enable\n");
            sniff_timer = sys_timer_add(NULL, bt_check_enter_sniff, 1000);
        }
    } else {

        if (addr) {
            log_info("sniff cmd timer remove\n");
            remove_user_cmd_timer();
        }

        if (sniff_timer) {
            log_info("check_sniff_disable\n");
            sys_timeout_del(sniff_timer);
            sniff_timer = 0;

            if (exit_sniff_timer == 0) {
                /* exit_sniff_timer = sys_timer_add(NULL, bt_check_exit_sniff, 5000); */
            }
        }
    }
#endif
}
/*开关可发现可连接的函数接口*/
static void bt_wait_phone_connect_control(u8 enable)
{
#if 0
    if (enable) {
        log_info("is_1t2_connection:%d \t total_conn_dev:%d\n", is_1t2_connection(), get_total_connect_dev());
        if (is_1t2_connection()) {
            /*达到最大连接数，可发现(0)可连接(0)*/
            user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
            user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
        } else {
            if (get_total_connect_dev() == 1) {
                /*支持连接2台，只连接一台的情况下，可发现(0)可连接(1)*/
                user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
                user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
            } else {
                /*可发现(1)可连接(1)*/
                user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
                user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
            }
        }
    } else {
        user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    }
#endif
}

void bt_wait_phone_connect_control_ext(u8 inquiry_en, u8 page_scan_en)
{
#if TCFG_USER_EDR_ENABLE

    if (inquiry_en) {
        user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_ENABLE, 0, NULL);
    } else {
        user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    }

    if (page_scan_en) {
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
    } else {
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    }
#endif
}

void bt_send_pair(u8 en)
{
    user_send_cmd_prepare(USER_CTRL_PAIR, 1, &en);
}


#define HCI_EVENT_INQUIRY_COMPLETE                            0x01
#define HCI_EVENT_CONNECTION_COMPLETE                         0x03
#define HCI_EVENT_DISCONNECTION_COMPLETE                      0x05
#define HCI_EVENT_PIN_CODE_REQUEST                            0x16
#define HCI_EVENT_IO_CAPABILITY_REQUEST                       0x31
#define HCI_EVENT_USER_CONFIRMATION_REQUEST                   0x33
#define HCI_EVENT_USER_PASSKEY_REQUEST                        0x34
#define HCI_EVENT_USER_PRESSKEY_NOTIFICATION			      0x3B
#define HCI_EVENT_VENDOR_NO_RECONN_ADDR                       0xF8
#define HCI_EVENT_VENDOR_REMOTE_TEST                          0xFE
#define BTSTACK_EVENT_HCI_CONNECTIONS_DELETE                  0x6D


#define ERROR_CODE_SUCCESS                                    0x00
#define ERROR_CODE_PAGE_TIMEOUT                               0x04
#define ERROR_CODE_AUTHENTICATION_FAILURE                     0x05
#define ERROR_CODE_PIN_OR_KEY_MISSING                         0x06
#define ERROR_CODE_CONNECTION_TIMEOUT                         0x08
#define ERROR_CODE_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED  0x0A
#define ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS                      0x0B
#define ERROR_CODE_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES       0x0D
#define ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR    0x0F
#define ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED         0x10
#define ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION          0x13
#define ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST        0x16

#define CUSTOM_BB_AUTO_CANCEL_PAGE                            0xFD  //// app cancle page
#define BB_CANCEL_PAGE                                        0xFE  //// bb cancle page


static void bt_hci_event_connection(struct bt_event *bt)
{
    bt_wait_phone_connect_control_ext(0, 0);
}

static void bt_hci_event_disconnect(struct bt_event *bt)
{
    /* #if (RCSP_BTMATE_EN && RCSP_UPDATE_EN) */
    /*     if (get_jl_update_flag()) { */
    /*         JL_rcsp_event_to_user(DEVICE_EVENT_FROM_RCSP, MSG_JL_UPDATE_START, NULL, 0); */
    /*     } */
    /* #endif */
    bt_wait_phone_connect_control_ext(1, 1);
}

static void bt_hci_event_linkkey_missing(struct bt_event *bt)
{
}

static void bt_hci_event_page_timeout(struct bt_event *bt)
{
    bt_wait_phone_connect_control_ext(1, 1);
}

static void bt_hci_event_connection_timeout(struct bt_event *bt)
{
    bt_wait_phone_connect_control_ext(1, 1);
}

static void bt_hci_event_connection_exist(struct bt_event *bt)
{
    bt_wait_phone_connect_control_ext(1, 1);
}



static int bt_hci_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_info("------------------------bt_hci_event_handler reason %x %x", bt->event, bt->value);

    if (bt->event == HCI_EVENT_VENDOR_REMOTE_TEST) {
        if (0 == bt->value) {
            set_remote_test_flag(0);
            log_info("clear_test_box_flag");
            return 0;
        } else {

#if TCFG_USER_BLE_ENABLE
            //1:edr con;2:ble con;
            if (1 == bt->value) {
                bt_ble_adv_enable(0);
            }
#endif
        }
    }

    if ((bt->event != HCI_EVENT_CONNECTION_COMPLETE) ||
        ((bt->event == HCI_EVENT_CONNECTION_COMPLETE) && (bt->value != ERROR_CODE_SUCCESS))) {
#if TCFG_TEST_BOX_ENABLE
        if (chargestore_get_testbox_status()) {
            if (get_remote_test_flag()) {
                chargestore_clear_connect_status();
            }
            //return 0;
        }
#endif
        if (get_remote_test_flag() \
            && !(HCI_EVENT_DISCONNECTION_COMPLETE == bt->event) \
            && !(HCI_EVENT_VENDOR_REMOTE_TEST == bt->event)) {
            log_info("cpu reset\n");
            cpu_reset();
        }
    }

    switch (bt->event) {
    case HCI_EVENT_INQUIRY_COMPLETE:
        log_info(" HCI_EVENT_INQUIRY_COMPLETE \n");
        /* bt_hci_event_inquiry(bt); */
        break;
    case HCI_EVENT_USER_CONFIRMATION_REQUEST:
        log_info(" HCI_EVENT_USER_CONFIRMATION_REQUEST \n");
        ///<可通过按键来确认是否配对 1：配对   0：取消
        bt_send_pair(1);
        break;
    case HCI_EVENT_USER_PASSKEY_REQUEST:
        log_info(" HCI_EVENT_USER_PASSKEY_REQUEST \n");
        ///<可以开始输入6位passkey
        break;
    case HCI_EVENT_USER_PRESSKEY_NOTIFICATION:
        log_info(" HCI_EVENT_USER_PRESSKEY_NOTIFICATION %x\n", bt->value);
        ///<可用于显示输入passkey位置 value 0:start  1:enrer  2:earse   3:clear  4:complete
        break;
    case HCI_EVENT_PIN_CODE_REQUEST :
        log_info("HCI_EVENT_PIN_CODE_REQUEST  \n");
        bt_send_pair(1);
        break;

    case HCI_EVENT_VENDOR_NO_RECONN_ADDR :
        log_info("HCI_EVENT_VENDOR_NO_RECONN_ADDR \n");
        bt_hci_event_disconnect(bt) ;
        break;

    case HCI_EVENT_DISCONNECTION_COMPLETE :
        log_info("HCI_EVENT_DISCONNECTION_COMPLETE \n");
        bt_hci_event_disconnect(bt) ;
        break;

    case BTSTACK_EVENT_HCI_CONNECTIONS_DELETE:
    case HCI_EVENT_CONNECTION_COMPLETE:
        log_info(" HCI_EVENT_CONNECTION_COMPLETE \n");
        switch (bt->value) {
        case ERROR_CODE_SUCCESS :
            log_info("ERROR_CODE_SUCCESS  \n");
            bt_hci_event_connection(bt);
            break;
        case ERROR_CODE_PIN_OR_KEY_MISSING:
            log_info(" ERROR_CODE_PIN_OR_KEY_MISSING \n");
            bt_hci_event_linkkey_missing(bt);

        case ERROR_CODE_SYNCHRONOUS_CONNECTION_LIMIT_TO_A_DEVICE_EXCEEDED :
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_LIMITED_RESOURCES:
        case ERROR_CODE_CONNECTION_REJECTED_DUE_TO_UNACCEPTABLE_BD_ADDR:
        case ERROR_CODE_CONNECTION_ACCEPT_TIMEOUT_EXCEEDED  :
        case ERROR_CODE_REMOTE_USER_TERMINATED_CONNECTION   :
        case ERROR_CODE_CONNECTION_TERMINATED_BY_LOCAL_HOST :
        case ERROR_CODE_AUTHENTICATION_FAILURE :
        case CUSTOM_BB_AUTO_CANCEL_PAGE:
            bt_hci_event_disconnect(bt) ;
            break;

        case ERROR_CODE_PAGE_TIMEOUT:
            log_info(" ERROR_CODE_PAGE_TIMEOUT \n");
            bt_hci_event_page_timeout(bt);
            break;

        case ERROR_CODE_CONNECTION_TIMEOUT:
            log_info(" ERROR_CODE_CONNECTION_TIMEOUT \n");
            bt_hci_event_connection_timeout(bt);
            break;

        case ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS  :
            log_info("ERROR_CODE_ACL_CONNECTION_ALREADY_EXISTS   \n");
            bt_hci_event_connection_exist(bt);
            break;
        default:
            break;

        }
        break;
    default:
        break;

    }
    return 0;
}


static int bt_connction_status_event_handler(struct bt_event *bt)
{
    log_info("-----------------------bt_connction_status_event_handler %d", bt->event);

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        /*
         * 蓝牙初始化完成
         */
        log_info("BT_STATUS_INIT_OK\n");

        /* transport_spp_init(); */
        /* bt_wait_phone_connect_control_ext(1, 1); */

#if TCFG_USER_BLE_ENABLE
#if POWER_ON_PAIR_START
        //蓝牙初始化后,才可调用
        log_info("power pair start");
        dongle_timer_handle(0);
        dongle_timer_id = sys_timer_add((void *)1, dongle_timer_handle, POWER_ON_PAIR_TIME);
#endif

        rf_set_24g_hackable_coded(CFG_RF_24G_CODE_ID);
        bt_ble_init();
#endif

#if TCFG_USER_EDR_ENABLE
        //no support close edr
#ifndef CONFIG_CPU_BR30
        radio_set_eninv(0);
#endif
        bredr_power_put();
        sys_auto_sniff_controle(0, NULL);
#endif


        break;

    case BT_STATUS_SECOND_CONNECTED:
    case BT_STATUS_FIRST_CONNECTED:
        log_info("BT_STATUS_CONNECTED\n");
        break;

    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
        log_info("BT_STATUS_DISCONNECT\n");
        break;

    case BT_STATUS_PHONE_INCOME:
        log_info("BT_STATUS_PHONE_INCOME\n");
        break;

    case BT_STATUS_PHONE_OUT:
        log_info("BT_STATUS_PHONE_OUT\n");
        break;

    case BT_STATUS_PHONE_ACTIVE:
        log_info("BT_STATUS_PHONE_ACTIVE\n");
        break;

    case BT_STATUS_PHONE_HANGUP:
        log_info("BT_STATUS_PHONE_HANGUP\n");
        break;

    case BT_STATUS_PHONE_NUMBER:
        log_info("BT_STATUS_PHONE_NUMBER\n");
        break;

    case BT_STATUS_INBAND_RINGTONE:
        log_info("BT_STATUS_INBAND_RINGTONE\n");
        break;

    case BT_STATUS_BEGIN_AUTO_CON:
        log_info("BT_STATUS_BEGIN_AUTO_CON\n");
        break;

    case BT_STATUS_A2DP_MEDIA_START:
        log_info(" BT_STATUS_A2DP_MEDIA_START");
        break;

    case BT_STATUS_SNIFF_STATE_UPDATE:
        log_info(" BT_STATUS_SNIFF_STATE_UPDATE %d\n", bt->value);    //0退出SNIFF
        if (bt->value == 0) {
            sys_auto_sniff_controle(1, bt->args);
        } else {
            sys_auto_sniff_controle(0, bt->args);
        }
        break;
    default:
        log_info(" BT STATUS DEFAULT\n");
        break;
    }
    return 0;
}

static void app_key_event_handler(struct sys_event *event)
{
    /* u16 cpi = 0; */
    u8 event_type = 0;
    u8 key_value = 0;

    if (event->arg == (void *)DEVICE_EVENT_FROM_KEY) {
        event_type = event->u.key.event;
        key_value = event->u.key.value;
        printf("app_key_evnet: %d,%d\n", event_type, key_value);
        /* app_key_deal_test(event_type,key_value); */

        if (event_type == KEY_EVENT_LONG && key_value == TCFG_ADKEY_VALUE6) {
            app_set_soft_poweroff();
        }
    }
}

static int event_handler(struct application *app, struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        app_key_event_handler(event);
        return 0;

    case SYS_BT_EVENT:
        if ((u32)event->arg == SYS_BT_EVENT_TYPE_CON_STATUS) {
            bt_connction_status_event_handler(&event->u.bt);
        } else if ((u32)event->arg == SYS_BT_EVENT_TYPE_HCI_STATUS) {
            bt_hci_event_handler(&event->u.bt);
        }
        return 0;

    case SYS_DEVICE_EVENT:
        if ((u32)event->arg == DEVICE_EVENT_FROM_POWER) {
            return app_power_event_handler(&event->u.dev);
        }
#if TCFG_CHARGE_ENABLE
        else if ((u32)event->arg == DEVICE_EVENT_FROM_CHARGE) {
            app_charge_event_handler(&event->u.dev);
        }
#endif
        return 0;

    default:
        return FALSE;
    }
    return FALSE;
}



static const struct application_operation app_ops = {
    .state_machine  = state_machine,
    .event_handler 	= event_handler,
};

/*
 * 注册AT Module模式
 */
REGISTER_APPLICATION(app_dongle) = {
    .name 	= "dongle",
    .action	= ACTION_DONGLE_MAIN,
    .ops 	= &app_ops,
    .state  = APP_STA_DESTROY,
};

//-----------------------
//system check go sleep is ok
static u8 app_state_idle_query(void)
{
    return !is_app_active;
}

REGISTER_LP_TARGET(app_state_lp_target) = {
    .name = "app_state_deal",
    .is_idle = app_state_idle_query,
};

#endif


