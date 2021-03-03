#include "includes.h"
#include "asm/power/p11.h"
#include "asm/power/p33.h"
#include "asm/lp_touch_key.h"
#include "device/key_driver.h"
#include "lp_touch_key_epd.h"
/* #include "bt_tws.h" */
#include "classic/tws_api.h"
#include "key_event_deal.h"
#include "app_config.h"

#define LOG_TAG_CONST       LP_KEY
#define LOG_TAG             "[LP_KEY]"
/* #define LOG_ERROR_ENABLE */
/* #define LOG_DEBUG_ENABLE */
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"


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
    BT_RES_MSG,
    BT_EVENT_HW_MSG,
    BT_EVENT_SW_MSG,
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

int eartch_event_deal_init(void);

#define __this 		(&_ctmu_key)

#if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
static int lp_touch_key_online_debug_init(void);
static int lp_touch_key_online_debug_send(u8 ch, u16 val);
static int lp_touch_key_online_debug_key_event_handle(u8 ch_index, struct sys_event *event);
#endif /* #if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE */

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
#endif /* #if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE */
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

static void tws_send_res_data(int res, int type)
{
    u32 event_data[4];
    if (tws_api_get_tws_state() & TWS_STA_SIBLING_CONNECTED) {
        event_data[0] = type;
        event_data[1] = res;
        tws_api_send_data_test(event_data, 2 * sizeof(int), TWS_FUNC_ID_VOL_LP_KEY);
    }
}

void p33_ctmu_key_event_irq_handler()
{
    static u32 cnt0 = 0;
    static u32 cnt1 = 0;
    u8 ctmu_event = P2M_CTMU_KEY_EVENT;
    int res = 0;
    u8 ret = 0;
    u32 data[4];
    u32 event_data[4];
    //log_debug("ctmu msg: 0x%x", ctmu_event);
    switch (ctmu_event) {
    case CTMU_P2M_CH0_DEBUG_EVENT:
        //log_debug("CH0: debug: %d, RES = 0x%x", cnt0++, (P2M_CTMU_CH0_H_RES << 8) | P2M_CTMU_CH0_L_RES);
        res = (P2M_CTMU_CH0_H_RES << 8) | P2M_CTMU_CH0_L_RES;
#if TWS_BT_SEND_CH0_RES_DATA_ENABLE
        log_debug("CH1: debug %d, RES = 0x%x", cnt1++, res);
        tws_send_res_data(res, BT_RES_MSG);
#endif /* #if TWS_BT_SEND_CH0_RES_DATA_ENABLE */
#if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
        lp_touch_key_online_debug_send(0, res);
#endif /* #if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE */
        break;
    case CTMU_P2M_CH1_DEBUG_EVENT:
        res = (P2M_CTMU_CH1_H_RES << 8) | P2M_CTMU_CH1_L_RES;
        //log_debug("CH1: debug %d, RES = 0x%x", cnt1++, res);
#if CFG_CH1_USE_ALGORITHM_ENABLE
        //log_debug("CH1: debug %d, RES = 0x%x", cnt1++, res);
        ret = lp_touch_key_epd_update_res(res);

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

#if TWS_BT_SEND_CH1_RES_DATA_ENABLE
        tws_send_res_data(res, BT_RES_MSG);
#endif /* #if TWS_BT_SEND_CH1_RES_DATA_ENABLE */

#if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
        lp_touch_key_online_debug_send(1, res);
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
        ctmu_ch1_event_handle(CH1_EAR_IN);
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
        ctmu_ch1_event_handle(CH1_EAR_OUT);
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
    u32 *data = _data;
    if (rx) {
        if (data[0] == BT_RES_MSG) {
            log_debug("len = %d, RES = %d", len, data[1]);
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

