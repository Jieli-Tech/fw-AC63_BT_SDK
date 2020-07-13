// binary representation
// attribute size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)
#ifndef _LE_CLIENT_DEMO_H
#define _LE_CLIENT_DEMO_H

#include <stdint.h>

enum {
    CLI_CREAT_BY_ADDRESS = 0,//指定地址创建连接
    CLI_CREAT_BY_NAME,//指定设备名称创建连接
    CLI_CREAT_BY_TAG,//匹配厂家标识创建连接
};

typedef struct {
    u16 services_uuid16;
    u16 characteristic_uuid16;
    u8  services_uuid128[16];
    u8  characteristic_uuid128[16];
    u16 opt_type;
} target_uuid_t;

//搜索操作记录的 handle
#define OPT_HANDLE_MAX   16
typedef struct {
    target_uuid_t *search_uuid;
    u16 value_handle;
} opt_handle_t;

typedef struct {
    u8 create_conn_mode;	
	u8 compare_data_len;
	const u8 *compare_data;//若是地址内容,由高到低位
	u8 search_uuid_cnt; // <= OPT_HANDLE_MAX
    const target_uuid_t *search_uuid_table;
	void (*report_data_callback)(att_data_report_t * data_report,target_uuid_t *search_uuid);	
} client_conn_cfg_t;


void ble_get_client_operation_table(struct ble_client_operation_t **interface_pt);

#endif
