/*********************************************************************************************
    *   Filename        : app_comm_edr.c

    *   Description     :

    *   Author          : MRL

    *   Email           : MRL@zh-jieli.com

    *   Last modifiled  : 2021-06-172 14:01

    *   Copyright:(c)JIELI  2011-2021  @ , All Rights Reserved.
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
#include "edr_hid_user.h"
#include "le_common.h"
#include <stdlib.h>
#include "standard_hid.h"
#include "rcsp_bluetooth.h"
#include "app_charge.h"
#include "app_power_manage.h"
#include "app_chargestore.h"
#include "app_comm_bt.h"

#define LOG_TAG_CONST       COMM_EDR
#define LOG_TAG             "[COMM_EDR]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


#if (!TCFG_USER_EDR_ENABLE) && (USER_SUPPORT_PROFILE_HID || USER_SUPPORT_PROFILE_HFP)
//配置需要一致
#error "need disable USER_SUPPORT_PROFILE_XXX!!!!!!"
#endif


#if TCFG_USER_EDR_ENABLE

#define SNIFF_ENABLE                  1 //是否允许主动进入sniff

//默认配置
static const edr_sniff_par_t edr_default_sniff_param = {
    .sniff_mode = SNIFF_MODE_DEF,
    .cnt_time = 1,
    .max_interval_slots = 800,
    .min_interval_slots = 100,
    .attempt_slots = 4,
    .timeout_slots = 1,
    .check_timer_period = 1000,
};


static u8 sniff_ready_status = 0; //0:sniff_ready 1:sniff_not_ready
static int sniff_timer = 0;
static const edr_sniff_par_t *sniff_param_info;
static u8 edr_remote_address[6];
static u16 negotiation_sniff_interval_offset = 0;
extern int edr_hid_timer_handle;

extern void user_spp_data_handler(u8 packet_type, u16 ch, u8 *packet, u16 size);
static void sys_auto_sniff_controle(u8 enable, u8 *addr);
extern void bredr_set_dut_enble(u8 en, u8 phone);

#define SNIFF_SLOT_STEP 6 //slot步进数
#define SNIFF_PARAM_COUNT 3 //参数请求组个数

/* ***************************************************************************/
/**
 * \Brief :       库调用:进入sniff成功后，协商的通信间隔值(unit:slot,625us)
 *
 * \Param :       addr---remote's addr
 * \Param :       t_sniff---通信间隔，slots
 * \Param :        negotiation
 */
/* ***************************************************************************/
void bt_sniff_param_hook(u8 *addr, u16 t_sniff)
{
    log_info("t_sniff= %d us", (u32)t_sniff * 625);
}

/* ***************************************************************************/
/**
 * \Brief :       库调用进行sniff请求参数更新
 *
 * \Param :
 * \Param :        attemp
 * \Param :        timeout
 * \Param :        negotiation
 */
/* ***************************************************************************/
void __attribute__((weak)) sniff_negotiation_hook(u16 *T_sniff, u16 *attemp, u16 *timeout, u8 negotiation)
{
    static u8 negotiation_count;
    if (sniff_param_info->sniff_mode == SNIFF_MODE_ANCHOR) {
        //库提供的negotiation变量的值是 1~3~1~3 循环
        if (negotiation == 1) {
            //三次请求的相同sniff参数对端设备都不接受 只能进行下一组参数的请求
            negotiation_count++;
        }
        if (negotiation_count > SNIFF_PARAM_COUNT) {
            negotiation_count = 0;
            log_error("sniff negotiation error Unable to negotiate");
            return;
        }
        negotiation_sniff_interval_offset += negotiation_count * SNIFF_SLOT_STEP;
    }
    *T_sniff = sniff_param_info->max_interval_slots + negotiation_sniff_interval_offset;
    *attemp = sniff_param_info->attempt_slots;
    *timeout = sniff_param_info->timeout_slots;
}

/* ***************************************************************************/
/**
 * \Brief :       获得sniff更新周期
 *
 * \Return :      sniff周期
 */
/* ***************************************************************************/
u16 get_app_sniff_interval()
{
    return (sniff_param_info->max_interval_slots + negotiation_sniff_interval_offset) * 5 / 8;  //ms
}

/*************************************************************************************************/
/*!
 *  \brief      获取电量等级
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/

static int bt_get_battery_value()
{
    //将当前电量转换为1~9级发送给手机同步电量
    u8 battery_level = 0;

#if TCFG_SYS_LVD_EN
    u8 vbat_percent = get_vbat_percent();

    if (vbat_percent < 5) { //小于5%电量等级为0，显示10%
        return 0;
    }
    battery_level = (vbat_percent - 5) / 10;

    //取消默认蓝牙定时发送电量给手机，需要更新电量给手机使用USER_CTRL_HFP_CMD_UPDATE_BATTARY命令
    //user_send_cmd_prepare(USER_CTRL_HFP_CMD_UPDATE_BATTARY, 0, NULL);
    /*电量协议的是0-9个等级，请比例换算*/
    log_info("bt_get_battery_value:%d\n", battery_level);
#endif
    return battery_level;
}

/*************************************************************************************************/
/*!
 *  \brief     edr配置,协议栈初始化前调用
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void btstack_edr_start_before_init(const edr_init_cfg_t *cfg, int param)
{
    if (!cfg) {
        log_error("cfg is null!!!");
        ASSERT(0);
    }

    sniff_param_info = cfg->sniff_param;

    if (!sniff_param_info) {
        sniff_param_info = &edr_default_sniff_param;
    }

    log_info("sniff_mode:%d", sniff_param_info->sniff_mode);
    log_info("cnt_time:%d", sniff_param_info->cnt_time);
    log_info("check_timer_ms:%d", sniff_param_info->check_timer_period);
    log_info("max_interval_slots:%d", sniff_param_info->max_interval_slots);
    log_info("min_interval_slots:%d", sniff_param_info->min_interval_slots);
    log_info("attempt_slots:%d", sniff_param_info->attempt_slots);
    log_info("timeout_slots:%d", sniff_param_info->timeout_slots);

    __set_user_ctrl_conn_num(1);

#if USER_SUPPORT_PROFILE_HFP
    __set_disable_sco_flag(1);////禁止发起esco 通话从手机出声音
#if TCFG_SYS_LVD_EN
//edr通过hfp显示电量
    __bt_set_update_battery_time(60);
    get_battery_value_register(bt_get_battery_value);   /*电量显示获取电量的接口*/
#else
    __bt_set_update_battery_time(0);
#endif
#else
    __bt_set_update_battery_time(0);
#endif

    /*回连搜索时间长度设置,可使用该函数注册使用，ms单位,u16*/
    __set_page_timeout_value(cfg->page_timeout);

    /*回连时超时参数设置。ms单位。做主机有效*/
    __set_super_timeout_value(cfg->super_timeout);

#if (USER_SUPPORT_PROFILE_HID==1)
    user_hid_set_icon(cfg->class_type);//default icon
    user_hid_set_ReportMap(cfg->report_map, cfg->report_map_size);
    user_hid_init(NULL);
    //搜索图标
#else
    __change_hci_class_type(cfg->class_type);//default icon
#endif

    //io_capabilities ; /*0: Display only 1: Display YesNo 2: KeyboardOnly 3: NoInputNoOutput*/
    //oob: 0:data not present 1:data from remote device present
    //authentication_requirements:
    //0:MITM protect not require - NO bonding
    //1:MITM protect require - NO bonding
    //2:MITM protect not require - dedicated bonding
    //3:MITM protect require - dedicated bonding
    //4:MITM protect not require - general bonding
    //5:MITM protect require - general bonding
    __set_simple_pair_param(cfg->io_capabilities, cfg->oob_data, cfg->authentication_req);
    __set_simple_pair_flag(!cfg->passkey_enable);

    log_info("---edr's address");
    printf_buf((void *)bt_get_mac_addr(), 6);

}

/*************************************************************************************************/
/*!
 *  \brief      协议栈初始化成功,调用
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void btstack_edr_start_after_init(int param)
{
    if (sniff_support_reset_anchor_point) {
        lmp_sniff_t_slot_attemp_reset(sniff_param_info->max_interval_slots, sniff_param_info->attempt_slots);
    }

#if SNIFF_ENABLE
    /* bt_wait_phone_connect_control_ext(1, 1); */
    sys_auto_sniff_controle(1, NULL);
#else
    //lmp_set_sniff_disable();
    //lmp_set_sniff_establish_by_remote(1);
#endif
}

/*************************************************************************************************/
/*!
 *  \brief      edr退出
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void btstack_edr_exit(int param)
{
    bt_wait_phone_connect_control_ext(0, 0);
#if USER_SUPPORT_PROFILE_HFP
    if (get_curr_channel_state() & HFP_CH) {
        log_info("disconnect hfp\n");
        user_send_cmd_prepare(USER_CTRL_HFP_DISCONNECT, 0, NULL);
    }
#endif

#if (USER_SUPPORT_PROFILE_HID==1)
    log_info("hid exit\n");
    user_hid_exit();
#endif

    //ble先退,edr最后退出的
    log_info("===btstack_exit\n");
    btstack_exit();
}

/*************************************************************************************************/
/*!
 *  \brief      公共事件消息处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
extern void transport_spp_init(void);
static int bt_comm_edr_status_event_handler(struct bt_event *bt)
{
    log_info("--------%s: %d", __FUNCTION__, bt->event);

    switch (bt->event) {
    case BT_STATUS_INIT_OK:
        /*
         * 蓝牙初始化完成
         */
        log_info("STATUS_INIT_OK\n");

#if TCFG_NORMAL_SET_DUT_MODE
        log_info("set dut mode\n");
        bredr_set_dut_enble(1, 1);
#endif
        btstack_edr_start_after_init(0);
        break;

    case BT_STATUS_SECOND_CONNECTED:
    case BT_STATUS_FIRST_CONNECTED:
        log_info("BT_STATUS_CONNECTED\n");
        put_buf(bt->args, 6);
        memcpy(edr_remote_address, bt->args, 6);
        log_info("edr remote rssi= %d\n", bredr_get_rssi_for_address(edr_remote_address));
        break;

    case BT_STATUS_FIRST_DISCONNECT:
    case BT_STATUS_SECOND_DISCONNECT:
        log_info("BT_STATUS_DISCONNECT\n");
        negotiation_sniff_interval_offset = 0;  //重置sinff偏移量
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
            if (edr_hid_timer_handle) {
                sys_s_hi_timer_modify(edr_hid_timer_handle, (u32)(get_app_sniff_interval()));
            }
        }
        break;

    case  BT_STATUS_TRIM_OVER:
        log_info("BT STATUS TRIM OVER\n");
        break;

    case BT_STATUS_RECONN_OR_CONN:
        log_info("BT_STATUS_RECONN_OR_CONN \n");
#if USER_SUPPORT_PROFILE_MAP
        log_info("USER_CTRL_MAP_READ_TIME");
        user_send_cmd_prepare(USER_CTRL_MAP_READ_TIME, 0, NULL);
#endif
    default:
        log_info("BT STATUS DEFAULT\n");
        break;
    }
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief     请求退出sniff
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
int bt_comm_edr_sniff_clean(void)
{
    if (sniff_param_info->sniff_mode == SNIFF_MODE_DEF) {
        sniff_ready_status = 1;
        if (sniff_timer) {
            user_send_cmd_prepare(USER_CTRL_ALL_SNIFF_EXIT, 0, NULL);
            return 0;
        }
    }
    return 1;
}

/*************************************************************************************************/
/*!
 *  \brief      定时检查是否进入sniff
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void bt_check_enter_sniff()
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
    conn_cnt = bt_api_enter_sniff_status_check(sniff_param_info->cnt_time, addr);

    ASSERT(conn_cnt <= 2);

    for (i = 0; i < conn_cnt; i++) {
        log_info("-----USER SEND SNIFF IN %d %d\n", i, conn_cnt);
        sniff_ctrl_config.sniff_max_interval = sniff_param_info->max_interval_slots;
        sniff_ctrl_config.sniff_mix_interval = sniff_param_info->min_interval_slots;
        sniff_ctrl_config.sniff_attemp = sniff_param_info->attempt_slots;
        sniff_ctrl_config.sniff_timeout  = sniff_param_info->timeout_slots;
        memcpy(sniff_ctrl_config.sniff_addr, addr + i * 6, 6);
        user_send_cmd_prepare(USER_CTRL_SNIFF_IN, sizeof(struct sniff_ctrl_config_t), (u8 *)&sniff_ctrl_config);
    }

}

/*************************************************************************************************/
/*!
 *  \brief      sniff 控制
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void sys_auto_sniff_controle(u8 enable, u8 *addr)
{
#if SNIFF_ENABLE
    static u8 enable_state = -1;

    if (enable_state == enable) {
        return;
    }

    log_info("---%s,%d", __FUNCTION__, enable);

    enable_state = enable;

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
            sniff_timer = sys_timer_add(NULL, bt_check_enter_sniff, sniff_param_info->check_timer_period);
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
        }
    }
#endif
}

/*************************************************************************************************/
/*!
 *  \brief       开关可发现可连接的函数接口
 *
 *  \param      [in] inquiry_en:可发现;  page_scan_en:可连接
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void bt_wait_phone_connect_control_ext(u8 inquiry_en, u8 page_scan_en)
{
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
}

/*************************************************************************************************/
/*!
 *  \brief      开等待连接
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void bt_wait_phone_connect_control(u8 enable)
{
    bt_wait_phone_connect_control_ext(enable, enable);
}

/*************************************************************************************************/
/*!
 *  \brief      配置是否接受配对
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void bt_send_pair(u8 en)
{
    user_send_cmd_prepare(USER_CTRL_PAIR, 1, &en);
}

//已连接
static void bt_hci_event_connection(struct bt_event *bt)
{
    bt_wait_phone_connect_control_ext(0, 0);
}

//已断开
static void bt_hci_event_disconnect(struct bt_event *bt)
{
    bt_wait_phone_connect_control_ext(1, 1);
}

//link key  丢失
static void bt_hci_event_linkkey_missing(struct bt_event *bt)
{
    bt_wait_phone_connect_control_ext(1, 1);
}

//回连超时
static void bt_hci_event_page_timeout(struct bt_event *bt)
{
    bt_wait_phone_connect_control_ext(1, 1);
}

//连接超时
static void bt_hci_event_connection_timeout(struct bt_event *bt)
{
    if (!bt_connect_phone_back_start()) {
        bt_wait_phone_connect_control_ext(1, 1);
    }
}

//连接退出
static void bt_hci_event_connection_exist(struct bt_event *bt)
{
    bt_wait_phone_connect_control_ext(1, 1);
}

/*************************************************************************************************/
/*!
 *  \brief      处理协议栈hci事件
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
extern void set_remote_test_flag(u8 own_remote_test);
int bt_comm_edr_hci_event_handler(struct bt_event *bt)
{
    //对应原来的蓝牙连接上断开处理函数  ,bt->value=reason
    log_info("--------%s reason %x %x", __FUNCTION__, bt->event, bt->value);

    if (bt->event == HCI_EVENT_VENDOR_REMOTE_TEST) {
        log_info("TEST_BOX:%d", bt->value);
        switch (bt->value) {
        case VENDOR_TEST_DISCONNECTED:
            set_remote_test_flag(0);
            log_info("clear_test_box_flag");
            cpu_reset();
            return 0;
            break;

        case VENDOR_TEST_LEGACY_CONNECTED_BY_BT_CLASSIC:
#if TCFG_USER_BLE_ENABLE
            ble_module_enable(0);
#endif
            break;

        default:
            break;
        }
        return 0;
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
/*************************************************************************************************/
/*!
 *  \brief       connect last device
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/

extern u8 connect_last_device_from_vm();
int bt_connect_phone_back_start(void)
{
    log_info("bt_connect_phone_back_start");
    if (connect_last_device_from_vm()) {
        log_info("---connect_last_device_from_vm");
        return 1 ;
    }
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      get remote address
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void bt_comm_edr_get_remote_address(bd_addr_t address)
{
    memcpy(address, edr_remote_address, 6);
}

/*************************************************************************************************/
/*!
 *  \brief       edr_mode_enable
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/

//使能edr模块 开关
void bt_comm_edr_mode_enable(u8 enable)
{
    static u8 edr_cfg_en = 1;
    if (edr_cfg_en == enable) {
        log_info("###repeat %s:%02x\n", __FUNCTION__, enable);
        return;
    }

    edr_cfg_en = enable;

    log_info("%s:%02x", __FUNCTION__, enable);

    if (enable) {
        btctrler_task_init_bredr();
        //default
#if (USER_SUPPORT_PROFILE_HID==1)
        user_hid_enable(1);
#endif
        sys_auto_sniff_controle(1, NULL);
    } else {

#if USER_SUPPORT_PROFILE_HFP
        if (get_curr_channel_state() & HFP_CH) {
            user_send_cmd_prepare(USER_CTRL_HFP_DISCONNECT, 0, NULL);
        }
#endif

#if (USER_SUPPORT_PROFILE_HID==1)
        user_hid_enable(0);
#endif
        bt_wait_phone_connect_control(0);
        sys_auto_sniff_controle(0, NULL);
#if 0
        log_info("radio_set_eninv");
#ifndef CONFIG_NEW_BREDR_ENABLE
        radio_set_eninv(0);
#endif
        bredr_power_put();
#else
        log_info("btctrler_task_close_bredr");
        btctrler_task_close_bredr();
#endif
    }

    log_info("%s end", __FUNCTION__);
}

#if USER_SUPPORT_PROFILE_MAP
#define PROFILE_CMD_TRY_AGAIN_LATER 	    -1004
void bt_get_time_date()
{
    log_info("hfp_get_time_date");
    int error = user_send_cmd_prepare(USER_CTRL_HFP_GET_PHONE_DATE_TIME, 0, NULL);
    log_info(">>>>>error = %d\n", error);
    if (error == PROFILE_CMD_TRY_AGAIN_LATER) {
        sys_timeout_add(NULL, bt_get_time_date, 100);
    }
}
void phone_date_and_time_feedback(u8 *data,  u16 len)
{
    log_info("hfp_get_time: %s", data);
}  
void map_get_time_data(char *time, int status)
{
    if (status  ==  0) {
        log_info("map_get_time: %s", time);
    } else  {
            log_info(">>>map get fail\n");
            sys_timeout_add(NULL, bt_get_time_date, 100);
        }  
    }
#endif

#endif



