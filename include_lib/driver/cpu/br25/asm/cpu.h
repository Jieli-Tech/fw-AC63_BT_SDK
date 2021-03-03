
#ifndef ASM_CPU_H
#define ASM_CPU_H


#include "br25.h"
#include "csfr.h"

#ifndef __ASSEMBLY__

typedef unsigned char   		u8, bool, BOOL;
typedef char            		s8;
typedef unsigned short  		u16;
typedef signed short    		s16;
typedef unsigned int    		u32;
typedef signed int      		s32;
typedef unsigned long long 		u64;
typedef u32						FOURCC;
typedef long long               s64;
typedef unsigned long long      u64;


#endif


#define ___trig        __asm__ volatile ("trigger")


#ifndef BIG_ENDIAN
#define BIG_ENDIAN 			0x3021
#endif
#ifndef LITTLE_ENDIAN
#define LITTLE_ENDIAN 		0x4576
#endif
#define CPU_ENDIAN 			LITTLE_ENDIAN


#define CPU_CORE_NUM     1


#define  CPU_TASK_CLR(a)
#define  CPU_TASK_SW(a) 		\
    do { \
        q32DSP(0)->ILAT_SET |= BIT(7-a); \
        clr_wdt(); \
    } while (0)


#define  CPU_INT_NESTING 	2


#ifndef __ASSEMBLY__

extern void clr_wdt();

///屏蔽的优先级
#define IRQ_IPMASK   6


#if CPU_CORE_NUM > 1
static inline int current_cpu_id()
{
    unsigned id;
    asm volatile("%0 = cnum" : "=r"(id) ::);
    return id ;
}
#else
static inline int current_cpu_id()
{
    return 0;
}
#endif

static inline int cpu_in_irq()
{
    int flag;
    __asm__ volatile("%0 = icfg" : "=r"(flag));
    return flag & 0xff;
}

static inline int cpu_irq_disabled()
{
    int flag;
    __asm__ volatile("%0 = icfg" : "=r"(flag));
    return (flag & 0x300) != 0x300 || ((q32DSP(0)->IPMASK) == IRQ_IPMASK);
}

#if 0
static inline int data_sat_s16(int ind)
{
    if (ind > 32767) {
        ind = 32767;
    } else if (ind < -32768) {
        ind = -32768;
    }
    return ind;
}

#else
static __attribute__((always_inline)) int data_sat_s16(int ind)
{
    __asm__ volatile(
        " %0 = sat16(%0)(s)  \t\n"
        : "=&r"(ind)
        : "0"(ind)
        :);
    return ind;
}
#endif


static inline u32 reverse_u32(u32 data32)
{
#if 0
    u8 *dataptr = (u8 *)(&data32);
    data32 = (((u32)dataptr[0] << 24) | ((u32)dataptr[1] << 16) | ((u32)dataptr[2] << 8) | (u32)dataptr[3]);
#else
    __asm__ volatile("%0 = rev8(%0) \t\n" : "=&r"(data32) : "0"(data32) :);
#endif
    return data32;
}

static inline u32 reverse_u16(u16 data16)
{
    u32 retv;
#if 0
    u8 *dataptr = (u8 *)(&data16);
    retv = (((u32)dataptr[0] << 8) | ((u32)dataptr[1]));
#else
    retv = ((u32)data16) << 16;
    __asm__ volatile("%0 = rev8(%0) \t\n" : "=&r"(retv) : "0"(retv) :);
#endif
    return retv;
}

static inline u32 rand32()
{
    return JL_RAND->R64L;
}

#define __asm_sine(s64, precision) \
    ({ \
        u64 ret; \
        u8 sel = (0x00 << 2) | (precision); \
	    __asm__ volatile ("%0 = copex(%1) (%2)" : "=r"(ret) : "r"(s64), "i"(sel)); \
        ret; \
    })

extern void chip_reset(void);
#define cpu_reset chip_reset
// void p33_soft_reset(void);
// static inline void cpu_reset(void)
// {
// // JL_CLOCK->PWR_CON |= (1 << 4);
// p33_soft_reset();
// }

#define __asm_csync() \
    do { \
		asm volatile("csync;"); \
    } while (0)

#include "asm/irq.h"
#include "generic/printf.h"
#include "system/generic/log.h"


#define arch_atomic_read(v)  \
	({ \
        __asm_csync(); \
		(*(volatile int *)&(v)->counter); \
	 })
#if 0
extern volatile int cpu_lock_cnt[];
extern volatile int irq_lock_cnt[];


static inline void local_irq_disable()
{
    __builtin_pi32v2_cli();
    irq_lock_cnt[current_cpu_id()]++;
}


static inline void local_irq_enable()
{
    if (--irq_lock_cnt[current_cpu_id()] == 0) {
        __builtin_pi32v2_sti();
    }
}
#else

extern void local_irq_disable();
extern void local_irq_enable();
#endif


#define arch_spin_trylock(lock) \
	do { \
        __asm_csync(); \
		while ((lock)->rwlock); \
		(lock)->rwlock = 1; \
	}while(0)

#define arch_spin_lock(lock) \
	do { \
        int ret = false; \
        __asm_csync(); \
		if (!(lock)->rwlock) { \
            ret = true; \
		    (lock)->rwlock = 1; \
        } \
        if (ret) \
            break; \
	}while(1)

#define arch_spin_unlock(lock) \
	do { \
		(lock)->rwlock = 0; \
	}while(0)




#define	CPU_SR_ALLOC() 	\
//	int flags

#define CPU_CRITICAL_ENTER()  \
	do { \
		local_irq_disable(); \
        __asm_csync(); \
	}while(0)


#define CPU_CRITICAL_EXIT() \
	do { \
		local_irq_enable(); \
	}while(0)


extern void cpu_assert_debug();
extern const int config_asser;

#define ASSERT(a,...)   \
		do { \
			if(config_asser){\
				if(!(a)){ \
					printf("file:%s, line:%d", __FILE__, __LINE__); \
					printf("ASSERT-FAILD: "#a" "__VA_ARGS__); \
					cpu_assert_debug(); \
				} \
			}else {\
				if(!(a)){ \
            		cpu_reset(); \
				}\
			}\
		}while(0);




#endif //__ASSEMBLY__


#endif

