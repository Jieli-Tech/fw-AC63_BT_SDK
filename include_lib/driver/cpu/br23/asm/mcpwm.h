#ifndef _MCPWM_H_
#define _MCPWM_H_


#include "typedef.h"


/* 对齐方式选择 */
typedef enum {
    pwm_edge_aligned,                                   ///< 边沿对齐模式
    pwm_center_aligned,                                 ///< 中心对齐模式
} pwm_aligned_mode_type;

/* pwm通道选择 */
typedef enum {
    pwm_ch0,
    pwm_ch1,
    pwm_ch2,
    pwm_ch3,
    pwm_ch4,
    pwm_ch5,
    pwm_ch_max,
} pwm_ch_num_type;

/* pwm timer通道选择 */
typedef enum {
    pwm_timer0,
    pwm_timer1,
    pwm_timer2,
    pwm_timer3,
    pwm_timer4,
    pwm_timer5,
    pwm_timer_max,
} pwm_timer_num_type;

/* MCPWM TIMER寄存器 */
typedef struct _pwm_timer_reg {
    volatile u32 tmr_con;
    volatile u32 tmr_cnt;
    volatile u32 tmr_pr;
} PWM_TIMER_REG;

/* MCPWM通道寄存器 */
typedef struct _pwm_ch_reg {
    volatile u32 ch_con0;
    volatile u32 ch_con1;
    volatile u32 ch_cmph;
    volatile u32 ch_cmpl;
} PWM_CH_REG;

/* 初始化要用的参数结构体 */
struct pwm_platform_data {
    u32 frequency;                               		///< 初始共同频率，CH0, CH, CH2, CH3, CH4
    pwm_ch_num_type pwm_ch_num;                         ///< 选择pwm通道
    pwm_timer_num_type pwm_timer_num;                   ///< 选择timer时基
    u16 duty;                                           ///< 初始占空比，0~10000 对应 0%~100% 。每个通道可以有不同的占空比。互补模式的占空比体现在高引脚的波形上。
    pwm_aligned_mode_type pwm_aligned_mode;             ///< PWM对齐方式选择
    u8 h_pin;                                           ///< 一个通道的高引脚，建议选择硬件引脚。填入-1 表示没有此引脚。当选择非硬件引脚时，不仅该引脚会输出波形，对应的硬件引脚也会有波形输出。
    u8 l_pin;                                           ///< 一个通道的低引脚，建议选择硬件引脚。填入-1 表示没有此引脚。当选择非硬件引脚时，不仅该引脚会输出波形，对应的硬件引脚也会有波形输出。
    u8 h_pin_output_ch_num;                             ///< 当高引脚选择了非硬件引脚时，该参数才有效。选用第几个gpio_output_channle, 值：0~2。
    u8 l_pin_output_ch_num;                             ///< 当高引脚选择了非硬件引脚时，该参数才有效。选用第几个gpio_output_channle, 值：0~2。不能与h_pin_output_ch_num的值一样。
    u8 complementary_en;                                ///< 该通道的两个引脚输出的波形是否需要互补。1: 同步， 0: 互补
};

//枚举触发模式
typedef enum {
    rising_edge_trigger,        //上升沿触发
    falling_edge_trigger,       //下降沿触发
} trigger_mode_type;

typedef void (*IO_ISR_FUNC)(void);


int timer_pwm_init(JL_TIMER_TypeDef *JL_TIMERx, u32 fre, u32 duty, u32 port, int output_ch);
void set_timer_pwm_duty(JL_TIMER_TypeDef *JL_TIMERx, u32 duty);

#endif

