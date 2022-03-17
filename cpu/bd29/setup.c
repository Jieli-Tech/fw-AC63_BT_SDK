#include "asm/includes.h"
//#include "asm/ldo.h"
//#include "asm/cache.h"
#include "system/task.h"
#include "timer.h"
#include "system/init.h"

#include "app_config.h"
#include "gpio.h"
#include "board_config.h"

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

extern void reset_source_dump(void);

extern void power_reset_source_dump(void);

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
}

static void (*uart_db_irq_handler_callback)(u8 *packet, u32 size);
extern void uart_dev_set_irq_handler_hook(void *uart_irq_hook);
static void uart_db_irq_handler_hook(u8 *rbuf, u32 len)
{
    if (len) {
        log_info("uart_rx_data %d:", len);
        put_buf(rbuf, len);
    }

    if (uart_db_irq_handler_callback) {
        uart_db_irq_handler_callback(rbuf, len);
    }
}

void uart_db_regiest_recieve_callback(void *rx_cb)
{
    uart_db_irq_handler_callback = rx_cb;
}

void setup_arch()
{
    wdt_init(WDT_4S);

    clk_init_osc_cap(0x0b, 0x0b);
    clk_voltage_init(TCFG_CLOCK_MODE, 1160);

    clk_early_init(TCFG_CLOCK_SYS_SRC, TCFG_CLOCK_OSC_HZ, TCFG_CLOCK_SYS_HZ);

    tick_timer_init();

#if (defined CONFIG_DEBUG_ENABLE) || (defined CONFIG_DEBUG_LITE_ENABLE)
    debug_uart_init(NULL);

#if TCFG_UART0_RX_PORT != NO_CONFIG_PORT
    JL_UART0->CON0 &= ~BIT(2); //disable Tx pending Enable
    uart_dev_set_irq_handler_hook(uart_db_irq_handler_hook);

#if TCFG_LOWPOWER_LOWPOWER_SEL
    /*需要关闭POWERDOWN,否则接收会丢数据*/
#error "need define TCFG_LOWPOWER_LOWPOWER_SEL  0 !!!!!!"
#endif
#endif

    /* void vmm_debug(); */
    /* vmm_debug(); */

    /*interrupt_init();*/

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

    power_reset_source_dump();

    request_irq(1, 2, exception_irq_handler, 0);

    debug_init();

    sys_timer_init();

    /* sys_timer_add(NULL, timer, 10 * 1000); */


    __crc16_mutex_init();
}

/*-----------------------------------------------------------*/



