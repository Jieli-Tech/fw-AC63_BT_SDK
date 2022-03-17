#include "includes.h"
#include "asm/power/p11.h"
#include "asm/power/p33.h"
#include "asm/lp_touch_key.h"
#include "device/key_driver.h"
#include "lp_touch_key_epd.h"
/* #include "bt_tws.h" */
#include "classic/tws_api.h"
#include "key_event_deal.h"
#include "btstack/avctp_user.h"
#include "app_config.h"

#define LOG_TAG_CONST       LP_KEY
#define LOG_TAG             "[LP_KEY]"
/* #define LOG_ERROR_ENABLE */
/* #define LOG_DEBUG_ENABLE */
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

#if (TCFG_LP_TOUCH_KEY_ENABLE || TCFG_LP_EARTCH_KEY_ENABLE)

//需要开debug的情况
//1.内置触摸调试工具
//2.入耳检测
#define CTMU_CH0_MODULE_DEBUG 	0
#define CTMU_CH1_MODULE_DEBUG 	0

#define TWS_FUNC_ID_VOL_LP_KEY      TWS_FUNC_ID('L', 'K', 'E', 'Y')
#define TWS_BT_SEND_CH0_RES_DATA_ENABLE 	0
#define TWS_BT_SEND_CH1_RES_DATA_ENABLE 	0
#define TWS_BT_SEND_EVENT_ENABLE 			0

#if CFG_CH1_USE_ALGORITHM_ENABLE

#ifdef CTMU_CH1_MODULE_DEBUG
#undef CTMU_CH1_MODULE_DEBUG
#endif

#define CTMU_CH1_MODULE_DEBUG 				1

#endif /* #if CFG_CH1_USE_ALGORITHM_ENABLE */



#define CTMU_CH0_PORT 	IO_PORTB_01
#define CTMU_CH1_PORT 	IO_PORTB_02

#define CTMU_INIT_CH0_ENABLE 		BIT(0)
#define CTMU_INIT_CH0_DEBUG 		BIT(1)
#define CTMU_INIT_CH0_FALL_ENABLE 	BIT(2)
#define CTMU_INIT_CH0_RAISE_ENABLE 	BIT(3)
#define CTMU_INIT_CH_PORT_INV 		BIT(4)
#define CTMU_INIT_CH0_SHORT_ENABLE 	BIT(5)

#define CTMU_INIT_CH1_ENABLE 		BIT(6)
#define CTMU_INIT_CH1_DEBUG 		BIT(7)

enum CTMU_P2M_EVENT {
    CTMU_P2M_CH0_DEBUG_EVENT = 0x50,
    CTMU_P2M_CH0_SHORT_EVENT,
    CTMU_P2M_CH0_LONG_EVENT,
    CTMU_P2M_CH0_FALLING_EVENT,
    CTMU_P2M_CH0_RAISING_EVENT,

    CTMU_P2M_CH1_DEBUG_EVENT,
    CTMU_P2M_CH1_IN_EVENT,
    CTMU_P2M_CH1_OUT_EVENT,
};


enum CTMU_M2P_CMD {
    CTMU_M2P_INIT = 0x50,
    CTMU_M2P_DISABLE, 		//模块关闭
    CTMU_M2P_ENABLE,  		//模块使能
    CTMU_M2P_CH0_ENABLE, 	//通道0打开
    CTMU_M2P_CH0_DISABLE, 	//通道0关闭
    CTMU_M2P_CH1_ENABLE, 	//通道1打开
    CTMU_M2P_CH1_DISABLE, 	//通道0关闭
    CTMU_M2P_UPDATE_BASE_TIME, 	//更新时基参数
    CTMU_M2P_CHARGE_ENTER_MODE, //进仓充电模式
    CTMU_M2P_CHARGE_EXIT_MODE,  //退出充电模式
};


//================= 关于模块内部的计算 ==================//
#define CTMU_DEFAULT_LRC_FREQ 			32000 			//默认LRC频率(Hz)
#define CTMU_DEFAULT_LRC_PRD 			(1000000 / CTMU_DEFAULT_LRC_FREQ)	//默认LRC周期(ms)
#define CTMU_TIME_BASE 					10 	//ms
#define CTMU_SHORT_CLICK_WINDOW_TIME 	CTMU_LONG_CLICK_DELAY_TIME
#define CTMU_LONG_CLICK_WINDOW_TIME 	1000 	//ms

#define CFG_M2P_CTMU_CH0_SHORT_TIME 	0 //(CTMU_SHORT_CLICK_WINDOW_TIME / 10 - 1)
#define CFG_M2P_CTMU_CH0_LONG_TIME 		(CTMU_LONG_CLICK_DELAY_TIME / 10 - 1)
//时基计算:
#define CFG_M2P_CTMU_BASE_TIME_PRD 		((CTMU_DEFAULT_LRC_FREQ * CTMU_TIME_BASE) / 1000 - 1)

//采样窗口时间:
//#define CFG_CTMU_CH0_PRD1_TIME 			2500 		//CH0, 单位: us
#define CFG_CTMU_CH0_PRD1_TIME 			4000 		//CH0, 单位: us
#define CFG_M2P_CTMU_CH0_PRD1_VALUE 	((CFG_CTMU_CH0_PRD1_TIME / CTMU_DEFAULT_LRC_PRD) + 1) 	//换算
#define CFG_CTMU_CH1_PRD1_TIME 			4000 		//CH1, 单位: us
#define CFG_M2P_CTMU_CH1_PRD1_VALUE 	((CFG_CTMU_CH1_PRD1_TIME / CTMU_DEFAULT_LRC_PRD) + 1) 	//换算

#define CTMU_RESET_TIMER_PRD_VALUE 		200 	//(ms)


#if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
#ifdef CTMU_RESET_TIME_CONFIG
#undef CTMU_RESET_TIME_CONFIG
#define CTMU_RESET_TIME_CONFIG 		0
#endif /* #ifdef CTMU_RESET_TIME_CONFIG */
#endif /* #if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE */


enum {
    BT_CH0_RES_MSG,
    BT_CH1_RES_MSG,
    BT_EVENT_HW_MSG,
    BT_EVENT_SW_MSG,
    BT_EVENT_VDDIO,
};

struct ctmu_key {
    u8 init;
    u8 ch_init;
    u8 click_cnt;
    u8 last_key;
    u8 last_ear_in_state;
    u8 ch0_msg_lock;
    u8 softoff_mode;
    u16 ch0_msg_lock_timer;
    u16 short_timer;
    u16 long_timer;
    u16 ear_in_timer;
    u32 lrc_hz;
    u8 ch1_inear_ok;
    u16 ch1_trim_value;
    u16 ch1_trim_flag;
    const struct lp_touch_key_platform_data *config;
};

enum ctmu_key_event {
    CTMU_KEY_NULL,
    CTMU_KEY_SHORT_CLICK,
    CTMU_KEY_LONG_CLICK,
    CTMU_KEY_HOLD_CLICK,
};

enum ch1_event_list {
    CH1_EAR_IN,
    CH1_EAR_OUT,
};

enum LP_TOUCH_SOFTOFF_MODE {
    LP_TOUCH_SOFTOFF_MODE_LEGACY  = 0, //普通关机
    LP_TOUCH_SOFTOFF_MODE_ADVANCE = 1, //带触摸关机
};

static struct ctmu_key _ctmu_key = {
    .click_cnt = 0,
    .last_ear_in_state = CH1_EAR_OUT,
    .short_timer = 0xFFFF,
    .long_timer = 0xFFFF,
    .ear_in_timer = 0xFFFF,
    .last_key = CTMU_KEY_NULL,
};

struct ch_adjust_table {
    u16 cfg0;
    u16 cfg1;
    u16 cfg2;
};

//cap(电容)检测灵敏度级数配置
//模具厚度, 触摸片面积有关
//模具越厚, 触摸片面积越大, 触摸时电容变化量
//cap检测灵敏度级数配置建议从级数0开始调, 选取合适的灵敏度;

//======================================================//
// 内置触摸灵敏度表, 由内置触摸灵敏度调试工具生成 		//
// 请将该表替换sdk中lp_touch_key.c中同名的参数表        //
//======================================================//
const static struct ch_adjust_table ch0_sensitivity_table[] = {
    /*  cfg0 		cfg1 		cfg2 */
    {15, 		20, 		300}, //cap检测灵敏度级数0
    {15, 		20, 		270}, //cap检测灵敏度级数1
    {15, 		20, 		240}, //cap检测灵敏度级数2
    {15, 		20, 		210}, //cap检测灵敏度级数3
    {15, 		20, 		180}, //cap检测灵敏度级数4
    {15, 		20, 		150}, //cap检测灵敏度级数5
    {15, 		20, 		120}, //cap检测灵敏度级数6
    {15, 		20, 		 90}, //cap检测灵敏度级数7
    {10, 		15, 		 60}, //cap检测灵敏度级数8
    {10, 		15, 		 30}, //cap检测灵敏度级数9
};


const static struct ch_adjust_table ch1_sensitivity_table[] = {
    /*  cfg0 		cfg1 		cfg2 */
    {15, 		20, 		230}, //cap检测灵敏度级数0
    {15, 		20, 		210}, //cap检测灵敏度级数1
    {15, 		20, 		190}, //cap检测灵敏度级数2
    {15, 		20, 		170}, //cap检测灵敏度级数3
    {15, 		20, 		150}, //cap检测灵敏度级数4
    {15, 		20, 		130}, //cap检测灵敏度级数5
    {15, 		20, 		110}, //cap检测灵敏度级数6
    {15, 		20, 		 80}, //cap检测灵敏度级数7
    {10, 		15, 		 70}, //cap检测灵敏度级数8
    {10, 		15, 		 50}, //cap检测灵敏度级数9
};

#define CH1_SOFT_INEAR_VAL  180
#define CH1_SOFT_OUTEAR_VAL 120

int eartch_event_deal_init(void);

#define __this 		(&_ctmu_key)

static volatile u8 is_lpkey_active = 0;

//init io HZ
static void ctmu_port_init(u8 port)
{
    gpio_set_die(port, 0);
    gpio_set_dieh(port, 0);
    gpio_set_direction(port, 1);
    gpio_set_pull_down(port, 0);
    gpio_set_pull_up(port, 0);
}

static void lp_touch_key_send_cmd(enum CTMU_M2P_CMD cmd)
{
    M2P_CTMU_CMD = cmd;
    P11_M2P_INT_SET = BIT(M2P_CTMU_INDEX);
}

#if TCFG_LP_EARTCH_KEY_ENABLE
void eartch_hardware_recover(void)
{
    lp_touch_key_send_cmd(CTMU_M2P_CH1_ENABLE);
}


void eartch_hardware_suspend(void)
{
    __this->ch_init &= ~BIT(1);
    lp_touch_key_send_cmd(CTMU_M2P_CH1_DISABLE);
}
#endif /* #if TCFG_LP_EARTCH_KEY_ENABLE */

#define IO_PORT_RESET_PORT_LDOIN   	LINDT_IN//IO编号
#define IO_RESET_PORTB_01 			11

void lp_touch_key_init(const struct lp_touch_key_platform_data *config)
{
    log_info("%s >>>>", __func__);

    ASSERT(config && (__this->init == 0));
    __this->config = config;

    M2P_CTMU_MSG = 0;
    //长按复位检测
    u8 pinr_io;
    if (P33_CON_GET(P3_PINR_CON) & BIT(0)) {
        pinr_io = P33_CON_GET(P3_PORT_SEL0);
        if (pinr_io == IO_RESET_PORTB_01) {
            P33_CON_SET(P3_PINR_CON, 0, 1, 0);
            p33_tx_1byte(P3_PORT_SEL0, IO_PORT_RESET_PORT_LDOIN);
            P33_CON_SET(P3_PINR_CON, 2, 1, 1);
            P33_CON_SET(P3_PINR_CON, 0, 1, 1);
            log_info("reset pin change: old: %d, new: %d, P33_PINR_CON = 0x%x", pinr_io, P33_CON_GET(P3_PORT_SEL0), P33_CON_GET(P3_PINR_CON));
        }
    }

    u8 ch0_sensity = __this->config->ch0.sensitivity;
    u8 ch1_sensity = __this->config->ch1.sensitivity;

    LP_TOUCH_KEY_CONFIG tool_cfg;
    int ret = syscfg_read(CFG_LP_TOUCH_KEY_ID, &tool_cfg, sizeof(LP_TOUCH_KEY_CONFIG));
    if (ret > 0) {
        log_info("cfg_en: %d, ch0_sensity_cfg: %d, ch1_sensity_cfg: %d", tool_cfg.cfg_en, tool_cfg.touch_key_sensity_class, tool_cfg.earin_key_sensity_class);
        if (tool_cfg.cfg_en) {
            ch0_sensity = tool_cfg.touch_key_sensity_class;
            ch1_sensity = tool_cfg.earin_key_sensity_class;
        }
    } else {
        log_error("touch key cfg not exist");
    }

    log_info("ch0_sensity: %d, ch1_sensity: %d", ch0_sensity, ch1_sensity);

    if (__this->config->ch0.enable) {
        if (__this->config->ch0.port == IO_PORTB_02) {
            M2P_CTMU_MSG |= CTMU_INIT_CH_PORT_INV; 		//PB1和PB2通道互换IO;
        } else {
            ASSERT(__this->config->ch0.port == IO_PORTB_01);
        }
        ctmu_port_init(__this->config->ch0.port);

        M2P_CTMU_MSG |= CTMU_INIT_CH0_ENABLE;
#if CTMU_CH0_MODULE_DEBUG
        M2P_CTMU_MSG |= CTMU_INIT_CH0_DEBUG;
#endif
        M2P_CTMU_MSG |= CTMU_INIT_CH0_FALL_ENABLE;
        M2P_CTMU_MSG |= CTMU_INIT_CH0_RAISE_ENABLE;
        //M2P_CTMU_MSG |= CTMU_INIT_CH0_SHORT_ENABLE;
        ASSERT(ch0_sensity < ARRAY_SIZE(ch0_sensitivity_table));
        M2P_CTMU_CH0_CFG0L = ((ch0_sensitivity_table[ch0_sensity].cfg0) & 0xFF);
        M2P_CTMU_CH0_CFG0H = (((ch0_sensitivity_table[ch0_sensity].cfg0) >> 8) & 0xFF);
        M2P_CTMU_CH0_CFG1L = ((ch0_sensitivity_table[ch0_sensity].cfg1) & 0xFF);
        M2P_CTMU_CH0_CFG1H = (((ch0_sensitivity_table[ch0_sensity].cfg1) >> 8) & 0xFF);
        M2P_CTMU_CH0_CFG2L = ((ch0_sensitivity_table[ch0_sensity].cfg2) & 0xFF);
        M2P_CTMU_CH0_CFG2H = (((ch0_sensitivity_table[ch0_sensity].cfg2) >> 8) & 0xFF);

        M2P_CTMU_CH0_SHORT_TIMEL = (CFG_M2P_CTMU_CH0_SHORT_TIME & 0xFF);
        M2P_CTMU_CH0_SHORT_TIMEH = ((CFG_M2P_CTMU_CH0_SHORT_TIME >> 8) & 0xFF);
        M2P_CTMU_CH0_LONG_TIMEL = (CFG_M2P_CTMU_CH0_LONG_TIME & 0xFF);
        M2P_CTMU_CH0_LONG_TIMEH = ((CFG_M2P_CTMU_CH0_LONG_TIME >> 8) & 0xFF);

        //长按开机时间配置
        M2P_CTMU_CH0_SOFTOFF_LONG_TIMEL = (CFG_M2P_CTMU_CH0_SOFTOFF_LONG_TIME & 0xFF);
        M2P_CTMU_CH0_SOFTOFF_LONG_TIMEH = ((CFG_M2P_CTMU_CH0_SOFTOFF_LONG_TIME >> 8) & 0xFF);

        //采样窗口时间配置
        M2P_CTMU_CH0_PRD1_L = (CFG_M2P_CTMU_CH0_PRD1_VALUE & 0xFF);
        M2P_CTMU_CH0_PRD1_H = ((CFG_M2P_CTMU_CH0_PRD1_VALUE >> 8) & 0xFF);

        log_info("M2P_CTMU_CH0_CFG0L = 0x%x", M2P_CTMU_CH0_CFG0L);
        log_info("M2P_CTMU_CH0_CFG0H = 0x%x", M2P_CTMU_CH0_CFG0H);
        log_info("M2P_CTMU_CH0_CFG1L = 0x%x", M2P_CTMU_CH0_CFG1L);
        log_info("M2P_CTMU_CH0_CFG1H = 0x%x", M2P_CTMU_CH0_CFG1H);
        log_info("M2P_CTMU_CH0_CFG2L = 0x%x", M2P_CTMU_CH0_CFG2L);
        log_info("M2P_CTMU_CH0_CFG2H = 0x%x", M2P_CTMU_CH0_CFG2H);

        log_info("M2P_CTMU_CH0_SHORT_TIMEL = 0x%x", M2P_CTMU_CH0_SHORT_TIMEL);
        log_info("M2P_CTMU_CH0_SHORT_TIMEH  = 0x%x", M2P_CTMU_CH0_SHORT_TIMEH);
        log_info("M2P_CTMU_CH0_LONG_TIMEL = 0x%x", M2P_CTMU_CH0_LONG_TIMEL);
        log_info("M2P_CTMU_CH0_LONG_TIMEH  = 0x%x", M2P_CTMU_CH0_LONG_TIMEH);

        log_info("M2P_CTMU_CH0_SOFTOFF_LONG_TIMEL = 0x%x", M2P_CTMU_CH0_SOFTOFF_LONG_TIMEL);
        log_info("M2P_CTMU_CH0_SOFTOFF_LONG_TIMEH  = 0x%x", M2P_CTMU_CH0_SOFTOFF_LONG_TIMEH);
        log_info("M2P_CTMU_CH0_PRD1L = 0x%x", M2P_CTMU_CH0_PRD1_L);
        log_info("M2P_CTMU_CH0_PRD1H = 0x%x", M2P_CTMU_CH0_PRD1_H);

#if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
        M2P_CTMU_CH0_RES_SEND = 1;
#else
        M2P_CTMU_CH0_RES_SEND = 0;
#endif
    }

    if (__this->config->ch1.enable) {
        if (__this->config->ch1.port == IO_PORTB_01) {
            M2P_CTMU_MSG |= CTMU_INIT_CH_PORT_INV; 		//PB1和PB2通道互换IO;
        } else {
            ASSERT(__this->config->ch1.port == IO_PORTB_02);
        }
        ctmu_port_init(__this->config->ch1.port);

        M2P_CTMU_MSG |= CTMU_INIT_CH1_ENABLE;
#if CTMU_CH1_MODULE_DEBUG
        log_info("ctmu CH1 DEBUG");
        M2P_CTMU_MSG |= CTMU_INIT_CH1_DEBUG;
#endif

        ASSERT(ch1_sensity < ARRAY_SIZE(ch1_sensitivity_table));
        M2P_CTMU_CH1_CFG0L = ((ch1_sensitivity_table[ch1_sensity].cfg0) & 0xFF);
        M2P_CTMU_CH1_CFG0H = (((ch1_sensitivity_table[ch1_sensity].cfg0) >> 8) & 0xFF);
        M2P_CTMU_CH1_CFG1L = ((ch1_sensitivity_table[ch1_sensity].cfg1) & 0xFF);
        M2P_CTMU_CH1_CFG1H = (((ch1_sensitivity_table[ch1_sensity].cfg1) >> 8) & 0xFF);
        M2P_CTMU_CH1_CFG2L = ((ch1_sensitivity_table[ch1_sensity].cfg2) & 0xFF);
        M2P_CTMU_CH1_CFG2H = (((ch1_sensitivity_table[ch1_sensity].cfg2) >> 8) & 0xFF);
        //采样窗口时间配置
        M2P_CTMU_CH1_PRD1_L = (CFG_M2P_CTMU_CH1_PRD1_VALUE & 0xFF);
        M2P_CTMU_CH1_PRD1_H = ((CFG_M2P_CTMU_CH1_PRD1_VALUE >> 8) & 0xFF);

        log_info("M2P_CTMU_CH1_CFG0L = 0x%x", M2P_CTMU_CH1_CFG0L);
        log_info("M2P_CTMU_CH1_CFG0H = 0x%x", M2P_CTMU_CH1_CFG0H);
        log_info("M2P_CTMU_CH1_CFG1L = 0x%x", M2P_CTMU_CH1_CFG1L);
        log_info("M2P_CTMU_CH1_CFG1H = 0x%x", M2P_CTMU_CH1_CFG1H);
        log_info("M2P_CTMU_CH1_CFG2L = 0x%x", M2P_CTMU_CH1_CFG2L);
        log_info("M2P_CTMU_CH1_CFG2H = 0x%x", M2P_CTMU_CH1_CFG2H);
        log_info("M2P_CTMU_CH1_PRD1L = 0x%x", M2P_CTMU_CH1_PRD1_L);
        log_info("M2P_CTMU_CH1_PRD1H = 0x%x", M2P_CTMU_CH1_PRD1_H);
#if CFG_CH1_USE_ALGORITHM_ENABLE
        lp_touch_key_epd_init();
#endif /* #if CFG_CH1_USE_ALGORITHM_ENABLE */
#if (TCFG_EARTCH_EVENT_HANDLE_ENABLE && TCFG_LP_EARTCH_KEY_ENABLE)
        if (eartch_event_deal_init() == 0) {
            sys_timeout_add(NULL, eartch_hardware_suspend, 500);
        }
#endif /* #if (TCFG_EARTCH_EVENT_HANDLE_ENABLE && TCFG_LP_EARTCH_KEY_ENABLE) */

#if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
        M2P_CTMU_CH1_RES_SEND = 1;
#else
        M2P_CTMU_CH1_RES_SEND = 0;
#endif
        u16 trim_value;
        int ret = syscfg_read(LP_KEY_EARTCH_TRIM_VALUE, &trim_value, sizeof(trim_value));
        __this->ch1_trim_flag = 0;
        __this->ch1_inear_ok = 0;

        if (ret > 0) {
            __this->ch1_trim_value = trim_value;
            M2P_CTMU_CH1_TRIM_VALUE_L = (__this->ch1_trim_value & 0xFF);
            M2P_CTMU_CH1_TRIM_VALUE_H = ((__this->ch1_trim_value >> 8) & 0xFF);
            __this->ch1_inear_ok = 1;
            log_info("ch1_trim_value = %d", __this->ch1_trim_value);
        } else {
            __this->ch1_trim_value = 0;
            //没有trim的情况下用不了
            M2P_CTMU_CH1_TRIM_VALUE_L = (10000 & 0xFF);
            M2P_CTMU_CH1_TRIM_VALUE_H = ((10000 >> 8) & 0xFF);
        }

        //软件触摸灵敏度调试
        M2P_CTM_INEAR_VALUE_L = CH1_SOFT_INEAR_VAL & 0xFF;
        M2P_CTM_INEAR_VALUE_H = CH1_SOFT_INEAR_VAL >> 8;
        M2P_CTM_OUTEAR_VALUE_L = CH1_SOFT_OUTEAR_VAL & 0xFF;
        M2P_CTM_OUTEAR_VALUE_H = CH1_SOFT_OUTEAR_VAL >> 8;
    }

    //CTMU 时基配置
    u16 time_prd = 0;
    if (__this->lrc_hz) {
        time_prd = (__this->lrc_hz * CTMU_TIME_BASE) / 1000 - 1;
    } else {
        time_prd = CFG_M2P_CTMU_BASE_TIME_PRD;
    }
    M2P_CTMU_BASE_TIME_PRD_L = (time_prd & 0xFF);
    M2P_CTMU_BASE_TIME_PRD_H = ((time_prd >> 8) & 0xFF);
    log_info("LRC_HZ = %d", __this->lrc_hz);
    log_info("M2P_CTMU_BASE_TIME_PRD_L = 0x%x", M2P_CTMU_BASE_TIME_PRD_L);
    log_info("M2P_CTMU_BASE_TIME_PRD_H = 0x%x", M2P_CTMU_BASE_TIME_PRD_H);

    //CTMU 初始化命令
    lp_touch_key_send_cmd(CTMU_M2P_INIT);

    __this->softoff_mode = LP_TOUCH_SOFTOFF_MODE_ADVANCE;
    __this->init = 1;

#if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
    lp_touch_key_online_debug_init();
#endif/* #if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE */

}

int __attribute__((weak)) lp_touch_key_event_remap(struct sys_event *e)
{
    return true;
}

static void __ctmu_notify_key_event(struct sys_event *event)
{
    event->type = SYS_KEY_EVENT;
    event->u.key.type = KEY_DRIVER_TYPE_CTMU_TOUCH; 	//区分按键类型
    event->arg  = (void *)DEVICE_EVENT_FROM_KEY;

    if ((event->u.key.value == __this->config->ch0.key_value) && (__this->ch0_msg_lock)
#if ((defined CFG_EAROUT_NOTIFY_CH0_ONE_CLICK_EVNET) && (CFG_EAROUT_NOTIFY_CH0_ONE_CLICK_EVNET == 0))
        && (event->u.key.event ==  KEY_EVENT_CLICK)
#endif /* #if (CFG_EAROUT_NOTIFY_CH0_ONE_CLICK_EVNET == 0) */
       ) {
        return;
    }

    if (__this->config->ch1.enable) {
#if (CFG_EAROUT_NOTIFY_CH0_EVENT_SEL == CFG_EAROUT_NO_NOTIFY_CH0_ALL_EVENT)
        if ((event->u.key.value == __this->config->ch0.key_value) && (__this->last_ear_in_state == CH1_EAR_OUT)) {
            return;
        }
#elif (CFG_EAROUT_NOTIFY_CH0_EVENT_SEL == CFG_EAROUT_NO_NOTIFY_CH0_CLICK_EVENT)
        if ((event->u.key.value == __this->config->ch0.key_value) && (__this->last_ear_in_state == CH1_EAR_OUT) &&
            ((event->u.key.event ==  KEY_EVENT_CLICK) ||
             (event->u.key.event ==  KEY_EVENT_DOUBLE_CLICK) ||
             (event->u.key.event ==  KEY_EVENT_TRIPLE_CLICK)
            )) {
            return;
        }
#else
        //TODO:
#endif
    }

#if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
    if (lp_touch_key_online_debug_key_event_handle(0, event)) {
        return;
    }
#endif /* #if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE */

    if (lp_touch_key_event_remap(event)) {
        sys_event_notify(event);
    }
}

__attribute__((weak)) u8 remap_ctmu_short_click_event(u8 click_cnt, u8 event)
{
    return event;
}

static void __ctmu_short_click_time_out_handle(void *priv)
{
    struct sys_event e;
    switch (__this->click_cnt) {
    case 0:
        return;
        break;
    case 1:
        e.u.key.event = KEY_EVENT_CLICK;
        break;
    case 2:
        e.u.key.event = KEY_EVENT_DOUBLE_CLICK;
        break;
    case 3:
        e.u.key.event = KEY_EVENT_TRIPLE_CLICK;
        break;
    default:
        e.u.key.event = KEY_EVENT_TRIPLE_CLICK;
        break;
    }

    e.u.key.event = remap_ctmu_short_click_event(__this->click_cnt, e.u.key.event);

    e.u.key.value = __this->config->ch0.key_value;
    __this->short_timer = 0xFFFF;
    __this->last_key = CTMU_KEY_NULL;
    log_debug("notify key short event, cnt: %d", __this->click_cnt);
    __ctmu_notify_key_event(&e);
}

static void ctmu_short_click_handle(void)
{
    __this->last_key = CTMU_KEY_SHORT_CLICK;
    if (__this->short_timer == 0xFFFF) {
        __this->click_cnt = 1;
        __this->short_timer = usr_timeout_add(NULL, __ctmu_short_click_time_out_handle, CTMU_SHORT_CLICK_DELAY_TIME, 1);
    } else {
        __this->click_cnt++;
        usr_timer_modify(__this->short_timer, CTMU_SHORT_CLICK_DELAY_TIME);
    }
}


static void __ctmu_hold_click_time_out_handle(void *priv)
{
    log_debug("notify key hold event");

    struct sys_event e;
    e.u.key.event = KEY_EVENT_HOLD;
    e.u.key.value = __this->config->ch0.key_value;

    __ctmu_notify_key_event(&e);
    __this->last_key = CTMU_KEY_HOLD_CLICK;
}


static void ctmu_long_click_handle(void)
{
    log_debug("notify key long event");
    __this->last_key = CTMU_KEY_LONG_CLICK;

    struct sys_event e;
    e.u.key.event = KEY_EVENT_LONG;
    e.u.key.value = __this->config->ch0.key_value;
    __ctmu_notify_key_event(&e);

    if (__this->long_timer == 0xFFFF) {
        __this->long_timer = usr_timer_add(NULL, __ctmu_hold_click_time_out_handle, CTMU_HOLD_CLICK_DELAY_TIME, 1);
    }
}


static void ctmu_raise_click_handle(void)
{
    struct sys_event e = {0};
    if (__this->last_key >= CTMU_KEY_LONG_CLICK) {
        if (__this->long_timer != 0xFFFF) {
            //sys_s_hi_timer_del(__this->long_timer);
            usr_timer_del(__this->long_timer);
            __this->long_timer = 0xFFFF;
        }

        e.u.key.event = KEY_EVENT_UP;
        e.u.key.value = __this->config->ch0.key_value;
        __ctmu_notify_key_event(&e);

        __this->last_key = CTMU_KEY_NULL;
        log_debug("notify key HOLD UP event");
    } else {
        ctmu_short_click_handle();
    }
}

__attribute__((weak))
void ear_lptouch_update_state(u8 state)
{
    return;
}

extern void ear_lptouch_update_state(u8 state);
extern void eartch_state_update(u8 state);
static void __ctmu_ear_in_timeout_handle(void *priv)
{
    u8 state;

    __this->ear_in_timer = 0xFFFF;

    if (__this->config->ch1.key_value == 0xFF) {
        //使用外部自定义流程
        if (__this->last_ear_in_state == CH1_EAR_IN) {
            ear_lptouch_update_state(0);
        } else {
            ear_lptouch_update_state(1);
        }
        return;
    }

#if TCFG_EARTCH_EVENT_HANDLE_ENABLE
    if (__this->last_ear_in_state == CH1_EAR_IN) {
        state = EARTCH_STATE_IN;
    } else if (__this->last_ear_in_state == CH1_EAR_OUT) {
        state = EARTCH_STATE_OUT;
    } else {
        return;
    }

    eartch_state_update(state);
#endif /* #if TCFG_EARTCH_EVENT_HANDLE_ENABLE */
}


static void __ctmu_ch0_unlock_timeout_handle(void *priv)
{
    __this->ch0_msg_lock = false;
    __this->ch0_msg_lock_timer = 0;
}

static void ctmu_ch1_event_handle(u8 ch1_event)
{
    __this->last_ear_in_state = ch1_event;

#if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
    struct sys_event event;
    if (ch1_event == CH1_EAR_IN) {
        event.u.key.event = 10;
    } else if (ch1_event == CH1_EAR_OUT) {
        event.u.key.event = 11;
    }
    if (lp_touch_key_online_debug_key_event_handle(1, &event)) {
        return;
    }
#endif /* #if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE */

    __this->ch0_msg_lock = true;

    if (__this->ch0_msg_lock_timer == 0) {
        __this->ch0_msg_lock_timer = sys_hi_timeout_add(NULL, __ctmu_ch0_unlock_timeout_handle, 3000);
    } else {
        sys_hi_timer_modify(__this->ch0_msg_lock_timer, 3000);
    }
    __ctmu_ear_in_timeout_handle(NULL);
}

static int tws_api_send_data_test(void *data, int len, u32 func_id);

static void tws_send_event_data(int msg, int type)
{
    u32 event_data[4];
    if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
        event_data[0] = type;
        event_data[1] = jiffies_to_msecs(jiffies);
        event_data[2] = msg;
        tws_api_send_data_test(event_data, 3 * sizeof(int), TWS_FUNC_ID_VOL_LP_KEY);
    }
}

void tws_send_vddio_data(u8 data)
{
    tws_send_event_data(data, BT_EVENT_VDDIO);
}

static void tws_send_res_data(int data1, int data2, int data3, int type)
{
    u32 event_data[4];
    if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
        event_data[0] = type;
        event_data[1] = data1;
        event_data[2] = data2;
        event_data[3] = data3;
        tws_api_send_data_test(event_data, 4 * sizeof(int), TWS_FUNC_ID_VOL_LP_KEY);
    }
}

__attribute__((weak))
u32 user_send_cmd_prepare(USER_CMD_TYPE cmd, u16 param_len, u8 *param)
{
    return 0;
}

static u8 lp_touch_key_testbox_remote_test(u8 event)
{
    u8 ret = true;
    u8 key_report = 0;

    switch (event) {
    case CTMU_P2M_CH0_FALLING_EVENT:
        key_report = 0xF1;
        log_info("Notify testbox CH0 Down");
        break;
    case CTMU_P2M_CH0_RAISING_EVENT:
        key_report = 0xF2;
        log_info("Notify testbox CH0 Up");
        break;

    case CTMU_P2M_CH0_LONG_EVENT:
    case CTMU_P2M_CH0_SHORT_EVENT:
        break;
    default:
        ret = false;
        break;
    }

    if (key_report) {
        user_send_cmd_prepare(USER_CTRL_TEST_KEY, 1, &key_report); //音量加
    }

    return ret;
}

void lp_touch_key_testbox_inear_trim(u8 flag)
{
#if (!TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE)
    if (flag == 1) {
        __this->ch1_trim_flag = 1;
        __this->ch1_inear_ok = 0;
        __this->ch1_trim_value == 0;
        M2P_CTMU_CH1_RES_SEND = 1;
        log_info("__this->ch1_trim_flag = %d", __this->ch1_trim_flag);
        is_lpkey_active = 1;
    } else {
        __this->ch1_trim_flag = 0;
        M2P_CTMU_CH1_RES_SEND = 0;
        log_info("__this->ch1_trim_flag = %d", __this->ch1_trim_flag);
    }
#endif
}

//======================================================//
//                 测试盒测试标志接口                   //
//======================================================//
extern u8 testbox_get_key_action_test_flag(void *priv);

#if CFG_CH1_USE_ALGORITHM_ENABLE
extern u8 _last_state;
extern EarphoneDetection epd;
static int test[1000] = {0};
#endif

u8 last_state = CTMU_P2M_CH1_OUT_EVENT;
//extern void spp_printf(const char *format, ...);

static int lp_touch_key_testbox_test_res_handle(u8 ctmu_event);
void p33_ctmu_key_event_irq_handler()
{
    static u32 cnt0 = 0;
    static u32 cnt1 = 0;
    static u32 cnt2 = 0;
    u8 ret = 0;
    u32 data[4];
    u32 event_data[4];
    //log_debug("ctmu msg: 0x%x", ctmu_event);

    u8 ctmu_event = P2M_CTMU_KEY_EVENT;
    u16 ch0_res = 0, ch1_res = 0, ch0_iir = 0, ch1_diff = 0, ch1_trim = 0;


    if (testbox_get_key_action_test_flag(NULL)) {
        ret = lp_touch_key_testbox_remote_test(ctmu_event);
        if (ret == true) {
            return;
        }
    }

    if (lp_touch_key_testbox_test_res_handle(ctmu_event)) {
        return;
    }

    /*log_debug("ctmu msg: 0x%x", ctmu_event);*/
    switch (ctmu_event) {
    case CTMU_P2M_CH0_DEBUG_EVENT:
        ch0_res = ((P2M_CTMU_CH0_H_RES << 8) | (P2M_CTMU_CH0_L_RES));
        ch0_iir = ((P2M_CTMU_CH0_H_IIR_VALUE << 8) | P2M_CTMU_CH0_L_IIR_VALUE);
        /*printf("ch0_res: %d, ch0_iir: %d\n", ch0_res, ch0_iir);*/
        /*spp_printf("ch0_res: %d, ch0_iir: %d\n", ch0_res, ch0_iir);*/

#if TWS_BT_SEND_CH0_RES_DATA_ENABLE
        tws_send_res_data(((ch0_res << 16) | ch0_iir), 0, 0, BT_CH0_RES_MSG);
#endif /* #if TWS_BT_SEND_CH0_RES_DATA_ENABLE */


#if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
        lp_touch_key_online_debug_send(0, ch0_res);
#endif /* #if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE */

        break;


    case CTMU_P2M_CH1_DEBUG_EVENT:
        ch1_diff = ((P2M_CTMU_CH1_H_DIFF_VALUE << 8) | P2M_CTMU_CH1_L_DIFF_VALUE);
        ch1_trim = ((P2M_CTMU_CH1_H_TRIM_VALUE << 8) | P2M_CTMU_CH1_L_TRIM_VALUE);
        ch0_res = ((P2M_CTMU_CH0_H_RES << 8) | P2M_CTMU_CH0_L_RES);
        ch1_res = ((P2M_CTMU_CH1_H_RES << 8) | P2M_CTMU_CH1_L_RES);
        ch0_iir = (P2M_CTMU_CH0_H_IIR_VALUE << 8 | P2M_CTMU_CH0_L_IIR_VALUE);

        /*log_debug("ch1_diff: %d, ch1_trim: %d, ch0_res: %d, ch1_res: %d, ch0_iir: %d\n", ch1_diff, ch1_trim, ch0_res, ch1_res, ch0_iir);	*/

        /*printf("ch1_diff: %d, ch1_trim: %d, ch0_res: %d, ch1_res: %d, ch0_iir: %d\n", ch1_diff, ch1_trim, ch0_res, ch1_res, ch0_iir);*/
        //spp_printf("ch1_diff: %d, ch1_trim: %d, ch0_res: %d, ch1_res: %d, ch0_iir: %d\n", ch1_diff, ch1_trim, ch0_res, ch1_res, ch0_iir);

#if !TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
        if (__this->ch1_trim_flag) {
            if (__this->ch1_trim_value == 0) {
                __this->ch1_trim_value = ch1_diff;
            } else {
                __this->ch1_trim_value = ((ch1_diff + __this->ch1_trim_value) >> 1);
            }
            if (__this->ch1_trim_flag++ > 20) {
                __this->ch1_trim_flag = 0;
                M2P_CTMU_CH1_RES_SEND = 0;
                int ret = syscfg_write(LP_KEY_EARTCH_TRIM_VALUE, &(__this->ch1_trim_value), sizeof(__this->ch1_trim_value));
                log_info("write ret = %d", ret);
                if (ret > 0) {
                    M2P_CTMU_CH1_TRIM_VALUE_L = (__this->ch1_trim_value & 0xFF);
                    M2P_CTMU_CH1_TRIM_VALUE_H = ((__this->ch1_trim_value >> 8) & 0xFF);
                    __this->ch1_inear_ok = 1;
#if TCFG_EARTCH_EVENT_HANDLE_ENABLE
                    eartch_state_update(EARTCH_STATE_TRIM_OK);
#endif
                    log_info("trim: %d\n", __this->ch1_inear_ok);
                    is_lpkey_active = 0;
                } else {
                    __this->ch1_inear_ok = 0;
#if TCFG_EARTCH_EVENT_HANDLE_ENABLE
                    eartch_state_update(EARTCH_STATE_TRIM_ERR);
#endif
                    log_info("trim: %d\n", __this->ch1_inear_ok);
                    is_lpkey_active = 0;
                }
            }
            log_debug("ch1 trim value: %d, res = %d", __this->ch1_trim_value, ch1_diff);
        }
#endif

#if TWS_BT_SEND_CH1_RES_DATA_ENABLE
        if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
            tws_send_res_data(((ch1_trim << 16) | ch0_res), ((ch0_iir << 16) | ch1_res), ch1_diff, BT_CH1_RES_MSG);
        }

#endif /* #if TWS_BT_SEND_CH1_RES_DATA_ENABLE */

        if (P2M_CTMU_CTMU_KEY_CNT != last_state) {
            last_state = P2M_CTMU_CTMU_KEY_CNT;
            if (last_state == CTMU_P2M_CH1_IN_EVENT) {
#if TWS_BT_SEND_EVENT_ENABLE
                tws_send_event_data(CH1_EAR_IN, BT_EVENT_SW_MSG);
#endif /* #if TWS_BT_SEND_EVENT_ENABLE */
                if (__this->ch1_inear_ok) {
                    ctmu_ch1_event_handle(CH1_EAR_IN);
                }

            } else if (last_state == CTMU_P2M_CH1_OUT_EVENT) {
#if TWS_BT_SEND_EVENT_ENABLE
                tws_send_event_data(CH1_EAR_OUT, BT_EVENT_SW_MSG);
#endif /* #if TWS_BT_SEND_EVENT_ENABLE */
                if (__this->ch1_inear_ok) {
                    ctmu_ch1_event_handle(CH1_EAR_OUT);
                }

            }
        }

#if CFG_CH1_USE_ALGORITHM_ENABLE
        ret = lp_touch_key_epd_update_res(res);
        if (_last_state == EPD_OUT) {
            res |= BIT(0);
        } else {
            res &= ~BIT(0);
        }
        //tws_send_res_data(epd.tracking_down_th, BT_RES_MSG);
        //tws_send_res_data(epd.tracking_peak, BT_RES_MSG);

        if (cnt1 < ARRAY_SIZE(test)) {
            test[cnt1++] = (epd.tracking_down_th << 16) | res;
        }

        if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
            if (cnt2 < ARRAY_SIZE(test)) {
                tws_send_res_data(test[cnt2++], BT_RES_MSG);
            } else {
                tws_send_res_data((epd.tracking_down_th << 16) | res, BT_RES_MSG);
            }
        }

        //tws_send_event_data(_last_state, BT_EVENT_SW_MSG);

        if (ret != EPD_STATE_NO_CHANCE) {
            if (ret == EPD_IN) {
                log_info("algorithm SW: IN");
                ctmu_ch1_event_handle(CH1_EAR_IN);
            } else {
                log_info("algorithm SW: OUT");
                ctmu_ch1_event_handle(CH1_EAR_OUT);
            }
        }
#if TWS_BT_SEND_EVENT_ENABLE
        tws_send_event_data(ret, BT_EVENT_SW_MSG);
#endif /* #if TWS_BT_SEND_EVENT_ENABLE */

#else

#if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
        lp_touch_key_online_debug_send(1, ch1_res);
#endif /* #if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE */

#endif /* #if CFG_CH1_USE_ALGORITHM_ENABLE */

        break;

    case CTMU_P2M_CH0_SHORT_EVENT:
        log_debug("CH0: Short click");
        //ctmu_short_click_handle();
        break;
    case CTMU_P2M_CH0_LONG_EVENT:
        ctmu_long_click_handle();
        log_debug("CH0: Long click");
        break;
    case CTMU_P2M_CH0_FALLING_EVENT:
        log_debug("CH0: FALLING");
        break;
    case CTMU_P2M_CH0_RAISING_EVENT:
        if (!(__this->ch_init & BIT(0))) {
            __this->ch_init |= BIT(0);
            load_p11_bank_code2ram(1, 1);
            return;
        }
        ctmu_raise_click_handle();
        log_debug("CH0: RAISING");
        break;

    case CTMU_P2M_CH1_IN_EVENT:
#if TWS_BT_SEND_EVENT_ENABLE
        tws_send_event_data(CH1_EAR_IN, BT_EVENT_HW_MSG);
#endif /* #if TWS_BT_SEND_EVENT_ENABLE */
        log_debug("CH1 HW: IN");

#if (CFG_CH1_USE_ALGORITHM_ENABLE == 0)
        //ctmu_ch1_event_handle(CH1_EAR_IN);
#endif /* #if CFG_CH1_USE_ALGORITHM_ENABLE */
        break;
    case CTMU_P2M_CH1_OUT_EVENT:
        if (!(__this->ch_init & BIT(1))) {
            __this->ch_init |= BIT(1);
            return;
        }

#if TWS_BT_SEND_EVENT_ENABLE
        tws_send_event_data(CH1_EAR_OUT, BT_EVENT_HW_MSG);
#endif /* #if TWS_BT_SEND_EVENT_ENABLE */
        log_debug("CH1 HW: OUT");

#if (CFG_CH1_USE_ALGORITHM_ENABLE == 0)
        //ctmu_ch1_event_handle(CH1_EAR_OUT);
#endif /* #if CFG_CH1_USE_ALGORITHM_ENABLE */
        break;
    default:
        break;
    }
}


void lp_touch_key_trace_lrc_hook(u32 lrc_hz)
{
    static u8 first_time = 0;
    if (first_time) {
        return;
    }
    first_time = 1;
    log_debug("%s", __func__);
    __this->lrc_hz = lrc_hz;

    u16 cnt = (CTMU_RESET_TIMER_PRD_VALUE * lrc_hz) / (1000L * 64);
#if CTMU_RESET_TIME_CONFIG
    M2P_LCTM_RESET_PCNT_PRD1 = cnt >> 8;
    M2P_LCTM_RESET_PCNT_PRD0 = cnt & 0xff;
    M2P_LCTM_RESET_PCNT_VALUE = (CTMU_RESET_TIME_CONFIG -  CTMU_LONG_CLICK_DELAY_TIME) / CTMU_RESET_TIMER_PRD_VALUE;
#else
    M2P_LCTM_RESET_PCNT_VALUE = 0;
#endif /* #if CTMU_RESET_TIME_CONFIG */
    log_debug("lp timer cnt = %d, lrc_hz = %d, RESET_PCNT = %d", cnt, lrc_hz, M2P_LCTM_RESET_PCNT_VALUE);

    if (__this->init) {
        u16 time_prd = 0;
        time_prd = (__this->lrc_hz * CTMU_TIME_BASE) / 1000 - 1;
        M2P_CTMU_BASE_TIME_PRD_L = (time_prd & 0xFF);
        M2P_CTMU_BASE_TIME_PRD_H = ((time_prd >> 8) & 0xFF);

        lp_touch_key_send_cmd(CTMU_M2P_UPDATE_BASE_TIME);
    }
}

u8 lp_touch_key_power_on_status()
{
    extern u8 power_reset_flag;
    u8 sfr = power_reset_flag;
    static u8 power_on_flag = 0;

    log_debug("P3_RST_SRC = %x, P2M_CTMU_CTMU_WKUP_MSG = 0x%x", sfr, P2M_CTMU_CTMU_WKUP_MSG);
    if ((sfr & BIT(0)) || (sfr & BIT(1))) {
        return 0;
    }

    if (P2M_CTMU_CTMU_WKUP_MSG & BIT(0)) {
        power_on_flag = 1;
        P2M_CTMU_CTMU_WKUP_MSG &= (~(BIT(0)));
    }

    return power_on_flag;
}


void lp_touch_key_disable(void)
{
    log_debug("%s", __func__);

    while (!__this->ch_init & BIT(0)) {
        asm volatile("nop");
    }

    //__this->ch_init = 0;
    P2M_CTMU_CTMU_WKUP_MSG &= (~(BIT(1)));
    lp_touch_key_send_cmd(CTMU_M2P_DISABLE);
    while (!(P2M_CTMU_CTMU_WKUP_MSG & BIT(1)));
    __this->softoff_mode = LP_TOUCH_SOFTOFF_MODE_LEGACY;
}


void lp_touch_key_enable(void)
{
    log_debug("%s", __func__);
    lp_touch_key_send_cmd(CTMU_M2P_ENABLE);
    __this->softoff_mode = LP_TOUCH_SOFTOFF_MODE_ADVANCE;
}

void lp_touch_key_charge_mode_enter()
{
#if (!LP_TOUCH_KEY_CHARGE_MODE_SW_DISABLE)
    log_debug("%s", __func__);

    while (!__this->ch_init & BIT(0)) {
        asm volatile("nop");
    }

    //__this->ch_init = 0;
    P2M_CTMU_CTMU_WKUP_MSG &= (~(BIT(1)));
    lp_touch_key_send_cmd(CTMU_M2P_CHARGE_ENTER_MODE);
    while (!(P2M_CTMU_CTMU_WKUP_MSG & BIT(1)));
    __this->softoff_mode = LP_TOUCH_SOFTOFF_MODE_LEGACY;
#endif
}

void lp_touch_key_charge_mode_exit()
{
#if (!LP_TOUCH_KEY_CHARGE_MODE_SW_DISABLE)
    log_debug("%s", __func__);
    lp_touch_key_send_cmd(CTMU_M2P_CHARGE_EXIT_MODE);
    __this->softoff_mode = LP_TOUCH_SOFTOFF_MODE_ADVANCE;
#endif
}

//=============================================//
//NOTE: 该函数为进关机时被库里面回调
//在板级配置struct low_power_param power_param中变量lpctmu_en配置为TCFG_LP_TOUCH_KEY_ENABLE时:
//该函数在决定softoff进触摸模式还是普通模式:
//	return 1: 进触摸模式关机(LP_TOUCH_SOFTOFF_MODE_ADVANCE);
//	return 0: 进普通模式关机(触摸关闭)(LP_TOUCH_SOFTOFF_MODE_LEGACY);
//使用场景:
// 	1)在充电舱外关机, 需要触摸开机, 进带触摸关机模式;
// 	2)在充电舱内关机，可以关闭触摸模块, 进普通关机模式, 关机功耗进一步降低.
//=============================================//
u8 lp_touch_key_softoff_mode_query(void)
{
    return __this->softoff_mode;
}

//================ bt tws debug ====================//
int tws_api_send_data_to_sibling(void *data, u16 len, u32 func_id);
static int tws_api_send_data_test(void *data, int len, u32 func_id)
{
    int ret = -EINVAL;

    local_irq_disable();
    ret = tws_api_send_data_to_sibling(data, len, func_id);
    local_irq_enable();

    return ret;
}

static void res_receive_handle(void *_data, u16 len, bool rx)
{
    static u32 cnt0 = 0;
    static u32 cnt1 = 0;
    u32 *data = _data;
    if (rx) {
        if (data[0] == BT_CH0_RES_MSG) {
            /*log_debug("len = %d, RES0 = %08d", len, data[1]);*/
            log_debug("cnt: %08d, ch0: %08d, iir: %08d\n", cnt1++, (data[1] >> 16), (data[1] & 0xffff));
        } else if (data[0] == BT_CH1_RES_MSG) {
            /*log_debug("RES1 ORIGIN = %08d", data[1] & 0xFFFF);*/
            log_debug("cnt: %08d, trim: %08d, ch0: %08d, ch0_iir: %08d, ch1: %08d, ch0-ch1 %08d", cnt1++, (data[1] >> 16), data[1] & 0xFFFF, (data[2] >> 16), data[2] & 0xFFFF, (data[1] & 0xFFFF));
        } else if (data[0] == BT_EVENT_SW_MSG) {
            log_debug("len = %d, %d ms", len, data[1]);
            if (data[2] == EPD_IN) {
                log_debug("SW: IN");
            } else if (data[2] == EPD_OUT) {
                log_debug("SW: OUT");
            }
        } else if (data[0] == BT_EVENT_HW_MSG) {
            log_debug("len = %d, %d ms", len, data[1]);
            if (data[2] == CH1_EAR_IN) {
                log_debug("HW: IN");
            } else if (data[2] == CH1_EAR_OUT) {
                log_debug("HW: OUT");
            }
        } else if (data[0] == BT_EVENT_VDDIO) {
            log_debug("BT_EVENT_VDDIO: %d", data[2]);
        }
    }
}

REGISTER_TWS_FUNC_STUB(lp_touch_res_sync_stub) = {
    .func_id = TWS_FUNC_ID_VOL_LP_KEY,
    .func    = res_receive_handle, //handle
};

//==================================================//
//==============  在线调节灵敏度参数表    ==========//
//==================================================//
#if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
#include "online_db_deal.h"

//LP KEY在线调试工具版本号管理
const u8 lp_key_sdk_name[16] = "AC897N";
const u8 lp_key_bt_ver[4] 	     = {0, 0, 1, 0};
struct lp_key_ver_info {
    char sdkname[16];
    u8 lp_key_ver[4];
};

#if TCFG_LP_EARTCH_KEY_ENABLE
//版本号                     按键事件个数  							   按键事件个数
const char ch_content[] = {0x01, 'c', 'h', '0', '\0', 6, 0, 1, 2, 3, 4, 5, 'c', 'h', '1', '\0', 2, 10, 11}; //ch0 & ch1
#else
const char ch_content[] = {0x01, 'c', 'h', '0', '\0', 6, 0, 1, 2, 3, 4, 5}; //only ch0
#endif /* #if TCFG_LP_EARTCH_KEY_ENABLE */

enum {
    TOUCH_RECORD_GET_VERSION = 0x05,
    TOUCH_RECORD_GET_CH_SIZE = 0x0B,
    TOUCH_RECORD_GET_CH_CONTENT = 0x0C,
    TOUCH_RECORD_CHANGE_MODE = 0x0E,
    TOUCH_RECORD_COUNT = 0x200,
    TOUCH_RECORD_START,
    TOUCH_RECORD_STOP,
    ONLINE_OP_QUERY_RECORD_PACKAGE_LENGTH,
    TOUCH_CH_CFG_UPDATE  = 0x3000,
    TOUCH_CH_CFG_CONFIRM = 0x3100,
};

enum {
    LP_KEY_ONLINE_ST_INIT = 0,
    LP_KEY_ONLINE_ST_READY,
    LP_KEY_ONLINE_ST_CH_RES_DEBUG_START,
    LP_KEY_ONLINE_ST_CH_RES_DEBUG_STOP,
    LP_KEY_ONLINE_ST_CH_KEY_DEBUG_START,
    LP_KEY_ONLINE_ST_CH_KEY_DEBUG_CONFIRM,
};

//小机接收命令包格式, 使用DB_PKT_TYPE_TOUCH通道接收
typedef struct {
    int cmd_id;
    int mode;
    int data[];
} touch_receive_cmd_t;

//小机发送按键消息格式, 使用DB_PKT_TYPE_TOUCH通道发送
typedef struct {
    u32 cmd_id;
    u32 mode;
    u32 key_event;
} lp_touch_online_key_event_t;

typedef struct {
    u8 state;
    u8 current_record_ch;
    u16 res_packet;
    struct ch_adjust_table debug_cfg;
    lp_touch_online_key_event_t online_key_event;
} lp_touch_key_online;

static lp_touch_key_online lp_key_online = {0};

static int lp_touch_key_online_debug_key_event_handle(u8 ch_index, struct sys_event *event)
{
    int err = 0;
    if ((lp_key_online.state == LP_KEY_ONLINE_ST_CH_KEY_DEBUG_START) && (lp_key_online.current_record_ch == ch_index)) {
        lp_key_online.online_key_event.cmd_id = 0x3100;
        lp_key_online.online_key_event.mode = 0;
        lp_key_online.online_key_event.key_event = event->u.key.event;
        log_debug("send %d event to PC", lp_key_online.online_key_event.key_event);
        err = app_online_db_send(DB_PKT_TYPE_TOUCH, (u8 *)(&(lp_key_online.online_key_event)), sizeof(lp_touch_online_key_event_t));
    }

    if ((lp_key_online.state == LP_KEY_ONLINE_ST_CH_KEY_DEBUG_CONFIRM) ||
        (lp_key_online.state <= LP_KEY_ONLINE_ST_READY)) {
        return 0;
    }

    return 1;
}

static int lp_touch_key_debug_reinit(u8 update_state)
{
    log_debug("%s, current_record_ch = %d", __func__, lp_key_online.current_record_ch);

    switch (update_state) {
    case LP_KEY_ONLINE_ST_CH_RES_DEBUG_START:
        if (lp_key_online.current_record_ch == 0) {
            M2P_CTMU_MSG &= ~(CTMU_INIT_CH1_DEBUG);
            M2P_CTMU_MSG |= CTMU_INIT_CH0_DEBUG;
        } else if (lp_key_online.current_record_ch == 1) {
            M2P_CTMU_MSG &= ~(CTMU_INIT_CH0_DEBUG);
            M2P_CTMU_MSG |= CTMU_INIT_CH1_DEBUG;
        }
        break;
    case LP_KEY_ONLINE_ST_CH_RES_DEBUG_STOP:
        M2P_CTMU_MSG &= ~(CTMU_INIT_CH0_DEBUG);
        M2P_CTMU_MSG &= ~(CTMU_INIT_CH1_DEBUG);
        break;
    case LP_KEY_ONLINE_ST_CH_KEY_DEBUG_START:
        M2P_CTMU_MSG &= ~(CTMU_INIT_CH0_DEBUG);
        M2P_CTMU_MSG &= ~(CTMU_INIT_CH1_DEBUG);
        if (lp_key_online.current_record_ch == 0) {
            M2P_CTMU_CH0_CFG0L = ((lp_key_online.debug_cfg.cfg0) & 0xFF);
            M2P_CTMU_CH0_CFG0H = (((lp_key_online.debug_cfg.cfg0) >> 8) & 0xFF);
            M2P_CTMU_CH0_CFG1L = ((lp_key_online.debug_cfg.cfg1) & 0xFF);
            M2P_CTMU_CH0_CFG1H = (((lp_key_online.debug_cfg.cfg1) >> 8) & 0xFF);
            M2P_CTMU_CH0_CFG2L = ((lp_key_online.debug_cfg.cfg2) & 0xFF);
            M2P_CTMU_CH0_CFG2H = (((lp_key_online.debug_cfg.cfg2) >> 8) & 0xFF);
        } else if (lp_key_online.current_record_ch == 1) {
            M2P_CTMU_CH1_CFG0L = ((lp_key_online.debug_cfg.cfg0) & 0xFF);
            M2P_CTMU_CH1_CFG0H = (((lp_key_online.debug_cfg.cfg0) >> 8) & 0xFF);
            M2P_CTMU_CH1_CFG1L = ((lp_key_online.debug_cfg.cfg1) & 0xFF);
            M2P_CTMU_CH1_CFG1H = (((lp_key_online.debug_cfg.cfg1) >> 8) & 0xFF);
            M2P_CTMU_CH1_CFG2L = ((lp_key_online.debug_cfg.cfg2) & 0xFF);
            M2P_CTMU_CH1_CFG2H = (((lp_key_online.debug_cfg.cfg2) >> 8) & 0xFF);
        }
        break;
    case LP_KEY_ONLINE_ST_CH_KEY_DEBUG_CONFIRM:
        M2P_CTMU_MSG &= ~(CTMU_INIT_CH0_DEBUG);
        M2P_CTMU_MSG &= ~(CTMU_INIT_CH1_DEBUG);
        break;
    default:
        break;
    }


    if (__this->short_timer != 0xFFFF) {
        usr_timer_del(__this->short_timer);
        __this->short_timer = 0xFFFF;
    }
    if (__this->long_timer != 0xFFFF) {
        usr_timer_del(__this->long_timer);
        __this->long_timer = 0xFFFF;
    }
    __this->last_key = CTMU_KEY_NULL;
    __this->ch_init = 0;

    load_p11_bank_code2ram(1, 0);

    //CTMU 初始化命令
    lp_touch_key_send_cmd(CTMU_M2P_INIT);

    return 0;
}

static int lp_touch_key_online_debug_parse(u8 *packet, u8 size, u8 *ext_data, u16 ext_size)
{
    int res_data = 0;
    touch_receive_cmd_t *touch_cmd;
    int err = 0;
    u8 parse_seq = ext_data[1];
    struct ch_adjust_table *receive_cfg;
    struct lp_key_ver_info ver_info = {0};

    res_data = 4;
    log_debug("%s", __func__);
    put_buf(packet, size);
    put_buf(ext_data, ext_size);
    //memcpy(&touch_cmd, packet, sizeof(touch_receive_cmd_t));
    touch_cmd = (touch_receive_cmd_t *)packet;
    switch (touch_cmd->cmd_id) {
    /* case TOUCH_RECORD_COUNT: */
    /* log_debug("TOUCH_RECORD_COUNT"); */
    /* err = app_online_db_ack(parse_seq, (u8 *)&res_data, 4); */
    /* break; */
    case TOUCH_RECORD_START:
        log_debug("TOUCH_RECORD_START");
        err = app_online_db_ack(parse_seq, (u8 *)&res_data, 1); //该命令随便ack一个byte即可
        lp_key_online.state = LP_KEY_ONLINE_ST_CH_RES_DEBUG_START;
        lp_touch_key_debug_reinit(lp_key_online.state);
        break;
    case TOUCH_RECORD_STOP:
        log_debug("TOUCH_RECORD_STOP");
        app_online_db_ack(parse_seq, (u8 *)&res_data, 1); //该命令随便ack一个byte即可
        lp_key_online.state = LP_KEY_ONLINE_ST_CH_RES_DEBUG_STOP;
        lp_touch_key_debug_reinit(lp_key_online.state);
        break;
    /* case ONLINE_OP_QUERY_RECORD_PACKAGE_LENGTH: */
    /* log_debug("ONLINE_OP_QUERY_RECORD_PACKAGE_LENGTH"); */
    /* err = app_online_db_ack(parse_seq, (u8 *)&res_data, 4); //回复对应的通道数据长度 */
    /* break; */
    case TOUCH_CH_CFG_UPDATE:
        log_debug("TOUCH_CH_CFG_UPDATE");
        app_online_db_ack(parse_seq, (u8 *)"OK", 2); //回"OK"字符串
        lp_key_online.state = LP_KEY_ONLINE_ST_CH_KEY_DEBUG_START;

        receive_cfg = (struct ch_adjust_table *)(touch_cmd->data);
        lp_key_online.debug_cfg.cfg0 = receive_cfg->cfg0;
        lp_key_online.debug_cfg.cfg1 = receive_cfg->cfg1;
        lp_key_online.debug_cfg.cfg2 = receive_cfg->cfg2;
        log_debug("update, cfg0 = %d, cfg1 = %d, cfg2 = %d", lp_key_online.debug_cfg.cfg0, lp_key_online.debug_cfg.cfg1, lp_key_online.debug_cfg.cfg2);
        lp_touch_key_debug_reinit(lp_key_online.state);
        break;
    case TOUCH_CH_CFG_CONFIRM:
        log_debug("TOUCH_CH_CFG_CONFIRM");
        app_online_db_ack(parse_seq, (u8 *)"OK", 2); //回"OK"字符串
        lp_key_online.state = LP_KEY_ONLINE_ST_CH_KEY_DEBUG_CONFIRM;
        break;
    case TOUCH_RECORD_GET_VERSION:
        log_debug("TOUCH_RECORD_GET_VERSION");
        memcpy(ver_info.sdkname, lp_key_sdk_name, sizeof(lp_key_sdk_name));
        memcpy(ver_info.lp_key_ver, lp_key_bt_ver, sizeof(lp_key_bt_ver));
        app_online_db_ack(parse_seq, (u8 *)(&ver_info), sizeof(ver_info)); //回复版本号数据结构
        break;
    case TOUCH_RECORD_GET_CH_SIZE:
        log_debug("TOUCH_RECORD_GET_CH_SIZE");
        res_data = sizeof(ch_content);
        err = app_online_db_ack(parse_seq, (u8 *)&res_data, 4); //回复对应的通道数据长度
        break;
    case TOUCH_RECORD_GET_CH_CONTENT:
        log_debug("TOUCH_RECORD_GET_CH_CONTENT");
        app_online_db_ack(parse_seq, (u8 *)(&ch_content), sizeof(ch_content));
        break;
    case TOUCH_RECORD_CHANGE_MODE:
        log_debug("TOUCH_RECORD_CHANGE_MODE, cmd_mode = %d", touch_cmd->mode);
        lp_key_online.current_record_ch = touch_cmd->mode;
        app_online_db_ack(parse_seq, (u8 *)"OK", 2); //回"OK"字符串
        break;
    default:
        break;
    }

    return 0;
}

static int lp_touch_key_online_debug_send(u8 ch, u16 val)
{
    int err = 0;

    putchar('s');
    if (lp_key_online.state == LP_KEY_ONLINE_ST_CH_RES_DEBUG_START) {
        lp_key_online.res_packet = val;
        err = app_online_db_send(DB_PKT_TYPE_DAT_CH0, (u8 *)(&(lp_key_online.res_packet)), 2);
    }

    return err;
}

static int lp_touch_key_online_debug_init(void)
{
    log_debug("%s", __func__);
    app_online_db_register_handle(DB_PKT_TYPE_TOUCH, lp_touch_key_online_debug_parse);
    lp_key_online.state = LP_KEY_ONLINE_ST_READY;

    return 0;
}

int lp_touch_key_online_debug_exit(void)
{
    return 0;
}

#endif /* #if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE */

static u8 lpkey_idle_query(void)
{
    return !is_lpkey_active;
}
#if TCFG_LP_TOUCH_KEY_ENABLE
REGISTER_LP_TARGET(key_lp_target) = {
    .name = "lpkey",
    .is_idle = lpkey_idle_query,
};
#endif /* #if !TCFG_LP_TOUCH_KEY_ENABLE */


//======================================================//
//              测试盒变化量测试命令接收                //
//======================================================//
#define LP_TOUCH_TEST_TIMEOUT_CONFIG 			8000 //ms
#define LP_TOUCH_TEST_END_DELAY_COUNTER 		20

extern int lp_touch_key_testbox_test_cmd_send(void *priv);

enum LP_TOUCH_KEY_TESTBOX_CMD_TABLE {
    LP_TOUCH_KEY_TESTBOX_CMD_TESTMODE_NONE = 0,
    LP_TOUCH_KEY_TESTBOX_CMD_TESTMODE_ENTER = 'T',
    LP_TOUCH_KEY_TESTBOX_CMD_TEST_KEY,
    LP_TOUCH_KEY_TESTBOX_CMD_TEST_TIMEOUT_REPORT, //测试盒超时, 请求测试结果
    LP_TOUCH_KEY_TESTBOX_CMD_TESTMODE_EXIT,
};

enum LP_TOUCH_KEY_TESTBOX_ERR_TABLE {
    LP_TOUCH_KEY_TESTBOX_ERR_NONE = 'O', 			//79, 测试通过
    LP_TOUCH_KEY_TESTBOX_ERR_DOUBLE_KEY_NOT_HIT, 	//80, 未检测到双击
    LP_TOUCH_KEY_TESTBOX_ERR_DETLA, 				//81, 触摸变化量太小
};

typedef struct lp_touch_key_test_cmd {
    u8 cmd;
} LP_TOUCH_TESTBOX_CMD;


struct lp_touch_key_test_report {
    u8 result;
    u16 res_max;
    u16 res_min;
    u16 res_delta;
    u16 res_percent;
    u8 fall_cnt;
    u8 raise_cnt;
};


struct lp_touch_key_test_statis {
    u16 last_value;
    u16 probe_max;
    u16 probe_min;
    u16 res_max0;
    u16 res_min0;
    u16 res_max1;
    u16 res_min1;
    u16 end_cnt;
    u8 double_key_hit;
    u8 cur_status;
    u8 fall_cnt;
    u8 raise_cnt;
};

struct lp_touch_key_test_handle {
    u8 cur_test_status;
    struct lp_touch_key_test_statis statis;
    u32 timeout_timer;
};

static struct lp_touch_key_test_handle testbox_test_handle = {0};

static int lp_touch_key_testbox_reinit(u8 mode)
{
    if (mode) {
        M2P_CTMU_MSG &= ~(CTMU_INIT_CH1_DEBUG);
        M2P_CTMU_MSG |= CTMU_INIT_CH0_DEBUG;
        M2P_CTMU_CH0_RES_SEND = 1;
    } else {
        M2P_CTMU_MSG &= ~(CTMU_INIT_CH0_DEBUG);
        M2P_CTMU_MSG &= ~(CTMU_INIT_CH1_DEBUG);
        M2P_CTMU_CH0_RES_SEND = 0;
    }

    if (__this->short_timer != 0xFFFF) {
        usr_timer_del(__this->short_timer);
        __this->short_timer = 0xFFFF;
    }
    if (__this->long_timer != 0xFFFF) {
        usr_timer_del(__this->long_timer);
        __this->long_timer = 0xFFFF;
    }
    __this->last_key = CTMU_KEY_NULL;
    __this->ch_init = 0;

    load_p11_bank_code2ram(1, 0);

    //CTMU 初始化命令
    lp_touch_key_send_cmd(CTMU_M2P_INIT);

    return 0;
}

static void lp_touch_key_testbox_test_timeout_handle(void *priv)
{
    log_info("==== lp key test local timeout ====");
    if ((testbox_test_handle.cur_test_status == LP_TOUCH_KEY_TESTBOX_CMD_TESTMODE_ENTER) ||
        (testbox_test_handle.cur_test_status == LP_TOUCH_KEY_TESTBOX_CMD_TEST_KEY)) {
        testbox_test_handle.timeout_timer = 0;
        lp_touch_key_testbox_reinit(0);
        testbox_test_handle.cur_test_status = LP_TOUCH_KEY_TESTBOX_CMD_TESTMODE_NONE;
    }

    return;
}


static int lp_touch_key_testbox_test_report(void)
{
    struct lp_touch_key_test_report report;
    u16 detla0, detla1, target_delta;
    u32 max, min;
    if (testbox_test_handle.statis.double_key_hit) {
        detla0 = testbox_test_handle.statis.res_max0 - testbox_test_handle.statis.res_min0;
        detla1 = testbox_test_handle.statis.res_max1 - testbox_test_handle.statis.res_min1;
        if (detla1 > detla0) {
            report.res_max = testbox_test_handle.statis.res_max1;
            report.res_min = testbox_test_handle.statis.res_min1;
        } else {
            report.res_max = testbox_test_handle.statis.res_max0;
            report.res_min = testbox_test_handle.statis.res_min0;
        }
    } else {
        report.res_max = testbox_test_handle.statis.probe_max;
        report.res_min = testbox_test_handle.statis.probe_min;
    }

    report.res_delta = report.res_max - report.res_min;
    max = report.res_max;
    min = report.res_min;
    if ((max && min) && (max > min)) {
        report.res_percent = (max - min) * 100 / max;
    } else {
        log_info("max err");
        report.res_percent = 0;
    }
    report.fall_cnt = testbox_test_handle.statis.fall_cnt;
    report.raise_cnt = testbox_test_handle.statis.raise_cnt;
    //
    u8 index = __this->config->ch0.sensitivity;
    target_delta = ch0_sensitivity_table[2].cfg2; //取工具生成的1级灵敏度, 标定过良好样机的30%, 小于30%, 确认为不良
    if ((report.res_delta < 100) || ((report.res_delta) < target_delta)) {
        report.result = LP_TOUCH_KEY_TESTBOX_ERR_DETLA;
    } else if (testbox_test_handle.statis.double_key_hit == 0) {
        report.result = LP_TOUCH_KEY_TESTBOX_ERR_DOUBLE_KEY_NOT_HIT;
    } else {
        report.result = LP_TOUCH_KEY_TESTBOX_ERR_NONE;
    }
    lp_touch_key_testbox_test_cmd_send(&report);

    return 0;
}

static void lp_touch_key_testbox_test_res_statis(u16 value)
{
    if (testbox_test_handle.statis.probe_max == 0) {
        testbox_test_handle.statis.probe_max = value;
        testbox_test_handle.statis.probe_min = value;
    } else {
        testbox_test_handle.statis.probe_min = MIN(value, testbox_test_handle.statis.probe_min);
        testbox_test_handle.statis.probe_max = MAX(value, testbox_test_handle.statis.probe_max);
    }

    if (testbox_test_handle.statis.cur_status == 'L') {
        if (testbox_test_handle.statis.fall_cnt == 1) {
            if (testbox_test_handle.statis.res_min0 == 0) {
                testbox_test_handle.statis.res_min0 = value;
            } else {
                testbox_test_handle.statis.res_min0 = MIN(value, testbox_test_handle.statis.res_min0);
            }
        } else if (testbox_test_handle.statis.fall_cnt == 2) {
            if (testbox_test_handle.statis.res_min1 == 0) {
                testbox_test_handle.statis.res_min1 = value;
            } else {
                testbox_test_handle.statis.res_min1 = MIN(value, testbox_test_handle.statis.res_min1);
            }
        }
    } else if (testbox_test_handle.statis.cur_status == 'H') {
        if (testbox_test_handle.statis.raise_cnt == 1) {
            if (testbox_test_handle.statis.res_max0 == 0) {
                testbox_test_handle.statis.res_max0 = value;
            } else {
                testbox_test_handle.statis.res_max0 = MAX(value, testbox_test_handle.statis.res_max0);
            }
        } else if (testbox_test_handle.statis.fall_cnt == 2) {
            testbox_test_handle.statis.end_cnt++;
            if (testbox_test_handle.statis.end_cnt == LP_TOUCH_TEST_END_DELAY_COUNTER) {
                testbox_test_handle.statis.double_key_hit = 1;
                lp_touch_key_testbox_test_report();
            }

            if (testbox_test_handle.statis.end_cnt > LP_TOUCH_TEST_END_DELAY_COUNTER) {
                return;
            }

            if (testbox_test_handle.statis.res_max1 == 0) {
                testbox_test_handle.statis.res_max1 = value;
            } else {
                testbox_test_handle.statis.res_max1 = MAX(value, testbox_test_handle.statis.res_max1);
            }
        }
    }
}

static int lp_touch_key_testbox_test_res_handle(u8 ctmu_event)
{
    u16 res = 0;

    putchar('+');

    if (testbox_test_handle.cur_test_status == LP_TOUCH_KEY_TESTBOX_CMD_TESTMODE_NONE) {
        return 0;
    }
    if (testbox_test_handle.cur_test_status != LP_TOUCH_KEY_TESTBOX_CMD_TEST_KEY) {
        return 1;
    }
    switch (ctmu_event) {
    case CTMU_P2M_CH0_FALLING_EVENT:
        testbox_test_handle.statis.fall_cnt++;
        testbox_test_handle.statis.cur_status = 'L';
        log_info("Key Down");
        break;
    case CTMU_P2M_CH0_RAISING_EVENT:
        testbox_test_handle.statis.raise_cnt++;
        testbox_test_handle.statis.cur_status = 'H';
        log_info("Key Up");
        break;
    case CTMU_P2M_CH0_DEBUG_EVENT:
        res = ((P2M_CTMU_CH0_H_RES << 8) | (P2M_CTMU_CH0_L_RES));
        lp_touch_key_testbox_test_res_statis(res);
        break;
    default:
        break;
    }

    return 1;
}

/* ---------------------------------------------------------------------------- */
/**
 * @brief 蓝牙lmp提供该接口, 用于给测试盒发送测试结果
 *
 * @param priv
 *
 * @return
 */
/* ---------------------------------------------------------------------------- */

static void lp_touch_key_testbox_test_report_get(void *priv);

__attribute__((weak))
int lp_touch_key_testbox_test_cmd_send(void *priv)
{
    //lp_touch_key_testbox_test_report_get(priv);

    return 0;
}

/* ---------------------------------------------------------------------------- */
/**
 * @brief 蓝牙lmp层回调函数, 用于接收测试盒
 *
 * @param priv
 *
 * @return
 */
/* ---------------------------------------------------------------------------- */
int lp_touch_key_receive_cmd_from_testbox(void *priv)
{
    LP_TOUCH_TESTBOX_CMD *test_cmd = (LP_TOUCH_TESTBOX_CMD *)priv;
    if (__this->init == 0) {
        return 0;
    }

    switch (test_cmd->cmd) {
    case LP_TOUCH_KEY_TESTBOX_CMD_TESTMODE_ENTER:
        if (testbox_test_handle.cur_test_status == LP_TOUCH_KEY_TESTBOX_CMD_TESTMODE_NONE) {
            lp_touch_key_testbox_reinit(1);
            testbox_test_handle.cur_test_status = LP_TOUCH_KEY_TESTBOX_CMD_TESTMODE_ENTER;
        }
        break;
    case LP_TOUCH_KEY_TESTBOX_CMD_TEST_KEY:
        if ((testbox_test_handle.cur_test_status == LP_TOUCH_KEY_TESTBOX_CMD_TESTMODE_ENTER) ||
            (testbox_test_handle.cur_test_status == LP_TOUCH_KEY_TESTBOX_CMD_TEST_KEY)) {
            local_irq_disable();
            memset((u8 *)(&(testbox_test_handle.statis)), 0, sizeof(testbox_test_handle.statis));
            testbox_test_handle.cur_test_status = LP_TOUCH_KEY_TESTBOX_CMD_TEST_KEY;
            if (testbox_test_handle.timeout_timer) {
                sys_timer_modify(testbox_test_handle.timeout_timer, LP_TOUCH_TEST_TIMEOUT_CONFIG);
            } else {
                testbox_test_handle.timeout_timer = sys_timeout_add(NULL, lp_touch_key_testbox_test_timeout_handle, LP_TOUCH_TEST_TIMEOUT_CONFIG);
            }
            local_irq_enable();
        }
        break;
#if 0
    case LP_TOUCH_KEY_TESTBOX_CMD_TEST_KEY_RETRY:
        if (testbox_test_handle.cur_test_status == LP_TOUCH_KEY_TESTBOX_CMD_TEST_KEY) {
            local_irq_disable();
            memset((u8 *)(&(testbox_test_handle.statis)), 0, sizeof(testbox_test_handle.statis));
            if (testbox_test_handle.timeout_timer) {
                sys_timer_modify(testbox_test_handle.timeout_timer, LP_TOUCH_TEST_TIMEOUT_CONFIG);
            } else {
                testbox_test_handle.timeout_timer = sys_timeout_add(NULL, lp_touch_key_testbox_test_timeout_handle, LP_TOUCH_TEST_TIMEOUT_CONFIG);
            }
            local_irq_enable();
        }
        break;
#endif /* #if 0 */
    case LP_TOUCH_KEY_TESTBOX_CMD_TESTMODE_EXIT:
        if ((testbox_test_handle.cur_test_status == LP_TOUCH_KEY_TESTBOX_CMD_TESTMODE_ENTER) ||
            (testbox_test_handle.cur_test_status == LP_TOUCH_KEY_TESTBOX_CMD_TEST_KEY)) {
            lp_touch_key_testbox_reinit(0);
            testbox_test_handle.cur_test_status = LP_TOUCH_KEY_TESTBOX_CMD_TESTMODE_NONE;
            if (testbox_test_handle.timeout_timer) {
                sys_timeout_del(testbox_test_handle.timeout_timer);
                testbox_test_handle.timeout_timer = 0;
            }
        }
        break;
    case LP_TOUCH_KEY_TESTBOX_CMD_TEST_TIMEOUT_REPORT:
        if (testbox_test_handle.cur_test_status == LP_TOUCH_KEY_TESTBOX_CMD_TEST_KEY) {
            testbox_test_handle.cur_test_status = LP_TOUCH_KEY_TESTBOX_CMD_TEST_TIMEOUT_REPORT;
            lp_touch_key_testbox_test_report();
            testbox_test_handle.cur_test_status = LP_TOUCH_KEY_TESTBOX_CMD_TEST_KEY;
        }
        break;
    default:
        break;
    }

    return 0;
}

/* ---------------------------------------------------------------------------- */
/**
 * @brief 模拟测试盒流程
 */
/* ---------------------------------------------------------------------------- */
#if 0// for test
#define LP_TEST_TASK_NAME 			"lp_test"
static u16 timeout_id = 0;
static u8 retry_cnt = 0;

enum TESTBOX_LOCAL_CMD {
    TESTBOX_LOCAL_CMD_REPORT,
    TESTBOX_LOCAL_CMD_TEST_END,
};

static void lp_touch_key_testbox_test_report_get(void *priv)
{
    LP_TOUCH_TESTBOX_CMD test_cmd;
    if (priv == NULL) {
        return;
    }

    struct lp_touch_key_test_report *info = (struct lp_touch_key_test_report *)priv;

    os_taskq_post_msg(LP_TEST_TASK_NAME, 8,
                      TESTBOX_LOCAL_CMD_REPORT,
                      info->result,
                      info->res_max,
                      info->res_min,
                      info->res_delta,
                      info->res_percent,
                      info->fall_cnt,
                      info->raise_cnt);
}

static void lp_touch_key_testbox_timeout(void *priv)
{
    LP_TOUCH_TESTBOX_CMD test_cmd;

    log_info("===== Touch Key Test Timeout =====");
    //查询测试结果:
    test_cmd.cmd = LP_TOUCH_KEY_TESTBOX_CMD_TEST_TIMEOUT_REPORT;
    lp_touch_key_receive_cmd_from_testbox(&test_cmd);
}

static void lp_touch_key_testbox_testmode(void *priv)
{
    LP_TOUCH_TESTBOX_CMD test_cmd;
    test_cmd.cmd = LP_TOUCH_KEY_TESTBOX_CMD_TEST_KEY;
    lp_touch_key_receive_cmd_from_testbox(&test_cmd);
    log_info("===== Please Double Click =====");
    timeout_id = sys_timeout_add(NULL, lp_touch_key_testbox_timeout, 4000);
}

static void lp_touch_key_testbox_enter(void)
{
    LP_TOUCH_TESTBOX_CMD test_cmd;
    test_cmd.cmd = LP_TOUCH_KEY_TESTBOX_CMD_TESTMODE_ENTER;
    log_info("===== Touch Key Test Enter =====");
    lp_touch_key_receive_cmd_from_testbox(&test_cmd);
    sys_timeout_add(NULL, lp_touch_key_testbox_testmode, 200);
}

static void testbox_report(void *priv)
{
    u32 *info = (u32 *)priv;
    LP_TOUCH_TESTBOX_CMD test_cmd;

    if (timeout_id) {
        sys_timeout_del(timeout_id);
        timeout_id = 0;
    }

    struct lp_touch_key_test_report report;
    report.result 		= info[0];
    report.res_max 		= info[1];
    report.res_min 		= info[2];
    report.res_delta 	= info[3];
    report.res_percent 	= info[4];
    report.fall_cnt 	= info[5];
    report.raise_cnt 	= info[6];

    log_info("===== Touch Key Test Report =====");
    log_info("info->result  = %d", report.result);
    log_info("info->res_max = %d", report.res_max);
    log_info("info->res_min = %d", report.res_min);
    log_info("info->res_delta   = %d", report.res_delta);
    log_info("info->res_percent = %d", report.res_percent);
    log_info("info->fall_cnt  = %d", report.fall_cnt);
    log_info("info->raise_cnt = %d", report.raise_cnt);

    if (report.result == LP_TOUCH_KEY_TESTBOX_ERR_NONE) {
        log_info("LP Key Test OK");
    } else if (report.result == LP_TOUCH_KEY_TESTBOX_ERR_DOUBLE_KEY_NOT_HIT) {
        log_info("Double Key Not Hit");
    } else if (report.result == LP_TOUCH_KEY_TESTBOX_ERR_DETLA) {
        log_info("Touch Delta Less");
    }

    if ((report.result != LP_TOUCH_KEY_TESTBOX_ERR_NONE) && (retry_cnt < 2)) {
        log_info("test err, retry_cnt = %d, try again", retry_cnt);
        test_cmd.cmd = LP_TOUCH_KEY_TESTBOX_CMD_TEST_KEY;
        lp_touch_key_receive_cmd_from_testbox(&test_cmd);
        log_info("===== Please Double Click =====");
        timeout_id = sys_timeout_add(NULL, lp_touch_key_testbox_timeout, 4000);
        retry_cnt++;
    } else {
        log_info("===== Touch Key Test End =====");
        retry_cnt = 0;
        test_cmd.cmd = LP_TOUCH_KEY_TESTBOX_CMD_TESTMODE_EXIT;
        lp_touch_key_receive_cmd_from_testbox(&test_cmd);
        os_taskq_post_msg(LP_TEST_TASK_NAME, 1, TESTBOX_LOCAL_CMD_TEST_END);
    }
}

static void touch_key_test(void *priv)
{
    int res = 0;
    int msg[30];

    log_info("==== Touch Key Test Task ====");

    os_time_dly(1000);

    lp_touch_key_testbox_enter();

    while (1) {
        res = os_taskq_pend(NULL, msg, ARRAY_SIZE(msg));
        if (res == OS_TASKQ) {
            switch (msg[1]) {
            case TESTBOX_LOCAL_CMD_REPORT:
                testbox_report(&(msg[2]));
                break;
            case TESTBOX_LOCAL_CMD_TEST_END:
                os_time_dly(500);
                lp_touch_key_testbox_enter();
                break;
            default:
                break;
            }
        }
    }
}

void lp_touch_key_testbox_test(void)
{
    os_task_create(touch_key_test, NULL, 1, 256, 256, LP_TEST_TASK_NAME);
}
#endif /* #if 0// for test */

#endif /* #if (TCFG_LP_TOUCH_KEY_ENABLE || TCFG_LP_EARTCH_KEY_ENABLE) */
