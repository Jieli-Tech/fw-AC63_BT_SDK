#ifndef _PREVENT_TASK_FILL_H
#define _PREVENT_TASK_FILL_H

#include "generic/typedef.h"
#include "generic/list.h"
#include "system/includes.h"

struct prevent_task_fill {
    struct list_head head;	// 链表头
    u8  enable;
    u16 run_cnt;
    u16 time_id;
    u16 to_10ms;
    int pend_to;
    void (*resume)(void *);
    void *resume_priv;
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

#if 0
struct prevent_task_fill *prevent_task_fill_create(const char *task_name);

struct prevent_task_fill_ch *prevent_task_fill_ch_open(struct prevent_task_fill *hdl, u16 time_ms);
void prevent_task_fill_ch_close(struct prevent_task_fill_ch **pp_ch);

int prevent_task_fill_ch_run(struct prevent_task_fill_ch *ch);
#else
struct prevent_task_fill *prevent_task_fill_create(u16 to);
void prevent_task_fill_enable(struct prevent_task_fill *hdl, u8 enable, void (*resume)(void *), void *resume_priv);

void prevent_task_fill_run(struct prevent_task_fill *hdl);
u16 prevent_task_fill_get_pend_to(struct prevent_task_fill *hdl);

struct prevent_task_fill_ch *prevent_task_fill_ch_open(struct prevent_task_fill *hdl, u16 time_ms);
void prevent_task_fill_ch_close(struct prevent_task_fill_ch **pp_ch);
int prevent_task_fill_ch_run(struct prevent_task_fill_ch *ch);

#endif

#endif /*_PREVENT_TASK_FILL_H*/

