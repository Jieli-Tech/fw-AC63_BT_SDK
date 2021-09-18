#include "asm/includes.h"
#include "asm/irflt.h"
#include "timer.h"
#include "generic/gpio.h"

#define ir_log 		log_d

//红外定时器定义
#define IR_TIMER                    TIMER5
#define IR_IRQ_TIME_IDX             IRQ_TIME5_IDX
#define IR_TIME_REG                 JL_TIMER5

#define IRFLT_OUTPUT_TIMER_SEL(x)		SFR(JL_IOMAP->CON0, 5, 3, x)


IR_CODE  ir_code;       ///<红外遥控信息

static u8 cmp_start = 0;

static const u16 timer_div[] = {
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


/*----------------------------------------------------------------------------*/
/**@brief   time1红外中断服务函数
   @param   void
   @param   void
   @return  void
   @note    void timer1_ir_isr(void)
*/
/*----------------------------------------------------------------------------*/
___interrupt
void timer_ir_isr(void)
{
    IR_TIME_REG->CON |= BIT(14);
    u16 bCap1 = IR_TIME_REG->PRD;
    IR_TIME_REG->CNT = 0;
    u8 cap = bCap1 / ir_code.timer_pad;

    ir_code.boverflow = 0;

    if (cmp_start < 3) {
        return;
    }

    /* putchar('0' + (cap/10)); */
    /* putchar('0' + (cap%10)); */

    if (cap <= 1) {
        ir_code.wData >>= 1;
        ir_code.bState++;
    } else if (cap == 2) {
        ir_code.wData >>= 1;
        ir_code.wData |= 0x8000;
        ir_code.bState++;
    }

    if (ir_code.bState == 16) {
        ir_code.wUserCode = ir_code.wData;
    }
    if (ir_code.bState == 33) {
        ir_code.bState = 1;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   ir按键初始化
   @param   void
   @param   void
   @return  void
   @note    void set_ir_clk(void)

   ((cnt - 1)* 分频数)/lsb_clk = 1ms
*/
/*----------------------------------------------------------------------------*/
/* #define APP_TIMER_CLK           clk_get("timer")//选osc时钟源 */
#define APP_TIMER_CLK           12000000L //pll12m
#define TIMER_UNIT_MS           1
#define MAX_TIME_CNT 0x07ff //分频准确范围，更具实际情况调整
#define MIN_TIME_CNT 0x0030
void set_ir_clk(void)
{
    u32 prd_cnt;
    u8 index;
    for (index = 0; index < (sizeof(timer_div) / sizeof(timer_div[0])); index++) {
        prd_cnt = TIMER_UNIT_MS * (APP_TIMER_CLK / 1000) / timer_div[index];
        if (prd_cnt > MIN_TIME_CNT && prd_cnt < MAX_TIME_CNT) {
            break;
        }
    }
    ir_code.timer_pad = prd_cnt;
    cmp_start = 0;
    request_irq(IR_IRQ_TIME_IDX, 5, timer_ir_isr, 0);
    /* IR_TIME_REG->CON = ((index << 4) | BIT(3) | BIT(1) | BIT(0));//选择osc时钟 */
    JL_IOMAP->CON0 |= BIT(21);//这里已选了timer5,时钟源选io信号里的pll_12m,不是所有的timer都可选pll,修改请看文档
    IR_TIME_REG->CON = ((index << 4) | BIT(2) | BIT(1) | BIT(0));
}

/*----------------------------------------------------------------------------*/
/**@brief   获取ir按键值
   @param   void
   @param   void
   @return  void
   @note    void get_irkey_value(void)
*/
/*----------------------------------------------------------------------------*/
u8 get_irflt_value(void)
{
    u8 tkey = 0xff;
    if (ir_code.bState != 32) {
        return tkey;
    }
    if ((((u8 *)&ir_code.wData)[0] ^ ((u8 *)&ir_code.wData)[1]) == 0xff) {
        tkey = (u8)ir_code.wData;
    } else {
        ir_code.bState = 0;
    }
    return tkey;
}

static u8 ir_io_level = 0;
static u8 ir_io = 0;
void ir_input_io_sel(u8 port)
{
    //选择input channel1输入
    // IOMC2[13 : 8]
    // 0 ~ 15: 		PA0 ~ PA15
    //16 ~ 31: 		PB0 ~ PB15
    //32 ~ 47: 		PC0 ~ PC15
    //48 ~ 55: 		PD0 ~ PD7
    ir_io = port;
    gpio_irflt_in(port);
    gpio_set_direction(port, 1);
    gpio_set_die(port, 1);
    gpio_set_pull_up(port, 1);
    gpio_set_pull_down(port, 0);
}

void ir_output_timer_sel()
{
    IRFLT_OUTPUT_TIMER_SEL(IR_TIMER);
}

static void ir_timeout(void *priv)
{
    ir_code.boverflow++;
    if (ir_code.boverflow > 56) { //56*2ms ~= 112ms
        ir_code.boverflow = 56;
        ir_code.bState = 0;
    }
    cmp_start ++;
    if (cmp_start > 3) {
        cmp_start = 3;
    }
}

void ir_timeout_set(void)
{
    sys_s_hi_timer_add(NULL, ir_timeout, 2); //2ms
}

static u8 ir_io_sus = 0;
u8 ir_io_suspend(void)
{
    if (ir_io_sus) {
        return 1;
    }
    if (ir_code.boverflow < 7) { //14ms内，红外接收有可能在忙碌
        return 1;
    }
    ir_io_level = gpio_read(ir_io);
    IR_TIME_REG->CON |= BIT(14);
    IR_TIME_REG->CON &= ~(0b11 << 0);
    ir_io_sus = 1;
    return 0;
}

u8 ir_io_resume(void)
{
    if (!ir_io_sus) {
        return 0;
    }
    ir_io_sus = 0;
    gpio_set_direction(ir_io, 1);
    gpio_set_die(ir_io, 1);
    gpio_set_pull_up(ir_io, 1);
    gpio_set_pull_down(ir_io, 0);
    delay(10);
    if ((ir_io_level) && (ir_io_level != (gpio_read(ir_io)))) {
        ir_code.boverflow = 0;
    }
    cmp_start = 0;
    IR_TIME_REG->CNT = 0;
    IR_TIME_REG->CON |= BIT(14);
    IR_TIME_REG->CON |= (0b11 << 0);
    return 0;
}


void irflt_config()
{
    JL_IR->RFLT_CON = 0;
    /* JL_IR->RFLT_CON |= BIT(7);		//256 div */
    /* JL_IR->RFLT_CON |= BIT(3);		//osc 24m */
    JL_IR->RFLT_CON |= BIT(7) | BIT(4);		//512 div
    JL_IR->RFLT_CON |= BIT(3) | BIT(2);		//PLL_48m（兼容省晶振）
    JL_IR->RFLT_CON |= BIT(0);		//irflt enable

    set_ir_clk();
}

void log_irflt_info()
{
    ir_log("IOMC0 = 0x%x", JL_IOMAP->CON0);
    ir_log("IOMC2 = 0x%x", JL_IOMAP->CON2);
    ir_log("RFLT_CON = 0x%x", JL_IR->RFLT_CON);
    ir_log("IR_TIME_REG = 0x%x", IR_TIME_REG->CON);
}


static u32 prd_1us = 0;
static u32 send_cnt = 0;
static u32 send_val = 0;
___interrupt
void timer5_isr(void)
{
    JL_TIMER5->CON |=  BIT(14);
    send_cnt ++;
    if (send_cnt % 2) {
        return;
    }
    u32 cnt = send_cnt / 2;
    if (cnt == 1) {
        JL_TIMER5->PRD = prd_1us * 5060;
        JL_TIMER5->PWM = prd_1us * 4500;
        JL_TIMER5->CNT = JL_TIMER5->PRD;
    } else if (cnt == 34) {
        JL_TIMER5->CON &= ~BIT(0);
        send_cnt = 0;
        return;
    } else if (send_val & BIT(33 - cnt)) {
        JL_TIMER5->PRD = prd_1us * 2240;
        JL_TIMER5->PWM = prd_1us * 1680;
        JL_TIMER5->CNT = JL_TIMER5->PRD;
    } else {
        JL_TIMER5->PRD = prd_1us * 1120;
        JL_TIMER5->PWM = prd_1us * 560;
        JL_TIMER5->CNT = JL_TIMER5->PRD;
    }
}

void timer5_send_ir_init(u32 port)
{
    u32 src_clk = 12000000;
    JL_IOMAP->CON0 |= BIT(21);//timer的io输入选择内部pll_12m,不是所有的timer的io输入都可选pll,修改请看文档
    JL_TIMER5->CON = 0;
    JL_TIMER5->CON |= (0b01 << 2);						//时钟源选择timer io
    JL_TIMER5->CON |= (0b0001 << 4);					//时钟源4分频
    JL_TIMER5->CON |= BIT(9);						    //波形反向
    prd_1us = (src_clk / (4 * 1000000));                //1us的计数值
    request_irq(IRQ_TIME5_IDX, 5, timer5_isr, 0);
    if ((IO_PORTB_07 == port) || (IO_PORTA_03 == port)) {//硬件io，br25的timer5有两个硬件IO口
        gpio_set_die(port, 1);
        gpio_set_pull_up(port, 0);
        gpio_set_pull_down(port, 0);
        gpio_set_direction(port, 0);
        JL_TIMER5->CON |= BIT(8);						//PWM使能
    } else {
        gpio_output_channle(port, CH2_T5_PWM_OUT);      //任意IO使用outputchannel
    }
}

void timer5_send_ir_data(u32 ir_data)
{
    send_val = ir_data;
    send_cnt = 0;
    JL_TIMER5->PRD = prd_1us * 9560;	                //设置周期 = 1us * 9605 = 9605us
    JL_TIMER5->PWM = prd_1us * 560;                     //设置高电平时间
    JL_TIMER5->CNT = JL_TIMER5->PRD;					//计数值先给满，起一次pend后，JL_TIMER->PWM寄存器才有效
    JL_TIMER5->CON |= BIT(0);                           //使能timer模块
}

void timer5_send_ir_test(void)
{
    /* timer5_send_ir_init(IO_PORTB_07); */
    timer5_send_ir_init(IO_PORTA_02);
    extern void wdt_clr();
    while (1) {
        wdt_clr();
        timer5_send_ir_data(0x6699aa55);
        os_time_dly(100);
    }

}


