#ifndef __USB_STORAGE_H__
#define __USB_STORAGE_H__

#include "system/task.h"
#include "device/device.h"
#include "usb/scsi.h"
#include "usb_bulk_transfer.h"
#include "usb/host/usb_host.h"

#define  UDISK_READ_ASYNC_ENABLE    1   //使能u盘预读功能
#if UDISK_READ_ASYNC_ENABLE
#define UDISK_READ_ASYNC_BLOCK_NUM  (16) //预读扇区数
#endif

//设备状态：
typedef enum usb_sta {
    DEV_IDLE = 0,
    DEV_INIT,
    DEV_OPEN,
    DEV_READ,
    DEV_WRITE,
    DEV_CLOSE,
    DEV_SUSPEND,
} USB_STA ;

struct udisk_end_desc {
    u8 host_epout;
    u8 target_epout;
    u8 host_epin;
    u8 target_epin;
#if HUSB_MODE
    u16 rxmaxp;
    u16 txmaxp;
#endif
};

struct mass_storage {
    OS_MUTEX mutex;

    struct usb_scsi_cbw cbw;
    struct usb_scsi_csw csw;
    struct request_sense_data sense;

    char *name;
    struct read_capacity_data capacity[2];
    u8 lun;
    u8 curlun;

    u8 dev_status;
    u8 suspend_cnt;
    u8 read_only;

    u32 remain_len;
    u32 prev_lba;
#if UDISK_READ_ASYNC_ENABLE
    u32 async_prev_lba;
#endif
#if ENABLE_DISK_HOTPLUG
    u8 media_sta_cur;  //for card reader, card removable
    u8 media_sta_prev;

    int test_unit_ready_tick;
#endif
};

#define MASS_LBA_INIT    (-2)

int usb_msd_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf);

#endif
