#include "generic/gpio.h"
#include "asm/includes.h"
#include "asm/power/p33.h"
#include "asm/pwm_led.h"
#include "system/timer.h"

/*******************************************************************
*	推灯注意事项:
*		1)PWM_CON1的BIT(4), OUT_LOGIC位一定要设置为1;
*		2)PWM_CON1的BIT(0), PWM0_INV位一定要设置为0;
*		3)在非呼吸灯效果下, 单IO双LED模式, PWM_PRD_DIV寄存器和PWM_BRI_PRD设置成相同值;
*		4)在闪灯模式下, PWM_BRI_PRD(亮度值)固定下来后, PWM0的BRI_DUTY0和BRI_DUTY1一定不能超过PWM_BRI_PRD;
*********************************************************************/
/* #ifdef SUPPORT_MS_EXTENSIONS */
/* #pragma bss_seg(	".pwm_led_bss") */
/* #pragma data_seg(	".pwm_led_data") */
/* #pragma const_seg(	".pwm_led_const") */
/* #pragma code_seg(	".pwm_led_code") */
/* #endif */

//#define PWM_LED_TEST_MODE 		//LED模块测试函数

//#define PWM_LED_DEBUG_ENABLE

#ifdef PWM_LED_DEBUG_ENABLE
#define led_debug(fmt, ...) 	printf("[PWM_LED] "fmt, ##__VA_ARGS__)
#define led_err(fmt, ...) 		printf("[PWM_LED_ERR] "fmt, ##__VA_ARGS__)
#else
#define led_debug(...)
#define led_err(...)
#endif

//////////////////////////////////////////////////////////////
#define PWM_LED_SFR 	SFR

//========================= 0.模块开关相关寄存器
#define LED_PWM_ENABLE 					(JL_PLED->CON0 |= BIT(0))
#define LED_PWM_DISABLE 				(JL_PLED->CON0 &= ~BIT(0))
#define IS_PWM_LED_ON    				(JL_PLED->CON0 & BIT(0))

#define RESET_PWM_CON0 					(JL_PLED->CON0 = 0)
#define RESET_PWM_CON1 					(JL_PLED->CON1 = 0)
#define RESET_PWM_CON2 					(JL_PLED->CON2 = 0)
#define RESET_PWM_CON3 					(JL_PLED->CON3 = 0)

//========================= 1.时钟设置相关寄存器
//PWM0 CLK DIV
#define	CLK_DIV_1 			0
#define CLK_DIV_4           1
#define CLK_DIV_16          2
#define CLK_DIV_64          3

#define CLK_DIV_x2(x)       (0x04 | x)
#define CLK_DIV_x256(x)		(0x08 | x)
#define CLK_DIV_x2_x256(x)	(0x0c | x)
//SOURCE
//0:rclk(250kHz) 1:btosc(24MHz) 2:osc32k(32kHz) 3:rc32k(32kHz)
#define LED_PWM0_CLK_SEL(x)			PWM_LED_SFR(JL_PLED->CON0, 2, 2, x)
//SOURCE -- DIV  --> PWM0
//[1:0] 00:div1  01:div4  10:div16  11:div64
//[2]:  0:x1     1:x2
//[3]:  0:x1     1:x256
//DIV = [1:0] * [2] * [3]
#define LED_PWM0_CLK_DIV(x)			PWM_LED_SFR(JL_PLED->CON0, 4, 4, x)
//PWM0 -- DIV --> PWM1
//PWM_CON3[3:0] -- PWM_PRD_DIVL[7:0]  //12bit
#define LED_PWM1_CLK_DIV(x)	   	    	{do {JL_PLED->PRD_DIVL = ((x-1) & 0xFF); JL_PLED->CON3 &= ~(0xF); JL_PLED->CON3 |= (((x-1) >> 8) & 0xF);} while(0);}

//========================= 2.亮度设置相关寄存器
//最高亮度级数设置
//PWM_BRI_PRDH[1:0] -- PWM_BRI_PRDL[7:0]  //10bit
#define LED_BRI_DUTY_CYCLE(x)			{do {JL_PLED->BRI_PRDL = (x & 0xFF); JL_PLED->BRI_PRDH = ((x >> 8) & 0x3);} while(0);}
//高电平亮灯亮度设置
#define LED_BRI_DUTY1_SET(x)			{do {JL_PLED->BRI_DUTY0L = (x & 0xFF); JL_PLED->BRI_DUTY0H = ((x >> 8) & 0x3);} while(0);}
//低电平亮灯亮度设置
#define LED_BRI_DUTY0_SET(x)			{do {JL_PLED->BRI_DUTY1L = (x & 0xFF); JL_PLED->BRI_DUTY1H = ((x >> 8) & 0x3);} while(0);}
//同步LED0 <-- LED1亮度
#define LED_BRI_DUTY0_SYNC1() 			{do {JL_PLED->BRI_DUTY0L = JL_PLED->BRI_DUTY1L; JL_PLED->BRI_DUTY0H = JL_PLED->BRI_DUTY1H;} while(0);}
//同步LED1 <-- LED0亮度
#define LED_BRI_DUTY1_SYNC0() 			{do {JL_PLED->BRI_DUTY1L = JL_PLED->BRI_DUTY0L; JL_PLED->BRI_DUTY1H = JL_PLED->BRI_DUTY0H;} while(0);}


//========================= 3.亮灭设置相关寄存器
//duty_cycle 固定为255或者设置PWM_DUTY3, 8bit
//   	 	  _duty1_	 	  _duty3_
//		      |     |         |     |
//		      |     |         |     |
//0__ __ duty0|     | __ duty2|     |__ __255/PWM_DUTY3

//PWM1 duty_cycle 选择
#define LED_PWM1_DUTY_FIX_SET 			(JL_PLED->CON1 &= ~BIT(3))  //固定PWM1周期为0xFF
#define LED_PWM1_DUTY_VARY_SET(x) 		{do {JL_PLED->CON1 |= BIT(3); JL_PLED->DUTY3 = x;} while(0);}

#define LED_PWM1_DUTY0_SET(x)			(JL_PLED->DUTY0 = x)
#define LED_PWM1_DUTY1_SET(x)			(JL_PLED->DUTY1 = x)
#define LED_PWM1_DUTY2_SET(x)			(JL_PLED->DUTY2 = x)
#define LED_PWM1_DUTY3_SET(x)			(JL_PLED->DUTY3 = x) 	//可以设置为PWM1_DUTY_CYCLE
#define LED_PWM1_PRD_SEL_DIS			(JL_PLED->CON1 &= ~BIT(3))

#define LED_PWM1_PRD_SEL_EN				(JL_PLED->CON1 |= BIT(3))
#define LED_PWM1_DUTY0_EN				(JL_PLED->CON1 |= BIT(4))
#define LED_PWM1_DUTY1_EN				(JL_PLED->CON1 |= BIT(5))
#define LED_PWM1_DUTY2_EN				(JL_PLED->CON1 |= BIT(6))
#define LED_PWM1_DUTY3_EN				(JL_PLED->CON1 |= BIT(7))

#define LED_PWM1_ALL_DUTY_DIS			(JL_PLED->CON1 &= ~(0xF << 4))
#define LED_PWM1_DUTY0_DIS				(JL_PLED->CON1 &= ~BIT(4))
#define LED_PWM1_DUTY1_DIS				(JL_PLED->CON1 &= ~BIT(5))
#define LED_PWM1_DUTY2_DIS				(JL_PLED->CON1 &= ~BIT(6))
#define LED_PWM1_DUTY3_DIS				(JL_PLED->CON1 &= ~BIT(7))

//========================= 5.输出取反相关寄存器
//以下几个需要设置为固定值
#define LED_PWM0_INV_DISABLE 			(JL_PLED->CON1 &= ~BIT(0))
#define LED_PWM1_INV_DISABLE 			(JL_PLED->CON1 &= ~BIT(1)) //周期从灭灯开始
#define LED_PWM1_INV_ENABLE 			(JL_PLED->CON1 |= BIT(1))  //周期从亮灯开始
#define LED_PWM_OUT_LOGIC_SET 			(JL_PLED->CON3 |= BIT(4))
//以下几个可以灵活设置
#define LED_PWM_OUTPUT_INV_ENABLE 		(JL_PLED->CON1 |= BIT(2))
#define LED_PWM_OUTPUT_INV_DISABLE 		(JL_PLED->CON1 &= ~BIT(2))

//========================= 6.与周期变色相关寄存器
#define LED_PWM_SHIFT_DUTY_SET(x) 		PWM_LED_SFR(JL_PLED->CON2, 4, 4, x)

//========================= 7.与驱动强度相关寄存器
#define LED_PWM_IO_MAX_DRIVE(x) 		PWM_LED_SFR(JL_PLED->CON2, 0, 2, x)

//========================= 8.与中断相关寄存器
#define LED_PWM_INT_EN 					(JL_PLED->CON3 |= BIT(5))
#define LED_PWM_INT_DIS 				(JL_PLED->CON3 &= ~BIT(5))
#define LED_PWM_CLR_PENDING 			(JL_PLED->CON3 |= BIT(6))

//========================= 9.与呼吸模式相关寄存器
#define LED_PWM_BREATHE_ENABLE 			(JL_PLED->CON0 |= BIT(1))
#define LED_PWM_BREATHE_DISABLE 		(JL_PLED->CON0 &= ~BIT(1))
//LED呼吸灭灯延时设置, 16bit
#define LED_PWM_BREATHE_BLANK_TIME_SET(x) 	{do {LED_PWM1_DUTY2_EN; LED_PWM1_DUTY3_EN; LED_PWM1_DUTY2_SET((x & 0xFF)); LED_PWM1_DUTY3_SET((x >> 8) & 0xFF)} while(0)}
//LED呼吸灯(低电平灯)最高亮度延时设置, 8bit
#define LED0_PWM_BREATHE_LIGHT_TIME_SET(x) 	{do {LED_PWM1_DUTY0_EN; LED_PWM1_DUTY0_SET(x)} while(0)}
//LED呼吸灯(高电平灯)最高亮度延时设置, 8bit
#define LED1_PWM_BREATHE_LIGHT_TIME_SET(x) 	{do {LED_PWM1_DUTY1_EN; LED_PWM1_DUTY1_SET(x)} while(0)}

struct pwm_led {
    u8 init;
    u8 led_pin;
    u8 clock;
    u8 last_mode;
    const struct led_platform_data *user_data;
    //中断相关
    void (*pwm_led_extern_isr)(void);
    void (*pwm_led_local_isr)(void);
#ifdef PWM_LED_TWO_IO_SUPPORT
    u8 display_index;
#endif /* #ifdef PWM_LED_TWO_IO_SUPPORT */
};

typedef struct {
    pwm_led_on_para on;
    pwm_led_one_flash_para one_flash;
    pwm_led_double_flash_para double_flash;
    pwm_led_breathe_para breathe;
} pwm_led_para_t;
static pwm_led_para_t led_para;

enum _PWM0_CLK {
    PWM0_CLK_32K, //for rc normal
    //BT24M
    PWM0_CLK_46K, //24M / 512 = 46875, for normal period < 22s
    PWM0_CLK_23K, //24M / 1024 = 23437, for normal period > 22s
};

#define PWM0_RC_CLK32K_VALUE 	32000
#define PWM0_BT_CLK23K_VALUE 	23437
#define PWM0_BT_CLK46K_VALUE 	46875

#define PWM_LED_USER_DEFINE_MODE 0xFE

static struct pwm_led _led;

#define __this 		(&_led)

#define P3_CLK_CON0             0xa6
extern void p33_tx_1byte(u16 addr, u8 data0);
extern u8 p33_rx_1byte(u16 addr);
static void _pwm_led_display_mode(u8 display);
static void _led_pwm1_duty_set(u8 duty_prd, u8 duty0, u8 duty1, u8 duty2, u8 breathe);

static void pwm_clock_set(u8 _clock)
{
    u8 clock = _clock;
    u8 clk_val = 0;

    switch (clock) {
    case PWM_LED_CLK_RC32K:
        clk_val = 1;
        break;
    case PWM_LED_CLK_BTOSC_24M:
        clk_val = 2;
        break;
    default:
        break;
    }

    P33_CON_SET(P3_CLK_CON0, 0, 2, clk_val);
    /* p33_tx_1byte(P3_CLK_CON0, clk_val); */
    __this->clock = clock;
    led_debug("clock = 0x%x, clk_val= %d, P3_CLK_CON0 = 0x%x", __this->clock, clk_val, p33_rx_1byte(P3_CLK_CON0));
}


/*
 * IO使能注意, 否则有可能会出现在显示效果前会出现闪烁
 * 1)设置PU, PD为1;
 * 2)设置DIR, OUT为1;
 * 3)最后设置为方向为输出;
 */
static void led_pin_set_enable(u8 gpio)
{
    gpio_set_pull_up(gpio, 1);
    gpio_set_pull_down(gpio, 1);
    gpio_set_die(gpio, 1);

    gpio_set_output_value(gpio, 1);
    gpio_set_direction(gpio, 0);
}

//把IO设置为高阻
static void led_pin_set_disable(u8 disable_pin)
{
    gpio_set_pull_down(disable_pin, 0);
    gpio_set_pull_up(disable_pin, 0);
    gpio_direction_input(disable_pin);
}


___interrupt
static void pwm_led_isr_func(void)
{
    LED_PWM_CLR_PENDING;
    led_debug("led isr");
    if (__this->pwm_led_extern_isr) {
        __this->pwm_led_extern_isr();
    }
    if (__this->pwm_led_local_isr) {
        __this->pwm_led_local_isr();
    }
}

//index = 0: 内部切IO中断;
//index = 1: 外部注册中断;
static void _pwm_led_register_irq(void (*func)(void), u8 index)
{
    LED_PWM_INT_DIS;
    LED_PWM_CLR_PENDING;

    if (func) {
        if (index) {
            __this->pwm_led_local_isr = func;
        } else {
            __this->pwm_led_extern_isr = func;
        }
        request_irq(IRQ_PWM_LED_IDX, 1, pwm_led_isr_func, 0);
        LED_PWM_INT_EN;
    }
}


static void _pwm_led_close_irq(void)
{
    __this->pwm_led_extern_isr = NULL;
    __this->pwm_led_local_isr = NULL;
    LED_PWM_INT_DIS;
}
extern const struct low_power_param power_param;
void pwm_led_init(const struct led_platform_data *user_data)
{
    led_debug("pwm led init ...");

    memset(__this, 0, sizeof(struct pwm_led));

    LED_PWM_DISABLE;
    LED_PWM_BREATHE_DISABLE;  //呼吸灯使能位

    RESET_PWM_CON0;
    RESET_PWM_CON1;
    RESET_PWM_CON2;
    RESET_PWM_CON3;

    LED_PWM_OUT_LOGIC_SET;
    LED_PWM_CLR_PENDING;

    if (power_param.config == SLEEP_EN && power_param.osc_type == OSC_TYPE_LRC) {
        pwm_clock_set(PWM_LED_CLK_RC32K);
    } else {
        pwm_clock_set(PWM_LED_CLK_BTOSC_24M);
    }

    if (user_data->io_mode == LED_ONE_IO_MODE) {
        __this->led_pin = user_data->io_cfg.one_io.pin;
        led_pin_set_enable(user_data->io_cfg.one_io.pin);  //一个IO推两个灯
    }

    __this->user_data = user_data;
    __this->last_mode = PWM_LED_ALL_OFF;

    __this->init = 1;
}

//prd: 亮度总级数
//duty: 设置亮度级数
//实际输出频率 = Flrc / prd
//输出占空比: duty /prd
//gpio: 设置输出的IO
//注意：duty不能大于prd，并且prd和duty是非标准非线性的，建议用示波器看着来调
int pwm_led_output_clk(u8 gpio, u8 prd, u8 duty)
{
    RESET_PWM_CON0;
    RESET_PWM_CON1;
    RESET_PWM_CON2;
    RESET_PWM_CON3;

    if (power_param.config == SLEEP_EN && power_param.osc_type == OSC_TYPE_LRC) {
        g_printf(">>>PWM_LED_CLK_RC32K");
        pwm_clock_set(PWM_LED_CLK_RC32K);
        LED_PWM0_CLK_DIV(0);
    } else {
        pwm_clock_set(PWM_LED_CLK_BTOSC_24M);
        g_printf(">>>PWM_LED_CLK_BTOSC_24M");
        LED_PWM0_CLK_DIV(CLK_DIV_x2_x256(CLK_DIV_1));
    }

    duty = duty > prd ? prd : duty;

    LED_BRI_DUTY_CYCLE(prd);
    LED_BRI_DUTY1_SET(duty);
    LED_BRI_DUTY0_SET(duty);

    LED_PWM1_INV_ENABLE;

    /* LED_PWM1_DUTY0_SET(1); */
    /* LED_PWM1_DUTY0_EN; */

    LED_PWM_OUT_LOGIC_SET;

    LED_PWM1_CLK_DIV(200);
    led_pin_set_enable(gpio);
    LED_PWM_ENABLE;

    return 0;
}

void log_pwm_led_info()
{
    led_debug("======== PWM LED CONFIG ======");
    led_debug("P3_CLK_CON0 	= 0x%x", p33_rx_1byte(P3_CLK_CON0));
    led_debug("PWM_CON0 	= 0x%x", JL_PLED->CON0);
    led_debug("PWM_CON1 	= 0x%x", JL_PLED->CON1);
    led_debug("PWM_CON2 	= 0x%x", JL_PLED->CON2);
    led_debug("PWM_CON3 	= 0x%x", JL_PLED->CON3);
    led_debug("PRD_DIVL     = 0x%x", JL_PLED->PRD_DIVL);

    led_debug("BRI_PRDL      = 0x%x", JL_PLED->BRI_PRDL);
    led_debug("BRI_PRDH      = 0x%x", JL_PLED->BRI_PRDH);

    led_debug("BRI_DUTY0L    = 0x%x", JL_PLED->BRI_DUTY0L);
    led_debug("BRI_DUTY0H    = 0x%x", JL_PLED->BRI_DUTY0H);

    led_debug("BRI_DUTY1L    = 0x%x", JL_PLED->BRI_DUTY1L);
    led_debug("BRI_DUTY1H    = 0x%x", JL_PLED->BRI_DUTY1H);

    led_debug("PWM1_DUTY0    = 0x%x", JL_PLED->DUTY0);
    led_debug("PWM1_DUTY1    = 0x%x", JL_PLED->DUTY1);
    led_debug("PWM1_DUTY2    = 0x%x", JL_PLED->DUTY2);
    led_debug("PWM1_DUTY3    = 0x%x", JL_PLED->DUTY3);
}


/**
 * @brief: pwm0 时钟设置
 * 默认RC = 32K,
 * 呼吸BT24M = 93750Hz
 * 普通BT24M = 46875Hz/23437Hz
 *
 * @param: clk0
 * @return int
 */
static int _led_pwm0_clk_set(enum _PWM0_CLK clk0)
{
    u8 pwm0_clk_div_val = 0;

    if (__this->clock == PWM_LED_CLK_RC32K) {
        if (clk0 != PWM0_CLK_32K) {
            return -1;
        }
        pwm0_clk_div_val = CLK_DIV_1;  		//RC32k div 1 = 32k
    } else {
#if CONFIG_FPGA_ENABLE
        //12M
        if (clk0 == PWM0_CLK_46K) {
            pwm0_clk_div_val = CLK_DIV_x256(CLK_DIV_1);  //12M div 256 = 46875Hz
        } else if (clk0 == PWM0_CLK_23K) {
            pwm0_clk_div_val = CLK_DIV_x256(CLK_DIV_x2(CLK_DIV_1));  //12M div 512 = 23437Hz
        } else {
            return -1;
        }
#else
        if (clk0 == PWM0_CLK_46K) {
            pwm0_clk_div_val = CLK_DIV_x256(CLK_DIV_x2(CLK_DIV_1));  //24M div 512 = 46875Hz
        } else if (clk0 == PWM0_CLK_23K) {
            pwm0_clk_div_val = CLK_DIV_x256(CLK_DIV_4);  //24M div 1024 = 23437Hz
        } else {
            return -1;
        }
#endif /* #if CONFIG_FPGA_ENABLE */
    }

    LED_PWM0_CLK_DIV(pwm0_clk_div_val);

    return 0;
}



static void led_pwm_pre_set()
{
    LED_PWM_DISABLE;
    LED_PWM_BREATHE_DISABLE;
}


#ifdef PWM_LED_TWO_IO_SUPPORT

static void _change_io_display(void)
{
    u8 disable_pin = 0;
    u8 enable_pin = 0;
    if (__this->display_index == 0) {
        enable_pin = __this->user_data->io_cfg.two_io.pin1;
        disable_pin = __this->user_data->io_cfg.two_io.pin0;
        __this->display_index = 1;
    } else {
        enable_pin = __this->user_data->io_cfg.two_io.pin0;
        disable_pin = __this->user_data->io_cfg.two_io.pin1;
        __this->display_index = 0;
    }

    led_pin_set_disable(disable_pin);
    led_pin_set_enable(enable_pin);
}

static void pwm_led_two_io_mode_display(u8 display)
{
    u8 change_mode = 0;
    u8 isr_mode = 0;

    led_pin_set_disable(__this->user_data->io_cfg.two_io.pin0);
    led_pin_set_disable(__this->user_data->io_cfg.two_io.pin1);

    switch (display) {
    case PWM_LED_ALL_OFF:
    case PWM_LED0_OFF:
    case PWM_LED1_OFF:
        _pwm_led_display_mode(PWM_LED_ALL_OFF);
        break;

    case PWM_LED1_ON:
    case PWM_LED1_SLOW_FLASH:
    case PWM_LED1_FAST_FLASH:
    case PWM_LED1_ONE_FLASH_5S:
    case PWM_LED1_DOUBLE_FLASH_5S:
    case PWM_LED1_BREATHE:
        led_pin_set_enable(__this->user_data->io_cfg.two_io.pin1);
        _pwm_led_display_mode(display);
        break;

    case PWM_LED0_ON:
        change_mode = PWM_LED1_ON;
        break;
    case PWM_LED0_SLOW_FLASH:
        change_mode = PWM_LED1_SLOW_FLASH;
        break;
    case PWM_LED0_FAST_FLASH:
        change_mode = PWM_LED1_FAST_FLASH;
        break;
    case PWM_LED0_ONE_FLASH_5S:
        change_mode = PWM_LED1_ONE_FLASH_5S;
        break;
    case PWM_LED0_DOUBLE_FLASH_5S:
        change_mode = PWM_LED1_DOUBLE_FLASH_5S;
        break;
    case PWM_LED0_BREATHE:
        change_mode = PWM_LED1_BREATHE;
        break;
    case PWM_LED_ALL_ON:
        _pwm_led_display_mode(PWM_LED1_ON);
        led_pin_set_enable(__this->user_data->io_cfg.two_io.pin0);
        led_pin_set_enable(__this->user_data->io_cfg.two_io.pin1);
        break;
/////////////

//双灯互闪
    case PWM_LED0_LED1_FAST_FLASH: //使用中断切灯
        isr_mode = PWM_LED1_FAST_FLASH;
        break;
    case PWM_LED0_LED1_SLOW_FLASH:
        isr_mode = PWM_LED1_SLOW_FLASH;
        break;
//呼吸模式
    case PWM_LED0_LED1_BREATHE: //使用中断切灯
        isr_mode = PWM_LED1_BREATHE;
        break;

    default:
        break;
    }
    if (change_mode) {
        _pwm_led_display_mode(change_mode);
        LED_PWM_DISABLE;
        LED_BRI_DUTY1_SYNC0();
        led_pin_set_enable(__this->user_data->io_cfg.two_io.pin0);
        LED_PWM_ENABLE;
    }
    if (isr_mode) {
        _pwm_led_display_mode(isr_mode);
        _led_pwm1_duty_set(0xFF, 2, 0, 0, 0); //占满整个周期
        _pwm_led_register_irq(_change_io_display, 0);
        led_pin_set_enable(__this->user_data->io_cfg.two_io.pin0);
        __this->display_index = 0;
    }
}

static void _pwm_led_two_io_user_define_mode(u8 led_index)
{
    led_pwm_pre_set(); //led disable
    led_pin_set_disable(__this->user_data->io_cfg.two_io.pin0);
    led_pin_set_disable(__this->user_data->io_cfg.two_io.pin1);

    if (led_index == 0) {
        led_pin_set_enable(__this->user_data->io_cfg.two_io.pin0);
    } else if (led_index == 1) {
        led_pin_set_enable(__this->user_data->io_cfg.two_io.pin1);
    } else if (led_index == 2) {
        //双灯互闪切换
        __this->display_index = 0;
        led_pin_set_enable(__this->user_data->io_cfg.two_io.pin0);
        _pwm_led_register_irq(_change_io_display, 0);
    }
}


#else
static void pwm_led_two_io_mode_display(u8 display)
{
    return;
}
#endif /* #ifdef PWM_LED_TWO_IO_SUPPORT */

static void pwm_led_one_io_mode_display(u8 display)
{
    _pwm_led_display_mode(display);
}

/////////////////////////////
/**
 * @brief: 设置led灯亮度
 * @param: bri_max, 最大亮度, 10bit, 0 ~ 1023
 * @param: bri_duty0, LED0亮度, 10bit, 0 ~ 1023
 * @param: bri_duty1, LED1亮度, 10bit, 0 ~ 1023
 *
 * @return void
 */
static void _led_pwm_bright_set(u16 bri_max, u16 bri_duty0, u16 bri_duty1)
{
    bri_max = bri_max >= 1024 ? 1023 : bri_max;
    bri_duty0 = bri_duty0 >= 1024 ? 1023 : bri_duty0;
    bri_duty1 = bri_duty1 >= 1024 ? 1023 : bri_duty1;

    bri_duty0 = bri_duty0 >= bri_max ? bri_max : bri_duty0;
    bri_duty1 = bri_duty1 >= bri_max ? bri_max : bri_duty1;

    LED_BRI_DUTY_CYCLE(bri_max);
    LED_BRI_DUTY0_SET(bri_duty0);
    LED_BRI_DUTY1_SET(bri_duty1);
}


/**
 * @brief: 设置PWM输出逻辑,
 * @param: pwm_inv_en, 最后pwm波形输出逻辑(默认是高电平灯亮),
			0: 不取反, 高电平灯亮; 1: 取反, 低电平灯亮;
 * @param: shift_num
 	是否需要互闪,
			0: 单闪; 1 ~ : 互闪;
 *
 * @return void
 */
static void _led_pwm_output_logic_set(u8 pwm_inv_en, u8 shift_num)
{
    if (pwm_inv_en) {
        LED_PWM_OUTPUT_INV_ENABLE;
    } else {
        LED_PWM_OUTPUT_INV_DISABLE;
    }
    LED_PWM_OUT_LOGIC_SET;
    LED_PWM_SHIFT_DUTY_SET(shift_num);
}

/**
 * @brief: 设置PWM1亮灭设置, 可以实现一个周期亮灭1次和2次,
 * @param: pwm_inv_en, 最后pwm波形输出逻辑(默认是高电平灯亮),
			0: 不取反, 高电平灯亮; 1: 取反, 低电平灯亮;
 * @param: shift_num
 	是否需要互闪,
			0: 单闪; 1 ~ : 互闪;
 *duty_cycle 固定为255或者设置PWM_DUTY3, 8bit
 *   	 	  _duty1_	 	  _duty3_
 *		      |     |         |     |
 *		      |     |         |     |
 *0__ __ duty0|     | __ duty2|     |__ __255/PWM_DUTY3
 * @return void
 */
static void _led_pwm1_duty_set(u8 duty_prd, u8 duty0, u8 duty1, u8 duty2, u8 breathe)
{
    if (duty_prd != 0xFF) {
        if (breathe == 0) {
            duty0 = duty0 > duty_prd ? duty_prd : duty0;
            duty1 = duty1 > duty_prd ? duty_prd : duty1;
            duty2 = duty2 > duty_prd ? duty_prd : duty2;
            LED_PWM1_PRD_SEL_EN;
            LED_PWM1_DUTY3_DIS;
        } else {
            LED_PWM1_PRD_SEL_DIS; //呼吸模式, duty0/1是亮灯延时, {duty3,duty2}是灭灯延时
            LED_PWM1_DUTY3_EN;
        }

        LED_PWM1_DUTY3_SET(duty_prd);
    } else {
        LED_PWM1_PRD_SEL_DIS;
        LED_PWM1_DUTY3_DIS;
    }
    if (duty0) {
        LED_PWM1_DUTY0_SET(duty0);
        LED_PWM1_DUTY0_EN;
    } else {
        LED_PWM1_DUTY0_DIS;
    }
    if (duty1) {
        LED_PWM1_DUTY1_SET(duty1);
        LED_PWM1_DUTY1_EN;
    } else {
        LED_PWM1_DUTY1_DIS;
    }
    if (duty2) {
        LED_PWM1_DUTY2_SET(duty2);
        LED_PWM1_DUTY2_EN;
    } else {
        LED_PWM1_DUTY2_DIS;
    }
}


/**
 * @brief:
 * @param: module_en = 0, breathe_en = 0, 关LED模块
 * @param: module_en = 0, breathe_en = 1, 关LED模块
 * @param: module_en = 1, breathe_en = 0, 开LED普通闪烁模式
 * @param: module_en = 1, breathe_en = 1, 开LED呼吸模式
 * @return void
 */
static void _led_pwm_module_enable(u8 module_en, u8 breathe_en)
{
    if (breathe_en) {
        LED_PWM_BREATHE_ENABLE;
    } else {
        LED_PWM_BREATHE_DISABLE;
    }

    if (module_en) {
        LED_PWM_ENABLE;
    } else {
        LED_PWM_DISABLE;
    }
}

/**
 * @brief: 设置pwm0时钟分频
 *          clk0_div
 * pwm0_clk --------> pwm1_clk
 * @param: clk0_div, 12bit, 0 ~ 4095
 *
 * @return void
 */
static void _led_pwm1_clk_set(u16 clk0_div)
{
    clk0_div = clk0_div > 4096 ? 4096 : clk0_div;
    LED_PWM1_CLK_DIV(clk0_div);
}

/**
 * @brief: 关 led 配置, 亮度也设置为0
 *
 * @param led_index: 0: led0, 1:led1, 3:led0 & led1
 * @param led0_bright: 0 ~ 100
 * @param led1_bright: 0 ~ 100
 *
 * @return void
 */
static void _pwm_led_off_display(void)
{
    //TODO: set bright duty 0: avoid set on case
    led_pwm_pre_set(); //led disable
    _led_pwm_bright_set(0xFF, 0, 0);
}



/**
 * @brief: led 常亮显示设置
 * @param led_index: 0: led0, 1:led1, 3:led0 & led1
 * @param led0_bright, LED0亮度: 0 ~ 500
 * @param led1_bright, LED1亮度: 0 ~ 500
 *
 * @return void
 */
static void _pwm_led_on_display(u8 led_index, u16 led0_bright, u16 led1_bright)
{
    //step1: pwm0 clock
    if (__this->clock == PWM_LED_CLK_RC32K) {
        _led_pwm0_clk_set(PWM0_CLK_32K);
    } else {
        _led_pwm0_clk_set(PWM0_CLK_46K);
    }
    //step2: pwm1 clock
    _led_pwm1_clk_set(4);

    //bright set
    u8 shift_num = 0;
    u8 out_inv = 0;
    u16 led0_bri_duty = 0;
    u16 led1_bri_duty = 0;
    u16 led_bri_prd = 200;

    led0_bri_duty = led0_bright;
    led1_bri_duty = led1_bright;

    switch (led_index) {
    case 0:
        out_inv = 1;
        break;
    case 1:
        break;
    case 2:
        shift_num = 1;
        led_bri_prd = 160;
        break;
    default:
        led_debug("%s led index err", __func__);
        return;
        break;
    }

    led0_bri_duty = (led0_bri_duty * led_bri_prd) / 500;
    led1_bri_duty = (led1_bri_duty * led_bri_prd) / 500; //调试数据

    //step3: bright亮度
    _led_pwm_bright_set(led_bri_prd, led0_bri_duty, led1_bri_duty);

    //step4: 1.输出取反, 2.变色(互闪);
    _led_pwm_output_logic_set(out_inv, shift_num);

    //step5: 周期亮灭配置
    //pwm1 duty0, duty1, duty2
    _led_pwm1_duty_set(40, 2, 0, 0, 0);

    //step6: enable led module
    _led_pwm_module_enable(1, 0);
}

static void __pwm_led_flash_common_handle(u8 led_index, u16 led0_bright, u16 led1_bright, u32 period)
{
//step1: pwm0 clock
    u16 pwm0_prd = 500;
    u16 pwm0_div = 0;
    if (__this->clock == PWM_LED_CLK_RC32K) {
        _led_pwm0_clk_set(PWM0_CLK_32K);
        pwm0_div = (PWM0_RC_CLK32K_VALUE * period) / (1000 * 256);
    } else {
        if (period < 22000) {
            _led_pwm0_clk_set(PWM0_CLK_46K);
            pwm0_div = (PWM0_BT_CLK46K_VALUE * period) / (1000 * 256);
        } else {
            _led_pwm0_clk_set(PWM0_CLK_23K);
            pwm0_div = (PWM0_BT_CLK23K_VALUE * period) / (1000 * 256);
            pwm0_prd = 300;
            led0_bright = (led0_bright * 300) / 500;
            led1_bright = (led1_bright * 300) / 500;
        }
    }
    //step2: pwm1 clock
    _led_pwm1_clk_set(pwm0_div);

    //bright set
    u8 shift_num = 0;
    u8 out_inv = 0;
    u16 led0_bri_duty = 0;
    u16 led1_bri_duty = 0;

    switch (led_index) {
    case 0:
        led0_bri_duty = led0_bright;
        led1_bri_duty = 0;
        out_inv = 1;
        break;

    case 1:
        led0_bri_duty = 0;
        led1_bri_duty = led1_bright;
        break;
    case 2:
        shift_num = 1;
        led0_bri_duty = led0_bright;
        led1_bri_duty = led1_bright;
        break;
    default:
        led_debug("%s led index err", __func__);
        return;
        break;
    }

    //step3: bright亮度
    _led_pwm_bright_set(pwm0_prd, led0_bri_duty, led1_bri_duty);

    //step4: 1.输出取反, 2.变色(互闪);
    _led_pwm_output_logic_set(out_inv, shift_num);
}

/**
 * @brief: led 周期闪一次显示设置
 * @param  led_index: 0: led0, 1:led1, 3:led0 & led1(互闪)
		  	led0_bright: led0亮度(0 ~ 500),
		  	led1_bright: led1亮度(0 ~ 500),
		  	period: 闪灯周期(ms), 多少ms闪一下(100 ~ 20000), 100ms - 20S,
			start_light_time: 在周期中开始亮灯的时间, -1: 周期最后亮灯, 默认填-1即可,
			light_time: 灯亮持续时间,
 *
 * @return void
 */
static void _pwm_led_one_flash_display(u8 led_index, u16 led0_bright, u16 led1_bright,
                                       u32 period, u32 start_light_time, u32 light_time)
{
    __pwm_led_flash_common_handle(led_index, led0_bright, led1_bright, period);
    //step5: 周期亮灭配置
    //pwm1 duty0, duty1, duty2
    u8 pwm1_duty0 = 0;
    u8 pwm1_duty1 = 0;
    if (start_light_time != -1) {
        if (start_light_time >= period) {
            led_err("start_light_time config err");
            _pwm_led_off_display(); //led off
            return;
        }
        //指定从哪个时间亮,
        pwm1_duty0 = (256 * start_light_time) / period;
        pwm1_duty0 = (pwm1_duty0) ? pwm1_duty0 : 2;
        if ((start_light_time + light_time) > period) {
            pwm1_duty1 = 0; //只开duty0
        } else {
            pwm1_duty1 = (256 * light_time) / period;
            pwm1_duty1 = (pwm1_duty1) ? pwm1_duty1 : 2;
            if ((pwm1_duty0 + pwm1_duty1) > 0xFF) {
                pwm1_duty1 = 0 ;
            } else {
                pwm1_duty1 += pwm1_duty0;
            }
        }
    } else {
        pwm1_duty0 = (256 * light_time) / period;
        pwm1_duty0 = (pwm1_duty0) ? pwm1_duty0 : 2;
        pwm1_duty0 = 256 - pwm1_duty0;
    }

    if ((led_index == 2) && (light_time == -1)) { //互闪, 占满整个周期
        pwm1_duty0 = 2;
        pwm1_duty1 = 0;
    }
    _led_pwm1_duty_set(0xFF, pwm1_duty0, pwm1_duty1, 0, 0);

    //step6: enable led module
    _led_pwm_module_enable(1, 0);
}


/**
 * @brief: led 周期闪一次显示设置
 * @param  led_index: 0: led0, 1:led1, 3:led0 & led1(互闪)
		  	led0_bright: led0亮度,
		  	led1_bright: led1亮度,
		  	period: 闪灯周期(ms), 多少ms闪一下
			first_light_time: 第一次亮灯持续时间,
			second_light_time: 第二次亮灯持续时间,
			gap_time: 两次亮灯时间间隔,
 * @param led0_bright, LED0亮度: 0 ~ 500
 * @param led1_bright, LED1亮度: 0 ~ 500
 *
 * @return void
 */
static void _pwm_led_double_flash_display(u8 led_index, u16 led0_bright, u16 led1_bright,
        u32 period, u32 first_light_time, u32 gap_time, u32 second_light_time)
{
    __pwm_led_flash_common_handle(led_index, led0_bright, led1_bright, period);

    //step5: 周期亮灭配置
    //pwm1 duty0, duty1, duty2
    u8 pwm1_duty0 = 0;
    u8 pwm1_duty1 = 0;
    u8 pwm1_duty2 = 0;

    pwm1_duty2 = (256 * second_light_time) / period;
    pwm1_duty2 = (pwm1_duty2) ? (0xFF - pwm1_duty2) : (0xFF - 2);

    pwm1_duty1 = (256 * gap_time) / period;
    pwm1_duty1 = (pwm1_duty1) ? (pwm1_duty2 - pwm1_duty1) : (pwm1_duty2 - 2);

    pwm1_duty0 = (256 * first_light_time) / period;
    pwm1_duty0 = (pwm1_duty0) ? (pwm1_duty1 - pwm1_duty0) : (pwm1_duty1 - 2);

    _led_pwm1_duty_set(0xFF, pwm1_duty0, pwm1_duty1, pwm1_duty2, 0);

    //step6: enable led module
    _led_pwm_module_enable(1, 0);
}

/**
 * @brief: led 周期呼吸显示,
 * @param  led_index: 0: led0, 1:led1, 2:led0 & led1(交互呼吸)
			breathe_time: 呼吸周期(灭->最亮->灭), 设置范围: 500ms以上;
		   led0_bright: led0呼吸到最亮的亮度(0 ~ 500);
		   led1_bright: led1呼吸到最亮的亮度(0 ~ 500);
		   led0_light_delay_time: led0最高亮度延时(0 ~ 100ms);
		   led1_light_delay_time: led1最高亮度延时(0 ~ 100ms);
		   led_blink_delay_time: led0和led1灭灯延时(0 ~ 20000ms), 0 ~ 20S;
 *
 * @return void
 */
static void _pwm_led_breathe_display(u8 led_index, u16 breathe_time, u16 led0_bright, u16 led1_bright,
                                     u32 led0_light_delay_time, u32 led1_light_delay_time, u32 led_blink_delay_time)
{
    u16 led0_bri_duty = led0_bright;
    u16 led1_bri_duty = led1_bright;
    u16 pwm1_div = 0;
    u16 Tpwm1 = 0;
    pwm1_div = led0_bri_duty > led1_bri_duty ? led0_bri_duty : led1_bri_duty;

    breathe_time = breathe_time / 2; //呼吸总时间, 单个灭到最亮的时间
    //step1: pwm0 clock
    if (__this->clock == PWM_LED_CLK_RC32K) {
        _led_pwm0_clk_set(PWM0_CLK_32K);
        pwm1_div = breathe_time * 32 / pwm1_div;
        Tpwm1 = (pwm1_div * 1000 / 32000); //ms
    } else {
        _led_pwm0_clk_set(PWM0_CLK_46K);
        pwm1_div = breathe_time * 46 / pwm1_div;
        Tpwm1 = (pwm1_div * 1000 / 46000); //ms
    }
    //step2: pwm1 clock
    _led_pwm1_clk_set(pwm1_div);

    //bright set
    u8 shift_num = 0;
    u8 out_inv = 0;

    switch (led_index) {
    case 0:
        led1_bri_duty = 0;
        out_inv = 1;
        break;

    case 1:
        led0_bri_duty = 0;
        break;
    case 2:
        shift_num = 2;
        break;
    default:
        led_debug("%s led index err", __func__);
        return;
    }

    //step3: bright亮度
    _led_pwm_bright_set(500, led0_bri_duty, led1_bri_duty);

    //step4: 1.输出取反, 2.变色(互闪);
    _led_pwm_output_logic_set(out_inv, shift_num);

    //step5: 周期亮灭配置
    //pwm1 duty0, duty1, duty2
    u8 pwm1_duty0 = 0;
    u8 pwm1_duty1 = 0;
    u8 pwm1_duty2 = 0;
    u8 pwm1_duty3 = 0xFF;

    if (Tpwm1 == 0) {
        led0_light_delay_time *= 2;
        led1_light_delay_time *= 2;
        led_blink_delay_time *= 2;
        Tpwm1 = 1;
    }

    //最高亮度延时
    pwm1_duty0 = led0_light_delay_time / Tpwm1;
    pwm1_duty1 = led1_light_delay_time / Tpwm1;
    //灭灯延时,{duty3, duty2}, 16bit
    pwm1_duty2 = (led_blink_delay_time / Tpwm1) & 0xFF;
    pwm1_duty3 = ((led_blink_delay_time / Tpwm1) >> 8) & 0xFF;

    _led_pwm1_duty_set(pwm1_duty3, pwm1_duty0, pwm1_duty1, pwm1_duty2, 1);

    //step6: enable led module
    _led_pwm_module_enable(1, 1);
}

static void _pwm_led_display_mode(u8 display)
{
    led_pwm_pre_set();

    switch (display) {

#if PWM_LED_NEW_FORMAT_EN
    case PWM_LED_ALL_OFF:
    case PWM_LED0_OFF:
    case PWM_LED1_OFF:
        _pwm_led_off_display();
        break;
//灯常亮
    case PWM_LED0_ON:
        printf("led0 on:%d %d \n", led_para.on.led0_bright, led_para.on.led1_bright);
        _pwm_led_on_display(0, led_para.on.led0_bright, led_para.on.led1_bright);
        break;
    case PWM_LED1_ON:
        printf("led1 on:%d %d \n", led_para.on.led0_bright, led_para.on.led1_bright);
        _pwm_led_on_display(1, led_para.on.led0_bright, led_para.on.led1_bright);
        break;
    case PWM_LED_ALL_ON:
        printf("led0_led1 on:%d %d \n", led_para.on.led0_bright, led_para.on.led1_bright);
        _pwm_led_on_display(2, led_para.on.led0_bright, led_para.on.led1_bright);
        break;

//单灯单闪
    case PWM_LED0_SLOW_FLASH:
        printf("led0 slow:%d %d %d %d %d \n", led_para.one_flash.led0_bright, led_para.one_flash.led1_bright, \
               led_para.one_flash.period, led_para.one_flash.start_light_time, \
               led_para.one_flash.light_time);
        _pwm_led_one_flash_display(0, \
                                   led_para.one_flash.led0_bright, led_para.one_flash.led1_bright, \
                                   led_para.one_flash.period, led_para.one_flash.start_light_time, \
                                   led_para.one_flash.light_time);
        break;
    case PWM_LED1_SLOW_FLASH:
        printf("led1 slow:%d %d %d %d %d \n", led_para.one_flash.led0_bright, led_para.one_flash.led1_bright, \
               led_para.one_flash.period, led_para.one_flash.start_light_time, \
               led_para.one_flash.light_time);
        _pwm_led_one_flash_display(1, \
                                   led_para.one_flash.led0_bright, led_para.one_flash.led1_bright, \
                                   led_para.one_flash.period, led_para.one_flash.start_light_time, \
                                   led_para.one_flash.light_time);

        break;
    case PWM_LED0_FAST_FLASH:
        printf("led0 fast:%d %d %d %d %d \n", led_para.one_flash.led0_bright, led_para.one_flash.led1_bright, \
               led_para.one_flash.period, led_para.one_flash.start_light_time, \
               led_para.one_flash.light_time);
        _pwm_led_one_flash_display(0, \
                                   led_para.one_flash.led0_bright, led_para.one_flash.led1_bright, \
                                   led_para.one_flash.period, led_para.one_flash.start_light_time, \
                                   led_para.one_flash.light_time);

        break;
    case PWM_LED1_FAST_FLASH:
        printf("led1 fast:%d %d %d %d %d \n", led_para.one_flash.led0_bright, led_para.one_flash.led1_bright, \
               led_para.one_flash.period, led_para.one_flash.start_light_time, \
               led_para.one_flash.light_time);
        _pwm_led_one_flash_display(1, \
                                   led_para.one_flash.led0_bright, led_para.one_flash.led1_bright, \
                                   led_para.one_flash.period, led_para.one_flash.start_light_time, \
                                   led_para.one_flash.light_time);

        break;
    case PWM_LED0_ONE_FLASH_5S:
        printf("led0 one 5s:%d %d %d %d %d \n", led_para.one_flash.led0_bright, led_para.one_flash.led1_bright, \
               led_para.one_flash.period, led_para.one_flash.start_light_time, \
               led_para.one_flash.light_time);
        _pwm_led_one_flash_display(0, \
                                   led_para.one_flash.led0_bright, led_para.one_flash.led1_bright, \
                                   led_para.one_flash.period, led_para.one_flash.start_light_time, \
                                   led_para.one_flash.light_time);
        break;
    case PWM_LED1_ONE_FLASH_5S:
        printf("led1 one 5s:%d %d %d %d %d \n", led_para.one_flash.led0_bright, led_para.one_flash.led1_bright, \
               led_para.one_flash.period, led_para.one_flash.start_light_time, \
               led_para.one_flash.light_time);
        _pwm_led_one_flash_display(1, \
                                   led_para.one_flash.led0_bright, led_para.one_flash.led1_bright, \
                                   led_para.one_flash.period, led_para.one_flash.start_light_time, \
                                   led_para.one_flash.light_time);
        break;
//双灯互闪
    case PWM_LED0_LED1_FAST_FLASH:
        printf("led0_led1 fast flash:%d %d %d %d %d \n", led_para.one_flash.led0_bright, led_para.one_flash.led1_bright, \
               led_para.one_flash.period, led_para.one_flash.start_light_time, \
               led_para.one_flash.light_time);
        _pwm_led_one_flash_display(2, \
                                   led_para.one_flash.led0_bright, led_para.one_flash.led1_bright, \
                                   led_para.one_flash.period, led_para.one_flash.start_light_time, \
                                   led_para.one_flash.light_time);
        break;
    case PWM_LED0_LED1_SLOW_FLASH:
        printf("led0_led1 slow flash:%d %d %d %d %d \n", led_para.one_flash.led0_bright, led_para.one_flash.led1_bright, \
               led_para.one_flash.period, led_para.one_flash.start_light_time, \
               led_para.one_flash.light_time);
        _pwm_led_one_flash_display(2, \
                                   led_para.one_flash.led0_bright, led_para.one_flash.led1_bright, \
                                   led_para.one_flash.period, led_para.one_flash.start_light_time, \
                                   led_para.one_flash.light_time);
        break;

//单灯双闪
    case PWM_LED0_DOUBLE_FLASH_5S:
        printf("led0 double 5s:%d %d %d %d %d %d \n", led_para.double_flash.led0_bright, led_para.double_flash.led1_bright, \
               led_para.double_flash.period, led_para.double_flash.first_light_time, \
               led_para.double_flash.gap_time, led_para.double_flash.second_light_time);
        _pwm_led_double_flash_display(2, \
                                      led_para.double_flash.led0_bright, led_para.double_flash.led1_bright, \
                                      led_para.double_flash.period, led_para.double_flash.first_light_time, \
                                      led_para.double_flash.gap_time, led_para.double_flash.second_light_time);
        break;
    case PWM_LED1_DOUBLE_FLASH_5S:
        printf("led1 double 5s:%d %d %d %d %d %d \n", led_para.double_flash.led0_bright, led_para.double_flash.led1_bright, \
               led_para.double_flash.period, led_para.double_flash.first_light_time, \
               led_para.double_flash.gap_time, led_para.double_flash.second_light_time);
        _pwm_led_double_flash_display(2, \
                                      led_para.double_flash.led0_bright, led_para.double_flash.led1_bright, \
                                      led_para.double_flash.period, led_para.double_flash.first_light_time, \
                                      led_para.double_flash.gap_time, led_para.double_flash.second_light_time);
        break;

//呼吸模式
    case PWM_LED0_BREATHE:
        printf("led0 breathe:%d %d %d %d %d %d \n", led_para.breathe.breathe_time, led_para.breathe.led0_bright, \
               led_para.breathe.led1_bright, led_para.breathe.led0_light_delay_time, \
               led_para.breathe.led1_light_delay_time, led_para.breathe.led_blink_delay_time);
        _pwm_led_breathe_display(0, \
                                 led_para.breathe.breathe_time, led_para.breathe.led0_bright, \
                                 led_para.breathe.led1_bright, led_para.breathe.led0_light_delay_time, \
                                 led_para.breathe.led1_light_delay_time, led_para.breathe.led_blink_delay_time);
        break;
    case PWM_LED1_BREATHE:
        printf("led1 breathe:%d %d %d %d %d %d \n", led_para.breathe.breathe_time, led_para.breathe.led0_bright, \
               led_para.breathe.led1_bright, led_para.breathe.led0_light_delay_time, \
               led_para.breathe.led1_light_delay_time, led_para.breathe.led_blink_delay_time);
        _pwm_led_breathe_display(1, \
                                 led_para.breathe.breathe_time, led_para.breathe.led0_bright, \
                                 led_para.breathe.led1_bright, led_para.breathe.led0_light_delay_time, \
                                 led_para.breathe.led1_light_delay_time, led_para.breathe.led_blink_delay_time);
        break;
    case PWM_LED0_LED1_BREATHE:
        printf("led0_led1 breathe:%d %d %d %d %d %d \n", led_para.breathe.breathe_time, led_para.breathe.led0_bright, \
               led_para.breathe.led1_bright, led_para.breathe.led0_light_delay_time, \
               led_para.breathe.led1_light_delay_time, led_para.breathe.led_blink_delay_time);
        _pwm_led_breathe_display(2, \
                                 led_para.breathe.breathe_time, led_para.breathe.led0_bright, \
                                 led_para.breathe.led1_bright, led_para.breathe.led0_light_delay_time, \
                                 led_para.breathe.led1_light_delay_time, led_para.breathe.led_blink_delay_time);
        break;

#else
    case PWM_LED_ALL_OFF:
    case PWM_LED0_OFF:
    case PWM_LED1_OFF:
        _pwm_led_off_display();
        break;
//灯常亮
    case PWM_LED0_ON:
        _pwm_led_on_display(0, CFG_LED0_LIGHT, CFG_LED1_LIGHT);
        break;
    case PWM_LED1_ON:
        _pwm_led_on_display(1, CFG_LED0_LIGHT, CFG_LED1_LIGHT);
        break;

    case PWM_LED_ALL_ON:
        _pwm_led_on_display(2, CFG_LED0_LIGHT, CFG_LED1_LIGHT);
        break;
//单灯单闪
    case PWM_LED0_SLOW_FLASH:
        _pwm_led_one_flash_display(0, CFG_LED0_LIGHT, CFG_LED1_LIGHT, CFG_SINGLE_SLOW_FLASH_FREQ, -1, CFG_SINGLE_SLOW_LIGHT_TIME);
        break;
    case PWM_LED1_SLOW_FLASH:
        _pwm_led_one_flash_display(1, CFG_LED0_LIGHT, CFG_LED1_LIGHT, CFG_SINGLE_SLOW_FLASH_FREQ, -1, CFG_SINGLE_SLOW_LIGHT_TIME);
        break;
    case PWM_LED0_FAST_FLASH:
        _pwm_led_one_flash_display(0, CFG_LED0_LIGHT, CFG_LED1_LIGHT, CFG_SINGLE_FAST_FLASH_FREQ, -1, CFG_SINGLE_FAST_LIGHT_TIME);
        break;
    case PWM_LED1_FAST_FLASH:
        _pwm_led_one_flash_display(1, CFG_LED0_LIGHT, CFG_LED1_LIGHT, CFG_SINGLE_FAST_FLASH_FREQ, -1, CFG_SINGLE_FAST_LIGHT_TIME);
        break;
    case PWM_LED0_ONE_FLASH_5S:
        _pwm_led_one_flash_display(0, CFG_LED0_LIGHT, CFG_LED1_LIGHT, 5000, 10, CFG_LED_5S_FLASH_LIGHT_TIME);
        break;
    case PWM_LED1_ONE_FLASH_5S:
        _pwm_led_one_flash_display(1, CFG_LED0_LIGHT, CFG_LED1_LIGHT, 5000, 10, CFG_LED_5S_FLASH_LIGHT_TIME);
        break;
//单灯双闪
    case PWM_LED0_DOUBLE_FLASH_5S:
        _pwm_led_double_flash_display(0, CFG_LED0_LIGHT, CFG_LED1_LIGHT, 5000, 100, 200, 100);
        break;
    case PWM_LED1_DOUBLE_FLASH_5S:
        _pwm_led_double_flash_display(1, CFG_LED0_LIGHT, CFG_LED1_LIGHT, 5000, 100, 200, 100);
        break;

//双灯互闪
    case PWM_LED0_LED1_FAST_FLASH:
        _pwm_led_one_flash_display(2, CFG_LED0_LIGHT, CFG_LED1_LIGHT, CFG_DOUBLE_FAST_FLASH_FREQ, -1, -1);
        break;
    case PWM_LED0_LED1_SLOW_FLASH:
        _pwm_led_one_flash_display(2, CFG_LED0_LIGHT, CFG_LED1_LIGHT, CFG_DOUBLE_SLOW_FLASH_FREQ, -1, -1);
        break;

//呼吸模式
    case PWM_LED0_BREATHE:
        _pwm_led_breathe_display(0, CFG_LED_BREATH_TIME, CFG_LED0_BREATH_BRIGHT, CFG_LED1_BREATH_BRIGHT, 0, 0, CFG_LED_BREATH_BLINK_TIME);
        break;
    case PWM_LED1_BREATHE:
        _pwm_led_breathe_display(1, CFG_LED_BREATH_TIME, CFG_LED0_BREATH_BRIGHT, CFG_LED1_BREATH_BRIGHT, 0, 0, CFG_LED_BREATH_BLINK_TIME);
        break;
    case PWM_LED0_LED1_BREATHE:
        _pwm_led_breathe_display(2, CFG_LED_BREATH_TIME, CFG_LED0_BREATH_BRIGHT, CFG_LED1_BREATH_BRIGHT, 0, 0, CFG_LED_BREATH_BLINK_TIME);
        break;
#endif
    }
}

static void _pwm_led_user_define_mode_handle(u8 dis_mode)
{
    return;
}

//=================================================================================//
//                        		以下为 LED API                    				   //
//=================================================================================//


//=================================================================================//
//@brief: LED模式显示模式设置
//@input: display, 显示模式
//@return: void
//@note:
//=================================================================================//
void pwm_led_mode_set(u8 display)
{
    if (__this->init == 0) {
        led_debug("led no init");
        return;
    }

    led_debug("Mode %d Display", display);

    if (display == PWM_LED_NULL) {
        return;
    }

    if (((display >= PWM_LED_USER_DEFINE_BEGIN) && (display <= PWM_LED_USER_DEFINE_END))) {
        //用户自定义模式
        if (display != __this->last_mode) {
            _pwm_led_user_define_mode_handle(display);
        }
        return;
    }

    if ((display >= PWM_LED_MODE_END)/* || (display == __this->last_mode)*/) {
        return;
    }

    if ((display == PWM_LED_ALL_OFF) || (display == PWM_LED0_OFF) || (display == PWM_LED1_OFF)) {
        led_pwm_pre_set(); 	//关LED
        __this->last_mode = display;
        return;
    }

    _pwm_led_close_irq();

    if (__this->user_data->io_mode == LED_TWO_IO_MODE) {
        pwm_led_two_io_mode_display(display);
    } else {
        pwm_led_one_io_mode_display(display);
    }
    __this->last_mode = display;
    log_pwm_led_info();
}

//=================================================================================//
//@brief: LED模块时钟源选择
//@input: src, 时钟源BT24M/RC32K
//@return: void
//@note:
//=================================================================================//
void pwm_led_clk_set(enum pwm_led_clk_source src)
{
    u8 mode_bak;

    if (src == __this->clock) {
        return;
    }

    if (src != PWM_LED_CLK_RC32K && src != PWM_LED_CLK_BTOSC_24M) {
        return;
    }

    mode_bak = __this->last_mode;
    pwm_led_mode_set(PWM_LED_ALL_OFF);
    pwm_clock_set(src);
    pwm_led_mode_set(mode_bak);
}

//=================================================================================//
//@brief: LED显示周期复位, 重新开始一个周期, 可用于同步等操作
//@input: void
//@return: void
//@note:
//=================================================================================//
void pwm_led_display_mode_reset(void)
{
    /* u8 last_mode; */
    /* last_mode = __this->last_mode; */
    /* pwm_led_mode_set(PWM_LED_ALL_OFF); */
    /* pwm_led_mode_set(last_mode); */
    LED_PWM_DISABLE;
    LED_PWM_ENABLE;
}

//=================================================================================//
//@brief: 获取LED当前显示模式
//@input: void
//@return: 当前LED显示模式
//@note:
//=================================================================================//
enum pwm_led_mode pwm_led_display_mode_get(void)
{
    return __this->last_mode;
}

//=================================================================================//
//@brief: 修改LED灯IO口驱动能力,
//@input: void
//@return: 当前LED显示模式
//@note:
//挡位: 0 ~ 3
//	0: 2.4mA(8mA mos + 120Ωres)
//	1: 8mA(8mA mos)
// 	2: 18.4mA(24mA mos + 120Ωres)
// 	3: 24mA(24mA mos)
//=================================================================================//
void pwm_led_io_max_drive_set(u8 strength)
{
    LED_PWM_IO_MAX_DRIVE(strength);
}

//=================================================================================//
//@brief: 获取LED模块是否开启, 可用于sniff灯同步
//@input: void
//@return: 0: 模块开启; 1: 模块关闭
//@note:
//=================================================================================//
u8 is_pwm_led_on(void)
{
    return IS_PWM_LED_ON;
}

//=================================================================================//
//@brief: 获取LED模块开启, 可用于sniff灯同步
//@input: void
//@return: void
//@note:
//=================================================================================//
void pwm_led_set_on(void)
{
    LED_PWM_ENABLE;
}

//=================================================================================//
//@brief: 获取LED模块关闭, 可用于sniff灯同步
//@input: void
//@return: void
//@note:
//=================================================================================//
void pwm_led_set_off(void)
{
    LED_PWM_DISABLE;
}

//=================================================================================//
//@brief: 自定义设置单灯闪状态
//@input: void
//		led_index: 0: led0, 1:led1, 2:led0 & led1(互闪)
// 		led0_bright: led0亮度(0 ~ 500),
// 		led1_bright: led1亮度(0 ~ 500),
// 		period: 闪灯周期(ms), 多少ms闪一下,
// 		start_light_time: 在周期中开始亮灯的时间, -1: 周期最后亮灯, 默认填-1即可,
// 		light_time: 灯亮持续时间(ms),
//@return: void
//@note:
//=================================================================================//
void pwm_led_one_flash_display(u8 led_index, u16 led0_bright, u16 led1_bright,
                               u32 period, u32 start_light_time, u32 light_time)
{
    _pwm_led_close_irq();
    //两个IO特殊处理
    if (__this->user_data->io_mode == LED_TWO_IO_MODE) {
#ifdef PWM_LED_TWO_IO_SUPPORT
        _pwm_led_two_io_user_define_mode(led_index);
        if (led_index == 0) {
            led_index = 1;
            led1_bright = led0_bright;
        }
#endif /* #ifdef PWM_LED_TWO_IO_SUPPORT */
    }
    _pwm_led_one_flash_display(led_index, led0_bright, led1_bright,
                               period, start_light_time, light_time);
    __this->last_mode = PWM_LED_USER_DEFINE_MODE;
}


//=================================================================================//
//@brief: 自定义设置单灯双闪状态
//@input:
// 		led_index: 0: led0, 1:led1, 3:led0 & led1(互闪)
// 		led0_bright: led0亮度,
// 		led1_bright: led1亮度,
// 		period: 闪灯周期(ms), 多少ms闪一下
//  	first_light_time: 第一次亮灯持续时间,
// 		second_light_time: 第二次亮灯持续时间,
// 		gap_time: 两次亮灯时间间隔,
//		led0_bright, LED0亮度: 0 ~ 500
// 		led1_bright, LED1亮度: 0 ~ 500
//@return: void
//@note:
//=================================================================================//
void pwm_led_double_flash_display(u8 led_index, u16 led0_bright, u16 led1_bright,
                                  u32 period, u32 first_light_time, u32 gap_time, u32 second_light_time)
{
    _pwm_led_close_irq();
    //两个IO特殊处理
    if (__this->user_data->io_mode == LED_TWO_IO_MODE) {
#ifdef PWM_LED_TWO_IO_SUPPORT
        _pwm_led_two_io_user_define_mode(led_index);

        if (led_index == 0) {
            led_index = 1;
            led1_bright = led0_bright;
        }

        led_index = 0;
#endif /* #ifdef PWM_LED_TWO_IO_SUPPORT */
    }
    _pwm_led_double_flash_display(led_index, led0_bright, led1_bright,
                                  period, first_light_time, gap_time, second_light_time);

    __this->last_mode = PWM_LED_USER_DEFINE_MODE;
}


//=================================================================================//
//@brief: 自定义设置呼吸模式
//@input:
//		led_index: 0: led0, 1:led1, 2:led0 & led1(互闪)
// 		led0_bright: led0亮度,
// 		led1_bright: led1亮度,
// 		period: 闪灯周期(ms), 多少ms闪一下,
// 		start_light_time: 在周期中开始亮灯的时间, -1: 周期最后亮灯
// 		light_time: 灯亮持续时间,
//		led0_bright, LED0亮度: 0 ~ 500
// 		led1_bright, LED1亮度: 0 ~ 500
//@return: void
//@note:
//=================================================================================//
void pwm_led_breathe_display(u8 led_index, u16 breathe_time, u16 led0_bright, u16 led1_bright,
                             u32 led0_light_delay_time, u32 led1_light_delay_time, u32 led_blink_delay_time)
{
    _pwm_led_close_irq();
    if (__this->user_data->io_mode == LED_TWO_IO_MODE) {
#ifdef PWM_LED_TWO_IO_SUPPORT
        _pwm_led_two_io_user_define_mode(led_index);
        if (led_index == 0) {
            led_index = 1;
            led1_bright = led0_bright;
        }

#endif /* #ifdef PWM_LED_TWO_IO_SUPPORT */
    }
    _pwm_led_breathe_display(led_index, breathe_time, led0_bright, led1_bright,
                             led0_light_delay_time, led1_light_delay_time, led_blink_delay_time);

    __this->last_mode = PWM_LED_USER_DEFINE_MODE;
}


//=================================================================================//
//@brief: 注册LED周期中断函数, 每个LED周期结束后会调用一次, 可以统计指定状态闪烁多少次
//@input:
//@return: void
//@note:
//=================================================================================//
void pwm_led_register_irq(void (*func)(void))
{
    _pwm_led_register_irq(func, 1);
}

//=================================================================================//
//@brief: LED模式显示参数设置
//@input: display, 显示模式
//	   	  para 显示的参数
//@return: void
//@note:
//=================================================================================//

static void pwm_led_para_set(u8 display, pwm_led_para para)
{
    switch (display) {
    case PWM_LED_ALL_OFF:
    case PWM_LED0_OFF:
    case PWM_LED1_OFF:
        break;

//灯常亮
    case PWM_LED0_ON:
    case PWM_LED1_ON:
    case PWM_LED_ALL_ON:
        led_para.on = para.on;
        break;

//单灯单闪
    case PWM_LED0_SLOW_FLASH:
    case PWM_LED1_SLOW_FLASH:
    case PWM_LED0_FAST_FLASH:
    case PWM_LED1_FAST_FLASH:
    case PWM_LED0_ONE_FLASH_5S:
    case PWM_LED1_ONE_FLASH_5S:
//双灯互闪
    case PWM_LED0_LED1_FAST_FLASH:
    case PWM_LED0_LED1_SLOW_FLASH:
        led_para.one_flash = para.one_flash;
        break;

//单灯双闪
    case PWM_LED0_DOUBLE_FLASH_5S:
    case PWM_LED1_DOUBLE_FLASH_5S:
        led_para.double_flash = para.double_flash;
        break;

//呼吸模式
    case PWM_LED0_BREATHE:
    case PWM_LED1_BREATHE:
    case PWM_LED0_LED1_BREATHE:
        led_para.breathe = para.breathe;
        break;

    default:
        return;
        break;
    }
}
//=================================================================================//
//@brief: LED模式显示模式设置
//@input: display, 显示模式
//@return: void
//@note:
//=================================================================================//
void pwm_led_mode_set_with_para(u8 display, pwm_led_para para)
{
    pwm_led_para_set(display, para);
    pwm_led_mode_set(display);
}

