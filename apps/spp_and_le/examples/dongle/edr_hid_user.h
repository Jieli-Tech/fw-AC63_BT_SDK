#ifndef __EDR_HID_USER_H__
#define __EDR_HID_USER_H__

#include "typedef.h"
#include "bt_common.h"

enum {
    HID_USER_ERR_NONE = 0x0,
    HID_USER_ERR_DONE,
    HID_USER_ERR_SEND_FAIL,
};

void user_hid_init(void (*user_hid_output_handler)(u8 *packet, u16 size, u16 channel));
void user_hid_exit(void);
void user_hid_enable(u8 en);
int  user_hid_send_data(u8 *buf, u32 len);
void user_hid_disconnect(void);
void user_hid_set_icon(u32 class_type);
void user_hid_set_ReportMap(u8 *map, u16 size);
int edr_hid_data_send(u8 report_id, u8 *data, u16 len);
int edr_hid_data_send_ext(u8 report_type, u8 report_id, u8 *data, u16 len);
void edr_hid_key_deal_test(u16 key_msg);
int edr_hid_is_connected(void);
int edr_hid_tx_buff_is_ok(void);

#endif//__SPP_USER_H__
