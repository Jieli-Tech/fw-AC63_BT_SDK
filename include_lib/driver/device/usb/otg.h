#ifndef  __OTG_H__
#define  __OTG_H__

#include "asm/usb.h"

enum {
    IDLE_MODE = 0,
    DISCONN_MODE = 1,
    HOST_MODE = 2,
    PRE_SLAVE_MODE,
    SLAVE_MODE_WAIT_CONFIRMATION,
    SLAVE_MODE,
    CHARGE_MODE,
};

enum {
    OTG_OP_NULL = 0,
    OTG_UNINSTALL = 1,
    OTG_KEEP_STATE,
};

#define     OTG_HOST_MODE      BIT(0)
#define     OTG_SLAVE_MODE     BIT(1)
#define     OTG_CHARGE_MODE    BIT(2)
#define     OTG_DET_DP_ONLY    BIT(3)

struct otg_dev_data {
    u8 usb_dev_en;
    u8 slave_online_cnt;
    u8 slave_offline_cnt;
    u8 host_online_cnt;
    u8 host_offline_cnt;
    u8 detect_mode;
    u8 detect_time_interval;
};


u32 usb_otg_online(const usb_dev usb_id);
// u32 usb_otg_init(u32 mode);
void usb_otg_io_suspend(usb_dev usb_id);
void usb_otg_io_resume(usb_dev usb_id);
void usb_otg_suspend(usb_dev usb_id, u8 op_mode);
void usb_otg_resume(usb_dev usb_id);

extern const struct device_operations usb_dev_ops;


#endif  /*OTG_H*/
