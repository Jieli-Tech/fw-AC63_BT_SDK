#include "btstack/bluetooth.h"
#include "system/includes.h"
#include "bt_common.h"
#include "api/sig_mesh_api.h"
#include "model_api.h"

#define LOG_TAG             "[Gen_Level]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"
#if (CONFIG_MESH_MODEL == SIG_MESH_LIGHT_LIGHTNESS_SERVER)

extern uint32_t btctler_get_rand_from_assign_range(uint32_t rand, uint32_t min, uint32_t max);
extern void pseudo_random_genrate(uint8_t *dest, unsigned size);
extern void set_led_duty(u16 duty);

_WEAK_ void set_led_duty(u16 duty)
{

}

/* Generic Level Model Operation Codes */
#define BT_MESH_MODEL_OP_GEN_LEVEL_GET			BT_MESH_MODEL_OP_2(0x82, 0x05)
#define BT_MESH_MODEL_OP_GEN_LEVEL_SET			BT_MESH_MODEL_OP_2(0x82, 0x06)
#define BT_MESH_MODEL_OP_GEN_LEVEL_SET_UNACK	BT_MESH_MODEL_OP_2(0x82, 0x07)
#define BT_MESH_MODEL_OP_GEN_LEVEL_STATUS		BT_MESH_MODEL_OP_2(0x82, 0x08)

#define BT_MESH_MODEL_OP_GEN_DELTA_SET			BT_MESH_MODEL_OP_2(0x82, 0x09)
#define BT_MESH_MODEL_OP_GEN_DELTA_SET_UNACK	BT_MESH_MODEL_OP_2(0x82, 0x0A)

#define BT_MESH_MODEL_OP_GEN_MOVE_SET			BT_MESH_MODEL_OP_2(0x82, 0x0B)
#define BT_MESH_MODEL_OP_GEN_MOVE_SET_UNACK		BT_MESH_MODEL_OP_2(0x82, 0x0C)

struct level_state {
    s16 level;
    u8_t led_gpio_pin;
};

struct level_state light_level_state = {
    .level = -32768,
    .led_gpio_pin = IO_PORTB_07,
};

struct onoff_state {
    u8_t current;
    u8_t previous;
    u8_t led_gpio_pin;
};

struct light_state {
    u16 lightness_actual;
    u16 lightness_linear;
    u16 lightness_default;
    u16 lightness_last;
    u16 lightness_range_min;
    u16 lightness_range_max;
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

void generic_level_state_sync(s16 level)
{
    struct light_state *r_light_state = root_models[4].user_data;
    r_light_state->lightness_actual = level + 32768;

    struct onoff_state *r_onoff_state = root_models[1].user_data;
    if (r_light_state->lightness_actual > 0) {
        r_onoff_state->current = 1;
    } else {
        r_onoff_state->current = 0;
    }
}

static void gen_level_get(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf)
{
    NET_BUF_SIMPLE_DEFINE(msg, 2 + 2 + 4);
    struct level_state *level_state = model->user_data;

    log_info("addr 0x%04x level %d\n", bt_mesh_model_elem(model)->addr, level_state->level);
    bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_OP_GEN_LEVEL_STATUS);
    buffer_add_le16_at_tail(&msg, level_state->level);

    if (bt_mesh_model_send(model, ctx, &msg, &rsp_msg_cb, (void *)ctx->recv_dst)) {
        log_info("Unable to send level Status response\n");
    }
}

static void gen_level_set_unack(struct bt_mesh_model *model,
                                struct bt_mesh_msg_ctx *ctx,
                                struct net_buf_simple *buf)
{
    struct level_state *level_state = model->user_data;

    level_state->level = buffer_pull_le16_from_head(buf);
    generic_level_state_sync(level_state->level);
    s16 bright = (32768 + level_state->level) * 10000 / 65536;
    set_led_duty(bright);
    log_info("addr 0x%02x level %d, bright = %d, is %%%d\n", bt_mesh_model_elem(model)->addr, level_state->level, bright, bright / 100);
    /* log_info_hexdump((u8 *)onoff_state, sizeof(*onoff_state)); */
}

static void gen_level_set(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf)
{
    log_info("gen_level_set\n");
    log_info("buf len = %d, buf size = %d\n", buf->len, buf->size);

    gen_level_set_unack(model, ctx, buf);
    gen_level_get(model, ctx, buf);
}

static void gen_delta_set_unack(struct bt_mesh_model *model,
                                struct bt_mesh_msg_ctx *ctx,
                                struct net_buf_simple *buf)
{
    struct level_state *level_state = model->user_data;

    level_state->level -= buffer_pull_le16_from_head(buf);
    generic_level_state_sync(level_state->level);
    s16 bright = (32768 + level_state->level) * 10000 / 65536;
    log_info("light bright set to %%%d\n", bright / 100);
    set_led_duty(bright);
    log_info("addr 0x%02x level 0x%02x\n",
             bt_mesh_model_elem(model)->addr, level_state->level);
    /* log_info_hexdump((u8 *)onoff_state, sizeof(*onoff_state)); */
}

static void gen_delta_set(struct bt_mesh_model *model,
                          struct bt_mesh_msg_ctx *ctx,
                          struct net_buf_simple *buf)
{
    log_info("gen_delta_set\n");

    gen_delta_set_unack(model, ctx, buf);
    gen_level_get(model, ctx, buf);
}

static void gen_move_set_unack(struct bt_mesh_model *model,
                               struct bt_mesh_msg_ctx *ctx,
                               struct net_buf_simple *buf)
{
    struct level_state *level_state = model->user_data;

    level_state->level -= buffer_pull_le16_from_head(buf);
    s16 bright = (32768 + level_state->level) * 10000 / 65536;
    generic_level_state_sync(level_state->level);
    set_led_duty(bright);
    log_info("addr 0x%02x level %d\n",
             bt_mesh_model_elem(model)->addr, level_state->level);
    /* log_info_hexdump((u8 *)onoff_state, sizeof(*onoff_state)); */
}

static void gen_move_set(struct bt_mesh_model *model,
                         struct bt_mesh_msg_ctx *ctx,
                         struct net_buf_simple *buf)
{
    log_info("gen_move_set\n");

    gen_move_set_unack(model, ctx, buf);
    gen_level_get(model, ctx, buf);
}

/*
 * Generic Level Model Server Op Dispatch Table
 *
 */
const struct bt_mesh_model_op gen_level_server[] = {
    { BT_MESH_MODEL_OP_GEN_LEVEL_GET, 		0, gen_level_get},
    { BT_MESH_MODEL_OP_GEN_LEVEL_SET, 		0, gen_level_set},
    { BT_MESH_MODEL_OP_GEN_LEVEL_SET_UNACK, 0, gen_level_set_unack},
    { BT_MESH_MODEL_OP_GEN_DELTA_SET, 		0, gen_delta_set},
    { BT_MESH_MODEL_OP_GEN_DELTA_SET_UNACK, 0, gen_delta_set_unack},
    { BT_MESH_MODEL_OP_GEN_MOVE_SET, 		0, gen_move_set},
    { BT_MESH_MODEL_OP_GEN_MOVE_SET_UNACK,  0, gen_move_set_unack},
    BT_MESH_MODEL_OP_END,
};
#endif
