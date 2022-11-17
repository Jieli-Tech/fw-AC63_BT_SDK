#ifndef _UI_API_H
#define _UI_API_H
#include "typedef.h"
#include "app_config.h"
#include "led_api.h"
#include "led_driver.h"

#if LED_DISP

enum {
    MENU_POWER_UP = 0,
    MENU_WAIT,
#ifdef USB_DEVICE_EN
    MENU_PC_MAIN,
    MENU_PC_VOL_UP,
    MENU_PC_VOL_DOWN,
#endif
    MENU_AUX_MAIN,
#ifdef RTC_ALARM_EN
    MENU_ALM_UP,
#endif
#ifdef REC_ENABLE
    MENU_REC,
#endif
#ifdef LOUDSPEAKER_EN
    MENU_LSP,
#endif

    MENU_PLAY,
    MENU_PLAYMODE,
    MENU_MAIN_VOL,
    MENU_EQ,
    MENU_NOFILE,
    MENU_NODEVICE,
    MENU_PLAY_TIME,
    MENU_FILENUM,
    MENU_INPUT_NUMBER,
    MENU_MUSIC_MAIN,
    MENU_PAUSE,
    MENU_FM_MAIN,
    MENU_FM_DISP_FRE,
    MENU_FM_FIND_STATION,
    MENU_FM_CHANNEL,
    MENU_USBREMOVE,
    MENU_SDREMOVE,
    MENU_SCAN_DISK,

    MENU_RTC_MAIN,
    MENU_RTC_SET,
    MENU_RTC_PWD,
    MENU_ALM_SET,
    MENU_ALM_REQUEST,


    MENU_200MS_REFRESH = 0x80,
    MENU_100MS_REFRESH,
    MENU_SET_EQ,
    MENU_SET_PLAY_MODE,
    MENU_HALF_SEC_REFRESH,
    MENU_POWER_DOWN,
    MENU_MAIN = 0xFF,
};


typedef struct _UI_VAR {
    u8  bCurMenu;
    u8  bMainMenu;
    u8  bMenuReturnCnt;
} UI_VAR;



typedef struct _DIS_VAR {
    u8  bCurMenu;
    u8  bMainMenu;
    u8  bMenuReturnCnt;
} DIS_VAR;


#define UI_RETURN				5

void UI_menu_api(u8 menu);

#if UI_ENABLE
extern UI_VAR UI_var;   /*UI 显示变量*/
#define UI_menu(x)			UI_menu_api(x)
#define SET_UI_MAIN(x)	UI_var.bMainMenu = x
#else
#define UI_menu(...)
#define SET_UI_MAIN(...)
#endif

#endif
#endif

