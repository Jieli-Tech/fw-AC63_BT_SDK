#include "app_config.h"

#ifdef CONFIG_BOARD_AC631N_DEMO

#include "system/includes.h"
#include "device/key_driver.h"
#include "asm/charge.h"
#include "rtc_alarm.h"
#include "usb/otg.h"

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
    .osc_type       = TCFG_LOWPOWER_OSC_TYPE,
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

/************************** CHARGE config****************************/
#if TCFG_CHARGE_ENABLE
CHARGE_PLATFORM_DATA_BEGIN(charge_data)
    .charge_en              = TCFG_CHARGE_ENABLE,              //内置充电使能
    .charge_full_V          = TCFG_CHARGE_FULL_V,              //充电截止电压
    .charge_full_mA			= TCFG_CHARGE_FULL_MA,             //充电截止电流
    .charge_mA				= TCFG_CHARGE_MA,                  //充电电流
	.ldo5v_off_filter		= 0,
	.ldo5v_pulldown_en		= 0,                               //ldo5v的100K下拉电阻使能,若充电舱需要更大的负载才能检测到插入时，请将该变量置1
CHARGE_PLATFORM_DATA_END()
#endif//TCFG_CHARGE_ENABLE

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

#if TCFG_IRKEY_ENABLE
const struct irkey_platform_data irkey_data = {
	    .enable = TCFG_IRKEY_ENABLE,                              //IR按键使能
	    .port = TCFG_IRKEY_PORT,                                       //IR按键口
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

	{
		.connect_way = TCFG_IOKEY_PREV_CONNECT_WAY,
		.key_type.one_io.port = TCFG_IOKEY_PREV_ONE_PORT,
		.key_value = 1,
	},

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

#if TCFG_RTC_ALARM_ENABLE
const struct sys_time def_sys_time = {  //初始一下当前时间
    .year = 2020,
    .month = 1,
    .day = 1,
    .hour = 0,
    .min = 0,
    .sec = 0,
};
const struct sys_time def_alarm = {     //初始一下目标时间，即闹钟时间
    .year = 2050,
    .month = 1,
    .day = 1,
    .hour = 0,
    .min = 0,
    .sec = 0,
};
extern void alarm_isr_user_cbfun(u8 index);
RTC_DEV_PLATFORM_DATA_BEGIN(rtc_data)
    .default_sys_time = &def_sys_time,
    .default_alarm = &def_alarm,
    /* .cbfun = NULL,                      //闹钟中断的回调函数,用户自行定义 */
    .cbfun = alarm_isr_user_cbfun,
RTC_DEV_PLATFORM_DATA_END()
#endif

/************************** otg data****************************/
#if TCFG_OTG_MODE
struct otg_dev_data otg_data = {
    .usb_dev_en = TCFG_OTG_USB_DEV_EN,
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

/*其他封装板级，移该函数*/
u8 get_power_on_status(void)
{
#if TCFG_IOKEY_ENABLE
    struct iokey_port *power_io_list = NULL;
    power_io_list = iokey_data.port;

    if (iokey_data.enable) {
        if (gpio_read(power_io_list->key_type.one_io.port) == power_io_list->connect_way){
            return 1;
        }
    }
#endif

#if TCFG_ADKEY_ENABLE
    if (adkey_data.enable) {
        return 1;
    }
#endif

#if TCFG_LP_TOUCH_KEY_ENABLE
    return lp_touch_key_power_on_status();
#endif

    return 0;
}


static void board_devices_init(void)
{
#if TCFG_PWMLED_ENABLE
//    pwm_led_init(&pwm_led_data);
#endif

#if (TCFG_IOKEY_ENABLE || TCFG_ADKEY_ENABLE || TCFG_IRKEY_ENABLE || TCFG_TOUCH_KEY_ENABLE)
	key_driver_init();
#endif

#if TCFG_CHARGE_ENABLE
    charge_api_init(&charge_data);
#else
    CHGBG_EN(0);
    CHARGE_EN(0);
#endif

#if TCFG_RTC_ALARM_ENABLE
    alarm_init(&rtc_data);
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

#if TCFG_CHARGE_ENABLE && TCFG_HANDSHAKE_ENABLE
    if(get_charge_online_flag()){
        handshake_app_start(0, NULL);
    }
#endif

    if(get_charge_online_flag()){
        power_set_mode(PWR_LDO15);
    } else {
        power_set_mode(TCFG_LOWPOWER_POWER_SEL);
    }

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
static void close_gpio(u8 is_softoff)
{
    u16 port_group[] = {
        [PORTA_GROUP] = 0x1ff,
        [PORTB_GROUP] = 0x3ff,//
        [PORTC_GROUP] = 0x3ff,//
    };


#if TCFG_ADKEY_ENABLE
    port_protect(port_group,TCFG_ADKEY_PORT);
#endif /* */

#if TCFG_IOKEY_ENABLE
    port_protect(port_group, TCFG_IOKEY_POWER_ONE_PORT);
    port_protect(port_group, TCFG_IOKEY_PREV_ONE_PORT);
    port_protect(port_group, TCFG_IOKEY_NEXT_ONE_PORT);
#endif /* TCFG_IOKEY_ENABLE */

#if TCFG_RTC_ALARM_ENABLE
    port_protect(port_group, IO_PORTA_01);
    port_protect(port_group, IO_PORTA_02);
#endif /* TCFG_RTC_ALARM_ENABLE */

#if TCFG_CHARGE_ENABLE && TCFG_HANDSHAKE_ENABLE
    if (is_softoff == 0) {
        port_protect(port_group, TCFG_HANDSHAKE_IO_DATA1);
        port_protect(port_group, TCFG_HANDSHAKE_IO_DATA2);
    }
#endif

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

    //< close usb io
    usb_iomode(1);
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

    usb1_iomode(1);
    gpio_set_pull_up(IO_PORT_DP1, 0);
    gpio_set_pull_down(IO_PORT_DP1, 0);
    gpio_set_direction(IO_PORT_DP1, 1);
    gpio_set_die(IO_PORT_DP1, 0);
    gpio_set_dieh(IO_PORT_DP1, 0);

    gpio_set_pull_up(IO_PORT_DM1, 0);
    gpio_set_pull_down(IO_PORT_DM1, 0);
    gpio_set_direction(IO_PORT_DM1, 1);
    gpio_set_die(IO_PORT_DM1, 0);
    gpio_set_dieh(IO_PORT_DM1, 0);

    /* printf("JL_USB_IO->CON0=0x%x\r\n", JL_USB_IO->CON0); */
    /* printf("JL_USB_IO->CON1=0x%x\r\n", JL_USB_IO->CON1); */
    /* printf("JL_USB->CON0=0x%x\r\n", JL_USB->CON0); */
    /*  */
    /* printf("JL_USB1_IO->CON0=0x%x\r\n", JL_USB1_IO->CON0); */
    /* printf("JL_USB1_IO->CON1=0x%x\r\n", JL_USB1_IO->CON1); */
    /* printf("JL_USB1->CON0=0x%x\r\n", JL_USB1->CON0); */
}

/************************** PWR config ****************************/
struct port_wakeup port0 = {
	.pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
	.edge               = FALLING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
	.attribute          = BLUETOOTH_RESUME,                  //保留参数

#if TCFG_ADKEY_ENABLE
    .iomap              = TCFG_ADKEY_PORT,                   //唤醒口选择
#endif

#if TCFG_IOKEY_ENABLE
    .iomap              = TCFG_IOKEY_POWER_ONE_PORT,                   //唤醒口选择
#endif

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
#if TCFG_ADKEY_ENABLE || TCFG_IOKEY_ENABLE
	.port[1]    = &port0,
#endif
	.sub        = &sub_wkup,
	.charge     = &charge_wkup,
};

//-----------------------------------------------


/*进软关机之前默认将IO口都设置成高阻状态，需要保留原来状态的请修改该函数*/
extern void dac_power_off(void);
void board_set_soft_poweroff(void)
{
#if 1
    close_gpio(1);
#else
    u32 porta_value = 0xffff;
    u32 portb_value = 0xfffe;

    gpio_dir(GPIOA, 0, 16, porta_value, GPIO_OR);
    gpio_set_pu(GPIOA, 0, 16, ~porta_value, GPIO_AND);
    gpio_set_pd(GPIOA, 0, 16, ~porta_value, GPIO_AND);
    gpio_die(GPIOA, 0, 16, ~porta_value, GPIO_AND);

    //保留长按Reset Pin - PB1
    gpio_dir(GPIOB, 1, 15, portb_value, GPIO_OR);
    gpio_set_pu(GPIOB, 1, 15, ~portb_value, GPIO_AND);
    gpio_set_pd(GPIOB, 1, 15, ~portb_value, GPIO_AND);
    gpio_die(GPIOB, 1, 15, ~portb_value, GPIO_AND);

    gpio_set_pull_up(IO_PORT_DP, 0);
    gpio_set_pull_down(IO_PORT_DP, 0);
    gpio_set_direction(IO_PORT_DP, 1);
    gpio_set_die(IO_PORT_DP, 0);

    gpio_set_pull_up(IO_PORT_DM, 0);
    gpio_set_pull_down(IO_PORT_DM, 0);
    gpio_set_direction(IO_PORT_DM, 1);
    gpio_set_die(IO_PORT_DM, 0);

    /* dac_power_off(); */
#endif

}

#define     APP_IO_DEBUG_0(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT &= ~BIT(x);}
#define     APP_IO_DEBUG_1(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT |= BIT(x);}


void sleep_exit_callback(u32 usec)
{
	putchar('>');
    APP_IO_DEBUG_0(A, 5);
}

void sleep_enter_callback(u8  step)
{
    /* 此函数禁止添加打印 */
    if (step == 1) {
		putchar('<');
        APP_IO_DEBUG_1(A, 5);
        /*dac_power_off();*/
    } else {
#if 1
        close_gpio(0);
#endif
    }
}


//-----------------------------------------------


void board_power_init(void)
{
    log_info("Power init : %s", __FILE__);

    power_init(&power_param);

    gpio_longpress_pin0_reset_config(IO_PORTB_01, 0, 0);
    gpio_shortpress_reset_config(0);//1--enable 0--disable
    /* //< close short key reset */
    /* power_mclr(0); */
    /* //< close long key reset */
    /* power_pin_reset(0); */

    power_set_callback(TCFG_LOWPOWER_LOWPOWER_SEL, sleep_enter_callback, sleep_exit_callback, board_set_soft_poweroff);

	power_keep_dacvdd_en(0);

	power_wakeup_init(&wk_param);

/* #if (!TCFG_IOKEY_ENABLE && !TCFG_ADKEY_ENABLE) */
    /* charge_check_and_set_pinr(0); */
/* #endif */
}
#endif
