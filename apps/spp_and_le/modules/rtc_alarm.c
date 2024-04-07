#include "rtc_alarm.h"
#include "system/timer.h"
#include "app_config.h"
#include "asm/rtc.h"

#if TCFG_RTC_ALARM_ENABLE

#define ALARM_DEBUG_EN   0

#if ALARM_DEBUG_EN
#define alarm_printf(x, ...)  printf("[RTC_ALARM]" x " ", ## __VA_ARGS__)
#define alarm_printf_buf      put_buf
#define alarm_putchar         putchar
#else
#define alarm_printf(...)
#define alarm_printf_buf(...)
#define alarm_putchar(...)
#endif

#define RTC_MASK (0xaa55)
static T_ALARM alarm_tab[M_MAX_ALARM_NUMS];
static T_ALARM_VM_MASK alarm_mask;
static u8 alarm_pnd_flag = 0;
static void (*_user_isr_cbfun)(u8) = NULL;

void alarm_send_event(u8 index);
#if TCFG_RTC_ALARM_ENABLE
struct sys_time rtc_read_test;
struct sys_time alm_read_test;
struct sys_time alm_write_test = {
    .year = 2024,
    .month  =  1,
    .day = 1,
    .hour = 0,
    .min = 0,
    .sec = 20,
};
#endif


static u8 alarm_vm_write_mask(u8 alarm_active_index)
{
    alarm_mask.head = RTC_MASK;
    alarm_mask.alarm_active_index = alarm_active_index;
    int ret = syscfg_write(VM_ALARM_MASK, (u8 *)&alarm_mask, sizeof(T_ALARM_VM_MASK));
    if (ret <= 0) {
        alarm_printf("alarm mask write vm err!\n");
        return 1;
    }
    return 0;
}

static u8 alarm_vm_read_mask(void)
{
    memset((u8 *)&alarm_mask, 0, sizeof(T_ALARM_VM_MASK));
    int ret = syscfg_read(VM_ALARM_MASK, (u8 *)&alarm_mask, sizeof(T_ALARM_VM_MASK));
    if ((ret <= 0) || (alarm_mask.head != RTC_MASK)) {       //第一次读的时候
        alarm_printf("alarm mask read vm err!\n");
        return 1;
    }
    return 0;
}

static u8 alarm_vm_write_time_tab(T_ALARM_VM *pAlarm_tab, u8 index)
{
    int ret;
    T_ALARM_VM tmp = {0};
    tmp.head = RTC_MASK;
    memcpy((u8 *)&tmp.alarm, (u8 *)pAlarm_tab, sizeof(T_ALARM));
    ret = syscfg_write(VM_ALARM_0 + index, (u8 *)&tmp, sizeof(T_ALARM_VM));
    if (ret <= 0) {
        alarm_printf("alarm write vm err!\n");
        return 1;
    }
    return 0;
}

static u8 alarm_vm_read_time_tab(T_ALARM_VM *pAlarm_tab, u8 index)
{
    T_ALARM_VM tmp = {0};
    int ret = syscfg_read(VM_ALARM_0 + index, (u8 *)&tmp, sizeof(T_ALARM_VM));
    if ((ret <= 0) || (tmp.head != RTC_MASK)) {
        alarm_printf("alarm read vm err!\n");
        return 1;
    }
    memcpy((u8 *)pAlarm_tab, (u8 *)&tmp.alarm, sizeof(T_ALARM));
    return 0;
}

static void alarm_calculate_next_few_time(struct sys_time *time, u16 days)
{
    if (!days) {
        return;
    }
    u16 tmp_day = ymd_to_day(time);
    tmp_day += days;
    day_to_ymd(tmp_day, time);
}

static void alarm_calculate_time_by_week_mode(struct sys_time *pTime, u8 mode)
{
    if (0 == mode) {
        return;
    }
    if ((BIT(0)) == mode) {
        return;
    }
    u16 tmp_mode = ((mode & 0xfe) << 7) | mode ;
    u8 alarm_week = caculate_weekday_by_time(pTime);  //获取闹钟是周几，1~7
    u8 i;
    for (i = 1; i < 15; i++) {
        if (tmp_mode & BIT(i)) {
            if (i >= alarm_week) {
                break;
            }
        }
    }
    alarm_calculate_next_few_time(pTime, i - alarm_week);
}
static u8 alarm_update_a_time_tab(struct sys_time *cTime, T_ALARM *pAlarm_tab, u32 *diff)
{
    struct sys_time *pTime = &(pAlarm_tab->time);
    u32 c_tmp = (ymd_to_day(cTime) << 17) | ((cTime->hour & 0x1f) << 12) | ((cTime->min & 0x3f) << 6) | (cTime->sec & 0x3f);
    u32 p_tmp = (ymd_to_day(pTime) << 17) | ((pTime->hour & 0x1f) << 12) | ((pTime->min & 0x3f) << 6) | (pTime->sec & 0x3f);
    if (c_tmp >= p_tmp) {
        pTime->year = cTime->year;
        pTime->month = cTime->month;
        pTime->day  = cTime->day;
        alarm_calculate_next_few_time(pTime, 1);
        alarm_calculate_time_by_week_mode(pTime, pAlarm_tab->mode);
        p_tmp = (ymd_to_day(pTime) << 17) | ((pTime->hour & 0x1f) << 12) | ((pTime->min & 0x3f) << 6) | (pTime->sec & 0x3f);
        *diff = p_tmp - c_tmp;
        return 1;
    } else if (pAlarm_tab->mode & 0xfe) {
        alarm_calculate_time_by_week_mode(pTime, pAlarm_tab->mode);
        p_tmp = (ymd_to_day(pTime) << 17) | ((pTime->hour & 0x1f) << 12) | ((pTime->min & 0x3f) << 6) | (pTime->sec & 0x3f);
        *diff = p_tmp - c_tmp;
        return 1;
    }
    *diff = p_tmp - c_tmp;
    return 0;
}
static u8 alarm_update_all_time(void)
{
    u8 err = 0;
    u32 diff = 0;
    u32 diff_min = -1;
    u8 closest = -1;

    struct sys_time current_time = {0};
    rtc_ioctl(IOCTL_GET_SYS_TIME, (u32)&current_time);
    for (u8 i = 0; i < M_MAX_ALARM_NUMS; i ++) {
        if (alarm_tab[i].en) {
            err = alarm_update_a_time_tab(&current_time, &alarm_tab[i], (u32 *)&diff);
            if (err) {  //时间有更新
                err = alarm_vm_write_time_tab(&alarm_tab[i], i);
                if (err) {
                    return 1;
                }
            }
            if (diff < diff_min) {
                diff_min = diff;
                closest = i;
            }
        }
    }
    if (closest > M_MAX_ALARM_NUMS) {
        set_alarm_ctrl(0);
    }

    if ((alarm_tab[closest].en)) {   //最接近闹钟号跟记录的不一样，则要重设硬件寄存器，更新记录
        rtc_ioctl(IOCTL_SET_ALARM, (u32)&alarm_tab[closest].time);
        if (closest != alarm_mask.alarm_active_index) {	//最接近闹钟号跟记录不一样，更新VM
            err = alarm_vm_write_mask(closest);
            if (err) {
                return 1;
            }
        }
    }

    return 0;
}


void alarm_init(const struct rtc_dev_platform_data *arg)
{
    rtc_init(arg);
    _user_isr_cbfun = arg->cbfun;

    u8 err = alarm_vm_read_mask();
    if (err) {
        err = alarm_vm_write_mask(0xff);
        if (err) {
            alarm_printf("init : alarm mask read & write vm err!\n");
            return;
        }
    }
    for (u8 i = 0; i < M_MAX_ALARM_NUMS; i ++) {
        memset(&(alarm_tab[i]), 0, sizeof(T_ALARM));
        alarm_vm_read_time_tab(&(alarm_tab[i]), i);     //读出数组
    }
    if (alarm_pnd_flag) {                               //防止初始化前，就已经起闹钟中断了
        alarm_send_event(alarm_mask.alarm_active_index);
    } else {
        err = alarm_update_all_time();                  //更新数组
        if (err) {
            alarm_printf("init : update alarm write vm err!\n");
            return;
        }
    }
    alarm_pnd_flag = 1;

    rtc_ioctl(IOCTL_GET_SYS_TIME,  &rtc_read_test); //读时钟
    alarm_printf("rtc_read_sys_time: %d-%d-%d %d:%d:%d\n",
                 rtc_read_test.year,
                 rtc_read_test.month,
                 rtc_read_test.day,
                 rtc_read_test.hour,
                 rtc_read_test.min,
                 rtc_read_test.sec);

    rtc_ioctl(IOCTL_GET_ALARM, &alm_read_test);        //读闹钟
    alarm_printf("rtc_read_alarm: %d-%d-%d %d:%d:%d\n",
                 alm_read_test.year,                        
                 alm_read_test.month,
                 alm_read_test.day,
                 alm_read_test.hour,
                 alm_read_test.min,
                 alm_read_test.sec);
}

void alarm_rtc_stop(void)
{
    set_alarm_ctrl(0);
}

void alarm_rtc_start(void)
{
    set_alarm_ctrl(1);
}

u8 alarm_get_active_index(void)
{
    return alarm_mask.alarm_active_index;
}

u8 alarm_get_en_info(void)
{
    u8 en = 0;
    for (u8 i = 0; i < M_MAX_ALARM_NUMS; i ++) {
        en |= BIT(!!(alarm_tab[i].en));
    }
    return en;
}

void alarm_get_time_info(T_ALARM *p, u8 index)
{
    memcpy((u8 *)p, (u8 *)&alarm_tab[index], sizeof(T_ALARM));
}

u8 alarm_add(T_ALARM *p, u8 index)
{
    if (index > M_MAX_ALARM_INDEX) {
        return 1;
    }
    if (p->mode > M_MAX_ALARM_MODE) {
        return 1;
    }
    memcpy((u8 *)&alarm_tab[index], (u8 *)p, sizeof(T_ALARM));
    u8 err = alarm_vm_write_time_tab(&alarm_tab[index], index);
    if (err) {
        return 1;
    }
    err = alarm_update_all_time();  //更新数组
    if (err) {
        return 1;
    }
    return 0;
}

u8 alarm_en(u8 index, u8 en)
{
    if (index > M_MAX_ALARM_INDEX) {
        return 1;
    }
    if ((!!en) == alarm_tab[index].en) {
        return 0;
    }
    alarm_tab[index].en = !!en;
    u8 err = alarm_vm_write_time_tab(&alarm_tab[index], index);
    if (err) {
        return 1;
    }
    err = alarm_update_all_time();  //更新数组
    if (err) {
        return 1;
    }
    return 0;
}

//设备事件响应demo
static void alarm_event_handler(struct sys_event *e)
{
    u8 index;
    u8 err;
    if ((u32)e->arg == DEVICE_EVENT_FROM_ALM) {
        if (e->u.dev.event == DEVICE_EVENT_IN) {
            index = (u8)(e->u.dev.value);
            if (alarm_tab[index].mode == E_ALARM_MODE_ONCE) {
                err = alarm_en(index, 0);
            } else {
                err = alarm_update_all_time();  //更新数组
            }
            if (err) {
                alarm_printf("isr : update alarm %d err!\n", index);
                return;
            }
            if (_user_isr_cbfun) {
                _user_isr_cbfun(index);
            }
        }
    }
}
SYS_EVENT_HANDLER(SYS_DEVICE_EVENT, alarm_event_handler, 0);

void alarm_send_event(u8 index)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = (void *)DEVICE_EVENT_FROM_ALM;
    e.u.dev.event = DEVICE_EVENT_IN;
    e.u.dev.value = index;
    sys_event_notify(&e);
}

void alm_wakeup_isr(void)
{
    if (alarm_pnd_flag) {
        alarm_send_event(alarm_mask.alarm_active_index);
    } else {
        alarm_pnd_flag = 1;
    }
}


void __attribute__((weak)) alarm_isr_user_cbfun(u8 index)
{
    printf("**** alarm %d : hello world ****\n", index);
}

//参考的测试代码
void user_alarm_test(void)
{
    T_ALARM tmp_alarm = {0};
    rtc_ioctl(IOCTL_GET_SYS_TIME, (u32)&tmp_alarm.time);
    tmp_alarm.en = 1;                           //初始化默认打开
#if 1
    tmp_alarm.mode = E_ALARM_MODE_ONCE;         //此闹钟只起作用一次
    tmp_alarm.time.hour = 0;
    tmp_alarm.time.min  = 1;
    alarm_add(&tmp_alarm, 0);
#endif

#if 1
    tmp_alarm.mode = E_ALARM_MODE_EVERY_DAY;    //此闹钟每天都起作用
    tmp_alarm.time.hour = 0;
    tmp_alarm.time.min  = 2;
    alarm_add(&tmp_alarm, 1);
#endif

#if 1
    tmp_alarm.mode = E_ALARM_MODE_EVERY_MONDAY | E_ALARM_MODE_EVERY_WEDNESDAY | E_ALARM_MODE_EVERY_SATURDAY; //此闹钟周1周3周6起作用
    tmp_alarm.time.hour = 0;
    tmp_alarm.time.min  = 3;
    alarm_add(&tmp_alarm, 2);
#endif

}

#if TCFG_RTC_ALARM_ENABLE
//24小时内定时起来
void rtc_alarm_set_timer(u32 seconds)
{
    alarm_printf("rtc_alarm_set_timer");

    u8 add_hour = seconds / 3600;
    u8 add_min = (seconds % 3600) / 60;
    u8 add_sec = seconds % 60;

    rtc_ioctl(IOCTL_GET_SYS_TIME,  &rtc_read_test); //读时钟
    alarm_printf("rtc_read_sys_time: %d-%d-%d %d:%d:%d\n",
                 rtc_read_test.year,
                 rtc_read_test.month,
                 rtc_read_test.day,
                 rtc_read_test.hour,
                 rtc_read_test.min,
                 rtc_read_test.sec);
    rtc_ioctl(IOCTL_GET_ALARM, &alm_read_test);        //读闹钟

    u16 tmp = rtc_read_test.sec + add_sec;
    rtc_read_test.sec = tmp % 60;

    tmp = rtc_read_test.min + add_min + tmp / 60;
    rtc_read_test.min = tmp % 60;

    tmp = rtc_read_test.hour + add_hour + tmp / 60;
    rtc_read_test.hour = tmp % 24;

    rtc_read_test.day += (tmp / 24);

    T_ALARM tmp_alarm = {0};
    tmp_alarm.en = 1;                           //初始化默认打开
    tmp_alarm.mode = E_ALARM_MODE_ONCE;         //此闹钟只起作用一次

    memcpy(&tmp_alarm.time, &rtc_read_test, sizeof(struct sys_time));
    alarm_add(&tmp_alarm, 0);
    memset(&rtc_read_test, 0, sizeof(struct sys_time));
    rtc_ioctl(IOCTL_GET_ALARM,  &alm_read_test);    //读闹钟,校验是否写成功
    alarm_printf("rtc_read_alarm: %d-%d-%d %d:%d:%d\n",
                 alm_read_test.year,                        
                 alm_read_test.month,
                 alm_read_test.day,
                 alm_read_test.hour,
                 alm_read_test.min,
                 alm_read_test.sec);
}


#endif

#endif

