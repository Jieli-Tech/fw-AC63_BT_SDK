#ifndef _CPU_CLOCK_
#define _CPU_CLOCK_

#include "typedef.h"

#include "clock_hw.h"

///原生时钟源作系统时钟源
#define         SYS_CLOCK_INPUT_RC_250k 0
#define         SYS_CLOCK_INPUT_PAT     1
#define         SYS_CLOCK_INPUT_RTC_OSC 2
#define         SYS_CLOCK_INPUT_RC_16M  3
#define         SYS_CLOCK_INPUT_BT_OSC  4          //BTOSC 双脚(12-26M)
#define         SYS_CLOCK_INPUT_BT_OSC_X2   5       //BTOSC 双脚(12-26M)

///衍生时钟源作系统时钟源
#define         SYS_CLOCK_INPUT_PLL_LRC     6
#define         SYS_CLOCK_INPUT_PLL_RC_16M  7
#define         SYS_CLOCK_INPUT_PLL_BT_OSC  8
#define         SYS_CLOCK_INPUT_PLL_PAT     9
#define         SYS_CLOCK_INPUT_PLL_RCL     10

typedef int SYS_CLOCK_INPUT;

typedef enum {
    SYS_ICLOCK_INPUT_BTOSC,          //BTOSC 双脚(12-26M)
    SYS_ICLOCK_INPUT_BTOSC_X2,          //BTOSC 双脚(12-26M)
    SYS_ICLOCK_INPUT_STD24M,
    SYS_ICLOCK_INPUT_RTC_OSC,
    SYS_ICLOCK_INPUT_LRC,
    SYS_ICLOCK_INPUT_PAT,
} SYS_ICLOCK_INPUT;

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

int clk_set(const char *name, int clk);

int clk_set_sys_lock(int clk, int lock_en);

enum CLK_OUT_SOURCE {
    NONE = 0,
    SFC_CLK_OUT,
    HSB_CLK_OUT,
    LSB_CLK_OUT,
    STD_48M_CLK_OUT,
    STD_24M_CLK_OUT,
    RC16M_CLK_OUT,
    LRC_CLK_OUT,
    RTC_OSC_CLK_OUT,
    BTOSC_24M_CLK_OUT,
    BTOSC_48M_CLK_OUT,
    XOSC_FSCL_CLK_OUT,
    P33_RCLK_CLK_OUT,
    PLL_ALINK0_CLK_OUT,
    PLL_D4P5_CLK_OUT,
    PLL_75M_CLK_OUT,
};

typedef enum {
    CLK_DIV_1,
    CLK_DIV_4,
    CLK_DIV_16,
    CLK_DIV_64,
    CLK_DIV_2,
    CLK_DIV_8,
    CLK_DIV_32,
    CLK_DIV_128,
    CLK_DIV_256,
    CLK_DIV_1024,
    CLK_DIV_4096,
    CLK_DIV_16384,
    CLK_DIV_512,
    CLK_DIV_2048,
    CLK_DIV_8192,
    CLK_DIV_32768,
} CLK_DIV_4bit;

void clk_out(u8 gpio, enum CLK_OUT_SOURCE clk);

void clock_dump(void);

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
    SYS_76M = 76800000,
    SYS_96M = 96 * MHz,
};

enum clk_mode {
    CLOCK_MODE_ADAPTIVE = 0,
    CLOCK_MODE_USR,
};

//clk : SYS_48M / SYS_24M
void sys_clk_set(enum sys_clk clk);

void clk_voltage_init(u8 mode, u8 sys_dvdd);


void clk_set_osc_cap(u8 sel_l, u8 sel_r);

u32 clk_get_osc_cap();

/**
 * @brief clock_set_sfc_max_freq
 * 使用前需要保证所使用的flash支持4bit 100Mhz 模式
 *
 * @param dual_max_freq for cmd 3BH BBH
 * @param quad_max_freq for cmd 6BH EBH
 */
void clock_set_sfc_max_freq(u32 dual_max_freq, u32 quad_max_freq);
#endif

