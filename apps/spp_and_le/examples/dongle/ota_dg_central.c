/*********************************************************************************************
    *   Filename        : .c

    *   Description     :

    *   Author          : ZQ

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2022-05-25 11:14

*********************************************************************************************/
#include "system/app_core.h"
#include "system/includes.h"
#include "server/server_core.h"
#include "app_config.h"
#include "app_action.h"
#include "os/os_api.h"
#include "btcontroller_config.h"
#include "btctrler/btctrler_task.h"
#include "config/config_transport.h"
#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "bt_common.h"
#include "rcsp_bluetooth.h"
#include "rcsp_user_update.h"
#include "app_charge.h"
#include "app_power_manage.h"
#include "le_client_demo.h"
#include "usb/device/hid.h"
#include "app_comm_bt.h"
#include "le_gatt_common.h"

#if RCSP_BTMATE_EN && CONFIG_APP_DONGLE && TCFG_PC_ENABLE && TCFG_USB_CUSTOM_HID_ENABLE

#if 1
#define log_info(x, ...)  printf("[BLE_DG_OTA]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

#define DONGLE_OTA_VERSION                  0
#define HID_OTA_DEVICE_NUM                  CONFIG_BT_GATT_CLIENT_NUM//支持连接的远端ota设备
#define HID_RX_HANDLER_HEND_TAG             0x4A4C
#define HID_RX_HANDLER_TAIL_TAG             0xED
#define HID_SEND_DATA_TAG_LONG              8//除了data之外的包长度
#define HID_USB_SEND_MAX                    64
#define BLE_FRIST_CONNECTION_CHANNEL        0x50
//USB串口指令
enum {
    //APP_BT_EVENT
    APP_CMD_RCSP_DATA = 0,
    APP_CMD_GET_DONGLE_MASSAGE,
    APP_CMD_GET_CONNECT_DEVICE,//DONGLE_REPLY_SEARCH_DEVICE,
    APP_CMD_RETURN_SUCC,
    APP_CMD_RECONNECT_DEVICE,
    APP_CMD_DISCONNECT_DEVICE,
    APP_CMD_AUTH_FLAG,
    //自定义命令
    APP_CMD_CUSTOM = 0xFF,
};
//通讯通道
//-----channel_0: pc->dongle
//-----channel_1: dongle->pc
//-----channel_2: pc->usb透传
//-----channel_3~9: 远端升级透传
enum {
    HID_RX_HANDLER_CHANNEL_COMMAND = 0x00 + DONGLE_OTA_VERSION,
    HID_RX_HANDLER_CHANNEL_RESPONSE = 0x10 + DONGLE_OTA_VERSION,
    HID_RX_HANDLER_CHANNEL_USB = 0x20 + DONGLE_OTA_VERSION,
    HID_RX_HANDLER_CHANNEL_REMOTE1 = 0x30 + DONGLE_OTA_VERSION,
    HID_RX_HANDLER_CHANNEL_REMOTE2 = 0x40 + DONGLE_OTA_VERSION,
    HID_RX_HANDLER_CHANNEL_REMOTE3 = 0x50 + DONGLE_OTA_VERSION,
    HID_RX_HANDLER_CHANNEL_REMOTE4 = 0x60 + DONGLE_OTA_VERSION,
    HID_RX_HANDLER_CHANNEL_REMOTE5 = 0x70 + DONGLE_OTA_VERSION,
    HID_RX_HANDLER_CHANNEL_REMOTE6 = 0x80 + DONGLE_OTA_VERSION,
    HID_RX_HANDLER_CHANNEL_REMOTE7 = 0x90 + DONGLE_OTA_VERSION,
    HID_RX_HANDLER_CHANNEL_REMOTE8 = 0xA0 + DONGLE_OTA_VERSION,
};
typedef struct {
    u8 auth_flag;//设备认证信息
    u8 ota_support_flag;//设备是否支持ota
    u8 conn_address[7];//连接地址
} device_massage;

typedef struct {
    u8 reconn_channel;//回连接通道
    u8 reconn_address[7];//回连接地址
    u8 reconn_map[HID_OTA_DEVICE_NUM];
    u8 reonn_channel_change;
} ronn_massage;

typedef struct {
    u8 data[HID_USB_SEND_MAX * 9];
} usb_data_info_t;

static device_massage remote_device[HID_OTA_DEVICE_NUM  + 1];
static ronn_massage deviece_ronn_massage;
/* static u8 reconn_channel = 3, reconn_address[7]; */
static u16 reconn_timer = 0;
static u8 is_reconn_address_device = 0;

/*************************************************************************************************/
/*!
 *  \brief      trans_channel set to connection_handle
 *
 *  \param      [in]
 *
 *  \return       [out]
 *
 *  \note
 */
/*************************************************************************************************/
static u16 trans_channel_set_to_connection_handle(u16 trans_channel)
{
    u16 connection_handle = trans_channel / 16 + BLE_FRIST_CONNECTION_CHANNEL - (HID_RX_HANDLER_CHANNEL_REMOTE1 / 16);

    if (connection_handle < BLE_FRIST_CONNECTION_CHANNEL) {
        log_info("trans_channel_set_to_connection_handle err!!");
        return 0;
    } else {
        return connection_handle;
    }
}

/*************************************************************************************************/
/*!
 *  \brief      connection_handle  set to trans_channel
 *
 *  \param      [in]
 *
 *  \return       [out]
 *
 *  \note
 */
/*************************************************************************************************/
static u16 connection_handle_set_to_trans_channel(u16 connection_handle)
{
    u16 trans_channel = (connection_handle + (HID_RX_HANDLER_CHANNEL_REMOTE1 / 16) - BLE_FRIST_CONNECTION_CHANNEL) * 16;
    if (trans_channel < (HID_RX_HANDLER_CHANNEL_REMOTE1 / 16)) {
        log_info("connection_handle_set_to_trans_channel err!!");
        return 0;
    } else {
        return trans_channel;
    }
}
/*************************************************************************************************/
/*!
 *  \brief      dongle发送给pc端(dongle直接发给pc)
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void dongle_send_data_to_pc(u16 channel, u8 *data, u16 len, u16 cmd_type)
{
    len = len + 1;//加上cmd_type长度
    u8 send_data_tag[HID_SEND_DATA_TAG_LONG - 1] = {'J', 'L', channel, 0x00, len / 256, len % 256, cmd_type};
    len = len - 1;//去除cmd_type长度
    u8 send_data[((len + HID_SEND_DATA_TAG_LONG - 1) / HID_USB_SEND_MAX + 1) * HID_USB_SEND_MAX];
    /* log_info("dongle send data to pc: %d", ((len + HID_SEND_DATA_TAG_LONG - 1) / HID_USB_SEND_MAX + 1) * HID_USB_SEND_MAX); */
    memset(send_data, 0x00, sizeof(send_data));

    memcpy(&send_data, &send_data_tag, HID_SEND_DATA_TAG_LONG - 1);
    memcpy(&send_data[HID_SEND_DATA_TAG_LONG - 1], data, len);
    send_data[HID_SEND_DATA_TAG_LONG + len - 1] = HID_RX_HANDLER_TAIL_TAG;

    //发送数据分包为每包64bytes发包,包含填包
    /* if ((len + HID_SEND_DATA_TAG_LONG) <= HID_USB_SEND_MAX) { */
    /*     log_info("dongle send data to pc: %d", custom_hid_tx_data(0, &send_data[0], HID_USB_SEND_MAX)); */
    /*     put_buf(&send_data[0], HID_USB_SEND_MAX); */
    /* } else { */
    u8 i = (len + HID_SEND_DATA_TAG_LONG) / HID_USB_SEND_MAX;
    i = ((len + HID_SEND_DATA_TAG_LONG) % HID_USB_SEND_MAX) ? (i + 1) : (i);
    for (u8 j = 1; j <= i; j++) {
        log_info("dongle send data to pc: %d", custom_hid_tx_data(0, &send_data[(j - 1) * 64], HID_USB_SEND_MAX));
        put_buf(&send_data[(j - 1) * 64], HID_USB_SEND_MAX);
    }
    /* } */
}

/*************************************************************************************************/
/*!
 *  \brief      dongle发送给pc端(从机透传给dongle,dongle转发给pc)
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void dongle_send_data_to_pc_2(u16 con_handle, u8 *data, u16 len)
{
    u8 dongle_to_pc_handle;
    u8 channel_seq = connection_handle_set_to_trans_channel(con_handle);

    for (u8 j = 0; j < CONFIG_BT_GATT_CLIENT_NUM; j++) {
        if (channel_seq / 16 == deviece_ronn_massage.reconn_map[j]) {
            dongle_to_pc_handle = j;
        }
    }

    dongle_send_data_to_pc((dongle_to_pc_handle + (HID_RX_HANDLER_CHANNEL_REMOTE1 / 16)) * 16, data, len, APP_CMD_RCSP_DATA);
}

/*************************************************************************************************/
/*!
 *  \brief      dongle发送给pc端(dongle RCSP透传给dongle,dongle转发给pc)
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void dongle_send_data_to_pc_3(u8 *data, u16 len)
{
    dongle_send_data_to_pc(0x20, data, len, APP_CMD_RCSP_DATA);
}

/*************************************************************************************************/
/*!
 *  \brief      dongle->pc 在线信息表---在ble_dg_central.c中连接or断开时触发
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void dongle_return_online_list(void)
{
    //返回所有gatt_client设备
    log_info("Notify pc device online or offline");
    u8 online_number = 0, new_reconn_channel = 0xff;
    u8 online_list[(HID_OTA_DEVICE_NUM + 1) * 2 + 1];

    if (custom_hid_get_ready(0)) {
        /* if (0) { */
        online_list[online_number * 2 + 1] = 0x02;
        online_list[online_number * 2 + 2] = 0x01;
    } else {
        online_list[online_number * 2 + 1] = 0x02;
        online_list[online_number * 2 + 2] = 0x00;
    }
    online_number++;

    if (is_reconn_address_device == 2 && (CONFIG_BT_GATT_CLIENT_NUM > 1)) {
        for (u8 i = 0; i < CONFIG_BT_GATT_CLIENT_NUM; i++) {
            u16 con_handle = ble_comm_dev_get_handle(i, GATT_ROLE_CLIENT);
            if (con_handle && (is_succ_connection() != 1)) {
                u8 *address_device = dg_central_get_conn_address(con_handle);
                put_buf(&(deviece_ronn_massage.reconn_address[1]), 6);
                put_buf(&address_device[1], 6);
                log_info("Get reconn real channel");
                if (memcmp(&(deviece_ronn_massage.reconn_address[1]), &address_device[1], 6) == 0) {
                    log_info("channel :%d", 0x03 + i);
                    new_reconn_channel = 0x03 + i;
                    if (new_reconn_channel == deviece_ronn_massage.reconn_channel) {
                        log_info("New chanel == Rconn channel!");
                        break;
                    } else {
                        log_info("New chanel != Rconn channel!");
                        deviece_ronn_massage.reonn_channel_change = 1;
                        deviece_ronn_massage.reconn_map[deviece_ronn_massage.reconn_channel - 0x03] = new_reconn_channel;
                        deviece_ronn_massage.reconn_map[new_reconn_channel - 0x03] = deviece_ronn_massage.reconn_channel;
                    }
                } else {
                    log_info("ronn_address_err!");
                }
            }
        }
    }

    for (u8 i = 0; i < HID_OTA_DEVICE_NUM; i++) {
        if (ble_comm_dev_get_handle(i, GATT_ROLE_CLIENT)) {
            online_list[online_number * 2 + 1] = 0x03 + i;
            online_list[online_number * 2 + 2] = 0x01;
        } else {
            online_list[online_number * 2 + 1] = 0x03 + i;
            online_list[online_number * 2 + 2] = 0x00;
        }

        if (is_reconn_address_device == 2 && (CONFIG_BT_GATT_CLIENT_NUM > 1)) {
            log_info("on_device_printf: %d, %d \n", deviece_ronn_massage.reconn_map[i], deviece_ronn_massage.reconn_channel);
            if (!(deviece_ronn_massage.reconn_channel == new_reconn_channel)) {
                if (deviece_ronn_massage.reconn_map[i] == deviece_ronn_massage.reconn_channel) {
                    online_list[online_number * 2 + 2] = 0x00;
                } else if (deviece_ronn_massage.reconn_map[i] == new_reconn_channel) {
                    online_list[online_number * 2 + 2] = 0x01;
                }
            }
        }

        online_number++;
    }

    online_list[0] = online_number;

    if (is_reconn_address_device == 2) {
        is_reconn_address_device = 0;
    }

    dongle_send_data_to_pc(HID_RX_HANDLER_CHANNEL_COMMAND, &online_list, online_number * 2 + 1, APP_CMD_RETURN_SUCC);
}

/*************************************************************************************************/
/*!
 *  \brief      dongle检查主机下发的回连接请求是否成功
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static u8 reconn_flag = 0;
void check_is_reconn_succ(u8 state, u16 con_handle)
{
    u8 result = 0;

    if (is_reconn_address_device) {
        u8 data_result[1];

        if (state) {
            if (reconn_timer) {
                sys_timeout_del(reconn_timer);
            }
            data_result[0] = 0x00;
            extern void sm_set_master_reconn_encryption(int enable);
            extern void sm_set_master_request_pair(int enable);
            sys_timeout_add(1, sm_set_master_reconn_encryption, 1000);//关闭加密失能
            sys_timeout_add(1, sm_set_master_request_pair, 1000);//关闭加密失能
            if (reconn_flag == 1) {
                log_info("check_is_reconn_succ1 : %d", data_result[0]);
                dongle_send_data_to_pc(HID_RX_HANDLER_CHANNEL_RESPONSE, &data_result, 1, APP_CMD_RECONNECT_DEVICE);
                reconn_flag = 0;
            } else {
                log_info("miss this recon");
            }

        } else {
            for (u8 i = 0; i < CONFIG_BT_GATT_CLIENT_NUM; i++) {
                u16 con_handle = ble_comm_dev_get_handle(i, GATT_ROLE_CLIENT);
                if (con_handle) {
                    u8 *address = dg_central_get_conn_address(con_handle);
                    put_buf(&(deviece_ronn_massage.reconn_address[1]), 6);
                    put_buf(&address[1], 6);
                    /* log_info("ssss: %d", memcmp(&reconn_address[1], &address[1], 6)); */
                    if (memcmp(&(deviece_ronn_massage.reconn_address[1]), &address[1], 6) == 0) {
                        result = 1;
                        break;
                    } else {
                        result = 0;
                    }
                }
            }

            if (result == 1) {
                data_result[0] = 0x00;
                log_info("check_is_reconn_succ2 : %d", data_result[0]);
                dongle_send_data_to_pc(HID_RX_HANDLER_CHANNEL_RESPONSE, &data_result, 1, APP_CMD_RECONNECT_DEVICE);
            } else {
                ble_gatt_client_create_connection_cannel();
                data_result[0] = 0x01;
                log_info("check_is_reconn_succ : %d", data_result[0]);
                dongle_send_data_to_pc(HID_RX_HANDLER_CHANNEL_RESPONSE, &data_result, 1, APP_CMD_RECONNECT_DEVICE);
            }
        }
        is_reconn_address_device = 2;
        reconn_timer = 0;
        ble_gatt_client_scan_enable(1);//回连接结束,恢复scan

        if (state == 0) {
            //ota重连接结束,打开关闭的加密
            is_reconn_address_device = 0;
            sm_set_master_reconn_encryption(1);
            sm_set_master_request_pair(1);
        }
    } else {

    }
}

static const u8 dg_ana_remoter_name1[] = "JL_KB(BLE)";//模拟连接设备名字
static const u8 dg_ana_remoter_name2[] = "JL_MOUSE(BLE)";
static const u8 dg_ana_remoter_name3[] = "Other_device(BLE)";
static u8 usb_massage[] = {0x02, 0x01, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x11, 'U', 'S', 'B', '_', 'U', 'p', 'd', 'a', 't', 'e', '_', 'D', 'o', 'n', 'g', 'l', 'e'};
/*************************************************************************************************/
/*!
 *  \brief      dongle 连接的ota设备信息给pc(近端远端的设备信息给上位机)
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void dongle_return_online_device_massage(void)
{
    u8 online_number = 0;
    u8 online_device[HID_OTA_DEVICE_NUM * (BT_NAME_LEN_MAX + 9) + 1 + sizeof(usb_massage)];
    u8 *address_info;
    u8 dg_remoter_name[BT_NAME_LEN_MAX];
    u8 dg_remoter_name_lenght = 0;
    u16 byte_number = 0;

    for (u8 i = 0; i < HID_OTA_DEVICE_NUM; i++) {
        u16 con_handle = ble_comm_dev_get_handle(i, GATT_ROLE_CLIENT);
        if (con_handle && (is_succ_connection() != 1)) {
            //根据match_id获取设备名字
            u8 match_id = dg_central_get_match_id(con_handle);
            if (match_id < CONFIG_BT_GATT_CLIENT_NUM) {
                match_id += CONFIG_BT_GATT_CLIENT_NUM;//刚好连接上时id还没有存,做一个处理
            }
            log_info("dg_central_get_match_id : %d %d", con_handle, match_id);

            if (match_id == HID_OTA_DEVICE_NUM) {
                memcpy(&dg_remoter_name, &dg_ana_remoter_name1, sizeof(dg_ana_remoter_name1));
                dg_remoter_name_lenght = sizeof(dg_ana_remoter_name1) - 1;
            } else if (match_id == (HID_OTA_DEVICE_NUM + 1)) {
                memcpy(&dg_remoter_name, &dg_ana_remoter_name2, sizeof(dg_ana_remoter_name2));
                dg_remoter_name_lenght = sizeof(dg_ana_remoter_name2) - 1;
            } else {
                memcpy(&dg_remoter_name, &dg_ana_remoter_name3, sizeof(dg_ana_remoter_name3));
                dg_remoter_name_lenght = sizeof(dg_ana_remoter_name3) - 1;
            }
            address_info = dg_central_get_conn_address(con_handle);

            put_buf(address_info, 7);

            if (&address_info[0] != 0) {
                put_buf(&address_info[1], 6);
                log_info("record device name :%s %d\n", dg_remoter_name, dg_remoter_name_lenght);

                /* online_device[byte_number + 1] = con_handle + (HID_RX_HANDLER_CHANNEL_REMOTE1 / 16) - 0x50; */
                log_info(" %d, %d \n", i, deviece_ronn_massage.reconn_map[i]);
                online_device[byte_number + 1] = deviece_ronn_massage.reconn_map[i];
                memcpy(&(remote_device[0].conn_address[0]), &address_info[0], 7);
                /* remote_device[online_device[byte_number + 1] - HID_RX_HANDLER_CHANNEL_USB / 16].auth_flag = rcsp_hid_auth_flag_get(); */
                log_info("auth is: %d", remote_device[i + 1].auth_flag);
                online_device[byte_number + 2] = (dg_central_get_ota_is_support(con_handle)) * BIT(0) + (remote_device[i + 1].auth_flag) * BIT(1);
                memcpy(&online_device[byte_number + 3], &(address_info[1]), 6);
                online_device[byte_number + 9] = dg_remoter_name_lenght;
                memcpy(&online_device[byte_number + 10], &dg_remoter_name, dg_remoter_name_lenght);
                byte_number = 9 + dg_remoter_name_lenght + byte_number;
                /* log_info("byte_number :%d", byte_number); */
                online_number++;
            }
        } else {
            /* byte_number = 1;//在线设备数量占1byte */
        }
    }

    if (custom_hid_get_ready(0)) {
        r_printf("USB is ready!!!");
        usb_massage[1] = rcsp_hid_auth_flag_get() * BIT(1) + 1;
        memcpy(&online_device[byte_number + 1], &usb_massage, sizeof(usb_massage));
        byte_number = byte_number + 1 + sizeof(usb_massage);//加上在线设备数量1byte
        online_device[0] = online_number + 1;
    } else {
        r_printf("USB is no ready!!!");
        usb_massage[1] = rcsp_hid_auth_flag_get() * BIT(1) + 1;
        byte_number += 1;//加上在线设备数量1byte
        online_device[0] = online_number;
    }

    r_printf("device_map_len %d", byte_number);
    put_buf(&online_device, byte_number);
    dongle_send_data_to_pc(HID_RX_HANDLER_CHANNEL_RESPONSE, &online_device, byte_number, APP_CMD_GET_CONNECT_DEVICE);
}

/*************************************************************************************************/
/*!
 *  \brief      usb的中断推到app线程处理(线程处理)
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void usb_rx_event_post(u32 arg_type, u8 priv_event, u8 *args, u8 *value)
{
    struct sys_event e;
    e.type = SYS_BT_EVENT;
    e.arg  = (void *)arg_type;
    e.u.dg_ota.event = priv_event;
    /* if (args) { */
    /*     memcpy(e.u.bt.args, args, 7); */
    /* } */
    e.u.dg_ota.packet = value;
    sys_event_notify(&e);
}

/*************************************************************************************************/
/*!
 *  \brief      dongle接收pc端数据中断(接收usb的数据流)
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/

#define cbuf_get_space(a) (a)->total_len
static cbuffer_t user_send_cbuf;
static u8 usb_tmp_buffer[HID_USB_SEND_MAX * 18];
/*************************************************************************************************/
/*!
 *  \brief      read send data form send_buf
 *
 *  \param      [in]
 *
 *  \return     len
 *
 *  \note
 */
/*************************************************************************************************/
static u16 user_data_read_sub(u8 *buf, u16 buf_size)
{
    u16 ret_len;
    if (0 == cbuf_get_data_size(&user_send_cbuf)) {
        log_info("no date to get!!");
        return 0;
    }

    OS_ENTER_CRITICAL();
    cbuf_read(&user_send_cbuf, &ret_len, 2);
    if (ret_len && ret_len <= buf_size) {
        cbuf_read(&user_send_cbuf, buf, ret_len);
    }
    OS_EXIT_CRITICAL();

    return ret_len;
}

/*************************************************************************************************/
/*!
 *  \brief      write data to send buffer
 *
 *  \param      [in]
 *
 *  \return     len
 *
 *  \note
 */
/*************************************************************************************************/
static u32 user_data_write_sub(u8 *data, u16 len)
{
    u16 wlen = 0;

    u16 buf_space = cbuf_get_space(&user_send_cbuf) - cbuf_get_data_size(&user_send_cbuf);
    if (len + 2 > buf_space) {
        return 0;
    }

    OS_ENTER_CRITICAL();
    wlen = cbuf_write(&user_send_cbuf, &len, 2);
    wlen += cbuf_write(&user_send_cbuf, data, len);
    OS_EXIT_CRITICAL();

    /* user_data_try_send(); */
    return wlen;
}

static int usb_data_send_ext(u8 *data, u16 len)
{

    if (len > (HID_USB_SEND_MAX * 9)) {
        log_info("buffer limitp!!!\n");
        return 2;
    }

    putchar('@');
    usb_data_info_t data_info;
    /* data_info.report_type = report_type; */
    /* data_info.report_id = report_id; */
    memcpy(data_info.data, data, len);

    if (!user_data_write_sub((u8 *)&data_info, 2 + len)) {
        log_info("hid buffer full!!!\n");
        return 3;
    }

    return 0;
}
static u8 realtime_channel, continue_value = 0;
void dongle_custom_hid_rx_handler(void *priv, u8 *buf, u32 len)
{
    /* log_info("%s size: %d, %d\n", __func__, len, buf[2] % 16); */
    static u8 receive_continue = 0;
    static u16 buf_total_number = 0;
    static u8 buf_total_usb[HID_USB_SEND_MAX * 9];//usb最大一包530bytes,usb发打包快速存到buff再操作

    if (receive_continue) {
        /* putchar('K'); */
        /* put_buf(buf, 16); */
        memcpy(&buf_total_usb[(continue_value - receive_continue) * HID_USB_SEND_MAX], buf, HID_USB_SEND_MAX);
        /* usb_data_send_ext(buf, HID_USB_SEND_MAX); */
        receive_continue--;

        if (receive_continue) {
            return;
        } else {
            usb_data_send_ext(buf_total_usb, HID_USB_SEND_MAX * continue_value);
            goto display_data;
        }
    }

    if (HID_RX_HANDLER_HEND_TAG != (buf[0] * 256 + buf[1])) {
        putchar('x');//头标识是否正确
        return;
    }

    if (DONGLE_OTA_VERSION != (buf[2] % 16)) {
        putchar('X');//v0.0版本
        return;
    }


    if ((buf[4] * 256 + buf[5] - 1) > (HID_USB_SEND_MAX - HID_SEND_DATA_TAG_LONG)) {
        receive_continue = (buf[4] * 256 + buf[5] + HID_SEND_DATA_TAG_LONG - 1) / HID_USB_SEND_MAX;
        receive_continue = ((buf[4] * 256 + buf[5] + HID_SEND_DATA_TAG_LONG - 1) % HID_USB_SEND_MAX) ? (receive_continue + 1) : (receive_continue);
        buf_total_number = HID_SEND_DATA_TAG_LONG + buf[4] * 256 + buf[5] - 1;
        log_info("continue to receive!! %d, %d, %d", receive_continue, buf_total_number, buf[2]);
        continue_value = receive_continue;
        realtime_channel = buf[2];

        /* put_buf(buf, 64); */
        /* usb_data_send_ext(buf, HID_USB_SEND_MAX); */
        memcpy(&buf_total_usb[(continue_value - receive_continue) * HID_USB_SEND_MAX], buf, HID_USB_SEND_MAX);
        receive_continue--;
        return;
    } else {
        if (HID_RX_HANDLER_TAIL_TAG != buf[HID_SEND_DATA_TAG_LONG + buf[4] * 256 + buf[5] - 2]) {
            putchar('E');//尾标识是否正确
            return;
        }
        /* put_buf(buf, 64); */
        log_info("receive!! %d", buf[2]);
        realtime_channel = buf[2];
        usb_data_send_ext(buf, HID_USB_SEND_MAX);
        /* memcpy(buf_total, buf, HID_USB_SEND_MAX); */

        buf_total_number = len;
        receive_continue = 0;
        continue_value = 0;
    }

display_data:

    switch (realtime_channel) {
    case HID_RX_HANDLER_CHANNEL_COMMAND:
    case HID_RX_HANDLER_CHANNEL_RESPONSE:
        usb_rx_event_post(DEVICE_EVENT_FROM_PC, realtime_channel, NULL, NULL);
        break;

    // 4a 4c 30~A0 00 00 03 00 11 22 ed
    case HID_RX_HANDLER_CHANNEL_USB:
    case HID_RX_HANDLER_CHANNEL_REMOTE1:
    case HID_RX_HANDLER_CHANNEL_REMOTE2:
    case HID_RX_HANDLER_CHANNEL_REMOTE3:
    case HID_RX_HANDLER_CHANNEL_REMOTE4:
    case HID_RX_HANDLER_CHANNEL_REMOTE5:
    case HID_RX_HANDLER_CHANNEL_REMOTE6:
    case HID_RX_HANDLER_CHANNEL_REMOTE7:
    case HID_RX_HANDLER_CHANNEL_REMOTE8:
        usb_rx_event_post(DEVICE_EVENT_FROM_OTG, continue_value, NULL, NULL);
        break;

    default:
        break;

    }

}

/*************************************************************************************************/
/*!
 *  \brief      dongle接收pc端cmd数据处理(将接收到的数据按照RCSP HID协议处理)
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
int dongle_pc_event_handler(struct dg_ota_event *dg_ota)
{
    static u8 buf_total[HID_USB_SEND_MAX + 2];

    u8 i = user_data_read_sub(buf_total, HID_USB_SEND_MAX + 2);

    log_info("===============%s %d, %d", __FUNCTION__, dg_ota->event, i);
    /* put_buf(buf_total, HID_USB_SEND_MAX); */

    u32 packet_length = buf_total[4] * 256 + buf_total[5] - 1;
    /* log_info("packet_length: %d",packet_length); */

    switch (buf_total[6]) {
    // 4a 4c 00 00 00 01 01 ed 获取dongle信息
    case APP_CMD_GET_DONGLE_MASSAGE:
        log_info("APP_CMD_GET_DONGLE_MASSAGE");
        u8 data[] = {0x02, 0x2E, 0x01, 0x00, 0x4a, 0x4c, 0x55, 0x41, 0x00, 0x01, 0x00};
        dongle_send_data_to_pc(HID_RX_HANDLER_CHANNEL_RESPONSE, &data, sizeof(data), APP_CMD_GET_DONGLE_MASSAGE);
        break;

    // 4a 4c 00 00 00 01 02 ed 返回连接设备信息
    case APP_CMD_GET_CONNECT_DEVICE:
        log_info("APP_CMD_GET_CONNECT_DEVICE");
        dongle_return_online_device_massage();
        break;

    // 4a 4c 00 00 00 01 03 ed
    case APP_CMD_RETURN_SUCC:
        log_info("APP_CMD_RETURN_SUCC");
        break;

    // 4a 4c 00 00 00 09 04 03 11 22 33 44 55 66 00 ed 重连接某个地址设备
    case APP_CMD_RECONNECT_DEVICE:
        log_info("APP_CMD_RECONNECT_DEVICE:");
        u8 data_result[] = {0x01};
        put_buf(&buf_total[7], packet_length);//重连接地址信息

        u8 ble_state = ble_gatt_client_get_work_state();
        if (ble_state == BLE_ST_SCAN || ble_state == BLE_ST_IDLE || ble_state == BLE_ST_CREATE_CONN) {
            ble_gatt_client_scan_enable(0);
        }
        //!!!!!!!!!!!!!考虑是否需要添加判断此channel seq是否已连接!!!!!!!!!!!!!!!!!!!!!!
        /* if(ble_comm_dev_get_handle(buf_total[7] - (HID_RX_HANDLER_CHANNEL_REMOTE1 / 16), GATT_ROLE_CLIENT)){ */
        /*     log_info("No this handle to channel seq!!"); */
        /*     data_result[0] = 0x00; */
        /*     dongle_send_data_to_pc(HID_RX_HANDLER_CHANNEL_RESPONSE, &data_result, 1, APP_CMD_RECONNECT_DEVICE); */
        /*     break; */
        /* } */

        u8 j = 0;
        if (buf_total[7 + packet_length - 1] == 0x00) {
            //旧地址回连接方式不改变ble地址,直接回连接设备
            if (reconn_timer == 0) {
                //如果此通道连接完成直接返回成功
                for (; j < HID_OTA_DEVICE_NUM; j++) {
                    if (deviece_ronn_massage.reconn_map[j] == buf_total[7]) {
                        log_info("check_is_reconned : %d", j);
                        break;
                    }
                }
                if (ble_comm_dev_get_handle(j, GATT_ROLE_CLIENT)) {
                    log_info("check_is_reconn_succ!!");
                    data_result[0] = 0x00;
                    dongle_send_data_to_pc(HID_RX_HANDLER_CHANNEL_RESPONSE, &data_result, 1, APP_CMD_RECONNECT_DEVICE);
                    break;
                }

                if (0 == ble_gatt_client_create_connection_request(&buf_total[7 + 1], 0x00, 0)) {
                    //data_result[0] = 0x00;
                    //dongle_send_data_to_pc(HID_RX_HANDLER_CHANNEL_RESPONSE, &data_result, 1, APP_CMD_RECONNECT_DEVICE);
                    //ota重连接开始,关闭加密
                    sm_set_master_reconn_encryption(0);
                    sm_set_master_request_pair(0);
                    uuid_count_set();
                    reconn_flag = 1;//one step

                    deviece_ronn_massage.reconn_channel = buf_total[7];
                    memcpy(&(deviece_ronn_massage.reconn_address[0]), &buf_total[7], 7);
                    put_buf(&(deviece_ronn_massage.reconn_address[0]), 7);
                    is_reconn_address_device = 1;
                    reconn_timer = sys_timeout_add((0, 0), check_is_reconn_succ, 8000);//暂时设置重连时间为8s
                    log_info("reconnect start0");
                } else {
                    //操作失败
                    data_result[0] = 0x01;
                    dongle_send_data_to_pc(HID_RX_HANDLER_CHANNEL_RESPONSE, &data_result, 1, APP_CMD_RECONNECT_DEVICE);
                    log_info("reconnect fail0");
                }
            } else {
                log_info("reconnect is busy, plealse wait");
                sys_timeout_del(reconn_timer);
                reconn_flag = 1;//one step
                reconn_timer = sys_timeout_add((0, 0), check_is_reconn_succ, 8000);//暂时设置重连时间为8s
            }
        } else if (buf_total[7 + packet_length - 1] == 0x01) {
            //新地址回连接需要打开scan匹配adv再去回连接设备
            log_info("new reconn way!!!\n");
//------------------------------------------------------------------------------------//
//---------------------未加入传入新地址到ota设备验证并重连接--------------------------//
//------------------------------------------------------------------------------------//
//------------------------------------------------------------------------------------//

            data_result[0] = 0x00;
            dongle_send_data_to_pc(HID_RX_HANDLER_CHANNEL_RESPONSE, &data_result, 1, APP_CMD_RECONNECT_DEVICE);
            ble_gatt_client_scan_enable(1);
        }
        break;

    // 4a 4c 00 00 00 02 05 03 ed 断链某个设备
    case APP_CMD_DISCONNECT_DEVICE:
        /* u16 disconn_handle = trans_channel_set_to_connection_handle(buf_total[7]); */
        u16 disconn_handle = buf_total[7] + BLE_FRIST_CONNECTION_CHANNEL - (HID_RX_HANDLER_CHANNEL_REMOTE1 / 16);
        if (disconn_handle < BLE_FRIST_CONNECTION_CHANNEL) {
            log_info("APP_CMD_DISCONNECT_DEVICE %x", disconn_handle);
            u8 ret = 0;
            dongle_send_data_to_pc(HID_RX_HANDLER_CHANNEL_RESPONSE, &ret, 1, APP_CMD_DISCONNECT_DEVICE);
            break;
        }
        log_info("APP_CMD_DISCONNECT_DEVICE %x", disconn_handle);
        /* ble_gatt_client_disconnect_all(); */
        u8 ret = ble_comm_disconnect(disconn_handle);//
        dongle_send_data_to_pc(HID_RX_HANDLER_CHANNEL_RESPONSE, &ret, 1, APP_CMD_DISCONNECT_DEVICE);
        /* ble_gatt_client_scan_enable(0);//---后面考虑单设备的直接回连接 */
        break;

    // 4a 4c 00 00 00 03(2~15) 06 03 01 ed
    case APP_CMD_AUTH_FLAG:
        u8 channel_cmd;
        /* buf_total[8] = 1;//手动设置为1,为了测试 */
        if (buf_total[7] == HID_RX_HANDLER_CHANNEL_USB / 16) {
            channel_cmd = 0;
        } else {
            for (u8 j = 0; j < CONFIG_BT_GATT_CLIENT_NUM; j++) {
                if (buf_total[7] == deviece_ronn_massage.reconn_map[j]) {
                    channel_cmd = j + 1;
                }
            }
        }

        log_info("APP_CMD_AUTH_FLAG:%d %d", channel_cmd + HID_RX_HANDLER_CHANNEL_USB / 16, buf_total[8]);
        remote_device[channel_cmd].auth_flag = buf_total[8];
        u8 return_number = 0;//返回成功0
        dongle_send_data_to_pc(HID_RX_HANDLER_CHANNEL_RESPONSE, &return_number, 1, APP_CMD_AUTH_FLAG);
        break;
    // 4a 4c 00 00 00 03 ff 11 22 ed----用户自定义向dongle端发送11 22
    case APP_CMD_CUSTOM:
        log_info("APP_CMD_CUSTOM");
        put_buf(&buf_total[7], packet_length);
        dongle_send_data_to_pc(HID_RX_HANDLER_CHANNEL_RESPONSE, &buf_total[7], packet_length, APP_CMD_CUSTOM);
        break;

    default:
        break;

    }
    return 0;
}

/*************************************************************************************************/
/*!
 *  \brief      dongle接收pc端数据下发(将接收到的数据下发给近端升级设备或者远端升级设备)
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
int dongle_otg_event_handler(struct dg_ota_event *dg_ota)
{

    int send_return;
    static u8 i, handle_dev;
    static u32 packet_length;
    u8 buf_total[HID_USB_SEND_MAX * 9 + 2];
    if (dg_ota->event != 0) {
        i = user_data_read_sub(buf_total, (HID_USB_SEND_MAX * dg_ota->event) + 2);
        packet_length = buf_total[4] * 256 + buf_total[5] - 1;
    } else {
        i = user_data_read_sub(buf_total, HID_USB_SEND_MAX + 2);
        packet_length = HID_USB_SEND_MAX - 7;
    }
    if (packet_length > HID_USB_SEND_MAX) {
        log_info("%d, %d, %d, %x, %x , %x, %x, %x , %x, %x, %x", dg_ota->event, i, packet_length, buf_total[HID_USB_SEND_MAX + 4], buf_total[HID_USB_SEND_MAX + 4 + 61], buf_total[HID_USB_SEND_MAX + 4 + 61 * 2], buf_total[HID_USB_SEND_MAX + 4 + 61 * 3], buf_total[HID_USB_SEND_MAX + 4 + 61 * 4], buf_total[HID_USB_SEND_MAX + 4 + 61 * 5], buf_total[HID_USB_SEND_MAX + 4 + 61 * 6], buf_total[HID_USB_SEND_MAX + 4 + 61 * 7]);
    } else {
        log_info("%d, %d, %d", dg_ota->event, i, packet_length);
    }
    put_buf(buf_total, HID_USB_SEND_MAX);

    /* log_info("---------------%s %d, %x\n", __FUNCTION__, dg_ota->event, data_packet[2]); */
    /* printf("%d, %x\n",dg_ota->event, data_packet[2]); */
    /* put_buf(&data_packet[0], 32); */

    if (buf_total[2] == HID_RX_HANDLER_CHANNEL_USB) {
        log_info("usb send handle i :%x\n", buf_total[2]);
        rcsp_hid_recieve(NULL, &buf_total[7], packet_length);
    } else {

        for (u8 j = 0; j < CONFIG_BT_GATT_CLIENT_NUM; j++) {
            if (buf_total[2] / 16 == deviece_ronn_massage.reconn_map[j]) {
                handle_dev = ble_comm_dev_get_handle(j, GATT_ROLE_CLIENT);
            }
        }

        send_return = ble_dongle_send_data(handle_dev, &buf_total[7], packet_length);//数据直接转发给从机
        log_info("send handle i : %x, %x, %x\n", buf_total[2], handle_dev, send_return);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      ota参数初始化
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void dongle_ota_init(void)
{
    cbuf_init(&user_send_cbuf, usb_tmp_buffer, sizeof(usb_tmp_buffer));
    deviece_ronn_massage.reconn_channel = 0x03;//默认回连接channel初始化为0x03
    for (u8 i = 0; i < CONFIG_BT_GATT_CLIENT_NUM; i++) {
        deviece_ronn_massage.reconn_map[i] = 0x03 + i;//默认channel映射关系
    }
    deviece_ronn_massage.reonn_channel_change = 0;
}

/*************************************************************************************************/
/*!
 *  \brief      返回回连接通道是否改变
 *
 *  \param      [out]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
int reonn_channel_is_change(void)
{
    return deviece_ronn_massage.reonn_channel_change;
}

/*************************************************************************************************/
/*!
 *  \brief      判断是ota升级回连接还是升级完毕需要开scan
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void judge_is_reonn(u16 channel)
{
    u8 *address = dg_central_get_conn_address(channel);
    if (!is_reconn_address_device) {
        if (HID_OTA_DEVICE_NUM == 1) {
            if (0 == ble_gatt_client_create_connection_request(&address[1], address[0], 0)) {
                log_info("reconnect start0");
            } else {
                ble_gatt_client_scan_enable(1);
                log_info("pair new start0");
            }
        } else {
            log_info("pair new start0");
            ble_gatt_client_scan_enable(1);
        }
    }
}

/*************************************************************************************************/
/*!
 *  \brief      返回回连接参数
 *
 *  \param      [out]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
int get_reonn_param(void)
{
    return is_reconn_address_device;
}

/*************************************************************************************************/
/*!
 *  \brief      清除认证标志位
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void clear_auth(u16 channel)
{
    log_info("clear auth! :%d", channel);
    remote_device[channel - BLE_FRIST_CONNECTION_CHANNEL + 1].auth_flag = 0;
}

#endif



