
#include "typedef.h"
#include "asm/clock.h"
#include "asm/adc_api.h"
#include "timer.h"
#include "init.h"
#include "asm/efuse.h"
#include "irq.h"
#include "asm/power/p33.h"
#include "asm/power/p11.h"
#include "asm/power_interface.h"
#include "jiffies.h"
#include "app_config.h"
#include "syscfg_id.h"
#include "adc_dtemp_alog.h"


//************************************************************
// dtemp pll trim*********************************************
//************************************************************

#define CHECK_TEMPERATURE_CYCLE             (500)   //
#define BTOSC_TEMPERATURE_THRESHOLD         (10 * 5) //

extern const int config_bt_temperature_pll_trim;

extern void get_bta_pll_midbank_temp(void);   //ok

static u8 dtemp_data_change = 0;


void check_pll_trim_condition(TempSensor *tem, u8 en)
{
    TempSensor *data = get_tempsensor_pivr();
    int vaild = data->valid;
    int stable = data->stable;
    /* printf("vaild:%d,stable:%d\n",vaild,stable); */
    if (vaild && stable) {
        //比较
        if ((data->ref > data->output) && ((data->ref - data->output) > BTOSC_TEMPERATURE_THRESHOLD)) {
            tempsensor_update_ref(data, data->ref - BTOSC_TEMPERATURE_THRESHOLD);
            dtemp_data_change = 1;
        } else if ((data->ref < data->output) && ((data->output - data->ref) > BTOSC_TEMPERATURE_THRESHOLD)) {
            tempsensor_update_ref(data, data->ref + BTOSC_TEMPERATURE_THRESHOLD);
            dtemp_data_change = 1;
        }

    }
    if (dtemp_data_change) {
        dtemp_data_change = 0;
        printf("midbank_trim\n");
        get_bta_pll_midbank_temp();
    }
}

static void pll_trim_timer_handler(void *priv)
{
    if (!config_bt_temperature_pll_trim) {
        return;
    }
    u16 dtemp_voltage = adc_get_voltage(AD_CH_DTEMP);
    tempsensor_process(get_tempsensor_pivr(), dtemp_voltage);

    static s32 pll_bank_offset = 0;
    check_pll_trim_condition(get_tempsensor_pivr(), 1);
}

void temp_pll_trim_init(void)
{
    if (!config_bt_temperature_pll_trim) {
        return;
    }
    tempsensor_init(get_tempsensor_pivr());
    sys_s_hi_timer_add(NULL, pll_trim_timer_handler, CHECK_TEMPERATURE_CYCLE);
}



