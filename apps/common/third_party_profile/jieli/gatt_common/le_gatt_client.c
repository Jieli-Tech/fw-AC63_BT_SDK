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

#include "le_client_demo.h"
#include "le_common.h"
#include "ble_user.h"
#include "le_gatt_common.h"
#include "ble/hci_ll.h"

#define LOG_TAG_CONST       GATT_CLIENT
#define LOG_TAG             "[GATT_CLIENT]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_USER_BLE_ENABLE && CONFIG_BT_GATT_COMMON_ENABLE

/* #ifdef log_info */
/* #undef log_info */
/* #define log_info(x, ...)  g_printf("[GATT_CLIENT]" x " ", ## __VA_ARGS__) */
/* #define log_info_hexdump  put_buf */
/* #endif */

//---------------
#define SEARCH_PROFILE_BUFSIZE    (512)                   //note:
static u8 search_ram_buffer[SEARCH_PROFILE_BUFSIZE] __attribute__((aligned(4)));
#define scan_buffer search_ram_buffer

//---------------
#define BASE_INTERVAL_MIN         (6)//最小的interval
#if(SUPPORT_MAX_GATT_CLIENT > 2)
#define BASE_INTERVAL_VALUE       (BASE_INTERVAL_MIN*4)
#else
#define BASE_INTERVAL_VALUE       (BASE_INTERVAL_MIN)
#endif
//---------------
//定时器类型
enum {
    TO_TYPE_CREAT_CONN = 0,
};

#define SUPPORT_OPT_HANDLE_MAX   16

//---------------
typedef struct {
    scan_conn_cfg_t *scan_conn_config; //扫描配置项
    gatt_client_cfg_t *client_config;//client配置项
    gatt_search_cfg_t *gatt_search_config;//搜索profile配置项
    u8  client_work_state;   //未连接状态
    u8  scan_ctrl_en;        //控制开关
    u16 client_timeout_id; //定时器
    u16 client_operation_handle; //操作流程中con_handle
    u16 client_encrypt_process; //配对加密流程
    u16 client_search_handle; //搜索的con_handle
    opt_handle_t operate_handle_table[SUPPORT_OPT_HANDLE_MAX];//记录需要read,write,notify,indicate的ATT handle
    u8 opt_handle_used_cnt; //记录个数
    u8 res_byes; //
    u16 just_search_handle; //操作模式:只搜索profile,不建立链路连接
} client_ctl_t;

static client_ctl_t client_s_hdl;
#define __this    (&client_s_hdl)
static u8 disconn_auto_scan_do = 1;//默认设置为1
extern const int config_btctler_coded_type;
//----------------------------------------------------------------------------
//---------------------------------------------------------------------------
static void __gatt_client_check_auto_scan(void);
bool ble_comm_need_wait_encryption(u8 role);

#if EXT_ADV_MODE_EN || PERIODIC_ADV_MODE_EN

//搜索类型
#define SET_EXT_SCAN_TYPE       SCAN_ACTIVE
//搜索 周期大小
#define SET_EXT_SCAN_INTERVAL   64 //(unit:0.625ms)
//搜索 窗口大小
#define SET_EXT_SCAN_WINDOW     16 //(unit:0.625ms)

//连接周期
#define SET_EXT_CONN_INTERVAL   24 //(unit:1.25ms)
//连接latency
#define SET_EXT_CONN_LATENCY    0  //(unit:conn_interval)
//连接超时
#define SET_EXT_CONN_TIMEOUT    400 //(unit:10ms)

static bool __check_device_is_match(u8 event_type, u8 info_type, u8 *data, int size, client_match_cfg_t **output_match_devices);
static bool __resolve_adv_report(adv_report_t *report_pt, u16 len);
static u8 periodic_scan_state = 0;

static struct __periodic_creat_sync periodic_creat_sync = {
    .Filter_Policy = 0,
    .Advertising_SID = CUR_ADVERTISING_SID,
    .Skip = {0, 0},
    .Sync_Timeout = SYNC_TIMEOUT_MS(10000),
};


const static le_ext_scan_param_lite_t ext_scan_param = {
    .Own_Address_Type = 0,
    .Scanning_Filter_Policy = 0,
    .Scanning_PHYs = SCAN_SET_1M_PHY,
    .Scan_Type = SET_EXT_SCAN_TYPE,
    .Scan_Interval = SET_EXT_SCAN_INTERVAL,
    .Scan_Window = SET_EXT_SCAN_WINDOW,
};

const struct __ext_scan_enable ext_scan_enable = {
    .Enable = 1,
    .Filter_Duplicates = 0,
    .Duration = 0,
    .Period = 0,
};

const struct __ext_scan_enable ext_scan_disable = {
    .Enable = 0,
    .Filter_Duplicates = 0,
    .Duration = 0,
    .Period = 0,
};

struct __ext_init {
    u8 Initiating_Filter_Policy;
    u8 Own_Address_Type;
    u8 Peer_Address_Type;
    u8 Peer_Address[6];
    u8 Initiating_PHYs;
    u16 Scan_Interval;
    u16 Scan_Window;
    u16 Conn_Interval_Min;
    u16 Conn_Interval_Max;
    u16 Conn_Latency;
    u16 Supervision_Timeout;
    u16 Minimum_CE_Length;
    u16 Maximum_CE_Length;
} _GNU_PACKED_;

#define GET_STRUCT_MEMBER_OFFSET(type, member) \
    (u32)&(((struct type*)0)->member)

/*************************************************************************************************/
/*!
 *  \brief      解析scan到的ext_adv&rsp包数据
 *
 *  \param      [in]
 *
 *  \return     是否有匹配的设备, true or false
 *
 *  \note
 */
/*************************************************************************************************/
static bool __resolve_ext_adv_report(le_ext_adv_report_evt_t *evt, u16 len)
{
    u8 i, lenght, ad_type;
    u8 *adv_data_pt;
    u8 find_remoter = 0;
    u32 tmp32;
    client_match_cfg_t *match_cfg = NULL;

    /* u16 event_type  = report_pt[GET_STRUCT_MEMBER_OFFSET(le_ext_adv_report_evt_t, Event_Type)]; */
    /* u8 address_type = report_pt[GET_STRUCT_MEMBER_OFFSET(le_ext_adv_report_evt_t, Address_Type)]; */
    /* u8 *adv_address = &report_pt[GET_STRUCT_MEMBER_OFFSET(le_ext_adv_report_evt_t,  Address)]; */
    /* u8 *data = &report_pt[GET_STRUCT_MEMBER_OFFSET(le_ext_adv_report_evt_t, Data)]; */
    /* u8 data_length = report_pt[GET_STRUCT_MEMBER_OFFSET(le_ext_adv_report_evt_t,  Data_Length)]; */
    /* s8 rssi = report_pt[GET_STRUCT_MEMBER_OFFSET(le_ext_adv_report_evt_t, RSSI)]; */
    /*  */
    /* log_info_hexdump(evt, len); */

    if (__check_device_is_match((u8)(evt->Event_Type.event_type), CLI_CREAT_BY_ADDRESS, evt->Address, 6, &match_cfg)) {
        find_remoter = 1;
        log_info("catch mac ok\n");
        /* goto just_creat; */
    }
    /* if (check_device_is_match(CLI_CREAT_BY_ADDRESS, adv_address, 6)) { */
    /*     find_remoter = 1; */
    /* } */

    adv_data_pt = evt->Data;
    for (i = 0; i < evt->Data_Length;) {
        if (*adv_data_pt == 0) {
            /* log_info("analyze end\n"); */
            break;
        }

        lenght = *adv_data_pt++;

        if (lenght >= evt->Data_Length || (lenght + i) >= evt->Data_Length) {
            /*过滤非标准包格式*/
            printf("!!!error_adv_packet:");
            put_buf(evt->Data, evt->Data_Length);
            break;
        }

        ad_type = *adv_data_pt++;
        i += (lenght + 1);

        switch (ad_type) {
        case HCI_EIR_DATATYPE_FLAGS:
            /* log_info("flags:%02x\n",adv_data_pt[0]); */
            break;

        case HCI_EIR_DATATYPE_MORE_16BIT_SERVICE_UUIDS:
        case HCI_EIR_DATATYPE_COMPLETE_16BIT_SERVICE_UUIDS:
        case HCI_EIR_DATATYPE_MORE_32BIT_SERVICE_UUIDS:
        case HCI_EIR_DATATYPE_COMPLETE_32BIT_SERVICE_UUIDS:
        case HCI_EIR_DATATYPE_MORE_128BIT_SERVICE_UUIDS:
        case HCI_EIR_DATATYPE_COMPLETE_128BIT_SERVICE_UUIDS:
            /* log_info("service uuid:"); */
            /* log_info_hexdump(adv_data_pt, lenght - 1); */
            break;

        case HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME:
        case HCI_EIR_DATATYPE_SHORTENED_LOCAL_NAME:
            tmp32 = adv_data_pt[lenght - 1];
            adv_data_pt[lenght - 1] = 0;;
            log_info("ble remoter_name: %s,rssi:%d\n", adv_data_pt, evt->RSSI);
            log_info_hexdump(evt->Address, 6);
            adv_data_pt[lenght - 1] = tmp32;

            if (__check_device_is_match((u8)(evt->Event_Type.event_type), CLI_CREAT_BY_NAME, adv_data_pt, lenght - 1, &match_cfg)) {
                find_remoter = 1;
                log_info("catch name ok\n");
            } else {}
            /* if (check_device_is_match(CLI_CREAT_BY_NAME, adv_data_pt, lenght - 1)) { */
            /*     find_remoter = 1; */
            /*     log_info("catch name ok\n"); */
            /* } */
            break;

        case HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA:
            if (__check_device_is_match((u8)(evt->Event_Type.event_type), CLI_CREAT_BY_TAG, adv_data_pt, lenght - 1, &match_cfg)) {
                log_info("get_tag_string!\n");
                find_remoter = 1;
            }
            /* if (check_device_is_match(CLI_CREAT_BY_TAG, adv_data_pt, lenght - 1)) { */
            /*     log_info("get_tag_string!\n"); */
            /*     find_remoter = 1; */
            /* } */
            break;

        case HCI_EIR_DATATYPE_APPEARANCE_DATA:
            /* log_info("get_class_type:%04x\n",little_endian_read_16(adv_data_pt,0)); */
            break;

        default:
            /* log_info("unknow ad_type:"); */
            break;
        }

        if (find_remoter) {
            /* log_info_hexdump(adv_data_pt, lenght - 1); */
        }
        adv_data_pt += (lenght - 1);
    }

    return find_remoter;

}

#endif /* EXT_ADV_MODE_EN */

#if PERIODIC_ADV_MODE_EN
/*************************************************************************************************/
/*!
 *  \brief      解析周期广播数据
 *
 *  \param      [u8 *] report_pt  [u16] len
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void client_report_periodic_adv_data(u8 *report_pt, u16 len)
{
    bool find_remoter;
    u8 Data_Length = report_pt[GET_STRUCT_MEMBER_OFFSET(__periodic_adv_report_event, Data_Length)];
    u8 *Data = &report_pt[GET_STRUCT_MEMBER_OFFSET(__periodic_adv_report_event, Data)];

#if CHAIN_DATA_TEST_EN
    log_info("\n********* Sync periodic adv succ ***********\n");
    log_info_hexdump(Data, Data_Length);
#else
    find_remoter = __resolve_adv_report(\
                                        Data_Length, \
                                        Data \
                                       );

    if (find_remoter) {
        log_info("\n********* Sync periodic adv succ ***********\n");
        log_info_hexdump(Data, Data_Length);
    }
#endif /*  CHAIN_DATA_TEST_EN */
}
#endif /* PERIODIC_ADV_MODE_EN */
//----------------------------------------------------------------------------

/*************************************************************************************************/
/*!
 *  \brief      事件回调输出接口
 *
 *  \param      [in]
 *
 *  \return    gatt_op_ret_e
 *
 *  \note
 */
/*************************************************************************************************/
static int __gatt_client_event_callback_handler(int event, u8 *packet, u16 size, u8 *ext_param)
{
    if (__this->client_config->event_packet_handler) {

        return __this->client_config->event_packet_handler(event, packet, size, ext_param);
    }
    return GATT_OP_RET_SUCESS;
}

/*************************************************************************************************/
/*!
 *  \brief      设置 连接和 未连接状态
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void __gatt_client_set_work_state(u16 conn_handle, ble_state_e state, u8 report_en)
{
    u8 packet_buf[4];

    //区分连接和未连接的两个状态维护
    if (conn_handle != INVAIL_CONN_HANDLE) {
        ;
    } else if (state != __this->client_work_state) {
        log_info("client_work_st:%x->%x\n", __this->client_work_state, state);
        __this->client_work_state = state;
    } else {
        return;
    }
    packet_buf[0] = state;
    little_endian_store_16(packet_buf, 1, conn_handle);
    __gatt_client_event_callback_handler(GATT_COMM_EVENT_CLIENT_STATE, packet_buf, 3, 0);
}

/*************************************************************************************************/
/*!
 *  \brief      timeout 定时处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void __gatt_client_timeout_handler(int type_id)
{
    __this->client_timeout_id = 0;
    if (TO_TYPE_CREAT_CONN == type_id) {
        if (ble_gatt_client_get_work_state() == BLE_ST_CREATE_CONN) {
            log_info("create connection timeout!!!");
            ble_gatt_client_create_connection_cannel();
            __gatt_client_event_callback_handler(GATT_COMM_EVENT_CREAT_CONN_TIMEOUT, 0, 0, 0);
            __gatt_client_check_auto_scan();
        }
    }
}

/*************************************************************************************************/
/*!
 *  \brief      timeout delete
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void __gatt_client_timeout_del(void)
{
    if (__this->client_timeout_id) {
        sys_timeout_del(__this->client_timeout_id);
        __this->client_timeout_id = 0;
    }
}

/*************************************************************************************************/
/*!
 *  \brief      timeout add
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void __gatt_client_timeout_add(int type_id, u32 set_ms)
{
    if (__this->client_timeout_id) {
        sys_timeout_del(__this->client_timeout_id);
    }
    __this->client_timeout_id = sys_timeout_add((void *)type_id, __gatt_client_timeout_handler, set_ms);
}

/*************************************************************************************************/
/*!
 *  \brief      协议栈发包完成事件
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note      可用于触发上层往协议栈发送数据
 */
/*************************************************************************************************/
static void __gatt_client_can_send_now_wakeup(void)
{
    /* log_info("can_send"); */
    __gatt_client_event_callback_handler(GATT_COMM_EVENT_CAN_SEND_NOW, 0, 0, 0);
}

/*************************************************************************************************/
/*!
 *  \brief      获取未连接状态
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
ble_state_e ble_gatt_client_get_work_state(void)
{
    return __this->client_work_state;
}

/*************************************************************************************************/
/*!
 *  \brief      获取已建立链路状态
 *
 *  \param      [in]   conn_handle
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
ble_state_e ble_gatt_client_get_connect_state(u16 conn_handle)
{
    if (!conn_handle) {
        return BLE_ST_NULL;
    }
    return ble_comm_dev_get_handle_state(conn_handle, GATT_ROLE_CLIENT);
}

/*************************************************************************************************/
/*!
 *  \brief      处理从机连接参数的情况，可以拒绝或接受
 *
 *  \param      [in]
 *
 *  \return    0--accept,1--reject
 *
 *  \note      可以指定接受后使用的参数
 */
/*************************************************************************************************/
//协议栈内部调用
int l2cap_connection_update_request_just(u8 *packet, hci_con_handle_t handle)
{
    log_info("slave request conn_update:\n-conn_handle= %04x\n-interval_min= %d,\n-interval_max= %d,\n-latency= %d,\n-timeout= %d\n",
             handle,
             little_endian_read_16(packet, 0), little_endian_read_16(packet, 2),
             little_endian_read_16(packet, 4), little_endian_read_16(packet, 6));

#if (SUPPORT_MAX_GATT_CLIENT > 1)
    u16 cfg_interval = BASE_INTERVAL_VALUE;
    u16 max_interval = little_endian_read_16(packet, 2);
    while (max_interval >= (cfg_interval + BASE_INTERVAL_VALUE)) {
        cfg_interval += BASE_INTERVAL_VALUE;
    }
    little_endian_store_16(packet, 0, cfg_interval);
    little_endian_store_16(packet, 2, cfg_interval);
    r_printf("conn_handle:%04x,confirm_interval:%d", handle, cfg_interval);
    log_info("conn_handle:%04x,confirm_interval:%d\n", handle, cfg_interval);
#endif

    if (little_endian_read_16(packet, 0) > little_endian_read_16(packet, 2)) {
        log_info("interval error,reject!!!\n");
        return 1;
    }

    if (config_vendor_le_bb & VENDOR_BB_CONNECT_SLOT) {
        if (little_endian_read_16(packet, 0) < 2) {
            log_info("interval_625 error,reject!!!\n");
            return 1;
        }
    } else {
        if (little_endian_read_16(packet, 0) < 6) {
            log_info("interval_1250 error,reject!!!\n");
            return 1;
        }
    }

    //change param
    /* little_endian_store_16(packet, 4,0);//disable latency */
    /* little_endian_store_16(packet, 6,400);//change timeout */
    return !__this->scan_conn_config->conn_update_accept;
    /* return 1; */
}


/*************************************************************************************************/
/*!
 *  \brief      接收server段的数据发送
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note      read动作返回数据、notify 或indicate 通知数据
 */
/*************************************************************************************************/
//协议栈内部调用
void user_client_report_data_callback(att_data_report_t *report_data)
{
    if (report_data->conn_handle != __this->just_search_handle) {
        if (INVAIL_INDEX == ble_comm_dev_get_index(report_data->conn_handle, GATT_ROLE_CLIENT)) {
            log_info("unknown_handle:%04x,drop data\n", report_data->conn_handle);
            return;
        }
    }

    /* log_info("data_report:hdl=%04x,pk_type=%02x,size=%d\n", report_data->conn_handle, report_data->packet_type, report_data->blob_length); */
    __gatt_client_event_callback_handler(GATT_COMM_EVENT_GATT_DATA_REPORT, report_data, sizeof(att_data_report_t), 0);
}

/*************************************************************************************************/
/*!
 *  \brief      搜索完profile，跟进配置处理搜索到的handle，使能通知等操作
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note       操作handle，完成 write ccc
 */
/*************************************************************************************************/
static void __do_operate_search_handle(void)
{
    u16 tmp_16, i, cur_opt_type;
    opt_handle_t *opt_hdl_pt;

    if (!__this->gatt_search_config || 0 == __this->gatt_search_config->search_uuid_count) {
        return;
    }

    log_info("opt_handle_used_cnt= %d\n", __this->opt_handle_used_cnt);

    if (!__this->opt_handle_used_cnt) {
        return;
    }

    for (i = 0; i < __this->opt_handle_used_cnt; i++) {

        opt_hdl_pt = &__this->operate_handle_table[i];
        log_info("do opt:service_uuid16:%04x,charactc_uuid16:%04x\n", \
                 opt_hdl_pt->search_uuid->services_uuid16, opt_hdl_pt->search_uuid->characteristic_uuid16);
        cur_opt_type = opt_hdl_pt->search_uuid->opt_type;

        if (cur_opt_type & ATT_PROPERTY_NOTIFY) {
            if (__this->gatt_search_config->auto_enable_ccc) {
                tmp_16  = 0x0001;//fixed
                log_info("write_ntf_ccc:%04x\n", opt_hdl_pt->value_handle);
                ble_comm_att_send_data(__this->client_search_handle, opt_hdl_pt->value_handle + 1, &tmp_16, 2, ATT_OP_WRITE);
                if (0x2a4d == opt_hdl_pt->search_uuid->characteristic_uuid16 && opt_hdl_pt->search_uuid->read_report_reference) {
                    tmp_16  = 0x55A1;//fixed
                    log_info("read_desc01:%04x\n", opt_hdl_pt->value_handle + 1);
                    ble_comm_att_send_data(__this->client_search_handle, opt_hdl_pt->value_handle + 1, &tmp_16, 2, ATT_OP_READ);
                    log_info("read_desc02:%04x\n", opt_hdl_pt->value_handle + 2);
                    ble_comm_att_send_data(__this->client_search_handle, opt_hdl_pt->value_handle + 2, &tmp_16, 2, ATT_OP_READ);
                }
            }

        } else if (cur_opt_type & ATT_PROPERTY_INDICATE) {
            if (__this->gatt_search_config->auto_enable_ccc) {
                tmp_16  = 0x0002;//fixed
                log_info("write_ind_ccc:%04x\n", opt_hdl_pt->value_handle);
                ble_comm_att_send_data(__this->client_search_handle, opt_hdl_pt->value_handle + 1, &tmp_16, 2, ATT_OP_WRITE);
            }

        } else if (cur_opt_type & ATT_PROPERTY_READ) {
            if (opt_hdl_pt->search_uuid->read_long_enable) {
                tmp_16  = 0x55A2;//fixed
                log_info("read_long:%04x\n", opt_hdl_pt->value_handle);
                ble_comm_att_send_data(__this->client_search_handle, opt_hdl_pt->value_handle, &tmp_16, 2, ATT_OP_READ_LONG);
            } else {
                tmp_16  = 0x55A1;//fixed
                log_info("read:%04x\n", opt_hdl_pt->value_handle);
                ble_comm_att_send_data(__this->client_search_handle, opt_hdl_pt->value_handle, &tmp_16, 2, ATT_OP_READ);
            }
        } else {
            ;
        }
    }

}

/*************************************************************************************************/
/*!
 *  \brief      检查是否有匹配的uuid
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void __check_target_uuid_match(search_result_t *result_info)
{
    u32 i;
    target_uuid_t *t_uuid;

    for (i = 0; i < __this->gatt_search_config->search_uuid_count; i++) {
        t_uuid = &__this->gatt_search_config->search_uuid_group[i];
        if (result_info->services.uuid16) {
            if (result_info->services.uuid16 != t_uuid->services_uuid16) {
                /* log_info("b1"); */
                continue;
            }
        } else {
            if (memcmp(result_info->services.uuid128, t_uuid->services_uuid128, 16)) {
                /* log_info("b2"); */
                continue;
            }
        }

        if (result_info->characteristic.uuid16) {
            if (result_info->characteristic.uuid16 != t_uuid->characteristic_uuid16) {
                /* log_info("b3"); */
                /* log_info("%d: %04x--%04x",result_info->characteristic.uuid16,t_uuid->characteristic_uuid16); */
                continue;
            }
        } else {
            if (memcmp(result_info->characteristic.uuid128, t_uuid->characteristic_uuid128, 16)) {
                /* log_info("b4"); */
                continue;
            }
        }

        break;//match one
    }

    if (i >= __this->gatt_search_config->search_uuid_count) {
        return;
    }

    if (__this->opt_handle_used_cnt >= SUPPORT_OPT_HANDLE_MAX) {
        log_info("opt_handle is full!!!\n");
        return;
    }

    if ((t_uuid->opt_type & result_info->characteristic.properties) != t_uuid->opt_type) {
        log_info("properties not match!!!\n");
        return;
    }

    log_info("match one uuid\n");

    opt_handle_t *opt_get = &__this->operate_handle_table[__this->opt_handle_used_cnt++];
    opt_get->value_handle = result_info->characteristic.value_handle;
    opt_get->search_uuid = t_uuid;

    switch (t_uuid->opt_type) {
    case ATT_PROPERTY_READ:
    case ATT_PROPERTY_WRITE_WITHOUT_RESPONSE:
    case ATT_PROPERTY_WRITE:
    case ATT_PROPERTY_NOTIFY:
    case ATT_PROPERTY_INDICATE:
        break;

    default:
        break;
    }

    __gatt_client_event_callback_handler(GATT_COMM_EVENT_GATT_SEARCH_MATCH_UUID, opt_get, sizeof(opt_handle_t), 0);

}

/*************************************************************************************************/
/*!
 *  \brief      协议栈回调搜索descriptor结果
 *
 *  \param      [in]    搜索结果
 *
 *  \return
 *
 *  \note      搜索charactc包含descriptor
 */
/*************************************************************************************************/
void user_client_report_descriptor_result(charact_descriptor_t *result_descriptor)
{
    log_info("report_descriptor,handle= %04x ,uuid16: %04x\n", result_descriptor->handle, result_descriptor->uuid16);
    if (result_descriptor->uuid16 == 0) {
        log_info("uuid128:");
        log_info_hexdump(result_descriptor->uuid128, 16);
    }

    __gatt_client_event_callback_handler(GATT_COMM_EVENT_GATT_SEARCH_DESCRIPTOR_RESULT, &__this->client_search_handle, 2, result_descriptor);
}

/*************************************************************************************************/
/*!
 *  \brief      协议栈回调搜索 service & charactc的结果
 *
 *  \param      [in]    搜索结果
 *
 *  \return
 *
 *  \note      每搜索到一个server 或 charactc uuid 都会调用,直到搜索结束
 */
/*************************************************************************************************/
//协议栈内部调用
void user_client_report_search_result(search_result_t *result_info)
{
    if (result_info == (void *) - 1) {
        log_info("client_report_search_result finish!!!\n");
        ble_comm_dev_set_handle_state(__this->client_search_handle, GATT_ROLE_CLIENT, BLE_ST_SEARCH_COMPLETE);
        __do_operate_search_handle();
        __gatt_client_set_work_state(__this->client_search_handle, BLE_ST_SEARCH_COMPLETE, 1);
        __gatt_client_event_callback_handler(GATT_COMM_EVENT_GATT_SEARCH_PROFILE_COMPLETE, &__this->client_search_handle, 2, 0);

        __this->client_search_handle = 0;//clear handle

        //搜索完profile,多机应用会触发尝试开新设备scan
        if (SUPPORT_MAX_GATT_CLIENT > 1) {
            __gatt_client_check_auto_scan();
        }
        return;
    }

    log_info("\n*** services, uuid16:%04x,index=%d ***\n", result_info->services.uuid16, result_info->service_index);
    log_info("{charactc, uuid16:%04x,index=%d,handle:%04x~%04x,value_handle=%04x}\n",
             result_info->characteristic.uuid16, result_info->characteristic_index,
             result_info->characteristic.start_handle, result_info->characteristic.end_handle,
             result_info->characteristic.value_handle
            );

    if (!result_info->services.uuid16) {
        log_info("######services_uuid128:");
        log_info_hexdump(result_info->services.uuid128, 16);
    }

    if (!result_info->characteristic.uuid16) {
        log_info("######charact_uuid128:");
        log_info_hexdump(result_info->characteristic.uuid128, 16);
    }

    __check_target_uuid_match(result_info);
}

/*************************************************************************************************/
/*!
 *  \brief      启动profile搜索
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void __gatt_client_search_profile_start(void)
{
    if (!__this->client_search_handle) {
        log_error("search_profile_fail:%04x\n", __this->client_search_handle);
        return;
    }

    __gatt_client_event_callback_handler(GATT_COMM_EVENT_GATT_SEARCH_PROFILE_START, &__this->client_search_handle, 2, 0);

    user_client_init(__this->client_search_handle, search_ram_buffer, SEARCH_PROFILE_BUFSIZE);
    __this->opt_handle_used_cnt = 0;

    if (!__this->gatt_search_config || 0 == __this->gatt_search_config->search_uuid_count) {
        log_info("skip search_profile:%04x\n\n", __this->client_search_handle);
        user_client_set_search_complete();
    } else {
        log_info("start search_profile_all:%04x\n", __this->client_search_handle);
        ble_op_search_profile_all();
    }
}

/*************************************************************************************************/
/*!
 *  \brief      检查是否配置自动开启scan
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void __gatt_client_check_auto_scan(void)
{
    if (__this->scan_conn_config->scan_auto_do) {
        ble_gatt_client_scan_enable(1);
    }
}

/*************************************************************************************************/
/*!
 *  \brief       检查是否支持新设备open scan
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static bool __gatt_client_just_new_dev_scan(void)
{
    log_info("%s\n", __FUNCTION__);
    u8 state = ble_gatt_client_get_work_state();

    switch (state) {
    case BLE_ST_IDLE:
    case BLE_ST_INIT_OK:
    case BLE_ST_DISCONN:
    case BLE_ST_CONNECT_FAIL:
    case BLE_ST_SEND_CREATE_CONN_CANNEL:
        break;
    default:
        log_info("dev_doing,%02x\n", state);
        return false;
        break;
    }

    s8 tmp_cid = ble_comm_dev_get_idle_index(GATT_ROLE_CLIENT);
    if (tmp_cid == INVAIL_INDEX) {
        log_info("no idle dev to do!!!\n");
        return false;
    }

    log_info("new_dev_scan\n");
    return true;
}

/*************************************************************************************************/
/*!
 *  \brief      打开设备scan
 *
 *  \param      [in]
 *
 *  \return     gatt_op_ret_e
 *
 *  \note
 */
/*************************************************************************************************/
int ble_gatt_client_scan_enable(u32 en)
{
    ble_state_e next_state, cur_state;

    if (!__this->scan_ctrl_en && en) {
        return 	GATT_CMD_OPT_FAIL;
    }


    if (en) {
        if (!__gatt_client_just_new_dev_scan()) {
            return 	GATT_CMD_OPT_FAIL;
        }
        next_state = BLE_ST_SCAN;
    } else {
        next_state = BLE_ST_IDLE;
    }

    cur_state =  ble_gatt_client_get_work_state();

    switch (cur_state) {
    case BLE_ST_SCAN:
    case BLE_ST_IDLE:
    case BLE_ST_INIT_OK:
    case BLE_ST_NULL:
    case BLE_ST_DISCONN:
    case BLE_ST_CONNECT_FAIL:
    case BLE_ST_SEND_CREATE_CONN_CANNEL:
        break;
    default:
        return 	GATT_CMD_OPT_FAIL;
        break;
    }

    if (cur_state == next_state) {
        return GATT_OP_RET_SUCESS;
    }

    __gatt_client_set_work_state(INVAIL_CONN_HANDLE, next_state, 1);

#if EXT_ADV_MODE_EN || PERIODIC_ADV_MODE_EN
    if (en) {
        ble_op_set_ext_scan_param(&ext_scan_param, sizeof(ext_scan_param));
        ble_op_ext_scan_enable(&ext_scan_enable, sizeof(ext_scan_enable));
    } else {
        ble_op_ext_scan_enable(&ext_scan_disable, sizeof(ext_scan_disable));
    }
#else
    if (en) {

        if (__this->scan_conn_config->set_local_addr_tag == USE_SET_LOCAL_ADDRESS_TAG) {
            le_controller_set_mac(&__this->scan_conn_config->local_address_info[1]);
        }

        log_info("scan_param: %d-%d-%d\n", \
                 __this->scan_conn_config->scan_type, __this->scan_conn_config->scan_interval, __this->scan_conn_config->scan_window);
        ble_op_set_scan_param(__this->scan_conn_config->scan_type, __this->scan_conn_config->scan_interval, __this->scan_conn_config->scan_window);
    }
    log_info("scan_en:%d\n", en);
    ble_op_scan_enable2(en, __this->scan_conn_config->scan_filter);
#endif /* EXT_ADV_MODE_EN */

    return GATT_OP_RET_SUCESS;
}

/*************************************************************************************************/
/*!
 *  \brief      检查是否有匹配scan配置的设备
 *
 *  \param      [in]
 *
 *  \return     true or false
 *
 *  \note
 */
/*************************************************************************************************/
static u8 device_match_index;
static bool __check_device_is_match(u8 event_type, u8 info_type, u8 *data, int size, client_match_cfg_t **output_match_devices)
{
    int i;
    u8  conn_mode = BIT(info_type);
    client_match_cfg_t *cfg;

    if (!__this->gatt_search_config || !__this->gatt_search_config->match_devices_count) {
        return false;
    }

    for (i = 0; i < __this->gatt_search_config->match_devices_count; i++) {
        cfg = &__this->gatt_search_config->match_devices[i];
        if (cfg == NULL) {
            continue;
        }

        if (0 != (cfg->filter_pdu_bitmap & BIT(event_type))) {
            //putchar('^');
            continue;//drop
        }

        /* log_info("cfg = %08x\n",cfg);	 */
        /* log_info_hexdump(cfg,sizeof(client_match_cfg_t)); */
        if (cfg->create_conn_mode == conn_mode && size == cfg->compare_data_len) {
            /* log_info("dev check\n"); */
            /* log_info_hexdump(data, size); */
            /* log_info_hexdump(cfg->compare_data, size); */
            if (0 == memcmp(data, cfg->compare_data, cfg->compare_data_len)) {
                log_info("match ok:%d\n", cfg->bonding_flag);
                *output_match_devices = cfg;
                device_match_index = i;
                return true;
            }
        }
    }
    return false;
}

/*************************************************************************************************/
/*!
 *  \brief      解析scan到的adv&rsp包数据
 *
 *  \param      [in]
 *
 *  \return     是否有匹配的设备, true or false
 *
 *  \note
 */
/*************************************************************************************************/
static bool __resolve_adv_report(adv_report_t *report_pt, u16 len)
{
    u8 i, length, ad_type;
    u8 *adv_data_pt;
    u8 find_remoter = 0;
    u32 tmp32;
    client_match_cfg_t *match_cfg = NULL;

    if (__check_device_is_match(report_pt->event_type, CLI_CREAT_BY_ADDRESS, report_pt->address, 6, &match_cfg)) {
        find_remoter = 1;
        log_info("catch mac ok\n");
        goto just_creat;
    }

    adv_data_pt = report_pt->data;
    for (i = 0; i < report_pt->length;) {
        if (*adv_data_pt == 0) {
            /* log_info("analyze end\n"); */
            break;
        }

        length = *adv_data_pt++;

        if (length >= report_pt->length || (length + i) >= report_pt->length) {
            /*过滤非标准包格式*/
            printf("!!!error_adv_packet:");
            put_buf(report_pt->data, report_pt->length);
            break;
        }

        ad_type = *adv_data_pt++;
        i += (length + 1);

        switch (ad_type) {
        case HCI_EIR_DATATYPE_FLAGS:
            /* log_info("flags:%02x\n",adv_data_pt[0]); */
            break;

        case HCI_EIR_DATATYPE_MORE_16BIT_SERVICE_UUIDS:
        case HCI_EIR_DATATYPE_COMPLETE_16BIT_SERVICE_UUIDS:
        case HCI_EIR_DATATYPE_MORE_32BIT_SERVICE_UUIDS:
        case HCI_EIR_DATATYPE_COMPLETE_32BIT_SERVICE_UUIDS:
        case HCI_EIR_DATATYPE_MORE_128BIT_SERVICE_UUIDS:
        case HCI_EIR_DATATYPE_COMPLETE_128BIT_SERVICE_UUIDS:
            /* log_info("service uuid:"); */
            /* log_info_hexdump(adv_data_pt, length - 1); */
            break;

        case HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME:
        case HCI_EIR_DATATYPE_SHORTENED_LOCAL_NAME:
            tmp32 = adv_data_pt[length - 1];
            adv_data_pt[length - 1] = 0;;
            log_info("remoter_name:%s ,rssi:%d\n", adv_data_pt, report_pt->rssi);
            log_info_hexdump(report_pt->address, 6);
            adv_data_pt[length - 1] = tmp32;

            //---------------------------------
#if SUPPORT_TEST_BOX_BLE_MASTER_TEST_EN
#define TEST_BOX_BLE_NAME		"JLBT_TESTBOX"
#define TEST_BOX_BLE_NAME_LEN	0xc
            if (0 == memcmp(adv_data_pt, TEST_BOX_BLE_NAME, TEST_BOX_BLE_NAME_LEN)) {
                log_info("catch TEST_BOX ok\n");
                find_remoter = 2;
                break;
            }
#endif
            //--------------------------------

            if (__check_device_is_match(report_pt->event_type, CLI_CREAT_BY_NAME, adv_data_pt, length - 1, &match_cfg)) {
                find_remoter = 1;
                log_info("catch name ok\n");
            }
            break;

        case HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA:
            if (__check_device_is_match(report_pt->event_type, CLI_CREAT_BY_TAG, adv_data_pt, length - 1, &match_cfg)) {
                log_info("get_tag_string!\n");
                find_remoter = 1;
            }
            break;

        case HCI_EIR_DATATYPE_APPEARANCE_DATA:
            /* log_info("get_class_type:%04x\n",little_endian_read_16(adv_data_pt,0)); */
            break;

        default:
            /* log_info("unknow ad_type:"); */
            break;
        }

        if (find_remoter) {
            log_info_hexdump(adv_data_pt, length - 1);
        }
        adv_data_pt += (length - 1);
    }

just_creat:
    if (find_remoter) {
        if (__this->gatt_search_config->match_rssi_enable && report_pt->rssi < __this->gatt_search_config->match_rssi_value) {
            log_info("rssi no match!!!\n");
            return 0;
        }

        u8 packet_data[10];
        packet_data[0] = report_pt->address_type;
        packet_data[7] = report_pt->rssi;
        packet_data[8] = find_remoter;
        packet_data[9] = device_match_index;
        memcpy(&packet_data[1], report_pt->address, 6);
        if (__gatt_client_event_callback_handler(GATT_COMM_EVENT_SCAN_DEV_MATCH, packet_data, 9, match_cfg)) {
            log_info("user can cannel to auto connect");
            find_remoter = 0;
        }
    }
    return find_remoter;
}

/*************************************************************************************************/
/*!
 *  \brief      建立指定设备连接创建
 *
 *  \param      [in]
 *
 *  \return     gatt_op_ret_e
 *
 *  \note
 */
/*************************************************************************************************/
static const struct create_conn_param_ext_t create_default_param_table = {
    .le_scan_interval = 24,
    .le_scan_window = 8,
    .initiator_filter_policy = 0,
    .peer_address_type = 0,
    .peer_address = {0, 0, 0, 0, 0, 0},
    .own_address_type = 0,
    .conn_interval_min = 8,
    .conn_interval_max = 32,
    .conn_latency = 0,
    .supervision_timeout = 200,
    .minimum_ce_length = 1,
    .maximum_ce_length = 1,
};

int ble_gatt_client_create_connection_request(u8 *address, u8 addr_type, int mode)
{
    u8 cur_state =  ble_gatt_client_get_work_state();

    log_info("===========create_connection_request\n");
    log_info("***remote type %d,addr:", addr_type);
    put_buf(address, 6);

    switch (cur_state) {
    case BLE_ST_SCAN:
    case BLE_ST_IDLE:
    case BLE_ST_INIT_OK:
    case BLE_ST_DISCONN:
    case BLE_ST_CONNECT_FAIL:
    case BLE_ST_SEND_CREATE_CONN_CANNEL:
        break;

    case BLE_ST_CREATE_CONN:
        if (CONFIG_BT_GATT_CLIENT_NUM == 1) {
            log_error("already create conn:%d!!!\n");
        } else if (CONFIG_BT_GATT_CLIENT_NUM > 1) {
#if RCSP_BTMATE_EN
            r_printf("To replace create conn!!");
            break;
#else
            r_printf("Please wait Please wait for the first connection to complete!!");
#endif
        }


    default:
        return GATT_CMD_PARAM_ERROR;
        break;
    }

    if (cur_state == BLE_ST_SCAN) {
        log_info("stop scan\n");
        ble_gatt_client_scan_enable(0);
    }

    int ret;

#if 0
    /*部分参数格式*/
    /* struct create_conn_param_t *create_conn_par = scan_buffer; */
    /* create_conn_par->conn_interval = __this->scan_conn_config->creat_conn_interval; */
    /* create_conn_par->conn_latency = __this->scan_conn_config->creat_conn_latency; */
    /* create_conn_par->supervision_timeout = __this->scan_conn_config->creat_conn_super_timeout; */
    /* memcpy(create_conn_par->peer_address, address, 6); */
    /* create_conn_par->peer_address_type = addr_type; */
    /* ret = ble_op_create_connection(create_conn_par); */
#else

#if EXT_ADV_MODE_EN
    struct __ext_init *create_conn_par = scan_buffer;
    memset(create_conn_par, 0, sizeof(*create_conn_par));
    create_conn_par->Conn_Interval_Min   = SET_EXT_CONN_INTERVAL;
    create_conn_par->Conn_Interval_Max   = SET_EXT_CONN_INTERVAL;
    create_conn_par->Conn_Latency        = SET_EXT_CONN_LATENCY;
    create_conn_par->Supervision_Timeout = SET_EXT_CONN_TIMEOUT;
    create_conn_par->Peer_Address_Type   = addr_type;
    create_conn_par->Initiating_PHYs     = INIT_SET_1M_PHY;
    create_conn_par->Scan_Window         = SET_EXT_SCAN_WINDOW;
    create_conn_par->Scan_Interval       = SET_EXT_SCAN_INTERVAL;
    memcpy(create_conn_par->Peer_Address, address, 6);

    log_info_hexdump(create_conn_par, sizeof(*create_conn_par));

    //printf("laowang3");
    ret = ble_op_ext_create_conn(create_conn_par, sizeof(*create_conn_par));
#else
    /*全参数格式*/
    struct create_conn_param_ext_t *create_conn_par = scan_buffer;
    memcpy(create_conn_par, &create_default_param_table, sizeof(struct create_conn_param_ext_t));

    create_conn_par->conn_interval_min = __this->scan_conn_config->creat_conn_interval;
    create_conn_par->conn_interval_max = __this->scan_conn_config->creat_conn_interval;
    create_conn_par->conn_latency = __this->scan_conn_config->creat_conn_latency;
    create_conn_par->supervision_timeout = __this->scan_conn_config->creat_conn_super_timeout;
    memcpy(create_conn_par->peer_address, address, 6);
    create_conn_par->peer_address_type = addr_type;
    ret = ble_op_create_connection_ext(create_conn_par);
#endif  //end of EXT_ADV_MODE_EN
#endif

    if (ret) {
        log_error("creat fail!!!\n");
    } else {
        __gatt_client_set_work_state(INVAIL_CONN_HANDLE, BLE_ST_CREATE_CONN, 1);
        if (__this->scan_conn_config->creat_state_timeout_ms) {
            __gatt_client_timeout_add(TO_TYPE_CREAT_CONN, __this->scan_conn_config->creat_state_timeout_ms);
        }
    }
    return ret;
}

/*************************************************************************************************/
/*!
 *  \brief      取消连接建立
 *
 *  \param      [in]
 *
 *  \return     gatt_op_ret_e
 *
 *  \note
 */
/*************************************************************************************************/
int ble_gatt_client_create_connection_cannel(void)
{
    if (ble_gatt_client_get_work_state() == BLE_ST_CREATE_CONN) {
        __gatt_client_set_work_state(INVAIL_CONN_HANDLE, BLE_ST_SEND_CREATE_CONN_CANNEL, 1);
        return ble_op_create_connection_cancel();
    }
    return GATT_CMD_OPT_FAIL;
}

/*************************************************************************************************/
/*!
 *  \brief      解析协议栈回调的scan到的adv&rsp 包
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void __gatt_client_report_adv_data(adv_report_t *report_pt, u16 len)
{
    u8 find_tag = 0;
    /* log_info("event_type,addr_type: %x,%x; ",report_pt->event_type,report_pt->address_type); */
    /* log_info_hexdump(report_pt->address,6); */
    /* log_info("adv_data_display:"); */
    /* log_info_hexdump(report_pt->data,report_pt->length); */
#if EXT_ADV_MODE_EN || PERIODIC_ADV_MODE_EN
    if (periodic_scan_state) {
        return;
    }
    le_ext_adv_report_evt_t *evt = (le_ext_adv_report_evt_t *)report_pt;
    find_tag = __resolve_ext_adv_report(evt, len);

#else

    if (!__this->gatt_search_config || !__this->gatt_search_config->match_devices_count) {
        /*没有加指定搜索,直接输出adv report*/
        putchar('~');
        __gatt_client_event_callback_handler(GATT_COMM_EVENT_SCAN_ADV_REPORT, report_pt, len, 0);
        return;
    }

    find_tag = __resolve_adv_report(report_pt, len);

#endif

#if PERIODIC_ADV_MODE_EN
    /*周期广播，不支持连接*/
    u8 adv_sid = evt->RSSI;
    u8 *address = evt->Address;
    u8 address_type = evt->Address_Type;
    u16 periodic_adv_interval = evt->Periodic_Advertising_Interval;

    if (periodic_adv_interval) {
        log_info("\n********* Scan AUX_ADV_IND with periodic info ***********\n");
        if (CUR_ADVERTISING_SID != adv_sid) {
            log_info("Current ADV_SID is not target :%d %d \n", CUR_ADVERTISING_SID, adv_sid);
            return;
        }
        log_info("***remote type %d, addr:", address_type);
        log_info_hexdump(address, 6);

        periodic_creat_sync.Advertising_Address_Type = address_type;
        memcpy(periodic_creat_sync.Advertiser_Address, address, 6);

        ble_user_cmd_prepare(BLE_CMD_PERIODIC_ADV_CREAT_SYNC, 2,
                             &periodic_creat_sync, sizeof(periodic_creat_sync));

        periodic_scan_state = 1;
    }

#else

    if (find_tag && __this->scan_conn_config && __this->scan_conn_config->creat_auto_do) {
        ble_gatt_client_scan_enable(0);
#if EXT_ADV_MODE_EN
        if (ble_gatt_client_create_connection_request(evt->Address, evt->Address_Type, 0))
#else
        if (ble_gatt_client_create_connection_request(report_pt->address, report_pt->address_type, 0))
#endif      //endif EXT_ADV_MODE_EN
        {

            log_info("creat fail,scan again!!!\n");
            ble_gatt_client_scan_enable(1);
        }
    }

#endif     //endif PERIOD_SCAN_EN
}

/*************************************************************************************************/
/*!
 *  \brief      配对加密key输入控制
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_gatt_client_passkey_input(u32 *key, u16 conn_handle)
{
    u16 tmp_val[3];
    *key = 123456; //default key
    tmp_val[0] = conn_handle;
    little_endian_store_32(&tmp_val, 2, (u32)key);
    __gatt_client_event_callback_handler(GATT_COMM_EVENT_SM_PASSKEY_INPUT, tmp_val, 6, 0);
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
void ble_gatt_client_sm_packet(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    sm_just_event_t *event = (void *)packet;
    u32 tmp32;
    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
        case SM_EVENT_JUST_WORKS_REQUEST:
            //发送接受配对命令sm_just_works_confirm,否则不发
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            log_info("first pair, %04x->Just Works Confirmed.\n", event->con_handle);
            __this->client_encrypt_process = LINK_ENCRYPTION_PAIR_JUST_WORKS;
            __gatt_client_event_callback_handler(GATT_COMM_EVENT_ENCRYPTION_REQUEST, &event->con_handle, 2, &__this->client_encrypt_process);
            break;

        case SM_EVENT_PASSKEY_DISPLAY_NUMBER:
            log_info_hexdump(packet, size);
            memcpy(&tmp32, event->data, 4);
            log_info("%04x->Passkey display: %06u.\n", event->con_handle, tmp32);
            __gatt_client_event_callback_handler(GATT_COMM_EVENT_ENCRYPTION_REQUEST, &event->con_handle, 2, &__this->client_encrypt_process);
            break;

        case SM_EVENT_PASSKEY_INPUT_NUMBER:
            /*IO_CAPABILITY_KEYBOARD_ONLY 方式*/
            __gatt_client_event_callback_handler(GATT_COMM_EVENT_ENCRYPTION_REQUEST, &event->con_handle, 2, &__this->client_encrypt_process);
            ble_gatt_client_passkey_input(&tmp32, event->con_handle);
            log_info("%04x->Passkey input: %06u.\n", event->con_handle, tmp32);
            sm_passkey_input(event->con_handle, tmp32); /*update passkey*/
            break;

        case SM_EVENT_PAIR_PROCESS:
            log_info("%04x->===Pair_process,sub= %02x\n", event->con_handle, event->data[0]);
            put_buf(event->data, 4);
            switch (event->data[0]) {
            case SM_EVENT_PAIR_SUB_RECONNECT_START:
                __this->client_encrypt_process = LINK_ENCRYPTION_RECONNECT;
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


static const char *const client_phy_result[] = {
    "None",
    "1M",
    "2M",
    "Coded",
};

/*************************************************************************************************/
/*!
 *  \brief      hci & gatt 事件处理
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_gatt_client_cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    u16 tmp_val[4];

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {

        case ATT_EVENT_CAN_SEND_NOW:
            __gatt_client_can_send_now_wakeup();
            break;

        case GAP_EVENT_ADVERTISING_REPORT:
            if (BLE_ST_SCAN == ble_gatt_client_get_work_state()) {
                putchar('V');
                __gatt_client_report_adv_data((void *)&packet[2], packet[1]);
            } else {
                log_info("drop scan_report!!!\n");
            }
            break;


        case HCI_EVENT_LE_META:
            switch (hci_event_le_meta_get_subevent_code(packet)) {

#if EXT_ADV_MODE_EN
            case HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE: {
                __gatt_client_timeout_del();
                if (BT_OP_SUCCESS != packet[3]) {
                    log_info("LE_MASTER CONNECTION FAIL!!! %0x\n", packet[3]);
                    __gatt_client_set_work_state(INVAIL_CONN_HANDLE, BLE_ST_IDLE, 1);
                    __gatt_client_check_auto_scan();
                } else {
                    tmp_val[0] = hci_subevent_le_enhanced_connection_complete_get_connection_handle(packet);
                    log_info("HCI_SUBEVENT_LE_ENHANCED_CONNECTION_COMPLETE : %0x\n", tmp_val[0]);
                    log_info("conn_interval = %d\n", hci_subevent_le_enhanced_connection_complete_get_conn_interval(packet));
                    log_info("conn_latency = %d\n", hci_subevent_le_enhanced_connection_complete_get_conn_latency(packet));
                    log_info("conn_timeout = %d\n", hci_subevent_le_enhanced_connection_complete_get_supervision_timeout(packet));

                    __this->client_search_handle = tmp_val[0];

                    ble_comm_dev_set_handle_state(__this->client_search_handle, GATT_ROLE_CLIENT, BLE_ST_CONNECT);
                    __this->client_encrypt_process = LINK_ENCRYPTION_NULL;
                    __gatt_client_set_work_state(INVAIL_CONN_HANDLE, BLE_ST_IDLE, 0);
                    __gatt_client_set_work_state(tmp_val[0], BLE_ST_CONNECT, 1);
                    __gatt_client_event_callback_handler(GATT_COMM_EVENT_CONNECTION_COMPLETE, tmp_val, 2, packet);
                    if (!ble_comm_need_wait_encryption(GATT_ROLE_CLIENT)) {
                        __gatt_client_search_profile_start();
                    }
                }
            }
            break;
#endif /*EXT_ADV_MODE_EN*/

#if EXT_ADV_MODE_EN || PERIODIC_ADV_MODE_EN
            case HCI_SUBEVENT_LE_EXTENDED_ADVERTISING_REPORT:
                if (BLE_ST_SCAN == ble_gatt_client_get_work_state()) {
                    putchar('v');
                    __gatt_client_report_adv_data(&packet[2], packet[1]);
                } else {
                    log_info("drop ext_adv_report!!!\n");
                }
                break;
#endif /*EXT_ADV_MODE_EN || PERIODIC_ADV_MODE_EN*/

#if PERIODIC_ADV_MODE_EN
            case HCI_SUBEVENT_LE_PERIODIC_ADVERTISING_REPORT:
                log_info("APP HCI_SUBEVENT_LE_PERIODIC_ADVERTISING_REPORT");
                client_report_periodic_adv_data(&packet[2], packet[1]);
                break;

            case HCI_SUBEVENT_LE_PERIODIC_ADVERTISING_SYNC_ESTABLISHED:
                u16 sync_handle = little_endian_read_16(packet, 4);
                log_info("APP HCI_SUBEVENT_LE_PERIODIC_ADVERTISING_SYNC_ESTABLISHED :0x%x 0x%x", packet[3], sync_handle);
                break;

            case HCI_SUBEVENT_LE_PERIODIC_ADVERTISING_SYNC_LOST:
                sync_handle = little_endian_read_16(packet, 3);
                log_info("APP HCI_SUBEVENT_LE_PERIODIC_ADVERTISING_SYNC_LOST sync_handle:0x%x", sync_handle);
                break;
#endif /*PERIODIC_ADV_MODE_EN*/

            case HCI_SUBEVENT_LE_CONNECTION_COMPLETE: {
                __gatt_client_timeout_del();
                if (BT_OP_SUCCESS != packet[3]) {
                    log_info("LE_MASTER CONNECTION FAIL!!! %0x\n", packet[3]);
                    __gatt_client_set_work_state(INVAIL_CONN_HANDLE, BLE_ST_IDLE, 1);
                    __gatt_client_check_auto_scan();
                    break;
                }

                tmp_val[0] = hci_subevent_le_connection_complete_get_connection_handle(packet);
                log_info("HCI_SUBEVENT_LE_CONNECTION_COMPLETE:conn_handle= %04x,rssi= %d\n", tmp_val[0], ble_vendor_get_peer_rssi(tmp_val[0]));
                log_info("conn_interval = %d\n", hci_subevent_le_connection_complete_get_conn_interval(packet));
                log_info("conn_latency = %d\n", hci_subevent_le_connection_complete_get_conn_latency(packet));
                log_info("conn_timeout = %d\n", hci_subevent_le_connection_complete_get_supervision_timeout(packet));

                __this->client_search_handle = tmp_val[0];
                ble_comm_dev_set_handle_state(__this->client_search_handle, GATT_ROLE_CLIENT, BLE_ST_CONNECT);
                __this->client_encrypt_process = LINK_ENCRYPTION_NULL;
                __gatt_client_set_work_state(INVAIL_CONN_HANDLE, BLE_ST_IDLE, 0);
                __gatt_client_set_work_state(tmp_val[0], BLE_ST_CONNECT, 1);
                __gatt_client_event_callback_handler(GATT_COMM_EVENT_CONNECTION_COMPLETE, tmp_val, 2, packet);

                if (!ble_comm_need_wait_encryption(GATT_ROLE_CLIENT)) {
                    __gatt_client_search_profile_start();
                }

            }
            break;

            case HCI_SUBEVENT_LE_CONNECTION_UPDATE_COMPLETE: {
                tmp_val[0] = little_endian_read_16(packet, 4);
                if (__this->client_operation_handle == tmp_val[0]) {
                    __this->client_operation_handle = 0;
                }

                log_info("conn_update_handle   = %04x\n", tmp_val[0]);
                log_info("conn_update_interval = %d\n", hci_subevent_le_connection_update_complete_get_conn_interval(packet));
                log_info("conn_update_latency  = %d\n", hci_subevent_le_connection_update_complete_get_conn_latency(packet));
                log_info("conn_update_timeout  = %d\n", hci_subevent_le_connection_update_complete_get_supervision_timeout(packet));
                __gatt_client_event_callback_handler(GATT_COMM_EVENT_CONNECTION_UPDATE_COMPLETE, tmp_val, 2, packet);

                if (!config_le_sm_support_enable) {
                    if (config_btctler_le_features & LE_DATA_PACKET_LENGTH_EXTENSION) {
                        log_info(">>>>>>>>s1--request DLE, %04x\n", tmp_val[0]);
                        ble_comm_set_connection_data_length(tmp_val[0], config_btctler_le_acl_packet_length, 2120);
                    } else if (config_btctler_le_features & LE_2M_PHY) {
                        log_info(">>>>>>>>s1--request 2M, %04x\n", tmp_val[0]);
                        ble_comm_set_connection_data_phy(tmp_val[0], CONN_SET_2M_PHY, CONN_SET_2M_PHY, CONN_SET_PHY_OPTIONS_NONE);
                    } else if (config_btctler_le_features & LE_CODED_PHY) {
                        if (config_btctler_coded_type == CONN_SET_PHY_OPTIONS_S2) {
                            log_info(">>>>>>>>s1--request CODED S2, %04x\n", tmp_val[0]);
                            ble_comm_set_connection_data_phy(tmp_val[0], CONN_SET_CODED_PHY, CONN_SET_CODED_PHY, CONN_SET_PHY_OPTIONS_S2);
                        } else {
                            log_info(">>>>>>>>s1--request CODED S8, %04x\n", tmp_val[0]);
                            ble_comm_set_connection_data_phy(tmp_val[0], CONN_SET_CODED_PHY, CONN_SET_CODED_PHY, CONN_SET_PHY_OPTIONS_S8);
                        }
                    }
                }
            }
            break;

            case HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE:
                tmp_val[0] = little_endian_read_16(packet, 3);
                log_info("conn_handle = %04x\n", tmp_val[0]);
                log_info("APP HCI_SUBEVENT_LE_DATA_LENGTH_CHANGE\n");
                log_info("TX_Octets:%d, Time:%d\n", little_endian_read_16(packet, 5), little_endian_read_16(packet, 7));
                log_info("RX_Octets:%d, Time:%d\n", little_endian_read_16(packet, 9), little_endian_read_16(packet, 11));

                if (config_btctler_le_features & LE_2M_PHY) {
                    log_info(">>>>>>>>s3--request 2M, %04x\n", tmp_val[0]);
                    ble_comm_set_connection_data_phy(tmp_val[0], CONN_SET_2M_PHY, CONN_SET_2M_PHY, CONN_SET_PHY_OPTIONS_NONE);
                } else if (config_btctler_le_features & LE_CODED_PHY) {
                    if (config_btctler_coded_type == CONN_SET_PHY_OPTIONS_S2) {
                        log_info(">>>>>>>>s3--request CODED S2, %04x\n", tmp_val[0]);
                        ble_comm_set_connection_data_phy(tmp_val[0], CONN_SET_CODED_PHY, CONN_SET_CODED_PHY, CONN_SET_PHY_OPTIONS_S2);
                    } else {
                        log_info(">>>>>>>>s3--request CODED S8, %04x\n", tmp_val[0]);
                        ble_comm_set_connection_data_phy(tmp_val[0], CONN_SET_CODED_PHY, CONN_SET_CODED_PHY, CONN_SET_PHY_OPTIONS_S8);
                    }
                }

                __gatt_client_event_callback_handler(GATT_COMM_EVENT_CONNECTION_DATA_LENGTH_CHANGE, tmp_val, 2, packet);
                break;

            case HCI_SUBEVENT_LE_PHY_UPDATE_COMPLETE:
                tmp_val[0] = little_endian_read_16(packet, 3);
                log_info("conn_handle = %04x\n", little_endian_read_16(packet, 3));
                log_info("APP HCI_SUBEVENT_LE_PHY_UPDATE %s\n", hci_event_le_meta_get_phy_update_complete_status(packet) ? "Fail" : "Succ");
                log_info("Tx PHY: %s\n", client_phy_result[hci_event_le_meta_get_phy_update_complete_tx_phy(packet)]);
                log_info("Rx PHY: %s\n", client_phy_result[hci_event_le_meta_get_phy_update_complete_rx_phy(packet)]);
                __gatt_client_event_callback_handler(GATT_COMM_EVENT_CONNECTION_PHY_UPDATE_COMPLETE, tmp_val, 2, packet);
                break;
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE: {
            tmp_val[0] = little_endian_read_16(packet, 3);
            tmp_val[1] = packet[5];
            if (__this->client_operation_handle == tmp_val[0]) {
                __this->client_operation_handle = 0;
            }
            log_info("HCI_EVENT_DISCONNECTION_COMPLETE:conn_handle= %04x, reason= %02x\n", tmp_val[0], packet[5]);
            ble_comm_dev_set_handle_state(tmp_val[0], GATT_ROLE_CLIENT, BLE_ST_DISCONN);
            __gatt_client_set_work_state(tmp_val[0], BLE_ST_DISCONN, 1);
            __gatt_client_event_callback_handler(GATT_COMM_EVENT_DISCONNECT_COMPLETE, tmp_val, 4, packet);
            __gatt_client_check_auto_scan();
        }
        break;

        case ATT_EVENT_MTU_EXCHANGE_COMPLETE: {
            tmp_val[0] = little_endian_read_16(packet, 2);
            tmp_val[1] = att_event_mtu_exchange_complete_get_MTU(packet);
            log_info("handle= %02x, ATT_MTU= %u,payload= %u\n", tmp_val[0], tmp_val[1], tmp_val[1] - 3);
            __gatt_client_event_callback_handler(GATT_COMM_EVENT_MTU_EXCHANGE_COMPLETE, tmp_val, 4, 0);
        }
        break;

        case HCI_EVENT_VENDOR_REMOTE_TEST:
            log_info("---HCI_EVENT_VENDOR_REMOTE_TEST\n");
            break;

        case HCI_EVENT_ENCRYPTION_CHANGE: {
            tmp_val[0] = little_endian_read_16(packet, 3);
            tmp_val[1] = packet[2] | (__this->client_encrypt_process << 8);
            log_info("HCI_EVENT_ENCRYPTION_CHANGE= %d,%04x\n", packet[2], tmp_val[0]);
            if (packet[2]) {
                log_info("Encryption fail!!!,%d,%04x\n", packet[2], tmp_val[0]);
            }
            __gatt_client_event_callback_handler(GATT_COMM_EVENT_ENCRYPTION_CHANGE, tmp_val, 4, 0);
            if (ble_comm_need_wait_encryption(GATT_ROLE_CLIENT)) {
                __gatt_client_search_profile_start();
            }

            if (config_le_sm_support_enable) {
                if (config_btctler_le_features & LE_DATA_PACKET_LENGTH_EXTENSION) {
                    log_info(">>>>>>>>s2--request DLE, %04x\n", tmp_val[0]);
                    ble_comm_set_connection_data_length(tmp_val[0], config_btctler_le_acl_packet_length, 2120);
                } else if (config_btctler_le_features & LE_2M_PHY) {
                    log_info(">>>>>>>>s2--request 2M, %04x\n", tmp_val[0]);
                    ble_comm_set_connection_data_phy(tmp_val[0], CONN_SET_2M_PHY, CONN_SET_2M_PHY, CONN_SET_PHY_OPTIONS_NONE);
                } else if (config_btctler_le_features & LE_CODED_PHY) {
                    if (config_btctler_coded_type == CONN_SET_PHY_OPTIONS_S2) {
                        log_info(">>>>>>>>s2--request CODED S2, %04x\n", tmp_val[0]);
                        ble_comm_set_connection_data_phy(tmp_val[0], CONN_SET_CODED_PHY, CONN_SET_CODED_PHY, CONN_SET_PHY_OPTIONS_S2);
                    } else {
                        log_info(">>>>>>>>s2--request CODED S8, %04x\n", tmp_val[0]);
                        ble_comm_set_connection_data_phy(tmp_val[0], CONN_SET_CODED_PHY, CONN_SET_CODED_PHY, CONN_SET_PHY_OPTIONS_S8);
                    }
                }
            }

        }
        break;
        }
        break;
    }
}

/*************************************************************************************************/
/*!
 *  \brief      更新连接参数
 *
 *  \param      [in]
 *
 *  \return     gatt_op_ret_e
 *
 *  \note
 */
/*************************************************************************************************/
int ble_gatt_client_connetion_update_set(u16 conn_handle, const struct conn_update_param_t *param)
{
    int ret;

    if (!conn_handle) {
        return GATT_CMD_PARAM_ERROR;
    }

    if (ble_comm_dev_get_handle_role(conn_handle) != GATT_ROLE_CLIENT) {
        log_info("connection_update busy!!!\n");
        return GATT_OP_ROLE_ERR;
    }

    if (__this->client_operation_handle) {
        log_info("update busy:%04x\n", __this->client_operation_handle);
        return GATT_CMD_RET_BUSY;
    }

    log_info("connection_update_set: %04x: -%d-%d-%d-%d-\n", conn_handle, \
             param->interval_min, param->interval_max, param->latency, param->timeout);
    ret = ble_op_conn_param_update(conn_handle, param);
    if (!ret) {
        __this->client_operation_handle = conn_handle;
    }
    return ret;
}

/*************************************************************************************************/
/*!
 *  \brief      模块开关使能
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note      会执行自动配置scan 打开
 */
/*************************************************************************************************/
void ble_gatt_client_module_enable(u8 en)
{
    log_info("mode_en:%d\n", en);
    if (en) {
        __this->scan_ctrl_en = 1;
        __gatt_client_check_auto_scan();
    } else {
        ble_gatt_client_scan_enable(0);
        __this->scan_ctrl_en = 0;
        ble_gatt_client_disconnect_all();
        __gatt_client_set_work_state(INVAIL_CONN_HANDLE, BLE_ST_IDLE, 0);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      断开所有的链路
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_gatt_client_disconnect_all(void)
{
    u8 i;
    u16 conn_handle;
    for (u8 i = 0; i < SUPPORT_MAX_GATT_CLIENT; i++) {
        conn_handle = ble_comm_dev_get_handle(i, GATT_ROLE_CLIENT);
        if (conn_handle) {
            ble_comm_disconnect(conn_handle);
            os_time_dly(1);
        }
    }
}

/*************************************************************************************************/
/*!
 *  \brief      配置scan，conn的参数
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note      没开启scan前，都可以配置
 */
/*************************************************************************************************/
void ble_gatt_client_set_scan_config(scan_conn_cfg_t *scan_conn_cfg)
{
    __this->scan_conn_config = scan_conn_cfg;
}

/*************************************************************************************************/
/*!
 *  \brief      配置scan匹配的设备 和 连接后搜索的profile
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note      没开启scan前，都可以配置
 */
/*************************************************************************************************/
void ble_gatt_client_set_search_config(gatt_search_cfg_t *gatt_search_cfg)
{
    __this->gatt_search_config = gatt_search_cfg;
}

/*************************************************************************************************/
/*!
 *  \brief      在已存在的从机链路上,发起搜索对方的的profile
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_gatt_just_search_profile_start(u16 conn_handle)
{
    __this->client_search_handle = conn_handle;
    __this->just_search_handle = conn_handle;
    __gatt_client_search_profile_start();
}

/*************************************************************************************************/
/*!
 *  \brief      只搜索链路已被释放，断开
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_gatt_just_search_profile_stop(u16 conn_handle)
{
    if (__this->just_search_handle == conn_handle) {
        __this->just_search_handle = 0;
    }
}



/*************************************************************************************************/
/*!
 *  \brief      gatt_client 协议栈初始化调用
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_gatt_client_profile_init(void)
{
    log_info("%s\n", __FUNCTION__);

    //setup GATT client
    gatt_client_init();
    __this->client_work_state = BLE_ST_INIT_OK;
}

/*************************************************************************************************/
/*!
 *  \brief      蓝牙协议栈初始化前调用
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_gatt_client_init(gatt_client_cfg_t *client_cfg)
{
    log_info("%s\n", __FUNCTION__);
    memset(__this, 0, sizeof(client_ctl_t));
    __this->client_config = client_cfg;
}

/*************************************************************************************************/
/*!
 *  \brief      gatt_client 模块退出
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void ble_gatt_client_exit(void)
{
    log_info("%s\n", __FUNCTION__);
    ble_gatt_client_module_enable(0);
}

#endif


