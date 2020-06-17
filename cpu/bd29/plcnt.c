#include "includes.h"
#include "asm/plcnt.h"

#define INPUT_CHANNLE2_SRC_SEL(x)		SFR(JL_IOMAP->CON2, 16, 6, x)

#define PLCNT_CON0_RESET				(JL_PCNT->CON = 0)
#define IOMC1							(JL_IOMAP->CON1)
#define IOMC2							(JL_IOMAP->CON2)

#define PLCNT_CLOCK_SEL(x)				SFR(JL_PCNT->CON, 2, 2, x)

#define PLCNT_EN 						JL_PCNT->CON |= BIT(1)
#define PLCNT_DIS 						JL_PCNT->CON &= ~BIT(1)

#define PLCNT_TEST_MODE_EN 				JL_PCNT->CON |= BIT(0)
#define PLCNT_TEST_MODE_DIS 			JL_PCNT->CON &= ~BIT(0)

#define PLCNT_VALUE						JL_PCNT->VAL

#define PLCNT_DEBUG 		0

#if PLCNT_DEBUG
#define plcnt_debug(fmt, ...) printf("[PLCNT] "fmt, ##__VA_ARGS__)
#else
#define plcnt_debug(fmt, ...)
#endif


static void touch_key_log_info()
{
    plcnt_debug("PLCNT_CON = 0x%x", JL_PCNT->CON);
    plcnt_debug("IOMC1 = 0x%x", JL_IOMAP->CON1);
    plcnt_debug("IOMC2 = 0x%x", JL_IOMAP->CON2);
    plcnt_debug("IOMC3 = 0x%x", JL_IOMAP->CON3);
    plcnt_debug("IOMC4 = 0x%x", JL_IOMAP->CON4);
}

void plcnt_io_init(u8 port)
{
    //INPUT_CHANNLE2_SRC_SEL(port);
    gpio_set_die(port, 1);
    gpio_set_pull_down(port, 0);
    gpio_set_pull_up(port, 0);
    gpio_direction_output(port, 1); //输出1
}

void plcnt_clk_init(u8 clk)
{
    PLCNT_CON0_RESET;
    PLCNT_CLOCK_SEL(clk);

    IOMC1 &= ~BIT(6); //input channel2边沿选择,0: 上升沿, 1: 下降沿
    //IOMC1 |= BIT(6); //input channel2边沿选择,0: 上升沿, 1: 下降沿

    PLCNT_DIS;
    PLCNT_TEST_MODE_DIS;
}

u32 plcnt_delta_cnt_get(u8 port)
{
    u32 value_ori = 0;
    u32 value_cur = 0;
    volatile u32 port_value = 0;
    u32 delta = 0;

    INPUT_CHANNLE2_SRC_SEL(port);
    value_ori = PLCNT_VALUE;

    PLCNT_EN;		//cnt计数三个条件, cnt_EN-->capture_io为高-->io为输入
    port_value = 1;

    gpio_set_pull_down(port, 1);  //下拉由'1'-->'0'
    gpio_direction_input(port);  //开始放电, 并触发计数
    while (port_value) {
        port_value = gpio_read(port);  //下拉放电, 由"1" -> "0"
    }
    //IO放电到为"低", cnt自动停止计数
    value_cur = PLCNT_VALUE;
    PLCNT_DIS;

    if (value_cur > value_ori) {
        delta = value_cur - value_ori;
    } else {
        delta = 0;  //实际上很难溢出
    }

    gpio_set_pull_down(port, 0); //关闭下拉
    gpio_direction_output(port, 1); //输出1, 恢复高电平

    return delta;
}



