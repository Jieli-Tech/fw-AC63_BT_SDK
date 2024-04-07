// binary representation
// attribute size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)

#ifndef _FMNA_API_H
#define _FMNA_API_H

#include <stdint.h>
#include "bluetooth.h"

#define ret_code_t                  int
#define fmna_ret_code_t             int

//ret_code & fmna_ret_code_t  return,list result
#define FMY_SUCCESS                   0 ///< Successful command
#define FMY_ERROR_INTERNAL            1 ///< Internal Error
#define FMY_ERROR_INVALID_STATE       2 ///< Invalid state, operation disallowed in this state
#define FMY_ERROR_INVALID_LENGTH      3 ///< Invalid Length
#define FMY_ERROR_INVALID_DATA        4 ///< Invalid Data
#define FMY_ERROR_NULL                5 ///< Null Pointer
#define FMY_ERROR_NO_MEM              6 ///< No memory
#define FMY_ERROR_WRITE_FLASH         7 ///<



typedef enum {
    FMNA_SM_BOOT = 0,
    FMNA_SM_PAIR,
    FMNA_SM_SEPARATED,
    FMNA_SM_NEARBY,
    FMNA_SM_CONNECTING,
    FMNA_SM_FMNA_PAIR,
    FMNA_SM_FMNA_PAIR_COMPLETE,
    FMNA_SM_CONNECTED,
    FMNA_SM_DISCONNECTING,
    FMNA_SM_NOCHANGE,
    FMNA_SM_UNPAIR
} FMNA_SM_State_t;

typedef enum {
    FMNA_SM_EVENT_BOOT = 0,
    FMNA_SM_EVENT_NEARBY_SEPARATED_TIMEOUT,
    FMNA_SM_EVENT_KEY_ROTATE,
    FMNA_SM_EVENT_BONDED,
    FMNA_SM_EVENT_UNBONDED,
    FMNA_SM_EVENT_CONNECTED,
    FMNA_SM_EVENT_DISCONNECTED,
    FMNA_SM_EVENT_NEARBY,
    FMNA_SM_EVENT_SEPARATED,
    FMNA_SM_EVENT_PAIR,
    FMNA_SM_EVENT_SOUND_START,
    FMNA_SM_EVENT_SOUND_STOP,
    FMNA_SM_EVENT_SOUND_COMPLETE,
    FMNA_SM_EVENT_LOST_UT_SPEAKER_START,
    FMNA_SM_EVENT_FMNA_PAIRING_INITIATE,
    FMNA_SM_EVENT_FMNA_PAIRING_FINALIZE,
    FMNA_SM_EVENT_FMNA_PAIRING_COMPLETE,
    FMNA_SM_EVENT_FMNA_PAIRING_MFITOKEN,
    FMNA_SM_EVENT_MOTION_DETECTED,
    FMNA_SM_EVENT_DEBUG_RESET_INTO_SEPARATED,

    FMNA_SM_EVENT_CALL_FUNC_HANDLER = 0x80,
} FMNA_SM_Event_t;

typedef enum {
    BAT_STATE_FULL = 0,
    BAT_STATE_MEDIUM,
    BAT_STATE_LOW,
    BAT_STATE_CRITICALLY_LOW,//5.1.4,stop advertising
} fmna_bat_state_level_t;

typedef enum {
    FMNA_ADV_MODE_FAST = 0,
    FMNA_ADV_MODE_SLOW,
} FMNA_ADV_Mode_t;

typedef enum {
    FMNA_SOUND_INIT = 0,
    FMNA_SOUND_START,
    FMNA_SOUND_STOP,
} FMNA_SOUND_OP_t;


enum {
    FMNA_OTA_OP_SUCC = 0,
    FMNA_OTA_OP_NO_SPACE,
    FMNA_OTA_OP_INIT_FAIL,
    FMNA_OTA_OP_WRITE_FAIL,
    FMNA_OTA_OP_CRC_FAIL,
    FMNA_OTA_OP_MALLOC_FAIL,
    FMNA_OTA_OP_PKT_NUM_ERR,
    FMNA_OTA_OP_LEN_ERR,
    FMNA_OTA_OP_OTHER_ERR,
};


typedef enum {
    FMNA_UARP_OTA_REQ = 1,
    FMNA_UARP_OTA_FILE_INFO,
    FMNA_UARP_OTA_DATA,
    FMNA_UARP_OTA_END,
    FMNA_UARP_OTA_DISCONNECT,
} uarp_cmd_type_t;

/*
0 = Powered
1 = Non-rechargeable battery
2 = Rechargeable battery
*/
typedef enum {
    FMNA_BAT_POWERED = 0,
    FMNA_BAT_NON_RECHARGEABLE,
    FMNA_BAT_RECHARGEABLE,
} FMNA_battery_level_t;

//Accessory category
#define   FMY_CATEGORY_Finder           1
#define   FMY_CATEGORY_Other           128
#define   FMY_CATEGORY_Luggage         129
#define   FMY_CATEGORY_Backpack        130
#define   FMY_CATEGORY_Jacket          131
#define   FMY_CATEGORY_Coat            132
#define   FMY_CATEGORY_Shoes           133
#define   FMY_CATEGORY_Bike            134
#define   FMY_CATEGORY_Scooter         135
#define   FMY_CATEGORY_Stroller        136
#define   FMY_CATEGORY_Wheelchair      137
#define   FMY_CATEGORY_Boat            138
#define   FMY_CATEGORY_Helmet          139
#define   FMY_CATEGORY_Skateboard      140
#define   FMY_CATEGORY_Skis            141
#define   FMY_CATEGORY_Snowboard       142
#define   FMY_CATEGORY_Surfboard       143
#define   FMY_CATEGORY_Camera          144
#define   FMY_CATEGORY_Laptop          145
#define   FMY_CATEGORY_Watch           146
#define   FMY_CATEGORY_Flash_drive     147
#define   FMY_CATEGORY_Drone           148
#define   FMY_CATEGORY_Headphones      149
#define   FMY_CATEGORY_Earphones       150
#define   FMY_CATEGORY_Inhaler         151
#define   FMY_CATEGORY_Sunglasses      152
#define   FMY_CATEGORY_Handbag         153
#define   FMY_CATEGORY_Wallet          154
#define   FMY_CATEGORY_Umbrella        155
#define   FMY_CATEGORY_Water_bottle    156
#define   FMY_CATEGORY_Tools_or_tool_box 157
#define   FMY_CATEGORY_Keys              158
#define   FMY_CATEGORY_Smart_case        159
#define   FMY_CATEGORY_Remote            160
#define   FMY_CATEGORY_Hat               161
#define   FMY_CATEGORY_Motorbike         162
#define   FMY_CATEGORY_Consumer_electronic_device     163
#define   FMY_CATEGORY_Apparel                        164
#define   FMY_CATEGORY_Transportation_device          165
#define   FMY_CATEGORY_Sports_equipment               166
#define   FMY_CATEGORY_Personal_item                  167

//Accessory capability
#define FMY_CAPABILITY_SUPPORTS_PLAY_SOUND            BIT(0) //Supports play sound
#define FMY_CAPABILITY_SUPPORTS_MOTION_DETECTOR_UT    BIT(1) //Supports motion detector UT
#define FMY_CAPABILITY_SUPPORTS_SN_LOOKUP_BY_NFC      BIT(2) //Supports serial number lookup by NFC
#define FMY_CAPABILITY_SUPPORTS_SN_LOOKUP_BY_BLE      BIT(3) //Supports serial number lookup by BLE
#define FMY_CAPABILITY_SUPPORTS_FW_UPDATE_SERVICE     BIT(4) //Supports ﬁrmware update service

//uuid,mac type
#define  STATIC_ADV_ADDR_TYPE_MASK                  (0x03 << 6)
#define  FMY_UUID_SERVICE                           0xFD44
#define  APPLE_VENDOR_ID                            0x004C

typedef struct {
    uint16_t pairing_control_point_handle;
    uint16_t owner_cfg_control_point_handle;
    uint16_t non_owner_control_point_handle;
    uint16_t owner_info_porint_handle;
    uint16_t debug_control_point_handle;
    uint16_t firmware_update_handle;
} fmna_att_handle_t;


typedef struct {
    int (*evnet_post_msg)(uint8_t priv_event, void *evt_data, uint32_t value, void *handler);
    int (*set_adv_data)(uint8_t fmna_state, uint8_t *fmna_payload, uint8_t size);
    int (*set_adv_mode)(uint8_t mode);
    int (*set_adv_enable)(uint8_t enable);
    int (*get_mac)(uint8_t *mac);
    int (*set_mac)(uint8_t *mac);
    int (*att_send_data)(uint16_t conn_handle, uint16_t att_handle, uint8_t *data, uint16_t len, att_op_type_e op_type);
    int (*get_battery_level)(void);
    int (*call_disconnect)(uint16_t conn_handle, uint8_t reason);
    int (*sound_control)(FMNA_SOUND_OP_t op);
    int (*uarp_ota_process)(uarp_cmd_type_t cmd_type, uint8_t *recv_data, uint32_t recv_len);
    int (*sensor_init)(void);
    int (*sensor_deinit)(void);
    bool (*motion_deteted)(void);
    uint16_t (*state_callback)(FMNA_SM_State_t fmy_state, const char *state_string);
    bool (*check_capability)(uint8_t cap);
} fmna_app_api_t;

typedef struct {
    //读取信息时,指针为0则不获取对应项信息
    uint8_t  *serial_number;//16byte
    uint8_t  *uuid;//16byte
    uint8_t  *token;//1024byte
} fmna_input_cfg_t;
//--------------------------------------------------------------------------------
int fmna_main_start(void);
int fmna_main_exit(void);

void fmna_gatt_set_att_handle_table(const fmna_att_handle_t *att_table);
void fmna_gatt_platform_connect(uint16_t conn_handle);
void fmna_state_machine_event_handle(void *event);
fmna_ret_code_t fmna_gatt_config_char_write_handler(uint16_t conn_handle, uint16_t uuid, uint16_t length, uint8_t const *data);
fmna_ret_code_t fmna_gatt_pairing_char_authorized_write_handler(uint16_t conn_handle, uint16_t uuid, uint16_t length, uint8_t const *data);
fmna_ret_code_t fmna_gatt_nonown_char_write_handler(uint16_t conn_handle, uint16_t uuid, uint16_t length, uint8_t const *data);
fmna_ret_code_t fmna_gatt_paired_owner_char_write_handler(uint16_t conn_handle, uint16_t uuid, uint16_t length, uint8_t const *data);
fmna_ret_code_t fmna_gatt_debug_char_write_handler(uint16_t conn_handle, uint16_t uuid, uint16_t length, uint8_t const *data);
fmna_ret_code_t fmna_gatt_uarp_char_write_handler(uint16_t conn_handle, uint16_t uuid, uint16_t length, uint8_t const *data);
void fmna_debug_control_point_rx_handler(uint16_t conn_handle, uint8_t const *data, uint16_t length) ;
void fmna_save_mac_vm_id(uint16_t vm_id);
void fmna_set_app_api(fmna_app_api_t *api_table);
bool fmna_connection_is_fmna_paired(void);
uint8_t fmna_get_current_state(void);
void fmna_gatt_set_mtu_size(uint16_t conn_handle, uint16_t mtu_size);
const uint8_t *fmna_get_product_data(void);
uint32_t fmna_version_get_fw_version(void);
uint8_t *fmna_version_get_network_version(void);
void fmna_set_accessory_category(const uint8_t *accessory_category);
const uint8_t *fmna_get_accessory_category(void);
uint32_t fmna_pm_peer_count(void);
void fmna_version_init(uint16_t major_number, uint8_t minor_number, uint8_t revision_number);
void fmna_paired_serialnumber_lookup_enable(uint8_t enable);
int fmna_get_paired_serialnumber_lookup_state(void);

void fmna_connection_connected_handler(uint16_t conn_handle, uint16_t conn_interval);
void fmna_connection_conn_param_update_handler(uint16_t conn_handle, uint16_t conn_interval);
void fmna_connection_disconnected_handler(uint16_t conn_handle, uint8_t disconnect_reason);
void fmna_connection_encryption_change_complete(uint16_t conn_handle, bool is_sucess);
int fmna_connection_pair_request_check(uint16_t conn_handle);
void fmna_connection_set_sys_max_connections(uint8_t max_connections);
void fmna_config_user_vm_rang(uint16_t vm_start_id, uint16_t vm_end_id);
void fmna_connection_disconnect_all(void);
void fmna_gatt_platform_recieve_indication_response(uint16_t conn_handle, uint16_t att_handle);
void fmna_plaform_reset_config(void);
void fmna_set_product_data(const uint8_t *data);

int fmna_Base64Decode(char *inStr, char *outHex, int outBufLen);
int uuid_str_to_hex(const char *uuid_str, uint8_t *out_buf);

void fmna_user_cfg_set_patch(const char *path);
ret_code_t fmna_user_cfg_open(void);
ret_code_t fmna_user_cfg_close(void);
ret_code_t fmna_user_cfg_write(fmna_input_cfg_t *input_cfg);
ret_code_t fmna_user_cfg_read(fmna_input_cfg_t *cfg);
ret_code_t fmna_set_crypto_enc_key_config(const char *server_enc_key, const char *signature_vf_key);
bool fmna_user_cfg_is_exist(void);

#endif
