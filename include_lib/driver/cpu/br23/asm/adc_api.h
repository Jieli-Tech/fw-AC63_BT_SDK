#ifndef __ADC_API_H__
#define __ADC_API_H__


//AD channel define
#define AD_CH_PA1    (0x0)
#define AD_CH_PA5    (0x1)
#define AD_CH_PA6    (0x2)
#define AD_CH_PA10   (0x3)
#define AD_CH_PA12   (0x4)
#define AD_CH_PB1    (0x5)
#define AD_CH_PB3    (0x6)
#define AD_CH_PB4    (0x7)
#define AD_CH_PB8    (0x8)
#define AD_CH_PB10   (0x9)
#define AD_CH_PC4    (0xA)
#define AD_CH_PC6    (0xB)
#define AD_CH_DP     (0xC)
#define AD_CH_PC5    (0xD)
#define AD_CH_RTC    (0xE)          //RTC to ADC

#define AD_CH_MUX    (0xF)          //PLL/PMU/AUDIO/BT/FM  to  ADC

//AD_CH_MUX
#define AD_OF_PMU           ((0x0<<16) | AD_CH_MUX)
#define AD_OF_AUDIO         ((0x1<<16) | AD_CH_MUX)
#define AD_OF_PLL           ((0x2<<16) | AD_CH_MUX)
#define AD_OF_FM            ((0x3<<16) | AD_CH_MUX)
#define AD_OF_BT            ((0x4<<16) | AD_CH_MUX)

//PMU
#define AD_CH_PMU_VBG       ((0x0<<20) | AD_OF_PMU)
#define AD_CH_VDC13         ((0x1<<20) | AD_OF_PMU)
#define AD_CH_SYSVDD        ((0x2<<20) | AD_OF_PMU)
#define AD_CH_DTEMP         ((0x3<<20) | AD_OF_PMU)
#define AD_CH_PROGF         ((0x4<<20) | AD_OF_PMU)
#define AD_CH_VBAT          ((0x5<<20) | AD_OF_PMU)     //1/4vbat
#define AD_CH_LDO5V         ((0x6<<20) | AD_OF_PMU)     //1/4 LDO5V
#define AD_CH_WVDD          ((0x7<<20) | AD_OF_PMU)
//AUDIO
#define AD_CH_F_VOUTL       ((0x0<<20) | AD_OF_AUDIO)
#define AD_CH_F_VOUTR       ((0x1<<20) | AD_OF_AUDIO)
#define AD_CH_MICLDO        ((0x2<<20) | AD_OF_AUDIO)
#define AD_CH_CTADCREF      ((0x3<<20) | AD_OF_AUDIO)
#define AD_CH_DACVDD        ((0x4<<20) | AD_OF_AUDIO)
#define AD_CH_R_VOUTL       ((0x5<<20) | AD_OF_AUDIO)
#define AD_CH_R_VOUTR       ((0x6<<20) | AD_OF_AUDIO)
//PLL
#define AD_CH_BIAS          ((0x0<<20) | AD_OF_PLL)
#define AD_CH_ANALOG        ((0x1<<20) | AD_OF_PLL)
#define AD_CH_DIGITAL       ((0x2<<20) | AD_OF_PLL)
#define AD_CH_480MEG        ((0x3<<20) | AD_OF_PLL)
//FM
#define AD_CH_FMTX_SEL      ((0x0<<20) | AD_OF_FM)
//BT
#define AD_CH_BT_SEL        ((0x0<<20) | AD_OF_BT)

#define AD_CH_LDOREF        AD_CH_PMU_VBG


#define     ADC_MAX_CH  10



extern void adc_init();
extern void adc_vbg_init();
//p33 define

extern void adc_pmu_ch_select(u32 ch);
extern void adc_pmu_detect_en(u32 ch);
extern void adc_vdc13_save();
extern void adc_vdc13_restore();

//
u32 adc_get_value(u32 ch);

u32 adc_add_sample_ch(u32 ch);

u32 adc_remove_sample_ch(u32 ch);

u32 adc_get_voltage(u32 ch);
u32 adc_check_vbat_lowpower();

void check_pmu_voltage(u8 tieup);

extern void adc_enter_occupy_mode(u32 ch);
extern void adc_exit_occupy_mode();
extern u32 adc_occupy_run();
extern u32 adc_get_occupy_value();
u32 adc_sample(u32 ch);
u32 adc_value_to_voltage(u32 adc_vbg, u32 adc_ch_val);

#endif
