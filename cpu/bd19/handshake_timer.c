#include "asm/includes.h"
#include "system/includes.h"
#include "app_config.h"
#include "chargebox.h"

#if TCFG_HANDSHAKE_ENABLE

#define HS_TIMER    JL_TIMER2

static u16 delay_tap[HS_DELAY_16US + 1] = {
    19, 39,  52, 141, 158, 309, 355,//timer clk = 24M, sys clk = 48m
};

/*------------------------------------------------------------------------------------*/
/**@brief    lighting握手延时
   @param    无
   @return   无
   @note     提供不同的ms级延时
*/
/*------------------------------------------------------------------------------------*/
void handshake_timer_delay_ms(u8 ms)
{
    HS_TIMER->CNT = 0;
#if (TCFG_CLOCK_SYS_SRC == SYS_CLOCK_INPUT_PLL_RCL)
    HS_TIMER->PRD = ms * (clk_get("lsb") / (2 * 1000));
    HS_TIMER->CON = BIT(0) | BIT(6) | BIT(14); //系统时钟,lsb, div 2
#else
    HS_TIMER->PRD = ms * (clk_get("timer") / 1000);
    HS_TIMER->CON = BIT(0) | BIT(3) | BIT(14); //1分频,osc时钟,24m，24次就1us
#endif
    while (!(HS_TIMER->CON & BIT(15))); //等pending
    HS_TIMER->CON = 0;
}

/*------------------------------------------------------------------------------------*/
/**@brief    lighting握手延时
   @param    无
   @return   无
   @note     提供不同的us级延时
*/
/*------------------------------------------------------------------------------------*/
SEC(.chargebox_code)
void handshake_timer_delay_us(u8 us)
{
    //delay 值要根据不同的频率去调整，小于48m的要先设置48m，方便延时
    HS_TIMER->CNT = 0;
#if (TCFG_CLOCK_SYS_SRC == SYS_CLOCK_INPUT_PLL_RCL)
    HS_TIMER->PRD = delay_tap[us];//这里不允许做乘除法
    HS_TIMER->CON = BIT(0) | BIT(6) | BIT(14); //系统时钟,lsb, div 2, 必须为24M
#else
    HS_TIMER->PRD = delay_tap[us];
    HS_TIMER->CON = BIT(0) | BIT(3) | BIT(14); //1分频,osc时钟,24m，24次就1us
#endif
    while (!(HS_TIMER->CON & BIT(15))); //等pending
    HS_TIMER->CON = 0;
}


#endif

