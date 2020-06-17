#ifndef __RCSP_BS_H__
#define __RCSP_BS_H__
#include "typedef.h"

enum {
    BS_FLODER = 0,
    BS_FILE,
};

enum {
    BS_UNICODE = 0,
    BS_ANSI,
};



bool rcsp_bs_start_check(u8 *data, u16 len);
bool rcsp_bs_start(u8 *data, u16 len);
void rcsp_bs_stop(void);
char *rcsp_bs_file_ext(void);
u16 rcsp_bs_file_ext_size(void);

#endif//__RCSP_BS_H__
