#ifndef  __GAMEBOX_H__
#define  __GAMEBOX_H__

#include "usb/device/usb_stack.h"
#include "usb/otg.h"
#include "usb_hid_keys.h"
#include "usb/host/usb_host.h"
#include "adb.h"
#include "aoa.h"
#include "hid.h"
enum {
    UT_DEBUG_MODE,
    UART_MODE,
    BT_MODE,
    USB_MODE,
    MAPPING_MODE,
    OTA_MODE,
};

#define     TOUCH_SCREEN_ID 1
#define     MOUSE_POINT_ID  2

#define     HID_REPORT_SIZE (167+51)

extern const char hid_report_desc[HID_REPORT_SIZE];
#define MOUSE_POINT_MODE     0x32
#define KEYBOARD_MODE        0x37
#define TOUCH_SCREEN_MODE    0x38

u32 get_run_mode();
void set_run_mode(u32 mode);
u32 get_phone_connect_status();

void set_phone_connect_status(u32 status);

void send2phone(u32 type, const void *_p);


void mouse_mapping(const struct mouse_data_t *m);
void key_mapping(const struct keyboard_data_t *k);

void key_list_init();
void point_list_empty();
u32 point_list_pop(struct touch_screen_t *t);

extern u8 mouse_data_send ;
extern u8 touch_data_send;
extern struct mouse_point_t mouse_data;
#endif  /*GAMEBOX_H*/
