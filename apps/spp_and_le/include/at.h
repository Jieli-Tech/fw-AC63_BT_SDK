#ifndef _AT_H_
#define _AT_H_

#include "typedef.h"

// #define DEVICE_EVENT_FROM_AT_UART (('A' << 24) | ('T' << 16) | ('U' << 8) | '\0')

void at_cmd_rx_handler(void);

#define AT_PACKET_TYPE_CMD      0x01
#define AT_PACKET_TYPE_EVT      0x02

//Byte 0        | Byte 1 | Byte 2 | Byte 3 ~ (length + 3)
//packet type   | Opcode | Length | Payload

struct at_format {
    u8 type;
    u8 opcode;
    u8 length;
    u8 payload[0];
};

#define AT_FORMAT_HEAD   3//sizeof(struct at_format)
//
#define AT_CMD_SET_BT_ADDR          0x00

struct cmd_set_bt_addr {
    u8 addr[6];
};

#define AT_CMD_SET_BLE_ADDR         0x01

struct cmd_set_ble_addr {
    u8 addr[6];
};

#define AT_CMD_SET_VISIBILITY       0x02

struct cmd_set_bt_visbility {
    u8 discovery : 1;
    u8 connect   : 1;
    u8 adv       : 1;
};

#define AT_CMD_SET_BT_NAME          0x03

struct cmd_set_bt_name {
    u8 name[0x20];
};


#define AT_CMD_SET_BLE_NAME         0x04

struct cmd_set_ble_name {
    u8 name[0x18];
};

#define AT_CMD_SEND_SPP_DATA        0x05

struct cmd_send_spp_data {
    u8 spp_data[0];
};

#define AT_CMD_SEND_BLE_DATA        0x09

struct cmd_send_ble_data {
    u16 att_handle;
    u8 att_data[0];
};

#define AT_CMD_SEND_DATA            0x0A

struct cmd_send_data {
    u8 data[0];
};

#define AT_CMD_STATUS_REQUEST       0x0B

#define AT_CMD_SET_PAIRING_MODE     0x0C

#define AT_PAIRING_MODE_PINCODE     0x00
#define AT_PAIRING_MODE_JUSTWORK    0x01
#define AT_PAIRING_MODE_PASSKEY     0x02
#define AT_PAIRING_MODE_CONFIRM     0x03

struct cmd_set_pairing_mode {
    u8 mode;
};

#define AT_CMD_SET_PINCODE          0x0D

struct cmd_set_pincode {
    u8 pincode[0x10];
};

#define AT_CMD_SET_UART_FLOW        0x0E
#define AT_CMD_SET_UART_BAUD        0x0F

struct cmd_set_uart_baud {
    char baudrate[0x07];
};

#define AT_CMD_VERSION_REQUEST      0x10
#define AT_CMD_BT_DISCONNECT        0x11
#define AT_CMD_BLE_DISCONNECT       0x12
#define AT_CMD_SET_COD              0x15
#define AT_CMD_SET_RF_MAX_TXPOWER   0x16
#define AT_CMD_SET_EDR_TXPOWER      0x17
#define AT_CMD_SET_BLE_TXPOWER      0x18

struct cmd_set_cod {
    u8 classofdevice[0x03];
};

#define AT_CMD_SET_LOW_POWER_MODE   0x26
#define AT_CMD_ENTER_SLEEP_MODE     0x27
#define AT_CMD_SET_CONFIRM_GKEY     0x28

struct cmd_set_confirm_gkey {
    u8 ok_or_no;
    u8 passkey[6];
};

#define AT_CMD_SET_ADV_DATA         0x2d

struct cmd_set_adv_data {
    u8 data[0x1f];
};

#define AT_CMD_SET_SCAN_DATA        0x2e

struct cmd_set_scan_data {
    u8 data[0x1f];
};

#define AT_CMD_SET_XTAL             0x30
#define AT_CMD_SET_DCDC             0x31

struct cmd_set_dcdc {
    u8 enable;
};

#define AT_CMD_GET_BT_ADDR          0x34
#define AT_CMD_GET_BLE_ADDR         0x35
#define AT_CMD_GET_BT_NAME          0x36
#define AT_CMD_GET_BLE_NAME         0x37
#define AT_CMD_GET_PINCODE          0x33


#define AT_CMD_BLE_CONN_PARAM_REQUEST 					0x38
#define AT_CMD_SET_BLE_SCAN_PARAM 						0x50
#define AT_CMD_SET_BLE_SCAN_ENABLE 						0x51
#define AT_CMD_BLE_CREAT_CONNECT						0x52
#define AT_CMD_BLE_CREAT_CONNECT_CANNEL					0x53
#define AT_CMD_BLE_PROFILE_SEARCH						0x54
#define AT_CMD_BLE_ATT_ENABLE_CCC 						0x55
#define AT_CMD_BLE_ATT_READ 							0x56
#define AT_CMD_BLE_ATT_WRITE 							0x57
#define AT_CMD_BLE_ATT_WRITE_NO_RSP 					0x58


#define AT_EVT_BT_CONNECTED         0x00
#define AT_EVT_BLE_CONNECTED        0x02
#define AT_EVT_BT_DISCONNECTED      0x03
#define AT_EVT_BLE_DISCONNECTED     0x05
#define AT_EVT_CMD_COMPLETE         0x06
#define AT_EVT_SPP_DATA_RECEIVED    0x07
#define AT_EVT_BLE_DATA_RECEIVED    0x08
#define AT_EVT_SYSTEM_READY         0x09
#define AT_EVT_STATUS_RESPONSE      0x0A
#define AT_EVT_WEIXIN_DATA_RECEIVE  0x0B
#define AT_EVT_INDICATE_COMPLETE    0x0C
#define AT_EVT_CONFIRM_GKEY         0x0E
#define AT_EVT_UART_EXCEPTION       0x0F

#define AT_EVT_CONN_PARAM_UPDATE_COMPLETE   0x10
#define AT_EVT_ADV_REPORT                   0x20
#define AT_EVT_PROFILE_REPOFT               0x21
#define AT_EVT_PROFILE_SEARCH_END           0x22

void at_uart_init(void *packet_handler);
int  ct_uart_send_packet(const u8 *packet, int size);
void slave_connect_param_update(u16 interval_min, u16 interval_max, u16 latency, u16 timeout);
void at_send_event(u8 opcode, const u8 *packet, int size);
void ct_uart_change_baud(u32 baud);


#endif
