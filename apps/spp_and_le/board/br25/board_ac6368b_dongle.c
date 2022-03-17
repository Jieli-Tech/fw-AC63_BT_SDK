#include "app_config.h"

#ifdef CONFIG_BOARD_AC6368B_DONGLE

#include "system/includes.h"
#include "asm/charge.h"
#include "device/key_driver.h"
#include "asm/power/p33.h"
#include "asm/pwm_led.h"
#include "usb/otg.h"
#include "asm/usb.h"

#define LOG_TAG_CONST       BOARD
#define LOG_TAG             "[BOARD]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

void board_power_init(void);
/************************** LOW POWER config ****************************/
const struct low_power_param power_param = {
    .config         = TCFG_LOWPOWER_LOWPOWER_SEL,          //0：sniff时芯片不进入低功耗  1：sniff时芯片进入powerdown
    .btosc_hz         = TCFG_CLOCK_OSC_HZ,                   //外接晶振频率
    .delay_us       = TCFG_CLOCK_SYS_HZ / 1000000L,        //提供给低功耗模块的延时(不需要需修改)
    .btosc_disable  = TCFG_LOWPOWER_BTOSC_DISABLE,         //进入低功耗时BTOSC是否保持
    .vddiom_lev     = TCFG_LOWPOWER_VDDIOM_LEVEL,          //强VDDIO等级,可选：2.0V  2.2V  2.4V  2.6V  2.8V  3.0V  3.2V  3.6V
    .vddiow_lev     = TCFG_LOWPOWER_VDDIOW_LEVEL,          //弱VDDIO等级,可选：2.1V  2.4V  2.8V  3.2V
    .osc_type       = OSC_TYPE_LRC,
    .dcdc_port      = TCFG_DCDC_PORT_SEL,
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

/************************** IO KEY ****************************/
#if TCFG_IOKEY_ENABLE
const struct iokey_port iokey_list[] = {
	{
		.connect_way = TCFG_IOKEY_POWER_CONNECT_WAY,          //IO按键的连接方式
		.key_type.one_io.port = TCFG_IOKEY_POWER_ONE_PORT,    //IO按键对应的引脚
		.key_value = 0,                                       //按键值
	},

#if(!TCFG_UART0_ENABLE || TCFG_UART0_TX_PORT !=	IO_PORT_DP)
	{
		.connect_way = TCFG_IOKEY_PREV_CONNECT_WAY,
		.key_type.one_io.port = TCFG_IOKEY_PREV_ONE_PORT,
		.key_value = 1,
	},
#endif

	{
		.connect_way = TCFG_IOKEY_NEXT_CONNECT_WAY,
		.key_type.one_io.port = TCFG_IOKEY_NEXT_ONE_PORT,
		.key_value = 2,
	},
};
const struct iokey_platform_data iokey_data = {
	.enable = TCFG_IOKEY_ENABLE,                              //是否使能IO按键
	.num = ARRAY_SIZE(iokey_list),                            //IO按键的个数
	.port = iokey_list,                                       //IO按键参数表
};

#if MULT_KEY_ENABLE
//组合按键消息映射表
//配置注意事项:单个按键按键值需要按照顺序编号,如power:0, prev:1, next:2
//bit_value = BIT(0) | BIT(1) 指按键值为0和按键值为1的两个按键被同时按下,
//remap_value = 3指当这两个按键被同时按下后重新映射的按键值;
const struct key_remap iokey_remap_table[] = {
	{.bit_value = BIT(0) | BIT(1), .remap_value = 3},
	{.bit_value = BIT(0) | BIT(2), .remap_value = 4},
	{.bit_value = BIT(1) | BIT(2), .remap_value = 5},
};

const struct key_remap_data iokey_remap_data = {
	.remap_num = ARRAY_SIZE(iokey_remap_table),
	.table = iokey_remap_table,
};
#endif

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

/************************** PWM_LED ****************************/
#if TCFG_PWMLED_ENABLE
LED_PLATFORM_DATA_BEGIN(pwm_led_data)
	.io_mode = TCFG_PWMLED_IOMODE,              //推灯模式设置:支持单个IO推两个灯和两个IO推两个灯
	.io_cfg.one_io.pin = TCFG_PWMLED_PIN,       //单个IO推两个灯的IO口配置
LED_PLATFORM_DATA_END()
#endif

/************************** otg data****************************/
#if TCFG_OTG_MODE
struct otg_dev_data otg_data = {
    .usb_dev_en = 0,
	.slave_online_cnt = TCFG_OTG_SLAVE_ONLINE_CNT,
	.slave_offline_cnt = TCFG_OTG_SLAVE_OFFLINE_CNT,
	.host_online_cnt = TCFG_OTG_HOST_ONLINE_CNT,
	.host_offline_cnt = TCFG_OTG_HOST_OFFLINE_CNT,
	.detect_mode = TCFG_OTG_MODE,
	.detect_time_interval = TCFG_OTG_DET_INTERVAL,
};
#endif
REGISTER_DEVICES(device_table) = {
#if TCFG_OTG_MODE
    { "otg",     &usb_dev_ops, (void *) &otg_data},
#endif
};

static void board_devices_init(void)
{
#if TCFG_PWMLED_ENABLE
   pwm_led_init(&pwm_led_data);
#endif

#if (TCFG_IOKEY_ENABLE || TCFG_ADKEY_ENABLE || TCFG_TOUCH_KEY_ENABLE)
	key_driver_init();
#endif

}

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
enum {
    PORTA_GROUP = 0,
    PORTB_GROUP,
    PORTC_GROUP,
};

static void port_protect(u16 *port_group, u32 port_num)
{
    if (port_num == NO_CONFIG_PORT) {
        return;
    }
    port_group[port_num / IO_GROUP_NUM] &= ~BIT(port_num % IO_GROUP_NUM);
}

/*进软关机之前默认将IO口都设置成高阻状态，需要保留原来状态的请修改该函数*/
static void close_gpio(void)
{
    u16 port_group[] = {
        [PORTA_GROUP] = 0xffff,
        [PORTB_GROUP] = 0xffff,
        [PORTC_GROUP] = 0xffff,
    };

#if TCFG_ADKEY_ENABLE
    port_protect(port_group,TCFG_ADKEY_PORT);
#endif

#if TCFG_IOKEY_ENABLE
	port_protect(port_group, TCFG_IOKEY_POWER_ONE_PORT);
    port_protect(port_group, TCFG_IOKEY_PREV_ONE_PORT);
    port_protect(port_group, TCFG_IOKEY_NEXT_ONE_PORT);
#endif /* TCFG_IOKEY_ENABLE */

    //< close gpio
    gpio_dir(GPIOA, 0, 16, port_group[PORTA_GROUP], GPIO_OR);
    gpio_set_pu(GPIOA, 0,16, ~port_group[PORTA_GROUP], GPIO_AND);
    gpio_set_pd(GPIOA, 0,16, ~port_group[PORTA_GROUP], GPIO_AND);
    gpio_die(GPIOA, 0,16, ~port_group[PORTA_GROUP], GPIO_AND);
    gpio_dieh(GPIOA, 0,16, ~port_group[PORTA_GROUP], GPIO_AND);

    gpio_dir(GPIOB, 0,16, port_group[PORTB_GROUP], GPIO_OR);
    gpio_set_pu(GPIOB, 0,16, ~port_group[PORTB_GROUP], GPIO_AND);
    gpio_set_pd(GPIOB, 0,16, ~port_group[PORTB_GROUP], GPIO_AND);
    gpio_die(GPIOB, 0,16, ~port_group[PORTB_GROUP], GPIO_AND);
    gpio_dieh(GPIOB, 0,16, ~port_group[PORTB_GROUP], GPIO_AND);


    gpio_dir(GPIOC, 0,16, port_group[PORTC_GROUP], GPIO_OR);
    gpio_set_pu(GPIOC, 0,16, ~port_group[PORTC_GROUP], GPIO_AND);
    gpio_set_pd(GPIOC, 0,16, ~port_group[PORTC_GROUP], GPIO_AND);
    gpio_die(GPIOC, 0,16, ~port_group[PORTC_GROUP], GPIO_AND);
    gpio_dieh(GPIOC, 0,16, ~port_group[PORTC_GROUP], GPIO_AND);

    //< close usb io
#if !USB_SUSPEND_RESUME
    usb_iomode(1);
#endif

	//dp dm is io_key
	/* gpio_set_pull_up(IO_PORT_DP, 0); */
    /* gpio_set_pull_down(IO_PORT_DP, 0); */
    /* gpio_set_direction(IO_PORT_DP, 1); */
    /* gpio_set_die(IO_PORT_DP, 0); */
    /* gpio_set_dieh(IO_PORT_DP, 0); */

    /* gpio_set_pull_up(IO_PORT_DM, 0); */
    /* gpio_set_pull_down(IO_PORT_DM, 0); */
    /* gpio_set_direction(IO_PORT_DM, 1); */
    /* gpio_set_die(IO_PORT_DM, 0); */
    /* gpio_set_dieh(IO_PORT_DM, 0); */

    /* printf("JL_USB_IO->CON0=0x%x\r\n", JL_USB_IO->CON0); */
    /* printf("JL_USB_IO->CON1=0x%x\r\n", JL_USB_IO->CON1); */
    /* printf("JL_USB->CON0=0x%x\r\n", JL_USB->CON0); */
    /*  */
    /* printf("JL_USB1_IO->CON0=0x%x\r\n", JL_USB1_IO->CON0); */
    /* printf("JL_USB1_IO->CON1=0x%x\r\n", JL_USB1_IO->CON1); */
    /* printf("JL_USB1->CON0=0x%x\r\n", JL_USB1->CON0); */
}

/************************** PWR config ****************************/
struct port_wakeup port1 = {
	.pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
	.edge               = FALLING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
	.attribute          = BLUETOOTH_RESUME,                  //保留参数
	.iomap              = IO_PORTB_01,                       //唤醒口选择
    .filter_enable      = ENABLE,
};

struct port_wakeup port2 = {
	.pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
	.edge               = FALLING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
	.attribute          = BLUETOOTH_RESUME,                  //保留参数
	.iomap              = IO_PORT_DP,                       //唤醒口选择
    .filter_enable      = ENABLE,
};
const struct sub_wakeup sub_wkup = {
	.attribute  = BLUETOOTH_RESUME,
};

const struct charge_wakeup charge_wkup = {
	.attribute  = BLUETOOTH_RESUME,
};

const struct wakeup_param wk_param = {
    .filter     = PORT_FLT_2ms,
	.port[1]    = &port1,
#if USB_SUSPEND_RESUME
	.port[2]    = &port2,
#endif
	.sub        = &sub_wkup,
	.charge     = &charge_wkup,
};

void usb_wkupup_disable(void)
{
	//log_info("usb wkup disable\n");
    power_wakeup_index_disable(2);
}

void usb_wkupup_enable(void)
{
    //log_info("usb wkup enable\n");
    power_wakeup_index_enable(2);
}
//-----------------------------------------------


static void keep_set_io_output(int io_sel,int en)
{
	gpio_set_pull_up(io_sel, 0);
	gpio_set_pull_down(io_sel, 0);
	gpio_set_direction(io_sel, 0);
	gpio_set_die(io_sel, 1);
	gpio_set_output_value(io_sel, en);
}

static void keep_set_io_input(int io_sel,u8 pull_up_en,u8 pull_down_en)
{
	gpio_set_pull_up(io_sel, pull_up_en);
	gpio_set_pull_down(io_sel, pull_down_en);
	gpio_set_direction(io_sel, 1);
	gpio_set_die(io_sel, 0);
}


/*进软关机之前默认将IO口都设置成高阻状态，需要保留原来状态的请修改该函数*/
extern void dac_power_off(void);
void board_set_soft_poweroff(void)
{
	if(TCFG_LOWPOWER_POWER_SEL == PWR_DCDC15){
		power_set_mode(PWR_LDO15);
	}

	close_gpio();

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

	if(TCFG_LOWPOWER_POWER_SEL == PWR_DCDC15
			&& TCFG_DCDC_PORT_SEL != NO_CONFIG_PORT){
		//need keep dcdc's io output 0
		keep_set_io_output(TCFG_DCDC_PORT_SEL,0);
	}

}

#define     APP_IO_DEBUG_0(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT &= ~BIT(x);}
#define     APP_IO_DEBUG_1(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT |= BIT(x);}



//-----------------------------------------------
void sleep_exit_callback(u32 usec)
{
	putchar('>');
    APP_IO_DEBUG_1(A, 6);

	if(TCFG_LOWPOWER_POWER_SEL == PWR_DCDC15){
		/* putchar('}'); */
		power_set_mode(TCFG_LOWPOWER_POWER_SEL);
	}

#if USB_SUSPEND_RESUME
    usb_phy_resume(0);

	u8 sfr = p33_rx_1byte(P3_WKUP_SRC);
	if(sfr & BIT(1)){
		sfr = p33_rx_1byte(P3_WKUP_PND);
	   	for(u8 i=0; i<8; i++){
			if(sfr & BIT(i)){
				p33_tx_1byte(P3_WKUP_CPND, BIT(i));
		    	log_info("Port index %d", i);
			}
		}

        void usb_resume_sign(const usb_dev usb_id);

		if(sfr & BIT(1)){
            usb_remote_wakeup(0);
		}
	}

	usb_wkupup_disable();
#endif
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

#if 1
        close_gpio();
#endif

#if USB_SUSPEND_RESUME
		usb_phy_suspend(0);
		usb_wkupup_enable();
#endif
    }
}


//-----------------------------------------------


void board_power_init(void)
{
    log_info("Power init : %s", __FILE__);

    //< close short key reset
    /* p33_and_1byte(P3_PR_PWR, ~BIT(3)); */
    //< close long key reset
    /* p33_and_1byte(P3_PINR_CON, 0); */

    power_init(&power_param);

    //< close long key reset
    VCM_DET_EN(0);

    power_set_callback(TCFG_LOWPOWER_LOWPOWER_SEL, sleep_enter_callback, sleep_exit_callback, board_set_soft_poweroff);

	power_keep_dacvdd_en(0);

	power_wakeup_init(&wk_param);

#if USB_SUSPEND_RESUME
    usb_wkupup_disable();
#endif


/* #if (!TCFG_IOKEY_ENABLE && !TCFG_ADKEY_ENABLE) */
    /* charge_check_and_set_pinr(0); */
/* #endif */
}
#endif
