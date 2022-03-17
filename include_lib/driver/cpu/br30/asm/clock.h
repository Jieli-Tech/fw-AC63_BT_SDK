#ifndef _CPU_CLOCK_
#define _CPU_CLOCK_

#include "typedef.h"

#include "clock_hw.h"
#include "asm/clock_define.h"

#if 0
///原生时钟源作系统时钟源
#define         SYS_CLOCK_INPUT_RC  0
#define         SYS_CLOCK_INPUT_BT_OSC  1          //BTOSC 双脚(12-26M)
#define         SYS_CLOCK_INPUT_RTOSCH  2
#define         SYS_CLOCK_INPUT_RTOSCL  3
#define         SYS_CLOCK_INPUT_PAT     4

///衍生时钟源作系统时钟源
#define         SYS_CLOCK_INPUT_PLL_BT_OSC  5
#define         SYS_CLOCK_INPUT_PLL_RTOSCH  6
#define         SYS_CLOCK_INPUT_PLL_PAT     7
#define         SYS_CLOCK_INPUT_PLL_RCL     8
#endif

typedef int SYS_CLOCK_INPUT;

typedef enum {
    SYS_ICLOCK_INPUT_BTOSC,          //BTOSC 双脚(12-26M)
    SYS_ICLOCK_INPUT_RTOSCH,
    SYS_ICLOCK_INPUT_RTOSCL,
    SYS_ICLOCK_INPUT_PAT,
} SYS_ICLOCK_INPUT;

typedef enum {
    PA0_CLOCK_OUTPUT = 0,
    PA0_CLOCK_OUT_BT_OSC,
    PA0_CLOCK_OUT_RTOSCH,
    PA0_CLOCK_OUT_NULL,

    PA0_CLOCK_OUT_LSB = 4,
    PA0_CLOCK_OUT_HSB,
    PA0_CLOCK_OUT_SFC,
    PA0_CLOCK_OUT_PLL,
} PA0_CLK_OUT;

typedef enum {
    PB8_CLOCK_OUTPUT = 0,
    PB8_CLOCK_OUT_RC,
    PB8_CLOCK_OUT_LRC,
    PB8_CLOCK_OUT_NULL,

    PB8_CLOCK_OUT_PLL75M = 4,
    PB8_CLOCK_OUT_XOSC_FSCK,
    PB8_CLOCK_OUT_PLL320,
    PB8_CLOCK_OUT_PLL107,
} PB8_CLK_OUT;

/*
 * system enter critical and exit critical handle
 * */
struct clock_critical_handler {
    void (*enter)();
    void (*exit)();
};

#define CLOCK_CRITICAL_HANDLE_REG(name, enter, exit) \
	const struct clock_critical_handler clock_##name \
		 SEC_USED(.clock_critical_txt) = {enter, exit};

extern struct clock_critical_handler clock_critical_handler_begin[];
extern struct clock_critical_handler clock_critical_handler_end[];

#define list_for_each_loop_clock_critical(h) \
	for (h=clock_critical_handler_begin; h<clock_critical_handler_end; h++)


int clk_early_init(u8 sys_in, u32 input_freq, u32 out_freq);

int clk_get(const char *name);

void clk_set_en(u8 en);

int clk_set(const char *name, int clk);

int clk_set_sys_lock(int clk, int lock_en);

enum CLK_OUT_SOURCE {
    NONE = 0,
    BTOSC_CLK_OUT,
    RTC_OSL_CLK_OUT,
    LSB_CLK_OUT,
    HSB_CLK_OUT,
    SFC_CLK_OUT,
    RC_CLK_OUT,
    LRC_CLK_OUT,
    PLL_75M_CLK_OUT,
    XOSC_FSCK_OUT,
    PLL_48M_CLK_OUT,
    PLL_24M_CLK_OUT,
};


void clk_out(u8 gpio, enum CLK_OUT_SOURCE clk);

void clock_dump(void);

void clock_dump_lite(void);

void clock_debug(void);

#define MHz	(1000000L)
enum sys_clk {
    SYS_6M  = 6 * MHz,
    SYS_8M  = 8 * MHz,
    SYS_12M = 12 * MHz,
    SYS_16M = 16 * MHz,
    SYS_24M = 24 * MHz,
    SYS_32M = 32 * MHz,
    SYS_48M = 48 * MHz,
    SYS_64M = 64 * MHz,
};

enum clk_mode {
    CLOCK_MODE_ADAPTIVE = 0,
    CLOCK_MODE_USR,
};

typedef enum {
    ALINK_CLOCK_12M288K,  //160M div 13, 48k采样率类型
    ALINK_CLOCK_11M2896K, //192M div 17, 44.1k采样率类型
} ALINK_INPUT_CLK_TYPE;

//clk : SYS_48M / SYS_24M
void sys_clk_set(enum sys_clk clk);

void clk_voltage_init(u8 mode, u8 sys_dvdd);

void clk_set_osc_cap(u8 sel_l, u8 sel_r);

void clk_set_default_osc_cap();

u32 clk_get_osc_cap();

void clk_init_osc_cap(u8 sel_l, u8 sel_r);

void audio_link_clock_sel(ALINK_INPUT_CLK_TYPE type);	//配置ALINK主时钟

void clock_set_pll_target_frequency(u32 freq);	//配置PLL_TARGET_FREQUENCY

u32 clock_get_pll_target_frequency();			//获取PLL_TARGET_FREQUENCY

/* ***************************************************************************/
/**
 * \Brief :         频率电压适配模式接口，支持动态配置频率电压为自适应或用户设置
 *
 * \Param :         mode    : CLOCK_MODE_ADAPTIVE 频率电压自适应使能 / CLOCK_MODE_USR 频率电压用户控制
 * \Param :         sys_dvdd: 用户设置值
 */
/* *****************************************************************************/
void clk_voltage_mode(u8 mode, u8 sys_dvdd);

/**
 * @brief clock_set_sfc_max_freq
 * 使用前需要保证所使用的flash支持4bit 100Mhz 模式
 *
 * @param dual_max_freq for cmd 3BH BBH
 * @param quad_max_freq for cmd 6BH EBH
 */
void clock_set_sfc_max_freq(u32 dual_max_freq, u32 quad_max_freq);
#endif

