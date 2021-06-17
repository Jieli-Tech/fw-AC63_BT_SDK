#include "app_config.h"
#include "generic/typedef.h"
#include "generic/gpio.h"
#include "asm/gpio.h"
#include "asm/clock.h"
#include "device/chargebox.h"

#if(TCFG_CHARGE_BOX_ENABLE)

#define BUF_LEN     (64)

struct chargebox_handle {
    const struct chargebox_platform_data *data;
    JL_UART_TypeDef *UART;
    u32 uart_baud;
    u32 input_chl;
    u32 output_chl;
    u32 ut_chl;
    u8 init_flag;
    volatile u8 lr_flag;
    volatile u8 rx_index;
};

static u8 uart_buf[BUF_LEN] __attribute__((aligned(4)));
#define __this  (&hdl)
static struct chargebox_handle hdl;

extern void printf_buf(u8 *buf, u32 len);
extern void chargebox_data_deal(u8 cmd, u8 l_r, u8 *data, u8 length);
void __attribute__((weak)) chargebox_data_deal(u8 cmd, u8 l_r, u8 *data, u8 len)
{

}

___interrupt
static void uart_isr(void)
{
    u32 rx_len, timeout;

    if ((__this->UART->CON0 & BIT(2)) && (__this->UART->CON0 & BIT(15))) {//发送完成
        __this->UART->CON0 |= BIT(13);
        chargebox_data_deal(CMD_COMPLETE, __this->lr_flag, NULL, 0);
    }
    if ((__this->UART->CON0 & BIT(3)) && (__this->UART->CON0 & BIT(14))) {//接收中断
        __this->UART->CON0 |= BIT(12);
        uart_buf[__this->rx_index] = __this->UART->BUF;
        __this->rx_index++;
        chargebox_data_deal(CMD_RECVBYTE, __this->lr_flag, &uart_buf[__this->rx_index - 1], 1);
    }
    if ((__this->UART->CON0 & BIT(5)) && (__this->UART->CON0 & BIT(11))) {
        //OTCNT PND
        __this->UART->CON0 |= BIT(7);//DMA模式
        __this->UART->CON0 |= BIT(10);//清OTCNT PND
        asm volatile("nop");
        rx_len = __this->UART->HRXCNT;//读当前串口接收数据的个数
        __this->UART->CON0 |= BIT(12);//清RX PND(这里的顺序不能改变，这里要清一次)
        rx_len = __this->rx_index;
        timeout = 20 * 1000000 /  __this->uart_baud;
        __this->UART->OTCNT = timeout * (clk_get("uart") / 1000000);
        chargebox_data_deal(CMD_RECVDATA, __this->lr_flag, uart_buf, rx_len);
        __this->rx_index = 0;
    }
}

u8 chargebox_write(u8 l_r, u8 *data, u8 len)
{
    if ((len == 0) || (len > BUF_LEN)) {
        return 0;
    }
    __this->lr_flag = l_r;
    memcpy(uart_buf, data, len);
    __this->UART->TXADR = (u32)uart_buf;
    __this->UART->TXCNT = (u16)len;
    return len;
}

void chargebox_open(u8 l_r, u8 mode)
{
    u32 io_port;
    if (l_r ==  EAR_L) {
        io_port = __this->data->L_port;
    } else {
        io_port = __this->data->R_port;
    }
    __this->lr_flag = l_r;
    if (mode == MODE_RECVDATA) {
        __this->UART->CON0 = BIT(13) | BIT(12) | BIT(10);
        gpio_uart_rx_input(io_port, __this->ut_chl, __this->input_chl);
        gpio_set_pull_up(io_port, 1);
        __this->rx_index = 0;
        __this->UART->CON0 |= BIT(5) | BIT(3);
        __this->UART->CON0 |= BIT(13) | BIT(12) | BIT(10) | BIT(0);
    } else {
        __this->UART->CON0 = BIT(13) | BIT(12) | BIT(10);
        gpio_output_channle(io_port, __this->output_chl);
        gpio_set_hd(io_port, 0);
        gpio_set_hd0(io_port, 0);
        __this->UART->CON0 = BIT(13) | BIT(12) | BIT(10) | BIT(2) | BIT(0);
    }
}

void chargebox_close(u8 l_r)
{
    u32 io_port;
    if (l_r ==  EAR_L) {
        io_port = __this->data->L_port;
    } else {
        io_port = __this->data->R_port;
    }
    __this->UART->CON0 = BIT(13) | BIT(12) | BIT(10);
    gpio_set_pull_down(io_port, 0);
    gpio_set_pull_up(io_port, 0);
    gpio_set_die(io_port, 1);
    gpio_set_hd(io_port, 0);
    gpio_direction_output(io_port, 1);
}

void chargebox_set_baud(u8 l_r, u32 baudrate)
{
    u32 uart_timeout;
    __this->uart_baud = baudrate;
    uart_timeout = 20 * 1000000 / __this->uart_baud;
    __this->UART->OTCNT = uart_timeout * (clk_get("lsb") / 1000000);
    __this->UART->BAUD = (clk_get("uart") / __this->uart_baud) / 4 - 1;
}


void chargebox_init(const struct chargebox_platform_data *data)
{
    u32 uart_timeout;
    __this->data = (struct chargebox_platform_data *)data;
    ASSERT(data);
    if (!(JL_UART0->CON0 & BIT(0))) {
        JL_UART0->CON0 = BIT(13) | BIT(12) | BIT(10);
        request_irq(IRQ_UART0_IDX, 2, uart_isr, 0);
        __this->UART = JL_UART0;
        __this->input_chl = INPUT_CH0;
        __this->output_chl = CH0_UT0_TX;
        __this->ut_chl = 0;
        gpio_set_uart0(-1);
    } else if (!(JL_UART1->CON0 & BIT(0))) {
        JL_UART1->CON0 = BIT(13) | BIT(12) | BIT(10);
        request_irq(IRQ_UART1_IDX, 2, uart_isr, 0);
        __this->UART = JL_UART1;
        __this->input_chl = INPUT_CH3;
        __this->output_chl = CH1_UT1_TX;
        __this->ut_chl = 1;
        gpio_set_uart1(-1);
    } else if (!(JL_UART2->CON0 & BIT(0))) {
        JL_UART2->CON0 = BIT(13) | BIT(12) | BIT(10);
        request_irq(IRQ_UART2_IDX, 2, uart_isr, 0);
        __this->UART = JL_UART2;
        __this->input_chl = INPUT_CH2;
        __this->output_chl = CH2_UT2_TX;
        __this->ut_chl = 2;
        gpio_set_uart2(-1);
    } else {
        ASSERT(0, "uart all used!\n");
    }

    uart_timeout = 20 * 1000000 / __this->data->baudrate;
    __this->uart_baud = __this->data->baudrate;
    __this->UART->CON0 = 0;
    __this->UART->CON0 = BIT(13) | BIT(12) | BIT(10) | BIT(0);
    __this->UART->OTCNT = uart_timeout * (clk_get("lsb") / 1000000);
    __this->UART->BAUD = (clk_get("uart") / __this->data->baudrate) / 4 - 1;
    gpio_set_pull_down(__this->data->L_port, 0);
    gpio_set_pull_up(__this->data->L_port, 0);
    gpio_set_die(__this->data->L_port, 1);
    gpio_direction_input(__this->data->L_port);
    gpio_set_pull_down(__this->data->R_port, 0);
    gpio_set_pull_up(__this->data->R_port, 0);
    gpio_set_die(__this->data->R_port, 1);
    gpio_direction_input(__this->data->R_port);
    __this->init_flag = 1;
}

static void clock_critical_enter(void)
{

}
static void clock_critical_exit(void)
{
    u32 uart_timeout;
    if (!__this->init_flag) {
        return;
    }
    uart_timeout = 20 * 1000000 / __this->uart_baud;
    __this->UART->OTCNT = uart_timeout * (clk_get("lsb") / 1000000);
    __this->UART->BAUD = (clk_get("uart") / __this->uart_baud) / 4 - 1;
}
CLOCK_CRITICAL_HANDLE_REG(chargebox, clock_critical_enter, clock_critical_exit)

#endif//end if TCFG_CHARGE_BOX_ENABLE
