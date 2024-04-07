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
#include "at.h"
#include "spp_at_trans.h"
#include "ble_at_com.h"
#include "ble_at_client.h"
#include "app_power_manage.h"

#define LOG_TAG_CONST       AT_COM
#define LOG_TAG             "[AT_CMD]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


#if CONFIG_APP_AT_COM

static struct at_layer {
    void *config;     //ci transport config
    u8 *pAT_buffer;   //ci data path memory
    // transport component with configuration
    ci_transport_t *dev_transport;

};

#define AT_BUFFER_SIZE      0x100

#ifdef HAVE_MALLOC
static struct at_layer *hdl;

#define __this      (hdl)
#else
static struct at_layer hdl;

#define __this      (&hdl)

static u8 pAT_buffer_static[AT_BUFFER_SIZE];   //ci data path memory
#endif

static u8 respond_buffer_static[32];   //ci data path memory

void at_send_event(u8 opcode, const u8 *packet, int size);
/* extern void at_set_soft_poweroff(void); */
extern void atcom_power_event_to_user(u8 event);
extern void bt_max_pwr_set(u8 pwr, u8 pg_pwr, u8 iq_pwr, u8 ble_pwr);

void bredr_set_fix_pwr(u8 fix);
void ble_set_fix_pwr(u8 fix);
void at_set_atcom_low_power_mode(u8 enable);


static u8 at_uart_fifo_buffer[AT_UART_FIFIO_BUFFER_SIZE];
cbuffer_t at_to_uart_cbuf;

//===========================================================

static u8 event_buffer[64 + 4];
static void at_send_event_cmd_complete(u8 opcode, u8 status, u8 *packet, u8 size)
{
    if (size > 64) {
        log_error("EVT payload overflow");
        return;
    }

    event_buffer[0] = opcode;
    event_buffer[1] = status;
    memcpy(event_buffer + 2, packet, size);
    at_send_event(AT_EVT_CMD_COMPLETE, event_buffer, 2 + size);
}

static void at_send_event_update(u8 event_type, const u8 *packet, int size)
{
    at_send_event(event_type, packet, size);
}

static void at_send_event_uart_exception()
{
    at_send_event(AT_EVT_UART_EXCEPTION, 0, 0);
}

#if TRANS_AT_COM
static void at_com_packet_handler(const u8 *packet, int size)
{
    struct at_format *cmd = packet;
    u8 status;

    if (cmd->type != AT_PACKET_TYPE_CMD) {
        log_info("AT CMD TYPE Mismatch");
        return;
    }

    switch (cmd->opcode) {
    case AT_CMD_SET_BT_ADDR: {
        log_info("AT_CMD_SET_BT_ADDR");
        /* struct cmd_set_bt_addr *payload = cmd->payload; */
        /* lmp_hci_write_local_address(payload->addr); */
        edr_at_set_address(cmd->payload);
        status = 0;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_SET_BLE_ADDR: {
        log_info("AT_CMD_SET_BLE_ADDR");
        struct cmd_set_ble_addr *payload = cmd->payload;
        ble_at_set_address(payload->addr);
        status = 0;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_SET_VISIBILITY: {
        log_info("AT_CMD_SET_VISIBILITY");
        struct cmd_set_bt_visbility *payload = cmd->payload;

        edr_at_set_visibility(payload->discovery, payload->connect);
        ble_at_set_visibility(payload->adv);

        status = 0;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_SET_BT_NAME: {
        log_info("AT_CMD_SET_BT_NAME");
        /* struct cmd_set_bt_name *payload = cmd->payload; */
        /* lmp_hci_write_local_name(payload->name); */

        edr_at_set_name(cmd->payload, cmd->length);
        status = 0;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_SET_BLE_NAME: {
        log_info("AT_CMD_SET_BLE_NAME");
        ble_at_set_name(cmd->payload, cmd->length);
        status = 0;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_SEND_SPP_DATA: {
        log_info("AT_CMD_SEND_SPP_DATA");
        status = edr_at_send_spp_data(cmd->payload, cmd->length);
        status = !!status;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_SEND_BLE_DATA: {
        log_info("AT_CMD_SEND_BLE_DATA");
        /* struct cmd_send_ble_data *payload = cmd->payload; */

        /* log_info("GATT handle : 0x%x", payload->att_handle); */
        /* log_info_hexdump(payload->att_data, cmd->length - 2); */
        status = ble_at_send_data(cmd->payload, cmd->length);
        status = !!status;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_SEND_DATA: {
        log_info("AT_CMD_SEND_DATA");
        if (edr_at_get_staus() & BIT(ST_BIT_SPP_CONN)) {
            status =  edr_at_send_spp_data(cmd->payload, cmd->length);
        } else {
            status =  ble_at_send_data_default(cmd->payload, cmd->length);
        }
        status = !!status;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_STATUS_REQUEST: {
        log_info("AT_CMD_STATUS_REQUEST");
        status = ble_at_get_staus();
        status |= edr_at_get_staus();
        at_send_event(AT_EVT_STATUS_RESPONSE, &status, 1);
    }
    break;
    case AT_CMD_SET_PAIRING_MODE: {
        log_info("AT_CMD_SET_PAIRING_MODE");
        /* edr_at_set_pair_mode(cmd->payload[0]); */
        /* ble_at_set_pair_mode(cmd->payload[0]);  */
        /* status = 0; */
        status = 1;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_SET_PINCODE: {
        log_info("AT_CMD_SET_PINCODE");
        edr_at_set_pincode(cmd->payload);
        status = 0;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_SET_UART_FLOW: {
        log_info("AT_CMD_SET_UART_FLOW");
        /*-TODO-*/

        status = 1;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_SET_UART_BAUD: {
        log_info("AT_CMD_SET_UART_BAUD");
        /*-TODO-*/

        status = 1;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_VERSION_REQUEST: {
        log_info("AT_CMD_VERSION_REQUEST");
        /*-TODO-*/
        u32 version = 0x20190601;

        status = 0;
        at_send_event_cmd_complete(cmd->opcode, status, &version, 4);
    }
    break;
    case AT_CMD_BT_DISCONNECT: {
        log_info("AT_CMD_BT_DISCONNECT");
        status = 0;
        edr_at_disconnect();
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_BLE_DISCONNECT: {
        log_info("AT_CMD_BLE_DISCONNECT");
        status = 0;
        ble_at_disconnect();
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_SET_COD:
        log_info("AT_CMD_SET_COD");
        status = 0;
        edr_at_set_cod(cmd->payload);
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
        break;

    case AT_CMD_SET_RF_MAX_TXPOWER:
        log_info("AT_CMD_SET_RF_MAX_TXPOWER");
        if ((cmd->payload[0] < 10) && (cmd->payload[1] < 10) && (cmd->payload[2] < 10) && (cmd->payload[3] < 10)) {
            put_buf(cmd->payload, 4);
            status = 0;
            bt_max_pwr_set(cmd->payload[0], cmd->payload[1], cmd->payload[2], cmd->payload[3]);
        } else {
            status = 1;
        }
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
        break;


    case AT_CMD_SET_EDR_TXPOWER:
        log_info("AT_CMD_SET_EDR_TXPOWER,%d", cmd->payload[0]);
        if (cmd->payload[0] < 10) {
            status = 0;
            bredr_set_fix_pwr(cmd->payload[0]);
        } else {
            status = 1;
        }
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
        break;

    case AT_CMD_SET_BLE_TXPOWER:
        log_info("AT_CMD_SET_BLE_TXPOWER,%d", cmd->payload[0]);
        if (cmd->payload[0] < 10) {
            status = 0;
            ble_set_fix_pwr(cmd->payload[0]);
        } else {
            status = 1;
        }
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
        break;


    case AT_CMD_ENTER_SLEEP_MODE: {
        log_info("AT_CMD_ENTER_SLEEP_MODE");
        atcom_power_event_to_user(POWER_EVENT_POWER_SOFTOFF);
    }
    break;

    case AT_CMD_SET_LOW_POWER_MODE : {
        status = 0;
        log_info("AT_CMD_SET_LOW_POWER_MODE: %d", cmd->payload[0]);
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
#if (defined CONFIG_CPU_BD19)
        extern void board_at_uart_wakeup_enalbe(u8 enalbe);
        board_at_uart_wakeup_enalbe(cmd->payload[0]);
        at_set_atcom_low_power_mode(cmd->payload[0]);
#endif

    }
    break;

    case AT_CMD_SET_CONFIRM_GKEY:
        log_info("AT_CMD_SET_CONFIRM_GKEY");
        status = 0;
        ble_at_confirm_gkey(cmd->payload);
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
        break;
    case AT_CMD_SET_ADV_DATA: {
        log_info("AT_CMD_SET_ADV_DATA");
        ble_at_set_adv_data(cmd->payload, cmd->length);
        status = 0;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_SET_SCAN_DATA: {
        log_info("AT_CMD_SET_RSP_DATA");
        ble_at_set_rsp_data(cmd->payload, cmd->length);
        status = 0;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_SET_XTAL:
        log_info("AT_CMD_SET_XTAL");
        break;
    case AT_CMD_SET_DCDC: {
        log_info("AT_CMD_SET_DCDC");
        struct cmd_set_dcdc *payload = cmd->payload;
        power_set_mode(payload->enable ? PWR_DCDC15 : PWR_LDO15);
        status = 0;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_GET_BT_ADDR: {
        log_info("AT_CMD_GET_BT_ADDR");
        status = 0;
        edr_at_get_address(respond_buffer_static);
        at_send_event_cmd_complete(cmd->opcode, status, respond_buffer_static, 6);
    }
    break;
    case AT_CMD_GET_BLE_ADDR: {
        log_info("AT_CMD_GET_BLE_ADDR");
        status = 0;
        ble_at_get_address(respond_buffer_static);
        at_send_event_cmd_complete(cmd->opcode, status, respond_buffer_static, 6);
    }
    break;
    case AT_CMD_GET_BT_NAME: {
        log_info("AT_CMD_GET_BT_NAME");
        status = 0;
        respond_buffer_static[0] = edr_at_get_name(&respond_buffer_static[1]);
        at_send_event_cmd_complete(cmd->opcode, status, &respond_buffer_static[1], respond_buffer_static[0]);
    }
    break;
    case AT_CMD_GET_BLE_NAME: {
        log_info("AT_CMD_GET_BLE_NAME");
        status = 0;
        respond_buffer_static[0] = ble_at_get_name(&respond_buffer_static[1]);
        at_send_event_cmd_complete(cmd->opcode, status, &respond_buffer_static[1], respond_buffer_static[0]);
    }
    break;
    case AT_CMD_GET_PINCODE:
        log_info("AT_CMD_GET_PINCODE");
        status = 1;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
        break;


    case AT_CMD_BLE_CONN_PARAM_REQUEST:
        log_info("AT_CMD_BLE_CONN_PARAM_REQUEST");
        status = 0;
        {
            u16 interval_min, interval_max, latency, timeout, check = 0;
            interval_min = cmd->payload[1] * 0x100 + cmd->payload[0];
            interval_max = cmd->payload[3] * 0x100 + cmd->payload[2];
            latency = cmd->payload[5] * 0x100 + cmd->payload[4];
            timeout = cmd->payload[7] * 0x100 + cmd->payload[6];

            check = (0x06 <= interval_min) && (interval_min <= interval_max) && (interval_max <= 0xc80) && (latency <= 0x1f3) && (0xa <= timeout) && (timeout <= 0xc80);

            if (check) {
                slave_connect_param_update(interval_min, interval_max, latency, timeout);
            } else {
                status = 1;
            }
        }
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);

        break;


    default:
        at_send_event_uart_exception();
        /* ASSERT(0, "AT CMD Opcode Mismatch"); */
        break;
    }

}
#endif



#if TRANS_AT_CLIENT
static void at_client_packet_handler(const u8 *packet, int size)
{
    struct at_format *cmd = packet;
    u8 status;

    if (cmd->type != AT_PACKET_TYPE_CMD) {
        log_info("AT CMD TYPE Mismatch");
        return;
    }

    switch (cmd->opcode) {

    case AT_CMD_SET_BLE_ADDR: {
        log_info("AT_CMD_SET_BLE_ADDR");

        struct cmd_set_ble_addr *payload = cmd->payload;
        ble_at_set_address(payload->addr);
        status = 0;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;

    case AT_CMD_SET_BLE_NAME: {
        log_info("AT_CMD_SET_BLE_NAME");
        ble_at_set_name(cmd->payload, cmd->length);
        status = 0;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;

    case AT_CMD_STATUS_REQUEST: {
        log_info("AT_CMD_STATUS_REQUEST");
        status = ble_at_get_staus();
        at_send_event(AT_EVT_STATUS_RESPONSE, &status, 1);
    }
    break;

    case AT_CMD_VERSION_REQUEST: {
        log_info("AT_CMD_VERSION_REQUEST");
        /*-TODO-*/
        u32 version = 0x20190601;

        status = 0;
        at_send_event_cmd_complete(cmd->opcode, status, &version, 4);
    }
    break;

    case AT_CMD_BLE_DISCONNECT: {
        log_info("AT_CMD_BLE_DISCONNECT");
        status = 0;
        ble_at_disconnect();
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;


    case AT_CMD_SET_RF_MAX_TXPOWER:
        log_info("AT_CMD_SET_RF_MAX_TXPOWER");
        if ((cmd->payload[0] < 10) && (cmd->payload[1] < 10) && (cmd->payload[2] < 10) && (cmd->payload[3] < 10)) {
            put_buf(cmd->payload, 4);
            status = 0;
            bt_max_pwr_set(cmd->payload[0], cmd->payload[1], cmd->payload[2], cmd->payload[3]);
        } else {
            status = 1;
        }
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
        break;

    case AT_CMD_SET_BLE_TXPOWER:
        log_info("AT_CMD_SET_BLE_TXPOWER,%d", cmd->payload[0]);
        if (cmd->payload[0] < 10) {
            status = 0;
            ble_set_fix_pwr(cmd->payload[0]);
        } else {
            status = 1;
        }
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
        break;


    case AT_CMD_ENTER_SLEEP_MODE: {
        log_info("AT_CMD_ENTER_SLEEP_MODE");
        atcom_power_event_to_user(POWER_EVENT_POWER_SOFTOFF);
    }
    break;

    case AT_CMD_SET_DCDC: {
        log_info("AT_CMD_SET_DCDC");
        struct cmd_set_dcdc *payload = cmd->payload;
        power_set_mode(payload->enable ? PWR_DCDC15 : PWR_LDO15);
        status = 0;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;

    case AT_CMD_GET_BLE_ADDR: {
        log_info("AT_CMD_GET_BLE_ADDR");
        status = 0;
        ble_at_get_address(respond_buffer_static);
        at_send_event_cmd_complete(cmd->opcode, status, respond_buffer_static, 6);
    }
    break;

    case AT_CMD_GET_BLE_NAME: {
        log_info("AT_CMD_GET_BLE_NAME");
        status = 0;
        respond_buffer_static[0] = ble_at_get_name(&respond_buffer_static[1]);
        at_send_event_cmd_complete(cmd->opcode, status, &respond_buffer_static[1], respond_buffer_static[0]);
    }
    break;


    //TODO参数安全判断
    /**
    请求 BLE 更新连接参数
    */
    case AT_CMD_BLE_CONN_PARAM_REQUEST: {

        log_info("AT_CMD_BLE_CONN_PARAM_REQUEST");
        status = 0;

        {
            u16 interval_min, interval_max, latency, timeout, check = 0;

            interval_min = cmd->payload[1] * 0x100 + cmd->payload[0];
            interval_max = cmd->payload[3] * 0x100 + cmd->payload[2];
            latency = cmd->payload[5] * 0x100 + cmd->payload[4];
            timeout = cmd->payload[7] * 0x100 + cmd->payload[6];

            check = (0x06 <= interval_min) && (interval_min <= interval_max) && (interval_max <= 0xc80) && (latency <= 0x1f3) && (0xa <= timeout) && (timeout <= 0xc80);

            if (check) {
                client_connect_param_update(interval_min, interval_max, latency, timeout);
            } else {
                status = 1;
            }
        }

        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);

    }
    break;

    /**
    扫描参数配置
    */
    case AT_CMD_SET_BLE_SCAN_PARAM : {

        log_info("AT_CMD_SET_BLE_SCAN_PARAM");
        status = 0;

        {
            u16 scan_interval, scan_window, check = 0;

            scan_interval = cmd->payload[1] * 0x100 + cmd->payload[0];
            scan_window = cmd->payload[3] * 0x100 + cmd->payload[2];
            check = (0x04 <= scan_window) && (scan_window <= scan_interval) && (scan_interval <= 0x4000) ;

            if (check) {
                ble_at_scan_param(scan_interval, scan_window);
            } else {
                status = 1;
            }

        }
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);

    }
    break;

    /**
    设置蓝牙 BLE 主机 SCAN 使能
    */
    case AT_CMD_SET_BLE_SCAN_ENABLE : {

        status = 0;
        log_info("AT_CMD_SET_BLE_SCAN");
        bt_ble_scan_enable(0, cmd->payload[0]);
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);

    }
    break;

    /***
    蓝牙 BLE 主机创建连接连接监听
    ***/
    case AT_CMD_BLE_CREAT_CONNECT : {
        log_info("AT_CMD_BLE_CREAT_CONNECT");
        status = 0;

        client_create_connection(cmd->payload + 1, cmd->payload[0]);
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;

    /***
    蓝牙 BLE 主机取消连接监听
    ***/
    case AT_CMD_BLE_CREAT_CONNECT_CANNEL : {
        log_info("AT_CMD_BLE_CREAT_CONNECT_CANNEL");
        status = 0;

        client_create_connection_cancel();
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;

    /**
    BLE 主机搜索 profile
    */
    case AT_CMD_BLE_PROFILE_SEARCH : {
        log_info("AT_CMD_BLE_PROFILE_SEARCH");
        status = 0;

        client_search_profile(cmd->payload[0], cmd->payload + 1);
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);

    }
    break;


    /**
    开启对从机数据的监听  , 即开启接收CCCD,从机可以通过CCCD发送数据
    **/
    case AT_CMD_BLE_ATT_ENABLE_CCC : {
        log_info("AT_CMD_BLE_ATT_ENABLE_CCC");
        status = 0;

        {
            u16 value_handle = cmd->payload[1] * 0x100 + cmd->payload[0];
            client_receive_ccc(value_handle, cmd->payload[2]);
        }

        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;


    /**
    读ATT
    **/
    case AT_CMD_BLE_ATT_READ : {
        log_info("AT_CMD_BLE_ATT_READ");
        status = 0;
        client_read_long_value(cmd->payload);
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;

    /**
      写ATT,带响应
    */

    case AT_CMD_BLE_ATT_WRITE : {
        log_info("AT_CMD_BLE_ATT_WRITE");
        status = 0;
        {
            u16 write_handle =  cmd->payload[1] * 0x100 + cmd->payload[0];
            client_write(write_handle, cmd->payload + 2, cmd->length - 2);
        }
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;

    /**
    写ATT不带响应
    */
    case AT_CMD_BLE_ATT_WRITE_NO_RSP : {
        log_info("AT_CMD_BLE_ATT_WRITE_NO_RSP");
        status = 0;
        {
            u16 write_handle = cmd->payload[1] * 0x100 + cmd->payload[0];

            client_write_without_respond(write_handle, cmd->payload + 2, cmd->length - 2);
        }
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);

    }
    break;

    default:
        at_send_event_uart_exception();
        /* ASSERT(0, "AT CMD Opcode Mismatch"); */
        break;

    }
}
#endif

void at_send_event(u8 opcode, const u8 *packet, int size)
{
    struct at_format *evt;

    evt = (struct at_format *)__this->pAT_buffer;

    evt->type   = AT_PACKET_TYPE_EVT;
    evt->opcode = opcode;
    evt->length = size;

    ASSERT(AT_FORMAT_HEAD + size <= AT_BUFFER_SIZE, "AT_BUFFER, Fatal Error");

    if (size) {
        memcpy(evt->payload, packet, size);
    }

    int packet_len = evt->length + AT_FORMAT_HEAD;
    u16 ret = cbuf_write(&at_to_uart_cbuf, evt, packet_len);

    if (ret < packet_len) {
        log_info("bt so fast, uart lose data,%d!!", packet_len);
        return;
    }

    struct sys_event e;
    e.type = SYS_BT_EVENT;
    e.arg  = (void *)SYS_BT_EVENT_FORM_AT;
    e.u.dev.event = 0;
    e.u.dev.value = 0;
    sys_event_notify(&e);
}


void at_cmd_init(void)
{
    log_info("%s,%d\n", __func__, __LINE__);

#ifdef HAVE_MALLOC
    __this = malloc(sizeof(struct at_layer));
    ASSERT(__this, "Fatal Error");
    memset(__this, 0x0, sizeof(struct at_layer));

    __this->pAT_buffer = malloc(AT_BUFFER_SIZE);
    ASSERT(__this, "Fatal Error");
    memset(__this->pAT_buffer, 0x0, CI_BUFFER_SIZE);
#else
    log_info("Static");
    __this->pAT_buffer = pAT_buffer_static;
#endif
    /* __this->config = config; */
    /* at_transport_setup(void); */
#if TRANS_AT_COM
    at_uart_init(at_com_packet_handler);
    edr_at_register_event_cbk(at_send_event_update);
    ble_at_register_event_cbk(at_send_event_update);
#endif

#if TRANS_AT_CLIENT
    at_uart_init(at_client_packet_handler);
#endif

    cbuf_init(&at_to_uart_cbuf, at_uart_fifo_buffer, AT_UART_FIFIO_BUFFER_SIZE);

    log_info("at com is ready");
    at_send_event(AT_EVT_SYSTEM_READY, 0, 0);
}

#endif
