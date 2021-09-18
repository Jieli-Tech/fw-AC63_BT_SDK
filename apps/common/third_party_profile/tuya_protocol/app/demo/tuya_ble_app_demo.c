#include "tuya_ble_stdlib.h"
#include "tuya_ble_type.h"
#include "tuya_ble_heap.h"
#include "tuya_ble_mem.h"
#include "tuya_ble_api.h"
#include "tuya_ble_port.h"
#include "tuya_ble_main.h"
#include "tuya_ble_secure.h"
#include "tuya_ble_data_handler.h"
#include "tuya_ble_storage.h"
#include "tuya_ble_sdk_version.h"
#include "tuya_ble_utils.h"
#include "tuya_ble_event.h"
#include "tuya_ble_app_demo.h"
//#include "tuya_ble_demo_version.h"
#include "tuya_ble_log.h"
#include "tuya_ota.h"
#include "system/generic/printf.h"

#include "log.h"
#include "system/includes.h"
//#include "ota.h"

tuya_ble_device_param_t device_param = {0};

#define LED_PIN IO_PORTA_01
static bool tuya_led_state = 0;
static const char device_id_test[] = "tuya58253f7c8ed1";
static const char auth_key_test[] = "I9ikbTKVMhfvEDgARBCtFbAWUpZIRdyv";
static const uint8_t mac_test[6] = {0xDC, 0x23, 0x4D, 0x65, 0xB0, 0x66}; //The actual MAC address is : 66:55:44:33:22:11


#define APP_CUSTOM_EVENT_1  1
#define APP_CUSTOM_EVENT_2  2
#define APP_CUSTOM_EVENT_3  3
#define APP_CUSTOM_EVENT_4  4
#define APP_CUSTOM_EVENT_5  5

static uint8_t dp_data_array[255 + 3];
static uint16_t dp_data_len = 0;

typedef struct {
    uint8_t data[50];
} custom_data_type_t;

void custom_data_process(int32_t evt_id, void *data)
{
    custom_data_type_t *event_1_data;
    TUYA_APP_LOG_DEBUG("custom event id = %d", evt_id);
    switch (evt_id) {
    case APP_CUSTOM_EVENT_1:
        event_1_data = (custom_data_type_t *)data;
        TUYA_APP_LOG_HEXDUMP_DEBUG("received APP_CUSTOM_EVENT_1 data:", event_1_data->data, 50);
        break;
    case APP_CUSTOM_EVENT_2:
        break;
    case APP_CUSTOM_EVENT_3:
        break;
    case APP_CUSTOM_EVENT_4:
        break;
    case APP_CUSTOM_EVENT_5:
        break;
    default:
        break;

    }
}

custom_data_type_t custom_data;

void custom_evt_1_send_test(uint8_t data)
{
    tuya_ble_custom_evt_t event;

    for (uint8_t i = 0; i < 50; i++) {
        custom_data.data[i] = data;
    }
    event.evt_id = APP_CUSTOM_EVENT_1;
    event.custom_event_handler = (void *)custom_data_process;
    event.data = &custom_data;
    tuya_ble_custom_event_send(event);
}

static uint16_t sn = 0;
static uint32_t time_stamp = 1587795793;

void tuya_data_init()
{
    struct {
        uint8_t id;
        uint8_t type;
        uint16_t len;
        uint8_t data;
    } p_dp_data;

    p_dp_data.id = 1;
    p_dp_data.type = 1;
    p_dp_data.len = 0x0100;
    p_dp_data.data = tuya_led_state;

#if (TUYA_BLE_PROTOCOL_VERSION_HIGN == 0x03)
    tuya_ble_dp_data_report(&p_dp_data, 5); //1
#endif
#if (TUYA_BLE_PROTOCOL_VERSION_HIGN == 0x04)
    tuya_ble_dp_data_send(sn, DP_SEND_TYPE_ACTIVE, DP_SEND_FOR_CLOUD_PANEL, DP_SEND_WITH_RESPONSE, &p_dp_data, 5);
#endif
}

void tuya_data_parse(tuya_ble_cb_evt_param_t *event)
{
    uint8_t *buf = event->dp_received_data.p_data;
    uint32_t sn = event->dp_received_data.sn;
    put_buf(buf, event->dp_received_data.data_len);
    struct {
        uint8_t id;
        uint8_t type;
        uint16_t len;
        uint8_t data;
    } p_dp_data;

    p_dp_data.id = buf[0];
    p_dp_data.type = buf[1];
    p_dp_data.len = 0x0100;
    p_dp_data.data = buf[4];

    printf("\n\n<--------------  tuya_data_parse  -------------->");
    printf("sn = %d, id = %d, type = %d, len = %d, data = %d", sn, p_dp_data.id, p_dp_data.type, p_dp_data.len, p_dp_data.data);


    switch (buf[0]) {
    case 1:
        printf("tuya switch control, onoff set to: %d\n", p_dp_data.data);
        tuya_led_state = p_dp_data.data;
        gpio_direction_output(LED_PIN, tuya_led_state);
        break;
    //case 2:
    //printf("tuya mode control, mode set to: %d\n", buf[3]);
    //break;
    default:
        printf("unknow control msg len = %d, data:", buf[2]);
        break;
    }
#if (TUYA_BLE_PROTOCOL_VERSION_HIGN == 0x03)
    tuya_ble_dp_data_report(&p_dp_data, 5); //1
#endif
#if (TUYA_BLE_PROTOCOL_VERSION_HIGN == 0x04)
    tuya_ble_dp_data_send(sn, DP_SEND_TYPE_ACTIVE, DP_SEND_FOR_CLOUD_PANEL, DP_SEND_WITH_RESPONSE, &p_dp_data, 5);
#endif

}

static void tuya_cb_handler(tuya_ble_cb_evt_param_t *event)
{
    int16_t result = 0;
    printf("tuya cb_handler event->evt=0x%x\n", event->evt);
    switch (event->evt) {
    case TUYA_BLE_CB_EVT_CONNECTE_STATUS:
        TUYA_APP_LOG_INFO("received tuya ble conncet status update event,current connect status = %d", event->connect_status);
        break;
    case TUYA_BLE_CB_EVT_DP_DATA_RECEIVED:
        tuya_data_parse(event);
        break;
    case TUYA_BLE_CB_EVT_DP_DATA_REPORT_RESPONSE:
        TUYA_APP_LOG_INFO("received dp data report response result code =%d", event->dp_response_data.status);

        break;
    case TUYA_BLE_CB_EVT_DP_DATA_WTTH_TIME_REPORT_RESPONSE:
        TUYA_APP_LOG_INFO("received dp data report response result code =%d", event->dp_response_data.status);
        break;
    case TUYA_BLE_CB_EVT_DP_DATA_WITH_FLAG_REPORT_RESPONSE:
        TUYA_APP_LOG_INFO("received dp data with flag report response sn = %d , flag = %d , result code =%d", event->dp_with_flag_response_data.sn, event->dp_with_flag_response_data.mode
                          , event->dp_with_flag_response_data.status);

        break;
    case TUYA_BLE_CB_EVT_DP_DATA_WITH_FLAG_AND_TIME_REPORT_RESPONSE:
        TUYA_APP_LOG_INFO("received dp data with flag and time report response sn = %d , flag = %d , result code =%d", event->dp_with_flag_and_time_response_data.sn,
                          event->dp_with_flag_and_time_response_data.mode, event->dp_with_flag_and_time_response_data.status);

        break;
    case TUYA_BLE_CB_EVT_UNBOUND:

        TUYA_APP_LOG_INFO("received unbound req");

        break;
    case TUYA_BLE_CB_EVT_ANOMALY_UNBOUND:

        TUYA_APP_LOG_INFO("received anomaly unbound req");

        break;
    case TUYA_BLE_CB_EVT_DEVICE_RESET:

        TUYA_APP_LOG_INFO("received device reset req");

        break;
    case TUYA_BLE_CB_EVT_DP_QUERY:
        TUYA_APP_LOG_INFO("received TUYA_BLE_CB_EVT_DP_QUERY event");
        if (dp_data_len > 0) {
#if (TUYA_BLE_PROTOCOL_VERSION_HIGN == 0x03)
            tuya_ble_dp_data_report(dp_data_array, dp_data_len);
#endif
#if (TUYA_BLE_PROTOCOL_VERSION_HIGN == 0x04)
            tuya_ble_dp_data_send(sn, DP_SEND_TYPE_ACTIVE, DP_SEND_FOR_CLOUD_PANEL, DP_SEND_WITH_RESPONSE, dp_data_array, dp_data_len);
#endif
        }
        break;
    case TUYA_BLE_CB_EVT_OTA_DATA:
        tuya_ota_proc(event->ota_data.type, event->ota_data.p_data, event->ota_data.data_len);
        break;
    case TUYA_BLE_CB_EVT_NETWORK_INFO:
        TUYA_APP_LOG_INFO("received net info : %s", event->network_data.p_data);
        tuya_ble_net_config_response(result);
        break;
    case TUYA_BLE_CB_EVT_WIFI_SSID:

        break;
    case TUYA_BLE_CB_EVT_TIME_STAMP:
        TUYA_APP_LOG_INFO("received unix timestamp : %s ,time_zone : %d", event->timestamp_data.timestamp_string, event->timestamp_data.time_zone);
        tuya_data_init();
        break;
    case TUYA_BLE_CB_EVT_TIME_NORMAL:

        break;
    case TUYA_BLE_CB_EVT_DATA_PASSTHROUGH:
        TUYA_APP_LOG_HEXDUMP_DEBUG("received ble passthrough data :", event->ble_passthrough_data.p_data, event->ble_passthrough_data.data_len);
        tuya_ble_data_passthrough(event->ble_passthrough_data.p_data, event->ble_passthrough_data.data_len);
        break;
    default:
        TUYA_APP_LOG_WARNING("app_tuya_cb_queue msg: unknown event type 0x%04x", event->evt);
        break;
    }
    tuya_ble_inter_event_response(event);
}


void tuya_ble_app_init(void)
{
    device_param.device_id_len = 16;    //If use the license stored by the SDK,initialized to 0, Otherwise 16 or 20.

    int ret = 0;
    device_param.use_ext_license_key = 1;
    if (device_param.device_id_len == 16) {
        memcpy(device_param.auth_key, (void *)auth_key_test, AUTH_KEY_LEN);
        memcpy(device_param.device_id, (void *)device_id_test, DEVICE_ID_LEN);
        memcpy(device_param.mac_addr.addr, mac_test, 6);
        device_param.mac_addr.addr_type = TUYA_BLE_ADDRESS_TYPE_RANDOM;
    }
    device_param.p_type = TUYA_BLE_PRODUCT_ID_TYPE_PID;
    device_param.product_id_len = 8;
    memcpy(device_param.product_id, APP_PRODUCT_ID, 8);
    device_param.firmware_version = TY_APP_VER_NUM;
    device_param.hardware_version = TY_HARD_VER_NUM;
    printf("HJY:12345\n");
    tuya_ble_sdk_init(&device_param);
    ret = tuya_ble_callback_queue_register(tuya_cb_handler);
    y_printf("tuya_ble_callback_queue_register,ret=%d\n", ret);

    //tuya_ota_init();

    //TUYA_APP_LOG_INFO("demo project version : "TUYA_BLE_DEMO_VERSION_STR);
    TUYA_APP_LOG_INFO("app version : "TY_APP_VER_STR);

}
