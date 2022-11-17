#include "includes.h"
#include "asm/power/p11.h"
#include "asm/power/p33.h"
#include "asm/lp_touch_key_api.h"
#include "asm/lp_touch_key_alog.h"
#include "asm/lp_touch_key_hw.h"
#include "device/key_driver.h"
#include "bt_tws.h"
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
#define CTMU_CH0_MODULE_DEBUG 	1
#define CTMU_CH1_MODULE_DEBUG 	0

#define TWS_FUNC_ID_VOL_LP_KEY      TWS_FUNC_ID('L', 'K', 'E', 'Y')
#define TWS_BT_SEND_CH0_RES_DATA_ENABLE 	0
#define TWS_BT_SEND_CH1_RES_DATA_ENABLE 	0
#define TWS_BT_SEND_EVENT_ENABLE 			0

#define LP_TOUCH_KEY_TIMER_MAGIC_NUM 		0xFFFF

#if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
#ifdef CTMU_RESET_TIME_CONFIG
#undef CTMU_RESET_TIME_CONFIG
#define CTMU_RESET_TIME_CONFIG 		0
#endif /* #ifdef CTMU_RESET_TIME_CONFIG */
#endif /* #if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE */


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
    .short_timer = LP_TOUCH_KEY_TIMER_MAGIC_NUM,
    .long_timer = LP_TOUCH_KEY_TIMER_MAGIC_NUM,
    .ear_in_timer = LP_TOUCH_KEY_TIMER_MAGIC_NUM,
    .last_key = CTMU_KEY_NULL,
};

struct ch_ana_cfg {
    u8 isel;
    u8 vhsel;
    u8 vlsel;
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
const static struct ch_adjust_table ch0_sensitivity_table[] = {
    /*  cfg0 		cfg1 		cfg2 */
    {20, 		30, 		550}, //cap检测灵敏度级数0
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

#define LPCTMU_VH_LEVEL     3 // 上限电压档位
#define LPCTMU_VL_LEVEL     0 // 下限电压档位
#define LPCTMU_CUR_LEVEL    7 // 充放电电流档位

static const u8 lpctmu_ana_vh_table[4] = {
    LPCTMU_VH_065V,
    LPCTMU_VH_070V,
    LPCTMU_VH_075V,
    LPCTMU_VH_080V,
};
static const u8 lpctmu_ana_vl_table[4] = {
    LPCTMU_VL_020V,
    LPCTMU_VL_025V,
    LPCTMU_VL_030V,
    LPCTMU_VL_035V,
};
static const u8 lpctmu_ana_cur_table[8] = {
    LPCTMU_ISEL_008UA,
    LPCTMU_ISEL_024UA,
    LPCTMU_ISEL_040UA,
    LPCTMU_ISEL_056UA,
    LPCTMU_ISEL_072UA,
    LPCTMU_ISEL_088UA,
    LPCTMU_ISEL_104UA,
    LPCTMU_ISEL_120UA
};

u8 get_lpctmu_ana_level(void)
{
    return ((lpctmu_ana_vh_table[LPCTMU_VH_LEVEL]) | \
            (lpctmu_ana_vl_table[LPCTMU_VL_LEVEL]) | \
            (lpctmu_ana_cur_table[LPCTMU_CUR_LEVEL]));
}

struct lp_touch_key_alog_cfg {
    u16 ready_flag;
    u16 range;
    s32 sigma;
};
static struct lp_touch_key_alog_cfg alog_cfg = {
    .ready_flag = 0,
    .range = 0,
    .sigma = 0,
};
static u16 res_stable_cnt = 0;
static u8 ch0_range_sensity = 7;
static u16 ctmu_ch0_res_scan_time_add = 0;
static u16 save_alog_cfg_time_out = 0;
#define TOUCH_RANGE_MIN     50
#define TOUCH_RANGE_MAX     500


#define CH1_SOFT_INEAR_VAL  180
#define CH1_SOFT_OUTEAR_VAL 120

int eartch_event_deal_init(void);

#define __this 		(&_ctmu_key)

#if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
static int lp_touch_key_online_debug_init(void);
static int lp_touch_key_online_debug_send(u8 ch, u16 val);
static int lp_touch_key_online_debug_key_event_handle(u8 ch_index, struct sys_event *event);
#endif /* #if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE */

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

static u32 lp_touch_key_get_lrc_hz(void)
{
    ASSERT(__this->lrc_hz, "lrc_hz is ZERO");
    return __this->lrc_hz;
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

//=========================================//
//模块配置顺序: Select ctmu clk --> Enable --> Reset --> 配置参数 --> Run
static void __lpctmu_reset(u8 para_reset)
{
    LP_CTMU_CLK_SEL(LP_CTMU_CLK_SEL_P11_SYS_CLK); //sel sys_clk 12M, 加快配置速度
    LP_CTMU_MODULE_EN(1); 			//lctm en

    LP_CTMU_RUN(0); //lctm run (内部状态机复位)

    if (para_reset) {
        LP_CTMU_CFG_RESET(0); //lctm 配置参数reset
    }
    delay(100);
    LP_CTMU_CFG_RESET(1);           //lctm reset
}

//=========================================//

#if !TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE


void lp_touch_key_save_alog_cfg(void *priv)
{
    int ret = syscfg_write(VM_LP_TOUCH_KEY_ALOG_CFG, (void *)&alog_cfg, sizeof(struct lp_touch_key_alog_cfg));
    if (ret != sizeof(struct lp_touch_key_alog_cfg)) {
        log_info("write vm alog cfg ready flag error !\n");
    }
    save_alog_cfg_time_out = 0;
}

void lp_touch_key_ctmu_ch0_cfg2_check_update(u16 cfg2_new)
{
    u16 cfg2_old = (M2P_CTMU_CH0_CFG2H << 8) | M2P_CTMU_CH0_CFG2L;
    if (cfg2_old != cfg2_new) {
        printf("ctmu ch0 cfg2_old = %d  cfg_new = %d\n", cfg2_old, cfg2_new);
        printf("change ctmu ch0 cfg2 !\n");
        M2P_CTMU_CH0_CFG2H = (cfg2_new >> 8) & 0xff;
        M2P_CTMU_CH0_CFG2L = (cfg2_new >> 0) & 0xff;
    }
}

static void lp_touch_key_ctmu_ch0_res_scan(void *priv)
{
    if (M2P_CTMU_CH0_RES_SEND) {
        return;
    }
    u16 ctmu_ch0_res0;
    u16 ctmu_ch0_res1;
__read_res:
    ctmu_ch0_res0 = (P2M_CTMU_CH0_H_RES << 8) | P2M_CTMU_CH0_L_RES;
    delay(100);
    ctmu_ch0_res1 = (P2M_CTMU_CH0_H_RES << 8) | P2M_CTMU_CH0_L_RES;
    if (ctmu_ch0_res0 != ctmu_ch0_res1) {
        goto __read_res;
    }

#if 1

    if ((ctmu_ch0_res0 < 2000) || (ctmu_ch0_res0 > 20000)) {
        return;
    }

    if (res_stable_cnt < 50) {
        res_stable_cnt ++;
        return;
    }

    TouchAlgo_Update(0, ctmu_ch0_res0);

    u8 range_valid = 0;
    u16 touch_range = TouchAlgo_GetRange(0, (u8 *)&range_valid);
    s32 touch_sigma = TouchAlgo_GetSigma(0);
    /* printf("res:%d range:%d val:%d sigma:%d\n", ctmu_ch0_res0, touch_range, range_valid, touch_sigma); */
    if ((range_valid) && (touch_range > TOUCH_RANGE_MIN) && (touch_range < TOUCH_RANGE_MAX)) {
        u16 cfg2_new = touch_range * (10 - ch0_range_sensity) / 10;
        lp_touch_key_ctmu_ch0_cfg2_check_update(cfg2_new);
        if (touch_range != alog_cfg.range) {
            alog_cfg.range = touch_range;
            alog_cfg.sigma = touch_sigma;
            if (save_alog_cfg_time_out == 0) {
                save_alog_cfg_time_out = sys_timeout_add(NULL, lp_touch_key_save_alog_cfg, 1);
            }
        }
    } else if ((range_valid) && (touch_range > TOUCH_RANGE_MAX)) {
        TouchAlgo_SetRange(0, alog_cfg.range);
    }
#endif
}

void lp_touch_key_alog_ready_flag_check_and_set(void)
{
    alog_cfg.ready_flag = 0;
    syscfg_read(VM_LP_TOUCH_KEY_ALOG_CFG, (void *)&alog_cfg, sizeof(struct lp_touch_key_alog_cfg));
    if (alog_cfg.ready_flag == 0) {
        alog_cfg.ready_flag = 1;
        lp_touch_key_save_alog_cfg(NULL);
    }
}

void lp_touch_key_alog_init(u8 cfg2_sensity)
{
    TouchAlgo_Init(0, TOUCH_RANGE_MIN, TOUCH_RANGE_MAX);
    res_stable_cnt = 0;
    ch0_range_sensity = cfg2_sensity;
    int ret = syscfg_read(VM_LP_TOUCH_KEY_ALOG_CFG, (void *)&alog_cfg, sizeof(struct lp_touch_key_alog_cfg));
    if ((ret == (sizeof(struct lp_touch_key_alog_cfg))) && (alog_cfg.range > TOUCH_RANGE_MIN) && (alog_cfg.range < TOUCH_RANGE_MAX) && (alog_cfg.ready_flag)) {
        log_info("vm read alog ready_flag:%d sigma:%d range:%d\n", alog_cfg.ready_flag, alog_cfg.sigma, alog_cfg.range);
        TouchAlgo_SetSigma(0, alog_cfg.sigma);
        TouchAlgo_SetRange(0, alog_cfg.range);
        u16 new_cfg2 = alog_cfg.range * (10 - ch0_range_sensity) / 10;
        LP_CTMU_CH0_EDGE_TH(new_cfg2);
    }

#if 1   //触摸算法要尽可能的在装完整机之后开始跑，而这里是耳机生产流程中获取它第一次在舱内开机的时候，在vm标记一个变量，从此开始跑算法。

    if (get_charge_online_flag()) {
        log_info("check charge online !\n");
        lp_touch_key_alog_ready_flag_check_and_set();//如果有其他好的位置，请将该函数移到那个位置执行。
    }

#endif

    if ((alog_cfg.ready_flag) && (ctmu_ch0_res_scan_time_add == 0)) {
        ctmu_ch0_res_scan_time_add = usr_timer_add(NULL, lp_touch_key_ctmu_ch0_res_scan, 20, 0);
    }
}


#endif


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

//==============================================================//
//                      CH0 初始化                              //
//==============================================================//
    __lpctmu_reset(1); //模块复位

    LP_CTMU_SWITCH_STABLE_TIME_SET();

    //CTMU 时基配置:
    u16 time_prd = 0;
    if (__this->lrc_hz) {
        time_prd = (__this->lrc_hz * CTMU_TIME_BASE) / 1000 - 1;
    } else {
        time_prd = CFG_M2P_CTMU_BASE_TIME_PRD;
    }

    LP_CTMU_TIMER_BASE_CONFIG(time_prd);
    log_info("LRC_HZ = %d, time_prd = %d", __this->lrc_hz, time_prd);

    if (__this->config->ch0.enable) {
        //PB1和PB2通道互换IO:
        if (__this->config->ch0.port == IO_PORTB_02) {
            LP_CTMU_IO_INV(1);
        } else {
            LP_CTMU_IO_INV(0);
            ASSERT(__this->config->ch0.port == IO_PORTB_01);
        }

        M2P_CTMU_MSG |= CTMU_INIT_CH0_ENABLE;

        ctmu_port_init(__this->config->ch0.port);

        //模拟参数配置:
        LP_CTMU_CH0_ANA_CFG(get_lpctmu_ana_level());

        //通用配置:
        LP_CTMU_CH0_ENABLE(1); 			//通道0使能
        LP_CTMU_CH0_FILTER_LEVEL(1); 	//通道0滤波级数
        LP_CTMU_CH0_FILTER_EN(1); 		//通道0滤波使能
        LP_CTMU_CH0_EDGE_EN(1);  		//通道0边沿检测使能
        LP_CTMU_CH0_EDGE_TEMP_EN(1); 	//fixed
        LP_CTMU_CH0_FIRST_RISE_MASK(0); //屏蔽第一个上升沿

        //上升沿中断:
        LP_CTMU_CH0_FALL_PENDING_CLR();
        LP_CTMU_CH0_FALL_PENDING_IE(1);
        LP_CTMU_CH0_FALL_PENDING_SEL(0);

        //下升沿中断:
        LP_CTMU_CH0_RISE_PENDING_CLR();
        LP_CTMU_CH0_RISE_PENDING_IE(1);
        LP_CTMU_CH0_RISE_PENDING_SEL(0);

        //多击模式:
        LP_CTMU_CH0_SHORT_KEY_MODE(1);

#if CTMU_CH0_MODULE_DEBUG
        LP_CTMU_CH0_RES_PENDING_IE(1);
        M2P_CTMU_MSG |= CTMU_INIT_CH0_DEBUG;
#endif /* #if CTMU_CH0_MODULE_DEBUG */

        LP_CTMU_CH0_SHORT_KEY_TRIGGER(0);
        LP_CTMU_CH0_SHORT_PENDING_CLR();
        LP_CTMU_CH0_SHORT_PENDING_IE(0);
        LP_CTMU_CH0_LONG_PENDING_CLR();
        LP_CTMU_CH0_LONG_PENDING_IE(1);

        LP_CTMU_CH0_HOLD_PENDING_CLR();
        LP_CTMU_CH0_HOLD_PENDING_IE(1);
        LP_CTMU_CH0_HOLD_EN(1);
        LP_CTMU_CH0_KEY_MODE_ENABLE(1);

        //快速 & 低速扫描周期
        LP_CTMU_CH0_HIGH_SPEED_PRD(CFG_CTMU_CH0_HS_PERIOD_VALUE);
        LP_CTMU_CH0_LOW_SPEED_PRD(CFG_CTMU_CH0_LS_PERIOD_VALUE);

        //采样窗口时间
        LP_CTMU_CH0_DET_TIME(CFG_CTMU_CH0_WINDOW_TIME_VALUE);

        ASSERT(ch0_sensity < ARRAY_SIZE(ch0_sensitivity_table));

        //CH0阈值:
        LP_CTMU_CH0_TEMP_TH(ch0_sensitivity_table[ch0_sensity].cfg0);
        LP_CTMU_CH0_STABLE_TH(ch0_sensitivity_table[ch0_sensity].cfg0 + 5);
        LP_CTMU_CH0_EDGE_TH(ch0_sensitivity_table[ch0_sensity].cfg2);

        M2P_CTMU_CH0_CFG2H = (ch0_sensitivity_table[ch0_sensity].cfg2 >> 8) & 0xff;
        M2P_CTMU_CH0_CFG2L = (ch0_sensitivity_table[ch0_sensity].cfg2 >> 0) & 0xff;

        //CH0 按键时间:
        LP_CTMU_CH0_SHORT_KEY_TIME(CFG_M2P_CTMU_CH0_SHORT_TIME_VALUE);
        LP_CTMU_CH0_LONG_KEY_TIME(CFG_M2P_CTMU_CH0_LONG_TIME_VALUE);
        LP_CTMU_CH0_HOLD_KEY_TIME(CFG_M2P_CTMU_CH0_HOLD_TIME_VALUE);

        LP_CTMU_CH0_IE(1);

        //长按开机时间配置
        M2P_CTMU_CH0_SOFTOFF_LONG_TIMEL = (CFG_M2P_CTMU_CH0_SOFTOFF_LONG_TIME & 0xFF);
        M2P_CTMU_CH0_SOFTOFF_LONG_TIMEH = ((CFG_M2P_CTMU_CH0_SOFTOFF_LONG_TIME >> 8) & 0xFF);

        log_info("P11_LCTM_CON = 0x%x", P11_LCTM_CON);
        log_info("LCTM_MOD = 0x%x", LCTM_MOD);
        log_info("LCTM_TMR = 0x%x", LCTM_TMR);
        log_info("LCTM_TIME_BASE_H = 0x%x", LCTM_TIME_BASE_H);
        log_info("LCTM_TIME_BASE_L = 0x%x", LCTM_TIME_BASE_L);
        log_info("LCTM_CHL0_ANA = 0x%x", LCTM_CHL0_ANA);
        log_info("LCTM_CHL0_CON0 = 0x%x", LCTM_CHL0_CON0);
        log_info("LCTM_CHL0_CON1 = 0x%x", LCTM_CHL0_CON1);
        log_info("LCTM_CHL0_CON2 = 0x%x", LCTM_CHL0_CON2);
        log_info("LCTM_CHL0_CON3 = 0x%x", LCTM_CHL0_CON3);
        log_info("LCTM_CHL0_HS_PRD = 0x%x", LCTM_CHL0_HS_PRD);
        log_info("LCTM_CHL0_LS_PRD = 0x%x", LCTM_CHL0_LS_PRD);
        log_info("LCTM_CHL0_DET_TIME_H = 0x%x", LCTM_CHL0_DET_TIME_H);
        log_info("LCTM_CHL0_DET_TIME_L = 0x%x", LCTM_CHL0_DET_TIME_L);
        log_info("LCTM_CHL0_TEMP_H = 0x%x", LCTM_CHL0_TEMP_H);
        log_info("LCTM_CHL0_TEMP_L = 0x%x", LCTM_CHL0_TEMP_L);
        log_info("LCTM_CHL0_STA_H = 0x%x", LCTM_CHL0_STA_H);
        log_info("LCTM_CHL0_STA_L = 0x%x", LCTM_CHL0_STA_L);
        log_info("LCTM_CHL0_EDGE_H = 0x%x", LCTM_CHL0_EDGE_H);
        log_info("LCTM_CHL0_EDGE_L = 0x%x", LCTM_CHL0_EDGE_L);
        log_info("LCTM_CHL0_SHORT_H = 0x%x", LCTM_CHL0_SHORT_H);
        log_info("LCTM_CHL0_SHORT_L = 0x%x", LCTM_CHL0_SHORT_L);
        log_info("LCTM_CHL0_LONG_H = 0x%x", LCTM_CHL0_LONG_H);
        log_info("LCTM_CHL0_LONG_L = 0x%x", LCTM_CHL0_LONG_L);
        log_info("LCTM_CHL0_HOLD_H = 0x%x", LCTM_CHL0_HOLD_H);
        log_info("LCTM_CHL0_HOLD_L = 0x%x", LCTM_CHL0_HOLD_L);
#if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
        M2P_CTMU_CH0_RES_SEND = 1;
#else
        M2P_CTMU_CH0_RES_SEND = 0;
#endif


#if CTMU_CH0_MODULE_DEBUG
#if !TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
        if (!M2P_CTMU_CH0_RES_SEND) {
            lp_touch_key_alog_init(ch0_sensity);
        }
#endif
#endif


    }

//==============================================================//
//                      CH1 初始化                              //
//==============================================================//
    if (__this->config->ch1.enable) {
        if (__this->config->ch1.port == IO_PORTB_01) {
            LP_CTMU_IO_INV(1);
        } else {
            LP_CTMU_IO_INV(0);
            ASSERT(__this->config->ch1.port == IO_PORTB_02);
        }
        M2P_CTMU_MSG |= CTMU_INIT_CH1_ENABLE;
        ctmu_port_init(__this->config->ch1.port);

        //模拟参数配置:
        LP_CTMU_CH1_ANA_CFG(get_lpctmu_ana_level());

        //通用配置:
        LP_CTMU_CH1_ENABLE(1); 			//通道0使能
        LP_CTMU_CH1_FILTER_LEVEL(1); 	//通道0滤波级数
        LP_CTMU_CH1_FILTER_EN(1); 		//通道0滤波使能
        LP_CTMU_CH1_EDGE_EN(1);  		//通道0边沿检测使能
        LP_CTMU_CH1_EDGE_TEMP_EN(1); 	//fixed
        LP_CTMU_CH1_FIRST_RISE_MASK(0); //屏蔽第一个上升沿

        //上升沿中断:
        LP_CTMU_CH1_FALL_PENDING_CLR();
        LP_CTMU_CH1_FALL_PENDING_IE(1);
        LP_CTMU_CH1_FALL_PENDING_SEL(0);

        //下升沿中断:
        LP_CTMU_CH1_RISE_PENDING_CLR();
        LP_CTMU_CH1_RISE_PENDING_IE(1);
        LP_CTMU_CH1_RISE_PENDING_SEL(0);

#if CTMU_CH1_MODULE_DEBUG
        LP_CTMU_CH1_RES_PENDING_IE(1);
        M2P_CTMU_MSG |= CTMU_INIT_CH1_DEBUG;
#endif /* #if CTMU_CH1_MODULE_DEBUG */

        //快速 & 低速扫描周期
        LP_CTMU_CH1_HIGH_SPEED_PRD(CFG_CTMU_CH1_HS_PERIOD_VALUE);
        LP_CTMU_CH1_LOW_SPEED_PRD(CFG_CTMU_CH1_LS_PERIOD_VALUE);

        //采样窗口时间
        LP_CTMU_CH1_DET_TIME(CFG_CTMU_CH1_WINDOW_TIME_VALUE);

        ASSERT(ch1_sensity < ARRAY_SIZE(ch1_sensitivity_table));

        //CH1阈值:
        LP_CTMU_CH1_TEMP_TH(ch1_sensitivity_table[ch1_sensity].cfg0);
        LP_CTMU_CH1_STABLE_TH(ch1_sensitivity_table[ch1_sensity].cfg0 + 5);
        LP_CTMU_CH1_EDGE_TH(ch1_sensitivity_table[ch1_sensity].cfg2);

        LP_CTMU_CH1_IE(1);

        log_info("P11_LCTM_CON = 0x%x", P11_LCTM_CON);
        log_info("LCTM_MOD = 0x%x", LCTM_MOD);
        log_info("LCTM_TMR = 0x%x", LCTM_TMR);
        log_info("LCTM_TIME_BASE_H = 0x%x", LCTM_TIME_BASE_H);
        log_info("LCTM_TIME_BASE_L = 0x%x", LCTM_TIME_BASE_L);
        log_info("LCTM_CHL1_ANA = 0x%x", LCTM_CHL1_ANA);
        log_info("LCTM_CHL1_CON0 = 0x%x", LCTM_CHL1_CON0);
        log_info("LCTM_CHL1_CON1 = 0x%x", LCTM_CHL1_CON1);
        log_info("LCTM_CHL1_CON2 = 0x%x", LCTM_CHL1_CON2);
        log_info("LCTM_CHL1_HS_PRD = 0x%x", LCTM_CHL1_HS_PRD);
        log_info("LCTM_CHL1_LS_PRD = 0x%x", LCTM_CHL1_LS_PRD);
        log_info("LCTM_CHL1_DET_TIME_H = 0x%x", LCTM_CHL1_DET_TIME_H);
        log_info("LCTM_CHL1_DET_TIME_L = 0x%x", LCTM_CHL1_DET_TIME_L);
        log_info("LCTM_CHL1_TEMP_H = 0x%x", LCTM_CHL1_TEMP_H);
        log_info("LCTM_CHL1_TEMP_L = 0x%x", LCTM_CHL1_TEMP_L);
        log_info("LCTM_CHL1_STA_H = 0x%x", LCTM_CHL1_STA_H);
        log_info("LCTM_CHL1_STA_L = 0x%x", LCTM_CHL1_STA_L);
        log_info("LCTM_CHL1_EDGE_H = 0x%x", LCTM_CHL1_EDGE_H);
        log_info("LCTM_CHL1_EDGE_L = 0x%x", LCTM_CHL1_EDGE_L);

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
    __this->short_timer = LP_TOUCH_KEY_TIMER_MAGIC_NUM;

    __this->last_key = CTMU_KEY_NULL;
    log_debug("notify key short event, cnt: %d", __this->click_cnt);
    __ctmu_notify_key_event(&e);
}

static void __ctmu_short_click_time_test(void *priv)
{
    return;
}

static void __ctmu_short_click_handle(void)
{
    __this->last_key = CTMU_KEY_SHORT_CLICK;

    u8 click_cnt = LP_CTMU_CH0_SHORT_KEY_CNT_GET();

    __this->click_cnt = click_cnt;

    log_debug("click_cnt = %d", click_cnt);

    __ctmu_short_click_time_out_handle(NULL);
}

static void ctmu_short_click_handle(void)
{
    __this->last_key = CTMU_KEY_SHORT_CLICK;
    if (__this->short_timer == LP_TOUCH_KEY_TIMER_MAGIC_NUM) {
        __this->click_cnt = 1;
        __this->short_timer = usr_timeout_add(NULL, __ctmu_short_click_time_out_handle, CTMU_SHORT_CLICK_DELAY_TIME, 1);
    } else {
        __this->click_cnt++;
        usr_timer_modify(__this->short_timer, CTMU_SHORT_CLICK_DELAY_TIME);
    }
}


static void ctmu_raise_click_handle(void)
{
    struct sys_event e = {0};
    if (__this->last_key >= CTMU_KEY_LONG_CLICK) {
#if 0
        if (__this->long_timer != LP_TOUCH_KEY_TIMER_MAGIC_NUM) {
            usr_timer_del(__this->long_timer);
            __this->long_timer = LP_TOUCH_KEY_TIMER_MAGIC_NUM;
        }
#endif
        e.u.key.event = KEY_EVENT_UP;
        e.u.key.value = __this->config->ch0.key_value;
        __ctmu_notify_key_event(&e);

        __this->last_key = CTMU_KEY_NULL;
        log_debug("notify key HOLD UP event");
    } else {
        ctmu_short_click_handle();
    }
}


static void ctmu_long_click_handle(void)
{
    __this->last_key = CTMU_KEY_LONG_CLICK;

    struct sys_event e;
    e.u.key.event = KEY_EVENT_LONG;
    e.u.key.value = __this->config->ch0.key_value;

    __ctmu_notify_key_event(&e);
}

static void ctmu_hold_click_handle(void)
{
    __this->last_key = CTMU_KEY_HOLD_CLICK;

    struct sys_event e;
    e.u.key.event = KEY_EVENT_HOLD;
    e.u.key.value = __this->config->ch0.key_value;

    __ctmu_notify_key_event(&e);
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

    case CTMU_P2M_CH0_LONG_KEY_EVENT:
    case CTMU_P2M_CH0_SHORT_KEY_EVENT:
    case CTMU_P2M_CH0_HOLD_KEY_EVENT:
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

u8 last_state = CTMU_P2M_CH1_OUT_EVENT;
//extern void spp_printf(const char *format, ...);

void p33_ctmu_key_event_irq_handler()
{
    u8 ctmu_event = P2M_CTMU_KEY_EVENT;
    int res = 0;
    static u32 cnt0 = 0;
    static u32 cnt1 = 0;
    u8 ret = 0;

    u16 ch0_res = 0, ch1_res = 0, ch0_iir = 0, ch1_diff = 0, ch1_trim = 0;

    if (testbox_get_key_action_test_flag(NULL)) {
        ret = lp_touch_key_testbox_remote_test(ctmu_event);
        if (ret == true) {
            return;
        }
    }
    //log_debug("ctmu msg: 0x%x", ctmu_event);
    switch (ctmu_event) {
    case CTMU_P2M_CH0_RES_EVENT:
        ch0_res = LP_CTMU_CH0_RES_GET();
        ch0_iir = ((P2M_CTMU_CH0_H_IIR_VALUE << 8) | P2M_CTMU_CH0_L_IIR_VALUE);
        /*printf("ch0_res: %d, ch0_iir: %d\n", ch0_res, ch0_iir);*/
        //log_debug("CH0: debug, RES = %08d", res);
        //log_debug("CH0: debug %08d, RES = %08d", cnt0++, res);
#if TWS_BT_SEND_CH0_RES_DATA_ENABLE
        tws_send_res_data(((ch0_res << 16) | ch0_iir), 0, 0, BT_CH0_RES_MSG);
#endif /* #if TWS_BT_SEND_CH0_RES_DATA_ENABLE */
#if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
        lp_touch_key_online_debug_send(0, ch0_res);
#endif /* #if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE */
        break;

    case CTMU_P2M_CH1_RES_EVENT:
        ch1_res = LP_CTMU_CH1_RES_GET();
        ch1_diff = ((P2M_CTMU_CH1_H_DIFF_VALUE << 8) | P2M_CTMU_CH1_L_DIFF_VALUE);
        ch1_trim = ((P2M_CTMU_CH1_H_TRIM_VALUE << 8) | P2M_CTMU_CH1_L_TRIM_VALUE);
        ch0_res = ((P2M_CTMU_CH0_H_RES << 8) | P2M_CTMU_CH0_L_RES);
        ch0_iir = (P2M_CTMU_CH0_H_IIR_VALUE << 8 | P2M_CTMU_CH0_L_IIR_VALUE);
        /*printf("ch1_diff: %d, ch1_trim: %d, ch0_res: %d, ch1_res: %d, ch0_iir: %d\n", ch1_diff, ch1_trim, ch0_res, ch1_res, ch0_iir);*/

////////////////////////////////////////////////////////////////////////////////////////////////////////
#if!TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
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


///////////////////////////////////////////////////////////////////////////////////////////////////////////

#if TWS_BT_SEND_CH1_RES_DATA_ENABLE
        tws_send_res_data(((ch1_trim << 16) | ch0_res), ((ch0_iir << 16) | ch1_res), ch1_diff, BT_CH1_RES_MSG);
#endif /* #if TWS_BT_SEND_CH1_RES_DATA_ENABLE */

//////////////////////////////////////////////////////////////////////////////////////////////////////////

#if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE
        lp_touch_key_online_debug_send(1, ch1_res);
#endif /* #if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE */

/////////////////////////////////////////////////////////////////////////////////////////////////////////
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
/////////////////////////////////////////////////////////////////////////////////////////////////////////
        break;

    case CTMU_P2M_CH0_SHORT_KEY_EVENT:
        log_debug("CH0: SHORT click");
        //__ctmu_short_click_handle();
        break;
    case CTMU_P2M_CH0_LONG_KEY_EVENT:
        log_debug("CH0: LONG click");
        ctmu_long_click_handle();
        break;

    case CTMU_P2M_CH0_HOLD_KEY_EVENT:
        log_debug("CH0: HOLD click");
        ctmu_hold_click_handle();
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
        log_debug("CH0: RAISING");
        ctmu_raise_click_handle();
        break;

    case CTMU_P2M_CH1_IN_EVENT:
#if TWS_BT_SEND_EVENT_ENABLE
        tws_send_event_data(CH1_EAR_IN, BT_EVENT_HW_MSG);
#endif /* #if TWS_BT_SEND_EVENT_ENABLE */
        log_debug("CH1 HW: IN");
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
    M2P_LCTM_RESET_PCNT_VALUE = (CTMU_RESET_TIME_CONFIG -  CTMU_LONG_KEY_DELAY_TIME) / CTMU_RESET_TIMER_PRD_VALUE;
#else
    M2P_LCTM_RESET_PCNT_VALUE = 0;
#endif /* #if CTMU_RESET_TIME_CONFIG */
    log_debug("lp timer cnt = %d, lrc_hz = %d, RESET_PCNT = %d", cnt, lrc_hz, M2P_LCTM_RESET_PCNT_VALUE);

#if 0
    if (__this->init) {
        u16 time_prd = 0;
        time_prd = (__this->lrc_hz * CTMU_TIME_BASE) / 1000 - 1;
        M2P_CTMU_BASE_TIME_PRD_L = (time_prd & 0xFF);
        M2P_CTMU_BASE_TIME_PRD_H = ((time_prd >> 8) & 0xFF);

        lp_touch_key_send_cmd(CTMU_M2P_UPDATE_BASE_TIME);
    }
#endif
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

    while (!__this->ch_init & BIT(0)) {
        asm volatile("nop");
    }

    log_debug("%s", __func__);
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
            log_debug("cnt: %08d, trim: %08d, ch0: %08d, ch0_iir: %08d, ch1: %08d, ch0-ch1 %08d", cnt1++, (data[1] >> 16), data[1] & 0xFFFF, (data[2] >> 16), data[2] & 0xFFFF, (data[3] & 0xFFFF));
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
const u8 lp_key_sdk_name[16] = "AC698N";
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
    TOUCH_CH_VOL_CFG_UPDATE  = 0x3001,
    TOUCH_CH_CFG_CONFIRM = 0x3100,
};

enum {
    LP_KEY_ONLINE_ST_INIT,
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
    struct ch_adjust_table ch0_debug_cfg;
    struct ch_adjust_table ch1_debug_cfg;
    lp_touch_online_key_event_t online_key_event;
    struct ch_ana_cfg ch0_ana;
    struct ch_ana_cfg ch1_ana;
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

static int lp_touch_key_debug_reinit_config(void)
{
//==============================================================//
//                      CH0 初始化                              //
//==============================================================//
    __lpctmu_reset(1); //模块复位

    LP_CTMU_SWITCH_STABLE_TIME_SET();

    //CTMU 时基配置:
    u16 time_prd = 0;
    if (__this->lrc_hz) {
        time_prd = (__this->lrc_hz * CTMU_TIME_BASE) / 1000 - 1;
    } else {
        time_prd = CFG_M2P_CTMU_BASE_TIME_PRD;
    }

    LP_CTMU_TIMER_BASE_CONFIG(time_prd);
    log_info("LRC_HZ = %d, time_prd = %d", __this->lrc_hz, time_prd);

    if (__this->config->ch0.enable) {
        //PB1和PB2通道互换IO:
        if (__this->config->ch0.port == IO_PORTB_02) {
            LP_CTMU_IO_INV(1);
        } else {
            LP_CTMU_IO_INV(0);
            ASSERT(__this->config->ch0.port == IO_PORTB_01);
        }

        M2P_CTMU_MSG |= CTMU_INIT_CH0_ENABLE;

        ctmu_port_init(__this->config->ch0.port);

        //模拟参数配置:
        if (lp_key_online.ch0_ana.isel) {
            LP_CTMU_CH0_ANA_CFG(((lp_key_online.ch0_ana.vlsel << 5) | (lp_key_online.ch0_ana.vhsel << 3) | (lp_key_online.ch0_ana.isel << 0)));
        } else {
            LP_CTMU_CH0_ANA_CFG(get_lpctmu_ana_level());
        }

        //通用配置:
        LP_CTMU_CH0_ENABLE(1); 			//通道0使能
        LP_CTMU_CH0_FILTER_LEVEL(1); 	//通道0滤波级数
        LP_CTMU_CH0_FILTER_EN(1); 		//通道0滤波使能
        LP_CTMU_CH0_EDGE_EN(1);  		//通道0边沿检测使能
        LP_CTMU_CH0_EDGE_TEMP_EN(1); 	//fixed
        LP_CTMU_CH0_FIRST_RISE_MASK(0); //屏蔽第一个上升沿

        //上升沿中断:
        LP_CTMU_CH0_FALL_PENDING_CLR();
        LP_CTMU_CH0_FALL_PENDING_IE(1);
        LP_CTMU_CH0_FALL_PENDING_SEL(0);

        //下升沿中断:
        LP_CTMU_CH0_RISE_PENDING_CLR();
        LP_CTMU_CH0_RISE_PENDING_IE(1);
        LP_CTMU_CH0_RISE_PENDING_SEL(0);

        //多击模式:
        LP_CTMU_CH0_SHORT_KEY_MODE(1);

        if (M2P_CTMU_MSG & CTMU_INIT_CH0_DEBUG) {
            LP_CTMU_CH0_RES_PENDING_IE(1);
        }

        LP_CTMU_CH0_SHORT_KEY_TRIGGER(0);
        LP_CTMU_CH0_SHORT_PENDING_CLR();
        LP_CTMU_CH0_SHORT_PENDING_IE(0);
        LP_CTMU_CH0_LONG_PENDING_CLR();
        LP_CTMU_CH0_LONG_PENDING_IE(1);

        LP_CTMU_CH0_HOLD_PENDING_CLR();
        LP_CTMU_CH0_HOLD_PENDING_IE(1);
        LP_CTMU_CH0_HOLD_EN(1);
        LP_CTMU_CH0_KEY_MODE_ENABLE(1);

        //快速 & 低速扫描周期
        LP_CTMU_CH0_HIGH_SPEED_PRD(CFG_CTMU_CH0_HS_PERIOD_VALUE);
        LP_CTMU_CH0_LOW_SPEED_PRD(CFG_CTMU_CH0_LS_PERIOD_VALUE);

        //采样窗口时间
        LP_CTMU_CH0_DET_TIME(CFG_CTMU_CH0_WINDOW_TIME_VALUE);

        //CH0阈值:
        LP_CTMU_CH0_TEMP_TH(lp_key_online.ch0_debug_cfg.cfg0);
        LP_CTMU_CH0_STABLE_TH(lp_key_online.ch0_debug_cfg.cfg0 + 5);
        LP_CTMU_CH0_EDGE_TH(lp_key_online.ch0_debug_cfg.cfg2);

        M2P_CTMU_CH0_CFG2H = (lp_key_online.ch0_debug_cfg.cfg2 >> 8) & 0xff;
        M2P_CTMU_CH0_CFG2L = (lp_key_online.ch0_debug_cfg.cfg2 >> 0) & 0xff;

        //CH0 按键时间:
        LP_CTMU_CH0_SHORT_KEY_TIME(CFG_M2P_CTMU_CH0_SHORT_TIME_VALUE);
        LP_CTMU_CH0_LONG_KEY_TIME(CFG_M2P_CTMU_CH0_LONG_TIME_VALUE);
        LP_CTMU_CH0_HOLD_KEY_TIME(CFG_M2P_CTMU_CH0_HOLD_TIME_VALUE);

        LP_CTMU_CH0_IE(1);

        //长按开机时间配置
        M2P_CTMU_CH0_SOFTOFF_LONG_TIMEL = (CFG_M2P_CTMU_CH0_SOFTOFF_LONG_TIME & 0xFF);
        M2P_CTMU_CH0_SOFTOFF_LONG_TIMEH = ((CFG_M2P_CTMU_CH0_SOFTOFF_LONG_TIME >> 8) & 0xFF);
#if 0
        log_info("P11_LCTM_CON = 0x%x", P11_LCTM_CON);
        log_info("LCTM_MOD = 0x%x", LCTM_MOD);
        log_info("LCTM_TMR = 0x%x", LCTM_TMR);
        log_info("LCTM_TIME_BASE_H = 0x%x", LCTM_TIME_BASE_H);
        log_info("LCTM_TIME_BASE_L = 0x%x", LCTM_TIME_BASE_L);
        log_info("LCTM_CHL0_ANA = 0x%x", LCTM_CHL0_ANA);
        log_info("LCTM_CHL0_CON0 = 0x%x", LCTM_CHL0_CON0);
        log_info("LCTM_CHL0_CON1 = 0x%x", LCTM_CHL0_CON1);
        log_info("LCTM_CHL0_CON2 = 0x%x", LCTM_CHL0_CON2);
        log_info("LCTM_CHL0_CON3 = 0x%x", LCTM_CHL0_CON3);
        log_info("LCTM_CHL0_HS_PRD = 0x%x", LCTM_CHL0_HS_PRD);
        log_info("LCTM_CHL0_LS_PRD = 0x%x", LCTM_CHL0_LS_PRD);
        log_info("LCTM_CHL0_DET_TIME_H = 0x%x", LCTM_CHL0_DET_TIME_H);
        log_info("LCTM_CHL0_DET_TIME_L = 0x%x", LCTM_CHL0_DET_TIME_L);
        log_info("LCTM_CHL0_TEMP_H = 0x%x", LCTM_CHL0_TEMP_H);
        log_info("LCTM_CHL0_TEMP_L = 0x%x", LCTM_CHL0_TEMP_L);
        log_info("LCTM_CHL0_STA_H = 0x%x", LCTM_CHL0_STA_H);
        log_info("LCTM_CHL0_STA_L = 0x%x", LCTM_CHL0_STA_L);
        log_info("LCTM_CHL0_EDGE_H = 0x%x", LCTM_CHL0_EDGE_H);
        log_info("LCTM_CHL0_EDGE_L = 0x%x", LCTM_CHL0_EDGE_L);
        log_info("LCTM_CHL0_SHORT_H = 0x%x", LCTM_CHL0_SHORT_H);
        log_info("LCTM_CHL0_SHORT_L = 0x%x", LCTM_CHL0_SHORT_L);
        log_info("LCTM_CHL0_LONG_H = 0x%x", LCTM_CHL0_LONG_H);
        log_info("LCTM_CHL0_LONG_L = 0x%x", LCTM_CHL0_LONG_L);
        log_info("LCTM_CHL0_HOLD_H = 0x%x", LCTM_CHL0_HOLD_H);
        log_info("LCTM_CHL0_HOLD_L = 0x%x", LCTM_CHL0_HOLD_L);
#endif
    }

//==============================================================//
//                      CH1 初始化                              //
//==============================================================//
    if (__this->config->ch1.enable) {
        if (__this->config->ch1.port == IO_PORTB_01) {
            LP_CTMU_IO_INV(1);
        } else {
            LP_CTMU_IO_INV(0);
            ASSERT(__this->config->ch1.port == IO_PORTB_02);
        }
        M2P_CTMU_MSG |= CTMU_INIT_CH1_ENABLE;
        ctmu_port_init(__this->config->ch1.port);

        //模拟参数配置:
        if (lp_key_online.ch1_ana.isel) {
            LP_CTMU_CH1_ANA_CFG(((lp_key_online.ch1_ana.vlsel << 5) | (lp_key_online.ch1_ana.vhsel << 3) | (lp_key_online.ch1_ana.isel << 0)));
        } else {
            LP_CTMU_CH1_ANA_CFG(get_lpctmu_ana_level());
        }

        //通用配置:
        LP_CTMU_CH1_ENABLE(1); 			//通道0使能
        LP_CTMU_CH1_FILTER_LEVEL(1); 	//通道0滤波级数
        LP_CTMU_CH1_FILTER_EN(1); 		//通道0滤波使能
        LP_CTMU_CH1_EDGE_EN(1);  		//通道0边沿检测使能
        LP_CTMU_CH1_EDGE_TEMP_EN(1); 	//fixed
        LP_CTMU_CH1_FIRST_RISE_MASK(0); //屏蔽第一个上升沿

        //上升沿中断:
        LP_CTMU_CH1_FALL_PENDING_CLR();
        LP_CTMU_CH1_FALL_PENDING_IE(1);
        LP_CTMU_CH1_FALL_PENDING_SEL(0);

        //下升沿中断:
        LP_CTMU_CH1_RISE_PENDING_CLR();
        LP_CTMU_CH1_RISE_PENDING_IE(1);
        LP_CTMU_CH1_RISE_PENDING_SEL(0);

        if (M2P_CTMU_MSG & CTMU_INIT_CH1_DEBUG) {
            LP_CTMU_CH1_RES_PENDING_IE(1);
        }

        //快速 & 低速扫描周期
        LP_CTMU_CH1_HIGH_SPEED_PRD(CFG_CTMU_CH1_HS_PERIOD_VALUE);
        LP_CTMU_CH1_LOW_SPEED_PRD(CFG_CTMU_CH1_LS_PERIOD_VALUE);

        //采样窗口时间
        LP_CTMU_CH1_DET_TIME(CFG_CTMU_CH1_WINDOW_TIME_VALUE);

        //CH1阈值:
        LP_CTMU_CH1_TEMP_TH(lp_key_online.ch1_debug_cfg.cfg0);
        LP_CTMU_CH1_STABLE_TH(lp_key_online.ch1_debug_cfg.cfg0 + 5);
        LP_CTMU_CH1_EDGE_TH(lp_key_online.ch1_debug_cfg.cfg2);

        LP_CTMU_CH1_IE(1);
#if 0
        log_info("P11_LCTM_CON = 0x%x", P11_LCTM_CON);
        log_info("LCTM_MOD = 0x%x", LCTM_MOD);
        log_info("LCTM_TMR = 0x%x", LCTM_TMR);
        log_info("LCTM_TIME_BASE_H = 0x%x", LCTM_TIME_BASE_H);
        log_info("LCTM_TIME_BASE_L = 0x%x", LCTM_TIME_BASE_L);
        log_info("LCTM_CHL1_ANA = 0x%x", LCTM_CHL1_ANA);
        log_info("LCTM_CHL1_CON0 = 0x%x", LCTM_CHL1_CON0);
        log_info("LCTM_CHL1_CON1 = 0x%x", LCTM_CHL1_CON1);
        log_info("LCTM_CHL1_CON2 = 0x%x", LCTM_CHL1_CON2);
        log_info("LCTM_CHL1_HS_PRD = 0x%x", LCTM_CHL1_HS_PRD);
        log_info("LCTM_CHL1_LS_PRD = 0x%x", LCTM_CHL1_LS_PRD);
        log_info("LCTM_CHL1_DET_TIME_H = 0x%x", LCTM_CHL1_DET_TIME_H);
        log_info("LCTM_CHL1_DET_TIME_L = 0x%x", LCTM_CHL1_DET_TIME_L);
        log_info("LCTM_CHL1_TEMP_H = 0x%x", LCTM_CHL1_TEMP_H);
        log_info("LCTM_CHL1_TEMP_L = 0x%x", LCTM_CHL1_TEMP_L);
        log_info("LCTM_CHL1_STA_H = 0x%x", LCTM_CHL1_STA_H);
        log_info("LCTM_CHL1_STA_L = 0x%x", LCTM_CHL1_STA_L);
        log_info("LCTM_CHL1_EDGE_H = 0x%x", LCTM_CHL1_EDGE_H);
        log_info("LCTM_CHL1_EDGE_L = 0x%x", LCTM_CHL1_EDGE_L);
#endif
    }

    load_p11_bank_code2ram(1, 0);

    //CTMU 初始化命令
    lp_touch_key_send_cmd(CTMU_M2P_INIT);

    return 0;
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
    /* if (__this->long_timer != 0xFFFF) { */
    /* usr_timer_del(__this->long_timer); */
    /* __this->long_timer = 0xFFFF; */
    /* } */
    __this->last_key = CTMU_KEY_NULL;

    lp_touch_key_debug_reinit_config();

    return 0;
}

static int lp_touch_key_online_debug_parse(u8 *packet, u8 size, u8 *ext_data, u16 ext_size)
{
    int res_data = 0;
    touch_receive_cmd_t *touch_cmd;
    int err = 0;
    u8 parse_seq = ext_data[1];
    struct ch_adjust_table *receive_cfg;
    struct ch_ana_cfg *ana_cfg;
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
        app_online_db_ack(parse_seq, (u8 *)"OK", 2); //该命令随便ack "OK"字符串
        lp_key_online.state = LP_KEY_ONLINE_ST_CH_KEY_DEBUG_START;

        receive_cfg = (struct ch_adjust_table *)(touch_cmd->data);
        if (lp_key_online.current_record_ch == 0) {
            lp_key_online.ch0_debug_cfg.cfg0 = receive_cfg->cfg0;
            lp_key_online.ch0_debug_cfg.cfg1 = receive_cfg->cfg1;
            lp_key_online.ch0_debug_cfg.cfg2 = receive_cfg->cfg2;
        } else if (lp_key_online.current_record_ch == 1) {
            lp_key_online.ch1_debug_cfg.cfg0 = receive_cfg->cfg0;
            lp_key_online.ch1_debug_cfg.cfg1 = receive_cfg->cfg1;
            lp_key_online.ch1_debug_cfg.cfg2 = receive_cfg->cfg2;
        }
        log_debug("update CH %d, cfg0 = %d, cfg1 = %d, cfg2 = %d", lp_key_online.current_record_ch, receive_cfg->cfg0, receive_cfg->cfg1, receive_cfg->cfg2);
        lp_touch_key_debug_reinit(lp_key_online.state);
        break;
    case TOUCH_CH_VOL_CFG_UPDATE:
        log_debug("TOUCH_CH_VOL_CFG_UPDATE");
        app_online_db_ack(parse_seq, (u8 *)"OK", 2); //回"OK"字符串
        ana_cfg = (struct ch_ana_cfg *)(touch_cmd->data);
        if (lp_key_online.current_record_ch == 0) {
            lp_key_online.ch0_ana.isel = ana_cfg->isel;
            lp_key_online.ch0_ana.vhsel = ana_cfg->vhsel;
            lp_key_online.ch0_ana.vlsel = ana_cfg->vlsel;
        } else if (lp_key_online.current_record_ch == 1) {
            lp_key_online.ch1_ana.isel = ana_cfg->isel;
            lp_key_online.ch1_ana.vhsel = ana_cfg->vhsel;
            lp_key_online.ch1_ana.vlsel = ana_cfg->vlsel;
        }
        log_debug("update, isel = %d, vhsel = %d, vlsel = %d", lp_key_online.ch_ana.isel, lp_key_online.ch_ana.vhsel, lp_key_online.ch_ana.vlsel);
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
        if (lp_key_online.current_record_ch == 0) {
            lp_key_online.ch0_ana.isel = 0;
        } else if (lp_key_online.current_record_ch == 1) {
            lp_key_online.ch1_ana.isel = 0;
        }
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

    u8 ch0_sensity = __this->config->ch0.sensitivity;
    u8 ch1_sensity = __this->config->ch1.sensitivity;

    if (__this->config->ch0.enable) {
        lp_key_online.ch0_debug_cfg.cfg0 = ch0_sensitivity_table[ch0_sensity].cfg0;
        lp_key_online.ch0_debug_cfg.cfg1 = ch0_sensitivity_table[ch0_sensity].cfg1;
        lp_key_online.ch0_debug_cfg.cfg2 = ch0_sensitivity_table[ch0_sensity].cfg2;
    }

    if (__this->config->ch1.enable) {
        lp_key_online.ch1_debug_cfg.cfg0 = ch1_sensitivity_table[ch1_sensity].cfg0;
        lp_key_online.ch1_debug_cfg.cfg1 = ch1_sensitivity_table[ch1_sensity].cfg1;
        lp_key_online.ch1_debug_cfg.cfg2 = ch1_sensitivity_table[ch1_sensity].cfg2;

    }

    return 0;
}

int lp_touch_key_online_debug_exit(void)
{
    return 0;
}
#endif /* #if TCFG_LP_TOUCH_KEY_BT_TOOL_ENABLE */

#endif

