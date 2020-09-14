/*  Bluetooth Mesh */

/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "btstack/bluetooth.h"
#include "system/includes.h"
#include "bt_common.h"
#include "system/crypto_toolbox/sha256.h"
#include "api/sig_mesh_api.h"
#include "model_api.h"
#include "unix_timestamp.h"

#define LOG_TAG             "[Mesh-AliSocket]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if (CONFIG_MESH_MODEL == SIG_MESH_ALIGENIE_SOCKET)

extern u16_t primary_addr;
extern void mesh_setup(void (*init_cb)(void));
extern void gpio_pin_write(u8_t led_index, u8_t onoff);
extern void bt_mac_addr_set(u8 *bt_addr);
extern void prov_complete(u16_t net_idx, u16_t addr);
extern void prov_reset(void);
extern uint32_t btctler_get_rand_from_assign_range(uint32_t rand, uint32_t min, uint32_t max);
extern void pseudo_random_genrate(uint8_t *dest, unsigned size);


/*
 * @brief Config current node features(Relay/Proxy/Friend/Low Power)
 */
/*-----------------------------------------------------------*/
#define BT_MESH_FEAT_SUPPORTED_TEMP         ( \
                                                BT_MESH_FEAT_FRIEND | \
                                                0 \
                                            )
#include "feature_correct.h"
const int config_bt_mesh_features = BT_MESH_FEAT_SUPPORTED;

/*
 * @brief Config proxy connectable adv hardware param
 */
/*-----------------------------------------------------------*/
#if BT_MESH_FEATURES_GET(BT_MESH_FEAT_LOW_POWER)
const u16 config_bt_mesh_proxy_node_adv_interval = ADV_SCAN_UNIT(3000); // unit: ms
#else
const u16 config_bt_mesh_proxy_node_adv_interval = ADV_SCAN_UNIT(300); // unit: ms
#endif /* BT_MESH_FEATURES_GET(BT_MESH_FEAT_LOW_POWER) */

/*
 * @brief Conifg complete local name
 */
/*-----------------------------------------------------------*/
#define BLE_DEV_NAME        'A', 'G', '-', 'S', 'o', 'c', 'k', 'e', 't'

const uint8_t mesh_name[] = {
    // Name
    BYTE_LEN(BLE_DEV_NAME) + 1, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, BLE_DEV_NAME,
};

void get_mesh_adv_name(u8 *len, u8 **data)
{
    *len = sizeof(mesh_name);

    *data = mesh_name;
}

/*
 * @brief Conifg AliGenie 三元组
 *
 * detail on https://www.aligenie.com/doc/357554/gtgprq
 */
/*-----------------------------------------------------------*/
#define PID_TO_LITTLE_ENDIAN(x) \
    (x & 0xff), \
    ((x >> 8) & 0xff), \
    ((x >> 16) & 0xff), \
    ((x >> 24) & 0xff)
#define PID_TO_BIG_ENDIAN(x) \
    ((x >> 24) & 0xff), \
    ((x >> 16) & 0xff), \
    ((x >> 8) & 0xff), \
    (x & 0xff)

#define PRODUCT_ID_STRING_SIZE      (sizeof(Product_ID) * 2)
#define MAC_ADDRESS_STRING_SIZE     (sizeof(Mac_Address) * 2)
#define SECRET_STRING_SIZE          (sizeof(Secret) - 1)

#define CUR_DEVICE_MAC_ADDR         0x28fa7a42bf0d
#define PRODUCT_ID                  12623
#define DEVICE_SECRET               "753053e923f30c9f0bc4405cf13ebda6"

/*
 * @brief Publication Declarations
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
/*-----------------------------------------------------------*/
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_pub_srv, NULL, 2 + 2);

/*
 * @brief Generic OnOff Model Operation Codes
 */
/*-----------------------------------------------------------*/
#define BT_MESH_MODEL_OP_GEN_ONOFF_GET			BT_MESH_MODEL_OP_2(0x82, 0x01)
#define BT_MESH_MODEL_OP_GEN_ONOFF_SET			BT_MESH_MODEL_OP_2(0x82, 0x02)
#define BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK	BT_MESH_MODEL_OP_2(0x82, 0x03)
#define BT_MESH_MODEL_OP_GEN_ONOFF_STATUS		BT_MESH_MODEL_OP_2(0x82, 0x04)

/*
 * @brief Vendor Model ID
 *
 * Company Identifiers (see Bluetooth Assigned Numbers)
 * detail on Mesh_v1.0 <3.7.2 Model identifier>
 * detail on https://www.aligenie.com/doc/357554/iv5it7
 */
/*-----------------------------------------------------------*/
#define BT_COMP_ID_LF                           0x01A8 // Alibaba
#define BT_MESH_VENDOR_MODEL_ID_SRV             0x0000
#define BT_MESH_VENDOR_MODEL_ID_CLI             0x0001

/*
 * @brief AliGenie Vendor Model Operation Codes
 *
 * detail on Mesh_v1.0 <3.7.3.1 Operation codes>
 * 扩展消息 detail on https://www.aligenie.com/doc/357554/iv5it7
 */
/*-----------------------------------------------------------*/
#define VENDOR_MSG_ATTR_GET			            BT_MESH_MODEL_OP_3(0xD0, BT_COMP_ID_LF)
#define VENDOR_MSG_ATTR_SET			            BT_MESH_MODEL_OP_3(0xD1, BT_COMP_ID_LF)
#define VENDOR_MSG_ATTR_SET_UNACK			    BT_MESH_MODEL_OP_3(0xD2, BT_COMP_ID_LF)
#define VENDOR_MSG_ATTR_STATUS			        BT_MESH_MODEL_OP_3(0xD3, BT_COMP_ID_LF)
#define VENDOR_MSG_ATTR_INDICAT			        BT_MESH_MODEL_OP_3(0xD4, BT_COMP_ID_LF)
#define VENDOR_MSG_ATTR_CONFIRM			        BT_MESH_MODEL_OP_3(0xD5, BT_COMP_ID_LF)
#define VENDOR_MSG_ATTR_TRANSPARENT			    BT_MESH_MODEL_OP_3(0xCF, BT_COMP_ID_LF)

/*
 * @brief AliGenie Vendor Model Message Struct
 *
 * 定时功能 detail on https://www.aligenie.com/doc/357554/ovzn6v
 */
/*-----------------------------------------------------------*/
#define ATTR_TYPE_UNIX_TIME                     0xF01F
#define ATTR_TYPE_SET_TIMEOUT                   0xF010

struct __unix_time {
    u32 Opcode: 24,
        TID: 8;
    u16 Attr_Type;
    u32 time;
} _GNU_PACKED_;

struct __set_timeout {
    u32 Opcode: 24,
        TID: 8;
    u16 Attr_Type;
    struct __param {
        u8 index;
        u32 time;
        u16 attr_type;
        u8 attr_para;
    } _GNU_PACKED_ param[1] ;
} _GNU_PACKED_;


/*
 * @brief Access Payload Fields
 *
 * detail on Mesh_v1.0 <3.7.3 Access payload>
 */
/*-----------------------------------------------------------*/
#define TRANSMIC_SIZE                           4
#define MAX_USEFUL_ACCESS_PAYLOAD_SIZE          11 // 32 bit TransMIC (unsegmented)
#define ACCESS_OP_SIZE                          3
#define ACCESS_PARAM_SIZE                       (MAX_USEFUL_ACCESS_PAYLOAD_SIZE - ACCESS_OP_SIZE)

/*
 * @brief Server Configuration Declaration
 */
/*-----------------------------------------------------------*/
static struct bt_mesh_cfg_srv cfg_srv = {
    .relay          = BT_MESH_FEATURES_GET(BT_MESH_FEAT_RELAY),
    .frnd           = BT_MESH_FEATURES_GET(BT_MESH_FEAT_FRIEND),
    .gatt_proxy     = BT_MESH_FEATURES_GET(BT_MESH_FEAT_PROXY),
    .beacon         = BT_MESH_BEACON_ENABLED,
    .default_ttl    = 7,
};

/*
 * @brief Generic OnOff State Set
 */
/*-----------------------------------------------------------*/
#define LED0_GPIO_PIN           0

struct onoff_state {
    u8_t current;
    u8_t previous;
    u8_t led_gpio_pin;
};

struct _switch {
    u8_t sw_num;
    u8_t onoff_state;
};

static struct onoff_state onoff_state[] = {
    { .led_gpio_pin = LED0_GPIO_PIN },
};

const u8 led_use_port[] = {

    IO_PORTA_01,

};

/*
 * @brief Generic OnOff Model Server Message Handlers
 *
 * Mesh Model Specification 3.1.1
 */
/*-----------------------------------------------------------*/
static void respond_messsage_schedule(u16 *delay, u16 *duration, void *cb_data)
{
    /*  Mesh_v1.0 <3.7.4.1 Transmitting an access message>
     *
     *	  If the message is sent in response to a received message
     *  that was sent to a unicast address, the node should transmit
     *  the response message with a random delay between 20 and 50 milliseconds.
     *
     *    If the message is sent in response to a received message
     *  that was sent to a group address or a virtual address,
     *  the node should transmit the response message with
     *  a random delay between 20 and 500 milliseconds.
     */
    u16 delay_ms;
    u16 dst_addr = (u16)cb_data;

    pseudo_random_genrate((u8 *)&delay_ms, 2);
    if (BT_MESH_ADDR_IS_UNICAST(dst_addr)) {
        delay_ms = btctler_get_rand_from_assign_range(delay_ms, 20, 50);
    } else {
        delay_ms = btctler_get_rand_from_assign_range(delay_ms, 20, 200);
    }

    *delay = delay_ms;
    log_info("respond_messsage delay =%u ms", delay_ms);
}

static const struct bt_mesh_send_cb rsp_msg_cb = {
    .user_intercept = respond_messsage_schedule,
    /* .user_intercept = NULL, */
};

static void gen_onoff_get(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf)
{
    NET_BUF_SIMPLE_DEFINE(msg, 2 + 1 + 4);
    struct onoff_state *onoff_state = model->user_data;

    log_info("addr 0x%04x onoff 0x%02x\n",
             bt_mesh_model_elem(model)->addr, onoff_state->current);
    bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_OP_GEN_ONOFF_STATUS);
    buffer_add_u8_at_tail(&msg, onoff_state->current);

    if (bt_mesh_model_send(model, ctx, &msg, &rsp_msg_cb, (void *)ctx->recv_dst)) {
        log_info("Unable to send On Off Status response\n");
    }
}

static void gen_onoff_set_unack(struct bt_mesh_model *model,
                                struct bt_mesh_msg_ctx *ctx,
                                struct net_buf_simple *buf)
{
    struct net_buf_simple *msg = model->pub->msg;
    struct onoff_state *onoff_state = model->user_data;
    int err;

    onoff_state->current = buffer_pull_u8_from_head(buf);
    log_info("addr 0x%02x state 0x%02x\n",
             bt_mesh_model_elem(model)->addr, onoff_state->current);
    /* log_info_hexdump((u8 *)onoff_state, sizeof(*onoff_state)); */

    gpio_pin_write(onoff_state->led_gpio_pin,
                   onoff_state->current);

#if 0
    /*
     * If a server has a publish address, it is required to
     * publish status on a state change
     *
     * See Mesh Profile Specification 3.7.6.1.2
     *
     * Only publish if there is an assigned address
     */

    if (onoff_state->previous != onoff_state->current &&
        model->pub->addr != BT_MESH_ADDR_UNASSIGNED) {
        log_info("publish last 0x%02x cur 0x%02x\n",
                 onoff_state->previous, onoff_state->current);
        onoff_state->previous = onoff_state->current;
        bt_mesh_model_msg_init(msg,
                               BT_MESH_MODEL_OP_GEN_ONOFF_STATUS);
        buffer_add_u8_at_tail(msg, onoff_state->current);
        err = bt_mesh_model_publish(model);
        if (err) {
            log_info("bt_mesh_model_publish err %d\n", err);
        }
    }
#endif /*  */
}

static void gen_onoff_set(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf)
{
    log_info("gen_onoff_set\n");

    gen_onoff_set_unack(model, ctx, buf);
    gen_onoff_get(model, ctx, buf);
}

/*
 * @brief OnOff Model Server Op Dispatch Table
 */
/*-----------------------------------------------------------*/
static const struct bt_mesh_model_op gen_onoff_srv_op[] = {
    { BT_MESH_MODEL_OP_GEN_ONOFF_GET, 0, gen_onoff_get },
    { BT_MESH_MODEL_OP_GEN_ONOFF_SET, 2, gen_onoff_set },
    { BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK, 2, gen_onoff_set_unack },
    BT_MESH_MODEL_OP_END,
};


/*
 * @brief AliGenie Vendor Model Message Handlers
 *
 * 定时功能 detail on https://www.aligenie.com/doc/357554/ovzn6v
 */
/*-----------------------------------------------------------*/
static void vendor_attr_status_send(struct bt_mesh_model *model,
                                    struct bt_mesh_msg_ctx *ctx,
                                    void *buf, u16 len)
{
    log_info("ready to send ATTR_TYPE_SET_TIMEOUT status");

    NET_BUF_SIMPLE_DEFINE(msg, len + TRANSMIC_SIZE);

    buffer_memcpy(&msg, buf, len);

    log_info_hexdump(msg.data, msg.len);

    if (bt_mesh_model_send(model, ctx, &msg, NULL, NULL)) {
        log_error("Unable to send Status response\n");
    }
}

static void vendor_attr_get(struct bt_mesh_model *model,
                            struct bt_mesh_msg_ctx *ctx,
                            struct net_buf_simple *buf)
{
    log_info("receive vendor_attr_get, len except opcode =0x%x", buf->len);
    log_info_hexdump(buf->data, buf->len);

}

static void vendor_attr_set(struct bt_mesh_model *model,
                            struct bt_mesh_msg_ctx *ctx,
                            struct net_buf_simple *buf)
{
    log_info("receive vendor_attr_set, len except opcode =0x%x", buf->len);
    log_info_hexdump(buf->data, buf->len);

    u8 tid = buffer_pull_u8_from_head(buf);
    u16 Attr_Type = buffer_pull_le16_from_head(buf);
    switch (Attr_Type) {
    case ATTR_TYPE_UNIX_TIME: {
        u32 time = buffer_pull_le32_from_head(buf);

        struct UTC_TIME cur_utc = unix32_to_UTC_beijing(time);
        log_info("北京时间: %d/%d/%d %02d:%02d:%02d, 星期 %d",
                 cur_utc.year, cur_utc.month, cur_utc.day,
                 cur_utc.hour, cur_utc.minute, cur_utc.second,
                 cur_utc.weekday);

        struct __unix_time unix_time = {
            .Opcode = buffer_head_init(VENDOR_MSG_ATTR_STATUS),
            .TID = tid,
            .Attr_Type = Attr_Type,
            .time = time,
        };

        vendor_attr_status_send(model, ctx, &unix_time, sizeof(unix_time));
    }
    break;

    case ATTR_TYPE_SET_TIMEOUT: {
        u8 index = buffer_pull_u8_from_head(buf);
        u32 time = buffer_pull_le32_from_head(buf);
        u16 attr_type = buffer_pull_le16_from_head(buf);
        u8 attr_para = buffer_pull_u8_from_head(buf);

        struct UTC_TIME cur_utc = unix32_to_UTC_beijing(time);
        log_info("北京时间: %d/%d/%d %02d:%02d:%02d, 星期 %d",
                 cur_utc.year, cur_utc.month, cur_utc.day,
                 cur_utc.hour, cur_utc.minute, cur_utc.second,
                 cur_utc.weekday);
        log_info("attr_type=0x%x, attr_para=0x%x", attr_type, attr_para);

        struct __set_timeout set_timeout = {
            .Opcode = buffer_head_init(VENDOR_MSG_ATTR_STATUS),
            .TID = tid,
            .Attr_Type = Attr_Type,
            .param[0] = {
                .index = index,
                .time = time,
                .attr_type = attr_type,
                .attr_para = attr_para,
            },
        };

        vendor_attr_status_send(model, ctx, &set_timeout, sizeof(set_timeout));
    }
    break;

    default :
        break;
    }

}

/*
 * @brief Vendor Model Server Op Dispatch Table
 */
/*-----------------------------------------------------------*/
static const struct bt_mesh_model_op vendor_srv_op[] = {
    { VENDOR_MSG_ATTR_GET, ACCESS_OP_SIZE, vendor_attr_get },
    { VENDOR_MSG_ATTR_SET, ACCESS_OP_SIZE, vendor_attr_set },
    BT_MESH_MODEL_OP_END,
};

/*
 * @brief Element Model Declarations
 *
 * Element 0 Root Models
 */
/*-----------------------------------------------------------*/
static struct bt_mesh_model root_models[] = {
    BT_MESH_MODEL_CFG_SRV(&cfg_srv),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_onoff_srv_op, &gen_onoff_pub_srv, &onoff_state[0]),
};

static struct bt_mesh_model vendor_server_models[] = {
    BT_MESH_MODEL_VND(BT_COMP_ID_LF, BT_MESH_VENDOR_MODEL_ID_CLI, NULL, NULL, NULL),
    BT_MESH_MODEL_VND(BT_COMP_ID_LF, BT_MESH_VENDOR_MODEL_ID_SRV, vendor_srv_op, NULL, &onoff_state[0]),
};

/*
 * @brief LED to Server Model Assigmnents
 */
/*-----------------------------------------------------------*/
static struct bt_mesh_model *mod_srv_sw[] = {
    &root_models[1],
};

/*
 * @brief Root and Secondary Element Declarations
 */
/*-----------------------------------------------------------*/
static struct bt_mesh_elem elements[] = {
    BT_MESH_ELEM(0, root_models, vendor_server_models),
};

static const struct bt_mesh_comp composition = {
    .cid = BT_COMP_ID_LF,
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

static const u8 dev_uuid[16] = {
    0xA8, 0x01,  // CID
    0x01 | BIT(4) | BIT(6), // PID
    PID_TO_LITTLE_ENDIAN(PRODUCT_ID), // ProductID
    MAC_TO_LITTLE_ENDIAN(CUR_DEVICE_MAC_ADDR), // MAC
    0x00, 0x00, // FeatureFlag
    0x00 // RFU
};

/*
 * @brief AliGenie 设备配网流程
 *
 * detail on https://www.aligenie.com/doc/357554/gtgprq
 */
/*-----------------------------------------------------------*/
static u8 auth_data[16];

static void hex_to_string(void *in, int in_len, void *out, int out_len)
{
    static const char hex[] = "0123456789abcdef";
    u8 *a = in;
    u8 *b = out;
    int i;

    if (in_len > (out_len / 2)) {
        log_error("\"error: in_len > (out_len / 2)\"");
        return;
    }

    for (i = 0; i < in_len; i++) {
        b[i * 2]     = hex[a[i] >> 4];
        b[i * 2 + 1] = hex[a[i] & 0xf];
    }
}

static void static_auth_value_calculate(void)
{
    log_info("--func=%s", __FUNCTION__);

    const u8 Product_ID[] = {
        PID_TO_BIG_ENDIAN(PRODUCT_ID)
    };
    const u8 Mac_Address[] = {
        MAC_TO_BIG_ENDIAN(CUR_DEVICE_MAC_ADDR)
    };
    const u8 Secret[] = {
        DEVICE_SECRET
    };

    u8 string_buf[PRODUCT_ID_STRING_SIZE + MAC_ADDRESS_STRING_SIZE + SECRET_STRING_SIZE + 2];
    u8 *string_p = string_buf;
    u8 digest[SHA256_DIGEST_SIZE];

    //< Product ID
    hex_to_string(Product_ID, sizeof(Product_ID), string_p, PRODUCT_ID_STRING_SIZE);
    string_p += PRODUCT_ID_STRING_SIZE;
    *string_p++ = ',';
    //< Mac Address
    hex_to_string(Mac_Address, sizeof(Mac_Address), string_p, MAC_ADDRESS_STRING_SIZE);
    string_p += MAC_ADDRESS_STRING_SIZE;
    *string_p++ = ',';
    //< Secert
    memcpy(string_p, Secret, SECRET_STRING_SIZE);

    sha256Compute(string_buf, sizeof(string_buf), digest);

    memcpy(auth_data, digest, sizeof(auth_data));

    log_info("string_buf : %s", string_buf);
    log_info_hexdump(string_buf, sizeof(string_buf));
    log_info_hexdump(auth_data, sizeof(auth_data));
}

static const struct bt_mesh_prov prov = {
    .uuid = dev_uuid,
    .static_val = auth_data,
    .static_val_len = sizeof(auth_data),
    .output_size = 0,
    .output_actions = 0,
    .output_number = 0,
    .complete = prov_complete,
    .reset = prov_reset,
};

/*
 * @brief Button Pressed Worker Task
 */
/*-----------------------------------------------------------*/
static bool server_publish(struct _switch *sw)
{
    struct bt_mesh_model *mod_srv;
    struct bt_mesh_model_pub *pub_srv;

    mod_srv = mod_srv_sw[sw->sw_num];
    pub_srv = mod_srv->pub;

    /* If unprovisioned, just call the set function.
     * The intent is to have switch-like behavior
     * prior to provisioning. Once provisioned,
     * the button and its corresponding led are no longer
     * associated and act independently. So, if a button is to
     * control its associated led after provisioning, the button
     * must be configured to either publish to the led's unicast
     * address or a group to which the led is subscribed.
     */
    if (primary_addr == BT_MESH_ADDR_UNASSIGNED) {
        NET_BUF_SIMPLE_DEFINE(msg, 1);
        struct bt_mesh_msg_ctx ctx = {
            .addr = sw->sw_num + primary_addr,
        };

        /* This is a dummy message sufficient
         * for the led server
         */
        buffer_add_u8_at_tail(&msg, sw->onoff_state);
        gen_onoff_set_unack(mod_srv, &ctx, &msg);
        return TRUE;
    }
    return FALSE;
}

static void button_pressed_worker(struct _switch *sw)
{
    if (sw->sw_num >= composition.elem_count) {
        log_info("sw_num over elem_count");
        return;
    }

    if (server_publish(sw)) {
        return;
    }
}

void input_key_handler(u8 key_status, u8 key_number)
{
    struct _switch press_switch;

    log_info("key_number=0x%x", key_number);

    if ((key_number == 2) && (key_status == KEY_EVENT_LONG)) {
        log_info("\n  <bt_mesh_reset> \n");
        bt_mesh_reset();
        return;
    }

    switch (key_status) {
    case KEY_EVENT_CLICK:
        log_info("  [KEY_EVENT_CLICK]  ");
        press_switch.sw_num = key_number;
        press_switch.onoff_state = 1;
        button_pressed_worker(&press_switch);
        break;
    case KEY_EVENT_LONG:
        log_info("  [KEY_EVENT_LONG]  ");
        press_switch.sw_num = key_number;
        press_switch.onoff_state = 0;
        button_pressed_worker(&press_switch);
        break;
    case KEY_EVENT_HOLD:
        log_info("  [KEY_EVENT_HOLD]  ");
        break;
    default :
        return;
    }
}

/*
 * @brief Mesh Profile Setup
 */
/*-----------------------------------------------------------*/
static void mesh_init(void)
{
    log_info("--func=%s", __FUNCTION__);

    static_auth_value_calculate();

    int err = bt_mesh_init(&prov, &composition);
    if (err) {
        log_error("Initializing mesh failed (err %d)\n", err);
        return;
    }

    settings_load();

    bt_mesh_prov_enable(BT_MESH_PROV_GATT | BT_MESH_PROV_ADV);
}

void bt_ble_init(void)
{
    u8 bt_addr[6] = {MAC_TO_LITTLE_ENDIAN(CUR_DEVICE_MAC_ADDR)};

    bt_mac_addr_set(bt_addr);

    mesh_setup(mesh_init);
}

#endif /* (CONFIG_MESH_MODEL == SIG_MESH_ALIGENIE_SOCKET) */
