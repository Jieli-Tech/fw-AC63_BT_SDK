#include "app_config.h"

#ifdef CONFIG_BOARD_AC6351D_KEYBOARD

#include "system/includes.h"
#include "asm/charge.h"
#include "device/key_driver.h"
#include "asm/power/p33.h"
#include "asm/iic_soft.h"
#include "matrix_keyboard.h"
#include "SYD9557M.h"

#define LOG_TAG_CONST       BOARD
#define LOG_TAG             "[BOARD]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

void board_power_init(void);
const struct soft_iic_config soft_iic_cfg[] = {
    //iic0 data
    {
        .scl = TCFG_SW_I2C0_CLK_PORT,                    //IIC CLK脚
        .sda = TCFG_SW_I2C0_DAT_PORT,                   //IIC DAT脚
        .delay = TCFG_SW_I2C0_DELAY_CNT,                //软件IIC延时参数，影响通讯时钟频率
        .io_pu = 1,                                     //是否打开上拉电阻，如果外部电路没有焊接上拉电阻需要置1
    },
};

/************************** LOW POWER config ****************************/
const struct low_power_param power_param = {
    .config         = TCFG_LOWPOWER_LOWPOWER_SEL,          //0：sniff时芯片不进入低功耗  1：sniff时芯片进入powerdown
    .btosc_hz         = TCFG_CLOCK_OSC_HZ,                   //外接晶振频率
    .delay_us       = TCFG_CLOCK_SYS_HZ / 1000000L,        //提供给低功耗模块的延时(不需要需修改)
    .btosc_disable  = TCFG_LOWPOWER_BTOSC_DISABLE,         //进入低功耗时BTOSC是否保持
    .vddiom_lev     = TCFG_LOWPOWER_VDDIOM_LEVEL,          //强VDDIO等级,可选：2.0V  2.2V  2.4V  2.6V  2.8V  3.0V  3.2V  3.6V
    .vddiow_lev     = TCFG_LOWPOWER_VDDIOW_LEVEL,          //弱VDDIO等级,可选：2.1V  2.4V  2.8V  3.2V
    .osc_type       = OSC_TYPE_LRC,
};


/************************** KEY MSG****************************/
/*各个按键的消息设置，如果USER_CFG中设置了USE_CONFIG_KEY_SETTING为1，则会从配置文件读取对应的配置来填充改结构体*/
u8 key_table[KEY_NUM_MAX][KEY_EVENT_MAX] = {
    // SHORT           LONG              HOLD              UP              DOUBLE           TRIPLE
};


// *INDENT-OFF*
/************************** UART config****************************/
#if TCFG_UART0_ENABLE
UART0_PLATFORM_DATA_BEGIN(uart0_data)
    .tx_pin = TCFG_UART0_TX_PORT,                             //串口打印TX引脚选择
    .rx_pin = TCFG_UART0_RX_PORT,                             //串口打印RX引脚选择
    .baudrate = TCFG_UART0_BAUDRATE,                          //串口波特率

    .flags = UART_DEBUG,                                      //串口用来打印需要把改参数设置为UART_DEBUG
UART0_PLATFORM_DATA_END()
#endif //TCFG_UART0_ENABLE


    /************************** AD KEY ****************************/
#if TCFG_ADKEY_ENABLE
const struct adkey_platform_data adkey_data = {
    .enable = TCFG_ADKEY_ENABLE,                              //AD按键使能
    .adkey_pin = TCFG_ADKEY_PORT,                             //AD按键对应引脚
    .ad_channel = TCFG_ADKEY_AD_CHANNEL,                      //AD通道值
    .extern_up_en = TCFG_ADKEY_EXTERN_UP_ENABLE,              //是否使用外接上拉电阻
    .ad_value = {                                             //根据电阻算出来的电压值
        TCFG_ADKEY_VOLTAGE0,
        TCFG_ADKEY_VOLTAGE1,
        TCFG_ADKEY_VOLTAGE2,
        TCFG_ADKEY_VOLTAGE3,
        TCFG_ADKEY_VOLTAGE4,
        TCFG_ADKEY_VOLTAGE5,
        TCFG_ADKEY_VOLTAGE6,
        TCFG_ADKEY_VOLTAGE7,
        TCFG_ADKEY_VOLTAGE8,
        TCFG_ADKEY_VOLTAGE9,
    },
    .key_value = {                                             //AD按键各个按键的键值
        TCFG_ADKEY_VALUE0,
        TCFG_ADKEY_VALUE1,
        TCFG_ADKEY_VALUE2,
        TCFG_ADKEY_VALUE3,
        TCFG_ADKEY_VALUE4,
        TCFG_ADKEY_VALUE5,
        TCFG_ADKEY_VALUE6,
        TCFG_ADKEY_VALUE7,
        TCFG_ADKEY_VALUE8,
        TCFG_ADKEY_VALUE9,
    },
};
#endif

#if TCFG_MATRIX_KEY_ENABLE
static u32 key_row[] = {IO_PORTB_09, IO_PORTB_07, IO_PORTB_11, IO_PORTC_07,    IO_PORTB_10, IO_PORTC_06, IO_PORTB_06, IO_PORTB_08 };

static u32 key_col[] = {IO_PORTA_04, IO_PORTA_00, IO_PORTA_02, IO_PORTA_01,    IO_PORTA_03, IO_PORTA_05, IO_PORTA_08, IO_PORTA_06, \
         IO_PORTA_07, IO_PORTA_12, IO_PORTC_00, IO_PORTA_15,    IO_PORTC_01, IO_PORTB_05, IO_PORTA_13, IO_PORTA_14, \
         IO_PORTA_09, IO_PORTA_11, IO_PORTA_10,
       };
/* static u32 key_row[] = {IO_PORTB_06, IO_PORTB_07, IO_PORTB_08, IO_PORTB_09, IO_PORTB_10, IO_PORTB_11, IO_PORTC_06, IO_PORTC_07}; */
/* static u32 key_col[] = {IO_PORTA_00, IO_PORTA_01, IO_PORTA_02, IO_PORTA_03, IO_PORTA_04, IO_PORTA_05, IO_PORTA_06, IO_PORTA_07, \ */
/*                         IO_PORTA_08, IO_PORTA_09, IO_PORTA_10,  IO_PORTA_11, IO_PORTA_12, IO_PORTA_13, IO_PORTC_00, IO_PORTA_14, \ */
/*                         IO_PORTC_01, IO_PORTA_15, IO_PORTB_05, */
/*                        }; */

static matrix_key_param  matrix_param = {
    .row_pin_list = key_row,
    .col_pin_list = key_col,
    .row_num = sizeof(key_row) / sizeof(u32),
    .col_num = sizeof(key_col) / sizeof(u32),
};
#endif

void debug_uart_init(const struct uart_platform_data *data)
{
#if TCFG_UART0_ENABLE
    if (data) {
        uart_init(data);
    } else {
        uart_init(&uart0_data);
    }
#endif
}

static void board_devices_init(void)
{
#if TCFG_PWMLED_ENABLE
    pwm_led_init(&pwm_led_data);
#endif

#if (TCFG_IOKEY_ENABLE || TCFG_ADKEY_ENABLE || TCFG_TOUCH_KEY_ENABLE)
	key_driver_init();
#endif


#if TCFG_MATRIX_KEY_ENABLE
    matrix_key_init(&matrix_param);
#endif

#if TCFG_TOUCHPAD_ENABLE
    syd9557m_init(0);
#endif
}

extern void set_dcdc_ctl_port(u8 port);
extern void cfg_file_parse(u8 idx);
void board_init()
{
    board_power_init();
    adc_vbg_init();
    adc_init();
    cfg_file_parse(0);
    devices_init();

	board_devices_init();

	log_info("board_init");

	//必须在切换到dcdc前设置好
	if(TCFG_LOWPOWER_POWER_SEL == PWR_DCDC15){
		//set_dcdc_ctl_port(TCFG_DCDC_PORT_SEL);
	}else{
		//set_dcdc_ctl_port(NO_CONFIG_PORT);
	}

	if(get_charge_online_flag()){
		log_info("charge...select LDO15");
		power_set_mode(PWR_LDO15);
	}else{
		power_set_mode(TCFG_LOWPOWER_POWER_SEL);
	}

#if(!TCFG_CHARGE_ENABLE)
	/*close FAST CHARGE */
	CHARGE_EN(0);
	CHGBG_EN(0);
#endif
}

//static u32 key_row[] = {IO_PORTB_06, IO_PORTB_07, IO_PORTB_08, IO_PORTB_09, IO_PORTB_10, IO_PORTB_11, IO_PORTC_06, IO_PORTC_07};
/************************** PWR config ****************************/
struct port_wakeup port0 = {
	.pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
	.edge               = RISING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
	.attribute          = BLUETOOTH_RESUME,                  //保留参数
	.iomap              = IO_PORTB_09,                       //唤醒口选择
    .filter_enable      = ENABLE,
};

struct port_wakeup port1 = {
	.pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
	.edge               = RISING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
	.attribute          = BLUETOOTH_RESUME,                  //保留参数
	.iomap              = IO_PORTB_07,                       //唤醒口选择
    .filter_enable      = ENABLE,
};

struct port_wakeup port2 = {
	.pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
	.edge               = RISING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
	.attribute          = BLUETOOTH_RESUME,                  //保留参数
	.iomap              = IO_PORTB_11,                       //唤醒口选择
    .filter_enable      = ENABLE,
};

struct port_wakeup port3 = {
	.pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
	.edge               = RISING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
	.attribute          = BLUETOOTH_RESUME,                  //保留参数
	.iomap              = IO_PORTC_07,                       //唤醒口选择
    .filter_enable      = ENABLE,
};

struct port_wakeup port4 = {
	.pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
	.edge               = RISING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
	.attribute          = BLUETOOTH_RESUME,                  //保留参数
	.iomap              = IO_PORTB_10,                       //唤醒口选择
    .filter_enable      = ENABLE,
};

struct port_wakeup port5 = {
	.pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
	.edge               = RISING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
	.attribute          = BLUETOOTH_RESUME,                  //保留参数
	.iomap              = IO_PORTC_06,                       //唤醒口选择
    .filter_enable      = ENABLE,
};

struct port_wakeup port6 = {
	.pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
	.edge               = RISING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
	.attribute          = BLUETOOTH_RESUME,                  //保留参数
	.iomap              = IO_PORTB_06,                       //唤醒口选择
    .filter_enable      = ENABLE,
};

struct port_wakeup port7 = {
	.pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
	.edge               = RISING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
	.attribute          = BLUETOOTH_RESUME,                  //保留参数
	.iomap              = IO_PORTB_08,                       //唤醒口选择
    .filter_enable      = ENABLE,
};

const struct sub_wakeup sub_wkup = {
	.attribute  = BLUETOOTH_RESUME,
};

const struct charge_wakeup charge_wkup = {
	.attribute  = BLUETOOTH_RESUME,
};

const struct wakeup_param wk_param = {
    .port[0] = &port0,
	.port[1] = &port1,
	.port[2] = &port2,
	.port[3] = &port3,
	.port[4] = &port4,
	.port[5] = &port5,
	.port[6] = &port6,
	.port[7] = &port7,
	.sub = &sub_wkup,
	.charge = &charge_wkup,
};

//-----------------------------------------------

//-----------------------------------------------

/*进软关机之前默认将IO口都设置成高阻状态，需要保留原来状态的请修改该函数*/
extern void dac_power_off(void);
void board_set_soft_poweroff(void)
{
    u32 porta_value = 0xffff;
    u32 portb_value = 0xfffe;
    u32 portc_value = 0xffff;

	if(TCFG_LOWPOWER_POWER_SEL == PWR_DCDC15){
		power_set_mode(PWR_LDO15);
	}

    gpio_dir(GPIOA, 0, 16, porta_value, GPIO_OR);
    gpio_set_pu(GPIOA, 0, 16, ~porta_value, GPIO_AND);
    gpio_set_pd(GPIOA, 0, 16, ~porta_value, GPIO_AND);
    gpio_die(GPIOA, 0, 16, ~porta_value, GPIO_AND);
    gpio_dieh(GPIOA, 0, 16, ~portc_value, GPIO_AND);

    //保留长按Reset Pin - PB1
    gpio_dir(GPIOB, 1, 15, portb_value, GPIO_OR);
    gpio_set_pu(GPIOB, 1, 15, ~portb_value, GPIO_AND);
    gpio_set_pd(GPIOB, 1, 15, ~portb_value, GPIO_AND);
    gpio_die(GPIOB, 1, 15, ~portb_value, GPIO_AND);
    gpio_dieh(GPIOB, 0, 16, ~portc_value, GPIO_AND);

    gpio_dir(GPIOC, 0, 16, portc_value, GPIO_OR);
    gpio_set_pu(GPIOC, 0, 16, ~portc_value, GPIO_AND);
    gpio_set_pd(GPIOC, 0, 16, ~portc_value, GPIO_AND);
    gpio_die(GPIOC, 0, 16, ~portc_value, GPIO_AND);
    gpio_dieh(GPIOC, 0, 16, ~portc_value, GPIO_AND);

    gpio_set_pull_up(IO_PORT_DP, 0);
    gpio_set_pull_down(IO_PORT_DP, 0);
    gpio_set_direction(IO_PORT_DP, 1);
    gpio_set_die(IO_PORT_DP, 0);
    gpio_set_dieh(IO_PORT_DP, 0);

    gpio_set_pull_up(IO_PORT_DM, 0);
    gpio_set_pull_down(IO_PORT_DM, 0);
    gpio_set_direction(IO_PORT_DM, 1);
    gpio_set_die(IO_PORT_DM, 0);
    gpio_set_dieh(IO_PORT_DM, 0);



    sdpg_config(0);

    /* rtc_port_pr_out(1, 1); */
    /* rtc_port_pr_out(0, 1); */
    /* rtc_port_pr_in(0); */
    /* rtc_port_pr_in(1); */
    /* rtc_port_pr_die(0, 0); */
    /* rtc_port_pr_die(1, 0); */
    /* rtc_port_pr_pd(0, 0); */
    /* rtc_port_pr_pd(1, 0); */
    /* rtc_port_pr_pu(0, 0); */
    /* rtc_port_pr_pu(1, 0); */

    u8 i = 0;
    for(; i< sizeof(key_col)/sizeof(u32); i++)
    {
        gpio_set_direction(key_col[i], 0);
        gpio_set_output_value(key_col[i], 1);
    }

    P33_TX_NBIT(P3_ANA_CON2, BIT(1), 0);
    P33_TX_NBIT(P3_ANA_CON2, BIT(2), 0);
    RC32K_EN(0);
    p33_tx_1byte(R3_OSL_CON, 0);
	/* dac_power_off(); */
}

#define     APP_IO_DEBUG_0(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT &= ~BIT(x);}
#define     APP_IO_DEBUG_1(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT |= BIT(x);}


void sleep_exit_callback(u32 usec)
{
	putchar('>');
    APP_IO_DEBUG_1(A, 6);

	if(TCFG_LOWPOWER_POWER_SEL == PWR_DCDC15){
		/* putchar('}'); */
		power_set_mode(TCFG_LOWPOWER_POWER_SEL);
	}

}

void sleep_enter_callback(u8  step)
{
    /* 此函数禁止添加打印 */
    if (step == 1) {
		putchar('<');
        APP_IO_DEBUG_0(A, 6);
        /*dac_power_off();*/
		if(TCFG_LOWPOWER_POWER_SEL == PWR_DCDC15){
			/* putchar('{'); */
			power_set_mode(PWR_LDO15);
		}
    } else {
        gpio_set_pull_up(IO_PORTA_03, 0);
        gpio_set_pull_down(IO_PORTA_03, 0);
        gpio_set_direction(IO_PORTA_03, 1);

        /* usb_iomode(1); */
        /*  */
        /* gpio_set_pull_up(IO_PORT_DP, 0); */
        /* gpio_set_pull_down(IO_PORT_DP, 0); */
        /* gpio_set_direction(IO_PORT_DP, 1); */
        /* gpio_set_die(IO_PORT_DP, 0); */
        /*  */
        /* gpio_set_pull_up(IO_PORT_DM, 0); */
        /* gpio_set_pull_down(IO_PORT_DM, 0); */
        /* gpio_set_direction(IO_PORT_DM, 1); */
        /* gpio_set_die(IO_PORT_DM, 0); */
	}
}



void board_power_init(void)
{
    log_info("Power init : %s", __FILE__);

    power_init(&power_param);

    /*sdpg_config(1);*/

    power_set_callback(TCFG_LOWPOWER_LOWPOWER_SEL, sleep_enter_callback, sleep_exit_callback, board_set_soft_poweroff);

	power_keep_dacvdd_en(0);

	power_wakeup_init(&wk_param);

#if (!TCFG_IOKEY_ENABLE && !TCFG_ADKEY_ENABLE)
    charge_check_and_set_pinr(0);
#endif

}
#endif
