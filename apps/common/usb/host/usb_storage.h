#ifndef __USB_STORAGE_H__
#define __USB_STORAGE_H__

#include "system/task.h"
#include "device/device.h"
#include "usb/scsi.h"
#include "usb_bulk_transfer.h"
#include "usb/host/usb_host.h"

/* u盘预读功能配置, 二选一
 * 当两种方式都不使能，则表示不开启预读 */
#define  UDISK_READ_BIGBLOCK_ASYNC_ENABLE    0   //使能大扇区预读方式(不需要额外buf,速度比512预读慢10%)
#define  UDISK_READ_512_ASYNC_ENABLE         1   //使能512Byte预读方式(需要额外的512byte buffer,速度比大扇区预读快10%)
/****************************/

#define UDISK_READ_ASYNC_BLOCK_NUM  (16) //预读扇区数

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

#define ENABLE_DISK_HOTPLUG  0
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
#if (UDISK_READ_BIGBLOCK_ASYNC_ENABLE || UDISK_READ_512_ASYNC_ENABLE)
    u8 async_en;
    u8 need_send_csw;
    u8 *udisk_512_buf;
    u32 async_prev_lba;
#endif
#if ENABLE_DISK_HOTPLUG
    u8 media_sta_cur;  //for card reader, card removable
    u8 media_sta_prev;

    int test_unit_ready_tick;
#endif
};

enum usb_async_mode {
    BULK_ASYNC_MODE_EXIT = 0, //取消异步模式
    BULK_ASYNC_MODE_ENTER, //异步512预读
    BULK_ASYNC_MODE_SEM_PEND, //异步预读等待信号量
};

#define MASS_LBA_INIT    (-2)

int usb_msd_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf);
int _usb_stor_async_wait_sem(struct usb_host_device *host_dev);

#endif
