#include "btstack/bluetooth.h"
#include "system/includes.h"
#include "bt_common.h"
#include "api/sig_mesh_api.h"
#include "model_api.h"

#define LOG_TAG             "[Mesh-OnOff_cli]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if (CONFIG_MESH_MODEL == SIG_MESH_LIGHT_LIGHTNESS_SERVER)
extern void gpio_pin_write(u8_t led_index, u8_t onoff);

extern u16_t primary_addr;
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
BT_MESH_MODEL_PUB_DEFINE(gen_onoff_pub_cli, NULL, 2 + 2);

/* Model Operation Codes */
#define BT_MESH_MODEL_OP_GEN_ONOFF_SET			BT_MESH_MODEL_OP_2(0x82, 0x02)
#define BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK	BT_MESH_MODEL_OP_2(0x82, 0x03)
#define BT_MESH_MODEL_OP_GEN_ONOFF_STATUS		BT_MESH_MODEL_OP_2(0x82, 0x04)

/* LED NUMBER */
#define LED0_GPIO_PIN           0

static u8_t trans_id;

struct _switch {
    u8_t sw_num;
    u8_t onoff_state;
};

static struct onoff_state {
    u8_t current;
    u8_t previous;
    u8_t led_gpio_pin;
};

static struct onoff_state onoff_state[] = {
    { .led_gpio_pin = LED0_GPIO_PIN },
};

static const u8 led_use_port[1] = {

    IO_PORTB_07,

};

/*
 * Models in an element must have unique op codes.
 *
 * The mesh stack dispatches a message to the first model in an element
 * that is also bound to an app key and supports the op code in the
 * received message.
 *
 */
static void gen_onoff_status(struct bt_mesh_model *model,
                             struct bt_mesh_msg_ctx *ctx,
                             struct net_buf_simple *buf)
{
    u8_t	state;

    state = buffer_pull_u8_from_head(buf);

    log_info("Node 0x%04x OnOff status from 0x%04x with state 0x%02x\n",
             bt_mesh_model_elem(model)->addr, ctx->addr, state);
}

/*
 * OnOff Model Client Op Dispatch Table
 */
const struct bt_mesh_model_op gen_onoff_cli_op[] = {
    { BT_MESH_MODEL_OP_GEN_ONOFF_STATUS, 1, gen_onoff_status },
    BT_MESH_MODEL_OP_END,
};

/*
 * Generic OnOff Model Server Message Handlers
 *
 * Mesh Model Specification 3.1.1
 */
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

static bool server_publish(struct _switch *sw)
{
    int err;
    struct bt_mesh_model *mod_srv;
    struct bt_mesh_model_pub *pub_srv;

    mod_srv = &root_models[2];
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

void client_publish(struct _switch *sw)
{
    int err;
    struct bt_mesh_model *mod_cli;
    struct bt_mesh_model_pub *pub_cli;

    mod_cli = &root_models[2];
    pub_cli = mod_cli->pub;

    log_info("publish to 0x%04x onoff 0x%04x sw->sw_num 0x%04x\n",
             pub_cli->addr, sw->onoff_state, sw->sw_num);

    bt_mesh_model_msg_init(pub_cli->msg,
                           BT_MESH_MODEL_OP_GEN_ONOFF_SET);
    buffer_add_u8_at_tail(pub_cli->msg, sw->onoff_state);
    buffer_add_u8_at_tail(pub_cli->msg, trans_id++);

    err = bt_mesh_model_publish(mod_cli);
    if (err) {
        log_info("bt_mesh_model_publish err %d\n", err);
    }
}

/*
 * Button Pressed Worker Task
 */
void button_pressed_worker(struct _switch *sw)
{
    if (root_models[2].pub->addr == BT_MESH_ADDR_UNASSIGNED) {
        log_info("publish addr is not bind\n");
        return;
    }
    client_publish(sw);

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
#endif
