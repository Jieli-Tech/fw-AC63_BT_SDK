/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "btstack/bluetooth.h"
#include "system/includes.h"
#include "bt_common.h"
#include "api/sig_mesh_api.h"
#include "model_api.h"
#include "board.h"
#include "elet_at_cmd.h"
#include "buf_io_uart_tx.h"

#define LOG_TAG         "[Mesh-vendor_srv]"
#define LOG_INFO_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_ERROR_ENABLE
#include "debug.h"

#if (CONFIG_MESH_MODEL == SIG_MESH_VENDOR_SERVER)

extern u16_t primary_addr;
extern void mesh_setup(void (*init_cb)(void));
extern void gpio_pin_write(u8_t led_index, u8_t onoff);
extern void bt_mac_addr_set(u8 *bt_addr);
extern void prov_complete(u16_t net_idx, u16_t addr);
extern void prov_reset(void);
static void vendor_set(struct bt_mesh_model *model,
					   struct bt_mesh_msg_ctx *ctx,
					   struct net_buf_simple *buf);

#define SERVER_PUBLISH_EN       1

/**
 * @brief Config current node features(Relay/Proxy/Friend/Low Power)
 */
/*-----------------------------------------------------------*/
#define BT_MESH_FEAT_SUPPORTED_TEMP         ( \
											  BT_MESH_FEAT_RELAY | \
											  BT_MESH_FEAT_PROXY | \
											  0 \
											)
#include "feature_correct.h"
const int config_bt_mesh_features = BT_MESH_FEAT_SUPPORTED;

/**
 * @brief Config proxy connectable adv hardware param
 */
/*-----------------------------------------------------------*/
#if BT_MESH_FEATURES_GET(BT_MESH_FEAT_LOW_POWER)
const u16 config_bt_mesh_proxy_node_adv_interval = ADV_SCAN_UNIT(3000); // unit: ms
#else
const u16 config_bt_mesh_proxy_node_adv_interval = ADV_SCAN_UNIT(300); // unit: ms
#endif /* BT_MESH_FEATURES_GET(BT_MESH_FEAT_LOW_POWER) */

/**
 * @brief Config adv cache buffer
 */
/*-----------------------------------------------------------*/
#define MESH_ADV_BUFFER_COUNT           4
const u8 config_bt_mesh_adv_buf_count = MESH_ADV_BUFFER_COUNT;
#if (MESH_ADV_BUFFER_COUNT < 4) // base on "config_bt_mesh_node_msg_adv_duration = 100"
#error " current MESH_ADV_BUFFER_COUNT must >= 4 "
#endif

/**
 * @brief Conifg complete local name
 */
/*-----------------------------------------------------------*/
static u8 ble_mesh_adv_name[32 + 2];

#define BLE_DEV_NAME        'V', 'd', '_','s', 'r', 'v'

const uint8_t mesh_default_name[] =
{
	// Name
	BYTE_LEN(BLE_DEV_NAME) + 1, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, BLE_DEV_NAME,
};

void get_mesh_adv_name(u8 *len, u8 **data)
{
	// *len = sizeof(ble_mesh_adv_name);

	// *data = ble_mesh_adv_name;
	*len = ble_mesh_adv_name[0] + 1;
	*data = ble_mesh_adv_name;
}

/**
 * @brief Conifg MAC of current demo
 */
/*-----------------------------------------------------------*/
#define CUR_DEVICE_MAC_ADDR         0x442233445566

struct _switch
{
	u8_t sw_num;
	u8_t onoff_state;
};

/* Company Identifiers (see Bluetooth Assigned Numbers) */
#define BT_COMP_ID_LF           0x05D6 // Zhuhai Jieli technology Co.,Ltd

/*
 * Vendor Model ID
 * detail on Mesh_v1.0 <3.7.2 Model identifier>
 */
#define BT_MESH_VENDOR_MODEL_ID_SRV             0x0001

/*
 * Vendor Model Operation Codes
 * detail on Mesh_v1.0 <3.7.3.1 Operation codes>
 */
#define BT_MESH_VENDOR_MODEL_OP_SET             BT_MESH_MODEL_OP_3(0x01, BT_COMP_ID_LF)
#define BT_MESH_VENDOR_MODEL_OP_STATUS          BT_MESH_MODEL_OP_3(0x04, BT_COMP_ID_LF)

/*
 * Access payload fields
 * detail on Mesh_v1.0 <3.7.3 Access payload>
 */
#define TRANSMIC_SIZE                           4
#define MAX_USEFUL_ACCESS_PAYLOAD_SIZE          11 // 32 bit TransMIC (unsegmented)
#define ACCESS_OP_SIZE      3
#define ACCESS_PARAM_SIZE   (MAX_USEFUL_ACCESS_PAYLOAD_SIZE - ACCESS_OP_SIZE)

/* test data */
#define LED_STATE_LEN       1
#define REMAIN_DATA_LEN     (ACCESS_PARAM_SIZE - LED_STATE_LEN)
#define REMAIN_DATA_VALUE   0x02

/* LED NUMBER */
#define LED0_GPIO_PIN       0

struct onoff_state
{
	u8_t onoff;
	u8_t led_gpio_pin;
};

static struct onoff_state onoff_state[] =
{
	{ .led_gpio_pin = LED0_GPIO_PIN },
};

const u8 led_use_port[1] =
{
	IO_PORTA_01,
};

/*
 * Publication Declarations
 *
 * The publication messages are initialized to the
 * the size of the opcode + content
 *
 * For publication, the message must be in static or global as
 * it is re-transmitted several times. This occurs
 * after the function that called bt_mesh_model_publish() has
 * exited and the stack is no longer valid.
 *
 * Note that the additional 4 bytes for the AppMIC is not needed
 * because it is added to a stack variable at the time a
 * transmission occurs.
 *
 */
BT_MESH_MODEL_PUB_DEFINE(vendor_pub_srv, NULL, MAX_USEFUL_ACCESS_PAYLOAD_SIZE);

// static  u8_t net_buf_data_bt_mesh_pub_msg_vendor_pub_srv[11];
// static struct net_buf_simple bt_mesh_pub_msg_vendor_pub_srv = { .data = net_buf_data_bt_mesh_pub_msg_vendor_pub_srv, .len = 0, .size = 11, .__buf = net_buf_data_bt_mesh_pub_msg_vendor_pub_srv, };
// static struct bt_mesh_model_pub vendor_pub_srv = { .update = ((void*)0), .msg = &bt_mesh_pub_msg_vendor_pub_srv, }

/*
 * Models in an element must have unique op codes.
 *
 * The mesh stack dispatches a message to the first model in an element
 * that is also bound to an app key and supports the op code in the
 * received message.
 *
 */
/*
 * Vendor Model Server Op Dispatch Table
 */
static const struct bt_mesh_model_op vendor_srv_op[] =
{
	{ BT_MESH_VENDOR_MODEL_OP_SET, ACCESS_OP_SIZE, vendor_set },
	BT_MESH_MODEL_OP_END,
};

/*
 * Server Configuration Declaration
 */
static struct bt_mesh_cfg_srv cfg_srv =
{
	.relay          = BT_MESH_FEATURES_GET(BT_MESH_FEAT_RELAY),
	.frnd           = BT_MESH_FEATURES_GET(BT_MESH_FEAT_FRIEND),
	.gatt_proxy     = BT_MESH_FEATURES_GET(BT_MESH_FEAT_PROXY),
	.beacon         = BT_MESH_BEACON_DISABLED,
	.default_ttl    = 7,
};

/*
 * Client Configuration Declaration
 */
static struct bt_mesh_cfg_cli cfg_cli;

/*
 *
 * Element Model Declarations
 *
 * Element 0 Root Models
 */
static struct bt_mesh_model root_models[] =
{
	BT_MESH_MODEL_CFG_SRV(&cfg_srv), // default for root model
	// BT_MESH_MODEL_CFG_CLI(&cfg_cli), // default for self-configuration network
};

static struct bt_mesh_model vendor_server_models[] =
{
	BT_MESH_MODEL_VND(BT_COMP_ID_LF, BT_MESH_VENDOR_MODEL_ID_SRV,
					  vendor_srv_op, &vendor_pub_srv, &onoff_state[0]),
};

/*
 * LED to Server Model Assigmnents
 */
static struct bt_mesh_model *mod_srv_sw[] =
{
	&vendor_server_models[0],
};

/*
 * Root and Secondary Element Declarations
 */
static struct bt_mesh_elem elements[] =
{
	BT_MESH_ELEM(0, root_models, vendor_server_models),
};

static const struct bt_mesh_comp composition =
{
	.cid = BT_COMP_ID_LF,
	.pid = 0x001A,
	.vid = 0x0001,
	.elem = elements,
	.elem_count = ARRAY_SIZE(elements),
};

typedef struct dev_uuid_filed
{
	uint16_t mid;
	uint16_t cid;
	uint8_t pid[3];
	uint8_t addr[6];
	uint8_t feature_flag;
	uint16_t rfu;
} dev_uuid_filed_t;
static u8_t dev_uuid[16] = {MAC_TO_LITTLE_ENDIAN(CUR_DEVICE_MAC_ADDR)};
uint8_t elet_prov_complete_flag = 0;

static const struct bt_mesh_prov prov =
{
	.uuid = dev_uuid,
	.output_size = 0,
	.output_actions = 0,
	.output_number = 0,
	.complete = prov_complete,
	.reset = prov_reset,
};

static uint8_t uart_tx_data[30 + 2]; // 1 byte 0x5A + 30 bytes data + 1 bytes 0xAA
static void vendor_set(struct bt_mesh_model *model,
					   struct bt_mesh_msg_ctx *ctx,
					   struct net_buf_simple *buf)
{
	log_info("receive message len except opcode =0x%x", buf->len);
	log_info_hexdump(buf->data, buf->len);

	struct net_buf_simple *msg = model->pub->msg;
	struct onoff_state *onoff_state = model->user_data;

	if (buf->len > 30)
	{
		return;
	}

	uart_tx_data[0] = 0x5A;
	memcpy(&uart_tx_data[1], buf->data, buf->len);
	uart_tx_data[buf->len + 1] = 0xAA;
	buf_io_uart_tx_send_data(uart_tx_data, buf->len + 2, 1);

	// //< set led onoff
	// onoff_state->onoff = buffer_pull_u8_from_head(buf);
	// log_info("Local Node 0x%02x shoult set led to 0x%02x\n",
	//       bt_mesh_model_elem(model)->addr, onoff_state->onoff);
	// gpio_pin_write(onoff_state->led_gpio_pin, onoff_state->onoff);

	// //< Ack to client with the same receive data
	// NET_BUF_SIMPLE_DEFINE(ack_msg, MAX_USEFUL_ACCESS_PAYLOAD_SIZE + TRANSMIC_SIZE);
	// bt_mesh_model_msg_init(&ack_msg, BT_MESH_VENDOR_MODEL_OP_STATUS); // Opcode: 3 octets
	// buffer_add_u8_at_tail(&ack_msg, onoff_state->onoff); // onoff state: 1 octets
	// buffer_memset(&ack_msg, buffer_pull_u8_from_head(buf) + 1, REMAIN_DATA_LEN);
	// log_info_hexdump(ctx, sizeof(struct bt_mesh_msg_ctx));
	// log_info("net_idx %d app_idx %d addr %d recv_dst %d recv_rssi %d recv_ttl %d send_rel %d send_ttl %d",
	//       ctx->net_idx, ctx->app_idx, ctx->addr, ctx->recv_dst, ctx->recv_rssi, ctx->recv_ttl, ctx->send_rel, ctx->send_ttl);

	// log_info_hexdump(ack_msg.data, MAX_USEFUL_ACCESS_PAYLOAD_SIZE);

	// if (bt_mesh_model_send(model, ctx, &ack_msg, NULL, NULL))
	// {
	//  log_info("Unable to send Status response\n");
	// }
}

static uint8_t mesh_tx_data[30]; // 1 byte 0x5A + 30 bytes data + 1 bytes 0xAA
void vendor_server_send(uint8_t *p_data, uint32_t len)
{
	struct bt_mesh_model *mod_srv = mod_srv_sw[0];
	struct bt_mesh_msg_ctx ctx =
	{
		.addr = 1,
		.recv_dst = 2,
		.send_rel = 0,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};

	if (len > sizeof(mesh_tx_data))
	{
		log_info("len > %d", sizeof(mesh_tx_data));
		return;
	}

	memcpy(&mesh_tx_data, p_data, len);

	NET_BUF_SIMPLE_DEFINE(ack_msg, 3 + len + TRANSMIC_SIZE);
	bt_mesh_model_msg_init(&ack_msg, BT_MESH_VENDOR_MODEL_OP_STATUS); // Opcode: 3 octets
	buffer_memcpy(&ack_msg, mesh_tx_data, len);
	log_info_hexdump(ack_msg.data, len + 3);

	if (bt_mesh_model_send(mod_srv, &ctx, &ack_msg, NULL, NULL))
	{
		log_info("Unable to send Status response\n");
	}
}

#if SERVER_PUBLISH_EN
static void server_publish(struct _switch *sw)
{
#if 1
	struct bt_mesh_model *mod_srv = mod_srv_sw[sw->sw_num];;
	struct bt_mesh_msg_ctx ctx =
	{
		.addr = 1,
		.recv_dst = 2,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	uint8_t temp_data[REMAIN_DATA_LEN] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77};
	NET_BUF_SIMPLE_DEFINE(ack_msg, MAX_USEFUL_ACCESS_PAYLOAD_SIZE + TRANSMIC_SIZE);
	bt_mesh_model_msg_init(&ack_msg, BT_MESH_VENDOR_MODEL_OP_STATUS); // Opcode: 3 octets
	// buffer_add_u8_at_tail(&ack_msg, 0x11); // onoff state: 1 octets
	buffer_memcpy(&ack_msg, temp_data, REMAIN_DATA_LEN);
	log_info_hexdump(ack_msg.data, MAX_USEFUL_ACCESS_PAYLOAD_SIZE);

	if (bt_mesh_model_send(mod_srv, &ctx, &ack_msg, NULL, NULL))
	{
		log_info("Unable to send Status response\n");
	}

#else
	int err;
	struct bt_mesh_model *mod_srv;
	struct bt_mesh_model_pub *pub_srv;

	mod_srv = mod_srv_sw[sw->sw_num];
	pub_srv = mod_srv->pub;

	if (pub_srv->addr == BT_MESH_ADDR_UNASSIGNED)
	{
		log_info("pub_srv->addr == BT_MESH_ADDR_UNASSIGNED");
		return;
	}

	log_info("publish to Remote 0x%04x onoff 0x%04x sw_num 0x%04x\n",
			 pub_srv->addr, sw->onoff_state, sw->sw_num);

	bt_mesh_model_msg_init(pub_srv->msg, BT_MESH_VENDOR_MODEL_OP_SET);
	buffer_add_u8_at_tail(pub_srv->msg, sw->onoff_state);
	buffer_memset(pub_srv->msg, REMAIN_DATA_VALUE, REMAIN_DATA_LEN);

	err = bt_mesh_model_publish(mod_srv);

	if (err)
	{
		log_info("bt_mesh_model_publish err %d\n", err);
	}

	log_info_hexdump(pub_srv->msg->data, MAX_USEFUL_ACCESS_PAYLOAD_SIZE);
#endif
}

/*
 * Button Pressed Worker Task
 */
static void button_pressed_worker(struct _switch *sw)
{
	if (sw->sw_num >= composition.elem_count)
	{
		log_info("sw_num over elem_count");
		return;
	}

	server_publish(sw);
}
#endif /* SERVER_PUBLISH_EN */

#define NODE_ADDR 0x0002

#define GROUP_ADDR 0xc000

#define OP_VENDOR_BUTTON BT_MESH_MODEL_OP_3(0x00, BT_COMP_ID_LF)

void input_key_handler(u8 key_status, u8 key_number)
{
	struct _switch press_switch;
	static uint8_t long_hold_6s_cnts = 0;

	log_info("key_number=0x%x", key_number);

	// if ((key_number == 2) && (key_status == KEY_EVENT_LONG))
	// {
	//  log_info("\n  <bt_mesh_reset> \n");
	//  bt_mesh_reset();
	//  return;
	// }

	if (key_number != 0)
	{
		return;
	}

	switch (key_status)
	{
		case KEY_EVENT_CLICK:
			log_info("  [KEY_EVENT_CLICK]  ");
			// #if SERVER_PUBLISH_EN
			//          press_switch.sw_num = key_number;
			//          press_switch.onoff_state = 1;
			//          button_pressed_worker(&press_switch);
			// #endif /* SERVER_PUBLISH_EN */
			long_hold_6s_cnts = 0;
			break;

		case KEY_EVENT_LONG:
			log_info("  [KEY_EVENT_LONG]  ");
			break;

		case KEY_EVENT_HOLD:
			log_info("  [KEY_EVENT_HOLD]  %d", long_hold_6s_cnts);
			long_hold_6s_cnts++;

			if (long_hold_6s_cnts == 42)
			{
				long_hold_6s_cnts = 0;
				bt_mesh_reset();
			}

			break;

		default :
			long_hold_6s_cnts = 0;
			return;
	}
}

static void mesh_init(void)
{
	u8 bt_addr[6];

	dev_uuid_filed_t *p_dev_uuid_filed = (dev_uuid_filed_t *)dev_uuid;
	p_dev_uuid_filed->mid = elet_nv_cfg_mesh_mcid_get();
	p_dev_uuid_filed->cid = elet_nv_cfg_mesh_ccid_get();
    *(uint32_t *)p_dev_uuid_filed->pid = elet_nv_cfg_mesh_pid_get();
	le_controller_get_mac(bt_addr);
	memcpy(p_dev_uuid_filed->addr, bt_addr, 6);
	p_dev_uuid_filed->feature_flag = 0x03;
	p_dev_uuid_filed->rfu = 0x0000;

	int err = bt_mesh_init(&prov, &composition);

	if (err)
	{
		log_error("Initializing mesh failed (err %d)\n", err);
		return;
	}

	settings_load();

	bt_mesh_prov_enable(BT_MESH_PROV_GATT | BT_MESH_PROV_ADV);

	buf_io_uart_tx_init_buffer();
	elet_at_cmd_init();
}

extern u32 hex_2_str(u8 *hex, u32 hex_len, u8 *str);
extern void mesh_set_gap_name(const u8 *name);
void bt_ble_init(void)
{
	u8 bt_addr[6] = {MAC_TO_LITTLE_ENDIAN(CUR_DEVICE_MAC_ADDR)};

	bt_mac_addr_set(NULL);
	le_controller_get_mac(bt_addr);
	reverse_bd_addr(bt_addr, bt_addr);

	u8 *name_p = &ble_mesh_adv_name[2];
	memcpy(name_p, "ET23_MESH", 9);
	hex_2_str(&bt_addr[3], 3, name_p + 9);
	ble_mesh_adv_name[0] = strlen(name_p) + 1;
	ble_mesh_adv_name[1] = BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME;

	mesh_set_gap_name(name_p);
	log_info("mesh_name:%s\n", name_p);

	mesh_setup(mesh_init);
}

void example_node_reset(void)
{
	//< reset the node to an unprovisioned device

	bt_mesh_reset();
}

#endif /* (CONFIG_MESH_MODEL == SIG_MESH_VENDOR_SERVER) */
