#ifndef  _MATRIX_KEYBOARD_H_
#define _MATRIX_KEYBOARD_H_

#include "typedef.h"

//左/右CTL，在/右ALT，在/右SHIFT，左/右WIN键盘
#define S_KEY(x)           (x|(0x1<<8))
#define _KEY_FN     0xff

#define ROW_MAX                 8
#define COL_MAX                 20

enum {
    MATRIX_KEY_UP = 0,
    MATRIX_KEY_SHORT,
    MATRIX_KEY_LONG,
    MATRIX_KEY_HOLD,
};

typedef struct _matrix_key_st {
    u32 press_cnt;
    u8 last_st;
    u8 filter_cnt;
    u8 filter_value;
    u8 click_cnt;
} matrix_key_st;


typedef struct _matrix_key_param {
    u32 *row_pin_list;      //row线IO口列表
    u32 *col_pin_list;      //col线IO口列表
    u8  row_num;            //row线数
    u8  col_num;            //col线数
} matrix_key_param;

int matrix_key_init(matrix_key_param *param);

#endif
