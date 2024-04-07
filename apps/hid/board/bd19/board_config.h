#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#ifndef   PWR_NO_CHANGE
//提供给预编译使用，定值同 power_interface.h
#define   PWR_NO_CHANGE             0
#define   PWR_LDO33                 1
#define   PWR_LDO15                 2
#define   PWR_DCDC15                3
#define   PWR_DCDC15_FOR_CHARGE     4
#endif

/*
 *  板级配置选择
 */

#define CONFIG_BOARD_AC632N_DEMO
// #define CONFIG_BOARD_AC6321A_DEMO
// #define CONFIG_BOARD_AC6321A_STAND_KEYBOARD//AC6321A+AD15键盘项目,download.bat需要修改添加打包ex_mcu.bin
// #define CONFIG_BOARD_AC6321A_MOUSE
// #define CONFIG_BOARD_AC6323A_DEMO
// #define CONFIG_BOARD_AC6328A_KEYFOB
// #define CONFIG_BOARD_AC6328B_DEMO
// #define CONFIG_BOARD_AC6329B_DEMO
// #define CONFIG_BOARD_AC6329C_DEMO
// #define CONFIG_BOARD_AC6329E_DEMO
// #define CONFIG_BOARD_AC6329F_DEMO

#include "board_ac632n_demo_cfg.h"
#include "board_ac6321a_demo_cfg.h"
#include "board_ac6321a_stand_keyboard_cfg.h"
#include "board_ac6321a_mouse_cfg.h"
#include "board_ac6323a_demo_cfg.h"
#include "board_ac6328a_keyfob_cfg.h"
#include "board_ac6328b_demo_cfg.h"
#include "board_ac6329b_demo_cfg.h"
#include "board_ac6329c_demo_cfg.h"
#include "board_ac6329e_demo_cfg.h"
#include "board_ac6329f_demo_cfg.h"

#endif
