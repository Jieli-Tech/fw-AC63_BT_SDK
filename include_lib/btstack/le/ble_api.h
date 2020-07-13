/*********************************************************************************************
    *   Filename        : ble_api.h

    *   Description     : 

    *   Author          : Minxian

    *   Email           : liminxian@zh-jieli.com

    *   Last modifiled  : 2020-07-01 16:36

    *   Copyright:(c)JIELI  2011-2020  @ , All Rights Reserved.
*********************************************************************************************/
#ifndef __BLE_API_H__
#define __BLE_API_H__


#include "typedef.h"
#include "btstack/btstack_typedef.h"


///***注意：该文件的枚举与库编译密切相关，主要是给用户提供调用所用。用户不能自己在中间添加值。*/
////----user (command) codes----////

/**
 * @brief hci connection handle type
 */

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
ble_cmd_ret_e ble_user_cmd_prepare(ble_cmd_type_e cmd, int argc, ...);

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


//---------------
void ble_set_gap_role(u8 role);

void user_client_init(u16 handle, u8 *buffer, u16 buffer_size);

s8 ble_vendor_get_peer_rssi(u16 conn_handle);

#endif
