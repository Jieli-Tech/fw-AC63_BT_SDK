//*********************************************************************************//
// Module name : csfr.h                                                            //
// Description : q32DSP core sfr define                                            //
// By Designer : zequan_liu                                                        //
// Dat changed :                                                                   //
//*********************************************************************************//

#ifndef __Q32DSP_CSFR__
#define __Q32DSP_CSFR__

#define __RW      volatile       // read write
#define __RO      volatile const // only read
#define __WO      volatile       // only write

#define __u8      unsigned int   // u8  to u32 special for struct
#define __u16     unsigned int   // u16 to u32 special for struct
#define __u32     unsigned int

//*********************************************************************************
//
// hcore_sfr
//
//*********************************************************************************

#define csfr_base    0x100000

//............. 0x0000 - 0x00ff............
// typedef struct {
//     __RO __u32 SOFT_INT;
//     __WO __u32 SOFT_SET;
//     __WO __u32 SOFT_CLR;
//     __RW __u32 CON;
// } JL_CMNG_TypeDef;
//
// #define JL_CMNG_BASE                   (csfr_base + map_adr(0x00, 0x00))
// #define JL_CMNG                        ((JL_CMNG_TypeDef *)JL_CMNG_BASE)

//............. 0x0100 - 0x01ff............
typedef struct {
    __RW __u32 CON;
    __RW __u32 KEY;
} JL_SDTAP_TypeDef;

#define JL_SDTAP_BASE                  (csfr_base + map_adr(0x01, 0x00))
#define JL_SDTAP                       ((JL_SDTAP_TypeDef *)JL_SDTAP_BASE)

//............. 0x0200 - 0x02ff............
typedef struct {
    __RW __u32 MBISTCTL;
    __RO __u32 MBISTSOGO;
} JL_MBIS_TypeDef;

#define JL_MBIS_BASE                   (csfr_base + map_adr(0x02, 0x00))
#define JL_MBIS                        ((JL_MBIS_TypeDef *)JL_MBIS_BASE)

//............. 0x0300 - 0x03ff............ for mmu
/* __attribute__((packed))
typedef struct {
    short page: 13;
    short vld: 1;
    short reserved: 2;
} JL_MMU_TLB1_TypeDef;

#define JL_MMU_TLB1                  ((JL_MMU_TLB1_TypeDef *)(JL_MMU->TLB1_BEG)) */

typedef struct {
    __RW __u32 CON;
    __RW __u32 TLB1_BEG;
    __RW __u32 TLB1_END;
} JL_MMU_TypeDef;

#define JL_MMU_BASE                  (csfr_base + map_adr(0x03, 0x00))
#define JL_MMU                       ((JL_MMU_TypeDef *)JL_MMU_BASE)

//............. 0x1000 - 0x10ff............ for debug
typedef struct {
    __RW __u32 CON;
    __RW __u32 RING_OSC;
    __RW __u32 CPASS_CON;
    __RW __u32 CPASS_ADRH;
    __RW __u32 CPASS_ADRL;
    __RW __u32 CPASS_BUF_LAST;
    __RW __u32 CPREFETCH_ADRH;
    __RW __u32 CPREFETCH_ADRL;
    __RO __u32 CACHE_MSG_CH;
} JL_DSP_TypeDef;

#define JL_DSP_BASE                   (csfr_base + map_adr(0x10, 0x00))
#define JL_DSP                        ((JL_DSP_TypeDef*)JL_DSP_BASE)

typedef struct {
    __RW __u32 DSP_BF_CON;
    __RW __u32 WR_EN;
    __RO __u32 MSG;
    __WO __u32 MSG_CLR;
    __RW __u32 DSP_EX_LIMH;
    __RW __u32 DSP_EX_LIML;
    __RW __u32 PRP_EX_LIMH;
    __RW __u32 PRP_EX_LIML;
    __RO __u32 PRP_MMU_MSG;
    __RO __u32 LSB_MMU_MSG_CH;
    __RO __u32 PRP_WR_LIMIT_MSG;
    __RO __u32 LSB_WR_LIMIT_CH;
    __RW __u32 DSP_PC_LIMH0;
    __RW __u32 DSP_PC_LIML0;
    __RW __u32 DSP_PC_LIMH1;
    __RW __u32 DSP_PC_LIML1;
    __RW __u32 PRP_SRM_INV_MSG;
    __RW __u32 LSB_SRM_INV_CH;
} JL_DEBUG_TypeDef;

#define JL_DEBUG_BASE                 (csfr_base + map_adr(0x10, 0x10))
#define JL_DEBUG                      ((JL_DEBUG_TypeDef *)JL_DEBUG_BASE)

//............. 0x2000 - 0x20ff............ for fft
typedef struct {
    __RW __u32 CON;
    __RW __u32 CADR;
    __RW __u32 TEST0;
    __RW __u32 TEST1;
} JL_FFT_TypeDef;

#define JL_FFT_BASE                  (csfr_base + map_adr(0x20, 0x00))
#define JL_FFT                       ((JL_FFT_TypeDef *)JL_FFT_BASE)

//*********************************************************************************
//
// q32DSP_sfr
//
//*********************************************************************************

//---------------------------------------------//
// q32DSP define
//---------------------------------------------//

#define q32DSP_sfr_offset   0x010000
#define q32DSP_sfr_base    (csfr_base + 0xf000)

#define q32DSP_cpu_base    (q32DSP_sfr_base + 0x00)
#define q32DSP_mpu_base    (q32DSP_sfr_base + 0x80)

#define q32DSP(n)          ((JL_TypeDef_q32DSP     *)(q32DSP_sfr_base + q32DSP_sfr_offset*n))
#define q32DSP_mpu(n)      ((JL_TypeDef_q32DSP_MPU *)(q32DSP_mpu_base + q32DSP_sfr_offset*n))

//---------------------------------------------//
// q32DSP core sfr
//---------------------------------------------//

typedef struct {
    /* 00 */  __RO __u32 DR00;
    /* 01 */  __RO __u32 DR01;
    /* 02 */  __RO __u32 DR02;
    /* 03 */  __RO __u32 DR03;
    /* 04 */  __RO __u32 DR04;
    /* 05 */  __RO __u32 DR05;
    /* 06 */  __RO __u32 DR06;
    /* 07 */  __RO __u32 DR07;
    /* 08 */  __RO __u32 DR08;
    /* 09 */  __RO __u32 DR09;
    /* 0a */  __RO __u32 DR10;
    /* 0b */  __RO __u32 DR11;
    /* 0c */  __RO __u32 DR12;
    /* 0d */  __RO __u32 DR13;
    /* 0e */  __RO __u32 DR14;
    /* 0f */  __RO __u32 DR15;

    /* 10 */  __RO __u32 RETI;
    /* 11 */  __RO __u32 RETE;
    /* 12 */  __RO __u32 RETX;
    /* 13 */  __RO __u32 RETS;
    /* 14 */  __RO __u32 SR04;
    /* 15 */  __RO __u32 PSR;
    /* 16 */  __RO __u32 CNUM;
    /* 17 */  __RO __u32 SR07;
    /* 18 */  __RO __u32 SR08;
    /* 19 */  __RO __u32 SR09;
    /* 1a */  __RO __u32 SR10;
    /* 1b */  __RO __u32 ICFG;
    /* 1c */  __RO __u32 USP;
    /* 1d */  __RO __u32 SSP;
    /* 1e */  __RO __u32 SP;
    /* 1f */  __RO __u32 PCRS;

    /* 20 */  __RW __u32 BPCON;
    /* 21 */  __RW __u32 BSP;
    /* 22 */  __RW __u32 BP0;
    /* 23 */  __RW __u32 BP1;
    /* 24 */  __RW __u32 BP2;
    /* 25 */  __RW __u32 BP3;
    /* 26 */  __WO __u32 CMD_PAUSE;
    /*    */  __RO __u32 REV_30_26[0x30 - 0x26 - 1];

    /* 30 */  __RW __u32 PMU_CON;
    /*    */  __RO __u32 REV_34_30[0x34 - 0x30 - 1];
    /* 34 */  __RW __u32 EMU_CON;
    /* 35 */  __RW __u32 EMU_MSG;
    /* 36 */  __RW __u32 EMU_SSP_H;
    /* 37 */  __RW __u32 EMU_SSP_L;
    /* 38 */  __RW __u32 EMU_USP_H;
    /* 39 */  __RW __u32 EMU_USP_L;
    /*    */  __RO __u32 REV_3b_39[0x3b - 0x39 - 1];
    /* 3b */  __RW __u8  TTMR_CON;
    /* 3c */  __RW __u32 TTMR_CNT;
    /* 3d */  __RW __u32 TTMR_PRD;
    /* 3e */  __RW __u32 BANK_CON;
    /* 3f */  __RW __u32 BANK_NUM;

    /* 40 */  __RW __u32 ICFG00;
    /* 41 */  __RW __u32 ICFG01;
    /* 42 */  __RW __u32 ICFG02;
    /* 43 */  __RW __u32 ICFG03;
    /* 44 */  __RW __u32 ICFG04;
    /* 45 */  __RW __u32 ICFG05;
    /* 46 */  __RW __u32 ICFG06;
    /* 47 */  __RW __u32 ICFG07;
    /* 48 */  __RW __u32 ICFG08;
    /* 49 */  __RW __u32 ICFG09;
    /* 4a */  __RW __u32 ICFG10;
    /* 4b */  __RW __u32 ICFG11;
    /* 4c */  __RW __u32 ICFG12;
    /* 4d */  __RW __u32 ICFG13;
    /* 4e */  __RW __u32 ICFG14;
    /* 4f */  __RW __u32 ICFG15;

    /* 50 */  __RW __u32 ICFG16;
    /* 51 */  __RW __u32 ICFG17;
    /* 52 */  __RW __u32 ICFG18;
    /* 53 */  __RW __u32 ICFG19;
    /* 54 */  __RW __u32 ICFG20;
    /* 55 */  __RW __u32 ICFG21;
    /* 56 */  __RW __u32 ICFG22;
    /* 57 */  __RW __u32 ICFG23;
    /* 58 */  __RW __u32 ICFG24;
    /* 59 */  __RW __u32 ICFG25;
    /* 5a */  __RW __u32 ICFG26;
    /* 5b */  __RW __u32 ICFG27;
    /* 5c */  __RW __u32 ICFG28;
    /* 5d */  __RW __u32 ICFG29;
    /* 5e */  __RW __u32 ICFG30;
    /* 5f */  __RW __u32 ICFG31;

    /* 60 */  __RO __u32 IPND0;
    /* 61 */  __RO __u32 IPND1;
    /* 62 */  __RO __u32 IPND2;
    /* 63 */  __RO __u32 IPND3;
    /* 64 */  __RO __u32 IPND4;
    /* 65 */  __RO __u32 IPND5;
    /* 66 */  __RO __u32 IPND6;
    /* 67 */  __RO __u32 IPND7;
    /* 68 */  __WO __u32 ILAT_SET;
    /* 69 */  __WO __u32 ILAT_CLR;
    /* 6a */  __RW __u32 IPMASK;
    /*    */  __RO __u32 REV_70_6a[0x70 - 0x6a - 1];

    /* 70 */  __RW __u32 ETM_CON;
    /* 71 */  __RO __u32 ETM_PC0;
    /* 72 */  __RO __u32 ETM_PC1;
    /* 73 */  __RO __u32 ETM_PC2;
    /* 74 */  __RO __u32 ETM_PC3;
    /* 75 */  __RW __u32 WP0_ADRH;
    /* 76 */  __RW __u32 WP0_ADRL;
    /* 77 */  __RW __u32 WP0_DATH;
    /* 78 */  __RW __u32 WP0_DATL;
    /* 79 */  __RW __u32 WP0_PC;
} JL_TypeDef_q32DSP;

#undef __RW
#undef __RO
#undef __WO

#undef __u8
#undef __u16
#undef __u32

typedef struct _CPU_REGS {
    unsigned int reti;
    unsigned int rets;
    unsigned int psr;
    unsigned int r0;
    unsigned int r1;
    unsigned int r2;
    unsigned int r3;
    unsigned int r4;
    unsigned int r5;
    unsigned int r6;
    unsigned int r7;
    unsigned int r8;
    unsigned int r9;
    unsigned int r10;
    unsigned int r11;
    unsigned int r12;
    unsigned int r13;
    unsigned int r14;
    unsigned int r15;
} CPU_REGS;

#define TICK_CON                (q32DSP(0)->TTMR_CON)
#define TICK_PRD                (q32DSP(0)->TTMR_PRD)
#define TICK_CNT                (q32DSP(0)->TTMR_CNT)

#define SOFT_CLEAR_PENDING      (q32DSP(0)->ILAT_CLR)

#define CPU_MSG                 (q32DSP(0)->EMU_MSG)
#define CPU_CON                 (q32DSP(0)->EMU_CON)

#endif

//*********************************************************************************//
//                                                                                 //
//                               end of this module                                //
//                                                                                 //
//*********************************************************************************//

