#ifndef  __USB_HOST_H__
#define  __USB_HOST_H__
#include "system/task.h"
#include "device/device.h"
#include "asm/usb.h"
#include "usb/ch9.h"
#include "usb/usb_phy.h"
// #include "usb_config.h"


struct usb_private_data {
    usb_dev usb_id;
    u8 status;
    u8 devnum;
    u8 ep0_max_packet_size;
    /*
    u8 speed;
    u16 vendor_id;
    u16 product_id;
    u16 language;
    u8 manufacturer[64];
    u8 product[64];
    */
};

struct usb_host_device;
struct interface_ctrl {
    u8 interface_class;
    int (*set_power)(struct usb_host_device *host_dev, u32 value);
    int (*get_power)(struct usb_host_device *host_dev, u32 value);
    int (*ioctl)(struct usb_host_device *host_dev, u32 cmd, u32 arg);
};
struct usb_interface_info {
    struct interface_ctrl *ctrl;
    union {
        struct mass_storage *disk;
        struct adb_device_t *adb;
        struct hid_device_t *hid;
        struct aoa_device_t *aoa;
        struct audio_device_t *audio;
        void *p;
    } dev;
};
#define     MAX_HOST_INTERFACE  4
struct usb_host_device {
#if USB_HUB
    struct usb_host_device *father;
#endif
    OS_SEM *sem;
    struct usb_private_data private_data;
    const struct usb_interface_info *interface_info[MAX_HOST_INTERFACE];
};


#define     device_to_usbdev(device)	((struct usb_host_device *)((device)->private_data))

u32 host_device2id(const struct usb_host_device *host_dev);

int host_dev_status(const struct usb_host_device *host_dev);

const struct usb_host_device *host_id2device(const usb_dev id);

#define     check_usb_mount(ret)    \
    if(ret == -DEV_ERR_OFFLINE){\
        log_error("%s() @ %d DEV_ERR_OFFLINE\n", __func__, __LINE__);\
        goto __exit_fail;\
    } else if(ret){\
        log_error("%s() @ %d %x\n", __func__, __LINE__, ret);\
        continue;\
    }


typedef void(*usb_h_interrupt)(struct usb_host_device *, u32 ep);

int usb_sem_init(struct usb_host_device *host_dev);
int usb_sem_pend(struct usb_host_device *host_dev, u32 timeout);
int usb_sem_post(struct usb_host_device *host_dev);
int usb_sem_del(struct usb_host_device *host_dev);

void usb_h_set_ep_isr(struct usb_host_device *host_dev, u32 ep, usb_h_interrupt hander, void *p);
u32 usb_h_set_intr_hander(const usb_dev usb_id, u32 ep, usb_h_interrupt hander);
u32 usb_host_mount(const usb_dev usb_id, u32 retry, u32 reset_delay, u32 mount_timeout);
u32 usb_host_unmount(const usb_dev usb_id);
u32 usb_host_remount(const usb_dev usb_id, u32 retry, u32 delay, u32 ot, u8 notify);
void usb_host_suspend(const usb_dev usb_id);
void usb_host_resume(const usb_dev usb_id);

#endif  /*USB_HOST_H*/
