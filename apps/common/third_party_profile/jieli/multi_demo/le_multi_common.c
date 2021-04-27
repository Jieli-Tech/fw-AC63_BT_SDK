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
#include "3th_profile_api.h"

#include "le_common.h"

#include "rcsp_bluetooth.h"
#include "JL_rcsp_api.h"
#include "custom_cfg.h"
#include "btstack/btstack_event.h"
#include "le_multi_common.h"

#if (TCFG_BLE_DEMO_SELECT == DEF_BLE_DEMO_MULTI)

#if LE_DEBUG_PRINT_EN
extern void printf_buf(u8 *buf, u32 len);
//#define log_info            r_printf
#define log_info(x, ...)  printf("[LE-MUL-COMM]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  printf_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

//----------------------------------------------------------------------------------------
#define MULTI_ATT_MTU_SIZE       (512) //ATT MTU的值

//ATT发送的包长,    note: 20 <= need >= MTU
#define MULTI_ATT_LOCAL_PAYLOAD_SIZE    (MULTI_ATT_MTU_SIZE)                   //
//ATT缓存的buffer大小,  note: need >= 20,可修改
#define MULTI_ATT_SEND_CBUF_SIZE        (512*3)                 //

//共配置的RAM
#define MULTI_ATT_RAM_BUFSIZE           (ATT_CTRL_BLOCK_SIZE + MULTI_ATT_LOCAL_PAYLOAD_SIZE + MULTI_ATT_SEND_CBUF_SIZE)                   //note:
static u8 multi_att_ram_buffer[MULTI_ATT_RAM_BUFSIZE] __attribute__((aligned(4)));

u16 server_con_handle[SUPPORT_MAX_SERVER];
u16 client_con_handle[SUPPORT_MAX_CLIENT];

extern const int config_btctler_le_hw_nums;
//----------------------------------------------------------------------------------------
s8 mul_get_dev_index(u16 handle, u8 role)
{
    s8 i;
    u16 *group_handle;
    u8 count;

    if (!role) {
        group_handle = server_con_handle;
        count = SUPPORT_MAX_SERVER;
    } else {
        group_handle = client_con_handle;
        count = SUPPORT_MAX_CLIENT;
    }

    for (i = 0; i < count; i++) {
        if (handle == group_handle[i]) {
            return i;
        }
    }
    return -1;
}

s8 mul_get_idle_dev_index(u8 role)
{
    s8 i;
    u16 *group_handle;
    u8 count;

    if (!role) {
        group_handle = server_con_handle;
        count = SUPPORT_MAX_SERVER;
    } else {
        group_handle = client_con_handle;
        count = SUPPORT_MAX_CLIENT;
    }

    for (i = 0; i < count; i++) {
        if (0 == group_handle[i]) {
            return i;
        }
    }
    return -1;
}


s8 mul_del_dev_index(u16 handle, u8 role)
{
    s8 i;
    u16 *group_handle;
    u8 count;

    if (!role) {
        group_handle = server_con_handle;
        count = SUPPORT_MAX_SERVER;
    } else {
        group_handle = client_con_handle;
        count = SUPPORT_MAX_CLIENT;
    }


    for (i = 0; i < count; i++) {
        if (handle == group_handle[i]) {
            group_handle[i] = 0;
            return i;
        }
    }
    return -1;
}

bool mul_dev_have_connected(u8 role)
{
    s8 i;
    u16 *group_handle;
    u8 count;

    if (!role) {
        group_handle = server_con_handle;
        count = SUPPORT_MAX_SERVER;
    } else {
        group_handle = client_con_handle;
        count = SUPPORT_MAX_CLIENT;
    }

    for (i = 0; i < count; i++) {
        if (group_handle[i]) {
            return true;
        }
    }
    return false;
}

u16 mul_dev_get_conn_handle(u8 index, u8 role)
{
    u16 *group_handle;
    u8 count;

    if (!role) {
        group_handle = server_con_handle;
        count = SUPPORT_MAX_SERVER;
    } else {
        group_handle = client_con_handle;
        count = SUPPORT_MAX_CLIENT;
    }

    if (index < count) {
        return group_handle[index];
    } else {
        return 0;
    }
}


void cbk_sm_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    sm_just_event_t *event = (void *)packet;
    u32 tmp32;
    log_info("%s\n", __FUNCTION__);

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
        case SM_EVENT_JUST_WORKS_REQUEST:
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            log_info("Just Works Confirmed.\n");
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

/* LISTING_START(packetHandler): Packet Handler */
static void cbk_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
#if SUPPORT_MAX_SERVER
    trans_cbk_packet_handler(packet_type, channel, packet, size);
#endif

#if SUPPORT_MAX_CLIENT
    client_cbk_packet_handler(packet_type, channel, packet, size);
#endif
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
    log_info("set new_key= %06u\n", *key);
#else
    *key = 123456; //for debug
#endif
}

void ble_sm_setup_init(io_capability_t io_type, u8 auth_req, uint8_t min_key_size, u8 security_en)
{
    //setup SM: Display only
    log_info("%s\n", __FUNCTION__);
    sm_init();
    sm_set_io_capabilities(io_type);
    sm_set_authentication_requirements(auth_req);
    sm_set_encryption_key_size_range(min_key_size, 16);
    sm_set_request_security(security_en);
    sm_event_callback_set(&cbk_sm_packet_handler);

    if (io_type == IO_CAPABILITY_DISPLAY_ONLY) {
        reset_PK_cb_register(reset_passkey_cb);
    }
}

void ble_profile_init(void)
{
    log_info("%s\n", __FUNCTION__);

#if SUPPORT_MAX_SERVER && SUPPORT_MAX_CLIENT
    ble_stack_gatt_role(2);
#elif SUPPORT_MAX_CLIENT
    ble_stack_gatt_role(1);
#else
    ble_stack_gatt_role(0);
#endif

    ble_vendor_set_default_att_mtu(MULTI_ATT_MTU_SIZE);

    le_device_db_init();

#if PASSKEY_ENTER_ENABLE
    ble_sm_setup_init(IO_CAPABILITY_DISPLAY_ONLY, SM_AUTHREQ_MITM_PROTECTION, 7, TCFG_BLE_SECURITY_EN);
#else
    ble_sm_setup_init(IO_CAPABILITY_NO_INPUT_NO_OUTPUT, SM_AUTHREQ_BONDING, 7, TCFG_BLE_SECURITY_EN);
#endif

#if SUPPORT_MAX_SERVER
    server_profile_init();
#endif

#if SUPPORT_MAX_CLIENT
    client_profile_init();
#endif

    // register for HCI events
    hci_event_callback_set(&cbk_packet_handler);
    /* ble_l2cap_register_packet_handler(packet_cbk); */
    /* sm_event_packet_handler_register(packet_cbk); */
    le_l2cap_register_packet_handler(&cbk_packet_handler);

    ble_op_multi_att_send_init(multi_att_ram_buffer, MULTI_ATT_SEND_CBUF_SIZE, MULTI_ATT_LOCAL_PAYLOAD_SIZE);
}

//统一接口，关闭模块
void ble_module_enable(u8 en)
{
    log_info("mode_en:%d\n", en);

#if SUPPORT_MAX_SERVER
    ble_trans_module_enable(en);
#endif

#if SUPPORT_MAX_CLIENT
    ble_client_module_enable(en);
#endif

}

void ble_app_disconnect(void)
{
#if SUPPORT_MAX_SERVER
    ble_multi_trans_disconnect();
#endif

#if SUPPORT_MAX_CLIENT
    ble_multi_client_disconnect();
#endif


}


void bt_ble_init(void)
{
    log_info("%s\n", __FUNCTION__);

    if (SUPPORT_MAX_SERVER + SUPPORT_MAX_CLIENT > config_btctler_le_hw_nums) {
        ASSERT(0, "le_hw_nums not enough!!!\n");
        while (1);
    }

    if (SUPPORT_MAX_SERVER + SUPPORT_MAX_CLIENT > config_le_hci_connection_num
        || SUPPORT_MAX_SERVER > config_le_gatt_server_num
        || SUPPORT_MAX_CLIENT > config_le_gatt_client_num) {
        ASSERT(0, "btstack not enough!!!\n");
        while (1);
    }

    if (att_send_check_multi_dev(SUPPORT_MAX_SERVER, SUPPORT_MAX_CLIENT)) {
        ASSERT(0, "att_send not enough!!!\n");
        while (1);
    }

#if SUPPORT_MAX_SERVER
    bt_multi_trans_init();
#endif

#if SUPPORT_MAX_CLIENT
    bt_multi_client_init();
#endif
}

void bt_ble_exit(void)
{
    log_info("%s\n", __FUNCTION__);

#if SUPPORT_MAX_SERVER
    bt_multi_trans_exit();
#endif

#if SUPPORT_MAX_CLIENT
    bt_multi_client_exit();
#endif
}

#endif


