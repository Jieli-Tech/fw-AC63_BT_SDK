/*********************************************************************************************
    *   Filename        : hci_ll.h

    *   Description     : 提供Vendor Host 直接调用Controller API LL Part

    *   Author          : Bingquan

    *   Email           : bingquan_cai@zh-jieli.com

    *   Last modifiled  : 2018-12-04 11:58

    *   Copyright:(c)JIELI  2011-2017  @ , All Rights Reserved.
*********************************************************************************************/
#ifndef _HCI_LL_H_
#define _HCI_LL_H_

#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

#include "typedef.h"

enum {
    LL_EVENT_SUPERVISION_TIMEOUT,
    LL_EVENT_RX,
    LL_EVENT_ACL_TX_POST,
};

typedef struct {
    u8 Own_Address_Type: 2; //0~3
    u8 Adv_Filter_Policy: 2;
    u8 Scan_Filter_Policy: 2;
    u8 initiator_filter_policy: 2;
} hci_ll_param_t;

/*! \brief      LE Set Extended Advertising Parameters. */
typedef struct {
    uint8_t         Advertising_Handle;
    uint16_t        Advertising_Event_Properties;
    uint8_t         Primary_Advertising_Interval_Min[3];
    uint8_t         Primary_Advertising_Interval_Max[3];
    uint8_t         Primary_Advertising_Channel_Map;
    uint8_t         Own_Address_Type;
    uint8_t         Peer_Address_Type;
    uint8_t         Peer_Address[6];
    uint8_t         Advertising_Filter_Policy;
    uint8_t         Advertising_Tx_Power;
    uint8_t         Primary_Advertising_PHY;
    uint8_t         Secondary_Advertising_Max_Skip;
    uint8_t         Secondary_Advertising_PHY;
    uint8_t         Advertising_SID;
    uint8_t         Scan_Request_Notification_Enable;
} _GNU_PACKED_ le_set_ext_adv_param_t;

/*! \brief      LE Set Extended Advertising Data. */
typedef struct {
    uint8_t         Advertising_Handle;
    uint8_t         Operation;
    uint8_t         Fragment_Preference;
    uint8_t         Advertising_Data_Length;
    uint8_t         Advertising_Data[31];
} _GNU_PACKED_ le_set_ext_adv_data_t;
/*! \brief      LE Set Extended Advertising Enable. */
typedef struct {
    uint8_t         Enable;
    uint8_t         Number_of_Sets;
    uint8_t         Advertising_Handle;
    uint16_t        Duration;
    uint8_t         Max_Extended_Advertising_Events;
} _GNU_PACKED_ le_set_ext_adv_en_t;

/*! \brief      LE Extended Advertising report event. */
typedef union {
    struct {
        uint16_t Connectable_advertising    : 1,
                 Scannable_advertising      : 1,
                 Directed_advertising       : 1,
                 Scan_response              : 1,
                 Legacy_adv_PDUs_used       : 1,
                 Data_status                : 2,
                 All_other_bits             : 9;
    };

    uint16_t event_type;
} _GNU_PACKED_ le_evt_type_t;

typedef struct {
    uint8_t         Subevent_Code;
    uint8_t         Num_Reports;
    le_evt_type_t   Event_Type;
    uint8_t         Address_Type;
    uint8_t         Address[6];
    uint8_t         Primary_PHY;
    uint8_t         Secondary_PHY;
    uint8_t         Advertising_SID;
    uint8_t         Tx_Power;
    uint8_t         RSSI;
    uint16_t        Periodic_Advertising_Interval;
    uint8_t         Direct_Address_Type;
    uint8_t         Direct_Address[6];
    uint8_t         Data_Length;
    uint8_t         Data[0];
} _GNU_PACKED_ le_ext_adv_report_evt_t;

typedef struct {
    uint8_t         Own_Address_Type;
    uint8_t         Scanning_Filter_Policy;
    uint8_t         Scanning_PHYs;
    uint8_t         Scan_Type;
    uint16_t        Scan_Interval;
    uint16_t        Scan_Window;
} _GNU_PACKED_ le_ext_scan_param_lite_t;


//Adjust Host part API
void ll_hci_init(void);

void ll_hci_reset(void);

void ll_hci_destory(void);

void ll_hci_set_event_mask(const u8 *mask);

void ll_hci_set_name(const char *name);

void ll_hci_adv_set_params(uint16_t adv_int_min, uint16_t adv_int_max, uint8_t adv_type,
                           uint8_t direct_address_type, uint8_t *direct_address,
                           uint8_t channel_map, uint8_t filter_policy);

void ll_hci_adv_set_data(uint8_t advertising_data_length, uint8_t *advertising_data);

void ll_hci_adv_scan_response_set_data(uint8_t scan_response_data_length, uint8_t *scan_response_data);

int ll_hci_adv_enable(bool enable);

void ll_hci_scan_set_params(uint8_t scan_type, uint16_t scan_interval, uint16_t scan_window);

int ll_hci_scan_enable(bool enable, u8 filter_duplicates);

int ll_hci_create_conn(u8 *conn_param, u8 *addr_param);
int ll_hci_create_conn_ext(void *param);

int ll_hci_create_conn_cancel(void);

int ll_hci_vendor_send_key_num(u16 con_handle, u8 num);

int ll_hci_encryption(u8 *key, u8 *plaintext_data);

int ll_hci_get_le_rand(void);


int ll_hci_start_encryption(u16 handle, u32 rand_low, u32 rand_high, u16 peer_ediv, u8 *ltk);

int ll_hci_long_term_key_request_reply(u16 handle, u8 *ltk);

int ll_hci_long_term_key_request_nagative_reply(u16 handle);

int ll_hci_connection_update(u16 handle, u16 conn_interval_min, u16 conn_interval_max,
                             u16 conn_latency, u16 supervision_timeout,
                             u16 minimum_ce_length, u16 maximum_ce_length);

u16 ll_hci_get_acl_data_len(void);

u16 ll_hci_get_acl_total_num(void);

void ll_hci_set_random_address(u8 *addr);

int ll_hci_disconnect(u16 handle, u8 reason);

int ll_hci_read_local_p256_pb_key(void);

int ll_hci_generate_dhkey(const u8 *data, u32 size);

//Adjust Controller part API
void ll_hci_cmd_handler(int *cmd);

void ll_event_handler(int *msg);

void ll_hci_private_free_dma_rx(u8 *rx_head);

void ll_hci_set_data_length(u16 conn_handle, u16 tx_octets, u16 tx_time);

hci_ll_param_t *ll_hci_param_config_get(void);
void hci_ll_get_device_address(uint8_t *addr_type, u8 *addr);
void hci_ll_get_link_local_address(uint16_t conn_handle, uint8_t *addr_type, u8 *addr);

/*
   定制主机的跳频表,使用时需要把系统自带的AFH关掉
 channel_map[5]:bit0~bit36 对应频点 0~36;未用bits默认填0
 */
void ll_hci_set_host_channel_classification(u8 *channel_map);

// ble5
void ll_hci_set_ext_adv_params(u8 *data, u32 size);
void ll_hci_set_ext_adv_data(u8 *data, u32 size);
void ll_hci_set_ext_adv_enable(u8 *data, u32 size);
void ll_hci_set_phy(u16 conn_handle, u8 all_phys, u8 tx_phy, u8 rx_phy, u16 phy_options);
void ll_hci_set_ext_scan_params(u8 *data, u32 size);
void ll_hci_set_ext_scan_enable(u8 *data, u32 size);
void ll_hci_ext_create_conn(u8 *data, u32 size);
void ll_hci_set_periodic_adv_params(u8 *data, u32 size);
void ll_hci_set_periodic_adv_data(u8 *data, u32 size);
void ll_hci_set_periodic_adv_enable(u8 *data, u32 size);
void ll_hci_periodic_adv_creat_sync(u8 *data, u32 size);
void ll_hci_periodic_adv_terminate_sync(u8 *data, u32 size);
void ll_hci_periodic_adv_create_sync_cancel(void);

int le_controller_set_mac(void *addr);

/*vendor function*/
bool ll_vendor_check_update_procedure_is_exist(u16 conn_handle);

//set miss latency's count
int ll_vendor_latency_hold_cnt(u16 conn_handle, u16 hold_cnt);

//0-NONE, 1-S2, 2-S8
void ll_vendor_set_code_type(u8 code_type);

/*获取按规则生成的access address,*/
void ll_vendor_access_addr_generate(u8 *out_address);

/*改变本地链路超时断开,用于切换链路时,快速断开链路*/
/*处理以下情况超时断开链路: 对方没有ack terminate 命令 ,或者非md状态下,多包ack */
bool ll_vendor_change_local_supervision_timeout(u16 conn_handle, u32 timeout);

/*ll get connect address info;
input:  role:0-local,1-pree; conn_handle, conntion handle
output: addr_info: address_type + address (7bytes)
*/
bool ll_vendor_get_link_address_info(u16 conn_handle, u8 role, u8 *addr_info);

#endif
