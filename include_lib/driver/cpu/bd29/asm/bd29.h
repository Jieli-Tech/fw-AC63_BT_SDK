
#ifndef __BD29__
#define __BD29__

//===============================================================================//
//
//      sfr define
//
//===============================================================================//

#define hs_base   0x70000
#define ls_base   0x60000

#define __RW      volatile       // read write
#define __RO      volatile const // only read
#define __WO      volatile       // only write

#define __u8      unsigned int   // u8  to u32 special for struct
#define __u16     unsigned int   // u16 to u32 special for struct
#define __u32     unsigned int

#define map_adr(grp, adr)  ((64 * grp + adr) * 4)     // grp(0x0-0xff), adr(0x0-0x3f)

//===============================================================================//
//
//      high speed sfr address define
//
//===============================================================================//

//............. 0x0000 - 0x01ff............ for cpu
#define CSFR_BASE0                    (*(__RW __u32 *)(hs_base + map_adr(0x00, 0x00)))
#define CSFR_BASE1                    (*(__RW __u32 *)(hs_base + map_adr(0x01, 0x00)))


//............. 0x0200 - 0x02ff............ for sfc
typedef struct {
    __RW __u32 CON;
    __WO __u32 BAUD;
    __WO __u32 CODE;
    __WO __u32 BASE_ADR;
    __WO __u32 QUCNT;
} JL_SFC_TypeDef;

#define JL_SFC_BASE                     (hs_base + map_adr(0x02, 0x00))
#define JL_SFC                          ((JL_SFC_TypeDef    *)JL_SFC_BASE)

//............. 0x0300 - 0x03ff............ for sfc encrypt
typedef struct {
    __RW __u8  CON;
    __RW __u16 KEY;
    __WO __u32 UNENC_ADRH;
    __WO __u32 UNENC_ADRL;
} JL_SFCENC_TypeDef;

#define JL_SFCENC_BASE                (hs_base + map_adr(0x03, 0x00))
#define JL_SFCENC                     ((JL_SFCENC_TypeDef *)JL_SFCENC_BASE)


//............. 0x0800 - 0x08ff............ for OTP
typedef struct {
    __RW __u32 MASK_CON   ;
    __RW __u32 UNMASK_ADRH;
    __RW __u32 UNMASK_ADRL;
    __RW __u32 OPEN_CNT   ;
    __RW __u32 CLOSE_CNT  ;
    __RW __u32 PWR        ;
} JL_OTP_TypeDef;

#define JL_OTP_BASE                     (hs_base + map_adr(0x08, 0x00))
#define JL_OTP                          ((JL_OTP_TypeDef  *)JL_OTP_BASE)

//............. 0x1100 - 0x11ff............ for debug
typedef struct {
    __RW __u32 CON;
    __RW __u32 KEY;
} JL_SDTAP_TypeDef;

#define JL_SDTAP_BASE                   (hs_base + map_adr(0x11, 0x00))
#define JL_SDTAP                        ((JL_SDTAP_TypeDef  *)JL_SDTAP_BASE)

//............. 0x1200 - 0x12ff............ for mem
typedef struct {
    __RW __u32 CON;
    __RW __u32 PAGE_END;     //page number - 1
} JL_MEM_TypeDef;

#define JL_MEM_BASE                    (hs_base + map_adr(0x12, 0x00))
#define JL_MEM                         ((JL_MEM_TypeDef *)JL_MEM_BASE)


//............. 0x1300 - 0x13ff............ for reserved

//............. 0x1400 - 0x14ff............ for fft
typedef struct {
    __RW __u32 CON;
    __RW __u32 CADR;
    __RW __u32 TEST0;
    __RW __u32 TEST1;
} JL_FFT_TypeDef;

#define JL_FFT_BASE               (hs_base + map_adr(0x14, 0x00))

#define JL_FFT                    ((JL_FFT_TypeDef			*)JL_FFT_BASE)

//............. 0x1500 - 0x15ff............ for eq
typedef struct {
    __RW __u32  CON;
    __RW __u32  DATAI_ADR;
    __RW __u32  DATAO_ADR;
    __RW __u32  DATA_LEN;
    __RW __u32  GLB_GAINL;
    __RW __u32  GLB_GAINR;

} JL_EQ_TypeDef;

#define JL_EQ_BASE               (hs_base + map_adr(0x15, 0x00))

#define JL_EQ                    ((JL_EQ_TypeDef			*)JL_EQ_BASE)

//............. 0x1600 - 0x16ff............ for src
typedef struct {
    __RW __u32 CON0;
    __RW __u32 CON1;
    __RW __u32 CON2;
    __RW __u32 CON3;
    __RW __u32 IDAT_ADR;
    __RW __u32 IDAT_LEN;
    __RW __u32 ODAT_ADR;
    __RW __u32 ODAT_LEN;
    __RW __u32 FLTB_ADR;
} JL_SRC_TypeDef;

#define JL_SRC_BASE               (hs_base + map_adr(0x16, 0x00))

#define JL_SRC                    ((JL_SRC_TypeDef			*)JL_SRC_BASE)

//............. 0x1700 - 0x17ff............ for fm
typedef struct {
    __RW __u32 CON;
    __RW __u32 BASE;
    __RW __u32 ADC_CON;
    __RW __u32 HF_CON0;
    __RW __u32 HF_CON1;
    __RW __u32 HBT_RSSI;
    __RW __u32 ADCI_RSSI;
    __RW __u32 ADCQ_RSSI;
    __RW __u32 HF_CRAM;
    __RW __u32 HF_DRAM;
    __RW __u32 LF_CON;
    __RW __u32 LF_RES;
} JL_FMRX_TypeDef;

#define JL_FMRX_BASE               (hs_base + map_adr(0x17, 0x00))

#define JL_FMRX                    ((JL_FMRX_TypeDef			*)JL_FMRX_BASE)


//............. 0x1800 - 0x18ff............ for wireless
typedef struct {
    __RW __u32 CON0;
    __RW __u32 CON3;
    __RW __u32 LOFC_CON;
    __RW __u32 LOFC_RES;
} JL_WL_TypeDef;

#define JL_WL_BASE                (hs_base + map_adr(0x18, 0x00))
#define JL_WL                     ((JL_WL_TypeDef     *)JL_WL_BASE)

//............. 0x3000 - 0x31ff............ for debug
typedef struct {
    __RW __u32 CON;
} JL_DSP_TypeDef;

#define JL_DSP_BASE               (hs_base + map_adr(0x30, 0x00))

#define JL_DSP                    ((JL_DSP_TypeDef			*)JL_DSP_BASE)

typedef struct {
    __RW __u32 DSP_BF_CON;
    __RW __u32 WR_EN;
    __RO __u32 MSG;
    __WO __u32 MSG_CLR;
    __RW __u32 RESERVED0; // DSP_PC_LIMH;
    __RW __u32 RESERVED1; // DSP_PC_LIML;
    __RW __u32 DSP_EX_LIMH;
    __RW __u32 DSP_EX_LIML;
    __RW __u32 PRP_EX_LIMH;
    __RW __u32 PRP_EX_LIML;
    __RO __u32 PRP_MMU_MSG;
    __RO __u32 LSB_MMU_MSG_CH;
    __RO __u32 PRP_WR_LIMIT_MSG;
    __RO __u32 LSB_WR_LIMIT_CH;
    __RW __u32 RING_OSC;
    __RW __u32 DSP_PC_LIMH0;
    __RW __u32 DSP_PC_LIML0;
    __RW __u32 DSP_PC_LIMH1;
    __RW __u32 DSP_PC_LIML1;
} JL_DEBUG_TypeDef;

#define JL_DEBUG_BASE               (hs_base + map_adr(0x30, 0x01))

#define JL_DEBUG                    ((JL_DEBUG_TypeDef			*)JL_DEBUG_BASE)



//===============================================================================//
//
//      low speed sfr address define
//
//===============================================================================//

//............. 0x0000 - 0x00ff............ for syscfg
typedef struct {
    __RW __u32 PWR_CON;
    __RW __u32 HTC_CON;
    __RW __u32 SYS_DIV;
    __RW __u32 CLK_CON0;
    __RW __u32 CLK_CON1;
    __RW __u32 CLK_CON2;
    __RW __u32 CLK_CON3;
    __u32 RESERVED0[0x10 - 0x6 - 1];
    __RW __u32 PLL_CON;
    __RW __u32 PLL_CON1;
    __RW __u32 RST_SRC;
} JL_CLOCK_TypeDef;

#define JL_CLOCK_BASE                   (ls_base + map_adr(0x00, 0x00))
#define JL_CLOCK                        ((JL_CLOCK_TypeDef      *)JL_CLOCK_BASE)

typedef struct {
    __RW __u32 SRC;
} JL_RST_TypeDef;

#define JL_RST_BASE                  (ls_base + map_adr(0x00, 0x30))
#define JL_RST                       ((JL_RST_TypeDef     *)JL_RST_BASE)

//............. 0x0100 - 0x01ff............
typedef struct {
    __RW __u32 MODE_CON;
} JL_MODE_TypeDef;

#define JL_MODE_BASE                  (ls_base + map_adr(0x01, 0x00))
#define JL_MODE                       ((JL_MODE_TypeDef     *)JL_MODE_BASE)

//............. 0x0200 - 0x02ff............
typedef struct {
    __WO __u32 CHIP_ID;
    __RW __u32 MBIST_CON;
} JL_SYSTEM_TypeDef;

#define JL_SYSTEM_BASE                  (ls_base + map_adr(0x02, 0x00))
#define JL_SYSTEM                       ((JL_SYSTEM_TypeDef     *)JL_SYSTEM_BASE)

//OTP BASE CONTROL
typedef struct {
    __RW __u32 CMD0;
    __RW __u32 CMD1;
    __WO __u32 CMD2;
    __WO __u32 WDAT0;
    __WO __u32 WDAT1;
} JL_OTPC_TypeDef;

#define JL_OTPC_BASE                    (ls_base + map_adr(0x02, 0x04))
#define JL_OTPC                         ((JL_OTPC_TypeDef     *)JL_OTPC_BASE)

//............. 0x0300 - 0x03ff............

//............. 0x0400 - 0x07ff............
typedef struct {
    __RW __u32 CON;
    __RW __u32 CNT;
    __RW __u32 PRD;
    __RW __u32 PWM;
} JL_TIMER_TypeDef;

#define JL_TIMER0_BASE                  (ls_base + map_adr(0x04, 0x00))
#define JL_TIMER0                       ((JL_TIMER_TypeDef     *)JL_TIMER0_BASE)

#define JL_TIMER1_BASE                  (ls_base + map_adr(0x05, 0x00))
#define JL_TIMER1                       ((JL_TIMER_TypeDef     *)JL_TIMER1_BASE)

#define JL_TIMER2_BASE                  (ls_base + map_adr(0x06, 0x00))
#define JL_TIMER2                       ((JL_TIMER_TypeDef     *)JL_TIMER2_BASE)

#define JL_TIMER3_BASE                  (ls_base + map_adr(0x07, 0x00))
#define JL_TIMER3                       ((JL_TIMER_TypeDef     *)JL_TIMER3_BASE)


//............. 0x1000 - 0x10ff............
typedef struct {
    __RW __u8 CON;
    __RW __u8 VAL;
} JL_PCNT_TypeDef;

#define JL_PCNT_BASE                    (ls_base + map_adr(0x10, 0x00))
#define JL_PCNT                         ((JL_PCNT_TypeDef       *)JL_PCNT_BASE)

//............. 0x1100 - 0x11ff............
typedef struct {
    __RW __u8 CON;
    __RW __u8 NUM;
} JL_GPCNT_TypeDef;

#define JL_GPCNT_BASE                   (ls_base + map_adr(0x11, 0x00))
#define JL_GPCNT                        ((JL_GPCNT_TypeDef     *)JL_GPCNT_BASE)


//............. 0x1400 - 0x16ff............
typedef struct {
    __RW __u32 CON0;
    __RW __u32 CON1;
    __RW __u32 CON2;
    __WO __u32 CPTR;
    __WO __u32 DPTR;
    __RW __u32 CTU_CON;
    __WO __u32 CTU_CNT;
} JL_SD_TypeDef;

#define JL_SD0_BASE                     (ls_base + map_adr(0x14, 0x00))
#define JL_SD0                          ((JL_SD_TypeDef        *)JL_SD0_BASE)

#define JL_SD1_BASE                     (ls_base + map_adr(0x15, 0x00))
#define JL_SD1                          ((JL_SD_TypeDef        *)JL_SD1_BASE)

//............. 0x1700 - 0x18ff............
typedef struct {
    __RW __u32 CON0;
    __RW __u32 CON1;
    __WO __u32 EP0_CNT;
    __WO __u32 EP1_CNT;
    __WO __u32 EP2_CNT;
    __WO __u32 EP3_CNT;
    __WO __u32 EP4_CNT;
    __WO __u32 EP0_ADR;
    __WO __u32 EP1_TADR;
    __WO __u32 EP1_RADR;
    __WO __u32 EP2_TADR;
    __WO __u32 EP2_RADR;
    __WO __u32 EP3_TADR;
    __WO __u32 EP3_RADR;
    __WO __u32 EP4_TADR;
    __WO __u32 EP4_RADR;
} JL_USB_TypeDef;

#define JL_USB_BASE                     (ls_base + map_adr(0x18, 0x00))
#define JL_USB                          ((JL_USB_TypeDef       *)JL_USB_BASE)

#define JL_USB1_BASE                    (ls_base + map_adr(0x17, 0x00))
#define JL_USB1                         ((JL_USB_TypeDef       *)JL_USB1_BASE)
//............. 0x1900 - 0x19ff............
typedef struct {
    __RW __u32 WLA_CON0 ;
    __RW __u32 WLA_CON1 ;
    __RW __u32 WLA_CON2 ;
    __RW __u32 WLA_CON3 ;
    __RW __u32 WLA_CON4 ;
    __RW __u32 WLA_CON5 ;
    __RW __u32 WLA_CON6 ;
    __RW __u32 WLA_CON7 ;
    __RW __u32 WLA_CON8 ;
    __RW __u32 WLA_CON9 ;
    __RW __u32 WLA_CON10;
    __RW __u32 WLA_CON11;
    __RW __u32 WLA_CON12;
    __RW __u32 WLA_CON13;
    __RW __u32 WLA_CON14;
    __RW __u32 WLA_CON15;
    __RW __u32 WLA_CON16;
    __RW __u32 WLA_CON17;
    __RW __u32 WLA_CON18;
    __RW __u32 WLA_CON19;
    __RW __u32 WLA_CON20;
    __RW __u32 WLA_CON21;
    __RW __u32 WLA_CON22;
    __RW __u32 WLA_CON23;
    __RW __u32 WLA_CON24;
    __RW __u32 WLA_CON25;
    __RW __u32 WLA_CON26;
    __RW __u32 WLA_CON27;
    __RW __u32 WLA_CON28;
    __RW __u32 WLA_CON29;
    __RO __u32 WLA_CON30;
    __RO __u32 WLA_CON31;
    __RO __u32 WLA_CON32;
    __RO __u32 WLA_CON33;
    __RO __u32 WLA_CON34;
    __RO __u32 WLA_CON35;
    __RO __u32 WLA_CON36;
    __RO __u32 WLA_CON37;
    __RO __u32 WLA_CON38;
    __RO __u32 WLA_CON39;
    __RO __u32 RESERVED0[0x30 - 0x27 - 1];
    __RW __u32 DAA_CON0;
    __RW __u32 DAA_CON1;
    __RW __u32 DAA_CON2;
    __RW __u32 DAA_CON3;
    __RO __u32 RESERVED1[0x37 - 0x33 - 1];
    __RW __u32 DAA_CON7;
    __RW __u32 ADA_CON0;
    __RW __u32 ADA_CON1;
    __RW __u32 ADA_CON2;
} JL_ANA_TypeDef;

#define JL_ANA_BASE                     (ls_base + map_adr(0x19, 0x00))
#define JL_ANA                          ((JL_ANA_TypeDef       *)JL_ANA_BASE)

//............. 0x1c00 - 0x1eff............
typedef struct {
    __RW __u32 CON;
    __WO __u32 BAUD;
    __RW __u32 BUF;
    __WO __u32 ADR;
    __WO __u32 CNT;
} JL_SPI_TypeDef;

#define JL_SPI0_BASE                    (ls_base + map_adr(0x1c, 0x00))
#define JL_SPI0                         ((JL_SPI_TypeDef      *)JL_SPI0_BASE)

#define JL_SPI1_BASE                    (ls_base + map_adr(0x1d, 0x00))
#define JL_SPI1                         ((JL_SPI_TypeDef      *)JL_SPI1_BASE)

#define JL_SPI2_BASE                    (ls_base + map_adr(0x1e, 0x00))
#define JL_SPI2                         ((JL_SPI_TypeDef      *)JL_SPI2_BASE)

//............. 0x2000 - 0x22ff............
typedef struct {
    __RW __u16 CON0;
    __RW __u16 CON1;
    __WO __u16 BAUD;
    __RW __u8  BUF;
    __RW __u32 OTCNT;
    __RW __u32 TXADR;
    __RW __u16 TXCNT;
    __RW __u32 RXSADR;
    __RW __u32 RXEADR;
    __RW __u32 RXCNT;
    __RO __u16 HRXCNT;
} JL_UART_TypeDef;

#define JL_UART0_BASE                   (ls_base + map_adr(0x20, 0x00))
#define JL_UART0                        ((JL_UART_TypeDef       *)JL_UART0_BASE)

#define JL_UART1_BASE                   (ls_base + map_adr(0x21, 0x00))
#define JL_UART1                        ((JL_UART_TypeDef       *)JL_UART1_BASE)

#define JL_UART2_BASE                   (ls_base + map_adr(0x22, 0x00))
#define JL_UART2                        ((JL_UART_TypeDef       *)JL_UART2_BASE)

//............. 0x2400 - 0x24ff............
typedef struct {
    __RW __u16 CON0;
    __RW __u8 BUF;
    __WO __u8 BAUD;
    __RW __u16 CON1;
} JL_IIC_TypeDef;

#define JL_IIC_BASE                     (ls_base + map_adr(0x24, 0x00))
#define JL_IIC                          ((JL_IIC_TypeDef       *)JL_IIC_BASE)

//............. 0x2800 - 0x28ff............
//typedef struct {
//    __RW __u32 CON;
//    __WO __u32 DAT0;
//    __WO __u32 DAT1;
//    __RW __u32 BUF;
//    __WO __u32 ADR;
//    __WO __u32 CNT;
//} JL_PAP_TypeDef;
//
//#define JL_PAP_BASE                     (ls_base + map_adr(0x28, 0x00))
//#define JL_PAP                          ((JL_PAP_TypeDef       *)JL_PAP_BASE)


//............. 0x2900 - 0x2bff............
typedef struct {
    __RW __u8 CON;
    __RW __u8 DAT;
} JL_RDEC_TypeDef;

#define JL_RDEC_BASE                    (ls_base + map_adr(0x29, 0x00))
#define JL_RDEC                         ((JL_RDEC_TypeDef       *)JL_RDEC_BASE)

#define JL_RDEC1_BASE                    (ls_base + map_adr(0x2a, 0x00))
#define JL_RDEC1                         ((JL_RDEC_TypeDef       *)JL_RDEC1_BASE)

#define JL_RDEC2_BASE                    (ls_base + map_adr(0x2b, 0x00))
#define JL_RDEC2                         ((JL_RDEC_TypeDef       *)JL_RDEC2_BASE)

//............. 0x2c00 - 0x2cff............
//typedef struct {
//    __RW __u32 CON;
//    __RO __u32 SR_CNT;
//    __RW __u32 IO_CON;
//    __RW __u32 DMA_CON;
//    __RW __u32 DMA_LEN;
//    __WO __u32 DAT_ADR;
//    __WO __u32 INF_ADR;
//    __RO __u32 CSB0;
//    __RO __u32 CSB1;
//    __RO __u32 CSB2;
//    __RO __u32 CSB3;
//    __RO __u32 CSB4;
//    __RO __u32 CSB5;
//} JL_SS_TypeDef;
//
//#define JL_SS_BASE                      (ls_base + map_adr(0x2c, 0x00))
//#define JL_SS                           ((JL_SS_TypeDef        *)JL_SS_BASE)


//............. 0x3000 - 0x30ff............
typedef struct {
    __RW __u32 TMR0_CON;
    __RW __u32 TMR0_CNT;
    __RW __u32 TMR0_PR;
    __RW __u32 TMR1_CON;
    __RW __u32 TMR1_CNT;
    __RW __u32 TMR1_PR;
    __RW __u32 TMR2_CON;
    __RW __u32 TMR2_CNT;
    __RW __u32 TMR2_PR;
    __RW __u32 TMR3_CON;
    __RW __u32 TMR3_CNT;
    __RW __u32 TMR3_PR;
    __RW __u32 TMR4_CON;
    __RW __u32 TMR4_CNT;
    __RW __u32 TMR4_PR;
    __RW __u32 TMR5_CON;
    __RW __u32 TMR5_CNT;
    __RW __u32 TMR5_PR;
    __RW __u32 TMR6_CON;
    __RW __u32 TMR6_CNT;
    __RW __u32 TMR6_PR;
    __RW __u32 TMR7_CON;
    __RW __u32 TMR7_CNT;
    __RW __u32 TMR7_PR;
    __RW __u32 FPIN_CON;
    __RW __u32 CH0_CON0;
    __RW __u32 CH0_CON1;
    __RW __u32 CH0_CMPH;
    __RW __u32 CH0_CMPL;
    __RW __u32 CH1_CON0;
    __RW __u32 CH1_CON1;
    __RW __u32 CH1_CMPH;
    __RW __u32 CH1_CMPL;
    __RW __u32 CH2_CON0;
    __RW __u32 CH2_CON1;
    __RW __u32 CH2_CMPH;
    __RW __u32 CH2_CMPL;
    __RW __u32 CH3_CON0;
    __RW __u32 CH3_CON1;
    __RW __u32 CH3_CMPH;
    __RW __u32 CH3_CMPL;
    __RW __u32 CH4_CON0;
    __RW __u32 CH4_CON1;
    __RW __u32 CH4_CMPH;
    __RW __u32 CH4_CMPL;
    __RW __u32 CH5_CON0;
    __RW __u32 CH5_CON1;
    __RW __u32 CH5_CMPH;
    __RW __u32 CH5_CMPL;
    __RW __u32 CH6_CON0;
    __RW __u32 CH6_CON1;
    __RW __u32 CH6_CMPH;
    __RW __u32 CH6_CMPL;
    __RW __u32 CH7_CON0;
    __RW __u32 CH7_CON1;
    __RW __u32 CH7_CMPH;
    __RW __u32 CH7_CMPL;
    __RW __u32 MCPWM_CON0;
} JL_MCPWM_TypeDef;

#define JL_MCPWM_BASE                   (ls_base + map_adr(0x30, 0x00))
#define JL_MCPWM                        ((JL_MCPWM_TypeDef  *)JL_MCPWM_BASE)

//............. 0x3100 - 0x31ff............
typedef struct {
    __RW __u32 CON;
    __RO __u32 RES;
} JL_ADC_TypeDef;

#define JL_ADC_BASE                     (ls_base + map_adr(0x31, 0x00))
#define JL_ADC                          ((JL_ADC_TypeDef       *)JL_ADC_BASE)

//............. 0x3200 - 0x32ff............
typedef struct {
    __RW __u32 RFLT_CON;
} JL_IR_TypeDef;

#define JL_IR_BASE                      (ls_base + map_adr(0x32, 0x00))
#define JL_IR                           ((JL_IR_TypeDef         *)JL_IR_BASE)

//............. 0x3300 - 0x33ff............

//............. 0x3400 - 0x34ff............
typedef struct {
    __RW __u32 CON;
} JL_OSA_TypeDef;

#define JL_OSA_BASE                     (ls_base + map_adr(0x34, 0x00))
#define JL_OSA                          ((JL_OSA_TypeDef          *)JL_OSA_BASE)

//............. 0x3500 - 0x35ff............
typedef struct {
    __WO __u32 FIFO;
    __RW __u32 REG;
} JL_CRC_TypeDef;

#define JL_CRC_BASE                     (ls_base + map_adr(0x35, 0x00))
#define JL_CRC                          ((JL_CRC_TypeDef       *)JL_CRC_BASE)


//............. 0x3600 - 0x36ff............
typedef struct {
    __WO __u32 CON;
    __RW __u32 NUM;
} JL_LRCT_TypeDef;

#define JL_LRCT_BASE                    (ls_base + map_adr(0x36, 0x00))
#define JL_LRCT                         ((JL_LRCT_TypeDef     *)JL_LRCT_BASE)

//............. 0x3700 - 0x37ff............
typedef struct {
    __WO __u32 CON;
    __RO __u32 RESERVED[8 - 0 - 1];
    __WO __u32 ME;
} JL_EFUSE_TypeDef;

#define JL_EFUSE_BASE                   (ls_base + map_adr(0x37, 0x00))
#define JL_EFUSE                        ((JL_EFUSE_TypeDef     *)JL_EFUSE_BASE)

//............. 0x3b00 - 0x3bff............
typedef struct {
    __RO __u32 R64L;
    __RO __u32 R64H;
} JL_RAND_TypeDef;

#define JL_RAND_BASE                    (ls_base + map_adr(0x3b, 0x00))
#define JL_RAND                         ((JL_RAND_TypeDef   *)JL_RAND_BASE)

//............. 0x3c00 - 0x3cff............
typedef struct {
    __RW __u32 CON0;
    __RW __u32 CON1;
    __RO __u32 RES;
} JL_CTM_TypeDef;

#define JL_CTM_BASE                     (ls_base + map_adr(0x3c, 0x00))
#define JL_CTM                          ((JL_CTM_TypeDef    *)JL_CTM_BASE)


//............. 0x3e00 - 0x3eff............ for p33
typedef struct {
    __RW __u32 PMU_CON;
    __RW __u32 RTC_CON;
    __RW __u32 SPI_CON;
    __RW __u32 SPI_DAT;
} JL_P33_TypeDef;

#define JL_P33_BASE                 (ls_base + map_adr(0x3e, 0x00))
#define JL_P33                      ((JL_P33_TypeDef        *)JL_P33_BASE)

//............. 0x3f00 - 0x3fff............ for dma
typedef struct {
    __RW __u32 PRI0;
    __RW __u32 PRI1;
    __RW __u32 PRI2;
    __RW __u32 PRI3;
    __u32 RESERVED0[0x08 - 0x03 - 1];
    __RW __u32 MSG;
    __RO __u32 MSG_CH;
    __RW __u32 RDL;
    __RW __u32 RDH;
    __RW __u32 WRL;
    __RW __u32 WRH;

} JL_DMA_TypeDef;

#define JL_DMA_BASE                 (ls_base + map_adr(0x3f, 0x00))
#define JL_DMA                      ((JL_DMA_TypeDef        *)JL_DMA_BASE)

//............. 0x4000 - 0x40ff............

//............. 0x4100 - 0x41ff............ for lsb peri(spi0/sd0)
typedef struct {
    __RW __u8  ENCCON ;
    __WO __u16 ENCKEY ;
    __WO __u16 ENCADR ;
} JL_PERIENC_TypeDef;

#define JL_PERIENC_BASE             (ls_base + map_adr(0x41, 0x00))
#define JL_PERIENC                  ((JL_PERIENC_TypeDef *)JL_PERIENC_BASE)

//............. 0x4200 - 0x42ff............ for sbc
typedef struct {
    __RW __u32  CON0    ;
    __WO __u32  DEC_SRC_ADR ;
    __WO __u32  DEC_DST_ADR ;
    __WO __u32  DEC_PCM_WCNT;
    __WO __u32  DEC_INBUF_LEN;
    __WO __u32  ENC_SRC_ADR ;
    __WO __u32  ENC_DST_ADR ;
    __RO __u32  DEC_DST_BASE;

} JL_SBC_TypeDef;

#define JL_SBC_BASE                   (ls_base + map_adr(0x42, 0x00))
#define JL_SBC                        ((JL_SBC_TypeDef *)JL_SBC_BASE)

//............. 0x4300 - 0x43ff............ for aes
typedef struct {
    __RW __u32 CON;
    __RW __u32 DATIN;
    __WO __u32 KEY;
    __RW __u32 ENCRES0;
    __RW __u32 ENCRES1;
    __RW __u32 ENCRES2;
    __RW __u32 ENCRES3;
    __WO __u32 NONCE;
    __WO __u8  HEADER;
    __WO __u32 SRCADR;
    __WO __u32 DSTADR;
    __WO __u32 CTCNT;
    __WO __u32 TAGLEN;
    __RO __u32 TAGRES0;
    __RO __u32 TAGRES1;
    __RO __u32 TAGRES2;
    __RO __u32 TAGRES3;
} JL_AES_TypeDef;

#define JL_AES_BASE               (ls_base + map_adr(0x43, 0x00))

#define JL_AES                    ((JL_AES_TypeDef			*)JL_AES_BASE)





//............. 0x0100 - 0x01ff............ for port
typedef struct {
    __RW __u32 OUT;
    __RO __u32 IN;
    __RW __u32 DIR;
    __RW __u32 DIE;
    __RW __u32 PU;
    __RW __u32 PD;
    __RW __u32 HD0;
    __RW __u32 HD;
    __RW __u32 DIEH;
} JL_PORT_FLASH_TypeDef;

#define JL_PORTA_BASE                   (ls_base + map_adr(0x50, 0x00))
#define JL_PORTA                        ((JL_PORT_FLASH_TypeDef *)JL_PORTA_BASE)

#define JL_PORTB_BASE                   (ls_base + map_adr(0x50, 0x10))
#define JL_PORTB                        ((JL_PORT_FLASH_TypeDef *)JL_PORTB_BASE)

#define JL_PORTC_BASE                   (ls_base + map_adr(0x50, 0x20))
#define JL_PORTC                        ((JL_PORT_FLASH_TypeDef *)JL_PORTC_BASE)

#define JL_PORTD_BASE                   (ls_base + map_adr(0x50, 0x30))
#define JL_PORTD                        ((JL_PORT_FLASH_TypeDef *)JL_PORTD_BASE)

typedef struct {
    __RW __u32 CON0;
    __RW __u32 CON1;
} JL_USB_IO_TypeDef;

#define JL_USB_IO_BASE                  (ls_base + map_adr(0x51, 0x00))
#define JL_USB_IO                       ((JL_USB_IO_TypeDef    *)JL_USB_IO_BASE)

#define JL_USB1_IO_BASE                  (ls_base + map_adr(0x51, 0x0e))
#define JL_USB1_IO                       ((JL_USB_IO_TypeDef    *)JL_USB1_IO_BASE)

typedef struct {
    __RW __u32 CON0;
    __RW __u32 CON1;
    __RW __u32 CON2;
    __RW __u32 CON3;
} JL_WAKEUP_TypeDef;

#define JL_WAKEUP_BASE               (ls_base + map_adr(0x51, 0x02))
#define JL_WAKEUP                    ((JL_WAKEUP_TypeDef			*)JL_WAKEUP_BASE)

typedef struct {
    __RW __u32 CON0;
    __RW __u32 CON1;
    __RW __u32 CON2;
    __RW __u32 CON3;
    __RW __u32 CON4;
    __RW __u32 CON5;
} JL_IOMAP_TypeDef;

#define JL_IOMAP_BASE                   (ls_base + map_adr(0x51, 0x06))
#define JL_IOMAP                        ((JL_IOMAP_TypeDef      *)JL_IOMAP_BASE)



typedef struct {
    __RW __u32 CON0;
    __RW __u32 CON1;
    __RW __u32 CON2;
    __RW __u32 CON3;
    __RW __u32 BRI_PRDL;
    __RW __u32 BRI_PRDH;
    __RW __u32 BRI_DUTY0L;
    __RW __u32 BRI_DUTY0H;
    __RW __u32 BRI_DUTY1L;
    __RW __u32 BRI_DUTY1H;
    __RW __u32 PRD_DIVL;
    __RW __u32 DUTY0;
    __RW __u32 DUTY1;
    __RW __u32 DUTY2;
    __RW __u32 DUTY3;
    __RO __u32 CNT_RD;
} JL_PLED_TypeDef;

#define JL_PLED_BASE                   (ls_base + map_adr(0x52, 0x00))
#define JL_PLED                        ((JL_PLED_TypeDef      *)JL_PLED_BASE)


//.............  Full Speed USB ...................
#define MUSB_FADDR                   0x00
#define MUSB_POWER                   0x01
#define MUSB_INTRTX1                 0x02
#define MUSB_INTRTX2                 0x03
#define MUSB_INTRRX1                 0x04
#define MUSB_INTRRX2                 0x05
#define MUSB_INTRUSB                 0x06
#define MUSB_INTRTX1E                0x07
#define MUSB_INTRTX2E                0x08
#define MUSB_INTRRX1E                0x09
#define MUSB_INTRRX2E                0x0a
#define MUSB_INTRUSBE                0x0b
#define MUSB_FRAME1                  0x0c
#define MUSB_FRAME2                  0x0d
#define MUSB_INDEX                   0x0e
#define MUSB_DEVCTL                  0x0f
#define MUSB_TXMAXP                  0x10
#define MUSB_CSR0                    0x11
#define MUSB_TXCSR1                  0x11
#define MUSB_TXCSR2                  0x12
#define MUSB_RXMAXP                  0x13
#define MUSB_RXCSR1                  0x14
#define MUSB_RXCSR2                  0x15
#define MUSB_COUNT0                  0x16
#define MUSB_RXCOUNT1                0x16
#define MUSB_RXCOUNT2                0x17
#define MUSB_TXTYPE                  0x18
#define MUSB_TXINTERVAL              0x19
#define MUSB_RXTYPE                  0x1a
#define MUSB_RXINTERVAL              0x1b

#endif



