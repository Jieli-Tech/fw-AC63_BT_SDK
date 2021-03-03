#include "app_config.h"

#ifdef CONFIG_BOARD_AC637N_DEMO

#include "system/includes.h"
#include "media/includes.h"
#include "asm/sdmmc.h"
#include "asm/chargestore.h"
#include "asm/charge.h"
#include "asm/pwm_led.h"
/* #include "tone_player.h" */
/* #include "audio_config.h" */
/* #include "gSensor/gSensor_manage.h" */
#include "key_event_deal.h"
#include "user_cfg.h"
#include "asm/power/p33.h"
/* #include "asm/lp_touch_key.h" */
#ifdef CONFIG_LITE_AUDIO
#include "media/includes.h"
#endif/*CONFIG_LITE_AUDIO*/

#define LOG_TAG_CONST       BOARD
#define LOG_TAG             "[BOARD]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

void board_power_init(void);

#define __this (&status_config)

/************************** KEY MSG****************************/
/*各个按键的消息设置，如果USER_CFG中设置了USE_CONFIG_KEY_SETTING为1，则会从配置文件读取对应的配置来填充改结构体*/
u8 key_table[KEY_NUM_MAX][KEY_EVENT_MAX] = {
    // SHORT           LONG              HOLD              UP              DOUBLE           TRIPLE
    /* {KEY_MUSIC_PP,   KEY_POWEROFF,  KEY_POWEROFF_HOLD,  KEY_NULL,     KEY_CALL_LAST_NO,     KEY_NULL},   //KEY_0 */
    /* {KEY_MUSIC_NEXT, KEY_VOL_UP,    KEY_VOL_UP,         KEY_NULL,     KEY_OPEN_SIRI,        KEY_NULL},   //KEY_1 */
    /* {KEY_MUSIC_PREV, KEY_VOL_DOWN,  KEY_VOL_DOWN,       KEY_NULL,     KEY_HID_CONTROL,      KEY_NULL},   //KEY_2 */
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
        .connect_way = TCFG_IOKEY_POWER_CONNECT_WAY,          //IO按键的连接方式
        .key_type.one_io.port = TCFG_IOKEY_POWER_ONE_PORT,    //IO按键对应的引脚
        .key_value = 0,                                       //按键值
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


/************************** DAC ****************************/
#if TCFG_AUDIO_DAC_ENABLE
struct dac_platform_data dac_data = {
    .ldo_volt       = TCFG_AUDIO_DAC_LDO_VOLT,                   //DACVDD等级
    .vcmo_en        = 0,                                         //是否打开VCOMO
    .output         = TCFG_AUDIO_DAC_CONNECT_MODE,               //DAC输出配置，和具体硬件连接有关，需根据硬件来设置
    .ldo_isel       = 0,
    .ldo_fb_isel    = 0,
    .lpf_isel       = 0x2,
    .dsm_clk        = DAC_DSM_6MHz,
};
#endif

/************************** ADC ****************************/
#if TCFG_AUDIO_ADC_ENABLE
const struct ladc_port ladc_list[] = {
	{// 0
		.channel = TCFG_AUDIO_ADC_LINE_CHA,
	},
	// total must < 4
};
struct adc_platform_data adc_data = {
/*MIC 是否省隔直电容：0: 不省电容  1: 省电容 */
	.mic_capless    = TCFG_MIC_CAPLESS_ENABLE,
/*差分mic使能,差分mic不能使用省电容模式*/
	.mic_diff    = 0,
/*MIC LDO电流档位设置：0:5    1:10ua    2:15ua    3:20ua*/
	.mic_ldo_isel   = TCFG_AUDIO_ADC_LD0_SEL,
/*MIC LDO电压档位设置,也会影响MIC的偏置电压0:1.5v  1:8v  2:2.1v  3:2.4v 4:2.7v 5:3.0 */
	.mic_ldo_vsel  = 3,
/*MIC免电容方案需要设置，影响MIC的偏置电压
    21:1.18K	20:1.42K 	19:1.55K 	18:1.99K 	17:2.2K 	16:2.4K 	15:2.6K		14:2.91K	13:3.05K 	12:3.5K 	11:3.73K
	10:3.91K  	9:4.41K 	8:5.0K  	7:5.6K		6:6K		5:6.5K		4:7K		3:7.6K		2:8.0K		1:8.5K				*/
    .mic_bias_res   = 18,
/*MIC电容隔直模式使用内部mic偏置(PA2)*/
	.mic_bias_inside = 1,
/*保持内部mic偏置(PA2)输出*/
	.mic_bias_keep = 0,

/*MIC1 是否省隔直电容：0: 不省电容  1: 省电容 */
	.mic1_capless    = TCFG_MIC1_CAPLESS_ENABLE,
/*差分mic使能,差分mic不能使用省电容模式*/
	.mic1_diff    = 0,
/*MIC1 LDO电流档位设置：0:5    1:10ua    2:15ua    3:20ua*/
	.mic1_ldo_isel   = TCFG_AUDIO_ADC_LD0_SEL,
/*MIC1 LDO电压档位设置,也会影响MIC的偏置电压0:1.5v  1:8v  2:2.1v  3:2.4v 4:2.7v 5:3.0 */
	.mic1_ldo_vsel  = 3,
/*MIC1免电容方案需要设置，影响MIC的偏置电压
    21:1.18K	20:1.42K 	19:1.55K 	18:1.99K 	17:2.2K 	16:2.4K 	15:2.6K		14:2.91K	13:3.05K 	12:3.5K 	11:3.73K
	10:3.91K  	9:4.41K 	8:5.0K  	7:5.6K		6:6K		5:6.5K		4:7K		3:7.6K		2:8.0K		1:8.5K				*/
    .mic1_bias_res   = 18,
/*MIC1电容隔直模式使用内部mic偏置(PB7)*/
	.mic1_bias_inside = 1,
/*保持内部mic偏置(PB7)输出*/
	.mic1_bias_keep = 0,

	// ladc 通道
    .ladc_num = ARRAY_SIZE(ladc_list),
    .ladc = ladc_list,
};
#endif




/************************** PWM_LED ****************************/
#if TCFG_PWMLED_ENABLE
LED_PLATFORM_DATA_BEGIN(pwm_led_data)
	.io_mode = TCFG_PWMLED_IOMODE,              //推灯模式设置:支持单个IO推两个灯和两个IO推两个灯
	.io_cfg.one_io.pin = TCFG_PWMLED_PIN,       //单个IO推两个灯的IO口配置
LED_PLATFORM_DATA_END()
#endif

#if 0
const struct soft_iic_config soft_iic_cfg[] = {
    //iic0 data
    {
        .scl = TCFG_SW_I2C0_CLK_PORT,                   //IIC CLK脚
        .sda = TCFG_SW_I2C0_DAT_PORT,                   //IIC DAT脚
        .delay = TCFG_SW_I2C0_DELAY_CNT,                //软件IIC延时参数，影响通讯时钟频率
        .io_pu = 1,                                     //是否打开上拉电阻，如果外部电路没有焊接上拉电阻需要置1
    },
#if 0
    //iic1 data
    {
        .scl = IO_PORTA_05,
        .sda = IO_PORTA_06,
        .delay = 50,
        .io_pu = 1,
    },
#endif
};


const struct hw_iic_config hw_iic_cfg[] = {
    //iic0 data
    {
        /*硬件IIC端口下选择
 			    SCL         SDA
		  	{IO_PORT_DP,  IO_PORT_DM},    //group a
        	{IO_PORTC_04, IO_PORTC_05},  //group b
        	{IO_PORTC_02, IO_PORTC_03},  //group c
        	{IO_PORTA_05, IO_PORTA_06},  //group d
         */
        .port = TCFG_HW_I2C0_PORTS,
        .baudrate = TCFG_HW_I2C0_CLK,      //IIC通讯波特率
        .hdrive = 0,                       //是否打开IO口强驱
        .io_filter = 1,                    //是否打开滤波器（去纹波）
        .io_pu = 1,                        //是否打开上拉电阻，如果外部电路没有焊接上拉电阻需要置1
    },
};
#endif

REGISTER_DEVICES(device_table) = {
    /* { "audio", &audio_dev_ops, (void *) &audio_data }, */

#if TCFG_CHARGE_ENABLE
    { "charge", &charge_dev_ops, (void *)&charge_data },
#endif
#if TCFG_SD0_ENABLE
	{ "sd0", 	&sd_dev_ops, 	(void *) &sd0_data},
#endif
};

/************************** LOW POWER config ****************************/
const struct low_power_param power_param = {
    .config         = TCFG_LOWPOWER_LOWPOWER_SEL,          //0：sniff时芯片不进入低功耗  1：sniff时芯片进入powerdown
    .btosc_hz       = TCFG_CLOCK_OSC_HZ,                   //外接晶振频率
    .delay_us       = TCFG_CLOCK_SYS_HZ / 1000000L,        //提供给低功耗模块的延时(不需要需修改)
    .btosc_disable  = TCFG_LOWPOWER_BTOSC_DISABLE,         //进入低功耗时BTOSC是否保持
    .vddiom_lev     = TCFG_LOWPOWER_VDDIOM_LEVEL,          //强VDDIO等级,可选：2.0V  2.2V  2.4V  2.6V  2.8V  3.0V  3.2V  3.6V
    .vddiow_lev     = TCFG_LOWPOWER_VDDIOW_LEVEL,          //弱VDDIO等级,可选：2.1V  2.4V  2.8V  3.2V
    .osc_type       = TCFG_LOWPOWER_OSC_TYPE,
	.lpctmu_en 		= 0,
	.vddio_keep     = 0,
};



/************************** PWR config ****************************/
struct port_wakeup port0 = {
    .pullup_down_enable = ENABLE,                            //配置I/O 内部上下拉是否使能
    .edge               = FALLING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
    .both_edge          = 0,
    .filter             = PORT_FLT_8ms,

#if TCFG_ADKEY_ENABLE
	.iomap              = TCFG_ADKEY_PORT,         //唤醒口选择
#endif

#if TCFG_IOKEY_ENABLE
	.iomap              = TCFG_IOKEY_POWER_ONE_PORT,         //唤醒口选择
#endif

};

#if TCFG_CHARGE_ENABLE
struct port_wakeup charge_port = {
    .edge               = RISING_EDGE,                       //唤醒方式选择,可选：上升沿\下降沿
    .both_edge          = 0,
    .filter             = PORT_FLT_16ms,
    .iomap              = IO_CHGFL_DET,                      //唤醒口选择
};

struct port_wakeup vbat_port = {
    .edge               = RISING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
    .both_edge          = 0,
    .filter             = PORT_FLT_16ms,
    .iomap              = IO_VBTCH_DET,                      //唤醒口选择
};

struct port_wakeup ldoin_rise_port = {
    .edge               = RISING_EDGE,                       //唤醒方式选择,可选：上升沿\下降沿
    .both_edge          = 1,
    .filter             = PORT_FLT_2ms,
    .iomap              = IO_LDOIN_DET,                      //唤醒口选择
};
#endif

#if TCFG_CHARGE_ENABLE || TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE
struct port_wakeup ldoin_fall_port = {
    .edge               = FALLING_EDGE,                      //唤醒方式选择,可选：上升沿\下降沿
    .both_edge          = 1,
    .filter             = PORT_FLT_2ms,
    .iomap              = IO_LDOIN_DET,                      //唤醒口选择
};
#endif

const struct wakeup_param wk_param = {

#if TCFG_IOKEY_ENABLE
	.port[1] = &port0,
#endif

#if TCFG_ADKEY_ENABLE
	.port[1] = &port0,
#endif


#if TCFG_CHARGE_ENABLE
    .port[2] = &charge_port,
    .port[3] = &vbat_port,
    .port[4] = &ldoin_rise_port,
#endif
#if TCFG_CHARGE_ENABLE || TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE
    .port[5] = &ldoin_fall_port,
#endif
};

void gSensor_wkupup_disable(void)
{
    log_info("gSensor wkup disable\n");
    power_wakeup_index_disable(1);
}

void gSensor_wkupup_enable(void)
{
    log_info("gSensor wkup enable\n");
    power_wakeup_index_enable(1);
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

    return 0;
}

//-----------------------------------------------

static void board_devices_init(void)
{
#if TCFG_PWMLED_ENABLE
    pwm_led_init(&pwm_led_data);
#endif

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

#if TCFG_AUDIO_ADC_ENABLE
    //针对硅mic要输出1给mic供电
    /*if(!adc_data.mic_capless){
        gpio_set_pull_up(IO_PORTA_04, 0);
        gpio_set_pull_down(IO_PORTA_04, 0);
        gpio_set_direction(IO_PORTA_04, 0);
        gpio_set_output_value(IO_PORTA_04,1);
    }*/
#endif

 #if TCFG_UART0_ENABLE
    if (uart0_data.rx_pin < IO_MAX_NUM) {
        gpio_set_die(uart0_data.rx_pin, 1);
    }
#endif

#if(!TCFG_CHARGE_ENABLE)
	/*close FAST CHARGE */
	 CHARGE_EN(0);
	 CHGBG_EN(0);
#endif

}



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

    boot_soft_flag.flag3.pc3_pc5.pc3 = SOFTFLAG_HIGH_RESISTANCE; //touch key power support
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
#if TCFG_CHARGE_ENABLE
    .port[2] = &charge_port,
    .port[3] = &vbat_port,
    .port[4] = &ldoin_rise_port,
#endif
#if TCFG_CHARGE_ENABLE || TCFG_CHARGESTORE_ENABLE || TCFG_TEST_BOX_ENABLE
    .port[5] = &ldoin_fall_port,
#endif
	.port[6] = &add_port,
};

//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
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
        [PORTA_GROUP] = 0xffff,
        [PORTB_GROUP] = 0xffff,
        [PORTC_GROUP] = 0xffff,
        [PORTD_GROUP] = 0xffff,
    };

#if TCFG_ADKEY_ENABLE
    port_protect(port_group,TCFG_ADKEY_PORT);
#endif

#if TCFG_IOKEY_ENABLE
	port_protect(port_group, TCFG_IOKEY_POWER_ONE_PORT);
    /* port_protect(port_group, TCFG_IOKEY_PREV_ONE_PORT); */
    /* port_protect(port_group, TCFG_IOKEY_NEXT_ONE_PORT); */
#endif /* TCFG_IOKEY_ENABLE */

    //< close gpio
    gpio_dir(GPIOA, 0, 9, port_group[PORTA_GROUP], GPIO_OR);
    gpio_set_pu(GPIOA, 0, 9, ~port_group[PORTA_GROUP], GPIO_AND);
    gpio_set_pd(GPIOA, 0, 9, ~port_group[PORTA_GROUP], GPIO_AND);
    gpio_die(GPIOA, 0, 9, ~port_group[PORTA_GROUP], GPIO_AND);
    gpio_dieh(GPIOA, 0, 9, ~port_group[PORTA_GROUP], GPIO_AND);

	/* gpio_dir(GPIOB, 0, 9, port_group[PORTB_GROUP], GPIO_OR); */
	/* gpio_set_pu(GPIOB, 0, 9, ~port_group[PORTB_GROUP], GPIO_AND); */
	/* gpio_set_pd(GPIOB, 0, 9, ~port_group[PORTB_GROUP], GPIO_AND); */
	/* gpio_die(GPIOB, 0, 9, ~port_group[PORTB_GROUP], GPIO_AND); */
	/* gpio_dieh(GPIOB, 0, 9, ~port_group[PORTB_GROUP], GPIO_AND); */

	/* gpio_dir(GPIOC, 0, 8, port_group[PORTC_GROUP], GPIO_OR); */
	/* gpio_set_pu(GPIOC, 0, 8, ~port_group[PORTC_GROUP], GPIO_AND); */
	/* gpio_set_pd(GPIOC, 0, 8, ~port_group[PORTC_GROUP], GPIO_AND); */
	/* gpio_die(GPIOC, 0, 8, ~port_group[PORTC_GROUP], GPIO_AND); */
	/* gpio_dieh(GPIOC, 0, 8, ~port_group[PORTC_GROUP], GPIO_AND); */

	/* gpio_dir(GPIOD, 0, 8, port_group[PORTD_GROUP], GPIO_OR); */
	/* gpio_set_pu(GPIOD, 0, 8, ~port_group[PORTD_GROUP], GPIO_AND); */
	/* gpio_set_pd(GPIOD, 0, 8, ~port_group[PORTD_GROUP], GPIO_AND); */
	/* gpio_die(GPIOD, 0, 8, ~port_group[PORTD_GROUP], GPIO_AND); */
	/* gpio_dieh(GPIOD, 0, 8, ~port_group[PORTD_GROUP], GPIO_AND); */

    //< close usb io
    usb_iomode(1);

	//dp dm io
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

    /* printf("JL_USB_IO->CON0=0x%x\r\n", JL_USB_IO->CON0); */
    /* printf("JL_USB_IO->CON1=0x%x\r\n", JL_USB_IO->CON1); */
    /* printf("JL_USB->CON0=0x%x\r\n", JL_USB->CON0); */
    /*  */
    /* printf("JL_USB1_IO->CON0=0x%x\r\n", JL_USB1_IO->CON0); */
    /* printf("JL_USB1_IO->CON1=0x%x\r\n", JL_USB1_IO->CON1); */
    /* printf("JL_USB1->CON0=0x%x\r\n", JL_USB1->CON0); */
}


/*进软关机之前默认将IO口都设置成高阻状态，需要保留原来状态的请修改该函数*/
extern void dac_power_off(void);
void board_set_soft_poweroff(void)
{
    u32 porta_value = 0xffff;

	//保留长按Reset Pin - PB1
    u32 portb_value = 0xffff & (~BIT(1));
    u32 portc_value = 0xffff;

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
#if TCFG_AUDIO_ENABLE
        dac_power_off();
#endif/*TCFG_AUDIO_ENABLE*/
	} else {
		close_gpio();
		/* gpio_set_pull_up(IO_PORTA_03, 0); */
        /* gpio_set_pull_down(IO_PORTA_03, 0); */
        /* gpio_set_direction(IO_PORTA_03, 1); */

        /* usb_iomode(1); */

        /* gpio_set_pull_up(IO_PORT_DP, 0); */
        /* gpio_set_pull_down(IO_PORT_DP, 0); */
        /* gpio_set_direction(IO_PORT_DP, 1); */
        /* gpio_set_die(IO_PORT_DP, 0); */

        /* gpio_set_pull_up(IO_PORT_DM, 0); */
        /* gpio_set_pull_down(IO_PORT_DM, 0); */
        /* gpio_set_direction(IO_PORT_DM, 1); */
        /* gpio_set_die(IO_PORT_DM, 0); */
    }
}

static void wl_audio_clk_on(void)
{
    JL_WL_AUD->CON0 = 1;
}

//-----------------------------------------------

void board_power_init(void)
{
    log_info("Power init : %s", __FILE__);
	gpio_direction_output(IO_PORTC_03, 1); //touch chip power support

    power_init(&power_param);

    power_set_callback(TCFG_LOWPOWER_LOWPOWER_SEL, sleep_enter_callback, sleep_exit_callback, board_set_soft_poweroff);

    wl_audio_clk_on();

    power_keep_dacvdd_en(0);

	power_wakeup_init(&wk_param);

#if (!TCFG_IOKEY_ENABLE && !TCFG_ADKEY_ENABLE)
    charge_check_and_set_pinr(1);
#endif
}

/* static void board_power_wakeup_init(void) */
/* { */
	/* power_wakeup_init(&wk_param); */
/* #if TCFG_POWER_ON_NEED_KEY */
	/* extern u8 get_por_flag(void); */
	/* if (get_por_flag()) { */
		/* power_set_soft_poweroff(); */
	/* } */
/* #endif */
/* } */

#endif /* #ifdef CONFIG_BOARD_AC697X_DEMO */
