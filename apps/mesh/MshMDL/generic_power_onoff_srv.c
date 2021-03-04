#include "btstack/bluetooth.h"
#include "system/includes.h"
#include "bt_common.h"
#include "api/sig_mesh_api.h"
#include "model_api.h"

#define LOG_TAG             "[On_Power_Up]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if (CONFIG_MESH_MODEL == SIG_MESH_LIGHT_LIGHTNESS_SERVER)

extern uint32_t btctler_get_rand_from_assign_range(uint32_t rand, uint32_t min, uint32_t max);
extern void pseudo_random_genrate(uint8_t *dest, unsigned size);


/* Generic Power OnOff Model Operation Codes */
#define BT_MESH_MODEL_OP_GEN_ONPOWERUP_GET			BT_MESH_MODEL_OP_2(0x82, 0x11)
#define BT_MESH_MODEL_OP_GEN_ONPOWERUP_STATUS		BT_MESH_MODEL_OP_2(0x82, 0x12)

/* Generic Power OnOff Setup Model Operation Codes */
#define BT_MESH_MODEL_OP_GEN_ONPOWERUP_SET			BT_MESH_MODEL_OP_2(0x82, 0x13)
#define BT_MESH_MODEL_OP_GEN_ONPOWERUP_SET_UNACK	BT_MESH_MODEL_OP_2(0x82, 0x14)

struct onpowerup_state {
    u8_t onpowerup;
};

struct onpowerup_state dev_onpowerup_state;

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

static void gen_onpowerup_get(struct bt_mesh_model *model,
                              struct bt_mesh_msg_ctx *ctx,
                              struct net_buf_simple *buf)
{
    NET_BUF_SIMPLE_DEFINE(msg, 2 + 1 + 4);
    struct onpowerup_state *onpowerup_state = model->user_data;

    log_info("addr 0x%04x onpowerup 0x%02x\n",
             bt_mesh_model_elem(model)->addr, onpowerup_state->onpowerup);
    bt_mesh_model_msg_init(&msg, BT_MESH_MODEL_OP_GEN_ONPOWERUP_STATUS);
    buffer_add_u8_at_tail(&msg, onpowerup_state->onpowerup);

    if (bt_mesh_model_send(model, ctx, &msg, &rsp_msg_cb, (void *)ctx->recv_dst)) {
        log_info("Unable to send onpowerup Status response\n");
    }
}

static void gen_onpowerup_set_unack(struct bt_mesh_model *model,
                                    struct bt_mesh_msg_ctx *ctx,
                                    struct net_buf_simple *buf)
{
    struct onpowerup_state *onpowerup_state = model->user_data;

    onpowerup_state->onpowerup = buffer_pull_u8_from_head(buf);
    log_info("addr 0x%02x onpowerup state 0x%02x\n",
             bt_mesh_model_elem(model)->addr, onpowerup_state->onpowerup);
    /* log_info_hexdump((u8 *)onoff_state, sizeof(*onoff_state)); */


}

static void gen_onpowerup_set(struct bt_mesh_model *model,
                              struct bt_mesh_msg_ctx *ctx,
                              struct net_buf_simple *buf)
{
    log_info("gen_onpowerup_set\n");

    gen_onpowerup_set_unack(model, ctx, buf);
    gen_onpowerup_get(model, ctx, buf);
}


/*
 * Power OnOff Model Server Op Dispatch Table
 *
 */
const struct bt_mesh_model_op gen_onpowerup_srv_op[] = {
    { BT_MESH_MODEL_OP_GEN_ONPOWERUP_GET, 0, gen_onpowerup_get },
    BT_MESH_MODEL_OP_END,
};

/*
 * Power OnOff Setup Model Server Op Dispatch Table
 *
 */
const struct bt_mesh_model_op gen_onpowerup_setup_srv_op[] = {
    { BT_MESH_MODEL_OP_GEN_ONPOWERUP_SET, 		0, gen_onpowerup_set },
    { BT_MESH_MODEL_OP_GEN_ONPOWERUP_SET_UNACK, 0, gen_onpowerup_set_unack },
    BT_MESH_MODEL_OP_END,
};
#endif
