#ifndef _CPU_CLOCK_
#define _CPU_CLOCK_

#include "typedef.h"

#include "clock_hw.h"
#include "asm/clock_define.h"

/*
typedef enum {
    ///原生时钟源作系统时钟源
    SYS_CLOCK_INPUT_RC,
    SYS_CLOCK_INPUT_BT_OSC,          //BTOSC 双脚(12-26M)
    SYS_CLOCK_INPUT_RTOSCH,
    SYS_CLOCK_INPUT_RTOSCL,
    SYS_CLOCK_INPUT_PAT,

    ///衍生时钟源作系统时钟源
    SYS_CLOCK_INPUT_PLL_RCL,
    SYS_CLOCK_INPUT_PLL_RCH,
    SYS_CLOCK_INPUT_PLL_BT_OSC,
    SYS_CLOCK_INPUT_PLL_RTOSCH,
    SYS_CLOCK_INPUT_PLL_PAT,
} SYS_CLOCK_INPUT;
*/

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

int clk_set(const char *name, int clk);

int clk_set_sys_lock(int clk, int lock_en);

void clock_dump(void);

enum sys_clk {
    SYS_24M,
    SYS_48M,
};

enum clk_mode {
    CLOCK_MODE_ADAPTIVE = 0,
    CLOCK_MODE_USR,
};

//clk : SYS_48M / SYS_24M
void sys_clk_set(u8 clk);

void clk_voltage_init(u8 mode, u8 sys_dvdd, u8 pwr_mode, u8 vdc13);

void clk_set_osc_cap(u8 sel_l, u8 sel_r);

void clk_set_default_osc_cap();

u32 clk_get_osc_cap();

void clk_init_osc_cap(u8 sel_l, u8 sel_r);

void clk_init_osc_ldos(u8 ldos);

/**
 * @brief clock_set_sfc_max_freq
 * 使用前需要保证所使用的flash支持4bit 100Mhz 模式
 *
 * @param dual_max_freq for cmd 3BH BBH
 * @param quad_max_freq for cmd 6BH EBH
 */
void clock_set_sfc_max_freq(u32 dual_max_freq, u32 quad_max_freq);
#endif

