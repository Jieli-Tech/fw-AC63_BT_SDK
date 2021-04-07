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

#define LOG_TAG             "[Mesh-LightLightness_srv]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if (CONFIG_MESH_MODEL == SIG_MESH_LIGHT_LIGHTNESS_SERVER)

extern u16_t primary_addr;
extern void mesh_setup(void (*init_cb)(void));
extern void gpio_pin_write(u8_t led_index, u8_t onoff);
extern void bt_mac_addr_set(u8 *bt_addr);
extern void prov_complete(u16_t net_idx, u16_t addr);
extern void prov_reset(void);
extern uint32_t btctler_get_rand_from_assign_range(uint32_t rand, uint32_t min, uint32_t max);
extern void pseudo_random_genrate(uint8_t *dest, unsigned size);
extern void ble_bqb_test_thread_init(void);
/**
 * @brief Config current node features(Relay/Proxy/Friend/Low Power)
 */
/*-----------------------------------------------------------*/
#define BT_MESH_FEAT_SUPPORTED_TEMP         ( \
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
 * @brief Conifg complete local name
 */
/*-----------------------------------------------------------*/
#define BLE_DEV_NAME        'L', 'e', 'd', 'L', 'i','g' ,'h' ,'t','n' ,'e' ,'s' ,'s'  , '_', 's', 'r', 'v'

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
#define CUR_DEVICE_MAC_ADDR         0x222233445566

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

/* Generic OnOff Model Operation Codes */
#define BT_MESH_MODEL_OP_GEN_ONOFF_GET			BT_MESH_MODEL_OP_2(0x82, 0x01)
#define BT_MESH_MODEL_OP_GEN_ONOFF_SET			BT_MESH_MODEL_OP_2(0x82, 0x02)
#define BT_MESH_MODEL_OP_GEN_ONOFF_SET_UNACK	BT_MESH_MODEL_OP_2(0x82, 0x03)
#define BT_MESH_MODEL_OP_GEN_ONOFF_STATUS		BT_MESH_MODEL_OP_2(0x82, 0x04)


/* Company Identifiers (see Bluetooth Assigned Numbers) */
#define BT_COMP_ID_LF           0x05D6 // Zhuhai Jieli technology Co.,Ltd

/* LED NUMBER */
#define LED0_GPIO_PIN           0

static struct onoff_state {
    u8_t current;
    u8_t previous;
    u8_t led_gpio_pin;
};

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

extern const struct bt_mesh_model_op gen_onpowerup_srv_op[];
extern const struct bt_mesh_model_op gen_onpowerup_setup_srv_op[];
extern const struct bt_mesh_model_op light_lightness_srv_op[];
extern const struct bt_mesh_model_op light_lightness_setup_srv_op[];
extern const struct bt_mesh_model_op gen_level_server[];
extern const struct bt_mesh_model_op gen_onoff_srv_op[];
extern const struct bt_mesh_model_op gen_onoff_cli_op[];

extern struct onoff_state dev_onoff_state[];
extern struct level_state light_level_state;
extern struct light_state dev_light_state;
extern struct onpowerup_state dev_onpowerup_state;

/*
 *
 * Element Model Declarations
 *
 * Element 0 Root Models
 */
struct bt_mesh_model root_models[] = {
    BT_MESH_MODEL_CFG_SRV(&cfg_srv),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_SRV, 			  gen_onoff_srv_op, 		  	&gen_onoff_pub_srv, &dev_onoff_state[0]),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_ONOFF_CLI, 			  gen_onoff_cli_op, 		  	&gen_onoff_pub_srv, &dev_onoff_state[0]),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_LEVEL_SRV, 	  		  gen_level_server, 			&gen_onoff_pub_srv, &light_level_state),
    //BT_MESH_MODEL(BT_MESH_MODEL_ID_GEN_POWER_ONOFF_SRV, 	  gen_onpowerup_srv_op, 	  	&gen_onoff_pub_srv, &dev_onpowerup_state),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SRV, 	  light_lightness_srv_op,		&gen_onoff_pub_srv, &dev_light_state),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_LIGHT_LIGHTNESS_SETUP_SRV, light_lightness_setup_srv_op, &gen_onoff_pub_srv, &dev_light_state),
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
};


static void mesh_init(void)
{
    log_info("--func=%s", __FUNCTION__);

    int err = bt_mesh_init(&prov, &composition);
    if (err) {
        log_error("Initializing mesh failed (err %d)\n", err);
        return;
    }

    settings_load();

    bt_mesh_prov_enable(BT_MESH_PROV_GATT | BT_MESH_PROV_ADV);
}


#if defined(CONFIG_CPU_BD19)
extern void timer_pwm_init(JL_TIMER_TypeDef *JL_TIMERx, u32 pwm_io, u32 fre, u32 duty);
#elif defined(CONFIG_CPU_BR25)
extern int timer_pwm_init(JL_TIMER_TypeDef *JL_TIMERx, u32 fre, u32 duty, u32 port, int output_ch);
#endif

void bt_ble_init(void)
{
    u8 bt_addr[6] = {MAC_TO_LITTLE_ENDIAN(CUR_DEVICE_MAC_ADDR)};

    bt_mac_addr_set(bt_addr);

    mesh_setup(mesh_init);
    if (BT_MODE_IS(BT_BQB)) {
        ble_bqb_test_thread_init();
    }
#if defined(CONFIG_CPU_BD19)
    timer_pwm_init(JL_TIMER0, IO_PORTB_07, 10000, 0);
#elif defined (CONFIG_CPU_BR25)
    timer_pwm_init(JL_TIMER5, 10000, 0, IO_PORTB_07, 0);
#endif
}

#endif /* (CONFIG_MESH_MODEL == SIG_MESH_GENERIC_ONOFF_SERVER) */
