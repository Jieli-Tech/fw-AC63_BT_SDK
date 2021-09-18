#include "system/includes.h"
#include "app_power_manage.h"
#include "app_main.h"
#include "app_config.h"
#include "app_action.h"
#include "asm/charge.h"
#include "asm/adc_api.h"
#include "btstack/avctp_user.h"
#include "user_cfg.h"
#include "asm/charge.h"

#define LOG_TAG_CONST       APP_POWER
#define LOG_TAG             "[APP_POWER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

enum {
    VBAT_NORMAL = 0,
    VBAT_WARNING,
    VBAT_LOWPOWER,
} VBAT_STATUS;

#define VBAT_DETECT_CNT       (2*1) //2*N
#define VBAT_DETECT_ADC_MS    (10)  //unint:ms
#define VBAT_PERIOD_CHECK_S   (30)  //unint:s

static int vbat_slow_timer = 0;
static int vbat_fast_timer = 0;
static int lowpower_timer = 0;
static u8 old_battery_level = 9;
static u16 bat_val = 0;
static volatile u8 cur_battery_level = 0;
static u16 battery_full_value = 0;
static u8 cur_bat_st = VBAT_NORMAL;


void vbat_check(void *priv);
void clr_wdt(void);

void power_event_to_user(u8 event)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = (void *)DEVICE_EVENT_FROM_POWER;
    e.u.dev.event = event;
    e.u.dev.value = 0;
    sys_event_notify(&e);
}

int app_power_event_handler(struct device_event *dev, void (*set_soft_poweroff_call)(void))
{
    int ret = false;

#if(TCFG_SYS_LVD_EN == 1)

    switch (dev->event) {
    case POWER_EVENT_POWER_NORMAL:
        break;
    case POWER_EVENT_POWER_WARNING:
        r_printf(" POWER_EVENT_POWER_WARNING");
        if (lowpower_timer == 0) {
            lowpower_timer = sys_timer_add((void *)POWER_EVENT_POWER_WARNING, (void (*)(void *))power_event_to_user, LOW_POWER_WARN_TIME);
        }
        break;
    case POWER_EVENT_POWER_LOW:
        r_printf(" POWER_EVENT_POWER_LOW");
        vbat_timer_delete();
        if (lowpower_timer) {
            sys_timer_del(lowpower_timer);
            lowpower_timer = 0 ;
        }

        if (set_soft_poweroff_call) {
            log_info("POWER_LOW TO SOFT_POWEROFF");
            set_soft_poweroff_call();
        }
        break;
    case POWER_EVENT_POWER_CHANGE:
        /* log_info("POWER_EVENT_POWER_CHANGE\n"); */
        user_send_cmd_prepare(USER_CTRL_HFP_CMD_UPDATE_BATTARY, 0, NULL);
        break;
    case POWER_EVENT_POWER_CHARGE:
        if (lowpower_timer) {
            sys_timer_del(lowpower_timer);
            lowpower_timer = 0 ;
        }
        break;
    default:
        break;
    }
#endif

    return ret;
}


u16 get_vbat_level(void)
{
    //return 370;     //debug
    return (adc_get_voltage(AD_CH_VBAT) * 4 / 10);
}

__attribute__((weak)) u8 remap_calculate_vbat_percent(u16 bat_val)
{
    return 0;
}

u16 get_vbat_value(void)
{
    return bat_val;
}

u8 get_vbat_percent(void)
{
    u16 tmp_bat_val;
    u16 bat_val = get_vbat_level();
    if (battery_full_value == 0) {
#if TCFG_CHARGE_ENABLE
        battery_full_value = (get_charge_full_value() - 100) / 10; //防止部分电池充不了这么高电量，充满显示未满的情况
#else
        battery_full_value = 420;
#endif
    }

    if (bat_val <= app_var.poweroff_tone_v) {
        return 0;
    }

    tmp_bat_val = remap_calculate_vbat_percent(bat_val);
    if (!tmp_bat_val) {
        tmp_bat_val = ((u32)bat_val - app_var.poweroff_tone_v) * 100 / (battery_full_value - app_var.poweroff_tone_v);
        if (tmp_bat_val > 100) {
            tmp_bat_val = 100;
        }
    }
    return (u8)tmp_bat_val;
}

bool get_vbat_need_shutdown(void)
{
    if ((bat_val <= LOW_POWER_SHUTDOWN) || adc_check_vbat_lowpower()) {
        return TRUE;
    }
    return FALSE;
}

//将当前电量转换为1~9级发送给手机同步电量
u8  battery_value_to_phone_level(u16 bat_val)
{
    u8  battery_level = 0;
    u8 vbat_percent = get_vbat_percent();

    if (vbat_percent < 5) { //小于5%电量等级为0，显示10%
        return 0;
    }

    battery_level = (vbat_percent - 5) / 10;

    return battery_level;
}

//获取自身的电量
u8  get_self_battery_level(void)
{
    return cur_battery_level;
}

u8  get_cur_battery_level(void)
{
    return cur_battery_level;
}

void vbat_check_slow(void *priv)
{
    if (vbat_fast_timer == 0) {
        vbat_fast_timer = usr_timer_add(NULL, vbat_check, VBAT_DETECT_ADC_MS, 1);
    }
    if (get_charge_online_flag()) {
        sys_timer_modify(vbat_slow_timer, 60 * 1000);
    } else {
        sys_timer_modify(vbat_slow_timer, VBAT_PERIOD_CHECK_S * 1000);
    }
}

void vbat_check_init(void)
{
    if (vbat_slow_timer == 0) {
        vbat_slow_timer = sys_timer_add(NULL, vbat_check_slow, VBAT_PERIOD_CHECK_S * 1000);
    } else {
        sys_timer_modify(vbat_slow_timer, VBAT_PERIOD_CHECK_S * 1000);
    }

    if (vbat_fast_timer == 0) {
        vbat_fast_timer = usr_timer_add(NULL, vbat_check, VBAT_DETECT_ADC_MS, 1);
    }
}

void vbat_timer_delete(void)
{
    if (vbat_slow_timer) {
        sys_timer_del(vbat_slow_timer);
        vbat_slow_timer = 0;
    }
    if (vbat_fast_timer) {
        usr_timer_del(vbat_fast_timer);
        vbat_fast_timer = 0;
    }
}

void vbat_check(void *priv)
{
    static u8 unit_cnt = 0;
    static u8 low_warn_cnt = 0;
    static u8 low_off_cnt = 0;
    static u8 low_voice_cnt = 0;
    static u8 low_power_cnt = 0;
    static u8 power_normal_cnt = 0;
    static u8 charge_ccvol_v_cnt = 0;
    static u8 charge_online_flag = 0;
    static u8 low_voice_first_flag = 1;//进入低电后先提醒一次

    if (!bat_val) {
        bat_val = get_vbat_level();
    } else {
        bat_val = (get_vbat_level() + bat_val) / 2;
    }

    cur_battery_level = battery_value_to_phone_level(bat_val);

    printf("bv:%d, bl:%d , check_vbat:%d\n", bat_val, cur_battery_level, adc_check_vbat_lowpower());

    unit_cnt++;

    if (adc_check_vbat_lowpower() || (bat_val <= app_var.poweroff_tone_v)) {
        low_off_cnt++;
    }
    if (bat_val <= app_var.warning_tone_v) {
        low_warn_cnt++;
    }
#if TCFG_CHARGE_ENABLE
    if (bat_val >= CHARGE_CCVOL_V) {
        charge_ccvol_v_cnt++;
    }
#endif

    if (unit_cnt >= VBAT_DETECT_CNT) {

        if (get_charge_online_flag() == 0) {
            if (low_off_cnt > (VBAT_DETECT_CNT / 2)) { //低电关机
                low_power_cnt++;
                low_voice_cnt = 0;
                power_normal_cnt = 0;
                cur_bat_st = VBAT_LOWPOWER;
                if (low_power_cnt > 6) {
                    log_info("\n*******Low Power,enter softpoweroff********\n");
                    low_power_cnt = 0;
                    power_event_to_user(POWER_EVENT_POWER_LOW);
                    usr_timer_del(vbat_fast_timer);
                    vbat_fast_timer = 0;
                }
            } else if (low_warn_cnt > (VBAT_DETECT_CNT / 2)) { //低电提醒
                low_voice_cnt ++;
                low_power_cnt = 0;
                power_normal_cnt = 0;
                cur_bat_st = VBAT_WARNING;
                if ((low_voice_first_flag && low_voice_cnt > 1) || //第一次进低电10s后报一次
                    (!low_voice_first_flag && low_voice_cnt >= 5)) {
                    low_voice_first_flag = 0;
                    low_voice_cnt = 0;
                    if (!lowpower_timer) {
                        log_info("\n**Low Power,Please Charge Soon!!!**\n");
                        power_event_to_user(POWER_EVENT_POWER_WARNING);
                    }
                }
            } else {
                power_normal_cnt++;
                low_voice_cnt = 0;
                low_power_cnt = 0;
                if (power_normal_cnt > 2) {
                    if (cur_bat_st != VBAT_NORMAL) {
                        log_info("[Noraml power]\n");
                        cur_bat_st = VBAT_NORMAL;
                        power_event_to_user(POWER_EVENT_POWER_NORMAL);
                    }
                }
            }
        } else {
            power_event_to_user(POWER_EVENT_POWER_CHARGE);
#if TCFG_CHARGE_ENABLE
            if (charge_ccvol_v_cnt > (VBAT_DETECT_CNT / 2)) {
                set_charge_mA(get_charge_mA_config());
            }
#endif
        }

        unit_cnt = 0;
        low_off_cnt = 0;
        low_warn_cnt = 0;
        charge_ccvol_v_cnt = 0;

        if (cur_bat_st != VBAT_LOWPOWER) {
            usr_timer_del(vbat_fast_timer);
            vbat_fast_timer = 0;
            cur_battery_level = battery_value_to_phone_level(bat_val);
            if (cur_battery_level != old_battery_level) {
                power_event_to_user(POWER_EVENT_POWER_CHANGE);
            } else {
                if (charge_online_flag != get_charge_online_flag()) {
                    //充电变化也要交换，确定是否在充电仓
                    power_event_to_user(POWER_EVENT_POWER_CHANGE);
                }
            }
            charge_online_flag =  get_charge_online_flag();
            old_battery_level = cur_battery_level;
        }
    }
}

bool vbat_is_low_power(void)
{
    return (cur_bat_st != VBAT_NORMAL);
}

void check_power_on_voltage(void)
{
#if(TCFG_SYS_LVD_EN == 1)

    u16 val = 0;
    u8 normal_power_cnt = 0;
    u8 low_power_cnt = 0;

    while (1) {
        clr_wdt();
        val = get_vbat_level();
        printf("vbat: %d\n", val);
        if ((val < app_var.poweroff_tone_v) || adc_check_vbat_lowpower()) {
            low_power_cnt++;
            normal_power_cnt = 0;
            if (low_power_cnt > 10) {
                os_time_dly(100);
                log_info("power on low power , enter softpoweroff!\n");
                power_set_soft_poweroff();
            }
        } else {
            normal_power_cnt++;
            low_power_cnt = 0;
            if (normal_power_cnt > 10) {
                vbat_check_init();
                return;
            }
        }
    }
#endif
}
