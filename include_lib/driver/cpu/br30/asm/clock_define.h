

#ifndef _CPU_CLOCK_DEFINE__
#define _CPU_CLOCK_DEFINE__

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

