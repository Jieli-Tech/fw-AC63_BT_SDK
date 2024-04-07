#include "asm/includes.h"
//#include "asm/ldo.h"
//#include "asm/cache.h"
#include "system/task.h"
#include "timer.h"
#include "system/init.h"

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

extern void reset_source_dump(void);

extern u8 power_reset_source_dump(void);

extern void exception_irq_handler(void);
int __crc16_mutex_init();

extern int __crc16_mutex_init();

#if TCFG_USE_VIRTUAL_RTC
extern void get_lp_timer1_status(void);
extern void vir_set_vm_id(u8 rtc_vm_id, u8 alm_vm_id, u8 sec_vm_id);
#endif


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

u8 power_reset_src = 0;

extern void sputchar(char c);
extern void sput_buf(const u8 *buf, int len);
void sput_u32hex(u32 dat);
void *vmem_get_phy_adr(void *vaddr);

void test_fun()
{
    wdt_close();
    while (1);

}



void load_common_code();
void app_bank_init()
{
#ifdef CONFIG_CODE_BANK_ENABLE
    extern void bank_syscall_entry();
    request_irq(IRQ_SYSCALL_IDX, 0, bank_syscall_entry, 0);
#endif

#ifdef CONFIG_CODE_BANK_ENABLE
    load_common_code();
#endif
}

static void power_sanity_check(void)
{
    if ((TCFG_LOWPOWER_POWER_SEL == PWR_DCDC15) && (TCFG_CLOCK_MODE != CLOCK_MODE_IGNORED)) {
        log_error("PWR_DCDC15 must choose CLOCK_MODE_IGNORED");
        while (1);
    }
    if ((TCFG_LOWPOWER_POWER_SEL == PWR_LDO15) && (TCFG_CLOCK_MODE == CLOCK_MODE_IGNORED)) {
        log_error("PWR_DCDC15 must not choose CLOCK_MODE_IGNORED");
        while (1);
    }
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

void memory_init(void);
void setup_arch()
{
    memory_init();

    /* memset(stack_magic, 0x5a, sizeof(stack_magic)); */
    /* memset(stack_magic0, 0x5a, sizeof(stack_magic0)); */

    wdt_init(WDT_8S);
    /* wdt_close(); */
    //上电初始所有io
    port_init();

    u8 mode = TCFG_CLOCK_MODE;
    if (TCFG_LOWPOWER_POWER_SEL == PWR_DCDC15) {
        mode = CLOCK_MODE_USR;
    }


#ifdef CONFIG_BOARD_AC6963A_TWS
    clk_init_osc_cap(0x07, 0x07);
#else
    clk_init_osc_cap(0x0a, 0x0a);
#endif

#if (TCFG_CLOCK_SYS_SRC == SYS_CLOCK_INPUT_PLL_RCL)
    mode = CLOCK_MODE_USR;//免晶振时,用usr,并提高内核电压
    clk_voltage_init(mode, SYSVDD_VOL_SEL_120V, VDC13_VOL_SEL_110V, TCFG_LOWPOWER_POWER_SEL);
#else
    clk_voltage_init(mode, SYSVDD_VOL_SEL_102V, VDC13_VOL_SEL_110V, TCFG_LOWPOWER_POWER_SEL);
#endif

#ifdef CONFIG_BOARD_AC696X_LIGHTER
    //only for br25 lighter
    clk_early_init(SYS_CLOCK_INPUT_BT_OSCX2, TCFG_CLOCK_OSC_HZ, 48000000);
#else
    clk_early_init(TCFG_CLOCK_SYS_SRC, TCFG_CLOCK_OSC_HZ, TCFG_CLOCK_SYS_HZ);
#endif

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

    power_sanity_check();

    /* log_info("resour est: %d", get_boot_flag()); */
    //set_boot_flag(99);
    /* log_info("resour est: %d", get_boot_flag()); */

    reset_source_dump();

    power_reset_src = power_reset_source_dump();

#if TCFG_USE_VIRTUAL_RTC
    vir_set_vm_id(VM_VIR_RTC_TIME, VM_VIR_ALM_TIME, VM_VIR_SUM_NSEC);
    get_lp_timer1_status();
#endif

    //Register debugger interrupt
    request_irq(0, 2, exception_irq_handler, 0);
    request_irq(1, 2, exception_irq_handler, 0);

    debug_init();

    sys_timer_init();

    /* sys_timer_add(NULL, timer, 10 * 1000); */


    __crc16_mutex_init();
}

/*-----------------------------------------------------------*/



/* --------------------------------------------------------------------------*/
/**
 * @brief 通过遍历链表获取当前已创建的任务
 */
/* ----------------------------------------------------------------------------*/
extern const char *pcTaskName(void *pxTCB);
struct list_head *tl_head = (struct list_head *)0x31df8;
struct task_list {
    struct list_head entry;
    void *task;
};
void task_name_loop(void)
{
    struct task_list *p;
    list_for_each_entry(p, tl_head, entry) {
        printf("task : %s", pcTaskName(p->task));
    }
}



