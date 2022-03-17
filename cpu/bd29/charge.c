#include "asm/clock.h"
#include "timer.h"
#include "asm/power/p33.h"
#include "asm/charge.h"
#include "uart.h"
#include "device/device.h"
#include "asm/power_interface.h"
/*#include "power/power_hw.h"*/
#include "system/event.h"
#include "asm/efuse.h"
#include "gpio.h"
#include "asm/adc_api.h"

#define LOG_TAG_CONST   CHARGE
#define LOG_TAG         "[CHARGE]"
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#include "debug.h"

typedef struct _CHARGE_VAR {
    struct charge_platform_data *data;
    volatile u8 charge_online_flag;
    volatile u8 charge_event_flag;
    volatile u8 init_ok;
    volatile u8 detect_stop;
    volatile int ldo5v_timer;
    volatile int charge_timer;
    volatile int cc_timer;      //涓流切恒流的sys timer
} CHARGE_VAR;

#define __this 	(&charge_var)
static CHARGE_VAR charge_var;
extern void charge_set_callback(void (*wakup_callback)(void), void (*sub_callback)(void));
static u8 charge_flag;

u8 get_charge_poweron_en(void)
{
    return __this->data->charge_poweron_en;
}

static void udelay(u32 usec)
{
    JL_TIMER0->CON = BIT(14);
    JL_TIMER0->CNT = 0;
    JL_TIMER0->PRD = clk_get("timer") / 1000000L  * usec; //1us
    JL_TIMER0->CON = BIT(0) | BIT(3); //sys clk
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

void set_charge_event_flag(u8 flag)
{
    __this->charge_event_flag = flag;
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
    return __this->data->ldo5v_pulldown_en;
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

static void set_charge_wkup_source(u8 source)
{
    CHARGE_WKUP_EDGE_SEL(0);	//0:上升沿,高电平 1:下降沿，低电平
    CHARGE_WKUP_SOURCE_SEL(source);
    CHARGE_EDGE_DETECT_EN(1);
    CHARGE_LEVEL_DETECT_EN(1);
    CHARGE_WKUP_EN(1);
    CHARGE_WKUP_PND_CLR();
}

/*检测是否满足进入恒流充电条件*/
static void charge_cc_check(void *priv)
{
    log_info("%s\n", __func__);
    if ((adc_get_voltage(AD_CH_VBAT) * 4 / 10) > CHARGE_CCVOL_V) {
        /*满足进入恒流充电条件 设置恒流充电电流大小*/
        set_charge_mA(__this->data->charge_mA);
        usr_timer_del(__this->cc_timer);
        __this->cc_timer = 0;
    }
}

void charge_start(void)
{
    log_info("%s\n", __func__);

    if (__this->charge_timer) {
        usr_timer_del(__this->charge_timer);
        __this->charge_timer = 0;
    }

    set_charge_wkup_source(CHARGE_FULL_33V);

    /*进入恒流充电(VBAT > 3V)*/
    if ((adc_get_voltage(AD_CH_VBAT) * 4 / 10) > CHARGE_CCVOL_V) {
        /*设置恒流充电电流大小*/
        set_charge_mA(__this->data->charge_mA);
    } else {
        /*设置涓流电流大小*/
        CHARGE_mA_SEL(CHARGE_mA_20);
        if (!__this->cc_timer) {
            /*每1分钟检测一次是否具备进入恒流充电的条件*/
            __this->cc_timer = usr_timer_add(NULL, charge_cc_check, 1000, 1);
        }
    }

    CHGBG_EN(1);
    CHARGE_EN(1);

    charge_event_to_user(CHARGE_EVENT_CHARGE_START);
}

void charge_close(void)
{
    log_info("%s\n", __func__);

    CHARGE_WKUP_EN(0);
    CHARGE_EDGE_DETECT_EN(0);
    CHARGE_LEVEL_DETECT_EN(0);
    CHGBG_EN(0);
    CHARGE_EN(0);
    CHARGE_WKUP_PND_CLR();

    charge_event_to_user(CHARGE_EVENT_CHARGE_CLOSE);

    if (__this->charge_timer) {
        usr_timer_del(__this->charge_timer);
        __this->charge_timer = 0;
    }

    if (__this->cc_timer) {
        usr_timer_del(__this->cc_timer);
        __this->cc_timer = 0;
    }

}

static void charge_full_detect(void *priv)
{
    static u16 charge_full_cnt = 0;
    static u16 charge_not_full_cnt = 0;

    if (CHARGE_FULL_FLAG_GET() && LVCMP_DET_GET()) {
        /* putchar('F'); */
        charge_not_full_cnt = 0;
        if (charge_full_cnt < 25) {
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
        if (charge_not_full_cnt < 5) {
            charge_not_full_cnt++;
        } else {
            charge_not_full_cnt = 0;
            usr_timer_del(__this->charge_timer);
            __this->charge_timer = 0;
            CHARGE_EDGE_DETECT_EN(1);
            CHARGE_LEVEL_DETECT_EN(1);
        }
    }
}

#define BIT_LDO5V_IN		BIT(0)
#define BIT_LDO5V_OFF		BIT(1)
#define BIT_LDO5V_ERR		BIT(2)

static void ldo5v_detect(void *priv)
{
    /* log_info("%s\n",__func__); */

    static u16 ldo5v_in_normal_cnt = 0;
    static u16 ldo5v_in_err_cnt = 0;
    static u16 ldo5v_off_cnt = 0;

    if (__this->detect_stop) {
        return;
    }

    if (LVCMP_DET_GET()) {	//ldoin > vbat
        /* putchar('X'); */
        if (ldo5v_in_normal_cnt < 50) {
            ldo5v_in_normal_cnt++;
        } else {
            /* printf("ldo5V_IN\n"); */
            set_charge_online_flag(1);
            ldo5v_off_cnt = 0;
            ldo5v_in_err_cnt = 0;
            //消息线程未准备好接收消息,继续扫描
            if (__this->charge_event_flag == 0) {
                return;
            }
            ldo5v_in_normal_cnt = 0;
            usr_timer_del(__this->ldo5v_timer);
            __this->ldo5v_timer = 0;
            if ((charge_flag & BIT_LDO5V_IN) == 0) {
                charge_flag = BIT_LDO5V_IN;
                charge_event_to_user(CHARGE_EVENT_LDO5V_IN);
                LVCMP_EDGE_SEL(1);	//检测ldoin比vbat电压低的情况(充电仓给电池充满后会关断，此时电压会掉下来)
                LDO5V_EDGE_SEL(1);
            }
        }
    } else if (LDO5V_DET_GET() == 0) {	//ldoin<拔出电压（0.6）
        /* putchar('Q'); */
        if (ldo5v_off_cnt < (__this->data->ldo5v_off_filter + 20)) {
            ldo5v_off_cnt++;
        } else {
            /* printf("ldo5V_OFF\n"); */
            set_charge_online_flag(0);
            ldo5v_in_normal_cnt = 0;
            ldo5v_in_err_cnt = 0;
            //消息线程未准备好接收消息,继续扫描
            if (__this->charge_event_flag == 0) {
                return;
            }
            ldo5v_off_cnt = 0;
            usr_timer_del(__this->ldo5v_timer);
            __this->ldo5v_timer = 0;
            if ((charge_flag & BIT_LDO5V_OFF) == 0) {
                charge_flag = BIT_LDO5V_OFF;
                LVCMP_EDGE_SEL(0);//拔出后重新检测插入
                LDO5V_EDGE_SEL(0);
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
            //消息线程未准备好接收消息,继续扫描
            if (__this->charge_event_flag == 0) {
                return;
            }
            ldo5v_in_err_cnt = 0;
            usr_timer_del(__this->ldo5v_timer);
            __this->ldo5v_timer = 0;
            if ((charge_flag & BIT_LDO5V_ERR) == 0) {
                charge_flag = BIT_LDO5V_ERR;
                LVCMP_EDGE_SEL(0);
                LDO5V_EDGE_SEL(1);
                if (__this->data->ldo5v_off_filter) {
                    charge_event_to_user(CHARGE_EVENT_LDO5V_KEEP);
                }
            }
        }
    }
}


void sub_wakeup_isr(void)
{
    /* printf("\sub_wakeup_isr\n"); */
    if (__this->ldo5v_timer == 0) {
        __this->ldo5v_timer = usr_timer_add(0, ldo5v_detect, 2, 1);
    }
}

void charge_wakeup_isr(void)
{
    /* printf("\ncharge_wkup_deal\n"); */
    if (__this->charge_timer == 0) {
        __this->charge_timer = usr_timer_add(0, charge_full_detect, 2, 1);
    }
}

void charge_set_ldo5v_detect_stop(u8 stop)
{
    __this->detect_stop = stop;
}

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
    3869, 3907, 3946, 3985, 4026, 4068, 4122, 4157,
    4202, 4249, 4295, 4350, 4398, 4452, 4509, 4567,
};
u16 get_charge_full_value(void)
{
    ASSERT(__this->init_ok, "charge not init ok!\n");
    ASSERT(__this->data->charge_full_V < CHARGE_FULL_V_MAX);
    return full_table[__this->data->charge_full_V];
}

static void charge_config(void)
{
    u8 charge_4202_trim_val = CHARGE_FULL_V_4202;
    u8 offset = 0;
    u8 charge_full_v_val = 0;

    if (get_vbat_trim() == 0xf) {
        log_info("vbat not trim, use default config!!!!!!");
    } else {
        charge_4202_trim_val = get_vbat_trim();		//4.2V对应的trim出来的实际档位
    }

    log_info("charge_4202_trim_val = %d\n", charge_4202_trim_val);

    if (__this->data->charge_full_V >= CHARGE_FULL_V_4202) {
        offset = __this->data->charge_full_V - CHARGE_FULL_V_4202;
        charge_full_v_val = charge_4202_trim_val + offset;
        if (charge_full_v_val > 0xf) {
            charge_full_v_val = 0xf;
        }
    } else {
        offset = CHARGE_FULL_V_4202 - __this->data->charge_full_V;
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
    CHGBG_EN(0);
    CHARGE_EN(0);

    /*LDO5V的100K下拉电阻使能*/
    L5V_LOAD_EN(__this->data->ldo5v_pulldown_en);

    /*LDO5V,检测上升沿，用于检测ldoin插入*/
    LDO5V_EN(1);
    LDO5V_EDGE_SEL(0);
    LDO5V_PND_CLR();
    LDO5V_EDGE_WKUP_EN(1);

    /*LVCMP,检测上升沿，用于检测ldoin插入*/
    LVCMP_EN(1);
    LVCMP_EDGE_SEL(0);
    LVCMP_PND_CLR();
    LVCMP_EDGE_WKUP_EN(1);
    LVCMP_CMP_SEL(1);

    charge_config();

    if (check_charge_state()) {
        if (__this->ldo5v_timer == 0) {
            __this->ldo5v_timer = usr_timer_add(0, ldo5v_detect, 2, 1);
        }
    } else {
        CHARGE_WKUP_PND_CLR();
        charge_flag = BIT_LDO5V_OFF;
    }
    charge_set_callback(charge_wakeup_isr, sub_wakeup_isr);

    __this->init_ok = 1;

    return 0;
}

int charge_api_init(void *arg)
{
    return charge_init(NULL, arg);
}

const struct device_operations charge_dev_ops = {
    .init  = charge_init,
};
