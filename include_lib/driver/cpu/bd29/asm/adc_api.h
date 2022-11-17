#ifndef __ADC_API_H__
#define __ADC_API_H__


//AD channel define
#define AD_CH_PA1    (0x0)
#define AD_CH_PA3    (0x1)
#define AD_CH_PA5    (0x2)
#define AD_CH_PA7    (0x3)
#define AD_CH_PA8    (0x4)
#define AD_CH_DP1    (0x5)
#define AD_CH_DM1    (0x6)
#define AD_CH_PB1    (0x7)
#define AD_CH_PB2    (0x8)
#define AD_CH_PB4    (0x9)
#define AD_CH_DP0    (0xA)
#define AD_CH_DM0    (0xB)
#define AD_CH_PB6    (0xC)
#define AD_CH_PMU    (0xD)
#define AD_CH_BT     (0xE)
#define AD_CH_SYS_PLL     (0xF)

//PMU
#define ADC_PMU_CH_VBG       (0x0<<16)
#define ADC_PMU_CH_VDC13     (0x1<<16)
#define ADC_PMU_CH_SYSVDD    (0x2<<16)
#define ADC_PMU_CH_DTEMP     (0x3<<16)
#define ADC_PMU_CH_PROGF     (0x4<<16)
#define ADC_PMU_CH_VBAT      (0x5<<16)     //1/4vbat
#define ADC_PMU_CH_LDO5V     (0x6<<16)     //1/4 LDO5V
#define ADC_PMU_CH_WVDD      (0x7<<16)

#define AD_CH_PMU_VBG   (AD_CH_PMU | ADC_PMU_CH_VBG)
#define AD_CH_VDC13     (AD_CH_PMU | ADC_PMU_CH_VDC13)
#define AD_CH_SYSVDD    (AD_CH_PMU | ADC_PMU_CH_SYSVDD)
#define AD_CH_DTEMP     (AD_CH_PMU | ADC_PMU_CH_DTEMP)
#define AD_CH_PROGF     (AD_CH_PMU | ADC_PMU_CH_PROGF)
#define AD_CH_VBAT      (AD_CH_PMU | ADC_PMU_CH_VBAT)
#define AD_CH_LDO5V     (AD_CH_PMU | ADC_PMU_CH_LDO5V)
#define AD_CH_WVDD      (AD_CH_PMU | ADC_PMU_CH_WVDD)

#define AD_CH_LDOREF    AD_CH_PMU_VBG


#define     ADC_MAX_CH  10


extern void adc_init();
extern void adc_vbg_init();
//p33 define

extern void adc_pmu_ch_select(u32 ch);
extern void adc_pmu_detect_en(u32 ch);

u32 adc_sample(u32 ch);
//
u32 adc_get_value(u32 ch);

u32 adc_add_sample_ch(u32 ch);

u32 adc_remove_sample_ch(u32 ch);

u32 adc_get_voltage(u32 ch);

u32 adc_value_to_voltage(u32 adc_vbg, u32 adc_ch_val);

void adc_set_vbat_vddio_tieup(u8 en);

u8 get_sysvdd_aims_level(u16 aims_mv);

u8 get_vdc13_aims_level(u16 aims_mv);

u8 get_mvddio_aims_level(u16 aims_mv);

u8 get_wvddio_aims_level(u16 aims_mv);

void pmu_voltage_dump(void);

void check_pmu_voltage(u8 tieup);


void set_change_vbg_value_flag(void);
u32 adc_check_vbat_lowpower();


#endif
