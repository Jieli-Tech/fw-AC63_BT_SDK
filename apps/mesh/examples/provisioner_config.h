#ifndef __PROVISIONER_CONFIG_H__
#define __PROVISIONER_CONFIG_H__

/* provisioner configuraion */
#define PROV_NODE_ADDR  0x7fff
#define PROV_GROUP_ADDR 0xf000

#define PROV_NET_IDX    0
#define PROV_APP_IDX    0
#define PROV_IV_IDX     0

#define CID_NVAL 0xffff

/* node configuraion struct */
struct app_key_param {
    u8_t *status;
    u16_t net_idx;
    u16_t app_idx;
};

struct mod_app_param {
    u8_t *status;
    u16_t elem_addr;
    u16_t mod_app_idx;
    u16_t mod_id;
    u16_t cid;
};

struct mod_pub_param {
    u16_t                       mod_id;
    u16_t                       cid;
    u16_t                       elem_addr;
    u8_t                       *status;
    struct bt_mesh_cfg_mod_pub *pub;
};

#endif /* __PROVISIONER_CONFIG_H__ */
