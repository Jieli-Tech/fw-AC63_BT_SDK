#ifndef __RTC_H__
#define __RTC_H__

#include "typedef.h"

#define IOCTL_PORT_PR_IN        _IOR('R',  'T'+0,  0)
#define IOCTL_PORT_PR_OUT       _IOW('R',  'T'+1,  0)
#define IOCTL_PORT_PR_PU        _IOR('R',  'T'+2,  0)
#define IOCTL_PORT_PR_PD        _IOR('R',  'T'+3,  0)
#define IOCTL_PORT_PR_HD        _IOW('R',  'T'+4,  0)
#define IOCTL_PORT_PR_DIE       _IOW('R',  'T'+5,  0)
#define IOCTL_PORT_PR_READ      _IOR('R',  'T'+6,  0)

#define WKUP_IO_PR0     0x01
#define WKUP_IO_PR1     0x02
#define WKUP_IO_PR2     0x04
#define WKUP_IO_PR3     0x08
#define WKUP_ALARM      0x10
#define BAT_POWER_FIRST 0x20
#define ABNORMAL_RESET  0x40
#define WKUP_SHORT_KEY	0x80


#define LEADING_EDGE  0
#define FALLING_EDGE  1


struct rtc_dev_data {
    u8 port;
    u8 edge;  //0 leading edge, 1 falling edge
    u8 port_en;
    u8 rtc_ldo;
    u8 clk_res;
};


extern const struct device_operations rtc_dev_ops;
extern const struct device_operations rtc_simulate_ops ;

int rtc_port_pr_in(u8 port);

int rtc_port_pr_read(u8 port);

int rtc_port_pr_out(u8 port, bool on);

int rtc_port_pr_hd(u8 port, bool on);

int rtc_port_pr_pu(u8 port, bool on);

int rtc_port_pr_pd(u8 port, bool on);

int rtc_port_pr_die(u8 port, bool on);


#endif // __RTC_API_H__
