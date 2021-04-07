// binary representation
// attribute size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)

#ifndef _BT_COMMON_H
#define _BT_COMMON_H
#include <stdint.h>
#include "app_config.h"

#define UART_FIFIO_BUF_LEN 1024

#ifdef APP_PRIVATE_PROFILE_CFG
#include "lib_profile_cfg.h"
#else
#include "bt_profile_cfg.h"
#endif

enum {
    ST_BIT_INQUIRY = 0,
    ST_BIT_PAGE_SCAN,
    ST_BIT_BLE_ADV,
    ST_BIT_SPP_CONN,
    ST_BIT_BLE_CONN,
    ST_BIT_WEIXIN_CONN,
};

enum {
    BLE_PRIV_MSG_PAIR_CONFIRM = 0xF0,
    BLE_PRIV_PAIR_ENCRYPTION_CHANGE,
};//ble_state_e


// #define SYS_BT_EVENT_FORM_COMMON       (('C' << 24) | ('M' << 16) | ('M' << 8) | '\0')
enum {
    COMMON_EVENT_EDR_REMOTE_TYPE = 1,
    COMMON_EVENT_BLE_REMOTE_TYPE,
};

extern const int config_le_hci_connection_num;//支持同时连接个数
extern const int config_le_sm_support_enable; //是否支持加密配对
extern const int config_le_gatt_server_num;   //支持server角色个数
extern const int config_le_gatt_client_num;   //支持client角色个数

extern uint16_t little_endian_read_16(const uint8_t *buffer, int pos);
extern uint32_t little_endian_read_24(const uint8_t *buffer, int pos);
extern uint32_t little_endian_read_32(const uint8_t *buffer, int pos);
extern void swapX(const uint8_t *src, uint8_t *dst, int len);


//common api
extern void bt_ble_init(void);
extern void bt_ble_exit(void);
extern void bt_ble_adv_enable(u8 enable);
extern void ble_app_disconnect(void);
extern void ble_module_enable(u8 en);
extern const u8 *bt_get_mac_addr();
extern void lib_make_ble_address(u8 *ble_address, u8 *edr_address);
extern void user_spp_data_handler(u8 packet_type, u16 ch, u8 *packet, u16 size);
extern void bt_get_vm_mac_addr(u8 *addr);
extern void set_remote_test_flag(u8 own_remote_test);
extern void bredr_power_get(void);
extern void bredr_power_put(void);
extern void radio_set_eninv(int v);
extern void transport_spp_init(void);
extern const char *bt_get_local_name();
extern void wdt_clear(void);
extern u8 ble_update_get_ready_jump_flag(void);
extern void reset_PK_cb_register(void (*reset_pk)(u32 *));
extern void att_server_flow_enable(u8 enable);
extern void le_device_db_init(void);
extern bool get_remote_test_flag();
extern void transport_spp_disconnect(void);
extern int transport_spp_flow_enable(u8 en);
extern void transport_spp_flow_cfg(void);

void att_set_conn_handle(u16 conn_handle, u8 handle_type);
void at_send_rx_cid_data(u8 cid, u8 *packet, u16 size);
void at_send_conn_result(u8 cid, u8 is_sucess);
void at_send_disconnect(u8 cid);
void at_send_connected(u8 cid);
void at_send_string(u8 *str);
void at_respond_send_err(u32 err_id);

int le_att_server_send_data(u8 cid, u8 *packet, u16 size);
int le_att_server_send_ctrl(u8 cid, u8 *packet, u16 size);
int le_att_client_send_data(u8 cid, u8 *packet, u16 size);
int le_at_client_creat_connection(u8 *conn_addr, u8 addr_type);
void black_list_check(u8 sta, u8 *peer_addr);


//errid
enum {
    ERR_AT_CMD = 1,
    //add here
};

/*
参数 bt_tx_power 挡位: 功率 dbm ,超过等级默认为最高档
BD29: rang(0~8)  {-18.3,  -14.6,  -12.1, -8.5,  -6.0,  -4.1,  -1.1,  +1.1,  +4.0, +6.1}
BD19: rang(0~10) {-17.6,  -14.0,  -11.5, -9.6,  -6.6,  -4.4,  -0.79, -1.12, +3.8, +5.65, +8.04}
BR23: rang(0~9)  {-15.7,  -12.5,  -10.0, -6.6,  -4.4,  -2.5,  -0.1,  +2.1,  +4.6, +6.4}
BR25: rang(0~9)  {-15.7,  -12.5,  -10.0, -6.6,  -4.4,  -2.5,  -0.1,  +2.1,  +4.6, +6.4}
BR30: rang(0~8)  {-17.48, -11.46, -7.96, -3.59, -0.79, +1.12, +3.8,  +6.5,  +8.44}
*/
void bt_max_pwr_set(u8 pwr, u8 pg_pwr, u8 iq_pwr, u8 ble_pwr);

#endif
