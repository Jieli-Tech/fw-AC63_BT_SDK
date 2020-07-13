/*********************************************************************************************
    *   Filename        : le_counter.h

    *   Description     :

    *   Author          : Bingquan

    *   Email           : bingquan_cai@zh-jieli.com

    *   Last modifiled  : 2017-01-17 15:17

    *   Copyright:(c)JIELI  2011-2016  @ , All Rights Reserved.
*********************************************************************************************/

#ifndef    __LE_COMMON_H_
#define    __LE_COMMON_H_

#include "typedef.h"
#include <stdint.h>
#include "btstack/bluetooth.h"

//--------------------------------------------

#define ADV_SET_1M_PHY                  1
#define ADV_SET_2M_PHY                  2
#define ADV_SET_CODED_PHY               3

#define SCAN_SET_1M_PHY                 BIT(0)
#define SCAN_SET_2M_PHY                 BIT(1)
#define SCAN_SET_CODED_PHY              BIT(2)

#define INIT_SET_1M_PHY                 BIT(0)
#define INIT_SET_2M_PHY                 BIT(1)
#define INIT_SET_CODED_PHY              BIT(2)

#define CONN_SET_1M_PHY                 BIT(0)
#define CONN_SET_2M_PHY                 BIT(1)
#define CONN_SET_CODED_PHY              BIT(2)

#define CONN_SET_PHY_OPTIONS_NONE        0
#define CONN_SET_PHY_OPTIONS_S2          1
#define CONN_SET_PHY_OPTIONS_S8          2

struct conn_param_t {
    u16 interval;
    u16 latency;
    u16 timeout;
};

// #define NOTIFY_TYPE           1
// #define INDICATION_TYPE       2
// Minimum/default MTU

#define ATT_CTRL_BLOCK_SIZE       (88)                    //note: fixed,libs use


typedef enum {
    BLE_ST_NULL = 0,
    BLE_ST_INIT_OK,             //协议栈初始化ok
    BLE_ST_IDLE,                //关闭广播或扫描状态
    BLE_ST_CONNECT,             //链路刚连上
    BLE_ST_SEND_DISCONN,        //发送断开命令，等待链路断开
    BLE_ST_DISCONN,             //链路断开状态
    BLE_ST_CONNECT_FAIL,        //连接失败

    BLE_ST_ADV = 0x20,          //设备处于广播状态
    BLE_ST_NOTIFY_IDICATE,      //设备已连上,允许发数(已被主机使能通知)

    BLE_ST_SCAN = 0x40,             //设备处于搜索状态
    BLE_ST_CREATE_CONN,             //发起设备连接
    BLE_ST_SEND_CREATE_CONN_CANNEL, //取消发起设备连接
    BLE_ST_SEARCH_COMPLETE,         //链路连上，已搜索完profile，可以发送数据操作

    BLE_ST_SEND_STACK_EXIT = 0x60,  //发送退出协议栈命令，等待完成
    BLE_ST_STACK_EXIT_COMPLETE,     //协议栈退出成功

} ble_state_e;

enum {
    APP_BLE_NO_ERROR = 0,
    APP_BLE_BUFF_FULL,       //buffer 满，会丢弃当前发送的数据包
    APP_BLE_BUFF_ERROR,      //
    APP_BLE_OPERATION_ERROR, //操作错误
    APP_BLE_IS_DISCONN,      //链路已断开
    APP_BLE_NO_WRITE_CCC,    //主机没有 write Client Characteristic Configuration
};

enum {
    ADV_IND = 0,
    ADV_DIRECT_IND,
    ADV_SCAN_IND,
    ADV_NONCONN_IND,
};

#define  ADV_CHANNEL_37    BIT(0)
#define  ADV_CHANNEL_38    BIT(1)
#define  ADV_CHANNEL_39    BIT(2)
#define  ADV_CHANNEL_ALL  (ADV_CHANNEL_37 | ADV_CHANNEL_38 | ADV_CHANNEL_39)

enum {
    SCAN_PASSIVE = 0,
    SCAN_ACTIVE,
};

#define FLAGS_LIMITED_DISCOVERABLE_MODE    BIT(0)
#define FLAGS_GENERAL_DISCOVERABLE_MODE    BIT(1)
#define FLAGS_EDR_NOT_SUPPORTED            BIT(2)
#define FLAGS_LE_AND_EDR_SAME_CONTROLLER   BIT(3)
#define FLAGS_LE_AND_EDR_SAME_HOST         BIT(4)


typedef enum {
    HCI_EIR_DATATYPE_FLAGS =                                                 0x01,
    HCI_EIR_DATATYPE_MORE_16BIT_SERVICE_UUIDS =                              0x02,
    HCI_EIR_DATATYPE_COMPLETE_16BIT_SERVICE_UUIDS =                          0x03,
    HCI_EIR_DATATYPE_MORE_32BIT_SERVICE_UUIDS =                              0x04,
    HCI_EIR_DATATYPE_COMPLETE_32BIT_SERVICE_UUIDS =                          0x05,
    HCI_EIR_DATATYPE_MORE_128BIT_SERVICE_UUIDS =                             0x06,
    HCI_EIR_DATATYPE_COMPLETE_128BIT_SERVICE_UUIDS =                         0x07,
    HCI_EIR_DATATYPE_SHORTENED_LOCAL_NAME =                                  0x08,
    HCI_EIR_DATATYPE_COMPLETE_LOCAL_NAME =                                   0x09,
    HCI_EIR_DATATYPE_TX_POWER_LEVEL =                                        0x0A,
    HCI_EIR_DATATYPE_CLASS_OF_DEVICE =                                       0x0D,
    HCI_EIR_DATATYPE_SIMPLE_PAIRING_HASH_C =                                 0x0E,
    HCI_EIR_DATATYPE_SIMPLE_PAIRING_RANDOMIZER_R =                           0x0F,
    HCI_EIR_DATATYPE_SECURITY_MANAGER_TK_VALUE =                             0x10,
    HCI_EIR_DATATYPE_SECURITY_MANAGER_OOB_FLAGS =                            0x11,
    HCI_EIR_DATATYPE_SLAVE_CONNECTION_INTERVAL_RANGE =                       0x12,
    HCI_EIR_DATATYPE_16BIT_SERVICE_SOLICITATION_UUIDS =                      0x14,
    HCI_EIR_DATATYPE_128BIT_SERVICE_SOLICITATION_UUIDS =                     0x15,
    HCI_EIR_DATATYPE_SERVICE_DATA =                                          0x16,
    HCI_EIR_DATATYPE_APPEARANCE_DATA =                                       0x19,
    HCI_EIR_DATATYPE_MANUFACTURER_SPECIFIC_DATA =                            0xFF
} HCI_EIR_datatype_t;



// struct ble_server_operation_t {
// 	int(*adv_enable)(void *priv, u32 enable);
// 	int(*disconnect)(void *priv);
// 	int(*get_buffer_vaild)(void *priv);
// 	int(*send_data)(void *priv, void *buf, u16 len);
// 	int(*regist_wakeup_send)(void *priv, void *cbk);
// 	int(*regist_recieve_cbk)(void *priv, void *cbk);
// 	int(*regist_state_cbk)(void *priv, void *cbk);
// };
// void ble_get_server_operation_table(struct ble_server_operation_t **interface_pt);
//
//
// struct ble_client_operation_t {
// 	int(*scan_enable)(void *priv, u32 enable);
// 	int(*disconnect)(void *priv);
// 	int(*get_buffer_vaild)(void *priv);
// 	int(*write_data)(void *priv, void *buf, u16 len);
// 	int(*read_do)(void *priv);
// 	int(*regist_wakeup_send)(void *priv, void *cbk);
// 	int(*regist_recieve_cbk)(void *priv, void *cbk);
// 	int(*regist_state_cbk)(void *priv, void *cbk);
// };
// void ble_get_client_operation_table(struct ble_client_operation_t **interface_pt);


static inline u8 make_eir_packet_data(u8 *buf, u16 offset, u8 data_type, u8 *data, u8 data_len)
{
    if (ADV_RSP_PACKET_MAX - offset < data_len + 2) {
        return offset + data_len + 2;
    }

    buf[0] = data_len + 1;
    buf[1] = data_type;
    memcpy(buf + 2, data, data_len);
    return data_len + 2;
}

static inline u8 make_eir_packet_val(u8 *buf, u16 offset, u8 data_type, u32 val, u8 val_size)
{
    if (ADV_RSP_PACKET_MAX - offset < val_size + 2) {
        return offset + val_size + 2;
    }

    buf[0] = val_size + 1;
    buf[1] = data_type;
    memcpy(buf + 2, &val, val_size);
    return val_size + 2;
}

extern void le_l2cap_register_packet_handler(void (*handler)(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size));

#endif


