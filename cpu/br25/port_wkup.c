#include "typedef.h"
#include "irq.h"
#include "asm/gpio.h"

/**
 * 注意：JL_WAKEUP 区别于PMU管理的唤醒。可理解为一个独立的模块使用。但在低功耗的情况下，中断无效。
 */

static void (*port_wkup_irq_cbfun)(void) = NULL;
static u32 user_port = -1;;
/**
 * @brief 引脚中断函数
 */
___interrupt
void port_wkup_irq_fun(void)
{
    if (JL_WAKEUP->CON3 & BIT(0)) {
        JL_WAKEUP->CON2 |= BIT(0);
        if (port_wkup_irq_cbfun) {
            port_wkup_irq_cbfun();
        }
    }
}
/**
 * @brief 引脚中断初始化
 * @param port 任意引脚的引脚号：IO_PORTA_00......
 * @param trigger_edge 触发边沿。 0：上升沿触发。 1；下降沿触发
 * @param cbfun 相应的中断回调函数
 */
void port_wkup_interrupt_init(u32 port, u8 trigger_edge, void (*cbfun)(void))
{
    JL_WAKEUP->CON0 &= ~BIT(0);
    gpio_set_die(port, 1);
    gpio_set_direction(port, 1);
    if (trigger_edge == 0) {
        JL_WAKEUP->CON1 &= ~BIT(0);
        gpio_set_pull_up(port, 0);
        gpio_set_pull_down(port, 1);
    } else {
        JL_WAKEUP->CON1 |= BIT(0);
        gpio_set_pull_up(port, 1);
        gpio_set_pull_down(port, 0);
    }
    user_port = port;
    if (cbfun) {
        port_wkup_irq_cbfun = cbfun;
    }
    request_irq(IRQ_PORT_IDX, 3, port_wkup_irq_fun, 0); //注册中断函数
    JL_IOMAP->CON2 &= ~(0b111111 << 0);                 //使用inputchannel 0
    JL_IOMAP->CON2 |= (port << 0);
    JL_WAKEUP->CON2 |= BIT(0);                          //清一次pnd
    JL_WAKEUP->CON0 |= BIT(0);                          //引脚中断使能
}

/**
 * @brief 关掉该引脚的中断功能
 * @param port 引脚号：IO_PORTA_00......
 */
void port_wkup_interrupt_close(u32 port)
{
    JL_WAKEUP->CON0 &= ~BIT(0);
    if (port == user_port) {
        gpio_set_die(port, 0);
        gpio_set_direction(port, 1);
        gpio_set_pull_up(port, 0);
        gpio_set_pull_down(port, 0);
    }
}

/*********************************************************************************************************
 * ******************************           使用举例如下           ***************************************
 * ******************************************************************************************************/
void port_irq_cbfun(void)
{
    printf("Hello world !\n");
}
void my_port_wkup_test()
{
    port_wkup_interrupt_init(IO_PORTA_03, 0, port_irq_cbfun);//上升沿触发
    /* port_wkup_interrupt_init(IO_PORT_DP, 1, port_irq_cbfun);//下降沿触发 */
    while (1);
}

