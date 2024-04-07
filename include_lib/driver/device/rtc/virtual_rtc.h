#ifndef  __VIRTUAL_RTC_H__
#define  __VIRTUAL_RTC_H__

#include "system/includes.h"
#include "system/sys_time.h"
#include "system/syscfg_id.h"


void vir_rtc_simulate_dump();
void get_lp_timer1_status(void);
int vir_rtc_simulate_init(const struct dev_node *node, void *arg);

void vir_write_alarm(struct sys_time *alarm_time);
void vir_read_alarm(struct sys_time *alarm_time);

void vir_write_sys_time(struct sys_time *curr_time);
void vir_read_sys_time(struct sys_time *curr_time);

void vir_alarm_enable(u8 set_alarm);
u8 vir_get_alarm_enable(void);

void vir_set_vm_id(u8 rtc_vm_id, u8 alm_vm_id, u8 sec_vm_id);


#endif  /*VIRTUAL_RTC_H*/
