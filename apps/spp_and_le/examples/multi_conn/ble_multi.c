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
#include "ble_multi.h"

#if CONFIG_APP_MULTI

#if LE_DEBUG_PRINT_EN
#define log_info(x, ...)  printf("[MUL]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif


//ATT发送的包长,    note: 23 <=need >= MTU
#define ATT_LOCAL_MTU_SIZE        (517)

//ATT缓存的buffer支持缓存数据包个数
#define ATT_PACKET_NUMS_MAX       (2)

//ATT缓存的buffer大小,  note: need >= 23,可修改
#define ATT_SEND_CBUF_SIZE        (ATT_PACKET_NUMS_MAX * (ATT_PACKET_HEAD_SIZE + ATT_LOCAL_MTU_SIZE))

//------------------------------------------------------
extern const char *bt_get_local_name();
extern void clr_wdt(void);
//------------------------------------------------------
//输入passkey 加密
#define PASSKEY_ENABLE                     0

static const sm_cfg_t sm_init_config = {
    .master_security_auto_req = 1,
    .master_set_wait_security = 1,
    .slave_security_auto_req = 0,
    .slave_set_wait_security = 1,

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

extern const gatt_server_cfg_t mul_server_init_cfg;
extern const gatt_client_cfg_t mul_client_init_cfg;

//gatt 控制块初始化
static gatt_ctrl_t mul_gatt_control_block = {
    //public
    .mtu_size = ATT_LOCAL_MTU_SIZE,
    .cbuffer_size = ATT_SEND_CBUF_SIZE,
    .multi_dev_flag	= 1,

    //config
#if CONFIG_BT_GATT_SERVER_NUM
    .server_config = &mul_server_init_cfg,
#else
    .server_config = NULL,
#endif

#if CONFIG_BT_GATT_CLIENT_NUM
    .client_config = &mul_client_init_cfg,
#else
    .client_config = NULL,
#endif

#if CONFIG_BT_SM_SUPPORT_ENABLE
    .sm_config = &sm_init_config,
#else
    .sm_config = NULL,
#endif
    //cbk,event handle
    .hci_cb_packet_handler = NULL,
};

//-------------------------------------------------------------------------------------
//协议栈开始前初始化
void bt_ble_before_start_init(void)
{
    log_info("%s", __FUNCTION__);
    ble_comm_init(&mul_gatt_control_block);
}

//模块初始化
void bt_ble_init(void)
{
    log_info("%s\n", __FUNCTION__);
    log_info("ble_file: %s", __FILE__);
    ble_comm_set_config_name(bt_get_local_name(), 1);

#if CONFIG_BT_GATT_SERVER_NUM
    multi_server_init();
#endif

#if CONFIG_BT_GATT_CLIENT_NUM
    multi_client_init();
#endif

    ble_module_enable(1);
}

//模块退出
void bt_ble_exit(void)
{
    log_info("%s\n", __FUNCTION__);
    ble_module_enable(0);

    ble_comm_exit();

#if CONFIG_BT_GATT_SERVER_NUM
    multi_server_exit();
#endif

#if CONFIG_BT_GATT_CLIENT_NUM
    multi_client_exit();
#endif

}

//模块使能
void ble_module_enable(u8 en)
{
    ble_comm_module_enable(en);
}

#endif


