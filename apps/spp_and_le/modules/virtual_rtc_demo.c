#include "rtc/virtual_rtc.h"
#include "board_config.h"
#include "user_cfg_id.h"

#if TCFG_USE_VIRTUAL_RTC //just support AC695/AC635

struct sys_time write_alarm_test;
struct sys_time write_sys_time_test;
struct sys_time read_alarm_test;
struct sys_time read_sys_time_test;

void vir_rtc_test(void)
{
    static u32 time = 0;
    time++;
    if (time >= 100) {
        time = 0;
        vir_read_sys_time(&read_sys_time_test);
        printf("vir_rtc_sys_time: %d-%d-%d %d:%d:%d\n",
               read_sys_time_test.year,
               read_sys_time_test.month,
               read_sys_time_test.day,
               read_sys_time_test.hour,
               read_sys_time_test.min,
               read_sys_time_test.sec);
    }
    JL_PORTB->DIR  |= BIT(2);
    JL_PORTB->PU   |= BIT(2);
    JL_PORTB->DIE  |= BIT(2);
    JL_PORTB->DIEH |= BIT(2);
    if (!(JL_PORTB->IN & BIT(2))) {
        printf(">>>>>>>>>>SOFT_OFF_ENTER");
        power_set_soft_poweroff();
    }

}


void alm_wakeup_isr()
{
    printf("alarm_wakeup_isr!!!!!");
}
void  set_rtc_default_time(struct sys_time *t)
{
    t->year = 2020;
    t->month = 2;
    t->day = 28;
    t->hour = 23;
    t->min = 59;
    t->sec = 40;
}

void virtual_rtc_test(void)
{
    sys_timer_add(NULL, vir_rtc_test, 10);
    vir_rtc_simulate_init(NULL, NULL);
    vir_read_sys_time(&read_sys_time_test);
    printf("vir_rtc_read_sys_time>>>>>>>>>>>>>>>>>>>>>>\n");
    printf("vir_rtc_sys_time: %d-%d-%d %d:%d:%d\n",
           read_sys_time_test.year,
           read_sys_time_test.month,
           read_sys_time_test.day,
           read_sys_time_test.hour,
           read_sys_time_test.min,
           read_sys_time_test.sec);

    vir_alarm_enable(1);
    write_alarm_test.year  = 2020;
    write_alarm_test.month = 2;
    write_alarm_test.day   = 28;
    write_alarm_test.hour  = 23;
    write_alarm_test.min   = 59;
    write_alarm_test.sec   = 50;

    vir_write_alarm(&write_alarm_test);
    vir_read_alarm(&read_alarm_test);
    printf("vir_rtc_read_alarm_time>>>>>>>>>>>>>>>>>>>>>>\n");
    printf("vir_rtc_alarm_time: %d-%d-%d %d:%d:%d\n",
           read_alarm_test.year,
           read_alarm_test.month,
           read_alarm_test.day,
           read_alarm_test.hour,
           read_alarm_test.min,
           read_alarm_test.sec);
}

#endif
