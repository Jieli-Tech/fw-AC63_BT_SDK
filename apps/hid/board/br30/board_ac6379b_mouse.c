#include "app_config.h"

#ifdef CONFIG_BOARD_AC6379B_MOUSE

#include "system/includes.h"
#include "media/includes.h"
#include "asm/sdmmc.h"
#include "asm/chargestore.h"
#include "asm/charge.h"
#include "key_event_deal.h"
#include "user_cfg.h"
/* #include "asm/lp_touch_key.h" */
#include "device/key_driver.h"
#include "asm/power/p33.h"
#include "code_switch.h"
#include "OMSensor_manage.h"
#include "asm/power_interface.h"

#define LOG_TAG_CONST       BOARD
#define LOG_TAG             "[BOARD]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define CPU_RUN_TARGET()    printf("%s:%d\n",__FUNCTION__,__LINE__)

void board_power_init(void);
void adc_set_vbat_vddio_tieup(u8 en);

#define __this (&status_config)

/************************** KEY MSG****************************/
/*各个按键的消息设置，如果USER_CFG中设置了USE_CONFIG_KEY_SETTING为1，则会从配置文件读取对应的配置来填充改结构体*/
u8 key_table[KEY_NUM_MAX][KEY_EVENT_MAX] = {
    // SHORT           LONG              HOLD              UP              DOUBLE           TRIPLE
    {KEY_MUSIC_PP,   KEY_POWEROFF,  KEY_POWEROFF_HOLD,  KEY_NULL,     KEY_CALL_LAST_NO,     KEY_NULL},   //KEY_0
    {KEY_MUSIC_NEXT, KEY_VOL_UP,    KEY_VOL_UP,         KEY_NULL,     KEY_OPEN_SIRI,        KEY_NULL},   //KEY_1
    {KEY_MUSIC_PREV, KEY_VOL_DOWN,  KEY_VOL_DOWN,       KEY_NULL,     KEY_HID_CONTROL,      KEY_NULL},   //KEY_2
    /* {KEY_EAR_IN,   KEY_EAR_OUT,  KEY_NULL,  KEY_NULL,     KEY_NULL,     KEY_NULL},   					//KEY_3 */
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


	/************************** CHARGE config****************************/
#if TCFG_CHARGE_ENABLE
CHARGE_PLATFORM_DATA_BEGIN(charge_data)
	.charge_en              = TCFG_CHARGE_ENABLE,              //内置充电使能
	.charge_poweron_en      = TCFG_CHARGE_POWERON_ENABLE,      //是否支持充电开机
	.charge_full_V          = TCFG_CHARGE_FULL_V,              //充电截止电压
	.charge_full_mA			= TCFG_CHARGE_FULL_MA,             //充电截止电流
	.charge_mA				= TCFG_CHARGE_MA,                  //充电电流
	/*ldo5v拔出过滤值，过滤时间 = (filter*2 + 20)ms,ldoin<0.6V且时间大于过滤时间才认为拔出
	  对于充满直接从5V掉到0V的充电仓，该值必须设置成0，对于充满由5V先掉到0V之后再升压到xV的
	  充电仓，需要根据实际情况设置该值大小*/
	.ldo5v_off_filter		= 100,
	.ldo5v_pulldown_lvl     = CHARGE_PULLDOWN_200K,            //下拉电阻档位选择
#if !TCFG_CHARGESTORE_ENABLE
	//1、对于自动升压充电舱,若充电舱需要更大的负载才能检测到插入时，请将该变量置1,并且根据需求配置下拉电阻档位
	//2、对于按键升压,并且是通过上拉电阻去提供维持电压的舱,请将该变量设置1,并且根据舱的上拉配置下拉需要的电阻挡位
	//3、对于常5V的舱,可将改变量设为0,省功耗
	.ldo5v_pulldown_en		= 1,
#else
	.ldo5v_pulldown_en		= 0,
#endif
	CHARGE_PLATFORM_DATA_END()
#endif//TCFG_CHARGE_ENABLE


	/************************** IO KEY ****************************/
#if TCFG_IOKEY_ENABLE
	const struct iokey_port iokey_list[] = {
		{
			.connect_way = ONE_PORT_TO_LOW,                     //IO按键的连接方式
			.key_type.one_io.port = TCFG_IOKEY_MOUSE_LK_PORT,   //IO按键对应的引脚
			.key_value = KEY_LK_VAL,                            //按键值
		},

		{
			.connect_way = ONE_PORT_TO_LOW,                     //IO按键的连接方式
			.key_type.one_io.port = TCFG_IOKEY_MOUSE_RK_PORT,   //IO按键对应的引脚
			.key_value = KEY_RK_VAL,                            //按键值
		},

		{
			.connect_way = ONE_PORT_TO_LOW,                     //IO按键的连接方式
			.key_type.one_io.port = TCFG_IOKEY_MOUSE_HK_PORT,   //IO按键对应的引脚
			.key_value = KEY_HK_VAL,                            //按键值
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
	{.bit_value = KEY_LK_VAL | KEY_RK_VAL, .remap_value = 3},
	{.bit_value = KEY_LK_VAL | KEY_HK_VAL, .remap_value = 4},
	{.bit_value = KEY_RK_VAL | KEY_HK_VAL, .remap_value = 5},
};

const struct key_remap_data iokey_remap_data = {
	.remap_num = ARRAY_SIZE(iokey_remap_table),
	.table = iokey_remap_table,
};
#endif

#endif

/************************ OPTICAL_MOUSE_SENSOR  config********************/
#if TCFG_OMSENSOR_ENABLE
#if TCFG_HAL3205_EN
OMSENSOR_PLATFORM_DATA_BEGIN(OMSensor_data)
	.OMSensor_id      = "hal3205",
	.OMSensor_sclk_io = TCFG_OPTICAL_SENSOR_SCLK_PORT,
	.OMSensor_data_io = TCFG_OPTICAL_SENSOR_DATA_PORT,
	.OMSensor_int_io  = TCFG_OPTICAL_SENSOR_INT_PORT,
	OMSENSOR_PLATFORM_DATA_END();
#endif  /*  TCFG_HAL3205_EN */
#endif  /* TCFG_OMSENSOR_ENABLE */


/**************************** CODE_SWITCH  config************************/
#if TCFG_CODE_SWITCH_ENABLE
SW_PLATFORM_DATA_BEGIN(sw_data)
	.a_phase_io = TCFG_CODE_SWITCH_A_PHASE_PORT,
	.b_phase_io = TCFG_CODE_SWITCH_B_PHASE_PORT,
	SW_PLATFORM_DATA_END();
#endif  /*  TCFG_CODE_SWITCH_ENABLE*/

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

/************************** LOW POWER config ****************************/
const struct low_power_param power_param = {
	.config         = TCFG_LOWPOWER_LOWPOWER_SEL,          //0：sniff时芯片不进入低功耗  1：sniff时芯片进入powerdown
	.btosc_hz       = TCFG_CLOCK_OSC_HZ,                   //外接晶振频率
	.delay_us       = TCFG_CLOCK_SYS_HZ / 1000000L,        //提供给低功耗模块的延时(不需要需修改)
	.btosc_disable  = TCFG_LOWPOWER_BTOSC_DISABLE,         //进入低功耗时BTOSC是否保持
	.vddiom_lev     = TCFG_LOWPOWER_VDDIOM_LEVEL,          //强VDDIO等级,可选：2.0V  2.2V  2.4V  2.6V  2.8V  3.0V  3.2V  3.6V
	.vddiow_lev     = TCFG_LOWPOWER_VDDIOW_LEVEL,          //弱VDDIO等级,可选：2.1V  2.4V  2.8V  3.2V
	.osc_type       = TCFG_LOWPOWER_OSC_TYPE,
	//.lpctmu_en 		= TCFG_LP_TOUCH_KEY_ENABLE,
	.vddio_keep     = 0,
};



/************************** PWR config ****************************/

struct port_wakeup port0 = {
	.pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
	.edge               = FALLING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
	.both_edge          = 0,
	.filter             = 0,
	.iomap              = TCFG_CODE_SWITCH_A_PHASE_PORT,                       //唤醒口选择
};

struct port_wakeup port1 = {
	.pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
	.edge               = FALLING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
	.both_edge          = 0,
	.filter             = 0,
	.iomap              = TCFG_CODE_SWITCH_B_PHASE_PORT,                       //唤醒口选择
};

struct port_wakeup port2 = {
	.pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
	.edge               = FALLING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
	.both_edge          = 0,
	.filter             = PORT_FLT_2ms,
	.iomap              = TCFG_OPTICAL_SENSOR_INT_PORT,                       //唤醒口选择
};

struct port_wakeup port3 = {
	.pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
	.edge               = FALLING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
	.both_edge          = 0,
	.filter             = PORT_FLT_2ms,
	.iomap              = TCFG_IOKEY_MOUSE_RK_PORT,                       //唤醒口选择
};

struct port_wakeup port4 = {
	.pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
	.edge               = FALLING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
	.both_edge          = 0,
	.filter             = PORT_FLT_2ms,
	.iomap              = TCFG_IOKEY_MOUSE_LK_PORT,                       //唤醒口选择
};

struct port_wakeup port5 = {
	.pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
	.edge               = FALLING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
	.both_edge          = 0,
	.filter             = PORT_FLT_2ms,
	.iomap              = TCFG_IOKEY_MOUSE_HK_PORT,                       //唤醒口选择
};



const struct wakeup_param wk_param = {

#if TCFG_CODE_SWITCH_ENABLE
	.port[1] = &port0,
	.port[2] = &port1,
#endif  /*  TCFG_CODE_SWITCH_ENABLE*/

#if TCFG_OMSENSOR_ENABLE
	.port[3] = &port2,
#endif /*  TCFG_OMSENSOR_ENABLE*/

#if TCFG_IOKEY_ENABLE
	.port[4] = &port3,
	.port[5] = &port4,
	.port[6] = &port5,
#endif  /* TCFG_IOKEY_ENABLE */

};


void mouse_board_devices_init(void)
{

#if TCFG_CODE_SWITCH_ENABLE
	code_switch_init(&sw_data);
#endif /*  TCFG_CODE_SWITCH_ENABLE*/

#if TCFG_OMSENSOR_ENABLE
	optical_mouse_sensor_init(&OMSensor_data);
#endif /* TCFG_OMSENSOR_ENABLE*/

#if TCFG_IOKEY_ENABLE
	key_driver_init();
#endif /* TCFG_IOKEY_ENABLE */

}

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

void gSensor_wkupup_disable(void)
{
	log_info("gSensor wkup disable\n");
	power_wakeup_index_disable(3);
}

void gSensor_wkupup_enable(void)
{
	log_info("gSensor wkup enable\n");
	power_wakeup_index_enable(3);
}

static void board_devices_init(void)
{

#if (TCFG_IOKEY_ENABLE || TCFG_ADKEY_ENABLE)
	key_driver_init();
#endif

#if TCFG_UART_KEY_ENABLE
	extern int uart_key_init(void);
	uart_key_init();
#endif /* #if TCFG_UART_KEY_ENABLE */

}

extern void cfg_file_parse(u8 idx);
void board_init()
{
	board_power_init();

	//封装是vbat和vddio绑一起的，要在adc初始化之前调用这个函数
	adc_set_vbat_vddio_tieup(1);

	adc_vbg_init();
	adc_init();
	cfg_file_parse(0);
	devices_init();

	board_devices_init();

	if(get_charge_online_flag()){
		power_set_mode(PWR_LDO15);
	}else{
		power_set_mode(TCFG_LOWPOWER_POWER_SEL);
	}

#if TCFG_UART0_ENABLE
	if (uart0_data.rx_pin < IO_MAX_NUM) {
		gpio_set_die(uart0_data.rx_pin, 1);
	}
#endif

	//
#if(!TCFG_CHARGE_ENABLE)
	CHARGE_EN(0);
	CHGBG_EN(0);

}
#endif

//maskrom 使用到的io
static void mask_io_cfg()
{
	struct boot_soft_flag_t boot_soft_flag = {0};
	boot_soft_flag.flag0.boot_ctrl.wdt_dis = 0;
	boot_soft_flag.flag0.boot_ctrl.poweroff = 0;
	boot_soft_flag.flag0.boot_ctrl.is_port_b = JL_IOMAP->CON0 & BIT(16) ? 1 : 0;

	boot_soft_flag.flag1.misc.usbdm = SOFTFLAG_HIGH_RESISTANCE;
	boot_soft_flag.flag1.misc.usbdp = SOFTFLAG_HIGH_RESISTANCE;

	boot_soft_flag.flag1.misc.uart_key_port = 0;
	boot_soft_flag.flag1.misc.ldoin = SOFTFLAG_HIGH_RESISTANCE;

	boot_soft_flag.flag2.pa7_pb4.pa7 = SOFTFLAG_HIGH_RESISTANCE;
	boot_soft_flag.flag2.pa7_pb4.pb4 = SOFTFLAG_HIGH_RESISTANCE;

	boot_soft_flag.flag3.pc3_pc5.pc3 = SOFTFLAG_OUT1; //touch key power support
	boot_soft_flag.flag3.pc3_pc5.pc5 = SOFTFLAG_HIGH_RESISTANCE;
	mask_softflag_config(&boot_soft_flag);
}
//-----------------------------------------------
static struct port_wakeup add_port = {
	.pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
	.edge               = FALLING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
	.both_edge          = 0,
	.filter             = PORT_FLT_NULL,
	.iomap              = 0,                       //唤醒口选择
};

const struct wakeup_param wk_param_add_io = {

#if TCFG_CODE_SWITCH_ENABLE
	.port[1] = &port0,
	.port[2] = &port1,
#endif  /*  TCFG_CODE_SWITCH_ENABLE*/

#if TCFG_OMSENSOR_ENABLE
	.port[3] = &port2,
#endif /*  TCFG_OMSENSOR_ENABLE*/

#if TCFG_IOKEY_ENABLE
	.port[4] = &port3,
	.port[5] = &port4,
	.port[6] = &port5,
#endif  /* TCFG_IOKEY_ENABLE */
	.port[7] = &add_port,
};




enum {
	PORTA_GROUP = 0,
	PORTB_GROUP,
	PORTC_GROUP,
	PORTD_GROUP,
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
		[PORTA_GROUP] = 0x1ff,
		[PORTB_GROUP] = 0x3ff,
		[PORTC_GROUP] = 0x3ff,
		[PORTD_GROUP] = 0x3ff,
	};

#if TCFG_CODE_SWITCH_ENABLE
	port_protect(port_group, TCFG_CODE_SWITCH_A_PHASE_PORT);
	port_protect(port_group, TCFG_CODE_SWITCH_B_PHASE_PORT);
#endif /* TCFG_CODE_SWITCH_ENABLE */
#if TCFG_OMSENSOR_ENABLE
	port_protect(port_group, TCFG_OPTICAL_SENSOR_SCLK_PORT);
	port_protect(port_group, TCFG_OPTICAL_SENSOR_DATA_PORT);
	port_protect(port_group, TCFG_OPTICAL_SENSOR_INT_PORT);
#endif /* TCFG_OMSENSOR_ENABLE */
#if TCFG_IOKEY_ENABLE
	port_protect(port_group, TCFG_IOKEY_MOUSE_LK_PORT);
	port_protect(port_group, TCFG_IOKEY_MOUSE_RK_PORT);
	port_protect(port_group, TCFG_IOKEY_MOUSE_HK_PORT);
#endif /* TCFG_IOKEY_ENABLE */

#if TCFG_RTC_ALARM_ENABLE
	port_protect(port_group, IO_PORTA_01);
	port_protect(port_group, IO_PORTA_02);
#endif /* TCFG_RTC_ALARM_ENABLE */

	//< close gpio
	gpio_dir(GPIOA, 0, 9, port_group[PORTA_GROUP], GPIO_OR);
	gpio_set_pu(GPIOA, 0, 9, ~port_group[PORTA_GROUP], GPIO_AND);
	gpio_set_pd(GPIOA, 0, 9, ~port_group[PORTA_GROUP], GPIO_AND);
	gpio_die(GPIOA, 0, 9, ~port_group[PORTA_GROUP], GPIO_AND);
	gpio_dieh(GPIOA, 0, 9, ~port_group[PORTA_GROUP], GPIO_AND);


	gpio_dir(GPIOB, 0, 10, port_group[PORTB_GROUP], GPIO_OR);
	gpio_set_pu(GPIOB, 0, 10, ~port_group[PORTB_GROUP], GPIO_AND);
	gpio_set_pd(GPIOB, 0, 10, ~port_group[PORTB_GROUP], GPIO_AND);
	gpio_die(GPIOB, 0, 10, ~port_group[PORTB_GROUP], GPIO_AND);
	gpio_dieh(GPIOB, 0, 10, ~port_group[PORTB_GROUP], GPIO_AND);


	gpio_dir(GPIOC, 0, 10, port_group[PORTC_GROUP], GPIO_OR);
	gpio_set_pu(GPIOC, 0, 10, ~port_group[PORTC_GROUP], GPIO_AND);
	gpio_set_pd(GPIOC, 0, 10, ~port_group[PORTC_GROUP], GPIO_AND);
	gpio_die(GPIOC, 0, 10, ~port_group[PORTC_GROUP], GPIO_AND);
	gpio_dieh(GPIOC, 0, 10, ~port_group[PORTC_GROUP], GPIO_AND);

	//< close usb io
	usb_iomode(1);

}

void power_wakeup_add_io(int io)
{
	log_info("reset wakeup add io");
	add_port.iomap = io;
	power_wakeup_init(&wk_param_add_io);
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
	u32 porta_value = 0xffff & ~(BIT(5)|BIT(6));
	u32 portb_value = 0xffff & ~(BIT(1)|BIT(6)|BIT(7));
	u32 portc_value = 0xffff & ~(BIT(4)|BIT(5));

	printf("portb_value %x\n",portb_value);

	/* gpio_direction_output(IO_PORTC_03, 1); //touch chip power support */

	mask_io_cfg();

	gpio_set_pu(GPIOA, 0, 16, ~porta_value, GPIO_AND);
	gpio_set_pd(GPIOA, 0, 16, ~porta_value, GPIO_AND);
	gpio_die(GPIOA, 0, 16, ~porta_value, GPIO_AND);
	gpio_dieh(GPIOA, 0, 16, ~porta_value, GPIO_AND);
	gpio_dir(GPIOA, 0, 16, porta_value, GPIO_OR);

	gpio_set_pu(GPIOB, 0, 16, ~portb_value, GPIO_AND);
	gpio_set_pd(GPIOB, 0, 16, ~portb_value, GPIO_AND);
	gpio_die(GPIOB, 0, 16, ~portb_value, GPIO_AND);
	gpio_dieh(GPIOB, 0, 16, ~portb_value, GPIO_AND);
	gpio_dir(GPIOB, 0, 16, portb_value, GPIO_OR);

	gpio_set_pu(GPIOC, 0, 16, ~portc_value, GPIO_AND);
	gpio_set_pd(GPIOC, 0, 16, ~portc_value, GPIO_AND);
	gpio_die(GPIOC, 0, 16, ~portc_value, GPIO_AND);
	gpio_dieh(GPIOC, 0, 16, ~portc_value, GPIO_AND);
	gpio_dir(GPIOC, 0, 16, portc_value, GPIO_OR);


	/*
	   gpio_set_pu(GPIOD, 0, 16, ~portd_value, GPIO_AND);
	   gpio_set_pd(GPIOD, 0, 16, ~portd_value, GPIO_AND);
	   gpio_die(GPIOD, 0, 16, ~portd_value, GPIO_AND);
	   gpio_dieh(GPIOD, 0, 16, ~portd_value, GPIO_AND);
	   gpio_dir(GPIOD, 0, 16, portd_value, GPIO_OR);
	 */

	gpio_set_pull_up(IO_PORTP_00, 0);
	gpio_set_pull_down(IO_PORTP_00, 0);
	gpio_set_die(IO_PORTP_00, 0);
	gpio_set_dieh(IO_PORTP_00, 0);
	gpio_set_direction(IO_PORTP_00, 1);

	gpio_set_pull_up(IO_PORT_DP, 0);
	gpio_set_pull_down(IO_PORT_DP, 0);
	gpio_set_die(IO_PORT_DP, 0);
	gpio_set_dieh(IO_PORT_DP, 0);
	gpio_set_direction(IO_PORT_DP, 1);

	gpio_set_pull_up(IO_PORT_DM, 0);
	gpio_set_pull_down(IO_PORT_DM, 0);
	gpio_set_die(IO_PORT_DM, 0);
	gpio_set_dieh(IO_PORT_DM, 0);
	gpio_set_direction(IO_PORT_DM, 1);
	if(add_port.iomap){
		keep_set_io_input(add_port.iomap,add_port.pullup_down_enable,add_port.pullup_down_enable);
	}

	/* dac_power_off(); */
}

#define     APP_IO_DEBUG_0(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT &= ~BIT(x);}
#define     APP_IO_DEBUG_1(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT |= BIT(x);}

void sleep_exit_callback(u32 usec)
{
	putchar('>');
	APP_IO_DEBUG_1(B, 0);
}

void sleep_enter_callback(u8  step)
{
	/* 此函数禁止添加打印 */
	if (step == 1) {
		putchar('<');
		APP_IO_DEBUG_0(B, 0);

#if 0//??? TCFG_CODE_SWITCH_ENABLE
		if (gpio_read(sw_data.a_phase_io) == 0) {
			wk_param.port[1]->edge = RISING_EDGE;
		} else if (gpio_read(sw_data.a_phase_io) == 1) {
			wk_param.port[1]->edge = FALLING_EDGE;
		}
		power_wakeup_port_set(1, wk_param.port[1]);

		if (gpio_read(sw_data.b_phase_io) == 0) {
			wk_param.port[2]->edge = RISING_EDGE;
		} else if (gpio_read(sw_data.b_phase_io) == 1) {
			wk_param.port[2]->edge = FALLING_EDGE;
		}
		power_wakeup_port_set(2, wk_param.port[2]);
#endif

		/* dac_power_off(); */
	} else {
		/*
		   gpio_set_pull_up(IO_PORTA_03, 0);
		   gpio_set_pull_down(IO_PORTA_03, 0);
		   gpio_set_direction(IO_PORTA_03, 1);

		   usb_iomode(1);

		   gpio_set_pull_up(IO_PORT_DP, 0);
		   gpio_set_pull_down(IO_PORT_DP, 0);
		   gpio_set_direction(IO_PORT_DP, 1);
		   gpio_set_die(IO_PORT_DP, 0);

		   gpio_set_pull_up(IO_PORT_DM, 0);
		   gpio_set_pull_down(IO_PORT_DM, 0);
		   gpio_set_direction(IO_PORT_DM, 1);
		   gpio_set_die(IO_PORT_DM, 0);
		 */
		close_gpio();
	}
}

static void wl_audio_clk_on(void)
{
	JL_WL_AUD->CON0 = 1;
}

//-----------------------------------------------
static void wakeup_callback(u8 index, u8 gpio)
{
	//printf("index is 0x%x, gpio is 0x%x\n", index, gpio);
}

void board_power_init(void)
{
	log_info("Power init : %s", __FILE__);
	/* gpio_direction_output(IO_PORTC_03, 1); //touch chip power support */

	power_init(&power_param);

	power_set_callback(TCFG_LOWPOWER_LOWPOWER_SEL, sleep_enter_callback, sleep_exit_callback, board_set_soft_poweroff);

	wl_audio_clk_on();

	power_keep_dacvdd_en(0);

	port_edge_wkup_set_callback(wakeup_callback);
	power_wakeup_init(&wk_param);

	//wdt_close();
	//while(1);

#if (!TCFG_IOKEY_ENABLE && !TCFG_ADKEY_ENABLE)
	charge_check_and_set_pinr(1);
#endif
}

#endif /* #ifdef CONFIG_BOARD_AC697X_DEMO */



