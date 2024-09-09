/**
 * @file uart.c
 * @author JL
 * @brief 串口UART模块C文件
 * @version 1.2
 * @date 2018-11-22
 */

#include "asm/clock.h"
#include "asm/uart_dev.h"
#include "asm/cpu.h"
#include "generic/gpio.h"
#include "spinlock.h"

#ifdef SUPPORT_MS_EXTENSIONS
#pragma bss_seg(".uart_bss")
#pragma data_seg(".uart_data")
#pragma const_seg(".uart_const")
#pragma code_seg(".uart_code")
#endif

#define UART_CLK  clk_get("uart")
#define UART_OT_CLK  clk_get("lsb")



static uart_bus_t uart0;
static uart_bus_t uart1;
static uart_bus_t uart2;

/* _WEAK_ */
/* extern */
const u32 CONFIG_UART0_ENABLE = 1;

/* _WEAK_ */
/* extern */
const u32 CONFIG_UART1_ENABLE  = 1;

/* _WEAK_ */
/* extern */
const u32 CONFIG_UART2_ENABLE  = 1;

/* _WEAK_ */
/* extern */
const u32 CONFIG_UART0_ENABLE_TX_DMA =  1;

/* _WEAK_ */
/* extern */
const u32 CONFIG_UART1_ENABLE_TX_DMA =  1;

/* _WEAK_ */
/* extern */
const u32 CONFIG_UART2_ENABLE_TX_DMA =  1;

static u32 kfifo_get(KFIFO *kfifo, u8 *buffer, u32 len)
{
    unsigned int i;
    len = MIN(len, kfifo->buf_in - kfifo->buf_out);

    i = MIN(len, kfifo->buf_size - (kfifo->buf_out & (kfifo->buf_size - 1)));

    memcpy(buffer, kfifo->buffer + (kfifo->buf_out & (kfifo->buf_size - 1)), i);

    memcpy(buffer + i, kfifo->buffer, len - i);

    kfifo->buf_out += len;
    return len;

}

static u32 kfifo_get_len_next(KFIFO *kfifo)
{
    u32 read_next = kfifo->buf_out & (kfifo->buf_size - 1);
    u32 write_next = kfifo->buf_in & (kfifo->buf_size - 1);
    u32 cnt = read_next > write_next ? kfifo->buf_size - read_next : write_next - read_next;
    return cnt;
}

static u32 kfifo_length(KFIFO *kfifo)
{
    return kfifo->buf_in - kfifo->buf_out;
}
/**
 * @brief ut0发送一个byte
 *
 * @param a 要发送的字节
 */
static void UT0_putbyte(char a)
{
    if (JL_UART0->CON0 & BIT(0)) {
        JL_UART0->BUF = a;
        __asm__ volatile("csync");
        while ((JL_UART0->CON0 & BIT(15)) == 0);
        JL_UART0->CON0 |= BIT(13);
    }
}
/**
 * @brief ut0接收一个byte
 *
 * @param buf 字节存放地址
 * @param timeout 接收超时时间，单位1ms
 * @return 返回0：接收失败；返回1：接收成功
 */
static u8 UT0_getbyte(u8 *buf, u32 timeout)
{
    u32 _timeout, _t_sleep;
    timeout = ut_msecs_to_jiffies(timeout);
    if (JL_UART0->CON0 & BIT(6)) {
        //DMA_MODE
        if (!kfifo_length(&uart0.kfifo)) {
            UT_OSSemPend(&uart0.sem_rx, timeout);
        }
        UT_OSSemSet(&uart0.sem_rx, 0);

        return kfifo_get(&uart0.kfifo, buf, 1);
    } else {
        _timeout = timeout + ut_get_jiffies();
        _t_sleep = ut_msecs_to_jiffies(10) + ut_get_jiffies();
        while (!(JL_UART0->CON0 & BIT(14))) {
            if (timeout && time_before(_timeout, ut_get_jiffies())) {
                return 0;
            }
            if (time_before(_t_sleep, ut_get_jiffies())) {
                ut_sleep();
                _t_sleep = ut_msecs_to_jiffies(10) + ut_get_jiffies();
            }
        }
        *buf = JL_UART0->BUF;
        JL_UART0->CON0 |= BIT(12);
        __asm__ volatile("csync");  //make RX_PND_CLR taking effect
    }
    return 1;
}
/**
 * @brief ut0中断函数
 */
SET_INTERRUPT
static void UT0_isr_fun(void)
{
    u32 rx_len = 0;
    if ((JL_UART0->CON0 & BIT(2)) && (JL_UART0->CON0 & BIT(15))) {
        JL_UART0->CON0 |= BIT(13);
        UT_OSSemPost(&uart0.sem_tx);
        if (uart0.isr_cbfun) {
            uart0.isr_cbfun(&uart0, UT_TX);
        }
    }
    if ((JL_UART0->CON0 & BIT(3)) && (JL_UART0->CON0 & BIT(14))) {
        JL_UART0->CON0 |= BIT(7);                     //DMA模式
        JL_UART0->CON0 |= BIT(12);           //清RX PND
        rx_len = JL_UART0->HRXCNT;             //读当前串口接收数据的个数
        /* uart0.kfifo.buf_in += uart0.frame_length; //每满frame_length字节则产生一次中断 */
        uart0.kfifo.buf_in += rx_len;
        UT_OSSemPost(&uart0.sem_rx);
        if (uart0.isr_cbfun) {
            uart0.isr_cbfun(&uart0, UT_RX);
        }
    }
    if ((JL_UART0->CON0 & BIT(5)) && (JL_UART0->CON0 & BIT(11))) {
        //OTCNT PND
        JL_UART0->CON0 |= BIT(7);                     //DMA模式
        JL_UART0->CON0 |= BIT(10);                    //清OTCNT PND
        JL_UART0->CON0 |= BIT(12);                    //清RX PND(这里的顺序不能改变，这里要清一次)
        rx_len = JL_UART0->HRXCNT;             //读当前串口接收数据的个数

        if (rx_len) {
            uart0.kfifo.buf_in += rx_len;
            /* printf("%s() %d\n", __func__, __LINE__); */
            UT_OSSemPost(&uart0.sem_rx);
            if (uart0.isr_cbfun) {
                uart0.isr_cbfun(&uart0, UT_RX_OT);
            }
        }
    }
}
/**
 * @brief ut0接收字符串
 *
 * @param buf 字符串存放首地址
 * @param len 预接收长度
 * @param timeout 接收超时时间，单位1ms
 * @return 返回实际接收长度
 */
static u32 UT0_read_buf(u8 *buf, u32 len, u32 timeout)
{
    u32 i;
    u32 _timeout, _t_sleep;
    if (len == 0) {
        return 0;
    }
    timeout = ut_msecs_to_jiffies(timeout);
    if (JL_UART0->CON0 & BIT(6)) {

        if (!kfifo_length(&uart0.kfifo)) {
            UT_OSSemPend(&uart0.sem_rx, timeout);
        }
        UT_OSSemSet(&uart0.sem_rx, 0);

        return kfifo_get(&uart0.kfifo, buf, len);
    } else {
        _timeout = timeout + ut_get_jiffies();
        _t_sleep = ut_msecs_to_jiffies(10) + ut_get_jiffies();
        for (i = 0; i < len; i++) {
            while (!(JL_UART0->CON0 & BIT(14))) {
                if (timeout && time_before(_timeout, ut_get_jiffies())) {
                    return i;
                }
                if (time_before(_t_sleep, ut_get_jiffies())) {
                    ut_sleep();
                    _t_sleep = ut_msecs_to_jiffies(10) + ut_get_jiffies();
                }
            }
            *(buf + i) = JL_UART0->BUF;
            JL_UART0->CON0 |= BIT(12);
            __asm__ volatile("csync");  //make RX_PND_CLR taking effect
        }
    }
    return len;
}
/**
 * @brief ut0发送字符串
 *
 * @param buf 字符串首地址
 * @param len 发送的字符串长度
 */
static void UT0_write_buf(const u8 *buf, u32 len)
{
    u32 i;
    if (len == 0) {
        return;
    }
    if (CONFIG_UART0_ENABLE_TX_DMA) {
        UT_OSSemSet(&uart0.sem_tx, 0);
        JL_UART0->CON0 |= BIT(13);
        JL_UART0->CON0 |= BIT(2);
        JL_UART0->TXADR = (u32)buf;
        JL_UART0->TXCNT = len;
        UT_OSSemPend(&uart0.sem_tx, 0);
        JL_UART0->CON0 &= ~BIT(2);
    } else {
        for (i = 0; i < len; i ++) {
            UT0_putbyte(*(buf + i));
        }
    }
}
/**
 * @brief ut0配置波特率
 *
 * @param baud 波特率值
 */
static void UT0_set_baud(u32 baud)
{
    JL_UART0->CON0 &= ~(BIT(0) | BIT(1));
    JL_UART0->CON0 |= BIT(13) | BIT(12) | BIT(10);
    JL_UART0->BAUD = ((UART_CLK + baud / 2) / baud) / 4 - 1;
    if (JL_UART0->CON0 & BIT(5)) {
        if (uart0.rx_timeout > 10) {
            JL_UART0->OTCNT = (uart0.rx_timeout / 10) * (UART_OT_CLK / 10) / 10;
        } else {
            JL_UART0->OTCNT = uart0.rx_timeout * UART_OT_CLK / 1000;
        }
    }
    JL_UART0->CON0 |= BIT(13) | BIT(12) | BIT(10) | BIT(0) | BIT(1);
}

/**
*获取串口数据长度
*/
static u32 uart1_get_data_len()
{
    return kfifo_length(&uart1.kfifo);
}

/**
 * @brief ut0使能
 */
static void UT0_open(u32 baud, u32 is_9bit, void *cbuf, u32 cbuf_size, u32 rx_cnt, u32 ot)
{
    JL_UART0->CON0 = BIT(13) | BIT(12) | BIT(10);
    UT_OSSemCreate(&uart0.sem_rx, 0);
    UT_OSSemCreate(&uart0.sem_tx, 0);
    request_irq(IRQ_UART0_IDX, 3, UT0_isr_fun, 0);
    if (cbuf) {
        uart0.kfifo.buffer = cbuf;
        uart0.kfifo.buf_size = cbuf_size;
        uart0.kfifo.buf_in = 0;
        uart0.kfifo.buf_out = 0;
        uart0.frame_length = rx_cnt;
        uart0.rx_timeout = ot;
        JL_UART0->RXSADR = (u32)uart0.kfifo.buffer;
        JL_UART0->RXEADR = (u32)(uart0.kfifo.buffer + uart0.kfifo.buf_size);
        JL_UART0->RXCNT = uart0.frame_length;
        JL_UART0->CON0 |= BIT(6) | BIT(5) | BIT(3);
    }
    if (is_9bit) {
        JL_UART0->CON2 |= BIT(0);
    } else {
        JL_UART0->CON2 &= ~BIT(0);
    }
    UT0_set_baud(baud);
}
/**
 * @brief ut0关闭，注销
 */
static void UT0_close(void)
{
    UT_OSSemClose(&uart0.sem_rx);
    UT_OSSemClose(&uart0.sem_tx);
    irq_disable(IRQ_UART0_IDX);
    JL_UART0->CON0 = BIT(13) | BIT(12) | BIT(10);
}

static u32 UT1_get_data_next_len(void)
{
    return kfifo_get_len_next(&uart1.kfifo);
}

/**
 * @brief ut1发送一个byte
 *
 * @param a 要发送的字节
 */
static void UT1_putbyte(char a)
{
    if (JL_UART1->CON0 & BIT(0)) {
        JL_UART1->BUF = a;
        __asm__ volatile("csync");
        while ((JL_UART1->CON0 & BIT(15)) == 0);
        JL_UART1->CON0 |= BIT(13);
    }
}
/**
 * @brief ut1接收一个byte
 *
 * @param buf 字节存放地址
 * @param timeout 接收超时时间，单位1ms
10ms¥；返回1：接收成功
 */
static u8 UT1_getbyte(u8 *buf, u32 timeout)
{
    u32 _timeout, _t_sleep;
    timeout = ut_msecs_to_jiffies(timeout);
    if (JL_UART1->CON0 & BIT(6)) {
        if (!kfifo_length(&uart1.kfifo)) {
            // UT_OSSemPend(&uart1.sem_rx, timeout);
        }
        // UT_OSSemSet(&uart1.sem_rx, 0);

        return kfifo_get(&uart1.kfifo, buf, 1);
    } else {
        _timeout = timeout + ut_get_jiffies();
        _t_sleep = ut_msecs_to_jiffies(10) + ut_get_jiffies();
        while (!(JL_UART1->CON0 & BIT(14))) {
            if (timeout && time_before(_timeout, ut_get_jiffies())) {
                return 0;
            }
            if (time_before(_t_sleep, ut_get_jiffies())) {
                ut_sleep();
                _t_sleep = ut_msecs_to_jiffies(10) + ut_get_jiffies();
            }
        }
        *buf = JL_UART1->BUF;
        JL_UART1->CON0 |= BIT(12);
        __asm__ volatile("csync");  //make RX_PND_CLR taking effect
    }
    return 1;
}
/**
 * @brief ut1中断函数
 */
SET_INTERRUPT
static void UT1_isr_fun(void)
{
    u32 rx_len = 0;
    if ((JL_UART1->CON0 & BIT(2)) && (JL_UART1->CON0 & BIT(15))) {
        JL_UART1->CON0 |= BIT(13);
        // UT_OSSemPost(&uart1.sem_tx);
        if (uart1.isr_cbfun) {
            uart1.isr_cbfun(&uart1, UT_TX);
        }
    }
    if ((JL_UART1->CON0 & BIT(3)) && (JL_UART1->CON0 & BIT(14))) {
        JL_UART1->CON0 |= BIT(7);                     //DMA模式
        JL_UART1->CON0 |= BIT(12);           //清RX PND
        __asm__ volatile("csync");
        rx_len = JL_UART1->HRXCNT;             //读当前串口接收数据的个数
        /* uart1.kfifo.buf_in += uart1.frame_length; //每满32字节则产生一次中断 */
        uart1.kfifo.buf_in += rx_len;
        // UT_OSSemPost(&uart1.sem_rx);
        if (uart1.isr_cbfun) {
            uart1.isr_cbfun(&uart1, UT_RX);
        }
    }

    if ((JL_UART1->CON0 & BIT(5)) && (JL_UART1->CON0 & BIT(11))) {
        //OTCNT PND
        JL_UART1->CON0 |= BIT(7);                     //DMA模式
        JL_UART1->CON0 |= BIT(10);                    //清OTCNT PND
        JL_UART1->CON0 |= BIT(12);                    //清RX PND(这里的顺序不能改变，这里要清一次)
        __asm__ volatile("csync");
        rx_len = JL_UART1->HRXCNT;             //读当前串口接收数据的个数

        if (rx_len) {
            uart1.kfifo.buf_in += rx_len;
            /* printf("%s() %d\n", __func__, __LINE__); */
            // UT_OSSemPost(&uart1.sem_rx);
            if (uart1.isr_cbfun) {
                uart1.isr_cbfun(&uart1, UT_RX_OT);
            }
        }
    }
}
/**
 * @brief ut1接收字符串
 *
 * @param buf 字符串存放首地址
 * @param len 预接收长度
 * @param timeout 接收超时时间，单位1ms
 * @return 返回实际接收的长度
 */
static u32 UT1_read_buf(u8 *buf, u32 len, u32 timeout)
{
    u32 i;
    u32 _timeout, _t_sleep;
    if (len == 0) {
        return 0;
    }
    timeout = ut_msecs_to_jiffies(timeout);
    if (JL_UART1->CON0 & BIT(6)) {
        if (!kfifo_length(&uart1.kfifo)) {
            // UT_OSSemPend(&uart1.sem_rx, timeout);
        }
        // UT_OSSemSet(&uart1.sem_rx, 0);

        return kfifo_get(&uart1.kfifo, buf, len);
    } else {
        _timeout = timeout + ut_get_jiffies();
        _t_sleep = ut_msecs_to_jiffies(10) + ut_get_jiffies();
        for (i = 0; i < len; i++) {
            while (!(JL_UART1->CON0 & BIT(14))) {
                if (timeout && time_before(_timeout, ut_get_jiffies())) {
                    return i;
                }
                if (time_before(_t_sleep, ut_get_jiffies())) {
                    ut_sleep();
                    _t_sleep = ut_msecs_to_jiffies(10) + ut_get_jiffies();
                }
            }
            *(buf + i) = JL_UART1->BUF;
            JL_UART1->CON0 |= BIT(12);
            __asm__ volatile("csync");  //make RX_PND_CLR taking effect
        }
    }
    return len;
}
/**
 * @brief ut1发送字符串
 *
 * @param buf 字符串首地址
 * @param len 发送的字符串长度
 * @param timeout 发送超时时间，单位10ms
 */
static void UT1_write_buf(const u8 *buf, u32 len)
{
    u32 i;
    if (len == 0) {
        return;
    }
    if (CONFIG_UART1_ENABLE_TX_DMA) {
        // UT_OSSemSet(&uart1.sem_tx, 0);
        JL_UART1->CON0 |= BIT(13);
        JL_UART1->CON0 |= BIT(2);
        JL_UART1->TXADR = (u32)buf;
        JL_UART1->TXCNT = len;
        // UT_OSSemPend(&uart1.sem_tx, 0);
        // JL_UART1->CON0 &= ~BIT(2);
    } else {
        for (i = 0; i < len; i ++) {
            UT1_putbyte(*(buf + i));
        }
    }
}
/**
 * @brief ut1配置波特率
 *
 * @param baud 波特率值
 */
static void UT1_set_baud(u32 baud)
{
    JL_UART1->CON0 &= ~(BIT(0) | BIT(1));
    JL_UART1->CON0 |= BIT(13) | BIT(12) | BIT(10);
#if 0
    JL_UART1->BAUD = ((UART_CLK + baud / 2) / baud) / 4 - 1;
#else
    JL_UART1->CON0 |= BIT(4);
    JL_UART1->BAUD = ((UART_CLK + baud / 2) / baud) / 3;
#endif
    if (JL_UART1->CON0 & BIT(5)) {
        if (uart1.rx_timeout > 10) {
            JL_UART1->OTCNT = (uart1.rx_timeout / 10) * (UART_OT_CLK / 10) / 10;
        } else {
            JL_UART1->OTCNT = uart1.rx_timeout * UART_OT_CLK / 1000;
        }
    }
    JL_UART1->CON0 |= BIT(13) | BIT(12) | BIT(10) | BIT(0) | BIT(1);
}
/**
 * @brief ut1使能
 */
static void UT1_open(u32 baud, u32 is_9bit, void *cbuf, u32 cbuf_size, u32 rx_cnt, u32 ot)
{
    JL_UART1->CON0 = BIT(13) | BIT(12) | BIT(10);
    JL_UART1->CON1 = BIT(14) | BIT(13); //暂无流控模式
    // UT_OSSemCreate(&uart1.sem_rx, 0);
    // UT_OSSemCreate(&uart1.sem_tx, 0);
    request_irq(IRQ_UART1_IDX, 3, UT1_isr_fun, 0);
    if (cbuf) {
        uart1.kfifo.buffer = cbuf;
        uart1.kfifo.buf_size = cbuf_size;
        uart1.kfifo.buf_in = 0;
        uart1.kfifo.buf_out = 0;
        uart1.frame_length = rx_cnt;
        uart1.rx_timeout = ot;
        JL_UART1->RXSADR = (u32)uart1.kfifo.buffer;
        JL_UART1->RXEADR = (u32)(uart1.kfifo.buffer + uart1.kfifo.buf_size);
        JL_UART1->RXCNT = uart1.frame_length;
        JL_UART1->CON0 |= BIT(6) | BIT(5) | BIT(3);
    }
    if (is_9bit) {
        JL_UART1->CON2 |= BIT(0);
    } else {
        JL_UART1->CON2 &= ~BIT(0);
    }
    UT1_set_baud(baud);
}
/**
 * @brief ut1关闭，注销
 */
static void UT1_close(void)
{
    // UT_OSSemClose(&uart1.sem_rx);
    // UT_OSSemClose(&uart1.sem_tx);
    irq_disable(IRQ_UART1_IDX);
    JL_UART1->CON0 = BIT(13) | BIT(12) | BIT(10);
}
/**
 * @brief ut2发送一个byte
 *
 * @param a 要发送的字节
 */
static void UT2_putbyte(char a)
{
    if (JL_UART2->CON0 & BIT(0)) {
        JL_UART2->BUF = a;
        __asm__ volatile("csync");
        while ((JL_UART2->CON0 & BIT(15)) == 0);
        JL_UART2->CON0 |= BIT(13);
    }
}
/**
 * @brief ut2接收一个byte
 *
 * @param buf 字节存放地址
 * @param timeout 接收超时时间，单位1ms
 * @return 返回0：接收失败；返回1：接收成功
 */
static u8 UT2_getbyte(u8 *buf, u32 timeout)
{
    u32 _timeout, _t_sleep;
    timeout = ut_msecs_to_jiffies(timeout);

    if (JL_UART2->CON0 & BIT(6)) {
        if (!kfifo_length(&uart2.kfifo)) {
            UT_OSSemPend(&uart2.sem_rx, timeout);
        }
        UT_OSSemSet(&uart2.sem_rx, 0);

        return kfifo_get(&uart2.kfifo, buf, 1);
    } else {
        _timeout = timeout + ut_get_jiffies();
        _t_sleep = ut_msecs_to_jiffies(10) + ut_get_jiffies();
        while (!(JL_UART2->CON0 & BIT(14))) {
            if (timeout && time_before(_timeout, ut_get_jiffies())) {
                return 0;
            }
            if (time_before(_t_sleep, ut_get_jiffies())) {
                ut_sleep();
                _t_sleep = ut_msecs_to_jiffies(10) + ut_get_jiffies();
            }
        }
        *buf = JL_UART2->BUF;
        JL_UART2->CON0 |= BIT(12);
        __asm__ volatile("csync");  //make RX_PND_CLR taking effect
    }

    return 1;
}
/**
 * @brief ut2中断函数
 */
SET_INTERRUPT
static void UT2_isr_fun(void)
{
    //JL_UART2->CON0 |= (BIT(13) | BIT(12) | BIT(10));
    u32 rx_len = 0;
    if ((JL_UART2->CON0 & BIT(2)) && (JL_UART2->CON0 & BIT(15))) {
        JL_UART2->CON0 |= BIT(13);
        UT_OSSemPost(&uart2.sem_tx);
        if (uart2.isr_cbfun) {
            uart2.isr_cbfun(&uart2, UT_TX);
        }
    }
    if ((JL_UART2->CON0 & BIT(3)) && (JL_UART2->CON0 & BIT(14))) {
        JL_UART2->CON0 |= BIT(7);                     //DMA模式
        JL_UART2->CON0 |= BIT(12);           //清RX PND
        rx_len = JL_UART2->HRXCNT;             //读当前串口接收数据的个数
        /* uart2.kfifo.buf_in += uart2.frame_length; //每满32字节则产生一次中断 */
        uart2.kfifo.buf_in += rx_len;
        UT_OSSemPost(&uart2.sem_rx);
        if (uart2.isr_cbfun) {
            uart2.isr_cbfun(&uart2, UT_RX);
        }
    }

    if ((JL_UART2->CON0 & BIT(5)) && (JL_UART2->CON0 & BIT(11))) {
        //OTCNT PND
        JL_UART2->CON0 |= BIT(7);                     //DMA模式
        JL_UART2->CON0 |= BIT(10);                    //清OTCNT PND
        JL_UART2->CON0 |= BIT(12);                    //清RX PND(这里的顺序不能改变，这里要清一次)
        rx_len = JL_UART2->HRXCNT;             //读当前串口接收数据的个数

        if (rx_len) {
            uart2.kfifo.buf_in += rx_len;
            /* printf("%s() %d\n", __func__, __LINE__); */
            UT_OSSemPost(&uart2.sem_rx);
            if (uart2.isr_cbfun) {
                uart2.isr_cbfun(&uart2, UT_RX_OT);
            }
        }
    }
}

/**
 * @brief ut1接收字符串
 *
 * @param buf 字符串存放首地址
 * @param len 预接收长度
 * @param timeout 接收超时时间，单位1ms
 * @return 返回实际接收的长度
 */
static u32 UT2_read_buf(u8 *buf, u32 len, u32 timeout)
{
    u32 i;
    u32 _timeout, _t_sleep;
    if (len == 0) {
        return 0;
    }

    timeout = ut_msecs_to_jiffies(timeout);
    if (JL_UART2->CON0 & BIT(6)) {
        if (!kfifo_length(&uart2.kfifo)) {
            UT_OSSemPend(&uart2.sem_rx, timeout);
        }
        UT_OSSemSet(&uart2.sem_rx, 0);
        return kfifo_get(&uart2.kfifo, buf, len);
    } else {
        _timeout = timeout + ut_get_jiffies();
        _t_sleep = ut_msecs_to_jiffies(10) + ut_get_jiffies();
        for (i = 0; i < len; i ++) {
            while (!(JL_UART2->CON0 & BIT(14))) {
                if (timeout && time_before(_timeout, ut_get_jiffies())) {
                    return i;
                }
                if (time_before(_t_sleep, ut_get_jiffies())) {
                    ut_sleep();
                    _t_sleep = ut_msecs_to_jiffies(10) + ut_get_jiffies();
                }
            }
            *(buf + i) = JL_UART2->BUF;
            JL_UART2->CON0 |= BIT(12);
            __asm__ volatile("csync");  //make RX_PND_CLR taking effect
        }
    }
    return len;
}
/**
 * @brief ut2发送字符串
 *
 * @param buf 字符串首地址
 * @param len 发送的字符串长度
 * @param timeout 发送超时时间，单位10ms
 */
static void UT2_write_buf(const u8 *buf, u32 len)
{
    u32 i;
    if (len == 0) {
        return;
    }
    if (CONFIG_UART2_ENABLE_TX_DMA) {
        UT_OSSemSet(&uart2.sem_tx, 0);
        JL_UART2->CON0 |= BIT(13);
        JL_UART2->CON0 |= BIT(2);
        JL_UART2->TXADR = (u32)buf;
        JL_UART2->TXCNT = len;
        UT_OSSemPend(&uart2.sem_tx, 0);
        JL_UART2->CON0 &= ~BIT(2);
    } else {
        for (i = 0; i < len; i++) {
            UT2_putbyte(*(buf + i));
        }
    }
}
/**
 * @brief ut2配置波特率
 *
 * @param baud 波特率值
 */
static void UT2_set_baud(u32 baud)
{
    JL_UART2->CON0 &= ~(BIT(0) | BIT(1));
    JL_UART2->CON0 |= BIT(13) | BIT(12) | BIT(10);
    JL_UART2->BAUD = ((UART_CLK + baud / 2) / baud) / 4 - 1;
    if (JL_UART2->CON0 & BIT(5)) {
        if (uart2.rx_timeout > 10) {
            JL_UART2->OTCNT = (uart2.rx_timeout / 10) * (UART_OT_CLK / 10) / 10;
        } else {
            JL_UART2->OTCNT = uart2.rx_timeout * UART_OT_CLK / 1000;
        }
    }
    JL_UART2->CON0 |= BIT(13) | BIT(12) | BIT(10) | BIT(0) | BIT(1);
}

/**
 * @brief ut2使能
 */
static void UT2_open(u32 baud, u32 is_9bit, void *cbuf, u32 cbuf_size, u32 rx_cnt, u32 ot)
{
    JL_UART2->CON0 = BIT(13) | BIT(12) | BIT(10);
    UT_OSSemCreate(&uart2.sem_rx, 0);
    UT_OSSemCreate(&uart2.sem_tx, 0);
    request_irq(IRQ_UART2_IDX, 3, UT2_isr_fun, 0);
    if (cbuf) {
        uart2.kfifo.buffer = cbuf;
        uart2.kfifo.buf_size = cbuf_size;
        uart2.kfifo.buf_in = 0;
        uart2.kfifo.buf_out = 0;
        uart2.frame_length = rx_cnt;
        uart2.rx_timeout = ot;
        JL_UART2->RXSADR = (u32)uart2.kfifo.buffer;
        JL_UART2->RXEADR = (u32)(uart2.kfifo.buffer + uart2.kfifo.buf_size);
        JL_UART2->RXCNT = uart2.frame_length;
        JL_UART2->CON0 |= BIT(6) | BIT(5) | BIT(3);
    }
    if (is_9bit) {
        JL_UART2->CON2 |= BIT(0);
    } else {
        JL_UART2->CON2 &= ~BIT(0);
    }
    UT2_set_baud(baud);
}

/**
 * @brief ut2关闭，注销
 */
static void UT2_close(void)
{
    UT_OSSemClose(&uart2.sem_rx);
    UT_OSSemClose(&uart2.sem_tx);
    irq_disable(IRQ_UART2_IDX);
    JL_UART2->CON0 = BIT(13) | BIT(12) | BIT(10);
}

static u32 uart_is_idle(u32 ut_num)
{
    switch (ut_num) {
    case 0 :
        return !(JL_UART0->CON0 & BIT(0));
    case 1 :
        return !(JL_UART1->CON0 & BIT(0));
    case 2:
        return !(JL_UART2->CON0 & BIT(0));
    default :
        break;
    }
    return 0;
}

static u8 uart_config(const struct uart_platform_data_t *arg, u8 tx_ch, enum PFI_TABLE rx_pfun)
{
    if (!(arg->tx_pin < IO_PORT_MAX || arg->rx_pin < IO_PORT_MAX)) {
        return -1;
    }

    if (arg->tx_pin < IO_PORT_MAX) {
        gpio_direction_output(arg->tx_pin, 1);
        gpio_set_fun_output_port(arg->tx_pin, tx_ch, 1, 1);
    }
    if (arg->rx_pin < IO_PORT_MAX) {
        gpio_direction_input(arg->rx_pin);
        gpio_set_pull_up(arg->rx_pin, 1);
        gpio_set_die(arg->rx_pin, 1);
        gpio_set_fun_input_port(arg->rx_pin, rx_pfun);
    }
    return 0;
}

/**
 * @brief ut模块初始const struct uart_platform_data_t *arg化函数，供外部调用
 *
 * @param arg 传入uartconst struct uart_platform_data_t *arg_argment型结构体指针
 * @return 返回uart_buconst struct uart_platform_data_t *args_t型结构体指针
 */
__attribute__((noinline))
const uart_bus_t *uart_dev_open(const struct uart_platform_data_t *arg)
{
    u8 ut_num;
    uart_bus_t *ut = NULL;
    u8 gpio_input_channle_flag = 0;

    if (uart_is_idle(0)) {
        ut_num = 0;
    } else if (uart_is_idle(1)) {
        ut_num = 1;
    } else if ((uart_is_idle(2))) {
        ut_num = 2;
    } else {
        return NULL;
    }
    if (arg->rx_cbuf) {
        if ((arg->rx_cbuf_size == 0) || (arg->rx_cbuf_size & (arg->rx_cbuf_size - 1))) {
            return NULL;
        }
    }

    if (CONFIG_UART0_ENABLE && ut_num == 0) {
        //gpio_set_uart0(ut_ch);
        if (uart_config(arg, FO_UART0_TX, PFI_UART0_RX)) {
            return NULL;
        }

        uart0.argv = arg->argv;
        uart0.isr_cbfun = arg->isr_cbfun;
        uart0.putbyte = UT0_putbyte;
        uart0.getbyte = UT0_getbyte;
        uart0.read = UT0_read_buf;
        uart0.write = UT0_write_buf;
        uart0.set_baud = UT0_set_baud;
        UT0_open(arg->baud, arg->is_9bit,
                 arg->rx_cbuf, arg->rx_cbuf_size,
                 arg->frame_length, arg->rx_timeout);
        ut = &uart0;
    } else if (CONFIG_UART1_ENABLE && ut_num == 1) {
        //gpio_set_uart1(ut_ch);
        if (uart_config(arg, FO_UART1_TX, PFI_UART1_RX)) {
            return NULL;
        }
        uart1.argv = arg->argv;
        uart1.isr_cbfun = arg->isr_cbfun;
        uart1.putbyte = UT1_putbyte;
        uart1.getbyte = UT1_getbyte;
        uart1.read    = UT1_read_buf;
        uart1.write   = UT1_write_buf;
        uart1.set_baud = UT1_set_baud;
        uart1.get_data_len = uart1_get_data_len;
        uart1.get_data_len_next = UT1_get_data_next_len;
        UT1_open(arg->baud, arg->is_9bit,
                 arg->rx_cbuf, arg->rx_cbuf_size,
                 arg->frame_length, arg->rx_timeout);
        ut = &uart1;
    } else if (CONFIG_UART2_ENABLE && ut_num == 2) {
        //gpio_set_uart2(ut_ch);
        if (uart_config(arg, FO_UART2_TX, PFI_UART2_RX)) {
            return NULL;
        }
        uart2.argv = arg->argv;
        uart2.isr_cbfun = arg->isr_cbfun;
        uart2.putbyte = UT2_putbyte;
        uart2.getbyte = UT2_getbyte;
        uart2.read    = UT2_read_buf;
        uart2.write   = UT2_write_buf;
        uart2.set_baud = UT2_set_baud;
        UT2_open(arg->baud, arg->is_9bit,
                 arg->rx_cbuf, arg->rx_cbuf_size,
                 arg->frame_length, arg->rx_timeout);
        ut = &uart2;
    } else {
        return NULL;
    }
    return ut;
}
u32 uart_dev_close(uart_bus_t *ut)
{
    UT_OSSemClose(&ut->sem_rx);
    UT_OSSemClose(&ut->sem_tx);
    if (&uart0 == ut) {
        UT0_close();
        return gpio_close_uart0();
    } else if (&uart1 == ut) {
        UT1_close();
        return gpio_close_uart1();
    } else {
        UT2_close();
        return gpio_close_uart2();
    }
    return 0;
}

void uart_disable_for_ota()
{
    JL_UART0->CON0 = BIT(13) | BIT(12) | BIT(10);
    JL_UART1->CON0 = BIT(13) | BIT(12) | BIT(10);
    JL_UART2->CON0 = BIT(13) | BIT(12) | BIT(10);
}

static u8 _rts_io = -1;
static u8 _cts_io = -1;
void uart1_flow_ctl_init(u8 rts_io, u8 cts_io)
{
    JL_UART1->CON1 = 0;
    //RTS
    if (rts_io < IO_PORT_MAX) {
        _rts_io = rts_io;
        gpio_set_die(rts_io, 1);
        gpio_set_direction(rts_io, 0);
        gpio_set_pull_up(rts_io, 0);
        gpio_set_pull_down(rts_io, 0);
        gpio_write(rts_io, 0);
        gpio_set_fun_output_port(rts_io, FO_UART1_RTS, 1, 1);
        JL_UART1->CON1 |= BIT(13) | BIT(0);
    }
    //CTS
    if (cts_io < IO_PORT_MAX) {
        _cts_io = cts_io;
        gpio_set_die(cts_io, 1);
        gpio_set_direction(cts_io, 1);
        gpio_set_pull_up(cts_io, 0);
        gpio_set_pull_down(cts_io, 1);
        gpio_set_fun_input_port(cts_io, PFI_UART1_CTS);
        JL_UART1->CON1 |= BIT(14) | BIT(2);
    }
}

void uart1_flow_ctl_rts_suspend(void)  //忙碌，硬件停止接收
{
    if (JL_UART1->CON1 & BIT(0)) {
        JL_UART1->CON1 |= BIT(4);  //RX_disable
        JL_UART1->CON1 |= BIT(1);  //RTS_DMAEN ///强制本地端RTS为高
    }

    if (!(JL_UART1->CON1 & BIT(0))) {
    gpio_write(_rts_io, 1);      //告诉对方，自己忙碌
    }
    JL_UART1->CON1 |= BIT(4);   //硬件停止接收
}

void uart1_flow_ctl_rts_resume(void) //空闲，表示可以继续接收数据
{
    if (JL_UART1->CON1 & BIT(0)) {
        JL_UART1->CON1 &= ~BIT(4); //RX_enable
        JL_UART1->CON1 &= ~BIT(1); //RTS_DMAEN  ///RTS可以硬件拉低
    }

    JL_UART1->CON1 &= ~BIT(4);  //硬件可以接收
    if (JL_UART1->CON1 & BIT(0)) {
    JL_UART1->CON1 |= BIT(13);
    } else {
    gpio_write(_rts_io, 0);      //表示可以继续接收数据
    }
}

static u8 rts_pin = -1;
static u8 cts_pin = -1;
void uart_tr_flow_ctl_init(u8 rts_io, u8 cts_io)
{
    // log_info("%s[rts:0x%x cts:0x%x]", __func__, rts_io, cts_io);
    JL_UART1->CON1 = 0;
    //RTS
    if (rts_io < IO_PORT_MAX) {
        rts_pin = rts_io;
        gpio_set_pull_up(rts_io, 0);
        gpio_set_pull_down(rts_io, 0);
        gpio_set_die(rts_io, 1);
        gpio_set_direction(rts_io, 0);
        /* gpio_write(rts_io, 0); */
        gpio_set_fun_output_port(rts_io, FO_UART1_RTS, 1, 1);
        JL_UART1->CON1 |= BIT(13) | BIT(0);
    }
    //CTS
    if (cts_io < IO_PORT_MAX) {
        cts_pin = cts_io;
        gpio_set_pull_up(cts_io, 1);
        gpio_set_pull_down(cts_io, 0);
        gpio_set_die(cts_io, 1);
        gpio_set_direction(cts_io, 1);
        gpio_set_fun_input_port(cts_io, PFI_UART1_CTS);
        JL_UART1->CON1 |= BIT(14) | BIT(2); // | BIT(3) CTS中断使能
    }
}

#if 0
/* 透传串口demo
*************************************************************************************
 * 一、固定使用uart1，TX/RX引脚任意IO
 * 二、低功耗串口注意点：
 * 1、低功耗2线串口，RX需要设置成唤醒口，数据包的包头要加73个0x00，才能保证数据包完整
 * 2、低功耗3线串口，使用独立IO来做唤醒口
 * 3、uart_tr_port_protect(port_group)，需要添加到close_gpio()
 * 4、uart_tr_port 需要添加到 wk_param
 * 5、不自动进入低功耗，UART_WAKEUP_DELAY设置为-1，需要自行调用uart_tr_suspend();进低功耗
 ************************************************************************************
 */
//RX TX可以配置任意IO口
#define UART_TR_TX_PIN                      IO_PORTB_04//IO_PORTA_00
#define UART_TR_RX_PIN                      IO_PORTB_05//IO_PORTA_01
#define UART_TR_BAUD                        921600//115200
//低功耗串口,及自动进入低功耗时间设置
#define UART_TR_LOWPOWER_MODE               false // true //
#define UART_WAKEUP_DELAY                   -1 // 100  //unit:2ms
//模式：单字节还是DMA
#define UART_BYTE_ONE                       0
#define UART_BYTE_DMA                       1
#define UART_BYTE_MODE                      UART_BYTE_DMA
//DMA模式参数
#define UART_DMA_TIMEOUT                    6   //unit:ms
#define UART_DMA_MAX_LEN                    0x100 //unit:Byte //单包最大长度
//单字节模式：才能开启校验位，奇校验还是偶校验
#define UART_PARITY_CHECK_NO                0//不用校验
#define UART_PARITY_CHECK_EVEN              1//奇
#define UART_PARITY_CHECK_ODD               2//偶
#define UART_PARITY_CHECK_MARK              3//校验位始终为1
#define UART_PARITY_CHECK_SPACE             4//校验位始终为0
#define UART_PARITY_CHECK_MODE              UART_PARITY_CHECK_NO
//串口流控功能，固定IO
#define UART_RTS_CTS_MODE                   true//false //true //
#define UART_TR_RTS_PIN                     IO_PORTA_02
#define UART_TR_CTS_PIN                     IO_PORTA_01

#include "board_config.h"
#include "system/includes.h"
#include "asm/power_interface.h"
#include "app_config.h"
#undef _DEBUG_H_
#define LOG_TAG_CONST       UART_TR
#define LOG_TAG             "[UART_TR]"
#include "debug.h"
const char log_tag_const_v_UART_TR AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_UART_TR AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_d_UART_TR AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_UART_TR AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_UART_TR AT(.LOG_TAG_CONST) = 1;

#define UART_TR_BUF_LEN     ((UART_DMA_MAX_LEN + 16) * 5) //最大缓存5包
static u8 uart_tr_wbuf[UART_DMA_MAX_LEN] __attribute__((aligned(4)));//线程发送
static u8 uart_tr_rbuf[UART_DMA_MAX_LEN] __attribute__((aligned(4)));//线程读取
static u8 uart_tr_ibuf[UART_DMA_MAX_LEN * 4] __attribute__((aligned(4)));//中断使用
static u8 uart_tr_cbuf[UART_TR_BUF_LEN] __attribute__((aligned(4)));//缓存使用,可以扩大几倍
static const uart_bus_t *uart_bus = NULL;
static volatile bool uart_tr_busy = 0;

struct port_wakeup uart_tr_port = {
    .pullup_down_enable = ENABLE,
    .edge               = FALLING_EDGE,
    .both_edge          = 0,
    .iomap              = UART_TR_RX_PIN,
    .filter             = PORT_FLT_NULL, //PORT_FLT_256us, //
};

void uart_tr_port_protect(u16 *port_group)
{
    u32 port;
    port = UART_TR_TX_PIN;
    port_group[port / IO_GROUP_NUM] &= ~BIT(port % IO_GROUP_NUM);
    port = UART_TR_RX_PIN;
    port_group[port / IO_GROUP_NUM] &= ~BIT(port % IO_GROUP_NUM);
}

static volatile u8 uart_tr_idle = UART_TR_LOWPOWER_MODE; //置1可进睡眠,置0不可进睡眠
static u8 uart_tr_idle_query(void)
{
    if(uart_tr_idle){
        return 1;
    }else{
        return 0;
    }
}
REGISTER_LP_TARGET(uart_tr_lp_target) = {
    .name = "uart_tr_lp",
    .is_idle = uart_tr_idle_query,
};

static int uart_tr_idle_delay = 0;

void uart_tr_suspend(void)
{
    log_debug("%s", __func__);
    JL_UART1->CON0 &= ~(BIT(0)|BIT(1));
#if 0
    /* usb_iomode(1); */
    gpio_set_pull_up(UART_TR_TX_PIN, 0);
    gpio_set_pull_down(UART_TR_TX_PIN, 0);
    gpio_set_direction(UART_TR_TX_PIN, 1);
    gpio_set_die(UART_TR_TX_PIN, 0);
    gpio_set_dieh(UART_TR_TX_PIN, 0);

    gpio_set_pull_up(UART_TR_RX_PIN, 1); //RX为唤醒口,需要上拉
    gpio_set_pull_down(UART_TR_RX_PIN, 0);
    gpio_set_direction(UART_TR_RX_PIN, 1);
    gpio_set_die(UART_TR_RX_PIN, 1);
    gpio_set_dieh(UART_TR_RX_PIN, 1);
#endif
    power_wakeup_enable_with_port(UART_TR_RX_PIN);
    uart_tr_idle = 1;
}
void uart_tr_resume(void)
{
    log_debug("%s", __func__);
    uart_tr_idle = 0;
    uart_tr_idle_delay = UART_WAKEUP_DELAY;
    power_wakeup_disable_with_port(UART_TR_RX_PIN);
#if 0
    gpio_direction_output(UART_TR_TX_PIN, 1);
    gpio_set_die(UART_TR_TX_PIN, 1);
    gpio_set_fun_output_port(UART_TR_TX_PIN, FO_UART1_TX, 1, 1);

    gpio_direction_input(UART_TR_RX_PIN);
    gpio_set_pull_up(UART_TR_RX_PIN, 1);
    gpio_set_die(UART_TR_RX_PIN, 1);
    gpio_set_fun_input_port(UART_TR_RX_PIN, PFI_UART1_RX);
#endif
    JL_UART1->CON0 |= (BIT(0)|BIT(1));
    JL_UART1->CON0 |= (BIT(10)|BIT(12)|BIT(13));
}

static void uart_tr_auto_idle(void *priv)
{
    if(uart_tr_idle_delay > 0){
        uart_tr_idle_delay--;
        if(uart_tr_idle_delay == 0){
            uart_tr_idle = UART_TR_LOWPOWER_MODE;
            if(UART_TR_LOWPOWER_MODE){
                uart_tr_suspend();
            }
        }
    }
}
//UART_BYTE_ONE模式需要判断满一帧的条件
static int uart_tr_one_frame(u8 *ibuf, u8 len)
{
    log_debug("%s[%d]", __func__, len);
    //演示例子,16B为一帧
    if(len == 16){
        put_buf(ibuf, len);
        return 1;
    }else{
        return 0;
    }
}
#define UART_TR_SEM              true
static OS_SEM uart_tr_sem;
#define DEVICE_EVENT_FROM_UTR   (('U' << 24) | ('T' << 16) | ('R' << 8) | '\0')
static void uart_tr_send_event(void)
{
#if UART_TR_SEM
    int ret = OS_NO_ERR;
    ret = os_sem_post(&uart_tr_sem);
    if(ret) log_error("%s[ret:%d]", __func__, ret);
    return;
#else
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = (void *)DEVICE_EVENT_FROM_UTR;
    e.u.dev.event = DEVICE_EVENT_CHANGE;
    e.u.dev.value = 0;
    sys_event_notify(&e);
#endif
}

///cbuf缓存多包串口中断接收数据
cbuffer_t uart_tr_cbuft;
#define cbuf_get_space(a) (a)->total_len
typedef struct {
    u8   uart_id;
    u32  rx_len;
}__attribute__((packed)) uart_tr_head_t;

static int uart_tr_cbuf_is_write_able(u32 len)
{
    u32 wlen, buf_space;
    /* OS_ENTER_CRITICAL(); */
    /* cbuf_is_write_able() */
    buf_space = cbuf_get_space(&uart_tr_cbuft) - cbuf_get_data_size(&uart_tr_cbuft);
    if (buf_space < len) {
        wlen = 0;
    }else {
        wlen = len;
    }
    /* OS_EXIT_CRITICAL(); */
    log_debug("%s[%d %d %d]", __func__, len, buf_space, wlen);
    return wlen;
}
static int uart_tr_ibuf_to_cbuf(u8 *buf, u32 len)
{
    int ret = 0;
    u16 wlen = 0;
    u16 head_size = sizeof(uart_tr_head_t);
    uart_tr_head_t rx_head;
    rx_head.uart_id = 1; //use uart1
    OS_ENTER_CRITICAL();
    if (uart_tr_cbuf_is_write_able(len + head_size)) {
        rx_head.rx_len = len;
        wlen = cbuf_write(&uart_tr_cbuft, &rx_head, head_size);
        wlen += cbuf_write(&uart_tr_cbuft, buf, rx_head.rx_len);
        if (wlen != (rx_head.rx_len + head_size)) {
            ret = -1;
        } else {
            ret = wlen;
        }
    }else{
        ret = -2;
    }
    OS_EXIT_CRITICAL();
    log_debug("%s[%d %d %d]", __func__, len, head_size, ret);
    return ret;
}
static int uart_tr_cbuf_read(u8 *buf, u32 *len)
{
    int ret = 0;
    u16 rlen = 0;
    u16 head_size = sizeof(uart_tr_head_t);
    uart_tr_head_t rx_head;
    OS_ENTER_CRITICAL();
    if (0 == cbuf_get_data_size(&uart_tr_cbuft)) {
        /* OS_EXIT_CRITICAL(); */
        ret =  -1;
    }else{
        rlen = cbuf_read(&uart_tr_cbuft, &rx_head, sizeof(uart_tr_head_t));
        if(rlen != sizeof(uart_tr_head_t)){
            ret =  -2;
        }
        else if(rx_head.rx_len) {
            rlen = cbuf_read(&uart_tr_cbuft, buf, rx_head.rx_len);
            if(rlen != rx_head.rx_len){
                ret =  -3;
            }else{
                *len = rx_head.rx_len;
            }
        }
    }
    OS_EXIT_CRITICAL();
    log_debug("%s[%d 0x%x %d]", __func__, ret, len, rx_head.rx_len);
    return ret;
}

//流控CTS/RTS
///Request To Send 输出口,"请求发送"数据,低电平有效,低电平说明本设备可以接收数据
///Clear To Send 输入口,"发送允许",低电平有效,低电平说明本设备可以向对方发送数据
//AP与MODEM的流控这样通信的：
//AP串口可用时，将AP-RTS拉低，MODEM-CTS检测到AP-RTS为低，知道AP串口已准备好，可以发送数据；
//AP串口不可用时,将AP-RTS拉高，MODEM-CTS检测到AP-RTS为高，知道AP串口还未准备好,就不会放数据
//默认启动时：
//AP的   CTS为高  RTS为低
//MODEM的CTS为高  RTS为低
//默认休眠时
//MODEM的CTS为高  RTS为高
//做蓝牙模块一般做modem
static u8 rts_pin = -1;
static u8 cts_pin = -1;
void uart_tr_flow_ctl_init(u8 rts_io, u8 cts_io)
{
    log_info("%s[rts:0x%x cts:0x%x]", __func__, rts_io, cts_io);
    JL_UART1->CON1 = 0;
    //RTS
    if (rts_io < IO_PORT_MAX) {
        rts_pin = rts_io;
        gpio_set_pull_up(rts_io, 0);
        gpio_set_pull_down(rts_io, 0);
        gpio_set_die(rts_io, 1);
        gpio_set_direction(rts_io, 0);
        /* gpio_write(rts_io, 0); */
        gpio_set_fun_output_port(rts_io, FO_UART1_RTS, 1, 1);
        JL_UART1->CON1 |= BIT(13) | BIT(0);
    }
    //CTS
    if (cts_io < IO_PORT_MAX) {
        cts_pin = cts_io;
        gpio_set_pull_up(cts_io, 1);
        gpio_set_pull_down(cts_io, 0);
        gpio_set_die(cts_io, 1);
        gpio_set_direction(cts_io, 1);
        gpio_set_fun_input_port(cts_io, PFI_UART1_CTS);
        JL_UART1->CON1 |= BIT(14) | BIT(2); // | BIT(3) CTS中断使能
    }
}
static void uart_tr_flow_ctl_close(void)
{
    JL_UART1->CON1 = 0;
    JL_UART1->CON1 |= (BIT(13)|BIT(14));
}
static int get_cts_state(void)  //返回 0:可以发, 1:不能发
{
    if (JL_UART1->CON1 & BIT(2)){
        log_info("%s[pin:0x%x gpio_read:%d]", __func__, cts_pin, gpio_read(cts_pin));
        return gpio_read(cts_pin);
    }else{
        return 0;
    }
}
static void rts_set_state(u8 state) // 0:可以收, 1:不能收
{
    log_info("%s[state:%d]", __func__, state);
    if (JL_UART1->CON1 & BIT(0))
    {
        if (state) {
            JL_UART1->CON1 |= BIT(4);  //RX_disable
            JL_UART1->CON1 |= BIT(1);  //RTS_DMAEN ///强制本地端RTS为高
        } else {
            JL_UART1->CON1 &= ~BIT(4); //RX_enable
            JL_UART1->CON1 &= ~BIT(1); //RTS_DMAEN  ///RTS可以硬件拉低
        }
    }
}

//奇校验:计算每个字节比特位是1的个数,
//如果是偶数个1,第九个位设为1,如果是奇数个1,那么就把第九个位设为0,
//这样连续9个字节比特位为1的位数肯定是奇数。
//偶校验类似
AT_VOLATILE_RAM_CODE
static int uart_tr_parity_check(u8 dir, u8 buf)//dir:0接收,1发送
{
    bool odd = false;
    bool rb8 = (JL_UART1->CON2 & BIT(2)) ? true : false;
    if((JL_UART1->CON2 & BIT(0)) == 0){
        return 0;
    }
    log_debug("%s[%s 0x%x]", __func__, dir ? "send" : "receive", buf);
    u8 sr = 0,i = 0;
    for(i = 0; i < 8; i++){
        if(buf & BIT(i))  sr++;
    }
    if(sr%2 == 0) odd = true; //buf的位指为1的个数是否为偶数
    log_debug("buf sr:%d, parity odd bit:%d, even bit:%d", sr, odd, !odd);
    if(dir == 1){
        if (UART_PARITY_CHECK_MODE == UART_PARITY_CHECK_EVEN){ //奇校验
            if(odd) JL_UART1->CON2 &= ~BIT(1);
            else    JL_UART1->CON2 |=  BIT(1);
        }else if (UART_PARITY_CHECK_MODE == UART_PARITY_CHECK_ODD){//偶校验
            if(odd) JL_UART1->CON2 |=  BIT(1);
            else    JL_UART1->CON2 &= ~BIT(1);
        }else if (UART_PARITY_CHECK_MODE == UART_PARITY_CHECK_MARK){
            JL_UART1->CON2 |=  BIT(1);
        }else if (UART_PARITY_CHECK_MODE == UART_PARITY_CHECK_SPACE){
            JL_UART1->CON2 &= ~BIT(1);
        }
    }else{
        log_debug("register parity bit:%d", rb8);
        if (UART_PARITY_CHECK_MODE == UART_PARITY_CHECK_EVEN){ //奇校验
            if(!odd != rb8){
                log_error("%s -> parity check even fail", __func__);
                return -1;
            }
        }else if (UART_PARITY_CHECK_MODE == UART_PARITY_CHECK_ODD){//偶校验
            if(odd != rb8){
                log_error("%s -> parity check odd fail", __func__);
                return -1;
            }
        }else if (UART_PARITY_CHECK_MODE == UART_PARITY_CHECK_MARK){
            if(rb8 == false){
                log_error("%s -> parity check mark fail", __func__);
                return -1;
            }
        }else if (UART_PARITY_CHECK_MODE == UART_PARITY_CHECK_SPACE){
            if(rb8 == true){
                log_error("%s -> parity check space fail", __func__);
                return -1;
            }
        }
    }
    return 0;
}

SET_INTERRUPT
AT_VOLATILE_RAM_CODE
static void uart_tr_isr(void)
{
    log_debug("%s[0x%02x 0x%02x 0x%02x]", __func__,
           JL_UART1->CON0, JL_UART1->CON1, JL_UART1->CON2);
    /* log_debug("%s[%d]", __func__, uart1.frame_length); */
    uart_tr_idle_delay = UART_WAKEUP_DELAY;
    u32 rx_len = 0;
    u8 buf = 0;
    uart_bus_t *ubus = &uart1;
    if ((JL_UART1->CON0 & BIT(2)) && (JL_UART1->CON0 & BIT(15))) {
        JL_UART1->CON0 |= BIT(13);
        if (uart1.isr_cbfun) {
            uart1.isr_cbfun(ubus, UT_TX);
        }
    }
#if 0
    if ((JL_UART1->CON0 & BIT(3)) && (JL_UART1->CON0 & BIT(14))) {
        JL_UART1->CON0 |= BIT(7);
        JL_UART1->CON0 |= BIT(12);
        __asm__ volatile("csync");
        if(uart1.frame_length){ //UART_BYTE_DMA
            rx_len = JL_UART1->HRXCNT;
            JL_UART1->RXSADR = (u32)uart1.kfifo.buffer;
            JL_UART1->RXCNT = uart1.frame_length;
        }else{ //UART_BYTE_ONE
            buf = JL_UART1->BUF;
            uart1.kfifo.buffer[uart1.kfifo.buf_in] = buf;
            rx_len = 1;
            if(uart_tr_parity_check(0, buf)){
                rx_len = 0; //奇偶校验失败
            }
        }
        if (rx_len) {
            uart1.kfifo.buf_in += rx_len;
            if (uart1.isr_cbfun) {
                uart1.isr_cbfun(ubus, UT_RX);
            }
            if(uart1.frame_length){
                uart1.kfifo.buf_in = 0;
            }
        }
    }
    if ((JL_UART1->CON0 & BIT(5)) && (JL_UART1->CON0 & BIT(11))) {
        JL_UART1->CON0 |= BIT(7);
        JL_UART1->CON0 |= BIT(10);
        JL_UART1->CON0 |= BIT(12);
        __asm__ volatile("csync");
        rx_len = JL_UART1->HRXCNT;
        JL_UART1->RXSADR = (u32)uart1.kfifo.buffer;
        JL_UART1->RXCNT = uart1.frame_length;
        if (rx_len) {
            uart1.kfifo.buf_in += rx_len;
            if (uart1.isr_cbfun) {
                uart1.isr_cbfun(ubus, UT_RX_OT);
            }
            if(uart1.frame_length){
                uart1.kfifo.buf_in = 0;
            }
        }
    }
#else
    if ((JL_UART1->CON0 & BIT(3)) && (JL_UART1->CON0 & BIT(14))) {
        JL_UART1->CON0 |= BIT(7);                     //DMA模式
        JL_UART1->CON0 |= BIT(12);           //清RX PND
        rx_len = JL_UART1->HRXCNT;             //读当前串口接收数据的个数
        /* uart1.kfifo.buf_in += uart1.frame_length; //每满32字节则产生一次中断 */
        uart1.kfifo.buf_in += rx_len;
        if (uart1.isr_cbfun) {
            uart1.isr_cbfun(&uart1, UT_RX);
        }
    }

    if ((JL_UART1->CON0 & BIT(5)) && (JL_UART1->CON0 & BIT(11))) {
        //OTCNT PND
        JL_UART1->CON0 |= BIT(7);                     //DMA模式
        JL_UART1->CON0 |= BIT(10);                    //清OTCNT PND
        JL_UART1->CON0 |= BIT(12);                    //清RX PND(这里的顺序不能改变，这里要清一次)
        rx_len = JL_UART1->HRXCNT;             //读当前串口接收数据的个数

        if (rx_len) {
            uart1.kfifo.buf_in += rx_len;
            /* printf("%s() %d\n", __func__, __LINE__); */
            if (uart1.isr_cbfun) {
                uart1.isr_cbfun(&uart1, UT_RX_OT);
            }
        }
    }
#endif
}

AT_VOLATILE_RAM_CODE
static void uart_tr_isr_hook(void *arg, u32 status)
{
    uart_bus_t *ubus = arg;
    u32 len = 0;
    int ret = 0;
    static u32 total_len = 0;
    log_debug("%s[%d %d] %d", __func__, status, uart1.kfifo.buf_in, total_len);
    if(status == UT_TX){
        /* JL_UART1->CON0 &= ~BIT(2); //关闭TX中断允许 */
        uart_tr_busy = 0;
        return;
    }
    else if(status == UT_RX){
        /* put_buf(uart1.kfifo.buffer, uart1.kfifo.buf_in); */
    }
    else if(status == UT_RX_OT){
        /* put_buf(uart1.kfifo.buffer, uart1.kfifo.buf_in); */
    }
    if(uart1.frame_length == 0){
        //UART_BYTE_ONE 需要加处理判断满一帧,再转存
        ret = uart_tr_one_frame(uart1.kfifo.buffer, uart1.kfifo.buf_in);
        if(ret == 0){
            return;
        }
    }
    // ret = uart_tr_ibuf_to_cbuf(uart1.kfifo.buffer, uart1.kfifo.buf_in);
    // log_debug("%s[%d]", __func__, status, ret);
    uart1_flow_ctl_rts_suspend();
    len = kfifo_get_len_next(&uart1.kfifo);
    extern int8_t send_data(const uint8_t *data, int len, int flush);
    send_data(&uart1.kfifo.buffer[uart1.kfifo.buf_out & (uart1.kfifo.buf_size - 1)], len, 0);
    uart1.kfifo.buf_out += len;
    total_len += len;
    log_debug("total_len %d", total_len);

    if(ret < 0){
        log_error("%s[%s]", __func__, "uart_tr_ibuf_to_cbuf faild");
        put_buf(uart1.kfifo.buffer, uart1.kfifo.buf_in);
    }
    // uart_tr_send_event();

    // if((ret > 0)&&(uart1.frame_length == 0)){
    //     uart1.kfifo.buf_in = 0;
    // }
}

static void uart_tr_open(u32 baud, u32 is_9bit, void *ibuf, u32 ibuf_size,
                         u32 rx_cnt, u32 ot)
{
    uart1.kfifo.buffer = ibuf;
    uart1.kfifo.buf_size = ibuf_size;
    uart1.kfifo.buf_in = 0;
    uart1.kfifo.buf_out = 0;
    uart1.frame_length = rx_cnt;
    uart1.rx_timeout = ot;
    JL_UART1->CON0 = BIT(13) | BIT(12) | BIT(10);
    JL_UART1->CON1 = BIT(14) | BIT(13); //暂无流控模式
    request_irq(IRQ_UART1_IDX, 3, uart_tr_isr, 0);//可自行调整优先级
    if (rx_cnt) { //UART_BYTE_DMA
        JL_UART1->RXSADR = (u32)uart1.kfifo.buffer;
        JL_UART1->RXEADR = (u32)(uart1.kfifo.buffer + uart1.kfifo.buf_size);
        JL_UART1->RXCNT = uart1.frame_length;
        JL_UART1->CON0 |= BIT(6) | BIT(5) | BIT(3);//UART_BYTE_DMA
    }else{
        JL_UART1->CON0 |= BIT(3);//UART_BYTE_ONE
    }
    if (is_9bit) {
        JL_UART1->CON2 |= BIT(0);
    } else {
        JL_UART1->CON2 &= ~BIT(0);
    }
    UT1_set_baud(baud);
}
const uart_bus_t *uart_tr_dev_open(const struct uart_platform_data_t *arg)
{
    u8 ut_num = 1;//固定使用uart1
    if ((!uart_is_idle(ut_num))||(!CONFIG_UART1_ENABLE)) {
        return NULL;
    }
    if ((arg->rx_cbuf == NULL) || (arg->rx_cbuf_size == 0)) {
        return NULL;
    }
    if (uart_config(arg, FO_UART1_TX, PFI_UART1_RX)) {
        return NULL;
    }
    uart1.argv = arg->argv;
    uart1.isr_cbfun = arg->isr_cbfun;
//    uart1.putbyte = UT1_putbyte;
//    uart1.getbyte = UT1_getbyte;
//    uart1.read    = UT1_read_buf;
//    uart1.write   = UT1_write_buf;
    /* uart1.set_baud = UT1_set_baud; */
    uart_tr_open(arg->baud, arg->is_9bit,
                 arg->rx_cbuf, arg->rx_cbuf_size,
                 arg->frame_length, arg->rx_timeout);
    return &uart1;
}
static void uart_tr_task(void *p);
static int uart_tr_task_init(void)
{
    int ret = OS_NO_ERR;
    os_sem_create(&uart_tr_sem, 0);
    os_sem_set(&uart_tr_sem, 0);
    /* ret = task_create(uart_tr_task, NULL, "uart_tr"); */
    ret = os_task_create(uart_tr_task, NULL, 31, 512, 0, "uart_tr");
    if(ret != OS_NO_ERR) log_error("%s %s create fail 0x%x", __func__, "uart_tr", ret);
    return ret;
}

int uart_tr_init(void)
{
    log_info("%s[0x%02x 0x%02x]", __func__, UART_TR_TX_PIN, UART_TR_RX_PIN);
    struct uart_platform_data_t uart_arg = {0};
    uart_arg.tx_pin = UART_TR_TX_PIN;
    uart_arg.rx_pin = UART_TR_RX_PIN;
    uart_arg.rx_cbuf = uart_tr_ibuf;
    uart_arg.rx_cbuf_size = sizeof(uart_tr_ibuf);
    uart_arg.isr_cbfun = uart_tr_isr_hook;
    uart_arg.baud = UART_TR_BAUD;
    uart_arg.rx_timeout = UART_DMA_TIMEOUT;//单位ms
    if (UART_PARITY_CHECK_MODE){//奇偶校验不能用DMA
        uart_arg.is_9bit = 1;
        uart_arg.frame_length = 0;//UART_BYTE_ONE
    }else{
        uart_arg.is_9bit = 0;
        if(UART_BYTE_MODE == UART_BYTE_DMA){
            uart_arg.frame_length = UART_DMA_MAX_LEN; //UART_BYTE_DMA //max:sizeof(uart_tr_ibuf);
        }else{
            uart_arg.frame_length = 0; //UART_BYTE_ONE
        }
    }
    uart_bus = uart_tr_dev_open(&uart_arg);
#if UART_RTS_CTS_MODE
    uart_tr_flow_ctl_init(UART_TR_RTS_PIN, UART_TR_CTS_PIN);
    JL_UART1->CON1 |= BIT(13) | BIT(0);
    JL_UART1->CON1 |= BIT(14) | BIT(2);
#endif
#if UART_TR_SEM
    // uart_tr_task_init();
#endif
    log_info("%s[uart_tr_dev_open 0x%x]\n", __func__, uart_bus);
    // cbuf_init(&uart_tr_cbuft, uart_tr_cbuf, sizeof(uart_tr_cbuf));
    uart_tr_idle = 0;
    uart_tr_idle_delay = UART_WAKEUP_DELAY;
    // sys_s_hi_timer_add(NULL, uart_tr_auto_idle, 2);
    uart_tr_busy = 0;
    return (int)uart_bus; //返回NULL,即初始化失败
}

int uart_tr_send_data(u8 *data, u32 len)
{
    log_info("%s[%d]", __func__, len);
    u32 cnt = 0;
    u8 buf = 0;
    if(data == NULL) return -1;
    if(uart_tr_busy) return -2;
    /* if(get_cts_state()) return -3; */
    if(uart_tr_idle) uart_tr_resume();
    log_debug_hexdump(data, len);
    if(UART_BYTE_MODE == UART_BYTE_DMA){
        uart_tr_busy = 1;
        memcpy(uart_tr_wbuf, data, len);
        /* JL_UART1->CON0 &= ~BIT(1); //关闭接收使能 */
        JL_UART1->CON0 |=  BIT(13);
        JL_UART1->CON0 |=  BIT(2);
        JL_UART1->TXADR = (u32)uart_tr_wbuf;
        JL_UART1->TXCNT = len;
        /* while((JL_UART1->CON0 & BIT(2)) == 0);//等待TX中断允许关闭 */
        /* JL_UART1->CON0 |=  BIT(1);//开启接收使能 */
    }else{
        while(cnt < len){
            buf = data[cnt];
            uart_tr_parity_check(1, buf);
            JL_UART1->BUF = buf;
            __asm__ volatile("csync");
            while ((JL_UART1->CON0 & BIT(15)) == 0);
            JL_UART1->CON0 |= BIT(13);
            cnt++;
        }
    }
    return 0;
}

static void uart_tr_task(void *p)
{
    int ret = 0;
    u32 rlen = 0;
    static u32 r_len_total = 0;
    while (1) {
        clr_wdt();
        ret = os_sem_pend(&uart_tr_sem, 0);
        /* os_sem_set(&case_task_sem, 0); */
        log_debug("%s[os_sem_pend -> ret:%d]", __func__, ret);
        rlen = 0;
        ret = uart_tr_cbuf_read(uart_tr_rbuf, &rlen);//栈变量取值可能会死机
        if(ret < 0) continue;
        r_len_total += rlen;
        log_info("%s[%d %d %u]", __func__, ret, rlen, r_len_total);
        log_info_hexdump(uart_tr_rbuf, rlen);
        ///可以在这里添加数据处理
        /* os_time_dly(10); //模拟耗时 */
        /* uart_tr_send_data(uart_tr_rbuf, rlen);//回显测试 */
    }
}

//如果串口数据处理会耗时,则不能在uart_tr_isr_hook,每一包都发一个事件出来
static void uart_tr_event_handler(struct sys_event *e)
{
    int ret = 0;
    u32 rlen = 0;
    if ((u32)e->arg == DEVICE_EVENT_FROM_UTR) {
        while(1){
            clr_wdt();
            ret = uart_tr_cbuf_read(uart_tr_rbuf, &rlen);//栈变量取值可能会死机
            if(ret < 0) continue;
            log_info("%s[%d %d]", __func__, ret, rlen);
            log_info_hexdump(uart_tr_rbuf, rlen);
            ///可以在这里添加数据处理
            /* os_time_dly(10); //模拟耗时 */
            /* uart_tr_send_data(uart_tr_rbuf, rlen);//回显测试 */
        }
    }
}
SYS_EVENT_HANDLER(SYS_DEVICE_EVENT, uart_tr_event_handler, 0);

///测试验证函数
static void uart_tr_test_loop(void *priv)
{
    static u8 data[16];
    static u8 cnt = 0;
//    if(cnt > 10){
//        uart_tr_suspend();
//        return;
//    }
    for(u8 i = 0; i < sizeof(data); i++){
        data[i] = i;
    }
    data[0] = cnt++;
    log_info("%s[cnt:0x%x]", __func__, cnt);
    uart_tr_send_data(data, sizeof(data));
//    static u8 flag = 0;
//    rts_set_state(flag);
//    flag = !flag;
}
void uart_tr_test(void)
{
    log_info("%s", __func__);
    uart_tr_init();
    /* sys_hi_timer_add(NULL, uart_tr_test_loop, 4); */
    // sys_timer_add(NULL, uart_tr_test_loop, 2 * 1000);
}
#endif

