#ifndef   OS_API_H
#define   OS_API_H


#ifdef __cplusplus
extern "C"
{
#endif




#include "generic/typedef.h"
//#include "generic/list.h"
#include "os/os_cpu.h"
#include "os/os_error.h"
#include "os/os_type.h"



#define Q_MSG           0x100000
#define Q_EVENT         0x200000
#define Q_CALLBACK      0x300000
#define Q_USER          0x400000

#define  OS_DEL_NO_PEND               0u
#define  OS_DEL_ALWAYS                1u

#define  OS_TASK_DEL_REQ           0x01u
#define  OS_TASK_DEL_RES           0x02u
#define  OS_TASK_DEL_OK            0x03u


#define  OS_TASK_SELF        (char *)0x1
#define  OS_TASK_FATHER      (char *)0x2


#define OS_MSG_KEY          1
#define OS_MSG_TOUCH        2
#define OS_MSG_EVENT        3
#define OS_MSG_CALLBACK     4
#define OS_MSG_DEL_REQ      5
#define OS_MSG_MSG          6


/*struct os_msg {
    int type;
    int msg;
    int err;
    u8 deleted;
    char ref;
    OS_SEM   sem;
    OS_MUTEX mutex;
    struct list_head entry;
    u32 data[0];
};*/

/*struct os_msg_callback {
    struct os_msg msg;
    u8 argc;
    u8 sync;
    u8 arg_exp;
    int argv[8];
    void *function;
};*/

#ifdef CONFIG_CPU_BR25
void *os_init();
#else
void os_init();
#endif
void os_start(void);
void os_init_tick(int);

int os_task_create(void (*task)(void *p_arg),
                   void *p_arg,
                   u8 prio,
                   u32 stksize,
                   int qsize,
                   const char *name);

const char *os_current_task();

void os_task_exit();

int os_task_del_req(const char *name);

int os_task_del_res(const char *name);

int os_task_del(const char *name);


void os_time_dly(int time_tick);


int __os_taskq_post(const char *name, int type, int argc, int *argv);

int __os_taskq_pend(int *argv, int argc, int tick);

int os_taskq_accept(int, int *);

int os_taskq_pend(const char *fmt, int *argv, int argc);

int os_task_pend(const char *fmt, int *argv, int argc);

int os_taskq_post(const char *name, int argc, ...);

int os_taskq_del(const char *name, int type);

int os_taskq_post_type(const char *name, int type, int argc, int *argv);

// int task_queue_post_msg(const char *name, void *data, int len);
int task_queue_post_event(const char *name, void *data, int len);

int os_taskq_post_msg(const char *name, int argc, ...);

int os_taskq_post_event(const char *name, int argc, ...);

int os_taskq_del_type(const char *name, int type);

int os_taskq_flush();

int os_sem_create(OS_SEM *, int);

int os_sem_accept(OS_SEM *);

int os_sem_pend(OS_SEM *, int timeout);

int os_sem_post(OS_SEM *);

int os_sem_del(OS_SEM *, int block);

int os_sem_set(OS_SEM *, u16 cnt);

int os_sem_valid(OS_SEM *);

int os_sem_query(OS_SEM *);

int os_mutex_create(OS_MUTEX *);

int os_mutex_accept(OS_MUTEX *);

int os_mutex_pend(OS_MUTEX *, int timeout);

int os_mutex_post(OS_MUTEX *);

int os_mutex_del(OS_MUTEX *, int block);

int os_mutex_valid(OS_MUTEX *);

/*struct os_msg *os_message_create(int size);

int os_message_receive(struct os_msg **msg, int block_time);

int os_message_send(const char *task_name, struct os_msg *msg, int msgflg);

int os_message_delete(struct os_msg *msg);*/



int os_q_create(OS_QUEUE *pevent, /*void **start, */QS size);

int os_q_del(OS_QUEUE *pevent, u8 opt);

int os_q_flush(OS_QUEUE *pevent);

int os_q_pend(OS_QUEUE *pevent, int timeout, void *msg);

int os_q_post(OS_QUEUE *pevent, void *msg);

int os_q_query(OS_QUEUE *pevent);

int os_q_valid(OS_QUEUE *pevent);



#ifdef __cplusplus
}
#endif
#endif
