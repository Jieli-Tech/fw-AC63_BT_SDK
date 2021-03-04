#include "asm/mcpwm.h"
#include "asm/clock.h"
#include "asm/gpio.h"

#define MCPWM_DEBUG_ENABLE  	1
#if MCPWM_DEBUG_ENABLE
#define mcpwm_debug(fmt, ...) printf("[MCPWM] "fmt, ##__VA_ARGS__)
#else
#define mcpwm_debug(...)
#endif

#define MCPWM_CLK   clk_get("mcpwm")

/*mcpwm硬件引脚，上下为一对：---CH0---     ---CH1---    ---CH2---    ---CH3---    ---CH4---    ---CH5--- */
static u8 pwm_hw_h_pin[6] = {IO_PORTA_00, IO_PORTB_00, IO_PORTB_04, IO_PORTB_09, IO_PORTA_09, IO_PORTC_04};
static u8 pwm_hw_l_pin[6] = {IO_PORTA_01, IO_PORTB_02, IO_PORTB_06, IO_PORTB_10, IO_PORTA_10, IO_PORTC_05};
//fpin
static u8 pwm_fpin[6] = {IO_PORTA_14, IO_PORTA_15, IO_PORTA_08, IO_PORTC_00, IO_PORTC_01, IO_PORTC_02};

//mctmr extern clk in pin
static u8 mctmr_clkin_pin[6] = {IO_PORTA_02, IO_PORTA_03, IO_PORTA_12, IO_PORTA_13, IO_PORTB_03, IO_PORTC_03};

//output_channle
static u8 CHx_CHx_PWM_H[3][3] = {CH0_CH0_PWM_H, CH0_CH1_PWM_H, CH0_CH2_PWM_H,  CH1_CH0_PWM_H, CH1_CH1_PWM_H, CH1_CH2_PWM_H,  CH2_CH0_PWM_H, CH2_CH1_PWM_H, CH2_CH2_PWM_H};
static u8 CHx_CHx_PWM_L[3][3] = {CH0_CH0_PWM_L, CH0_CH1_PWM_L, CH0_CH2_PWM_L,  CH1_CH0_PWM_L, CH1_CH1_PWM_L, CH1_CH2_PWM_L,  CH2_CH0_PWM_L, CH2_CH1_PWM_L, CH2_CH2_PWM_L};


PWM_TIMER_REG *get_pwm_timer_reg(pwm_timer_num_type index)
{
    PWM_TIMER_REG *reg = NULL;
    switch (index) {
    case pwm_timer0:
        reg = (PWM_TIMER_REG *)(&(JL_MCPWM->TMR0_CON));
        break;
    case pwm_timer1:
        reg = (PWM_TIMER_REG *)(&(JL_MCPWM->TMR1_CON));
        break;
    case pwm_timer2:
        reg = (PWM_TIMER_REG *)(&(JL_MCPWM->TMR2_CON));
        break;
    case pwm_timer3:
        reg = (PWM_TIMER_REG *)(&(JL_MCPWM->TMR3_CON));
        break;
    case pwm_timer4:
        reg = (PWM_TIMER_REG *)(&(JL_MCPWM->TMR4_CON));
        break;
    case pwm_timer5:
        reg = (PWM_TIMER_REG *)(&(JL_MCPWM->TMR5_CON));
        break;
    default:
        break;
    }

    return reg;
}


PWM_CH_REG *get_pwm_ch_reg(pwm_ch_num_type index)
{
    PWM_CH_REG *reg = NULL;
    switch (index) {
    case pwm_timer0:
        reg = (PWM_CH_REG *)(&(JL_MCPWM->CH0_CON0));
        break;
    case pwm_timer1:
        reg = (PWM_CH_REG *)(&(JL_MCPWM->CH1_CON0));
        break;
    case pwm_timer2:
        reg = (PWM_CH_REG *)(&(JL_MCPWM->CH2_CON0));
        break;
    case pwm_timer3:
        reg = (PWM_CH_REG *)(&(JL_MCPWM->CH3_CON0));
        break;
    case pwm_timer4:
        reg = (PWM_CH_REG *)(&(JL_MCPWM->CH4_CON0));
        break;
    case pwm_timer5:
        reg = (PWM_CH_REG *)(&(JL_MCPWM->CH5_CON0));
        break;
    default:
        break;
    }

    return reg;
}

static u32 _pow(u32 num, int n)
{
    u32 powint = 1;
    int i;
    for (i = 1; i <= n; i++) {
        powint *= num;
    }
    return powint;
}

/*
 * @brief 更改MCPWM的频率
 * @parm frequency 频率
 */
void mcpwm_set_frequency(pwm_timer_num_type ch, pwm_aligned_mode_type align, u32 frequency)
{
    PWM_TIMER_REG *reg = get_pwm_timer_reg(ch);
    if (reg == NULL) {
        return;
    }

    u32 i = 0;
    u32 mcpwm_div_clk = 0;
    u32 mcpwm_tmr_pr = 0;
    u32 mcpwm_fre_min = 0;

    reg->tmr_con = 0;
    reg->tmr_cnt = 0;
    reg->tmr_pr = 0;

    u32 clk = MCPWM_CLK;

    for (i = 0; i < 16; i++) {
        mcpwm_fre_min = clk / (65536 * _pow(2, i));
        if ((frequency >= mcpwm_fre_min) || (i == 15)) {
            break;
        }
    }

    reg->tmr_con |= (i << 4); //div 2^i

    mcpwm_div_clk = clk / _pow(2, i);

    if (frequency == 0) {
        mcpwm_tmr_pr = 0;
    } else {
        if (align == pwm_center_aligned) { //中心对齐
            mcpwm_tmr_pr = mcpwm_div_clk / (frequency * 2) - 1;
        } else {
            mcpwm_tmr_pr = mcpwm_div_clk / frequency - 1;
        }
    }
    reg->tmr_pr = mcpwm_tmr_pr;

    //timer mode
    if (align == pwm_center_aligned) { //中心对齐
        reg->tmr_con |= 0b10;
    } else {
        reg->tmr_con |= 0b01;
    }
}


/*
 * @brief 设置一个通道的占空比
 * @parm pwm_ch_num 通道号：pwm_ch0，pwm_ch1，pwm_ch2
 * @parm duty 占空比：0 ~ 10000 对应 0% ~ 100%
 */
void mcpwm_set_duty(pwm_ch_num_type pwm_ch, pwm_timer_num_type timer_ch, u16 duty)
{
    PWM_TIMER_REG *timer_reg = get_pwm_timer_reg(timer_ch);
    PWM_CH_REG *pwm_reg = get_pwm_ch_reg(pwm_ch);

    if (pwm_reg && timer_reg) {
        pwm_reg->ch_cmpl = timer_reg->tmr_pr * duty / 10000;
        pwm_reg->ch_cmph = pwm_reg->ch_cmpl;
        timer_reg->tmr_cnt = 0;
        timer_reg->tmr_con |= 0b01;
        if (duty == 10000) {
            timer_reg->tmr_cnt = 0;
            timer_reg->tmr_con &= ~(0b11);
        } else if (duty == 0) {
            timer_reg->tmr_cnt = pwm_reg->ch_cmpl;
            timer_reg->tmr_con &= ~(0b11);
        }
    }
}

/*
 * @brief 打开或者关闭一个时基
 * @parm pwm_ch_num 通道号：pwm_ch0，pwm_ch1，pwm_ch2
 * @parm enable 1：打开  0：关闭
 */
void mctimer_ch_open_or_close(pwm_timer_num_type timer_ch, u8 enable)
{
    if (timer_ch > pwm_timer_max) {
        return;
    }
    if (enable) {
        JL_MCPWM->MCPWM_CON0 |= BIT(timer_ch + 8); //TnEN
    } else {
        JL_MCPWM->MCPWM_CON0 &= (~BIT(timer_ch + 8)); //TnDIS
    }
}


/*
 * @brief 打开或者关闭一个通道
 * @parm pwm_ch_num 通道号：pwm_ch0，pwm_ch1，pwm_ch2
 * @parm enable 1：打开  0：关闭
 */
void mcpwm_ch_open_or_close(pwm_ch_num_type pwm_ch, u8 enable)
{
    if (pwm_ch >= pwm_ch_max) {
        return;
    }

    if (enable) {
        JL_MCPWM->MCPWM_CON0 |= BIT(pwm_ch); //PWMnEN
    } else {
        JL_MCPWM->MCPWM_CON0 &= (~BIT(pwm_ch)); //PWMnDIS
    }
}

/*
 * @brief 关闭MCPWM模块
 */
void mcpwm_open(pwm_ch_num_type pwm_ch, pwm_timer_num_type timer_ch)
{
    if ((pwm_ch >= pwm_ch_max) || (timer_ch >= pwm_timer_max)) {
        return;
    }
    PWM_CH_REG *pwm_reg = get_pwm_ch_reg(pwm_ch);
    pwm_reg->ch_con1 &= ~(0b111 << 8);
    pwm_reg->ch_con1 |= (timer_ch << 8); //sel mctmr

    mcpwm_ch_open_or_close(pwm_ch, 1);
    mctimer_ch_open_or_close(timer_ch, 1);
}


/*
 * @brief 关闭MCPWM模块
 */
void mcpwm_close(pwm_ch_num_type pwm_ch, pwm_timer_num_type timer_ch)
{
    mctimer_ch_open_or_close(timer_ch, 0);
    mcpwm_ch_open_or_close(pwm_ch, 0);
}


void log_pwm_info(pwm_ch_num_type pwm_ch, pwm_timer_num_type timer_ch)
{
    PWM_CH_REG *pwm_reg = get_pwm_ch_reg(pwm_ch);
    PWM_TIMER_REG *timer_reg = get_pwm_timer_reg(timer_ch);
    mcpwm_debug("pwm_ch %d @ 0x%x, timer_ch %d @ 0x%x", pwm_ch, pwm_reg, timer_ch, timer_reg);
    mcpwm_debug("tmr%d con0 = 0x%x", timer_ch, timer_reg->tmr_con);
    mcpwm_debug("tmr%d pr = 0x%x", timer_ch, timer_reg->tmr_pr);
    mcpwm_debug("pwm ch%d_con0 = 0x%x", pwm_ch, pwm_reg->ch_con0);
    mcpwm_debug("pwm ch%d_con1 = 0x%x", pwm_ch, pwm_reg->ch_con1);
    mcpwm_debug("pwm ch%d_cmph = 0x%x, pwm ch%d_cmpl = 0x%x", pwm_ch, pwm_reg->ch_cmph, pwm_ch, pwm_reg->ch_cmpl);
    mcpwm_debug("MCPWM_CON0 = 0x%x", JL_MCPWM->MCPWM_CON0);
    mcpwm_debug("mcpwm clk = %d", MCPWM_CLK);
}

void mcpwm_init(struct pwm_platform_data *arg)
{
    u8 h_output_channel = 0;
    u8 l_output_channel = 0;
    u8 use_output_ch_flag = 0;

    //set mctimer frequency
    mcpwm_set_frequency(arg->pwm_timer_num, arg->pwm_aligned_mode, arg->frequency);
    //set output IO
    PWM_CH_REG *pwm_reg = get_pwm_ch_reg(arg->pwm_ch_num);
    if (pwm_reg == NULL) {
        return;
    }

    pwm_reg->ch_con0 = 0;

    //H:
    if (arg->h_pin == pwm_hw_h_pin[arg->pwm_ch_num]) {    //硬件引脚
        pwm_reg->ch_con0 |= BIT(2); //H_EN
        gpio_set_direction(arg->h_pin, 0); //DIR output
    } else {
        pwm_reg->ch_con0 &= ~BIT(2); //H_DISABLE
        if (arg->h_pin < IO_MAX_NUM) {    //任意引脚
            //TODO: output_channle
            if (arg->pwm_ch_num >= pwm_ch3) {
                printf("error: mcpwm ch %d not support output_channel", arg->pwm_ch_num);
                goto _CH_L_SET;
            }
            h_output_channel = 1;
            /* gpio_output_channle(arg->h_pin, CHx_CHx_PWM_H[arg->h_pin_output_ch_num][arg->pwm_ch_num]); */
            use_output_ch_flag = 1;
        }
    }

_CH_L_SET:
    //L:
    if (arg->l_pin == pwm_hw_l_pin[arg->pwm_ch_num]) {    //硬件引脚
        pwm_reg->ch_con0 |= BIT(3); //L_EN
        gpio_set_direction(arg->l_pin, 0); //DIR output
    } else {
        pwm_reg->ch_con0 &= ~BIT(3); //L_DISABLE
        if (arg->l_pin < IO_MAX_NUM) {    //任意引脚
            //TODO:
            if (arg->pwm_ch_num >= pwm_ch3) {
                printf("error: mcpwm ch %d not support output_channel", arg->pwm_ch_num);
                goto _PWM_OPEN;
            }
            if ((use_output_ch_flag == 1) && (arg->h_pin_output_ch_num == arg->l_pin_output_ch_num)) {
                arg->l_pin_output_ch_num ++;
                if (arg->l_pin_output_ch_num > 2) {
                    arg->l_pin_output_ch_num = 0;
                }
            }
            l_output_channel = 1;
            /* gpio_output_channle(arg->l_pin, CHx_CHx_PWM_L[arg->l_pin_output_ch_num][arg->pwm_ch_num]); */
        }
    }

    if (arg->complementary_en) {            //是否互补
        pwm_reg->ch_con0 &= ~(BIT(5) | BIT(4));
        pwm_reg->ch_con0 |= BIT(5); //L_INV
    } else {
        pwm_reg->ch_con0 &= ~(BIT(5) | BIT(4));
    }

_PWM_OPEN:
    mcpwm_open(arg->pwm_ch_num, arg->pwm_timer_num); 	 //mcpwm enable
    if (h_output_channel) {
        gpio_output_channle(arg->h_pin, CHx_CHx_PWM_H[arg->h_pin_output_ch_num][arg->pwm_ch_num]);
    }
    if (l_output_channel) {
        gpio_output_channle(arg->l_pin, CHx_CHx_PWM_L[arg->l_pin_output_ch_num][arg->pwm_ch_num]);
    }
    //set duty
    mcpwm_set_duty(arg->pwm_ch_num, arg->pwm_timer_num, arg->duty);

    /* log_pwm_info(arg->pwm_ch_num, arg->pwm_timer_num); */
}


///////////// for test code //////////////////
void mcpwm_test(void)
{
#define PWM_CH0_ENABLE 		1
#define PWM_CH1_ENABLE 		1
#define PWM_CH2_ENABLE 		1
#define PWM_CH3_ENABLE 		1
#define PWM_CH4_ENABLE 		1
#define PWM_CH5_ENABLE 		1

    struct pwm_platform_data pwm_p_data;
#if PWM_CH0_ENABLE
    //CH0
    pwm_p_data.pwm_aligned_mode = pwm_edge_aligned;         //边沿对齐
    pwm_p_data.frequency = 1000;                     //1KHz

    pwm_p_data.pwm_ch_num = pwm_ch0;                        //通道0
    pwm_p_data.pwm_timer_num = pwm_timer0;                  //时基选择通道0
    pwm_p_data.duty = 5000;                                 //占空比50%
    //hw
    pwm_p_data.h_pin = IO_PORTA_00;                         //没有则填 -1。h_pin_output_ch_num无效，可不配置
    pwm_p_data.l_pin = IO_PORTA_01;                         //硬件引脚，l_pin_output_ch_num无效，可不配置
    //output_channel
    /* pwm_p_data.h_pin = IO_PORTB_00;                          //没有则填 -1。h_pin_output_ch_num无效，可不配置 */
    /* pwm_p_data.l_pin = IO_PORTB_01;                         //硬件引脚，l_pin_output_ch_num无效，可不配置 */
    /* pwm_p_data.h_pin_output_ch_num = 0;                          //output channel0 */
    /* pwm_p_data.l_pin_output_ch_num = 1;                          //output channel1 */
    pwm_p_data.complementary_en = 1;                        //两个引脚的波形, 1: 互补, 0: 同步;

    mcpwm_init(&pwm_p_data);
#endif

#if PWM_CH1_ENABLE
    //CH1
    pwm_p_data.pwm_aligned_mode = pwm_edge_aligned;         //边沿对齐
    pwm_p_data.frequency = 100;                     //1KHz

    pwm_p_data.pwm_ch_num = pwm_ch1;                        //通道0
    pwm_p_data.pwm_timer_num = pwm_timer1;                  //时基选择通道0
    pwm_p_data.duty = 5000;                                 //占空比50%
    //hw
    pwm_p_data.h_pin = IO_PORTB_00;                         //没有则填 -1。h_pin_output_ch_num无效，可不配置
    pwm_p_data.l_pin = IO_PORTB_02;                         //硬件引脚，l_pin_output_ch_num无效，可不配置
    //output_channel
    /* pwm_p_data.h_pin = IO_PORTA_00;                         //没有则填 -1。h_pin_output_ch_num无效，可不配置 */
    /* pwm_p_data.l_pin = IO_PORTA_04;                         //硬件引脚，l_pin_output_ch_num无效，可不配置 */
    /* pwm_p_data.h_pin_output_ch_num = 0;                          //output channel0 */
    /* pwm_p_data.l_pin_output_ch_num = 2;                          //output channel1 */
    pwm_p_data.complementary_en = 1;                        //两个引脚的波形同步

    mcpwm_init(&pwm_p_data);
#endif

#if PWM_CH2_ENABLE
    //CH2
    pwm_p_data.pwm_aligned_mode = pwm_edge_aligned;         //边沿对齐
    pwm_p_data.frequency = 2000;                     //1KHz

    pwm_p_data.pwm_ch_num = pwm_ch2;                        //通道0
    pwm_p_data.pwm_timer_num = pwm_timer2;                  //时基选择通道0
    pwm_p_data.duty = 5000;                                 //占空比50%
    //hw
    pwm_p_data.h_pin = IO_PORTB_04;                                  //没有则填 -1。h_pin_output_ch_num无效，可不配置
    //output_channel
    /* pwm_p_data.h_pin = IO_PORTB_05;                                  //没有则填 -1。h_pin_output_ch_num无效，可不配置 */
    /* pwm_p_data.h_pin_output_ch_num = 1;                          //output channel0 */
    //hw
    pwm_p_data.l_pin = IO_PORTB_06;                         //硬件引脚，l_pin_output_ch_num无效，可不配置
    pwm_p_data.complementary_en = 1;                        //两个引脚的波形同步

    mcpwm_init(&pwm_p_data);
#endif

    //注意: CH3, CH4, CH5不支持通过output channel输出
#if PWM_CH3_ENABLE
    //CH3
    pwm_p_data.pwm_aligned_mode = pwm_edge_aligned;         //边沿对齐
    pwm_p_data.frequency = 4000;                     //1KHz

    pwm_p_data.pwm_ch_num = pwm_ch3;                        //通道0
    pwm_p_data.pwm_timer_num = pwm_timer3;                  //时基选择通道0
    pwm_p_data.duty = 5000;                                 //占空比50%
    pwm_p_data.h_pin = IO_PORTB_09;                                  //没有则填 -1。h_pin_output_ch_num无效，可不配置
    pwm_p_data.l_pin = IO_PORTB_10;                         //硬件引脚，l_pin_output_ch_num无效，可不配置
    pwm_p_data.complementary_en = 1;                        //两个引脚的波形同步

    mcpwm_init(&pwm_p_data);
#endif

#if PWM_CH4_ENABLE
    //CH4
    pwm_p_data.pwm_aligned_mode = pwm_edge_aligned;         //边沿对齐
    pwm_p_data.frequency = 8000;                     //1KHz

    pwm_p_data.pwm_ch_num = pwm_ch4;                        //通道0
    pwm_p_data.pwm_timer_num = pwm_timer4;                  //时基选择通道0
    pwm_p_data.duty = 5000;                                 //占空比50%
    pwm_p_data.h_pin = IO_PORTA_09;                                  //没有则填 -1。h_pin_output_ch_num无效，可不配置
    pwm_p_data.l_pin = IO_PORTA_10;                         //硬件引脚，l_pin_output_ch_num无效，可不配置
    pwm_p_data.complementary_en = 1;                        //两个引脚的波形同步

    mcpwm_init(&pwm_p_data);
#endif

#if PWM_CH5_ENABLE
    //CH5
    pwm_p_data.pwm_aligned_mode = pwm_edge_aligned;         //边沿对齐
    pwm_p_data.frequency = 16000;                     //1KHz
    //pwm_p_data.frequency = 50000;                     //1KHz

    pwm_p_data.pwm_ch_num = pwm_ch5;                        //通道0
    pwm_p_data.pwm_timer_num = pwm_timer5;                  //时基选择通道0
    pwm_p_data.duty = 5000;                                 //占空比50%
    pwm_p_data.h_pin = IO_PORTC_04;                                  //没有则填 -1。h_pin_output_ch_num无效，可不配置
    pwm_p_data.l_pin = IO_PORTC_05;                         //硬件引脚，l_pin_output_ch_num无效，可不配置
    pwm_p_data.complementary_en = 1;                        //两个引脚的波形同步

    mcpwm_init(&pwm_p_data);
#endif

    while (1);
}


/**************************************  外部引脚中断参考代码  **************************/
static IO_ISR_FUNC io_isr_cbfun;
___interrupt
void io_interrupt(void)
{
    JL_MCPWM->CH0_CON1 |= BIT(14);
    io_isr_cbfun();
}
void io_ext_interrupt_init(u8 port, trigger_mode_type trigger_mode, IO_ISR_FUNC cbfun)
{
    if (port > IO_PORT_MAX) {
        return;
    }
    gpio_set_die(port, 1);
    gpio_set_direction(port, 1);
    if (trigger_mode) {
        gpio_set_pull_up(port, 1);
        gpio_set_pull_down(port, 0);
        JL_MCPWM->FPIN_CON = 0;
    } else {
        gpio_set_pull_up(port, 0);
        gpio_set_pull_down(port, 1);
        JL_MCPWM->FPIN_CON = BIT(16);
    }
    if (port == IO_PORTA_14) {
        JL_IOMAP->CON3 &= ~BIT(12);             //使用硬件引脚
    } else {
        JL_IOMAP->CON3 |= BIT(12);              //使用input_channel 2
        JL_IOMAP->CON2 &= ~(0b111111 << 16);
        JL_IOMAP->CON2 |= (port << 16);
    }
    io_isr_cbfun = cbfun;
    request_irq(IRQ_CHX_PWM_IDX, 3, io_interrupt, 0);   //注册中断函数
    JL_MCPWM->CH0_CON1 = BIT(14) | BIT(11) | BIT(4);
    JL_MCPWM->MCPWM_CON0 |= BIT(0);
}
void io_ext_interrupt_close(u8 port)
{
    if (port > IO_PORT_MAX) {
        return;
    }
    gpio_set_die(port, 0);
    gpio_set_direction(port, 1);
    gpio_set_pull_up(port, 0);
    gpio_set_pull_down(port, 0);
    JL_MCPWM->CH0_CON1 = BIT(14);
}

///////////// 使用举例如下 //////////////////
void my_io_isr_cbfun(void)
{
    printf("Hello world !\n");
}
void io_ext_interrupt_test(void)
{
    io_ext_interrupt_init(IO_PORTA_09, rising_edge_trigger, my_io_isr_cbfun);
    while (1);
}


#define		OSC_Hz		24000000
/**
 * @param JL_TIMERx : JL_TIMER0/1/2/3/4/5
 * @param fre : 频率，单位Hz，不小于95
 * @param duty : 初始占空比，0~10000对应0~100%
 * @param port : pwm脚，可选硬件脚，也可选非硬件脚。(建议选择硬件引脚)
 * @param output_ch : 映射通道，当pwm脚选择非硬件脚时有效，这时我们给他分配output_channel 0/1/2
 */
int timer_pwm_init(JL_TIMER_TypeDef *JL_TIMERx, u32 fre, u32 duty, u32 port, int output_ch)
{
    u32 hw_port;
    switch ((u32)JL_TIMERx) {
    case (u32)JL_TIMER0 :
        hw_port = IO_PORTA_05;
        break;
    case (u32)JL_TIMER1 :
        hw_port = IO_PORTA_12;
        break;
    case (u32)JL_TIMER2 :
        hw_port = IO_PORTB_03;
        break;
    case (u32)JL_TIMER3 :
        bit_clr_ie(IRQ_TIME3_IDX);
        hw_port = IO_PORTB_05;
        break;
    default:
        return (-1);
    }
    if ((output_ch == (-1)) && (hw_port != port)) {
        //not support output_ch
        return (-1);
    }

    //初始化timer
    JL_TIMERx->CON = 0;
    JL_TIMERx->CON |= (0b10 << 2);						//选择晶振时钟源：24MHz
    JL_TIMERx->CON |= (0b0001 << 4);					//时钟源4分频
    JL_TIMERx->PRD = OSC_Hz / (4 * fre);				//设置周期
    JL_TIMERx->PWM = (JL_TIMERx->PRD * duty) / fre;	//0~10000对应0~100g
    JL_TIMERx->CNT = 0;									//清计数值
    JL_TIMERx->CON |= (0b01 << 0);						//计数模式

    if (hw_port == port) {
        gpio_set_die(hw_port, 1);
        gpio_set_pull_up(hw_port, 0);
        gpio_set_pull_down(hw_port, 0);
        gpio_set_direction(hw_port, 0);
        JL_TIMERx->CON |= BIT(8);						//PWM使能
    } else {

        gpio_output_channle(port, output_ch);
        return 1;
    }
    return 0;
}

/**
 * @param JL_TIMERx : JL_TIMER0/1/2/3/4/5
 * @param duty : 占空比，0~10000对应0~100%
 */
void set_timer_pwm_duty(JL_TIMER_TypeDef *JL_TIMERx, u32 duty)
{
    JL_TIMERx->PWM = (JL_TIMERx->PRD * duty) / 10000;	//0~10000对应0~100%
}

/*  */
/* void timer_pwm_test(void) */
/* { */
/*     timer_pwm_init(JL_TIMER2, 10000, 2000, IO_PORTB_00, CH2_T2_PWM_OUT); */
/*     timer_pwm_init(JL_TIMER3, 10000, 5000, IO_PORTB_05, 0); */
/*     timer_pwm_init(JL_TIMER5, 10000, 8000, IO_PORTB_07, 0); */
/* } */

