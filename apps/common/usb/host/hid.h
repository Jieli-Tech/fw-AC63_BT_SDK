#ifndef  __HID_H__
#define  __HID_H__

#include "system/task.h"
#include "device/device.h"
#include "usb/scsi.h"
#include "usb_bulk_transfer.h"
#include "usb/host/usb_host.h"

struct report_info_t {
    u8 report_id;
    u8 usage;

    u8 btn_start_bit;
    u8 btn_width;

    u8 xy_start_bit;
    u8 xy_width;

    u8 wheel_start_bit;
    u8 wheel_width;
};

#define     MAX_REPORT_COUNT    4
struct hid_device_t {
    void *parent;
    u8 ep_pair[4];
    u8 report_count;
    u8 bNumEndpoints;
    struct report_info_t report_list[MAX_REPORT_COUNT];
};

int usb_hid_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf);
void hid_process(u32 id);

#endif  /*HID_H*/
