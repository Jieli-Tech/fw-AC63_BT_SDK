#include "app_config.h"
#include "asm/charge.h"
#include "asm/pwm_led.h"
#include "system/event.h"
#include "system/app_core.h"
#include "system/includes.h"
#include "app_action.h"
#include "asm/wdt.h"
#include "app_main.h"
#include "app_power_manage.h"
#include "app_handshake.h"

#define LOG_TAG_CONST       APP_CHARGE
#define LOG_TAG             "[APP_CHARGE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#if TCFG_CHARGE_ENABLE

#if TCFG_HANDSHAKE_ENABLE
static void handshake_complete(void)
{
    handshake_app_stop();
    if (get_lvcmp_det()) {
        charge_start();
    }
}
#endif

void charge_start_deal(void)
{
    log_info("%s\n", __func__);
    power_set_mode(PWR_LDO15);
}

void ldo5v_keep_deal(void)
{
    log_info("%s\n", __func__);
#if TCFG_HANDSHAKE_ENABLE
    handshake_app_start(2, handshake_complete);
#endif
}

void charge_full_deal(void)
{
    log_info("%s\n", __func__);
    charge_close();
}

void charge_close_deal(void)
{
    log_info("%s\n", __FUNCTION__);
}

void charge_ldo5v_in_deal(void)
{
    log_info("%s\n", __FUNCTION__);
#if TCFG_HANDSHAKE_ENABLE
    handshake_app_start(2, handshake_complete);
#else
    charge_start();
#endif
}

void charge_ldo5v_off_deal(void)
{
    log_info("%s\n", __FUNCTION__);
#if TCFG_HANDSHAKE_ENABLE
    handshake_app_stop();
#endif
    charge_close();
    power_set_mode(TCFG_LOWPOWER_POWER_SEL);
#if TCFG_SYS_LVD_EN
    vbat_check_init();
#endif
}

int app_charge_event_handler(struct device_event *dev)
{
    switch (dev->event) {
    case CHARGE_EVENT_CHARGE_START:
        charge_start_deal();
        break;
    case CHARGE_EVENT_CHARGE_CLOSE:
        charge_close_deal();
        break;
    case CHARGE_EVENT_CHARGE_FULL:
        charge_full_deal();
        break;
    case CHARGE_EVENT_LDO5V_KEEP:
        ldo5v_keep_deal();
        break;
    case CHARGE_EVENT_LDO5V_IN:
        charge_ldo5v_in_deal();
        break;
    case CHARGE_EVENT_LDO5V_OFF:
        charge_ldo5v_off_deal();
        break;
    default:
        break;
    }
    return 0;
}

#endif

