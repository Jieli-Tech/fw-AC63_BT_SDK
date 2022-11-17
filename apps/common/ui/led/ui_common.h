#ifndef _UI_COMMON_H_
#define _UI_COMMON_H_

#if LED_UI_COMMON

#include "typedef.h"

void itoa1(u8 i);
void itoa2(u8 i);
void itoa3(u16 i);
void itoa4(u16 i);

extern const u8 asc_number[];

extern  u8  bcd_number[5];

#endif

#endif


