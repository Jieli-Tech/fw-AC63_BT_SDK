#ifndef __HID_USER_H__
#define __HID_USER_H__

#include "typedef.h"
#include "bt_common.h"

enum {
    HID_USER_ERR_NONE = 0x0,
    HID_USER_ERR_DONE,
    HID_USER_ERR_SEND_FAIL,
};

void user_hid_init(void (*user_hid_interrupt_handler)(u8 *packet, u16 size));
void user_hid_exit(void);
void user_hid_enable(u8 en);
int  user_hid_send_data(u8 *buf, u32 len);
void user_hid_disconnect(void);
void user_hid_set_icon(u32 class_type);
void user_hid_set_ReportMap(u8 *map, u16 size);

#endif//__SPP_USER_H__
