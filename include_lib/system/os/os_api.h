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

/* --------------------------------------------------------------------------*/
/**
 * @brief :创建任务
 *
 * @Param task :任务回调函数
 * @Param p_arg :传递给任务回调函数的参数
 * @Param prio :任务的优先级
 * @Param stksize :任务的堆栈大小, 单位(u32)
 * @Param qsize :任务的queue大小，单位(byte)
 * @Param name :任务名
 *
 * @Returns :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_task_create(void (*task)(void *p_arg),
                   void *p_arg,
                   u8 prio,
                   u32 stksize,
                   int qsize,
                   const char *name);


/* --------------------------------------------------------------------------*/
/**
 * @brief :获取当前任务名
 *
 * @Returns   :当前任务名
 */
/* ----------------------------------------------------------------------------*/
const char *os_current_task();

/* --------------------------------------------------------------------------*/
/**
 * @brief :删除任务
 *
 * @Param name :任务名
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_task_del_req(const char *name);

/* --------------------------------------------------------------------------*/
/**
 * @brief :响应任务删除请求，标记资源已经释放，可以删除当前任
 *
 * @Param name: 任务名，任务自己可以用OS_TASK_SELF
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_task_del_res(const char *name);

/* --------------------------------------------------------------------------*/
/**
 * @brief :删除任务
 *
 * @Param name :任务名
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_task_del(const char *name);


/* --------------------------------------------------------------------------*/
/**
 * @brief :延时。中断函数或者关闭系统总中断的情况下不能调用此函数
 *
 * @Param time_tick :延时时间
 */
/* ----------------------------------------------------------------------------*/
void os_time_dly(int time_tick);


/* --------------------------------------------------------------------------*/
/**
 * @brief :发送Q_USER类型taskq
 *
 * @Param name :任务名
 * @Param argc :后面传入的参数的个数。发送的最大参数个数限制为8个int类型
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_taskq_post(const char *name, int argc, ...);

/* --------------------------------------------------------------------------*/
/**
 * @brief :非阻塞方式查询taskq
 *
 * @Param argc :最大可获取的queue长度，单位(int)
 * @Param argv :存放queue的buf
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_taskq_accept(int argc, int *argv);

/* --------------------------------------------------------------------------*/
/**
 * @brief :阻塞方式获取taskq
 *
 * @Param fmt :保留，传NULL
 * @Param argv :存放queue的buf
 * @Param argc :最大可获取的queue长度，单位(int)
 * @Param tick :阻塞时长，0表示一直阻塞
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_taskq_pend(const char *fmt, int *argv, int argc);
int os_task_pend(const char *fmt, int *argv, int argc);
int __os_taskq_pend(int *argv, int argc, int tick);



/* --------------------------------------------------------------------------*/
/**
 * @brief :发送指定类型的taskq
 *
 * @Param name :任务名
 * @Param type :queue类型
 * @Param argc :后面传入的参数的个数
 * @Param argv
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_taskq_post_type(const char *name, int type, int argc, int *argv);


/* --------------------------------------------------------------------------*/
/**
 * @brief :发送Q_MSG类型的taskq
 *
 * @Param name :任务名
 * @Param argc :后面参数的个数
 * @Param ...
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_taskq_post_msg(const char *name, int argc, ...);


/* --------------------------------------------------------------------------*/
/**
 * @brief :发送Q_EVENT类型的taskq
 *
 * @Param name :任务名
 * @Param argc :后面参数的个数
 * @Param ...
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_taskq_post_event(const char *name, int argc, ...);


/* --------------------------------------------------------------------------*/
/**
 * @brief :删除指定类型的taskq
 *
 * @Param name :任务名
 * @Param type :taskq的类型
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_taskq_del_type(const char *name, int type);
int os_taskq_del(const char *name, int type);

/* --------------------------------------------------------------------------*/
/**
 * @brief :清除所有taskq
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_taskq_flush();

/* --------------------------------------------------------------------------*/
/**
 * @brief :创建信号量
 *
 * @Param sem:信号量
 * @Param int:初始计数值
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_sem_create(OS_SEM *, int);

/* --------------------------------------------------------------------------*/
/**
 * @brief :非阻塞方式查询信号量
 *
 * @Param sem :信号量
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_sem_accept(OS_SEM *);

/* --------------------------------------------------------------------------*/
/**
 * @brief :阻塞方式获取信号量
 *
 * @Param sem:信号量
 * @Param timeout:等待时长，0表示一直等待
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_sem_pend(OS_SEM *, int timeout);

/* --------------------------------------------------------------------------*/
/**
 * @brief :发送信号量
 *
 * @Param sem:信号量
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_sem_post(OS_SEM *);

/* --------------------------------------------------------------------------*/
/**
 * @brief :信号量删除
 *
 * @Param sem:信号量
 * @Param block:保留
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_sem_del(OS_SEM *, int block);

/* --------------------------------------------------------------------------*/
/**
 * @brief :信号量设置
 *
 * @Param sem:信号量
 * @Param cnt:计数值
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_sem_set(OS_SEM *, u16 cnt);

/* --------------------------------------------------------------------------*/
/**
 * @brief :信号量类型是否queueQUEUE_TYPE_COUNTING_SEMAPHORE
 *
 * @Param : true:信号量匹配, fail:信号量不匹配
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_sem_valid(OS_SEM *);

/* --------------------------------------------------------------------------*/
/**
 * @brief :判断信号量是否可用
 *
 * @Param sem:信号量
 *
 * @Returns   :可用数量
 */
/* ----------------------------------------------------------------------------*/
int os_sem_query(OS_SEM *);

/* --------------------------------------------------------------------------*/
/**
 * @brief :创建互斥量
 *
 * @Param mutex: 互斥量
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_mutex_create(OS_MUTEX *);

/* --------------------------------------------------------------------------*/
/**
 * @brief :非阻塞方式查询互斥量
 *
 * @Param mutex:互斥量
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_mutex_accept(OS_MUTEX *);

/* --------------------------------------------------------------------------*/
/**
 * @brief :阻塞方式查询互斥量
 *
 * @Param mutex:互斥量
 * @Param timeout:等待时间，0表示一直等待
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_mutex_pend(OS_MUTEX *, int timeout);

/* --------------------------------------------------------------------------*/
/**
 * @brief :发送斥量
 *
 * @Param mutex:互斥量
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_mutex_post(OS_MUTEX *);

/* --------------------------------------------------------------------------*/
/**
 * @brief :删除斥量
 *
 * @Param mutex:互斥量
 * @Param block:保留
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
int os_mutex_del(OS_MUTEX *, int block);

/* --------------------------------------------------------------------------*/
/**
 * @brief :互斥量类型是否queueQUEUE_TYPE_MUTEX
 *
 * @Param : true:互斥量匹配, fail:互斥量不匹配
 *
 * @Returns   :错误码
 */
/* ----------------------------------------------------------------------------*/
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

int task_queue_post_event(const char *name, void *data, int len);

#ifdef __cplusplus
}
#endif
#endif
