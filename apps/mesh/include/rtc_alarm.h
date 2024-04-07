#ifndef __RTC_ALARM_H__
#define __RTC_ALARM_H__

#include "typedef.h"
#include "system/includes.h"

/* macro */
#define M_MAX_ALARM_NUMS          5
#define M_MAX_ALARM_INDEX        (M_MAX_ALARM_NUMS-1)
#define M_MAX_ALARM_MODE         (0xFE)

enum {
    E_ALARM_MODE_ONCE            = 0x00,
    E_ALARM_MODE_EVERY_DAY       = 0x01,
    E_ALARM_MODE_EVERY_MONDAY    = 0x02,
    E_ALARM_MODE_EVERY_TUESDAY   = 0x04,
    E_ALARM_MODE_EVERY_WEDNESDAY = 0x08,
    E_ALARM_MODE_EVERY_THURSDAY  = 0x10,
    E_ALARM_MODE_EVERY_FRIDAY    = 0x20,
    E_ALARM_MODE_EVERY_SATURDAY  = 0x40,
    E_ALARM_MODE_EVERY_SUNDAY    = 0x80,
};

typedef struct __ALARM__ {
    u8 en;                  //是否使能, 0:关闭， 1:打开
    u8 mode;                //闹钟模式，0:只作用一次。 BIT(0)：每天都作用。 BIT(1)~BIT(7)：周1~7，哪一位被置1则表示周几起作用。
    struct sys_time time;   //该闹钟时间
} T_ALARM;

typedef struct __ALARM_VM__ {
    u16 head;
    T_ALARM alarm;
} T_ALARM_VM;

typedef struct __ALARM_VM_MASK__ {
    u16 head;
    u8  alarm_active_index;
    u8  alarm_en_num;
} T_ALARM_VM_MASK;

void alarm_init(const struct rtc_dev_platform_data *arg);

void alarm_rtc_stop(void);
void alarm_rtc_start(void);
u8 alarm_get_active_index(void);
u8 alarm_get_en_info(void);
void alarm_get_time_info(T_ALARM *p, u8 index);
u8 alarm_add(T_ALARM *p, u8 index);
u8 alarm_en(u8 index, u8 en);

#endif  //end of __ALARM_H__


