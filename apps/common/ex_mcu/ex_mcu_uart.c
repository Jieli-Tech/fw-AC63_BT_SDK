#include "ex_mcu_uart.h"


#if TCFG_EX_MCU_ENABLE

#define LOG_TAG_CONST       EX_MCU_UART
#define LOG_TAG             "[ex_mcu_uart]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


//=======================================================================//
//                        模块操作部分                                   //
//=======================================================================//
static struct ex_mcu_uart_handle ex_mcu_uart_hdl;
#define __this  (&ex_mcu_uart_hdl)

//标志进出推代码流程，用于屏蔽串口中断
static u8 ex_mcu_enter_flag = 0;
//=======================================================================//
//                          串口收发部分                                 //
//=======================================================================//

volatile u8 send_busy;

//串口时钟和串口超时时钟是分开的
#define UART_SRC_CLK    clk_get("uart")
#define UART_OT_CLK     clk_get("lsb")

//=======================================================================//
//                       复位操作部分                                    //
//=======================================================================//
u8 ucEx_mcu_reset(void)
{
    return true;
}
//=======================================================================//
//                        文件系统操作部分                               //
//=======================================================================//
void *vEx_mcu_file_open(const char *file_path)
{
    u32 err = 0;
    FILE *pvfile = 0;

    pvfile = fopen(file_path, "r");

    log_debug("pvfile:0x%x\n", pvfile);

    if (pvfile == NULL) {
        log_error("%s:%d, vfs open %s err\n", __func__, __LINE__, file_path);
        return -ENOENT;
    }

    //获取EX_MCU_FILE的cache地址
    struct vfs_attr code_attr = {0};
    if (fget_attrs(pvfile, &code_attr) != 0) {
        log_error("%s:%d, get_attrs %s err\n", __func__, __LINE__, file_path);
        return -ENOENT;
    }

    __this->file_cpu_acess_begin = boot_info.sfc.app_addr + code_attr.sclust;

    log_info("attr.attr = 0x%x, attr.fsize = 0x%x, attr.sclust = 0x%x, cpu_acess_begin = 0x%x\n", code_attr.attr, code_attr.fsize, code_attr.sclust, __this->file_cpu_acess_begin);

    fclose(pvfile);
    //返回一个flash地址变量指针进行读操作
    return (void *)&__this->file_cpu_acess_begin;
}


u32 ulEx_mcu_file_read(void *file, void *buf, u32 len)
{
    if (buf == NULL) {
        log_error("%s:%d, need read buf is null\n", __func__, __LINE__);
        return 0;
    }

    log_debug("%s:%d, file: 0x%x\n", __func__, __LINE__, (u8 *)(*(u32 *)(file)));

//void*先转换为32位指针，再取32位指针内容得到flash地址
    memcpy(buf, (u8 *)(*(u32 *)(file)), len);

    return len;
}

u32 ulEx_mcu_file_seek(void *file, u32 offset)
{
    *(u32 *)file = *(u32 *)file + offset;

    log_debug("%s:%d, file: 0x%x\n", __func__, __LINE__, (*(u32 *)(file)));

    return offset;
}

//=======================================================================//
//                      带os串口收发操作部分                               //
//=======================================================================//
#if TCFG_EX_MCU_OS_ENABLE

//OS调度使用lsb时钟源进行定时,未添加外部接口
static void udelay(u32 usec)
{
    JL_TIMER0->CON = BIT(14);
    JL_TIMER0->CNT = 0;
    JL_TIMER0->PRD = clk_get("lsb") / 1000000L  * usec;//1us
    JL_TIMER0->CON = BIT(0); //lsb clk
    while ((JL_TIMER0->CON & BIT(15)) == 0);
    JL_TIMER0->CON = BIT(14);
}

___interrupt
static void ex_mcu_uart_isr(void)
{
    if ((__this->UART->CON0 & BIT(2)) && (__this->UART->CON0 & BIT(15))) {

        __this->UART->CON0 |= BIT(13);
        send_busy = 0;

//*****************接收部分***************
        //发送成功,把引脚拉低设为接收模式
        gpio_direction_input(__this->data->io_port);

        //开启接收功能
        __this->UART->CON1 &= ~BIT(4);
        if (__this->UART == JL_UART0) {
            gpio_set_fun_input_port(__this->data->io_port, PFI_UART0_RX);
        } else if (__this->UART == JL_UART1) {
            gpio_set_fun_input_port(__this->data->io_port, PFI_UART1_RX);
        } else {
            gpio_set_fun_input_port(__this->data->io_port, PFI_UART2_RX);
        }

        __this->UART->CON0 = BIT(13) | BIT(12) | BIT(10);
        __this->UART->CON0 |= BIT(6) | BIT(3) | BIT(1) | BIT(0);
        log_debug("%s:%d,tx mode to rx\n", __func__, __LINE__);

    }

//*****************接收中断***************
    if ((__this->UART->CON0 & BIT(3)) && (__this->UART->CON0 & BIT(14))) {
        //接收成功
        __this->UART->CON0 |= BIT(7);//DMA模式
        __this->UART->CON0 |= BIT(12) ;
        __this->UART->CON0 |= BIT(10) ;

        /* log_debug("%s:%d,rx cnt:%d\n", __func__, __LINE__, __this->UART->HRXCNT); */

    }

    //屏蔽串口中断
    if (ex_mcu_enter_flag == 1) {
        //发送和接收都发信号量
        os_sem_post(__this->data->sem_tx_rx);
    }
}


u8 ucEx_mcu_tx_rx_init(const struct ex_mcu_platform_data *data)
{
    u32 uart_timeout;

    __this->data = data;
    //动态申请内存空间，注意exit要释放
    __this->data->ex_mcu_buf = (u8 *)malloc(EX_MCU_APP_BUF_SIZE);

    ASSERT(data);
    if (!(JL_UART0->CON0 & BIT(0))) {
        JL_UART0->CON0 = BIT(13) | BIT(12) | BIT(10);
        request_irq(IRQ_UART0_IDX, 2, ex_mcu_uart_isr, 0);
        __this->UART = JL_UART0;
        log_info("Ex_mcu using JL_UART0\n");
    } else if (!(JL_UART1->CON0 & BIT(0))) {
        JL_UART1->CON0 = BIT(13) | BIT(12) | BIT(10);
        request_irq(IRQ_UART1_IDX, 2, ex_mcu_uart_isr, 0);
        __this->UART = JL_UART1;
        log_info("Ex_mcu using JL_UART1\n");
    } else if (!(JL_UART2->CON0 & BIT(0))) {
        JL_UART2->CON0 = BIT(13) | BIT(12) | BIT(10);
        request_irq(IRQ_UART2_IDX, 2, ex_mcu_uart_isr, 0);
        __this->UART = JL_UART2;
        log_info("Ex_mcu using JL_UART2\n");
    } else {
        ASSERT(0, "uart all used!\n");
    }

    send_busy = 0;

    uart_timeout = 20 * 1000000 / __this->data->hand_baudrate;
// 6 rx读dma  3 rx中断 2 tx中断 1 tx使能 0tx使能
    __this->UART->CON0 = BIT(13) | BIT(12) | BIT(6) | BIT(10) |  BIT(3) | BIT(2) | BIT(1) | BIT(0); //占用该串口,不被其他模块使用
    /* __this->UART->OTCNT = uart_timeout * (UART_OT_CLK / 1000000); */
    __this->UART->BAUD = (UART_SRC_CLK / __this->data->hand_baudrate) / 4 - 1;

    __this->baudrate = __this->data->hand_baudrate;
    gpio_set_pull_down(__this->data->io_port, 0);
    gpio_set_pull_up(__this->data->io_port, 1);
    gpio_set_die(__this->data->io_port, 1);
    gpio_direction_input(__this->data->io_port);

    return true;
}

u32 vEx_mcu_uart_tx_buf_client(u8 *uart_dma_buf, u32 dma_buf_len)
{

    __this->UART->CON0 |= BIT(13) | BIT(12) | BIT(10) | BIT(0);//清空TX Pending/RX Pending/OTPND打开发送使能
    gpio_direction_output(__this->data->io_port, 1);

    __this->UART->CON1 |= BIT(4);
    if (__this->UART == JL_UART0) {
        gpio_set_fun_output_port(__this->data->io_port, FO_UART0_TX, 1, 1);
    } else if (__this->UART == JL_UART1) {
        gpio_set_fun_output_port(__this->data->io_port, FO_UART1_TX, 1, 1);
    } else {
        gpio_set_fun_output_port(__this->data->io_port, FO_UART2_TX, 1, 1);

    }

    u32 data_addr = (u32)uart_dma_buf;
    if (data_addr % 4) {//4byte对齐
        ASSERT(0, "%s: unaligned accesses!", __func__);
    }
    send_busy = 1;
    __this->UART->TXADR = data_addr;
    __this->UART->TXCNT = dma_buf_len;

    while (!(__this->UART->CON0 & BIT(15)));

    send_busy = 0;
    __this->UART->CON0 |= BIT(13);

    gpio_direction_input(__this->data->io_port);
}

void vEx_mcu_uart_tx_buf(u8 *uart_dma_buf, u32 dma_buf_len)
{
    __this->UART->CON0 = BIT(13) | BIT(12) | BIT(10);
    gpio_direction_output(__this->data->io_port, 1);

    //关闭接收功能
    __this->UART->CON1 |= BIT(4);
    if (__this->UART == JL_UART0) {
        gpio_set_fun_output_port(__this->data->io_port, FO_UART0_TX, 1, 1);
    } else if (__this->UART == JL_UART1) {
        gpio_set_fun_output_port(__this->data->io_port, FO_UART1_TX, 1, 1);
    } else {
        gpio_set_fun_output_port(__this->data->io_port, FO_UART2_TX, 1, 1);
    }
    u32 data_addr = (u32)uart_dma_buf;
    if (data_addr % 4) {//4byte对齐
        ASSERT(0, "%s: unaligned accesses!", __func__);
    }
    //打开发送中断
    __this->UART->CON0 = BIT(2) | BIT(0);

    __this->UART->TXADR = data_addr;
    __this->UART->TXCNT = dma_buf_len;

    send_busy = 1;
}

//初始化内存
u8 ucEx_mcu_uart_rx_init(u8 *uart_dma_buf, u32 dma_buf_len/* , u32 timeout*/)
{
    memset((void *)uart_dma_buf, 0, sizeof(uart_dma_buf));
    __this->UART->RXSADR = (u32)uart_dma_buf;
    __this->UART->RXEADR = (u32)((((u32)uart_dma_buf + dma_buf_len) + 3) / 4 * 4);
    __this->UART->RXCNT = dma_buf_len;

    return true;
}

u32 ulEx_mcu_uart_rx_buf_client(u8 *uart_dma_buf, u32 dma_buf_len, u32 timeout)
{
    JL_PORTA->OUT |= BIT(2);
    u32 ret = 0;
    __this->UART->CON0 = BIT(13) | BIT(12) | BIT(10);

    gpio_direction_input(__this->data->io_port);
    gpio_set_die(__this->data->io_port, 1);

    __this->UART->CON1 &= ~BIT(4);
    if (__this->UART == JL_UART0) {
        gpio_set_fun_input_port(__this->data->io_port, PFI_UART0_RX);
    } else if (__this->UART == JL_UART1) {
        gpio_set_fun_input_port(__this->data->io_port, PFI_UART1_RX);
    } else {
        gpio_set_fun_input_port(__this->data->io_port, PFI_UART2_RX);
    }

    /* memset((void *)uart_dma_buf, 0, sizeof(uart_dma_buf)); */
    __this->UART->RXSADR = (u32)uart_dma_buf;
    __this->UART->RXEADR = (u32)((((u32)uart_dma_buf + dma_buf_len) + 3) / 4 * 4);
    __this->UART->RXCNT = dma_buf_len;

    __this->UART->CON0 |= BIT(6);
    __this->UART->CON0 |= BIT(13) | BIT(12) | BIT(10) | BIT(1);
    //轮询检查是否收到内容
    //TODO
    /* printf("__this->UART->CON0 & (BIT(14)) = %d",__this->UART->CON0 & (BIT(14)) ); */

    /* timeout = 1000; */
    /* __asm__ volatile("csync"); */
    /* while (timeout--) { */
    /* __asm__ volatile("csync"); */
    /* if ((__this->UART->CON0 & BIT(14))) { */
    /* [> log_debug("CAAAAAAAAAC\n"); <] */
    /* ret = dma_buf_len; */
    /* break; */
    /* } */
    /* //1ms */
    /* //log_debug("\n%d\n",timeout); */
    /* } */

    /* //time_out == 0 意味等待时间超时 */
    /* if (timeout == 0) */
    /* { */
    /* log_debug("error\n"); */
    /* return -1; */
    /* } */

    timeout = 1000;
    while (timeout && ((__this->UART->CON0 & BIT(14)) == 0)) {
        timeout--;
        __asm__ volatile("csync");
        /* printf("CAAAAAAAAAC\n");  */
        /* wdt_clear(); */
    }
    __this->UART->CON0 |= BIT(12) | BIT(10);
    udelay(200);
    /* printf_buf((u8 *)uart_dma_buf, dma_buf_len); */
    ret = dma_buf_len;
    return ret;
}

u32 ulEx_mcu_uart_rx_buf(void)
{
    //读当前串口接收数据的个数
    return __this->UART->HRXCNT;
}

//目前没有用到
void vEx_mcu_os_delay(unsigned int t)
{
    os_time_dly(t);
}

#else
//=======================================================================//
//                      裸板串口操作部分                                 //
//=======================================================================//

void vEx_mcu_udelay(u32 usec)
{
    //可在此处添加裸板的自定义代码，需尽量保证延时为10ms，对应到总函数接口的retry_timeout,单位10ms

    JL_TIMER0->CON = BIT(14);
    JL_TIMER0->CNT = 0;
    JL_TIMER0->PRD = clk_get("timer") / 100L  * usec; //10ms
    JL_TIMER0->CON = BIT(0) | BIT(2) | BIT(6); //sys clk
    //针对串口中断进行修改，一有串口消息就退出
    while ((JL_TIMER0->CON & BIT(15)) == 0) {
        //延时的时候起串口中断就退出
        if (__this->UART->CON0 & BIT(14)) {
            break;
        }

        if (__this->UART->CON0 & BIT(15)) {
            break;
        }

        clr_wdt();
    }

    JL_TIMER0->CON = BIT(14);
}


u8 ucEx_mcu_tx_rx_init(const struct ex_mcu_platform_data *data)
{
    u32 uart_timeout;
    __this->data = data;
    ASSERT(data);
    if (!(JL_UART0->CON0 & BIT(0))) {
        JL_UART0->CON0 = BIT(13) | BIT(12) | BIT(10);
        /* request_irq(IRQ_UART0_IDX, 2, uart_isr, 0); */
        __this->UART = JL_UART0;
        log_info("Ex_mcu using JL_UART0\n");
    } else if (!(JL_UART1->CON0 & BIT(0))) {
        JL_UART1->CON0 = BIT(13) | BIT(12) | BIT(10);
        /* request_irq(IRQ_UART1_IDX, 2, uart_isr, 0); */
        __this->UART = JL_UART1;
        log_info("Ex_mcu using JL_UART1\n");
    } else if (!(JL_UART2->CON0 & BIT(0))) {
        JL_UART2->CON0 = BIT(13) | BIT(12) | BIT(10);
        /* request_irq(IRQ_UART2_IDX, 2, uart_isr, 0); */
        __this->UART = JL_UART2;
        log_info("Ex_mcu using JL_UART2\n");
    } else {
        ASSERT(0, "uart all used!\n");
    }
    send_busy = 0;
    uart_timeout = 20 * 1000000 / __this->data->hand_baudrate;
    __this->UART->CON0 = BIT(13) | BIT(12) | BIT(10) | BIT(1) | BIT(0);//占用该串口,不被其他模块使用
    /* __this->UART->OTCNT = uart_timeout * (UART_OT_CLK / 1000000); */
    __this->UART->BAUD = (UART_SRC_CLK / __this->data->hand_baudrate) / 4 - 1;

    __this->baudrate = __this->data->hand_baudrate;
    gpio_set_pull_down(__this->data->io_port, 0);
    gpio_set_pull_up(__this->data->io_port, 1);
    gpio_set_die(__this->data->io_port, 1);
    gpio_direction_input(__this->data->io_port);

    return true;
}


void vEx_mcu_uart_tx_buf(u8 *uart_dma_buf, u32 dma_buf_len)
{
    __this->UART->CON0 = BIT(13) | BIT(12) | BIT(10);
    gpio_direction_output(__this->data->io_port, 1);

    __this->UART->CON1 |= BIT(4);
    if (__this->UART == JL_UART0) {
        gpio_set_fun_output_port(__this->data->io_port, FO_UART0_TX, 1, 1);
    } else if (__this->UART == JL_UART1) {
        gpio_set_fun_output_port(__this->data->io_port, FO_UART1_TX, 1, 1);
    } else {
        gpio_set_fun_output_port(__this->data->io_port, FO_UART2_TX, 1, 1);

    }
    /* __this->UART->CON0 |= BIT(2); */

    __this->UART->CON0 |= BIT(13) | BIT(12) | BIT(10) | BIT(0);

    u32 data_addr = (u32)uart_dma_buf;
    if (data_addr % 4) {//4byte对齐
        ASSERT(0, "%s: unaligned accesses!", __func__);
    }
    send_busy = 1;
    __this->UART->TXADR = data_addr;
    __this->UART->TXCNT = dma_buf_len;

    //最长等10ms
    vEx_mcu_udelay(10);
    while (!(__this->UART->CON0 & BIT(15)));

    send_busy = 0;
    __this->UART->CON0 |= BIT(13);

    gpio_direction_input(__this->data->io_port);
}

u32 ulEx_mcu_uart_rx_buf(u8 *uart_dma_buf, u32 dma_buf_len, u32 timeout)
{

    u32 ret = 0;
    __this->UART->CON0 = BIT(13) | BIT(12) | BIT(10);

    gpio_direction_input(__this->data->io_port);
    gpio_set_die(__this->data->io_port, 1);

    __this->UART->CON1 &= ~BIT(4);
    if (__this->UART == JL_UART0) {
        gpio_set_fun_input_port(__this->data->io_port, PFI_UART0_RX);
    } else if (__this->UART == JL_UART1) {
        gpio_set_fun_input_port(__this->data->io_port, PFI_UART1_RX);
    } else {
        gpio_set_fun_input_port(__this->data->io_port, PFI_UART2_RX);
    }

    memset((void *)uart_dma_buf, 0, sizeof(uart_dma_buf));
    __this->UART->RXSADR = (u32)uart_dma_buf;
    __this->UART->RXEADR = (u32)((((u32)uart_dma_buf + dma_buf_len) + 3) / 4 * 4);
    __this->UART->RXCNT = dma_buf_len;

    __this->UART->CON0 |= BIT(6) ;
    __this->UART->CON0 |= BIT(13) | BIT(12) | BIT(1);

//轮询检查是否收到内容
    while (timeout--) {
        __asm__ volatile("csync");
        if (__this->UART->CON0 & (BIT(14))) {
            log_debug("CAAAAAAAAAC\n");
            ret = dma_buf_len;
            break;
        }
        //1ms
        //log_debug("\n%d\n",timeout);
        __this->data->delay(1);
    }
    __this->UART->CON0 |= BIT(12);
    printf_buf((u8 *)uart_dma_buf, dma_buf_len);

    return ret;
}

#endif
//=======================================================================//
//                          公共部分                             //
//=======================================================================//
//从机复位部分
static void reset_ex_mcu(void)
{
    gpio_set_direction(TCFG_EX_RESET_PORT, 0);
    gpio_set_output_value(TCFG_EX_RESET_PORT, 0);
    udelay(5000);
    gpio_set_output_value(TCFG_EX_RESET_PORT, 1);
}

void vEx_mcu_set_baudrate(u32 baudrate)
{
    __this->baudrate = baudrate;
    __this->UART->BAUD = (UART_SRC_CLK / __this->baudrate) / 4 - 1;
}

u8 ucEx_mcu_exit(void)
{
#if TCFG_EX_MCU_OS_ENABLE
    free(__this->data->ex_mcu_buf);
#endif

    //还是关闭串口功能
#if !TCFG_EX_MCU_UART_TO_USE
    __this->UART->CON0 = BIT(13) | BIT(12) | BIT(10);
#endif
    return true;
}

//防止其他任务设置时钟紊乱串口时钟内容发送
static void clock_critical_enter(void)
{
    if (__this->UART == NULL) {
        return;
    }

    //等待数据发完
    while (send_busy);
}
static void clock_critical_exit(void)
{
    u32 uart_timeout;
    if (__this->UART == NULL) {
        return;
    }
    uart_timeout = 20 * 1000000 / __this->baudrate;
    /* __this->UART->OTCNT = uart_timeout * (UART_OT_CLK / 1000000); */
    __this->UART->BAUD = (UART_SRC_CLK / __this->baudrate) / 4 - 1;
}
CLOCK_CRITICAL_HANDLE_REG(ex_mcu, clock_critical_enter, clock_critical_exit)

u8 ucEx_mcu_uart_app_file_download(const char *file_app, u32 retry, u32 retry_timeout, u32 file_timeout)
{
    ex_mcu_enter_flag = 1;
    ucEx_mcu_app_file_download(file_app, /*u8 mode,*/ retry, retry_timeout, file_timeout);
    //屏蔽串口中断
    ex_mcu_enter_flag = 0;
}

void Ex_mcu_uart_test(void)
{
    u8 ret = 0;
    //注册从机复位接口到底层
    reset_ex_mcu_register(reset_ex_mcu);
//多次使用需要多次初始化参数
#if 0
#include "ex_mcu_uart.h"
    extern struct ex_mcu_platform_data ex_mcu_data;
    ucEx_mcu_init(&ex_mcu_data);
#endif

    ret = ucEx_mcu_uart_app_file_download(EX_MCU_APP_FILE_PATH, 50, 50, 50);
}
#endif  /* #if TCFG_EX_MCUE_ENABLE */
