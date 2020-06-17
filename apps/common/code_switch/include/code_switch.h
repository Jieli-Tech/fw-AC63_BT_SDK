#ifndef _SW_H_
#define _SW_H_

#include "cpu.h"

typedef struct {
    u8 a_phase_io;
    u8 b_phase_io;
} SW_PLATFORM_DATA;

#define SW_PLATFORM_DATA_BEGIN(pdata) \
	SW_PLATFORM_DATA pdata = {

#define SW_PLATFORM_DATA_END() \
};

void code_switch_init(SW_PLATFORM_DATA *priv);

#endif
