/*********************************************************************************************
    *   Filename        : at_cmds.c

    *   Description     :

    *   Author          : Bingquan

    *   Email           : caibingquan@zh-jieli.com

    *   Last modifiled  : 2019-05-11 15:09

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
#include "at.h"
#include "spp_at_com.h"
#include "le_at_com.h"
/* #include "config/config_transport.h" */
/* #include "ble/hci_ll.h" */
/* #include "classic/hci_lmp.h" */
/* #include "btcontroller_modules.h" */
/* #include "asm/power_interface.h" */
/* #include "le_at_com.h" */
/* #include "spp_at_com.h" */
/* #include "app_config.h" */
/* #include "bt_common.h" */

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
extern void at_set_soft_poweroff(void);
extern void bt_max_pwr_set(u8 pwr, u8 pg_pwr, u8 iq_pwr, u8 ble_pwr);

void bredr_set_fix_pwr(u8 fix);
void ble_set_fix_pwr(u8 fix);

static u8 event_buffer[64+4];
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

static void at_send_event_update(u8 event_type,const u8 *packet, int size)
{
	/* switch(event_type) */
	/* { */
		/* case AT_EVT_BLE_DATA_RECEIVED: */
			/* break; */
		/* default: */
			/* memcpy(event_buffer, packet, size); */
			/* break; */
	/* } */
	at_send_event(event_type, packet, size);
}

static void packet_handler(const u8 *packet, int size)
{
    struct at_format *cmd = packet;
    u8 status;

	if(cmd->type != AT_PACKET_TYPE_CMD){
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
		
		edr_at_set_visibility(payload->discovery,payload->connect);
		ble_at_set_visibility(payload->adv);
		
		status = 0;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_SET_BT_NAME: {
        log_info("AT_CMD_SET_BT_NAME");
        /* struct cmd_set_bt_name *payload = cmd->payload; */
        /* lmp_hci_write_local_name(payload->name); */
        
        edr_at_set_name(cmd->payload,cmd->length);
		status = 0;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_SET_BLE_NAME: {
        log_info("AT_CMD_SET_BLE_NAME");
        ble_at_set_name(cmd->payload,cmd->length);
		status = 0;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_SEND_SPP_DATA: {
        log_info("AT_CMD_SEND_SPP_DATA");
        status = edr_at_send_spp_data(cmd->payload,cmd->length);
        status = !!status;
		at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_SEND_BLE_DATA: {
        log_info("AT_CMD_SEND_BLE_DATA");
        /* struct cmd_send_ble_data *payload = cmd->payload; */

        /* log_info("GATT handle : 0x%x", payload->att_handle); */
        /* log_info_hexdump(payload->att_data, cmd->length - 2); */
		status = ble_at_send_data(cmd->payload,cmd->length);
        status = !!status;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_SEND_DATA: {
        log_info("AT_CMD_SEND_DATA");
		if(edr_at_get_staus() & BIT(ST_BIT_SPP_CONN))
		{
			status =  edr_at_send_spp_data(cmd->payload,cmd->length);
		}
		else
		{
			status =  ble_at_send_data_default(cmd->payload,cmd->length);
		}
        status = !!status;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_STATUS_REQUEST: {
        log_info("AT_CMD_STATUS_REQUEST");
		status = ble_at_get_staus();
		status |= edr_at_get_staus();
        at_send_event(AT_EVT_STATUS_RESPONSE,&status,1);
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
		if((cmd->payload[0] < 10) && (cmd->payload[1] < 10) && (cmd->payload[2] < 10) && (cmd->payload[3] < 10)) 
		{
			put_buf(cmd->payload,4);
			status = 0;
			bt_max_pwr_set(cmd->payload[0],cmd->payload[1],cmd->payload[2],cmd->payload[3]);
		}
		else{
			status = 1;
		}
		at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
		break;

	
	case AT_CMD_SET_EDR_TXPOWER:
		log_info("AT_CMD_SET_EDR_TXPOWER,%d",cmd->payload[0]);
		if(cmd->payload[0] < 10) 
		{
			status = 0;
			bredr_set_fix_pwr(cmd->payload[0]);
		}
		else{
			status = 1;
		}
		at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
		break;

	case AT_CMD_SET_BLE_TXPOWER:
		log_info("AT_CMD_SET_BLE_TXPOWER,%d",cmd->payload[0]);
		if(cmd->payload[0] < 10) 
		{
			status = 0;
			ble_set_fix_pwr(cmd->payload[0]);
		}
		else{
			status = 1;
		}
		at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
		break;


	case AT_CMD_ENTER_SLEEP_MODE: {
        log_info("AT_CMD_ENTER_SLEEP_MODE");
        at_set_soft_poweroff();
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
        ble_at_set_adv_data(cmd->payload,cmd->length);
		status = 0;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
    }
    break;
    case AT_CMD_SET_SCAN_DATA: {
        log_info("AT_CMD_SET_RSP_DATA");
        ble_at_set_rsp_data(cmd->payload,cmd->length);
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
		at_send_event_cmd_complete(cmd->opcode, status,respond_buffer_static, 6);
    }
    break;
    case AT_CMD_GET_BLE_ADDR: {
        log_info("AT_CMD_GET_BLE_ADDR");
        status = 0;
		ble_at_get_address(respond_buffer_static);
		at_send_event_cmd_complete(cmd->opcode, status,respond_buffer_static, 6);
    }
    break;
    case AT_CMD_GET_BT_NAME: {
        log_info("AT_CMD_GET_BT_NAME");
		status = 0;
        respond_buffer_static[0] = edr_at_get_name(&respond_buffer_static[1]);
		at_send_event_cmd_complete(cmd->opcode, status,&respond_buffer_static[1],respond_buffer_static[0]);
    }
    break;
    case AT_CMD_GET_BLE_NAME: {
        log_info("AT_CMD_GET_BLE_NAME");
        status = 0;
        respond_buffer_static[0] = ble_at_get_name(&respond_buffer_static[1]);
		at_send_event_cmd_complete(cmd->opcode, status,&respond_buffer_static[1],respond_buffer_static[0]);
    }
    break;
    case AT_CMD_GET_PINCODE:
        log_info("AT_CMD_GET_PINCODE");
        status = 1;
        at_send_event_cmd_complete(cmd->opcode, status, NULL, 0);
		break;
    default:
        ASSERT(0, "AT CMD Opcode Mismatch");
        break;
    }
}

/* static void at_transport_setup(void) */
/* { */
    /* if (transport == NULL) { */
        /* log_error("AT TRANSPORT NULL"); */
    /* } */

    /* __this->dev_transport = transport; */

    /* log_info("init : 0x%x", __this->dev_transport->init); */
    /* if (__this->dev_transport->init) { */
        /* __this->dev_transport->init(__this->config); */
    /* } */

    /* log_info("open : 0x%x", __this->dev_transport->open); */
    /* if (__this->dev_transport->open) { */
        /* __this->dev_transport->open(); */
    /* } */

    /* ASSERT(__this->dev_transport->register_packet_handler, "Transport AT output NULL"); */

    /* log_info("register"); */
    /* __this->dev_transport->register_packet_handler(packet_handler); */
/* } */

extern int at_uart_send_packet(const u8 *packet, int size);
void at_send_event(u8 opcode, const u8 *packet, int size)
{
    struct at_format *evt;

    evt = (struct at_format *)__this->pAT_buffer;

    evt->type   = AT_PACKET_TYPE_EVT;
    evt->opcode = opcode;
    evt->length = size;

    ASSERT(AT_FORMAT_HEAD + size <= AT_BUFFER_SIZE, "Fatal Error");

    if (size) {
        memcpy(evt->payload, packet, size);
    }
    at_uart_send_packet(evt, evt->length + AT_FORMAT_HEAD);
}


extern void at_uart_init(void *packet_handler);
void at_cmd_init(void)
{
	log_info("%s,%d\n",__func__,__LINE__);

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
	at_uart_init(packet_handler);
	edr_at_register_event_cbk(at_send_event_update);
	ble_at_register_event_cbk(at_send_event_update);
	
    log_info("at com is ready");
	at_send_event(AT_EVT_SYSTEM_READY,0,0);
}

#endif
