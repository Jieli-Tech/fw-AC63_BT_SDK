#ifndef __RTC_H__
#define __RTC_H__

#include "typedef.h"
#include "asm/power/p33.h"

#define IOCTL_PORT_PR_IN        _IOR('R',  'T'+0,  0)
#define IOCTL_PORT_PR_OUT       _IOW('R',  'T'+1,  0)
#define IOCTL_PORT_PR_PU        _IOR('R',  'T'+2,  0)
#define IOCTL_PORT_PR_PD        _IOR('R',  'T'+3,  0)
#define IOCTL_PORT_PR_HD        _IOW('R',  'T'+4,  0)
#define IOCTL_PORT_PR_DIE       _IOW('R',  'T'+5,  0)
#define IOCTL_PORT_PR_READ      _IOR('R',  'T'+6,  0)

#define IOCTL_SET_TIME_256HZ_ENABLE _IOW('R',  'T'+7,  0)
#define IOCTL_SET_TIME_64HZ_ENABLE	_IOW('R',  'T'+8,  0)
#define IOCTL_SET_TIME_2HZ_ENABLE 	_IOW('R',  'T'+9,  0)
#define IOCTL_SET_TIME_1HZ_ENABLE 	_IOW('R',  'T'+10, 0)

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
    enum RTC_CLK clk_sel;	/*rtc clk 选择*/
    u8 trim_time;			/*几秒trim一次rtc时间*/
};

#define RTC_PLATFORM_DATA_BEGIN(data) \
		const struct rtc_dev_data data = {

#define RTC_PLATFORM_DATA_END() \
};


extern const struct device_operations rtc_dev_ops;

int rtc_port_pr_in(u8 port);

int rtc_port_pr_read(u8 port);

int rtc_port_pr_out(u8 port, bool on);

int rtc_port_pr_hd(u8 port, bool on);

int rtc_port_pr_pu(u8 port, bool on);

int rtc_port_pr_pd(u8 port, bool on);

int rtc_port_pr_die(u8 port, bool on);

u8 rtc_wakeup();

enum RTC_M2P_CMD {
    RTC_M2P_ALMEN = 1,
    RTC_M2P_ALMUEN,
    RTC_M2P_SYS_TIME,
    RTC_M2P_ALM_TIME,
};

struct rtc_m2p_cmd {
    enum RTC_M2P_CMD cmd;
    u8 dat0;
    u8 dat1;
    u8 dat2;
    u8 dat3;
    u8 dat4;
};

#endif // __RTC_API_H__
