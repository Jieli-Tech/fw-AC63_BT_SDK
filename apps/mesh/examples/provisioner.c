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
#include "adaptation.h"
#include "net.h"
#include "provisioner_config.h"
#include "foundation.h"

#define LOG_TAG             "[Mesh-Provisioner]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if (CONFIG_MESH_MODEL == SIG_MESH_PROVISIONER)

extern u16_t primary_addr;
extern void mesh_setup(void (*init_cb)(void));
extern void gpio_pin_write(u8_t led_index, u8_t onoff);
extern void bt_mac_addr_set(u8 *bt_addr);
extern void prov_complete(u16_t net_idx, u16_t addr);
extern void prov_reset(void);


static void provisioner_node_configuration(void);
/**
 * @brief Config current node features(Relay/Proxy/Friend/Low Power)
 */
/*-----------------------------------------------------------*/
#define BT_MESH_FEAT_SUPPORTED_TEMP         ( \
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
 * @brief Conifg complete local name
 */
/*-----------------------------------------------------------*/
#define BLE_DEV_NAME        'P', 'r', 'o', 'v', 'i', 't', 'i', 'o', 'n', 'e', 'r'

const uint8_t mesh_name[] = {
    // Name
    BYTE_LEN(BLE_DEV_NAME) + 1, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME, BLE_DEV_NAME,
};

void get_mesh_adv_name(u8 *len, u8 **data)
{
    *len = sizeof(mesh_name);

    *data = mesh_name;
}

/**
 * @brief Conifg MAC of current demo
 */
/*-----------------------------------------------------------*/
#define CUR_DEVICE_MAC_ADDR         		0x112233440000

const u8_t prov_net_key[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

const u8_t prov_dev_key[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

const u8_t prov_app_key[16] = {
    0x06, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    0x06, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
};

static u8_t flags;
static struct bt_mesh_comp prov_comp;
static uint8_t unprov_uuid[16];

typedef struct {
    u16_t net_idx;
    u16_t addr;
    u8_t num_elem;
    u8_t config_step;
    u8_t config_failed_cnt;
    u8_t uuid[16];
} __prov_node;

static __prov_node curr_pair_node;
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

/* Company Identifiers (see Bluetooth Assigned Numbers) */
#define BT_COMP_ID_LF           0x05D6// Zhuhai Jieli technology Co.,Ltd

/*
 * Server Configuration Declaration
 */
static struct bt_mesh_cfg_srv cfg_srv = {
    .relay          = BT_MESH_FEATURES_GET(BT_MESH_FEAT_RELAY),
    .frnd           = BT_MESH_FEATURES_GET(BT_MESH_FEAT_FRIEND),
    .gatt_proxy     = BT_MESH_FEATURES_GET(BT_MESH_FEAT_PROXY),
    .beacon         = BT_MESH_BEACON_DISABLED,
    .default_ttl    = 7,
};

static struct bt_mesh_cfg_cli cfg_cli;


/*
 * 给配网者添加generic_onoff_server模型,用于测试实例的待配网设备是否配置成功
 *
 * 不需要可以去掉
 */
#if 0

BT_MESH_MODEL_PUB_DEFINE(gen_onoff_pub_srv, NULL, 2 + 2);

/* Model Operation Codes */
#define BT_MESH_MODEL_OP_GEN_ONOFF_GET			BT_MESH_MODEL_OP_2(0x82, 0x01)
#define BT_MESH_MODEL_OP_GEN_ONOFF_SET			BT_MESH_MODEL_OP_2(0x82, 0x02)
#define BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK	BT_MESH_MODEL_OP_2(0x82, 0x03)
#define BT_MESH_MODEL_OP_GEN_ONOFF_STATUS		BT_MESH_MODEL_OP_2(0x82, 0x04)

#define LED0_GPIO_PIN           0

struct onoff_state {
    u8_t current;
    u8_t previous;
    u8_t led_gpio_pin;
};

static struct onoff_state onoff_state[] = {
    { .led_gpio_pin = LED0_GPIO_PIN },
};

const u8 led_use_port[] = {
    IO_PORTA_01,
};

static void respond_messsage_schedule(u16 *delay, u16 *duration, void *cb_data)
{
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
 * OnOff Model Server Op Dispatch Table
 *
 */
static const struct bt_mesh_model_op gen_onoff_srv_op[] = {
    { BT_MESH_MODEL_OP_GEN_ONOFF_GET, 0, gen_onoff_get },
    { BT_MESH_MODEL_OP_GEN_ONOFF_SET, 2, gen_onoff_set },
    { BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK, 2, gen_onoff_set_unack },
    BT_MESH_MODEL_OP_END,
};
#endif
// test---------------------------------------------------------------------

/*
 *
 * Element Model Declarations
 *
 * Element 0 Root Models
 */
static struct bt_mesh_model root_models[] = {
    BT_MESH_MODEL_CFG_SRV(&cfg_srv),
    BT_MESH_MODEL_CFG_CLI(&cfg_cli), // default for self-configuration network
    //BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, gen_onoff_srv_op, &gen_onoff_pub_srv, &onoff_state[0]),
};

/*
 * Root and Secondary Element Declarations
 */
static struct bt_mesh_elem elements[] = {
    BT_MESH_ELEM(0, root_models, BT_MESH_MODEL_NONE),
};

static const struct bt_mesh_comp composition = {
    .cid = BT_COMP_ID_LF,
    .elem = elements,
    .elem_count = ARRAY_SIZE(elements),
};

static void node_configuration_error()
{
    log_error("node configuration error!");
    memset(&curr_pair_node, 0, sizeof(curr_pair_node));
    cfg_cli.op_pending = 0;
    cfg_cli.op_param = NULL;
    bt_mesh_tx_reset();
}

/*
 * 节点配置回应检查,没有回应会重发配置消息
 */
static void node_configuration_check(u32_t opcode)
{
    printf("mesh_configuration_check:0x%x", opcode);
    if (cfg_cli.op_pending != opcode) {
        log_info("check pass");
    } else {
        // 重发超过10次放弃配置节点
        if (curr_pair_node.config_failed_cnt < 10) {
            cfg_cli.op_pending = 0;
            cfg_cli.op_param = NULL;
            log_info("not get the configuration msg rsp:0x%x, resend", opcode);
            bt_mesh_tx_reset();
            provisioner_node_configuration();
            curr_pair_node.config_failed_cnt++;
        } else {
            node_configuration_error();
        }
    }
}

/*
 * 节点配置确认定时器
 */
static void mesh_set_configuration_check_timer(u32_t opcode)
{
    // 节点配置消息发出后3.5秒校验是否有收到回应
    sys_timeout_add(opcode, node_configuration_check, 3500);
}

// 发送配置消息失败后重发
static void config_msg_resend()
{
    if (curr_pair_node.config_failed_cnt < 10) {
        curr_pair_node.config_failed_cnt++;
        sys_timeout_add(NULL, provisioner_node_configuration, 1500);
    } else {
        node_configuration_error();
    }
}

/*
 * 节点配置状态机,按步骤配置节点
 *
 * 此处可自定义新添加节点的配置内容
 */
static void provisioner_node_configuration(void)
{
    int err = 0;
    static uint8_t status;

    log_info("provisioner_node_configuration, step:%d", curr_pair_node.config_step);
    switch (curr_pair_node.config_step) {
    case 0:
        err = bt_mesh_cfg_app_key_add(curr_pair_node.net_idx, curr_pair_node.addr, curr_pair_node.net_idx, 0, prov_app_key, &status);
        if (err) {
            config_msg_resend();
            return;
        }
        mesh_set_configuration_check_timer(OP_APP_KEY_STATUS);
        break;
    case 1:
        err = bt_mesh_cfg_mod_app_bind(curr_pair_node.net_idx, curr_pair_node.addr, curr_pair_node.addr, 0, BT_MESH_MODEL_ID_GEN_ONOFF_CLI, &status);
        if (err) {
            config_msg_resend();
            return;
        }
        mesh_set_configuration_check_timer(OP_MOD_APP_STATUS);
        break;
    case 2:
        struct bt_mesh_cfg_mod_pub pub = {
            .addr = PROV_GROUP_ADDR,
            .app_idx = 0,
            .cred_flag = 0,
            .ttl = 7,
            .period = 0,
            .transmit = 0,
        };
        err = bt_mesh_cfg_mod_pub_set(curr_pair_node.net_idx, curr_pair_node.addr, curr_pair_node.addr, BT_MESH_MODEL_ID_GEN_ONOFF_CLI, &pub, &status);
        if (err) {
            config_msg_resend();
            return;
        }
        mesh_set_configuration_check_timer(OP_MOD_PUB_STATUS);
        break;
    default:
        break;
    }
    if (curr_pair_node.config_step == 3) {
        log_info("node Configuration finish!");
        memset(&curr_pair_node, 0, sizeof(curr_pair_node));
    }
}

/*
 * 新节点配网完成回调,开始节点配置
 */
static void provisioner_node_added(u16_t net_idx, u8_t uuid[16], u16_t addr, u8_t num_elem)
{
    log_info("Add new node!");

    log_info("addr:0x%04x", addr);
    log_info("net_idx:0x%x", net_idx);
    log_info("num_elem:%d", num_elem);
    log_info("uuid:");
    put_buf(uuid, 16);

    curr_pair_node.net_idx = net_idx;
    curr_pair_node.addr = addr;
    curr_pair_node.num_elem = num_elem;
    memcpy(curr_pair_node.uuid, uuid, 16);

    curr_pair_node.config_step = 0;
    curr_pair_node.config_failed_cnt = 0;
    provisioner_node_configuration();
}

/*
 * 扫描到未配网设备beacon回调
 */
static void get_unprov_beacon(u8_t uuid[16],
                              bt_mesh_prov_oob_info_t oob_info,
                              u32_t *uri_hash)
{
    printf("get_unprov_beacon, uuid:%s", bt_hex(uuid, 16));
    memcpy(unprov_uuid, uuid, 16);
}

static const u8_t dev_uuid[16] = {MAC_TO_LITTLE_ENDIAN(CUR_DEVICE_MAC_ADDR)};
static const struct bt_mesh_prov prov = {
    .uuid = dev_uuid,
#if 0
    .output_size = 6,
    .output_actions = (BT_MESH_DISPLAY_NUMBER | BT_MESH_DISPLAY_STRING),
    .output_number = output_number,
    .output_string = output_string,
#else
    .output_size = 0,
    .output_actions = 0,
    .output_number = 0,
#endif
    .complete = prov_complete,
    .reset = prov_reset,
    .node_added = provisioner_node_added,
    .unprovisioned_beacon = get_unprov_beacon,
};

static u8_t show_node(struct bt_mesh_cdb_node *node,
                      void *user_data)
{
    printf("\n[node addr = 0x%x]\n", node->addr);
    printf("uuid = %s\n", bt_hex(node->uuid, 6));
    printf("net_idx = 0x%x\n", node->net_idx);
    printf("num_elem = %d\n", node->num_elem);
    printf("dev_key = %s\n", bt_hex(node->dev_key, 16));

    return 1;
}

/*
 * 显示所有配网的设备
 */
static void Provisioner_show_cdb_node(void)
{
    printf("Provisioner_show_cdb_node\n");

    bt_mesh_cdb_node_foreach(show_node, NULL);
}

static u8_t del_node(struct bt_mesh_cdb_node *node,
                     void *user_data)
{
    printf("\n<Del> [node addr = 0x%x]\n", node->addr);

    bt_mesh_cdb_node_del(node, true);

    return 1;
}

/*
 * 清除配网节点记录
 */
static void clear_store_node(void)
{
    log_info("clear_store_node");
    clear_rpl();
    bt_mesh_cdb_node_foreach(del_node, NULL);
}

static u8_t get_store_node_num(struct bt_mesh_cdb_node *node,
                               void *user_data)
{
    *(u8_t *)user_data += 1;
    return 1;
}

/*
 * 发起配网
 */
int bt_mesh_provision_adv(const uint8_t uuid[16], uint16_t net_idx, uint16_t addr,
                          uint8_t attention_duration)
{
    u8_t node_cnt = 0;
    bt_mesh_cdb_node_foreach(get_store_node_num, &node_cnt);

    if (node_cnt >= CONFIG_BT_MESH_CDB_NODE_COUNT) {
        log_error("node store is full!");
        return -1;
    }

    if (bt_mesh_cdb_subnet_get(net_idx) == NULL) {
        log_error("subnet is NULL!");
        return -EINVAL;
    }

    if (IS_ENABLED(CONFIG_BT_MESH_PROVISIONER) && IS_ENABLED(CONFIG_BT_MESH_PB_ADV)) {
        return bt_mesh_pb_adv_open(uuid, net_idx, addr,
                                   attention_duration);
    }

    log_error("provisioner is no support!");
    return -ENOTSUP;
}

static void provisioner_reset()
{
    bt_mesh_reset();
    p33_soft_reset();
}

void input_key_handler(u8 key_status, u8 key_number)
{
    log_info("key_number=0x%x", key_number);

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
    if (key_status == KEY_EVENT_LONG) {
        switch (key_number) {
        case 0:
            break;
        case 2:
            log_info("\n  <bt_mesh_reset> \n");
            sys_timeout_add(NULL, provisioner_reset, 1000 * 3);
            break;
        default:
            break;
        }
    }

    if (key_status == KEY_EVENT_CLICK) {
        switch (key_number) {
        case 0:
            // 显示已配网节点
            Provisioner_show_cdb_node();
            break;
        case 1:
            // 清理配网节点记录
            clear_store_node();
            break;
        case 5:
            // 对最后扫描到的设备uuid发起配网
            bt_mesh_provision_adv(unprov_uuid, PROV_NET_IDX, BT_MESH_ADDR_UNASSIGNED, 0);
            break;
        default:
            break;
        }
    }
}

/*
 * 节点配置回调,每条配置消息收到回复后调用
 */
static void node_config_cmd_rsp_cb(u32_t opcode)
{
    printf("node_config_cmd_rsp_cb, opcode:0x%x", opcode);
    cfg_cli.op_pending = 0;
    cfg_cli.op_param = NULL;

    curr_pair_node.config_step++;
    provisioner_node_configuration();
}

/*
 * 配网者自配置
 */
static void self_configure(void)
{
    u16_t elem_addr = 0x0001;

    int err = 0;
    err = bt_mesh_cfg_app_key_add(PROV_NET_IDX, PROV_NODE_ADDR, PROV_NET_IDX, PROV_APP_IDX, prov_app_key, NULL);
    if (err) {
        log_info("add app key err:%d!", err);
    }
    err = bt_mesh_cfg_mod_app_bind(PROV_NET_IDX, PROV_NODE_ADDR, elements[0].addr, PROV_APP_IDX, BT_MESH_MODEL_ID_GEN_ONOFF_SRV, NULL);
    if (err) {
        log_info("bind app key err:%d!", err);
    }
    err = bt_mesh_cfg_mod_sub_add(PROV_NET_IDX, PROV_NODE_ADDR, elements[0].addr, PROV_GROUP_ADDR, BT_MESH_MODEL_ID_GEN_ONOFF_SRV, NULL);
    if (err) {
        log_info("add sub addr err:%d!", err);
    }
}

static void mesh_init(void)
{
    log_info("--func=%s", __FUNCTION__);

    int err = bt_mesh_init(&prov, &composition);
    if (err) {
        log_error("Initializing mesh failed (err %d)\n", err);
        return;
    }

    settings_load();

    err = bt_mesh_provision(prov_net_key, PROV_NET_IDX, flags, PROV_IV_IDX, PROV_NODE_ADDR, prov_dev_key);
    if (err) {
        log_info("Using stored settings\n");
    } else {
        log_info("Provisioning completed\n");
        self_configure();
    }

    mesh_cli_cmd_rsp_callback_register(node_config_cmd_rsp_cb);

    bt_mesh_cdb_create(prov_net_key);

    bt_mesh_prov_enable(BT_MESH_PROV_GATT | BT_MESH_PROV_ADV);
    //sys_timer_add(NULL, mem_stats, 1000);
}

void bt_ble_init(void)
{
    u8 bt_addr[6] = {MAC_TO_LITTLE_ENDIAN(CUR_DEVICE_MAC_ADDR)};

    bt_mac_addr_set(bt_addr);

    mesh_setup(mesh_init);
}

#endif /* (CONFIG_MESH_MODEL == SIG_MESH_PROVISIONER) */
