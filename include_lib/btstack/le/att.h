/*********************************************************************************************
    *   Filename        : att.h

    *   Description     :

    *   Author          : minxian

    *   Email           : liminxian@zh-jieli.com

    *   Last modifiled  : 2020-07-01 16:33

    *   Copyright:(c)JIELI  2011-2020  @ , All Rights Reserved.
*********************************************************************************************/
#ifndef _BT_ATT_H_
#define _BT_ATT_H_

#include "typedef.h"
#include "btstack/btstack_typedef.h"

#if defined __cplusplus
extern "C" {
#endif

// Minimum/default MTU
#define ATT_DEFAULT_MTU           23

#define ATT_EVENT_MTU_EXCHANGE_COMPLETE                    0xB5
#define ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE         0xB6
#define ATT_EVENT_CAN_SEND_NOW                             0xB7

#define ATT_TRANSACTION_MODE_NONE      0x0
#define ATT_TRANSACTION_MODE_ACTIVE    0x1
#define ATT_TRANSACTION_MODE_EXECUTE   0x2
#define ATT_TRANSACTION_MODE_CANCEL    0x3
#define ATT_TRANSACTION_MODE_VALIDATE  0x4

#define ATT_PROPERTY_BROADCAST                0x01
#define ATT_PROPERTY_READ                     0x02
#define ATT_PROPERTY_WRITE_WITHOUT_RESPONSE   0x04
#define ATT_PROPERTY_WRITE                    0x08
#define ATT_PROPERTY_NOTIFY                   0x10
#define ATT_PROPERTY_INDICATE                 0x20


    typedef enum {
        ATT_OP_AUTO_READ_CCC = 0,
        ATT_OP_NOTIFY = 1,
        ATT_OP_INDICATE = 2,
        ATT_OP_READ,
        ATT_OP_READ_LONG,
        ATT_OP_WRITE,
        ATT_OP_WRITE_WITHOUT_RESPOND,
        //add here

        ATT_OP_CMD_MAX = 15,
    } att_op_type_e;

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
        u16  packet_type;
        u16  value_handle;
        u16  value_offset;
        u16  blob_length;
        u8  *blob;
    } att_data_report_t;


    typedef struct {
        service_report_t services;
        charact_report_t characteristic;
        u16 service_index;
        u16 characteristic_index;
    } search_result_t;

    void att_ccc_config_init(void);

    void att_set_ccc_config(uint16_t handle, uint16_t cfg);

    uint16_t att_get_ccc_config(uint16_t handle);

    void att_server_set_exchange_mtu(u16 con_handle);

    void att_set_db(uint8_t const *db);//change profile_data

// ATT Client Read Callback for Dynamic Data
// - if buffer == NULL, don't copy data, just return size of value
// - if buffer != NULL, copy data and return number bytes copied
// @param con_handle of hci le connection
// @param attribute_handle to be read
// @param offset defines start of attribute value
// @param buffer
// @param buffer_size
    typedef uint16_t (*att_read_callback_t)(uint16_t con_handle, uint16_t attribute_handle, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);

// ATT Client Write Callback for Dynamic Data
// @param con_handle of hci le connection
// @param attribute_handle to be written
// @param transaction - ATT_TRANSACTION_MODE_NONE for regular writes, ATT_TRANSACTION_MODE_ACTIVE for prepared writes and ATT_TRANSACTION_MODE_EXECUTE
// @param offset into the value - used for queued writes and long attributes
// @param buffer
// @param buffer_size
// @param signature used for signed write commmands
// @returns 0 if write was ok, ATT_ERROR_PREPARE_QUEUE_FULL if no space in queue, ATT_ERROR_INVALID_OFFSET if offset is larger than max buffer
    typedef int (*att_write_callback_t)(uint16_t con_handle, uint16_t attribute_handle, uint16_t transaction_mode, uint16_t offset, uint8_t *buffer, uint16_t buffer_size);

    void ble_att_server_setup_init(const u8 *profile_db, att_read_callback_t read_cbk, att_write_callback_t write_cbk);

    void att_server_request_can_send_now_event(hci_con_handle_t con_handle);

    int att_server_notify(hci_con_handle_t con_handle, uint16_t attribute_handle, uint8_t *value, uint16_t value_len);

    int att_server_indicate(hci_con_handle_t con_handle, uint16_t attribute_handle, uint8_t *value, uint16_t value_len);

    extern void att_server_init(uint8_t const *db, att_read_callback_t read_callback, att_write_callback_t write_callback);
    extern void att_server_register_packet_handler(btstack_packet_handler_t handler);

#endif
