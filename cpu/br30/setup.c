#include "asm/includes.h"
//#include "asm/ldo.h"
//#include "asm/cache.h"
#include "asm/wdt.h"
#include "asm/debug.h"
#include "asm/efuse.h"
#include "asm/charge.h"
#include "asm/power/p33.h"
#include "system/task.h"
#include "timer.h"
#include "system/includes.h"

#include "app_config.h"
#include "gpio.h"
//#include "power_manage.h"
//
#define LOG_TAG_CONST       SETUP
#define LOG_TAG             "[SETUP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

//extern void dv15_dac_early_init(u8 ldo_sel, u8 pwr_sel, u32 dly_msecs);
//
extern void sys_timer_init(void);

extern void tick_timer_init(void);

extern void vPortSysSleepInit(void);

extern u32 reset_source_dump(void);

extern u8 power_reset_source_dump(void);

extern void exception_irq_handler(void);
int __crc16_mutex_init();

extern int __crc16_mutex_init();


#define DEBUG_SINGAL_IDLE(x)        //if (x) IO_DEBUG_1(A, 7) else IO_DEBUG_0(A, 7)
#define DEBUG_SINGAL_1S(x)          //if (x) IO_DEBUG_1(A, 6) else IO_DEBUG_0(A, 6)

#if (defined CONFIG_DEBUG_ENABLE) || (defined CONFIG_DEBUG_LITE_ENABLE)
void debug_uart_init(const struct uart_platform_data *data);
#endif

#if 0
___interrupt
void exception_irq_handler(void)
{
    ___trig;

    exception_analyze();

    log_flush();
    while (1);
}
#endif



/*
 * 此函数在cpu0上电后首先被调用,负责初始化cpu内部模块
 *
 * 此函数返回后，操作系统才开始初始化并运行
 *
 */

#if 0
static void early_putchar(char a)
{
    if (a == '\n') {
        UT2_BUF = '\r';
        __asm_csync();
        while ((UT2_CON & BIT(15)) == 0);
    }
    UT2_BUF = a;
    __asm_csync();
    while ((UT2_CON & BIT(15)) == 0);
}

void early_puts(char *s)
{
    do {
        early_putchar(*s);
    } while (*(++s));
}
#endif

void cpu_assert_debug()
{
#ifdef CONFIG_DEBUG_ENABLE
    log_flush();
    local_irq_disable();
    while (1);
#else
    cpu_reset();
#endif
}

void timer(void *p)
{
    /* DEBUG_SINGAL_1S(1); */
    sys_timer_dump_time();
    /* DEBUG_SINGAL_1S(0);*/
    /* usr_timer_dump(); */
}

void test_fun()
{
    wdt_close();
    while (1);

}


__attribute__((weak))
void maskrom_init(void)
{
    return;
}

static void timeout_handler(void *p)
{
    log_debug("---Timeout ");
}

static void power_mode_init()
{
#if TCFG_POWER_MODE_QUIET_ENABLE
    power_set_mode(PWR_LDO15);
#else
    if (get_charge_online_flag()) {
        power_set_mode(PWR_LDO15);
    } else {
        power_set_mode(TCFG_LOWPOWER_POWER_SEL);
    }
#endif
}
late_initcall(power_mode_init);


void setup_arch()
{
    //asm("trigger ");
    JL_UART0->CON0 |= BIT(2); //use lp waiting, must set Tx pending Enable

    memory_init();

    //P11 系统必须提前打开
    p11_init();
    wdt_init(WDT_4S);

    /* #if TCFG_AUDIO_ANC_ENABLE */
    /* wdt_close(); */
    /* #else */
    /* wdt_init(WDT_4S); */
    /* #endif */
    clk_init_osc_cap(0x0a, 0x0a);
    clk_voltage_init(TCFG_CLOCK_MODE, SYSVDD_VOL_SEL_126V);
    clk_early_init(TCFG_CLOCK_SYS_SRC, TCFG_CLOCK_OSC_HZ, TCFG_CLOCK_SYS_HZ);

    /*interrupt_init();*/

    tick_timer_init();

#if (defined CONFIG_DEBUG_ENABLE) || (defined CONFIG_DEBUG_LITE_ENABLE)
    debug_uart_init(NULL);

#ifdef CONFIG_DEBUG_ENABLE
    log_early_init(1024);
#endif

#endif



    log_i("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    log_i("         setup_arch %s %s", __DATE__, __TIME__);
    log_i("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

    clock_dump();

    /* log_info("resour est: %d", get_boot_flag()); */
    //set_boot_flag(99);
    /* log_info("resour est: %d", get_boot_flag()); */

    reset_source_dump();

    extern u32 p33_rd_page_test(u8 page);
    //r_printf("page0:%x page1:%x\n",p33_rd_page_test(0),p33_rd_page_test(1));
    extern void set_vbg_level_from_efuse(void);
    set_vbg_level_from_efuse();

    /* power_reset_source_dump(); */

    //Register debugger interrupt
    request_irq(0, 2, exception_irq_handler, 0);
    request_irq(1, 2, exception_irq_handler, 0);

    sys_timer_init();

    debug_init();

    /* sys_timer_add(NULL, timer, 1 * 1000); */


    __crc16_mutex_init();
}

/*-----------------------------------------------------------*/
