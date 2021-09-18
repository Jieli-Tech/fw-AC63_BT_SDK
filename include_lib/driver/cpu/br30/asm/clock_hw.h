#ifndef __CLOCK_HW_H__
#define __CLOCK_HW_H__

#include "typedef.h"

// #define CLOCK_HAL_DEBUG

#ifdef CLOCK_HAL_DEBUG

#define RC_EN(x)
#define TEST_SEL(x)
#define OSC_CLOCK_IN(x)
//for MACRO - OSC_CLOCK_IN
enum {
    OSC_CLOCK_IN_BT_OSC = 0,
    OSC_CLOCK_IN_RTOSC_H,
    OSC_CLOCK_IN_RTOSC_L,
    OSC_CLOCK_IN_PAT,
};

#define MAIN_CLOCK_SEL(x)
//for MACRO - CLOCK_IN
enum {
    MAIN_CLOCK_IN_RC = 0,

    MAIN_CLOCK_IN_BTOSC = 4,
    MAIN_CLOCK_IN_RTOSC_H,
    MAIN_CLOCK_IN_RTOSC_L,
    MAIN_CLOCK_IN_PLL,

    MAIN_CLOCK_IN_PAT, //for tes
};

#define SFR_MODE(x)
enum {
    SFR_CLOCK_IDLE = 0,
    SFR_CLOCK_ALWAYS_ON,
};
#define PB0_CLOCK_OUT(x)
#define PA2_CLOCK_OUT(x)


#define USB_CLOCK_IN(x)
//for MACRO - USB_CLOCK_IN
enum {
    USB_CLOCK_IN_PLL48M = 0,
    USB_CLOCK_IN_DISABLE,
    USB_CLOCK_IN_LSB,
    USB_CLOCK_IN_DISABLE_PAD,
};
#define DAC_CLOCK_IN(x)
//for MACRO - DAC_CLOCK_IN
enum {
    DAC_CLOCK_IN_PLL24M = 0,
    DAC_CLOCK_IN_OSC,
    DAC_CLOCK_IN_LSB,
    DAC_CLOCK_IN_DISABLE_PAD,
};
#define APC_CLOCK_IN(x)
//for MACRO - APC_CLOCK_IN
enum {
    APC_CLOCK_IN_PLL64M = 0,
    APC_CLOCK_IN_PLLAPC,
    APC_CLOCK_IN_LSB,
    APC_CLOCK_IN_DISABLE,
};
#define UART_CLOCK_IN(x)
//for MACRO - UART_CLOCK_IN
enum {
    UART_CLOCK_IN_PLL48M = 0,
    UART_CLOCK_IN_OSC,
    UART_CLOCK_IN_LSB,
    UART_CLOCK_IN_DISABLE,
};
#define BT_CLOCK_IN(x)
//for MACRO - BT_CLOCK_IN
enum {
    BT_CLOCK_IN_PLL64M = 0,
    BT_CLOCK_IN_DISABLE,
    BT_CLOCK_IN_LSB,
    BT_CLOCK_IN_DISABLE_PAD,
};


#define SFC_CLOCK_DELAY(x)


#define PLL_SYS_SEL(x)
//for MACRO - PLL_SYS_SEL
enum {
    PLL_SYS_SEL_PLL192M = 0,
    PLL_SYS_SEL_PLL137M,
    PLL_SYS_SEL_PLL480M,
    PLL_SYS_SEL_DISABLE,
};
#define PLL_SYS_DIV(x)
//for MACRO - PLL_SYS_DIV
enum {
    PLL_SYS_DIV1 = 0,
    PLL_SYS_DIV3,
    PLL_SYS_DIV5,
    PLL_SYS_DIV7,

    PLL_SYS_DIV1X2 = 4,
    PLL_SYS_DIV3X2,
    PLL_SYS_DIV5X2,
    PLL_SYS_DIV7X2,

    PLL_SYS_DIV1X4 = 8,
    PLL_SYS_DIV3X4,
    PLL_SYS_DIV5X4,
    PLL_SYS_DIV7X4,

    PLL_SYS_DIV1X8 = 12,
    PLL_SYS_DIV3X8,
    PLL_SYS_DIV5X8,
    PLL_SYS_DIV7X8,
};

#define PLL_APC_SEL(x)
//for MACRO - PLL_APC_SEL
enum {
    PLL_APC_SEL_PLL192M = 0,
    PLL_APC_SEL_PLL137M,
    PLL_APC_SEL_DISABLE,
};
#define PLL_APC_DIV(x)
//for MACRO - PLL_APC_DIV
enum {
    PLL_APC_DIV1 = 0,
    PLL_APC_DIV3,
    PLL_APC_DIV5,
    PLL_APC_DIV7,

    PLL_APC_DIV1X2 = 4,
    PLL_APC_DIV3X2,
    PLL_APC_DIV5X2,
    PLL_APC_DIV7X2,

    PLL_APC_DIV1X4 = 8,
    PLL_APC_DIV3X4,
    PLL_APC_DIV5X4,
    PLL_APC_DIV7X4,

    PLL_APC_DIV1X8 = 12,
    PLL_APC_DIV3X8,
    PLL_APC_DIV5X8,
    PLL_APC_DIV7X8,
};

#define PLL_ALNK_SEL(x)
//for MACRO - PLL_ALNK_SEL
enum {
    PLL_ALNK_192M_DIV17 = 0,
    PLL_ALNK_480M_DIV39,
};

#define PLL_EN(x)
#define PLL_REST(x)
#define PLL_DIVn(x)
#define PLL_DIVn_EN(x)
#define PLL_REF_SEL(x)
//for MACRO - PLL_RSEL
enum {
    PLL_REF_SEL_BTOSC = 0, 	//bt
    PLL_REF_SEL_RTOSC,		//rt
    PLL_REF_SEL_PAT = 3,
};

#define PLL_TEST(x)
#define PLL_DSMS(x)
#define PLL_DSM_TSEL(x)
#define PLL_DSM_RSEL(x)
#define PLL_DSM_MSEL(x)

#define PLL_DIVSEL(x)

//for MACRO - PLL_RSEL
enum {
    PLL_DIVIDER_INTE = 0,
    PLL_DIVIDER_FRAC,
};

#define PLL_PFD(x)
#define PLL_ICP(x)
#define PLL_LPFR2(x)
#define PLL_LPFR3(x)


#define PLL_RSEL(x)
//for MACRO - PLL_RSEL
enum {
    PLL_RSEL_BTOSC_DIFF = 0,
    PLL_RSEL_RTOSC_DIFF,
    PLL_RSEL_PLL_REF_SEL,
};

#define PLL_DIVm(x)
#define PLL_LD012A(x)
#define PLL_LDO12D(x)
#define PLL_IVCO(x)
#define PLL_LDO_BYPASS(x)
#define PLL_TSSEL(x)
#define PLL_TSOE(x)

#define PLL_CLK480M_OE(x)
#define PLL_CLK320M_OE(x)
#define PLL_CLK192M_OE(x)
#define PLL_CLK137M_OE(x)
#define PLL_CLK107M_OE(x)

#define PLL_FRAC(x)
#define PLL_DMAX(x)
#define PLL_DMIN(x)
#define PLL_DSTP(x)

#define HSB_CLK_DIV(x)
#define LSB_CLK_DIV(x)
#define OTP_CLK_DIV(x)


#else

#define RC_EN(x)                SFR(JL_CLOCK->CLK_CON0,  0,  1,  x)
#define RCH_EN(x)				SFR(JL_CLOCK->CLK_CON0,  1,  1,  x)
//for MACRO - RCH_EN
enum {
    RCH_EN_250K = 0,
    RCH_EN_16M,
};

#define TEST_SEL(x)				SFR(JL_CLOCK->CLK_CON0,  2,  2,  x)
//for MACRO - TS_SEL
enum {
    TS_SEL_IN_MAIN = 0,
    TS_SEL_IN_PAT,
};

#define GET_OSC_CLOCK_IN()		((JL_CLOCK->CLK_CON0 & 0x3<<4)>>4)

#define OSC_CLOCK_IN(x)         SFR(JL_CLOCK->CLK_CON0,  4,  2,  x)
//for MACRO - OSC_CLOCK_IN
enum {
    OSC_CLOCK_IN_BT_OSC = 0,
    OSC_CLOCK_IN_XOSC_FSCK,
    OSC_CLOCK_IN_RTOSC_L,
    OSC_CLOCK_IN_PAT,
};

//#define MAIN_CLOCK_SEL(x)	    SFR(JL_CLOCK->CLK_CON0,  6,  3,  x)
//for MACRO - CLOCK_IN
enum {
    MAIN_CLOCK_IN_RC = 0,

    MAIN_CLOCK_IN_BTOSC = 1,
    MAIN_CLOCK_IN_PLL = 2,
    MAIN_CLOCK_IN_RTOSC_L = 3,

    MAIN_CLOCK_IN_PAT = 4, //for test
};

#define MAIN_CLOCK_SEL(x) 		\
	do { \
		if (x == MAIN_CLOCK_IN_RC) { \
			SFR(JL_CLOCK->CLK_CON0, 1, 1, 1); \
			SFR(JL_CLOCK->CLK_CON0, 7, 2, x); \
		} else if (x == MAIN_CLOCK_IN_RTOSC_L) { \
			SFR(JL_CLOCK->CLK_CON0, 1, 2, 0); \
			SFR(JL_CLOCK->CLK_CON0, 6, 3, 1); \
		} else if (x == MAIN_CLOCK_IN_PAT) { \
			SFR(JL_CLOCK->CLK_CON0, 1, 2, 2); \
			SFR(JL_CLOCK->CLK_CON0, 6, 3, 1); \
		} else { \
			SFR(JL_CLOCK->CLK_CON0, 7, 2, x); \
		} \
	} while (0);

#define SFR_MODE(x)             SFR(JL_CLOCK->CLK_CON0,  9,  1,  x)
enum {
    SFR_CLOCK_IDLE = 0,
    SFR_CLOCK_ALWAYS_ON,
};
#define PA0_CLOCK_OUT(x)        SFR(JL_CLOCK->CLK_CON0,  10,  3,  x)
#define PB8_CLOCK_OUT(x)     	SFR(JL_CLOCK->CLK_CON0,  13,  3,  x)


#define USB_CLOCK_IN(x)         SFR(JL_CLOCK->CLK_CON1,  0,  2,  x)

//for MACRO - USB_CLOCK_IN
enum {
    USB_CLOCK_IN_PLL48M = 0,
    USB_CLOCK_IN_OSC,
    USB_CLOCK_IN_LSB,
    USB_CLOCK_IN_DISABLE,
};
#define AUDIO_CLOCK_IN(x)       SFR(JL_CLOCK->CLK_CON1,  2,  2,  x)
//for MACRO - AUDIO_CLOCK_IN
enum {
    AUDIO_CLOCK_IN_PLL48M = 0,
    AUDIO_CLOCK_IN_OSC,
    AUDIO_CLOCK_IN_LSB,
    AUDIO_CLOCK_IN_DISABLE,
};
#define GPCNT_CLOCK_IN(x)         SFR(JL_CLOCK->CLK_CON1,  4,  3,  x)
//for MACRO - DAC_CLOCK_IN
enum {
    GPCNT_CLOCK_IN_NULL = 0,
    GPCNT_CLOCK_IN_SRC,
    GPCNT_CLOCK_IN_SFC,
    GPCNT_CLOCK_IN_HSB,
    GPCNT_CLOCK_IN_XOSC_FSCK,
    GPCNT_CLOCK_IN_NULL0,
    GPCNT_CLOCK_IN_WL,
    GPCNT_CLOCK_IN_USB,
};

#define UART_CLOCK_IN(x)        SFR(JL_CLOCK->CLK_CON1,  10,  2,  x)
//for MACRO - UART_CLOCK_IN
enum {
    UART_CLOCK_IN_PLL48M = 0,
    UART_CLOCK_IN_OSC,
    UART_CLOCK_IN_LSB,
    UART_CLOCK_IN_DISABLE,
};
#define BT_CLOCK_IN(x)          SFR(JL_CLOCK->CLK_CON1,  14,  2,  x)
//for MACRO - BT_CLOCK_IN
enum {
    BT_CLOCK_IN_PLL48M = 0,
    BT_CLOCK_IN_HSB,
    BT_CLOCK_IN_LSB,
    BT_CLOCK_IN_DISABLE,
};

#define SFC_SCKE(x)             SFR(JL_CLOCK->CLK_CON1,  17,  1,  x)

#define WL2ADC_CLOCK_IN(x)      SFR(JL_CLOCK->CLK_CON1,  18,  2,  x)
//for MACRO - WL2ADC_CLOCK_IN
enum {
    WL2ADC_CLOCK_IN_PLL96M = 0,
    WL2ADC_CLOCK_IN_HSB,
    WL2ADC_CLOCK_IN_LSB,
    WL2ADC_CLOCK_IN_DISABLE,
};

#define WL2DAC_CLOCK_IN(x)      SFR(JL_CLOCK->CLK_CON1,  21,  2,  x)
//for MACRO - WL2DAC_CLOCK_IN
enum {
    WL2DAC_CLOCK_IN_PLL96M = 0,
    WL2DAC_CLOCK_IN_HSB,
    WL2DAC_CLOCK_IN_LSB,
    WL2DAC_CLOCK_IN_DISABLE,
};


#define SFC_CLOCK_DELAY(x)      SFR(JL_CLOCK->CLK_CON1,  28,  2,  x)


#define PLL_EN(x)         		SFR(JL_PLL->PLL_CON0,  0,  1,  x)
#define PLL_REST(x)             SFR(JL_PLL->PLL_CON0,  1,  1,  x)
#define PLL_DIVn(x)          	SFR(JL_PLL->PLL_CON0,  2,  7,  x)
#define PLL_REF_SEL(x)        	SFR(JL_PLL->PLL_CON0,  9,  1,  x)
//for MACRO - PLL_REF_SEL
enum {
    PLL_REF_SEL_BTOSC = 0, 	//btosc
    PLL_REF_SEL_RCLK,
};

#define PLL_TEST(x)         	SFR(JL_PLL->PLL_CON0,  10, 1,  x)

#define PLL_DSMS(x)         	SFR(JL_PLL->PLL_CON0,  11, 1,  x)
#define PLL_TSEL(x)         	SFR(JL_PLL->PLL_CON0,  12, 4,  x)
#define PLL_RSEL(x)         	SFR(JL_PLL->PLL_CON0,  16, 2,  x)
#define PLL_MSEL(x)         	SFR(JL_PLL->PLL_CON0,  18, 2,  x)

#define PLL_DSM_RST(x)          SFR(JL_PLL->PLL_CON0,  20, 1,  x)
#define PLL_MODE(x)             SFR(JL_PLL->PLL_CON0,  21, 1,  x)
#define PLL_PFD(x)              SFR(JL_PLL->PLL_CON0,  22, 2,  x)
#define PLL_ICP(x)              SFR(JL_PLL->PLL_CON0,  24, 3,  x)
#define PLL_LPFR2(x)            SFR(JL_PLL->PLL_CON0,  27, 3,  x)
#define PLL_RCLK_SEL(x)         SFR(JL_PLL->PLL_CON0,  30, 2,  x)
//for MACRO - PLL_REF_CLK_SEL
enum {
    PLL_RCLK_SEL_262K = 0, 	//btosc
    PLL_RCLK_SEL_RC16M,
    PLL_RCLK_SEL_BTOSC,
    PLL_RCLK_SEL_PAT,
};


#define PLL_FBDS(x)             SFR(JL_PLL->PLL_CON1, 0,  12,  x)
#define PLL_IVCO(x)             SFR(JL_PLL->PLL_CON1, 12, 3,  x)
#define PLL_LDO_BYPASS(x)       SFR(JL_PLL->PLL_CON1, 15, 1,  x)
#define PLL_TSSEL(x)            SFR(JL_PLL->PLL_CON1, 16, 2,  x)
#define PLL_TSOE(x)             SFR(JL_PLL->PLL_CON1, 18, 1,  x)

#define PLL_LDO12A(x)           SFR(JL_PLL->PLL_CON1, 20, 3,  x)
#define PLL_LDO12D(x)           SFR(JL_PLL->PLL_CON1, 23, 3,  x)

#define PLL_DIVn_EN(x)         	SFR(JL_PLL->PLL_CON1, 26, 2,  x)
//for MACRO - PLL_DIVn_EN
enum {
    PLL_DIVn_EN_X2 = 0,
    PLL_DIVn_DIS_DIV1,
    PLL_DIVn_EN2_33,
};

#define PLL_CLK_EN(x)         	SFR(JL_PLL->PLL_CON1, 31, 1,  x)

#define PLL_SYS_SEL(x)          SFR(JL_PLL->PLL_CON2,  0,  4,  x)
//for MACRO - PLL_SYS_SEL
enum {
    PLL_SYS_SEL_NULL = 0,
    PLL_SYS_SEL_BT_OSC_X2,
    PLL_SYS_SEL_3DIV2,
    PLL_SYS_SEL_1DIV1,

    PLL_SYS_SEL_9DIV2 = 4,

    PLL_SYS_SEL_7DIV2 = 8,

    PLL_SYS_SEL_5DIV2 = 12,
};

#define PLL_SYS_SEL_GET()       ((JL_PLL->PLL_CON2 & 0x0f))

#define PLL_SYS_DIV(x)          SFR(JL_PLL->PLL_CON2,  4,  4,  x)
//for MACRO - PLL_SYS_DIV
enum {
    PLL_SYS_DIV1 = 0,
    PLL_SYS_DIV3,
    PLL_SYS_DIV5,
    PLL_SYS_DIV7,

    PLL_SYS_DIV1X2 = 4,
    PLL_SYS_DIV3X2,
    PLL_SYS_DIV5X2,
    PLL_SYS_DIV7X2,

    PLL_SYS_DIV1X4 = 8,
    PLL_SYS_DIV3X4,
    PLL_SYS_DIV5X4,
    PLL_SYS_DIV7X4,

    PLL_SYS_DIV1X8 = 12,
    PLL_SYS_DIV3X8,
    PLL_SYS_DIV5X8,
    PLL_SYS_DIV7X8,
};


#define PLL_ALNK0_SEL(x)        SFR(JL_PLL->PLL_CON2, 8, 2,  x)
#define PLL_ALNK0_DIV(x)        SFR(JL_PLL->PLL_CON2, 10, 2,  x)
#define PLL_48M_SEL(x)          SFR(JL_PLL->PLL_CON2, 12, 2,  x)
enum {
    PLL_48M_SEL_DIV2 = 0,
    PLL_48M_SEL_BT_OSC_X2,
    PLL_48M_SEL_DIV1,
    PLL_48M_SEL_BT_OSC_X2_PAD,
};

#define PLL_48M_SEL_GET()       ((JL_PLL->PLL_CON2 & (BIT(12)|BIT(13)))>>12)

#define PLL_96M_SEL(x)          SFR(JL_PLL->PLL_CON2, 14, 4,  x)
enum {
    PLL_96M_SEL_NULL = 0,

    PLL_96M_SEL_7DIV2 = 4,
    PLL_96M_SEL_5DIV2,
    PLL_96M_SEL_3DIV2,
    PLL_96M_SEL_1DIV1,

    PLL_96M_SEL_7DIV4 = 8,
    PLL_96M_SEL_5DIV4,
    PLL_96M_SEL_3DIV4,
    PLL_96M_SEL_1DIV2,

    PLL_96M_SEL_9DIV2 = 12,
};

#define PLL_96M_SEL_GET()       ((JL_PLL->PLL_CON2 & (BIT(14)|BIT(15)|BIT(16)|BIT(17)))>>14)

#define PLL_CLK_1DIV1_OE(x)     SFR(JL_PLL->PLL_CON2, 18, 1,  x)
#define PLL_CLK_3DIV2_OE(x)     SFR(JL_PLL->PLL_CON2, 19, 1,  x)
#define PLL_CLK_5DIV2_OE(x)     SFR(JL_PLL->PLL_CON2, 20, 1,  x)
#define PLL_CLK_7DIV2_OE(x)     SFR(JL_PLL->PLL_CON2, 21, 1,  x)
#define PLL_CLK_9DIV2_OE(x)     SFR(JL_PLL->PLL_CON2, 22, 1,  x)
#define PLL_CLK_DAC_OE(x)       SFR(JL_PLL->PLL_CON2, 23, 1,  x)
#define PLL_DIVS(x)             SFR(JL_PLL->PLL_CON2, 24, 2,  x)


#define HSB_CLK_DIV(x)			SFR(JL_CLOCK->SYS_DIV,  0,  8,  x)
#define LSB_CLK_DIV(x)			SFR(JL_CLOCK->SYS_DIV,  8,  3,  x)
#define SFC_CLK_DIV(x)			SFR(JL_CLOCK->SYS_DIV,  12, 3,  x)

/********************************************************************************/
#define GPCNT_EN(x)             SFR(JL_GPCNT->CON,  0,  1,  x)
#define GPCNT_CSS(x)            SFR(JL_GPCNT->CON,  1,  3,  x)
//for MACRO - GPCNT_CSS
enum {
    GPCNT_CSS_LSB = 0,
    GPCNT_CSS_OSC,
    GPCNT_CSS_INPUT_CH2,    //iomap con1[8:11]
    GPCNT_CSS_INPUT_CH3,    //iomap con1[12:15]
    GPCNT_CSS_CLOCK_IN,
    GPCNT_CSS_RING,
    GPCNT_CSS_PLL,
    GPCNT_CSS_INTPUT_CH1,   //iomap con1[4:7]
};

#define GPCNT_CLR_PEND(x)       SFR(JL_GPCNT->CON,  6,  1,  x)
#define GPCNT_GTS(x)            SFR(JL_GPCNT->CON,  8,  4,  x)

#define GPCNT_GSS(x)            SFR(JL_GPCNT->CON,  12, 3,  x)
//for MACRO - GPCNT_CSS
enum {
    GPCNT_GSS_LSB = 0,
    GPCNT_GSS_OSC,
    GPCNT_GSS_INPUT_CH2,    //iomap con1[8:11]
    GPCNT_GSS_INPUT_CH3,    //iomap con1[12:15]
    GPCNT_GSS_CLOCK_IN,
    GPCNT_GSS_RING,
    GPCNT_GSS_PLL480M,
    GPCNT_GSS_INTPUT_CH1,   //iomap con1[4:7]
};


#define lrc_con1_init                                 \
    /*                               */    ((0 << 7) |\
    /*                               */     (0 << 6) |\
    /*                               */     (0 << 5) |\
    /*RC32K_CAP_S2_33v               */     (1 << 4) |\
    /*RC32K_CAP_S1_33v               */     (0 << 3) |\
    /*RC32K_CAP_S0_33v               */     (0 << 2) |\
    /*                               */     (0 << 1) |\
    /*RC32K_RNPS_S1_33v              */     (1 << 0))

#define lrc_con0_init                                 \
    /*RC32K_RNPS_S0_33v              */    ((0 << 7) |\
    /*                               */     (0 << 6) |\
    /*RC32K_RPPS_S1_33v              */     (1 << 5) |\
    /*RC32K_RPPS_S0_33v              */     (0 << 4) |\
    /*                               */     (0 << 3) |\
    /*                               */     (0 << 2) |\
    /*RC32K_RN_TRIM_33v              */     (0 << 1) |\
    /*RC32K_EN_33v                   */     (1 << 0))


#define lrc_pll_con2                                     \
    /*reserved                      5 bit*/    ((   0b00000 << 27 ) |\
    /*SYSPLL_DIVS_BTADC(2-0)        3 bit*/     (     0b000 << 24 ) |\
    /*SYSPLL_CKOE_BTADC             1 bit*/     (         0 << 23 ) |\
    /*SYSPLL_CKOUT_D4P5_OE          1 bit*/     (         1 << 22 ) |\
    /*SYSPLL_CKOUT_D3P5_OE          1 bit*/     (         1 << 21 ) |\
    /*SYSPLL_CKOUT_D2P5_OE          1 bit*/     (         1 << 20 ) |\
    /*SYSPLL_CKOUT_D1P5_OE          1 bit*/     (         1 << 19 ) |\
    /*SYSPLL_CKOUT_D1_OE            1 bit*/     (         1 << 18 ) |\
    /*SYSPLL_96M_DIV(1-0)           2 bit*/     (      0b10 << 16 ) |\
    /*SYSPLL_96M_SEL(1-0)           2 bit*/     (      0b11 << 14 ) |\
    /*SYSPLL_48M_MUX                1 bit*/     (         0 << 13 ) |\
    /*SYSPLL_48M_SEL                1 bit*/     (         0 << 12 ) |\
    /*SYSPLL_ALNK0_DIV(1-0)         2 bit*/     (      0b11 << 10 ) |\
    /*SYSPLL_ALNK0_SEL(1-0)         2 bit*/     (      0b11 <<  8 ) |\
    /*SYSPLL_SYS_DIV(3-0)           4 bit*/     (    0b1000 <<  4 ) |\
    /*SYSPLL_SYS_SEL(3-0)           4 bit*/     (      0b11 <<  0 ))

#define lrc_pll_con1                                     \
    /*SYSPLL_CKE                    1 bit*/    ((         1 << 31 ) |\
    /*reserved                      1 bit*/     (         0 << 30 ) |\
    /*SYSPLL_REFMOD(1-0)            2 bit*/     (      0b00 << 28 ) |\
    /*SYSPLL_REFDSEN(1-0)           2 bit*/     (      0b01 << 26 ) |\
    /*SYSPLL_LDO12D_S(2-0)          3 bit*/     (     0b101 << 23 ) |\
    /*SYSPLL_LDO12A_S(2-0)          3 bit*/     (     0b101 << 20 ) |\
    /*reserved                      1 bit*/     (         0 << 19 ) |\
    /*SYSPLL_TEST_EN                1 bit*/     (         0 << 18 ) |\
    /*SYSPLL_TEST_S(1-0)            2 bit*/     (      0b00 << 16 ) |\
    /*SYSPLL_LDO_BYPASS             1 bit*/     (         0 << 15 ) |\
    /*SYSPLL_IVCOS(2-0)             3 bit*/     (     0b011 << 12 ) |\
    /*SYSPLL_DS(11-0)              12 bit*/     (((192000000/(32000*8))-2)<< 0))

#define lrc_pll_con0                                     \
    /*ref_sel                       2 bit*/    ((      0b00 << 30 ) |\
    /*SYSPLL_LPFR2S(2-0)            3 bit*/     (     0b111 << 27 ) |\
    /*SYSPLL_ICPS(2-0)              3 bit*/     (     0b000 << 24 ) |\
    /*SYSPLL_PFDS(1-0)              2 bit*/     (      0b01 << 22 ) |\
    /*SYSPLL_MODE                   1 bit*/     (         0 << 21 ) |\
    /*RST                           1 bit*/     (         0 << 20 ) |\
    /*SYSPLL_MSEL(1-0)              2 bit*/     (      0b00 << 18 ) |\
    /*SYSPLL_RSEL(1-0)              2 bit*/     (      0b00 << 16 ) |\
    /*SYSPLL_TSEL(3-0)              4 bit*/     (    0b0000 << 12 ) |\
    /*SYSPLL_DSMS                   1 bit*/     (         0 << 11 ) |\
    /*SYSPLL_TSCK480M_OE            1 bit*/     (         0 << 10 ) |\
    /*SYSPLL_REFSEL                 1 bit*/     (         1 << 9  ) |\
    /*SYSPLL_REFDS(6-0)             7 bit*/     (         0 << 2  ) |\
    /*SYSPLL_RN                     1 bit*/     (         0 << 1  ) |\
    /*SYSPLL_EN                     1 bit*/     (         0 << 0  ))

#endif
#endif
