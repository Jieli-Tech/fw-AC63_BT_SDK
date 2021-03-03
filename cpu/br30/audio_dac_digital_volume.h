#ifndef _DAC_DIGITAL_VOL_H_
#define _DAC_DIGITAL_VOL_H_

#include "generic/typedef.h"

void dac_digital_vol_open();
void dac_digital_vol_tab_register(u16 *tab, u8 max_level);
void dac_digital_vol_set(u16 vol_l, u16 vol_r, u8 fade);
void dac_digital_vol_close();

#endif/*_DAC_DIGITAL_VOL_H_*/
