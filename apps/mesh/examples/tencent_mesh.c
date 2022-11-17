#include "btstack/bluetooth.h"
#include "system/includes.h"
#include "bt_common.h"
#include "sha256.h"
#include "api/sig_mesh_api.h"
#include "model_api.h"
#include "unix_timestamp.h"
#include "ble_qiot_template.h"
#include "ble_qiot_common.h"

#define LOG_TAG             "[Mesh-Tencent]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if (CONFIG_MESH_MODEL == SIG_MESH_TENCENT_MESH)

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
                                                BT_MESH_FEAT_RELAY | \
                                                BT_MESH_FEAT_PROXY | \
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
#define BLE_DEV_NAME        'T', 'E', 'C', 'E', 'N', 'T', '_', 'L', 'L'


const uint8_t mesh_name[] = {
    // Name
    BYTE_LEN(BLE_DEV_NAME) + 1, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, BLE_DEV_NAME,
};

/*
 * @brief Conifg Tencent 三元组
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

#define PRODUCT_ID_STRING_SIZE      (sizeof(Product_ID) - 1)
#define MAC_ADDRESS_STRING_SIZE     (sizeof(Mac_Address) * 2)
#define SECRET_STRING_SIZE          (sizeof(Secret) - 1)

#define PRODUCT_ID_TO_CHAR          PRODUCT_ID[0], PRODUCT_ID[1], PRODUCT_ID[2], PRODUCT_ID[3], PRODUCT_ID[4], PRODUCT_ID[5], PRODUCT_ID[6], PRODUCT_ID[7], PRODUCT_ID[8], PRODUCT_ID[9]

#if 1
#define CUR_DEVICE_MAC_ADDR         0x001122334455
#define PRODUCT_ID                  "PGKCRQIHE9"
#define DEVICE_SECRET               "LhxVvs7uaLKSHOJclfSbWA=="
#else
#define CUR_DEVICE_MAC_ADDR         0x112233445566
#define PRODUCT_ID                  "PGKCRQIHE9"
#define DEVICE_SECRET               "dYjvwn+RGw51NI4rhZf7hA=="
#endif

#define LLSYNC_GET_EXPIRATION_TIME(_cur_time) ((_cur_time) + 60)

static void tencent_vendor_update()
{
    printf("vendor_update");
}

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
BT_MESH_MODEL_PUB_DEFINE(vendor_pub_srv, tencent_vendor_update, 3 + 1 + 2 + 4);

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
 */
/*-----------------------------------------------------------*/
#define BT_COMP_ID_LF                           0x013A      // Zhuhai Jieli technology Co.,Ltd
#define BT_MESH_VENDOR_MODEL_ID_SRV             0x013A0000

/*
 * @brief Tencent Vendor Model Operation Codes
 */
/*-----------------------------------------------------------*/
#define VENDOR_MESSAGE_ATTRIBUTE_SET                 BT_MESH_MODEL_OP_3(0xC0, BT_COMP_ID_LF)
#define VENDOR_MESSAGE_ATTRIBUTE_GET                 BT_MESH_MODEL_OP_3(0xC1, BT_COMP_ID_LF)
#define VENDOR_MESSAGE_ATTRIBUTE_SET_UNACKNOWLEDGED  BT_MESH_MODEL_OP_3(0xC2, BT_COMP_ID_LF)
#define VENDOR_MESSAGE_ATTRIBUTE_STATUS              BT_MESH_MODEL_OP_3(0xC3, BT_COMP_ID_LF)
#define VENDOR_MESSAGE_ATTRIBUTE_INDICATION          BT_MESH_MODEL_OP_3(0xC4, BT_COMP_ID_LF)
#define VENDOR_MESSAGE_ATTRIBUTE_CONFIRMATION        BT_MESH_MODEL_OP_3(0xC5, BT_COMP_ID_LF)

/*
 * @brief Tencent Vendor Model Message Struct
 *
 */
/*-----------------------------------------------------------*/
#define ATTR_TYPE_ERROR_CODE                0xFF00
#define ATTR_TYPE_VERSION_NUM               0xFF01
#define ATTR_TYPE_DEV_BIND                  0xFF02
#define ATTR_TYPE_ONOFF                     0xF000

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
    .beacon         = BT_MESH_BEACON_DISABLED,
    .default_ttl    = 3,
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

static uint8_t mac_name[14];
static uint16_t llsync_group_addr = 0xf000;
static uint16_t llsync_node_addr = 0x0001;
static struct bt_mesh_model vendor_server_models[];

struct bt_mesh_model   sg_model;
struct bt_mesh_msg_ctx sg_ctx;

static uint8_t hex_to_ascii(u8 in)
{
    if (in >= 0 && in <= 9) {
        return in + '0';
    } else if (in >= 0xa && in <= 0xf) {
        return in - 0xa + 'a';
    } else {
        printf("joy ascii to hex error, data:0x%x", in);
        return 0;
    }
}

void get_mesh_adv_name(u8 *len, u8 **data)
{
    *len = 12 + 2;

    mac_name[0] = 13;
    mac_name[1] = BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME;

    for (int i = 0; i < 6; i++) {
        mac_name[2 + 2 * i] = hex_to_ascii((CUR_DEVICE_MAC_ADDR >> (4 * (12 - 2 * i) - 4)) & 0xf);
        mac_name[2 + 2 * i + 1] = hex_to_ascii((CUR_DEVICE_MAC_ADDR >> (4 * (12 - 2 * i) - 8)) & 0xf);
    }
    *data = mac_name;
}

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
    //struct bt_mesh_msg_ctx *ctx = cb_data;
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

/*
 * @brief Tencent Vendor Model Message Handlers
 */
/*-----------------------------------------------------------*/
void llsync_vendor_attr_send(uint32_t opcode, void *buf, u16 len)
{
    printf("llsync_vendor_attr_send len:%d", len);
    NET_BUF_SIMPLE_DEFINE(msg, 3 + len + TRANSMIC_SIZE);

    bt_mesh_model_msg_init(&msg, opcode);
    buffer_memcpy(&msg, buf, len);

    log_info_hexdump(msg.data, msg.len);

    if (bt_mesh_model_send(&sg_model, &sg_ctx, &msg, NULL, NULL)) {
        log_error("Unable to send Status response\n");
    }
    return;
}

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
}

static void gen_onoff_set(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf)
{
    log_info("gen_onoff_set\n");

    gen_onoff_set_unack(model, ctx, buf);
    gen_onoff_get(model, ctx, buf);
}

static void vendor_attr_get(struct bt_mesh_model *model,
                            struct bt_mesh_msg_ctx *ctx,
                            struct net_buf_simple *buf)
{
    memcpy(&sg_model, model, sizeof(struct bt_mesh_model));
    memcpy(&sg_ctx, ctx, sizeof(struct bt_mesh_msg_ctx));

    log_info("receive vendor_attr_get, len except opcode =0x%x", buf->len);
    log_info_hexdump(buf->data, buf->len);
    llsync_mesh_recv_data_handle(VENDOR_MESSAGE_ATTRIBUTE_GET, buf->data, buf->len);
}

static void vendor_attr_set(struct bt_mesh_model *model,
                            struct bt_mesh_msg_ctx *ctx,
                            struct net_buf_simple *buf)
{
    memcpy(&sg_model, model, sizeof(struct bt_mesh_model));
    memcpy(&sg_ctx, ctx, sizeof(struct bt_mesh_msg_ctx));

    log_info("receive vendor_attr_set, len except opcode =0x%x", buf->len);
    log_info_hexdump(buf->data, buf->len);
    llsync_mesh_recv_data_handle(VENDOR_MESSAGE_ATTRIBUTE_SET, buf->data, buf->len);
}

static void vendor_attr_set_unack(struct bt_mesh_model *model,
                                  struct bt_mesh_msg_ctx *ctx,
                                  struct net_buf_simple *buf)
{
    memcpy(&sg_model, model, sizeof(struct bt_mesh_model));
    memcpy(&sg_ctx, ctx, sizeof(struct bt_mesh_msg_ctx));

    log_info("receive vendor_attr_set_unack, len except opcode =0x%x", buf->len);
    log_info_hexdump(buf->data, buf->len);
    llsync_mesh_recv_data_handle(VENDOR_MESSAGE_ATTRIBUTE_SET_UNACKNOWLEDGED, buf->data, buf->len);
}

static void vendor_attr_cfm(struct bt_mesh_model *model,
                            struct bt_mesh_msg_ctx *ctx,
                            struct net_buf_simple *buf)
{
    memcpy(&sg_model, model, sizeof(struct bt_mesh_model));
    memcpy(&sg_ctx, ctx, sizeof(struct bt_mesh_msg_ctx));

    log_info("receice vendor_attr_confirm\r\n");
    log_info_hexdump(buf->data, buf->len);
    llsync_mesh_recv_data_handle(VENDOR_MESSAGE_ATTRIBUTE_CONFIRMATION, buf->data, buf->len);
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
 * @brief Vendor Model Server Op Dispatch Table
 */
/*-----------------------------------------------------------*/
static const struct bt_mesh_model_op vendor_srv_op[] = {
    { VENDOR_MESSAGE_ATTRIBUTE_GET,     			ACCESS_OP_SIZE, vendor_attr_get },
    { VENDOR_MESSAGE_ATTRIBUTE_SET,     			ACCESS_OP_SIZE, vendor_attr_set },
    { VENDOR_MESSAGE_ATTRIBUTE_SET_UNACKNOWLEDGED,  ACCESS_OP_SIZE, vendor_attr_set_unack },
    { VENDOR_MESSAGE_ATTRIBUTE_CONFIRMATION, 		1, 				vendor_attr_cfm },
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
    BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SRV, gen_onoff_srv_op, &gen_onoff_pub_srv, &onoff_state[0]),
};

static struct bt_mesh_model vendor_server_models[] = {
    BT_MESH_MODEL_VND(BT_COMP_ID_LF, BT_MESH_VENDOR_MODEL_ID_SRV, vendor_srv_op, &vendor_pub_srv, &onoff_state[0]),
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

/*
 * @brief Tencent UUID格式
 */
/*-----------------------------------------------------------*/
static const u8 dev_uuid[16] = {
    0x3A, 0x01,  // CID
    0x01,   //flag
    0x00,   //flag2
    PRODUCT_ID_TO_CHAR, // ProductID
    0x00, 0x00 // RFU
};

/*
 * @brief Tencent 设备配网流程
 */
/*-----------------------------------------------------------*/
static u8 auth_data[16];

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

void llsync_led_set(uint8_t onoff)
{
    onoff_state[0].current = onoff;
    gpio_pin_write(onoff_state[0].led_gpio_pin, onoff_state[0].current);
}

static void llsync_led_switch()
{
    ble_property_t *sg_ble_property_array = llsync_mesh_property_array_get();
    onoff_state[0].current = !onoff_state[0].current;
    printf("key_press led set to %d", onoff_state[0].current);

    gpio_pin_write(onoff_state[0].led_gpio_pin, onoff_state[0].current);
    sg_ble_property_array[0].set_cb(&onoff_state[0].current, 1);
    llsync_mesh_vendor_data_report();
}

void input_key_handler(u8 key_status, u8 key_number)
{
    struct _switch press_switch;
    log_info("key_number=0x%x", key_number);

    if ((key_number == 3) && (key_status == KEY_EVENT_CLICK)) {
        log_info("\n  <bt_mesh_reset> \n");
        ble_app_disconnect();
        bt_mesh_reset();
        return;
    }
    if ((key_number == 0) && (key_status == KEY_EVENT_CLICK)) {
        llsync_led_switch();
        return;
    }
    switch (key_status) {
    case KEY_EVENT_CLICK:
        log_info("  [KEY_EVENT_CLICK]  ");
        break;
    case KEY_EVENT_LONG:
        log_info("  [KEY_EVENT_LONG]  ");
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

    llsync_mesh_init();

    int err = bt_mesh_init(&prov, &composition);
    if (err) {
        log_error("Initializing mesh failed (err %d)\n", err);
        return;
    }

    settings_load();
    bt_mesh_prov_enable(BT_MESH_PROV_GATT | BT_MESH_PROV_ADV);
    mesh_proxy_connect_finish_callback_register(llsync_mesh_vendor_data_report);
}

void bt_ble_init(void)
{
    u8 bt_addr[6] = {MAC_TO_LITTLE_ENDIAN(CUR_DEVICE_MAC_ADDR)};

    bt_mac_addr_set(bt_addr);

    mesh_setup(mesh_init);
}

#endif /* (CONFIG_MESH_MODEL == SIG_MESH_TENCENT_MESH) */

