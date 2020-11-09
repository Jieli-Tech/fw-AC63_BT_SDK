
#ifndef _CPU_CLOCK_DEFINE__
#define _CPU_CLOCK_DEFINE__

#define    SYS_CLOCK_INPUT_RC  0
#define    SYS_CLOCK_INPUT_BT_OSC  1          //BTOSC 双脚(12-26M)
#define    SYS_CLOCK_INPUT_RTOSCH  2
#define    SYS_CLOCK_INPUT_RTOSCL  3
#define    SYS_CLOCK_INPUT_PAT     4

#define    SYS_CLOCK_INPUT_BT_OSCX2  5          //BTOSC 双脚(12-26M)

///衍生时钟源作系统时钟源
#define    SYS_CLOCK_INPUT_PLL_RCL    6
#define    SYS_CLOCK_INPUT_PLL_RCH    7
#define    SYS_CLOCK_INPUT_PLL_BT_OSC  8
#define    SYS_CLOCK_INPUT_PLL_RTOSCH  9
#define    SYS_CLOCK_INPUT_PLL_PAT    10

#endif

