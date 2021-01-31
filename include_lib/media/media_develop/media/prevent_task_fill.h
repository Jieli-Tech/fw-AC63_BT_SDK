#ifndef _PREVENT_TASK_FILL_H
#define _PREVENT_TASK_FILL_H

#include "generic/typedef.h"
#include "generic/list.h"
#include "system/includes.h"

struct prevent_task_fill {
    struct list_head head;	// 链表头
    const char *name;
    u16 run_cnt;
    u16 time_id;
    int pend_to;
    OS_SEM sem;
};

struct prevent_task_fill_ch {
    struct list_head list_entry;	// 链表
    struct prevent_task_fill *prevent;
    u16 run_cnt;
    u16 time_cnt;
    u16 time_ms;
    u16 no_run_time;	// 多长时间没运行
    OS_SEM sem;
};

struct prevent_task_fill *prevent_task_fill_create(const char *task_name);

struct prevent_task_fill_ch *prevent_task_fill_ch_open(struct prevent_task_fill *hdl, u16 time_ms);
void prevent_task_fill_ch_close(struct prevent_task_fill_ch **pp_ch);

int prevent_task_fill_ch_run(struct prevent_task_fill_ch *ch);

#endif /*_PREVENT_TASK_FILL_H*/

