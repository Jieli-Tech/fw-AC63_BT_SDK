#include "app_config.h"

#ifdef CONFIG_BOARD_AC6319A_MOUSE

#include "system/includes.h"
#include "code_switch.h"
#include "OMSensor_manage.h"
#include "device/key_driver.h"
#include "rtc_alarm.h"

#define LOG_TAG_CONST       BOARD
#define LOG_TAG             "[BOARD]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

static void board_power_init(void);

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


/************************** UART config****************************/
#if TCFG_UART0_ENABLE
UART0_PLATFORM_DATA_BEGIN(uart0_data)
.tx_pin = TCFG_UART0_TX_PORT,                             //串口打印TX引脚选择
 .rx_pin = TCFG_UART0_RX_PORT,                             //串口打印RX引脚选择
  .baudrate = TCFG_UART0_BAUDRATE,                          //串口波特率

   .flags = UART_DEBUG,                                      //串口用来打印需要把改参数设置为UART_DEBUG
    UART0_PLATFORM_DATA_END()
#endif //TCFG_UART0_ENABLE


    /**********************************IO KEY ********************************/
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

    /* { */
    /*     .connect_way = DOUBLE_PORT_TO_IO, */
    /*     .key_type.two_io.out_port 	= TCFG_IOKEY_MOUSE_MK_OUT_PORT, */
    /*     .key_type.two_io.in_port    = TCFG_IOKEY_MOUSE_MK_IN_PORT, */
    /*     .key_value = 3, */
    /* }, */
};

const struct iokey_platform_data iokey_data = {
    .enable = TCFG_IOKEY_ENABLE,                         //是否使能IO按键
    .num = ARRAY_SIZE(iokey_list),       //IO按键的个数
    .port = iokey_list,                  //IO按键参数表
};

const struct key_remap key_remap_table[] = {
    {
        .bit_value = BIT(KEY_LK_VAL) | BIT(KEY_RK_VAL),
        .remap_value = KEY_LK_RK_VAL,
    },

    {
        .bit_value = BIT(KEY_LK_VAL) | BIT(KEY_HK_VAL),
        .remap_value = KEY_LK_HK_VAL,
    },

    {
        .bit_value = BIT(KEY_RK_VAL) | BIT(KEY_HK_VAL),
        .remap_value = KEY_RK_HK_VAL,
    },

    {
        .bit_value = BIT(KEY_LK_VAL) | BIT(KEY_RK_VAL) | BIT(KEY_HK_VAL),
        .remap_value = KEY_LK_RK_HK_VAL,
    },
};

const struct key_remap_data iokey_remap_data = {
    .remap_num = ARRAY_SIZE(key_remap_table),
    .table = key_remap_table,
};

#endif  /* TCFG_IOKEY_ENABLE */

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


/******************************** PWR config *****************************/
struct port_wakeup port0 = {
    .pullup_down_enable = DISABLE,                         //配置I/O 内部上下拉是否使能
    .edge       = FALLING_EDGE,                            //唤醒方式选择,可选：上升沿\下降沿
    .attribute  = BLUETOOTH_RESUME,                        //保留参数
    .iomap      = TCFG_CODE_SWITCH_A_PHASE_PORT,           //唤醒口选择
    .filter_enable      = ENABLE,
};

struct port_wakeup port1 = {
    .pullup_down_enable = DISABLE,                         //配置I/O 内部上下拉是否使能
    .edge       = FALLING_EDGE,                            //唤醒方式选择,可选：上升沿\下降沿
    .attribute  = BLUETOOTH_RESUME,                        //保留参数
    .iomap      = TCFG_CODE_SWITCH_B_PHASE_PORT,           //唤醒口选择
    .filter_enable      = ENABLE,
};

struct port_wakeup port2 = {
    .pullup_down_enable = DISABLE,                         //配置I/O 内部上下拉是否使能
    .edge       = FALLING_EDGE,                            //唤醒方式选择,可选：上升沿\下降沿
    .attribute  = BLUETOOTH_RESUME,                        //保留参数
    .iomap      = TCFG_OPTICAL_SENSOR_INT_PORT,            //唤醒口选择
    .filter_enable      = ENABLE,
};

struct port_wakeup port3 = {
    .pullup_down_enable = ENABLE,                          //配置I/O 内部上下拉是否使能
    .edge       = FALLING_EDGE,                            //唤醒方式选择,可选：上升沿\下降沿
    .attribute  = BLUETOOTH_RESUME,                        //保留参数
    .iomap      = TCFG_IOKEY_MOUSE_RK_PORT,                //唤醒口选择
    .filter_enable      = ENABLE,
};

struct port_wakeup port4 = {
    .pullup_down_enable = ENABLE,                          //配置I/O 内部上下拉是否使能
    .edge       = FALLING_EDGE,                            //唤醒方式选择,可选：上升沿\下降沿
    .attribute  = BLUETOOTH_RESUME,                        //保留参数
    .iomap      = TCFG_IOKEY_MOUSE_LK_PORT,                //唤醒口选择
    .filter_enable      = ENABLE,
};

struct port_wakeup port5 = {
    .pullup_down_enable = ENABLE,                          //配置I/O 内部上下拉是否使能
    .edge       = FALLING_EDGE,                            //唤醒方式选择,可选：上升沿\下降沿
    .attribute  = BLUETOOTH_RESUME,                        //保留参数
    .iomap      = TCFG_IOKEY_MOUSE_HK_PORT,                //唤醒口选择
    .filter_enable      = ENABLE,
};

const struct sub_wakeup sub_wkup = {
    .attribute  = BLUETOOTH_RESUME,
};

const struct charge_wakeup charge_wkup = {
    .attribute  = BLUETOOTH_RESUME,
};

struct wakeup_param wk_param = {
    .filter     = PORT_FLT_2ms,
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

    .sub = &sub_wkup,
    .charge = &charge_wkup,
};

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


void cfg_file_parse(u8 idx);

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

#if TCFG_RTC_ALARM_ENABLE
    alarm_init(&rtc_data);
#endif
}


void board_init()
{
    board_power_init();

    //封装是vbat和vddio绑一起的，要在adc初始化之前调用这个函数
    adc_set_vbat_vddio_tieup(1);

    adc_vbg_init();
    adc_init();
    cfg_file_parse(0);

    devices_init();

    /* mouse_board_devices_init(); */

    power_set_mode(TCFG_LOWPOWER_POWER_SEL);

    /*close FAST CHARGE */
    /* CHARGE_EN(0); */
    /* CHGBG_EN(0); */
}

enum {
    PORTA_GROUP = 0,
    PORTB_GROUP,
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

#define     LP_IO_DEBUG_0(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT &= ~BIT(x);}
#define     LP_IO_DEBUG_1(i,x)       //{JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT |= BIT(x);}

void sleep_exit_callback(u32 usec)
{
    putchar('>');
    LP_IO_DEBUG_1(A, 6);
}

void sleep_enter_callback(u8  step)
{
    /* 此函数禁止添加打印 */
    if (step == 1) {
        putchar('<');

        LP_IO_DEBUG_0(A, 6);

#if TCFG_CODE_SWITCH_ENABLE
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

        /* power_wakeup_init(&wk_param); */
#endif
    } else {
        close_gpio();
    }
}

static void board_power_init(void)
{
    log_info("Power init : %s", __FILE__);

    power_init(&power_param);

    //< close short key reset
    power_mclr(0);
    //< close long key reset
    power_pin_reset(0);

    power_set_callback(TCFG_LOWPOWER_LOWPOWER_SEL, sleep_enter_callback, sleep_exit_callback, close_gpio);

    power_keep_dacvdd_en(0);

    power_wakeup_init(&wk_param);

#if 0
    extern void power_set_soft_poweroff(void);
    power_set_soft_poweroff();
#endif /*  */
}

#endif /* CONFIG_BOARD_AC6302A_MOUSE */
