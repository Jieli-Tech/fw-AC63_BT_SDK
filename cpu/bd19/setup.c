#include "asm/includes.h"
//#include "asm/ldo.h"
//#include "asm/cache.h"
#include "asm/wdt.h"
#include "asm/debug.h"
#include "asm/efuse.h"
#include "asm/power/p33.h"
#include "asm/efuse.h"
#include "system/task.h"
#include "timer.h"

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
    P3_PCNT_SET0 = 0xac;
    cpu_reset();
#endif
}

void timer(void *p)
{
    /* DEBUG_SINGAL_1S(1); */
    sys_timer_dump_time();
    /* DEBUG_SINGAL_1S(0);*/
}

void app_bank_init()
{
#ifdef CONFIG_CODE_BANK_ENABLE
    extern void bank_syscall_entry();
    request_irq(IRQ_SYSCALL_IDX, 3, bank_syscall_entry, 0);
    extern void load_common_code();
    load_common_code();
#endif
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


u32 stack_magic[4] sec(.stack_magic);
u32 stack_magic0[4] sec(.stack_magic0);
void memory_init(void);

void setup_arch()
{
#if TCFG_CHARGE_ENABLE
    //防止LDOIN先上电,VBAT后上电导致的VDC12过压(越靠前调用越好)
    extern void power_enter_charge_mode(void);
    power_enter_charge_mode();
#endif

    //asm("trigger ");
    JL_UART0->CON0 |= BIT(2); //use lp waiting, must set Tx pending Enable

    memory_init();
    memset(stack_magic, 0x5a, sizeof(stack_magic));
    memset(stack_magic0, 0x5a, sizeof(stack_magic0));

    //P11 系统必须提前打开
    p11_init();

    wdt_init(WDT_4S);
    /* wdt_close(); */

    clk_voltage_init(TCFG_CLOCK_MODE, SYSVDD_VOL_SEL_126V);
    clk_early_init(TCFG_CLOCK_SYS_SRC, TCFG_CLOCK_OSC_HZ, TCFG_CLOCK_SYS_HZ);

    tick_timer_init();
    /*interrupt_init();*/

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

#ifdef CONFIG_DEBUG_ENABLE
    log_early_init(1024);
#endif

#endif

    log_i("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    log_i("         setup_arch %s %s", __DATE__, __TIME__);
    log_i("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

    clock_dump();

    efuse_dump();

    reset_source_dump();

    /* power_reset_source_dump(); */

    //Register debugger interrupt
    /* request_irq(0, 2, exception_irq_handler, 0); */
    request_irq(1, 2, exception_irq_handler, 0);

    sys_timer_init();

    debug_init();

    /* sys_timer_add(NULL, timer, 1 * 1000); */


    __crc16_mutex_init();
}

/*-----------------------------------------------------------*/
