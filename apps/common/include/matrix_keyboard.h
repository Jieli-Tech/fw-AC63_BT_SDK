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

// int matrix_key_init(matrix_key_param *param);
// void ex_mcu_enter_powerdown(void);
// void ex_mcu_enter_poweroff(void);
// void ex_mcu_exit_powerdown(u32 gpio);

//------------------------按键宏接口---------------------------------//
#if TCFG_EX_MCU_ENABLE
void ex_mcu_enter_powerdown();
void ex_mcu_exit_powerdown(u32 gpio);
void ex_mcu_enter_poweroff();
void ex_mcu_full_key_map(u8 row, u8 col, u8 st);
void ex_mcu_matrix_key_set_io_state(u8 state, u32 *io_table, u8 len);
void ex_mcu_matrix_key_scan();
void ex_mcu_matrix_key_wakeup_timeout_handler(void *arg);
void ex_mcu_matrix_key_wakeup_keep();
void ex_mcu_matrix_key_wakeup(u8 idx, u32 gpio);
int ex_mcu_matrix_key_init(matrix_key_param *param);

#define EX_MCU_ENTER_POWERDOWN()           ex_mcu_enter_powerdown()
#define EX_MCU_EXIT_POWERDOWN(a)           ex_mcu_exit_powerdown(a)
#define EX_MCU_ENTER_POWEROFF()            ex_mcu_enter_poweroff()
#define FULL_KEY_MAP(a, b, c)       ex_mcu_full_key_map(a, b, c)
#define MATRIX_KEY_SET_IO_STATE(a, b, c)   ex_mcu_matrix_key_set_io_state(a, b, c)
#define MATRIX_KEY_SCAN()                  ex_mcu_matrix_key_scan()
#define MATRIX_KEY_WAKEUP_TIMEOUT_HANDLER(a) ex_mcu_matrix_key_wakeup_timeout_handler(a)
#define MATRIX_KEY_WAKEUP_KEEP()           ex_mcu_matrix_key_wakeup_keep()
#define MATRIX_KEY_WAKEUP(a, b)            ex_mcu_matrix_key_wakeup(a, b)
#define MATRIX_KEY_INIT(a)                 ex_mcu_matrix_key_init(a)

#else
void full_key_map(u8 row, u8 col, u8 st);
void matrix_key_set_io_state(u8 state, u32 *io_table, u8 len);
void matrix_key_scan();
void matrix_key_wakeup_timeout_handler(void *arg);
void matrix_key_wakeup_keep();
void matrix_key_wakeup(u8 idx, u32 gpio);
int matrix_key_init(matrix_key_param *param);

#define EX_MCU_ENTER_POWERDOWN()
#define EX_MCU_EXIT_POWERDOWN(a)
#define EX_MCU_ENTER_POWEROFF()
#define FULL_KEY_MAP(a, b, c)       full_key_map(a, b, c)
#define MATRIX_KEY_SET_IO_STATE(a, b, c)   matrix_key_set_io_state(a, b, c)
#define MATRIX_KEY_SCAN()                  matrix_key_scan()
#define MATRIX_KEY_WAKEUP_TIMEOUT_HANDLER(a) matrix_key_wakeup_timeout_handler(a)
#define MATRIX_KEY_WAKEUP_KEEP()           matrix_key_wakeup_keep()
#define MATRIX_KEY_WAKEUP(a, b)            matrix_key_wakeup(a, b)
#define MATRIX_KEY_INIT(a)                 matrix_key_init(a)

#endif

#endif
