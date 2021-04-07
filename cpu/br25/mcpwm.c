#include "asm/clock.h"
#include "asm/gpio.h"


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
        hw_port = IO_PORTC_04;
        break;
    case (u32)JL_TIMER2 :
        hw_port = IO_PORTB_03;
        break;
    case (u32)JL_TIMER3 :
        bit_clr_ie(IRQ_TIME3_IDX);
        hw_port = IO_PORTB_05;
        break;
    case (u32)JL_TIMER4 :
        hw_port = IO_PORTA_01;
        break;
    case (u32)JL_TIMER5 :
        hw_port = IO_PORTB_07;
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


void timer_pwm_test(void)
{
    timer_pwm_init(JL_TIMER2, 10000, 2000, IO_PORTB_00, CH2_T2_PWM_OUT);
    timer_pwm_init(JL_TIMER3, 10000, 5000, IO_PORTB_05, 0);
    timer_pwm_init(JL_TIMER5, 10000, 8000, IO_PORTB_07, 0);
}

