#include "asm/clock.h"
#include "timer.h"
#include "asm/power/p33.h"
#include "asm/charge.h"
#include "asm/adc_api.h"
#include "uart.h"
#include "device/device.h"
#include "asm/power_interface.h"
/*#include "power/power_hw.h"*/
#include "system/event.h"
#include "asm/efuse.h"
#include "gpio.h"
#include "app_config.h"

#define LOG_TAG_CONST   CHARGE
#define LOG_TAG         "[CHARGE]"
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#include "debug.h"

#define IO_PORT_LDOIN   LINDT_IN//IO编号

typedef struct _CHARGE_VAR {
    struct charge_platform_data *data;
    volatile u8 charge_online_flag;
    volatile u8 init_ok;
    volatile int ldo5v_timer;
    volatile int charge_timer;
    volatile int power_sw_timer;
} CHARGE_VAR;

#define __this 	(&charge_var)
static CHARGE_VAR charge_var;
extern void charge_set_callback(void (*wakup_callback)(void), void (*sub_callback)(void));
static u8 charge_flag;

#define BIT_LDO5V_IN		BIT(0)
#define BIT_LDO5V_OFF		BIT(1)
#define BIT_LDO5V_ERR		BIT(2)

u8 get_charge_poweron_en(void)
{
    return __this->data->charge_poweron_en;
}

void charge_check_and_set_pinr(u8 level)
{
    u8 pinr_io;
    if (P33_CON_GET(P3_PINR_CON) & BIT(0)) {
        pinr_io = P33_CON_GET(P3_PORT_SEL0);
        if (pinr_io == IO_PORT_LDOIN) {
            if (level == 0) {
                P33_CON_SET(P3_PINR_CON, 2, 1, 0);
            } else {
                P33_CON_SET(P3_PINR_CON, 2, 1, 1);
            }
        }
    }
}

static void udelay(u32 usec)
{
    JL_TIMER0->CON = BIT(14);
    JL_TIMER0->CNT = 0;
    JL_TIMER0->PRD = clk_get("timer") / 1000000L  * usec; //1us
    JL_TIMER0->CON = BIT(0) | BIT(2) | BIT(6); //sys clk
    while ((JL_TIMER0->CON & BIT(15)) == 0);
    JL_TIMER0->CON = BIT(14);
}

static u8 check_charge_state(void)
{
    u8 online_cnt = 0;
    u8 i = 0;

    __this->charge_online_flag = 0;

    for (i = 0; i < 20; i++) {
        if (LVCMP_DET_GET() || LDO5V_DET_GET()) {
            online_cnt++;
        }
        udelay(1000);
    }
    log_info("online_cnt = %d\n", online_cnt);
    if (online_cnt > 5) {
        __this->charge_online_flag = 1;
    }

    return __this->charge_online_flag;
}

void set_charge_online_flag(u8 flag)
{
    __this->charge_online_flag = flag;
}

u8 get_charge_online_flag(void)
{
    return __this->charge_online_flag;
}

u8 get_ldo5v_online_hw(void)
{
    return LDO5V_DET_GET();
}

u8 get_lvcmp_det(void)
{
    return LVCMP_DET_GET();
}

u8 get_ldo5v_pulldown_en(void)
{
    if (__this->data) {
        return __this->data->ldo5v_pulldown_en;
    }
    return 0;
}

u8 get_ldo5v_pulldown_res(void)
{
    if (__this->data) {
        return __this->data->ldo5v_pulldown_lvl;
    }
    return CHARGE_PULLDOWN_200K;
}

void charge_event_to_user(u8 event)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg  = (void *)DEVICE_EVENT_FROM_CHARGE;
    e.u.dev.event = event;
    e.u.dev.value = 0;
    sys_event_notify(&e);
}

void charge_power_switch_timer(void *priv)
{
    u32 vbat_voltage;
    vbat_voltage = adc_get_voltage(AD_CH_VBAT) * 4 / 10;
    if (vbat_voltage > 300) {
        power_set_charge_mode(0);
        power_set_mode(PWR_DCDC15);
        power_set_charge_mode(1);
        usr_timer_del(__this->power_sw_timer);
        __this->power_sw_timer = 0;
        adc_set_sample_freq(AD_CH_VBAT, 5000);
    }
}

void power_enter_charge_mode(void)
{
    if (TCFG_LOWPOWER_POWER_SEL == PWR_DCDC15) {
        u8 chip_id = get_chip_version() & 0x0f;
        u32 vbat_voltage;
        if ((chip_id >= 0x0C) || (chip_id == 0x01) || (chip_id == 0x02)) {
            vbat_voltage = adc_get_voltage(AD_CH_VBAT) * 4 / 10;
            if (vbat_voltage > 300) {
                power_set_charge_mode(0);
                power_set_mode(PWR_DCDC15);
                power_set_charge_mode(1);
            } else {
                power_set_charge_mode(0);
                power_set_mode(PWR_LDO15);
                power_set_charge_mode(1);
                if (__this->power_sw_timer == 0) {
                    //低压充电不进低功耗
                    adc_set_sample_freq(AD_CH_VBAT, 20);
                    __this->power_sw_timer = usr_timer_add(NULL, charge_power_switch_timer, 1000, 1);
                }
            }
        } else if ((chip_id == 0x03) || (chip_id == 0x04) || (chip_id == 0x06)) {
            power_set_charge_mode(1);
            power_set_mode(PWR_DCDC15_FOR_CHARGE);
        }
    }
}

void power_exit_charge_mode(void)
{
    if (TCFG_LOWPOWER_POWER_SEL == PWR_DCDC15) {
        power_set_charge_mode(0);
        power_set_mode(TCFG_LOWPOWER_POWER_SEL);
        if (__this->power_sw_timer) {
            usr_timer_del(__this->power_sw_timer);
            __this->power_sw_timer = 0;
        }
    }
}

void charge_start(void)
{
    log_info("%s\n", __func__);

    if (__this->charge_timer) {
        usr_timer_del(__this->charge_timer);
        __this->charge_timer = 0;
    }

    power_wakeup_enable_with_port(IO_CHGFL_DET);

    power_enter_charge_mode();

    CHGBG_EN(1);
    CHARGE_EN(1);

    charge_event_to_user(CHARGE_EVENT_CHARGE_START);
}

void charge_close(void)
{
    log_info("%s\n", __func__);

    if (charge_flag != BIT_LDO5V_IN) {
        CHGBG_EN(0);
        CHARGE_EN(0);
    }

    power_exit_charge_mode();

    power_wakeup_disable_with_port(IO_CHGFL_DET);

    charge_event_to_user(CHARGE_EVENT_CHARGE_CLOSE);

    if (__this->charge_timer) {
        usr_timer_del(__this->charge_timer);
        __this->charge_timer = 0;
    }
}

static void charge_full_detect(void *priv)
{
    static u16 charge_full_cnt = 0;

    if (CHARGE_FULL_FLAG_GET() && LVCMP_DET_GET()) {
        /* putchar('F'); */
        if (charge_full_cnt < 5) {
            charge_full_cnt++;
        } else {
            charge_full_cnt = 0;
            usr_timer_del(__this->charge_timer);
            __this->charge_timer = 0;
            charge_event_to_user(CHARGE_EVENT_CHARGE_FULL);
        }
    } else {
        /* putchar('K'); */
        charge_full_cnt = 0;
        usr_timer_del(__this->charge_timer);
        __this->charge_timer = 0;
        power_wakeup_enable_with_port(IO_CHGFL_DET);
    }
}

static void ldo5v_detect(void *priv)
{
    /* log_info("%s\n",__func__); */

    static u16 ldo5v_in_normal_cnt = 0;
    static u16 ldo5v_in_err_cnt = 0;
    static u16 ldo5v_off_cnt = 0;

    if (LVCMP_DET_GET()) {	//ldoin > vbat
        /* putchar('X'); */
        if (ldo5v_in_normal_cnt < 50) {
            ldo5v_in_normal_cnt++;
        } else {
            /* printf("ldo5V_IN\n"); */
            set_charge_online_flag(1);
            ldo5v_off_cnt = 0;
            ldo5v_in_normal_cnt = 0;
            ldo5v_in_err_cnt = 0;
            usr_timer_del(__this->ldo5v_timer);
            __this->ldo5v_timer = 0;
            if ((charge_flag & BIT_LDO5V_IN) == 0) {
                charge_flag = BIT_LDO5V_IN;
                charge_event_to_user(CHARGE_EVENT_LDO5V_IN);
                power_wakeup_set_edge(IO_VBTCH_DET, FALLING_EDGE);//检测ldoin比vbat电压低的情况(充电仓给电池充满后会关断，此时电压会掉下来)
            }
        }
    } else if (LDO5V_DET_GET() == 0) {	//ldoin<拔出电压（0.6）
        /* putchar('Q'); */
        if (ldo5v_off_cnt < (__this->data->ldo5v_off_filter + 10)) {
            ldo5v_off_cnt++;
        } else {
            /* printf("ldo5V_OFF\n"); */
            set_charge_online_flag(0);
            ldo5v_off_cnt = 0;
            ldo5v_in_normal_cnt = 0;
            ldo5v_in_err_cnt = 0;
            usr_timer_del(__this->ldo5v_timer);
            __this->ldo5v_timer = 0;
            if ((charge_flag & BIT_LDO5V_OFF) == 0) {
                charge_flag = BIT_LDO5V_OFF;
                power_wakeup_set_edge(IO_VBTCH_DET, RISING_EDGE);//拔出后重新检测插入
                charge_event_to_user(CHARGE_EVENT_LDO5V_OFF);
            }
        }
    } else {	//拔出电压（0.6左右）< ldoin < vbat
        /* putchar('E'); */
        if (ldo5v_in_err_cnt < 220) {
            ldo5v_in_err_cnt++;
        } else {
            /* printf("ldo5V_ERR\n"); */
            set_charge_online_flag(1);
            ldo5v_off_cnt = 0;
            ldo5v_in_normal_cnt = 0;
            ldo5v_in_err_cnt = 0;
            usr_timer_del(__this->ldo5v_timer);
            __this->ldo5v_timer = 0;
            if ((charge_flag & BIT_LDO5V_ERR) == 0) {
                charge_flag = BIT_LDO5V_ERR;
                power_wakeup_set_edge(IO_VBTCH_DET, RISING_EDGE);
                if (__this->data->ldo5v_off_filter) {
                    charge_event_to_user(CHARGE_EVENT_CHARGE_ERR);
                }
            }
        }
    }
}

void sub_wakeup_isr(void)
{
    /* printf(" %s \n", __func__); */
    if (__this->ldo5v_timer == 0) {
        __this->ldo5v_timer = usr_timer_add(0, ldo5v_detect, 2, 1);
    }
}

void charge_wakeup_isr(void)
{
    /* printf(" %s \n", __func__); */
    power_wakeup_disable_with_port(IO_CHGFL_DET);
    if (__this->charge_timer == 0) {
        __this->charge_timer = usr_timer_add(0, charge_full_detect, 2, 1);
    }
}

#if 0
static void test_func(void *priv)
{
    /* if (P33_CON_GET(P3_CHG_READ) & BIT(0)) { */
    /* JL_PORTA->DIR &= ~BIT(3); */
    /* JL_PORTA->OUT |= BIT(3); */
    /* } else { */
    /* JL_PORTA->OUT &= ~BIT(3); */
    /* } */
}
#endif

u8 get_charge_mA_config(void)
{
    return __this->data->charge_mA;
}

void set_charge_mA(u8 charge_mA)
{
    static u8 charge_mA_old = 0xff;
    if (charge_mA_old != charge_mA) {
        charge_mA_old = charge_mA;
        CHARGE_mA_SEL(charge_mA);
    }
}

const u16 full_table[CHARGE_FULL_V_MAX] = {
    3962, 4002, 4044, 4086, 4130, 4175, 4222, 4270,
    4308, 4349, 4391, 4434, 4472, 4517, 4564, 4611
};
u16 get_charge_full_value(void)
{
    ASSERT(__this->init_ok, "charge not init ok!\n");
    ASSERT(__this->data->charge_full_V < CHARGE_FULL_V_MAX);
    return full_table[__this->data->charge_full_V];
}

static void charge_config(void)
{
    u8 charge_4202_trim_val = CHARGE_FULL_V_4222;
    u8 offset = 0;
    u8 charge_full_v_val = 0;

    if (get_vbat_trim() == 0xf) {
        log_info("vbat not trim, use default config!!!!!!");
    } else {
        charge_4202_trim_val = get_vbat_trim();		//4.2V对应的trim出来的实际档位
    }

    log_info("charge_4202_trim_val = %d\n", charge_4202_trim_val);

    if (__this->data->charge_full_V >= CHARGE_FULL_V_4222) {
        offset = __this->data->charge_full_V - CHARGE_FULL_V_4222;
        charge_full_v_val = charge_4202_trim_val + offset;
        if (charge_full_v_val > 0xf) {
            charge_full_v_val = 0xf;
        }
    } else {
        offset = CHARGE_FULL_V_4222 - __this->data->charge_full_V;
        if (charge_4202_trim_val >= offset) {
            charge_full_v_val = charge_4202_trim_val - offset;
        } else {
            charge_full_v_val = 0;
        }
    }

    log_info("charge_full_v_val = %d\n", charge_full_v_val);

    CHARGE_FULL_V_SEL(charge_full_v_val);
    CHARGE_FULL_mA_SEL(__this->data->charge_full_mA);
    /* CHARGE_mA_SEL(__this->data->charge_mA); */
    CHARGE_mA_SEL(CHARGE_mA_20);
}

static int charge_init(const struct dev_node *node, void *arg)
{
    log_info("%s\n", __func__);

    __this->data = (struct charge_platform_data *)arg;

    ASSERT(__this->data);

    __this->init_ok = 0;
    __this->charge_online_flag = 0;

    /*先关闭充电使能，后面检测到充电插入再开启*/
    power_wakeup_disable_with_port(IO_CHGFL_DET);
    CHGBG_EN(0);
    CHARGE_EN(0);

    /*LDO5V的100K下拉电阻使能*/
    L5V_RES_DET_S_SEL(__this->data->ldo5v_pulldown_lvl);
    L5V_LOAD_EN(__this->data->ldo5v_pulldown_en);

    charge_config();

    if (check_charge_state()) {
        if (__this->ldo5v_timer == 0) {
            __this->ldo5v_timer = usr_timer_add(0, ldo5v_detect, 2, 1);
        }
    } else {
        charge_flag = BIT_LDO5V_OFF;
    }
    charge_set_callback(charge_wakeup_isr, sub_wakeup_isr);

    /* usr_timer_add(0, test_func, 10); */

    __this->init_ok = 1;

    return 0;
}

void charge_module_stop(void)
{
    if (!__this->init_ok) {
        return;
    }
    charge_set_callback(NULL, NULL);
    charge_close();
    power_wakeup_disable_with_port(IO_LDOIN_DET);
    power_wakeup_disable_with_port(IO_VBTCH_DET);
    if (__this->ldo5v_timer) {
        usr_timer_del(__this->ldo5v_timer);
        __this->ldo5v_timer = 0;
    }
}

void charge_module_restart(void)
{
    if (!__this->init_ok) {
        return;
    }
    if (!__this->ldo5v_timer) {
        __this->ldo5v_timer = usr_timer_add(NULL, ldo5v_detect, 2, 1);
    }
    charge_set_callback(charge_wakeup_isr, sub_wakeup_isr);
    power_wakeup_enable_with_port(IO_LDOIN_DET);
    power_wakeup_enable_with_port(IO_VBTCH_DET);
}

const struct device_operations charge_dev_ops = {
    .init  = charge_init,
};
