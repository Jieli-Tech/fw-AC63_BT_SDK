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

struct rtc_dev_platform_data {
    const struct sys_time *default_sys_time;
    const struct sys_time *default_alarm;
    void (*cbfun)(u8);
    u8 x32xs;
    u8 port;
    u8 edge;  //0 leading edge, 1 falling edge
    u8 port_en;
    u8 rtc_ldo;
    u8 clk_sel;
};

#define RTC_DEV_PLATFORM_DATA_BEGIN(data) \
	const struct rtc_dev_platform_data data = {

#define RTC_DEV_PLATFORM_DATA_END()  \
    .x32xs = 0, \
};

extern const struct device_operations rtc_dev_ops;
extern const struct device_operations rtc_simulate_ops;

//PR口操作接口
int rtc_port_pr_in(u8 port);
int rtc_port_pr_read(u8 port);
int rtc_port_pr_out(u8 port, bool on);
int rtc_port_pr_hd(u8 port, bool on);
int rtc_port_pr_pu(u8 port, bool on);
int rtc_port_pr_pd(u8 port, bool on);
int rtc_port_pr_die(u8 port, bool on);
int rtc_port_pr_wkup_en_port(u8 port, bool en);
int rtc_port_pr_wkup_edge(u8 port, bool up_down);
int rtc_port_pr_wkup_clear_pnd(u8 port);
int __rtc_port_pr_wkup_clear_pnd();

//RTC操作接口
int rtc_init(const struct rtc_dev_platform_data *arg);
int rtc_ioctl(u32 cmd, u32 arg);
void set_alarm_ctrl(u8 set_alarm);
void write_sys_time(struct sys_time *curr_time);
void read_sys_time(struct sys_time *curr_time);
void write_alarm(struct sys_time *alarm_time);
void read_alarm(struct sys_time *alarm_time);

//时间换算接口
u16 month_to_day(u16 year, u8 month);
u16 __month_to_day(u16 year, u8 month);
void day_to_ymd(u16 day, struct sys_time *sys_time);
u16 ymd_to_day(struct sys_time *time);
u8 caculate_weekday_by_time(struct sys_time *r_time);
#endif // __RTC_API_H__
