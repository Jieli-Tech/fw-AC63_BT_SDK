/*********************************************************************************************
    *   Filename        : ble_trans_ios_services.c

    *   Description     :配置搜索ios自带的ancs 和 ams服务处理

    *   Author          :JM

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2022-10-18 11:14

    *   Copyright:(c)JIELI  2011-2016  @ , All Rights Reserved.
*********************************************************************************************/
#include "system/app_core.h"
#include "system/includes.h"

#include "app_config.h"
#include "app_action.h"

#include "btstack/btstack_task.h"
#include "btstack/bluetooth.h"
#include "user_cfg.h"
#include "vm.h"
#include "btcontroller_modules.h"
#include "bt_common.h"
#include "3th_profile_api.h"
#include "le_common.h"
#include "rcsp_bluetooth.h"
#include "custom_cfg.h"
#include "btstack/btstack_event.h"
#include "le_client_demo.h"
#include "gatt_common/le_gatt_common.h"

#if CONFIG_APP_SPP_LE && CONFIG_BT_GATT_CLIENT_NUM && CONFIG_BT_SM_SUPPORT_ENABLE

#if LE_DEBUG_PRINT_EN
#define log_info(x, ...)  printf("[TRANS-IOS]" x " ", ## __VA_ARGS__)
/* #define log_info(x, ...)  y_printf("[TRANS-IOS]" x " ", ## __VA_ARGS__) */
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

#if TRANS_ANCS_EN
//profile event
#define ANCS_SUBEVENT_CLIENT_CONNECTED                              0xF0 /*搜索到服务*/
#define ANCS_SUBEVENT_CLIENT_NOTIFICATION                           0xF1
#define ANCS_SUBEVENT_CLIENT_DISCONNECTED                           0xF2

static u8 ancs_service_ok;
//ancs get info buffer
#define ANCS_INFO_BUFFER_SIZE  (512)
static u8 ancs_info_buffer[ANCS_INFO_BUFFER_SIZE];


void ancs_client_init(void);
void ancs_client_exit(void);
void ancs_client_register_callback(btstack_packet_handler_t callback);
const char *ancs_client_attribute_name_for_id(int id);
void ancs_set_notification_buffer(u8 *buffer, u16 buffer_size);
u32 get_notification_uid(void);
u16 get_controlpoint_handle(void);
void ancs_set_out_callback(void *cb);
#endif

//ams
#if TRANS_AMS_EN
//profile event
#define AMS_SUBEVENT_CLIENT_CONNECTED                               0xF3/*搜索到服务*/
#define AMS_SUBEVENT_CLIENT_NOTIFICATION                            0xF4
#define AMS_SUBEVENT_CLIENT_DISCONNECTED                            0xF5

static u8 ams_service_ok;

void ams_client_init(void);
void ams_client_exit(void);
void ams_client_register_callback(btstack_packet_handler_t handler);
const char *ams_get_entity_id_name(u8 entity_id);
const char *ams_get_entity_attribute_name(u8 entity_id, u8 attr_id);
#endif

//-------------------------------------------------------------------------------------
//处理 ios services 处理回调的事件
static void trans_ios_services_event_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size)
{
    int mtu;
    u32 tmp;
    const char *attribute_name;

    switch (packet_type) {
    case HCI_EVENT_PACKET:
        switch (hci_event_packet_get_type(packet)) {
#if TRANS_ANCS_EN
        case HCI_EVENT_ANCS_META:
            switch (hci_event_ancs_meta_get_subevent_code(packet)) {
            case ANCS_SUBEVENT_CLIENT_NOTIFICATION:
                log_info("ANCS_SUBEVENT_CLIENT_NOTIFICATION\n");
                attribute_name = ancs_client_attribute_name_for_id(ancs_subevent_client_notification_get_attribute_id(packet));
                if (!attribute_name) {
                    log_info("ANCS unknow attribute_id :%d\n", ancs_subevent_client_notification_get_attribute_id(packet));
                    break;
                } else {
                    u16 attribute_strlen = little_endian_read_16(packet, 7);
                    u8 *attribute_str = (void *)little_endian_read_32(packet, 9);
                    log_info("ANCS Notification: %s - %s \n", attribute_name, attribute_str);
                }
                break;

            case ANCS_SUBEVENT_CLIENT_CONNECTED:
                log_info("ANCS_SUBEVENT_CLIENT_CONNECTED\n");
                ancs_service_ok = 1;
                break;

            case ANCS_SUBEVENT_CLIENT_DISCONNECTED:
                log_info("ANCS_SUBEVENT_CLIENT_DISCONNECTED\n");
                ancs_service_ok = 0;
                break;

            default:
                break;
            }

            break;
#endif

#if TRANS_AMS_EN
        case HCI_EVENT_AMS_META:
            switch (packet[2]) {
            case AMS_SUBEVENT_CLIENT_NOTIFICATION: {
                log_info("AMS_SUBEVENT_CLIENT_NOTIFICATION\n");
                u16 Entity_Update_len = little_endian_read_16(packet, 7);
                u8 *Entity_Update_data = (void *)little_endian_read_32(packet, 9);
                /* log_info("EntityID:%d, AttributeID:%d, Flags:%d, utf8_len(%d):",\ */
                /* Entity_Update_data[0],Entity_Update_data[1],Entity_Update_data[2],Entity_Update_len-3); */
                log_info("AMS %s(%s), Flags:%d, utf8_len(%d)\n", ams_get_entity_id_name(Entity_Update_data[0]),
                         ams_get_entity_attribute_name(Entity_Update_data[0], Entity_Update_data[1]),
                         Entity_Update_data[2], Entity_Update_len - 3);

#if 1 //for printf debug
                static u8 music_files_buf[128];
                u8 str_len = Entity_Update_len - 3;
                if (str_len > sizeof(music_files_buf)) {
                    str_len = sizeof(music_files_buf) - 1;
                }
                memcpy(music_files_buf, &Entity_Update_data[3], str_len);
                music_files_buf[str_len] = 0;
                log_info("AMS string:%s\n", music_files_buf);
#endif

                log_info_hexdump(&Entity_Update_data[3], Entity_Update_len - 3);
            }
            break;

            case AMS_SUBEVENT_CLIENT_CONNECTED:
                log_info("AMS_SUBEVENT_CLIENT_CONNECTED\n");
                ams_service_ok = 1;
                break;

            case AMS_SUBEVENT_CLIENT_DISCONNECTED:
                log_info("AMS_SUBEVENT_CLIENT_DISCONNECTED\n");
                ams_service_ok = 0;
                break;


            default:
                break;
            }
            break;
#endif

        }
        break;
    }
}

/*定时测试发送 ams播放控制命令*/
static void trans_ams_timer_handle(void)
{
#if TRANS_AMS_EN
    if (ams_service_ok) {
        log_info("ams for test: send pp_key\n");
        ams_send_request_command(AMS_RemoteCommandIDTogglePlayPause);
    }
#endif
}

/*推送的uid处理*/
static void trans_ancs_message_output(u8 *packet, u16 size)
{
#if TRANS_ANCS_EN
    u8 *value;
    u32 ancs_notification_uid;
    value = &packet[8];
    ancs_notification_uid = little_endian_read_32(value, 4);

    log_info("Notification: EventID %02x, EventFlags %02x, CategoryID %02x, CategoryCount %u, UID %04x",
             value[0], value[1], value[2], value[3], ancs_notification_uid);
    if (value[1] & BIT(2)) {
        log_info("is PreExisting Message!!!");
    }

    if (value[0] == 2) { //0:added 1:modifiled 2:removed
        log_info("ancs remove message:");
    } else if (value[0] == 0) {
        log_info("ancs add message:");
    } else if (value[0] == 1) {
        log_info("ancs modifiled message:");
    } else {
        log_info("ancs Unknown op:");
    }

#endif
}

//trans ios的 ams 和 ancs 初始化
void trans_ios_services_init(void)
{
    log_info("%s\n", __FUNCTION__);

#if TRANS_ANCS_EN || TRANS_AMS_EN
    if ((!config_le_sm_support_enable) || (!config_le_gatt_client_num)) {
        log_info("ANCS AMS need sm and client support!!!\n");
        ASSERT(0);
    }
#endif

    /* if (config_le_gatt_client_num) {        */
    /* gatt_client_init(); */
    /* } */

#if TRANS_ANCS_EN
    log_info("ANCS init...\n");
    //setup ANCS clent
    ancs_client_init();
    ancs_set_notification_buffer(ancs_info_buffer, sizeof(ancs_info_buffer));
    ancs_client_register_callback(&trans_ios_services_event_packet_handler);
    ancs_set_out_callback(trans_ancs_message_output);
    ancs_service_ok = 0;

#endif

#if TRANS_AMS_EN
    log_info("AMS init...\n");
    ams_client_init();
    ams_client_register_callback(&trans_ios_services_event_packet_handler);
    ams_entity_attribute_config(AMS_IDPlayer_ENABLE | AMS_IDQueue_ENABLE | AMS_IDTrack_ENABLE);
    ams_service_ok = 0;
    /* ams_entity_attribute_config(AMS_IDTrack_ENABLE); */
    sys_timer_add(0, trans_ams_timer_handle, 8000);/*for test*/
#endif
}

//trans exit
void trans_ios_services_exit(void)
{
    log_info("%s\n", __FUNCTION__);
}

#endif


