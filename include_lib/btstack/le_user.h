
#ifndef _LE_USER_H_
#define _LE_USER_H_

#if defined __cplusplus
extern "C" {
#endif

#include "btstack/ble_api.h"


    typedef void (*btstack_packet_handler_t)(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
    typedef int (*sm_stack_packet_handler_t)(uint8_t packet_type, uint8_t *packet, uint16_t size);

    typedef void (*ble_cbk_handler_t)(void);

// Minimum/default MTU
#define ATT_DEFAULT_MTU           23
#define BT_NAME_LEN_MAX		      29
#define ADV_RSP_PACKET_MAX        31

// hci con handles (12 bit): 0x0000..0x0fff
#define HCI_CON_HANDLE_INVALID 0xffff

#define BTSTACK_EVENT_STATE                                0x60
#define L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE   0x77

#define ATT_EVENT_MTU_EXCHANGE_COMPLETE                    0xB5
#define ATT_EVENT_HANDLE_VALUE_INDICATION_COMPLETE         0xB6
#define ATT_EVENT_CAN_SEND_NOW                             0xB7

#define SM_EVENT_JUST_WORKS_REQUEST                        0xD0
#define SM_EVENT_PASSKEY_DISPLAY_NUMBER                    0xD2


#define ATT_TRANSACTION_MODE_NONE      0x0
#define ATT_TRANSACTION_MODE_ACTIVE    0x1
#define ATT_TRANSACTION_MODE_EXECUTE   0x2
#define ATT_TRANSACTION_MODE_CANCEL    0x3
#define ATT_TRANSACTION_MODE_VALIDATE  0x4

#define GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NONE          0
#define GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_NOTIFICATION  1
#define GATT_CLIENT_CHARACTERISTICS_CONFIGURATION_INDICATION    2

#define ATT_PROPERTY_BROADCAST                0x01
#define ATT_PROPERTY_READ                     0x02
#define ATT_PROPERTY_WRITE_WITHOUT_RESPONSE   0x04
#define ATT_PROPERTY_WRITE                    0x08
#define ATT_PROPERTY_NOTIFY                   0x10
#define ATT_PROPERTY_INDICATE                 0x20


#define GATT_EVENT_NOTIFICATION                                  0xA7
#define GATT_EVENT_INDICATION                                    0xA8
#define GATT_EVENT_CHARACTERISTIC_VALUE_QUERY_RESULT             0xA5
#define GATT_EVENT_LONG_CHARACTERISTIC_VALUE_QUERY_RESULT        0xA6
// #define GATT_EVENT_SERVICE_QUERY_RESULT                          0xA1
// #define GATT_EVENT_CHARACTERISTIC_QUERY_RESULT                   0xA2
// #define GATT_EVENT_QUERY_COMPLETE                                0xA0
#define GAP_EVENT_ADVERTISING_REPORT                             0xE2

// Authentication requirement flags
#define SM_AUTHREQ_NO_BONDING           0x00
#define SM_AUTHREQ_BONDING              0x01
#define SM_AUTHREQ_MITM_PROTECTION      0x04
#define SM_AUTHREQ_SECURE_CONNECTION    0x08
#define SM_AUTHREQ_KEYPRESS             0x10


#define L2CAP_EVENT_CONNECTION_PARAMETER_UPDATE_RESPONSE   0x77


#define  BT_OP_SUCCESS                             0x00
#define  BT_ERR_ADVERTISING_TIMEOUT                0x3C


//--------------------------------------------
    enum {
        HCI_STATE_OFF = 0,
        HCI_STATE_INITIALIZING,
        HCI_STATE_WORKING,
        HCI_STATE_HALTING,
        HCI_STATE_SLEEPING,
        HCI_STATE_FALLING_ASLEEP
    } ;


// IO Capability Values
    typedef enum {
        IO_CAPABILITY_DISPLAY_ONLY = 0,
        IO_CAPABILITY_DISPLAY_YES_NO,
        IO_CAPABILITY_KEYBOARD_ONLY,
        IO_CAPABILITY_NO_INPUT_NO_OUTPUT,
        IO_CAPABILITY_KEYBOARD_DISPLAY, // not used by secure simple pairing
    } io_capability_t;


    struct ble_server_operation_t {
        int(*adv_enable)(void *priv, u32 enable);
        int(*disconnect)(void *priv);
        int(*get_buffer_vaild)(void *priv);
        int(*send_data)(void *priv, void *buf, u16 len);
        int(*regist_wakeup_send)(void *priv, void *cbk);
        int(*regist_recieve_cbk)(void *priv, void *cbk);
        int(*regist_state_cbk)(void *priv, void *cbk);
        int(*latency_enable)(void *priv, u32 enable);
    };
    void ble_get_server_operation_table(struct ble_server_operation_t **interface_pt);


    struct ble_client_operation_t {
        int(*scan_enable)(void *priv, u32 enable);
        int(*disconnect)(void *priv);
        int(*get_buffer_vaild)(void *priv);
        int(*write_data)(void *priv, void *buf, u16 len);
        int(*read_do)(void *priv);
        int(*regist_wakeup_send)(void *priv, void *cbk);
        int(*regist_recieve_cbk)(void *priv, void *cbk);
        int(*regist_state_cbk)(void *priv, void *cbk);
    };
    void ble_get_client_operation_table(struct ble_client_operation_t **interface_pt);



    typedef struct {
        //base info
        uint8_t   type;                 ///< See <btstack/hci_cmds.h> SM_...
        uint8_t   size;
        hci_con_handle_t con_handle;
        uint8_t   addr_type;
        uint8_t   address[6];
        //extend info
        uint8_t   data[4];
    } sm_just_event_t;

    static inline uint32_t ble_min(uint32_t a, uint32_t b)
    {
        return a < b ? a : b;
    }

//---------------------------------------------------------------------------------------------------


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

//----------------------------------------
    extern uint16_t little_endian_read_16(const uint8_t *buffer, int pos);
    extern uint32_t little_endian_read_24(const uint8_t *buffer, int pos);
    extern uint32_t little_endian_read_32(const uint8_t *buffer, int pos);

//----------------------------------------

    extern void ble_sm_setup_init(io_capability_t io_type, u8 auth_req, uint8_t min_key_size, u8 security_en);

    extern void ble_att_server_setup_init(const u8 *profile_db, att_read_callback_t read_cbk, att_write_callback_t write_cbk);

    extern void att_server_request_can_send_now_event(hci_con_handle_t con_handle);
    extern int att_server_notify(hci_con_handle_t con_handle, uint16_t attribute_handle, uint8_t *value, uint16_t value_len);

    extern uint8_t gatt_client_read_long_value_of_characteristic_using_value_handle_with_offset(btstack_packet_handler_t callback, hci_con_handle_t con_handle, uint16_t characteristic_value_handle, uint16_t offset);
    extern uint8_t gatt_client_read_value_of_characteristic_using_value_handle(btstack_packet_handler_t callback, hci_con_handle_t con_handle, uint16_t value_handle);
    extern uint8_t gatt_client_write_value_of_characteristic_without_response(hci_con_handle_t con_handle, uint16_t value_handle, uint16_t value_length, uint8_t *value);
    extern uint8_t gatt_client_write_value_of_characteristic(btstack_packet_handler_t callback, hci_con_handle_t con_handle, uint16_t value_handle, uint16_t value_length, uint8_t *data);

    extern int get_ble_btstack_state(void);
    extern int get_indicate_state(void);
    extern int att_server_indicate(hci_con_handle_t con_handle, uint16_t attribute_handle, uint8_t *value, uint16_t value_len);

    extern void ble_cbk_handler_register(btstack_packet_handler_t packet_cbk, sm_stack_packet_handler_t sm_cbk);

    extern int gap_request_connection_parameter_update(hci_con_handle_t con_handle, uint16_t conn_interval_min,
            uint16_t conn_interval_max, uint16_t conn_latency, uint16_t supervision_timeout);


    extern void gap_advertisements_enable(int enabled);
    extern void gap_advertisements_set_data(uint8_t advertising_data_length, uint8_t *advertising_data);
    extern void gap_scan_response_set_data(uint8_t scan_response_data_length, uint8_t *scan_response_data);
    extern void gap_advertisements_set_params(uint16_t adv_int_min, uint16_t adv_int_max, uint8_t adv_type,
            uint8_t direct_address_typ, uint8_t *direct_address, uint8_t channel_map, uint8_t filter_policy);

    extern u8 get_ble_local_name(u8 *name_buf);
    extern u8 get_ble_local_name_len();


    extern void gatt_client_request_can_send_now_event(hci_con_handle_t con_handle);

//--------------------------------------
    static inline hci_con_handle_t hci_subevent_le_connection_update_complete_get_connection_handle(const uint8_t *event)
    {
        return little_endian_read_16(event, 4);
    }

    static inline uint16_t hci_subevent_le_connection_update_complete_get_conn_interval(const uint8_t *event)
    {
        return little_endian_read_16(event, 6);
    }
    static inline uint16_t hci_subevent_le_connection_update_complete_get_conn_latency(const uint8_t *event)
    {
        return little_endian_read_16(event, 8);
    }
    static inline uint16_t hci_subevent_le_connection_update_complete_get_supervision_timeout(const uint8_t *event)
    {
        return little_endian_read_16(event, 10);
    }

    static inline uint8_t hci_event_packet_get_type(const uint8_t *event)
    {
        return event[0];
    }

    static inline uint8_t hci_event_le_meta_get_subevent_code(const uint8_t *event)
    {
        return event[2];
    }
    static inline uint16_t att_event_mtu_exchange_complete_get_MTU(const uint8_t *event)
    {
        return little_endian_read_16(event, 4);
    }

    static inline uint8_t btstack_event_state_get_state(const uint8_t *event)
    {
        return event[2];
    }

    static inline uint16_t hci_event_disconnection_complete_get_connection_handle(const uint8_t *event)
    {
        return little_endian_read_16(event, 3);
    }

    static inline hci_con_handle_t hci_subevent_le_connection_complete_get_connection_handle(const uint8_t *event)
    {
        return little_endian_read_16(event, 4);
    }

    static inline uint16_t hci_subevent_le_connection_complete_get_conn_interval(const uint8_t *event)
    {
        return little_endian_read_16(event, 14);
    }

    static inline hci_con_handle_t att_event_mtu_exchange_complete_get_handle(const uint8_t *event)
    {
        return little_endian_read_16(event, 2);
    }
    static inline hci_con_handle_t sm_event_just_works_request_get_handle(const uint8_t *event)
    {
        return little_endian_read_16(event, 2);
    }

    static inline uint8_t hci_event_le_meta_get_phy_update_complete_status(const uint8_t *event)
    {
        return event[3];
    }
    static inline uint8_t hci_event_le_meta_get_phy_update_complete_tx_phy(const uint8_t *event)
    {
        return event[6];
    }
    static inline uint8_t hci_event_le_meta_get_phy_update_complete_rx_phy(const uint8_t *event)
    {
        return event[7];
    }

    static inline uint8_t hci_subevent_le_enhanced_connection_complete_get_status(const uint8_t *event)
    {
        return event[3];
    }
    static inline hci_con_handle_t hci_subevent_le_enhanced_connection_complete_get_connection_handle(const uint8_t *event)
    {
        return little_endian_read_16(event, 4);
    }
    static inline uint8_t hci_subevent_le_enhanced_connection_complete_get_role(const uint8_t *event)
    {
        return event[6];
    }
    static inline uint8_t hci_subevent_le_enhanced_connection_complete_get_peer_address_type(const uint8_t *event)
    {
        return event[7];
    }
    static inline uint16_t hci_subevent_le_enhanced_connection_complete_get_conn_interval(const uint8_t *event)
    {
        return little_endian_read_16(event, 26);
    }
    static inline uint16_t hci_subevent_le_enhanced_connection_complete_get_conn_latency(const uint8_t *event)
    {
        return little_endian_read_16(event, 28);
    }
    static inline uint16_t hci_subevent_le_enhanced_connection_complete_get_supervision_timeout(const uint8_t *event)
    {
        return little_endian_read_16(event, 30);
    }
    static inline uint8_t hci_subevent_le_enhanced_connection_complete_get_master_clock_accuracy(const uint8_t *event)
    {
        return event[32];
    }


    extern void ble_sm_setup_init(io_capability_t io_type, u8 auth_req, uint8_t min_key_size, u8 security_en);
    extern void sm_just_works_confirm(hci_con_handle_t con_handle);
    extern void sm_init(void);
    extern void sm_set_io_capabilities(io_capability_t io_capability);
    extern void sm_set_authentication_requirements(uint8_t auth_req);
    extern void sm_set_encryption_key_size_range(uint8_t min_size, uint8_t max_size);
    extern void sm_set_request_security(int enable);
    extern void sm_event_callback_set(void(*cbk_sm_ph)(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size));
    extern void att_server_init(uint8_t const *db, att_read_callback_t read_callback, att_write_callback_t write_callback);
    extern void att_server_register_packet_handler(btstack_packet_handler_t handler);
    extern void hci_event_callback_set(void(*cbk_ph)(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size));
    extern void ll_hci_connection_updata(u8 *data);

#endif


