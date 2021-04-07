
#include "includes.h"
#include "app_config.h"
#include "asm/cpu.h"
#include "asm/irq.h"
#include "asm/clock.h"
#include "system/timer.h"
#include "system/init.h"
#include "gpio.h"

/* #define LOG_TAG_CONST   TMR */
/* #define LOG_TAG         "[USER_TMR]" */
/* #define LOG_INFO_ENABLE */
/* #define LOG_DUMP_ENABLE */
/* #define LOG_ERROR_ENABLE */
/* #define LOG_DEBUG_ENABLE */
#include "debug.h"
/*
注意

timer 现在定义优先级为6 ，关总中断不关闭该优先级，
该中断里面使用函数 const 变量都必须定义在ram，否则会跑飞



 *
 */


#if (TCFG_LED7_RUN_RAM &&(TCFG_UI_LED1888_ENABLE ||  TCFG_UI_LED7_ENABLE))
#define TCFG_USER_TIMER_ENABLE
#endif

#ifdef TCFG_USER_TIMER_ENABLE


struct timer_hdl {
    int index;
    int prd;
};

static struct timer_hdl hdl;

#define __this  (&hdl)

static const u32 timer_div[] = {
    /*0000*/    1,
    /*0001*/    4,
    /*0010*/    16,
    /*0011*/    64,
    /*0100*/    2,
    /*0101*/    8,
    /*0110*/    32,
    /*0111*/    128,
    /*1000*/    256,
    /*1001*/    4 * 256,
    /*1010*/    16 * 256,
    /*1011*/    64 * 256,
    /*1100*/    2 * 256,
    /*1101*/    8 * 256,
    /*1110*/    32 * 256,
    /*1111*/    128 * 256,
};

#define APP_TIMER_CLK           clk_get("timer")
#define MAX_TIME_CNT            0x7fff
#define MIN_TIME_CNT            0x100


#define TIMER_CON               JL_TIMER4->CON
#define TIMER_CNT               JL_TIMER4->CNT
#define TIMER_PRD               JL_TIMER4->PRD
#define TIMER_VETOR             IRQ_TIME4_IDX

#define TIMER_UNIT_MS           2 //1ms起一次中断
#define MAX_TIMER_PERIOD_MS     (1000/TIMER_UNIT_MS)

/*-----------------------------------------------------------*/


static void (*timer_led_scan)(void *param);

void app_timer_led_scan(void (*led_scan)(void *))
{
    timer_led_scan = led_scan;
}

/////下面函数调用的使用函数都必须放在ram
___interrupt
AT_VOLATILE_RAM_CODE
static void timer2_isr()
{

    TIMER_CON |= BIT(14);
    if (timer_led_scan) {
        timer_led_scan(NULL);
    }
}

int led7_timer_init()
{
    u32 prd_cnt;
    u8 index;

    printf("------------%s :%d", __func__, __LINE__);

    for (index = 0; index < (sizeof(timer_div) / sizeof(timer_div[0])); index++) {
        prd_cnt = TIMER_UNIT_MS * (APP_TIMER_CLK / 1000) / timer_div[index];
        if (prd_cnt > MIN_TIME_CNT && prd_cnt < MAX_TIME_CNT) {
            break;
        }
    }
    __this->index   = index;
    __this->prd     = prd_cnt;

    TIMER_CNT = 0;
    TIMER_PRD = prd_cnt; //1ms
    request_irq(TIMER_VETOR, 6, timer2_isr, 0);
    TIMER_CON = (index << 4) | BIT(0) | BIT(3);

    printf("PRD : 0x%x / %d", TIMER_PRD, clk_get("timer"));

    return 0;
}
__initcall(led7_timer_init);

#endif

