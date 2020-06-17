#ifndef __BLE_API_H__
#define __BLE_API_H__


#include "typedef.h"


///***注意：该文件的枚举与库编译密切相关，主要是给用户提供调用所用。用户不能自己在中间添加值。*/
////----user (command) codes----////

/**
 * @brief hci connection handle type
 */
typedef uint16_t hci_con_handle_t;

typedef enum {
    /*
     */
    BLE_CMD_ADV_ENABLE  = 1,
    BLE_CMD_ADV_PARAM,
    BLE_CMD_ADV_DATA,
    BLE_CMD_RSP_DATA,
    BLE_CMD_DISCONNECT,
    BLE_CMD_REGIEST_THREAD,
    BLE_CMD_ATT_SEND_INIT,
    BLE_CMD_ATT_MTU_SIZE,
    BLE_CMD_ATT_VAILD_LEN,
    BLE_CMD_ATT_SEND_DATA,
    BLE_CMD_REQ_CONN_PARAM_UPDATE,

    BLE_CMD_SCAN_ENABLE,
    BLE_CMD_SCAN_PARAM,
    BLE_CMD_STACK_EXIT,
    BLE_CMD_CREATE_CONN,
    BLE_CMD_CREATE_CONN_CANCEL,

    BLE_CMD_ADV_PARAM_EXT,
    BLE_CMD_SEND_TEST_KEY_NUM,
    BLE_CMD_LATENCY_HOLD_CNT,//阻止latency 生效的次数，系统递减
    BLE_CMD_SET_DATA_LENGTH,

    //< ble5
    BLE_CMD_EXT_ADV_PARAM = 0x40,
    BLE_CMD_EXT_ADV_DATA,
    BLE_CMD_EXT_RSP_DATA,
    BLE_CMD_EXT_ADV_ENABLE,
    BLE_CMD_SET_PHY,
    BLE_CMD_EXT_SCAN_PARAM,
    BLE_CMD_EXT_SCAN_ENABLE,
    BLE_CMD_EXT_CREATE_CONN,
    BLE_CMD_PERIODIC_ADV_PARAM,
    BLE_CMD_PERIODIC_ADV_DATA,
    BLE_CMD_PERIODIC_ADV_ENABLE,
    BLE_CMD_PERIODIC_ADV_CREAT_SYNC,
    
	//client
    BLE_CMD_SEARCH_PROFILE = 0x80,
    BLE_CMD_WRITE_CCC,
    BLE_CMD_ONNN_PARAM_UPDATA,
} ble_cmd_type_e;

typedef enum {
    BLE_CMD_RET_SUCESS =  0, //
    BLE_CMD_RET_BUSY = -100, //
    BLE_CMD_PARAM_OVERFLOW, //
    BLE_CMD_OPT_FAIL, //
    BLE_BUFFER_FULL, //
    BLE_BUFFER_ERROR, //
    BLE_CMD_PARAM_ERROR, //
    BLE_CMD_STACK_NOT_RUN,
    BLE_CMD_CCC_FAIL,    //not enable Client Characteristic Configuration
} ble_cmd_ret_e;

//--------------------------------------------
enum {
    ATT_OP_AUTO_READ_CCC = 0,
    ATT_OP_NOTIFY = 1,
    ATT_OP_INDICATE = 2,
    ATT_OP_READ,
    ATT_OP_READ_LONG,
    ATT_OP_WRITE,
    ATT_OP_WRITE_WITHOUT_RESPOND,
    //add here

    ATT_OP_CMD_MAX = 15,
};
extern ble_cmd_ret_e ble_user_cmd_prepare(ble_cmd_type_e cmd, int argc, ...);

struct conn_update_param_t {
    u16 interval_min;
    u16 interval_max;
    u16 latency;
    u16 timeout;
};

typedef enum {
    PFL_SERVER_UUID16 = 1,
    PFL_SERVER_UUID128,
    PFL_SERVER_ALL,
} search_profile_type_e;

//------
typedef struct {
    uint16_t start_group_handle;
    uint16_t end_group_handle;
    uint16_t uuid16;
    uint8_t  uuid128[16];
} service_report_t; //==le_service_t

typedef struct {
    uint16_t start_handle;
    uint16_t value_handle;
    uint16_t end_handle;
    uint16_t properties;
    uint16_t uuid16;
    uint8_t  uuid128[16];
} charact_report_t; //==le_characteristic_t


typedef struct {
    service_report_t services;
    charact_report_t characteristic;
    u16 service_index;
    u16 characteristic_index;
} search_result_t;

//------
struct create_conn_param_t {
    u16 conn_interval;
    u16 conn_latency;
    u16 supervision_timeout;
    u8 peer_address_type;
    u8 peer_address[6];
} _GNU_PACKED_;

typedef struct {
    u8   event_type;
    u8   address_type;
    u8   address[6];
    s8   rssi;
    u8   length;
    u8   data[0];
} adv_report_t;

typedef struct {
    u16  packet_type;
    u16  value_handle;
    u16  value_offset;
    u16  blob_length;
    u8  *blob;
} att_data_report_t;



//---------------
void ble_set_gap_role(u8 role);
void att_ccc_config_init(void);
void att_set_ccc_config(uint16_t handle, uint16_t cfg);
uint16_t att_get_ccc_config(uint16_t handle);
void user_client_init(u16 handle, u8 *buffer, u16 buffer_size);
void att_server_set_exchange_mtu(u16 con_handle);
s8 ble_vendor_get_peer_rssi(u16 conn_handle);
void att_set_db(uint8_t const *db);//change profile_data

#endif
