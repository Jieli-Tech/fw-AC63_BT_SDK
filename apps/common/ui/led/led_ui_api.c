/*--------------------------------------------------------------------------*/
/**@file    LED_UI_API.c
   @brief   LED 显示界面接口函数
   @details
   @author  bingquan Cai
   @date    2012-8-30
   @note    AC109N
*/
/*----------------------------------------------------------------------------*/

#include "ui_api.h"

#if LED_DISP
#include "ui_common.h"
#include "led_api.h"
#include "led_driver.h"
#include "msg.h"

#define LOG_TAG_CONST       NORM
#define LOG_TAG             "[normal]"
#include "debug.h"


UI_VAR UI_var;   /*UI 显示变量*/
DIS_VAR dis_var;   //显示相关参数

/*----------------------------------------------------------------------------*/
/**@brief   UI 显示界面处理函数
   @param   menu：需要显示的界面
   @return  无
   @note    void UI_menu_api(u8 menu)
*/
/*----------------------------------------------------------------------------*/
void UI_menu_api(u8 menu)
{
    /*界面属性-非主界面自动返回*/
    if (menu == MENU_MAIN) {
        if (UI_var.bMenuReturnCnt < UI_RETURN) {
            UI_var.bMenuReturnCnt++;
            if (UI_var.bMenuReturnCnt == UI_RETURN) {
                if (UI_var.bCurMenu == MENU_INPUT_NUMBER) {
                    post_msg(MSG_INPUT_TIMEOUT);    //输入超时
                } else {
                    UI_var.bCurMenu = UI_var.bMainMenu;
                }
            }
        } else {
            /*等待界面不重复刷新界面*/
            if (UI_var.bCurMenu == UI_var.bMainMenu) {
                return;
            }
            UI_var.bCurMenu = UI_var.bMainMenu;
        }
    } else {
        if (menu > 0x80) {  //仅在当前界面为主界面时刷新界面,例如：在主界面刷新播放时间
            if (UI_var.bCurMenu != UI_var.bMainMenu) {
                return;
            }
        } else {
            /*非主界面需要启动返回计数器*/
            if (menu != UI_var.bMainMenu) {
                UI_var.bMenuReturnCnt = 0;
            }
            UI_var.bCurMenu = menu;
            // if (menu != MENU_INPUT_NUMBER) {
            //     input_number = 0;
            // }
        }
    }
    led_drvier_setX(0);

//     switch (UI_var.bCurMenu) {
//     /*-----System Power On UI*/
//     case MENU_POWER_UP:
//     case MENU_WAIT:
// #ifdef USB_DEVICE_EN
//     case MENU_PC_MAIN:
//     case MENU_PC_VOL_UP:
//     case MENU_PC_VOL_DOWN:
// #endif
//     case MENU_AUX_MAIN:
// #ifdef RTC_ALARM_EN
//     case MENU_ALM_UP:
// #endif
// #ifdef REC_ENABLE
//     case MENU_REC:
// #endif
// #ifdef LOUDSPEAKER_EN
//     case MENU_LSP:
// #endif
//         led_drvier_show_string_menu(UI_var.bCurMenu);
//         break;

//     /*-----Common Info UI*/
//     case MENU_MAIN_VOL:
//         led_drvier_show_volume();
//         break;

//     case MENU_INPUT_NUMBER:
//         led_drvier_show_IR_number();
//         break;

//     /*-----Music Related UI*/
//     case MENU_MUSIC_MAIN:
//     case MENU_PAUSE:
//         led_drvier_show_music_main();
//         break;
//     case MENU_FILENUM:
//         led_drvier_show_filenumber();
//         break;
//     case MENU_EQ:
//         led_drvier_show_eq();
//         break;
//     case MENU_PLAYMODE:
//         led_drvier_show_playmode();
//         break;

//         /*-----FM Related UI*/
// #ifdef FM_ENABLE
//     case MENU_FM_MAIN:
//     case MENU_FM_DISP_FRE:
//         led_drvier_show_fm_main();
//         break;
//     case MENU_FM_FIND_STATION:
//     case MENU_FM_CHANNEL:
//         led_drvier_show_fm_station();
//         break;
// #endif

// #ifdef RTC_EN
//     case MENU_RTC_MAIN:
//         RTC_setting_var.bMode = 0;    //模式与界面同步返回
//     case MENU_RTC_SET:
//         led_drvier_show_RTC_main();
//         break;
// #ifdef RTC_ALARM_EN
//     case MENU_ALM_SET:
//         led_drvier_show_alarm();
//         break;
// #endif
// #endif

//     default:
//         break;
//     }
}



























#endif


