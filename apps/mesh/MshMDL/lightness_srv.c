#include "btstack/bluetooth.h"
#include "system/includes.h"
#include "bt_common.h"
#include "api/sig_mesh_api.h"
#include "model_api.h"

#define LOG_TAG             "[Light_Lightness]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if (CONFIG_MESH_MODEL == SIG_MESH_LIGHT_LIGHTNESS_SERVER)

extern uint32_t btctler_get_rand_from_assign_range(uint32_t rand, uint32_t min, uint32_t max);
extern void pseudo_random_genrate(uint8_t *dest, unsigned size);

/* Light Lightness Server Model Operation Codes */
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_GET				BT_MESH_MODEL_OP_2(0x82, 0x4B)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_SET				BT_MESH_MODEL_OP_2(0x82, 0x4C)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_SET_UNACK			BT_MESH_MODEL_OP_2(0x82, 0x4D)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_STATUS				BT_MESH_MODEL_OP_2(0x82, 0x4E)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_LINEAR_GET			BT_MESH_MODEL_OP_2(0x82, 0x4F)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_LINEAR_SET			BT_MESH_MODEL_OP_2(0x82, 0x50)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_LINEAR_SET_UNACK	BT_MESH_MODEL_OP_2(0x82, 0x51)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_LINEAR_STATUS		BT_MESH_MODEL_OP_2(0x82, 0x52)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_LAST_GET			BT_MESH_MODEL_OP_2(0x82, 0x53)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_LAST_STATUS		BT_MESH_MODEL_OP_2(0x82, 0x54)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_DEFAULT_GET		BT_MESH_MODEL_OP_2(0x82, 0x55)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_DEFAULT_STATUS		BT_MESH_MODEL_OP_2(0x82, 0x56)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_RANGE_GET			BT_MESH_MODEL_OP_2(0x82, 0x56)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_RANGE_STATUS		BT_MESH_MODEL_OP_2(0x82, 0x58)

/* Light Lightness Server Setup Model Operation Codes */
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_DEFAULT_SET			BT_MESH_MODEL_OP_2(0x82, 0x59)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_DEFAULT_SET_UNACK		BT_MESH_MODEL_OP_2(0x82, 0x5A)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_RANGE_SET				BT_MESH_MODEL_OP_2(0x82, 0x5B)
#define BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_RANGE_SET_UNACK		BT_MESH_MODEL_OP_2(0x82, 0x5C)

#define Set_Success				0x00
#define Cannot_Set_Range_Min	0x01
#define Cannot_Set_Range_Max	0x02

struct light_state {
    u16 lightness_actual;
    u16 lightness_linear;
    u16 lightness_default;
    u16 lightness_last;
    u16 lightness_range_min;
    u16 lightness_range_max;
    u8_t led_gpio_pin;
};

struct onoff_state {
    u8_t current;
    u8_t previous;
    u8_t led_gpio_pin;
};

struct light_state dev_light_state =  {
    .lightness_actual = 0,
    .lightness_last = 0,
    .led_gpio_pin = IO_PORTB_07,
};

struct level_state {
    s16 level;
    u8_t led_gpio_pin;
};

extern struct bt_mesh_model root_models[];
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
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_pub_srv, NULL, 2 + 2);

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
};

void light_lightness_state_sycn(u16 actual_lightness)
{
    struct level_state *r_level_state = root_models[2].user_data;
    r_level_state->level = actual_lightness - 32768;

    struct onoff_state *r_onoff_state = root_models[1].user_data;
    if (actual_lightness > 0) {
        r_onoff_state->current = 1;
    } else {
        r_onoff_state->current = 0;
    }
}

static void lightness_get(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf)
{
    NET_BUF_SIMPLE_DEFINE(msg, 2 + 2 + 4);
    struct light_state *light_state = model->user_data;

    log_info("addr 0x%04x actual lightness 0x%02x\n", bt_mesh_model_elem(model)->addr, light_state->lightness_actual);
    bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_STATUS);
    buffer_add_le16_at_tail(&msg, light_state->lightness_actual);

    if (bt_mesh_model_send(model, ctx, &msg, &rsp_msg_cb, (void *)ctx->recv_dst)) {
        log_info("Unable to send lightness Status response\n");
    }
}

static void lightness_set_unack(struct bt_mesh_model *model,
                                struct bt_mesh_msg_ctx *ctx,
                                struct net_buf_simple *buf)
{
    struct light_state *light_state = model->user_data;

    u16 lightness = buffer_pull_le16_from_head(buf);
    if (lightness > light_state->lightness_range_min) {
        if (lightness < light_state->lightness_range_max) {
            light_state->lightness_actual = light_state->lightness_range_max;
        }
    } else {
        light_state->lightness_actual = light_state->lightness_range_min;
    }

    light_state->lightness_last = light_state->lightness_actual;

    light_lightness_state_sycn(light_state->lightness_actual);

    log_info("addr 0x%02x actual lightness 0x%02x\n", bt_mesh_model_elem(model)->addr, light_state->lightness_actual);
}

static void lightness_set(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf)
{
    log_info("lightness_set\n");
    lightness_set_unack(model, ctx, buf);
    lightness_get(model, ctx, buf);
}

static void lightness_linear_get(struct bt_mesh_model *model,
                                 struct bt_mesh_msg_ctx *ctx,
                                 struct net_buf_simple *buf)
{
    NET_BUF_SIMPLE_DEFINE(msg, 2 + 2 + 4);
    struct light_state *light_state = model->user_data;

    log_info("addr 0x%04x lightness linear 0x%02x\n", bt_mesh_model_elem(model)->addr, light_state->lightness_linear);
    bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_LINEAR_STATUS);
    buffer_add_le16_at_tail(&msg, light_state->lightness_linear);

    if (bt_mesh_model_send(model, ctx, &msg, &rsp_msg_cb, (void *)ctx->recv_dst)) {
        log_info("Unable to send lightness linear response\n");
    }
}

static void lightness_linear_set_unack(struct bt_mesh_model *model,
                                       struct bt_mesh_msg_ctx *ctx,
                                       struct net_buf_simple *buf)
{
    struct light_state *light_state = model->user_data;

    light_state->lightness_linear = buffer_pull_le16_from_head(buf);
    log_info("addr 0x%02x state 0x%02x\n", bt_mesh_model_elem(model)->addr, light_state->lightness_linear);

}

static void lightness_linear_set(struct bt_mesh_model *model,
                                 struct bt_mesh_msg_ctx *ctx,
                                 struct net_buf_simple *buf)
{
    log_info("lightness linear set\n");
    lightness_linear_set_unack(model, ctx, buf);
    lightness_linear_get(model, ctx, buf);
}

static void lightness_last_get(struct bt_mesh_model *model,
                               struct bt_mesh_msg_ctx *ctx,
                               struct net_buf_simple *buf)
{
    NET_BUF_SIMPLE_DEFINE(msg, 2 + 2 + 4);
    struct light_state *light_state = model->user_data;

    log_info("addr 0x%04x lightness last 0x%02x\n", bt_mesh_model_elem(model)->addr, light_state->lightness_last);
    bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_LAST_STATUS);
    buffer_add_le16_at_tail(&msg, light_state->lightness_last);

    if (bt_mesh_model_send(model, ctx, &msg, &rsp_msg_cb, (void *)ctx->recv_dst)) {
        log_info("Unable to send lightness last response\n");
    }
}

static void lightness_default_get(struct bt_mesh_model *model,
                                  struct bt_mesh_msg_ctx *ctx,
                                  struct net_buf_simple *buf)
{
    NET_BUF_SIMPLE_DEFINE(msg, 2 + 2 + 4);
    struct light_state *light_state = model->user_data;

    log_info("addr 0x%04x lightness default 0x%02x\n", bt_mesh_model_elem(model)->addr, light_state->lightness_default);
    bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_DEFAULT_STATUS);
    buffer_add_le16_at_tail(&msg, light_state->lightness_default);

    if (bt_mesh_model_send(model, ctx, &msg, &rsp_msg_cb, (void *)ctx->recv_dst)) {
        log_info("Unable to send lightness default response\n");
    }
}

static void lightness_default_set_unack(struct bt_mesh_model *model,
                                        struct bt_mesh_msg_ctx *ctx,
                                        struct net_buf_simple *buf)
{
    struct light_state *light_state = model->user_data;

    light_state->lightness_default = buffer_pull_le16_from_head(buf);
    log_info("addr 0x%02x default 0x%02x\n", bt_mesh_model_elem(model)->addr, light_state->lightness_default);
}

static void lightness_default_set(struct bt_mesh_model *model,
                                  struct bt_mesh_msg_ctx *ctx,
                                  struct net_buf_simple *buf)
{
    log_info("light default set\n");
    lightness_default_set_unack(model, ctx, buf);
    lightness_default_get(model, ctx, buf);
}

static void lightness_range_get(struct bt_mesh_model *model,
                                struct bt_mesh_msg_ctx *ctx,
                                struct net_buf_simple *buf)
{
    NET_BUF_SIMPLE_DEFINE(msg, 2 + 2 + 4);
    struct light_state *light_state = model->user_data;

    log_info("addr 0x%04x lightness range_min 0x%02x, range_max 0x%02x\n", bt_mesh_model_elem(model)->addr, light_state->lightness_range_min, light_state->lightness_range_max);
    bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_RANGE_STATUS);
    buffer_add_u8_at_tail(&msg, Set_Success);
    buffer_add_le16_at_tail(&msg, light_state->lightness_range_min);
    buffer_add_le16_at_tail(&msg, light_state->lightness_range_max);

    if (bt_mesh_model_send(model, ctx, &msg, &rsp_msg_cb, (void *)ctx->recv_dst)) {
        log_info("Unable to send lightness default response\n");
    }
}

static void lightness_range_set_unack(struct bt_mesh_model *model,
                                      struct bt_mesh_msg_ctx *ctx,
                                      struct net_buf_simple *buf)
{
    struct light_state *light_state = model->user_data;

    u16 min  = buffer_pull_le16_from_head(buf);
    u16 max  = buffer_pull_le16_from_head(buf);
    if (light_state->lightness_range_min > light_state->lightness_range_max) {
        log_info("Set lightness range fail, range_max smaller than range_min");
        return;
    }
    light_state->lightness_range_min = min;
    light_state->lightness_range_max = max;
    log_info("addr 0x%04x lightness range_min 0x%02x, range_max 0x%02x\n", bt_mesh_model_elem(model)->addr, light_state->lightness_range_min, light_state->lightness_range_max);

}

static void lightness_range_set(struct bt_mesh_model *model,
                                struct bt_mesh_msg_ctx *ctx,
                                struct net_buf_simple *buf)
{
    log_info("lightness range set\n");
    lightness_range_set_unack(model, ctx, buf);
    lightness_range_get(model, ctx, buf);
}

/*
 * Light Lightness Model Server Op Dispatch Table
 *
 */
const struct bt_mesh_model_op light_lightness_srv_op[] = {
    { BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_GET, 				0, lightness_get},
    { BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_SET, 				0, lightness_set},
    { BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_SET_UNACK, 			0, lightness_set_unack},
    { BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_LINEAR_GET, 			0, lightness_linear_get},
    { BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_LINEAR_SET, 			0, lightness_linear_set},
    { BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_LINEAR_SET_UNACK, 	0, lightness_linear_set_unack},
    { BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_LAST_GET	, 			0, lightness_last_get},
    { BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_DEFAULT_GET, 		0, lightness_default_get},
    { BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_RANGE_GET, 			0, lightness_range_get},
    BT_MESH_MODEL_OP_END,
};

/*
 * Light Lightness Setup Model Server Op Dispatch Table
 *
 */
const struct bt_mesh_model_op light_lightness_setup_srv_op[] = {
    { BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_DEFAULT_SET, 		 0, lightness_default_set},
    { BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_DEFAULT_SET_UNACK, 	 0,	lightness_default_set_unack},
    { BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_RANGE_SET, 		 	 0, lightness_range_set},
    { BT_MESH_MODEL_OP_LIGHT_LIGHTNESS_RANGE_SET_UNACK,   	 0, lightness_range_set_unack},
    BT_MESH_MODEL_OP_END,
};
#endif
