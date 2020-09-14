/*********************************************************************************************
    *   Filename        : app_main.c

    *   Description     :

    *   Author          : Bingquan

    *   Email           : caibingquan@zh-jieli.com

    *   Last modifiled  : 2019-05-11 14:54

    *   Copyright:(c)JIELI  2011-2019  @ , All Rights Reserved.
*********************************************************************************************/
#include "system/includes.h"
#include "app_config.h"
#include "app_action.h"
#include "app_main.h"
#include "update.h"

#define LOG_TAG_CONST       APP
#define LOG_TAG             "[APP]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

/*任务列表 */
const struct task_info task_info_table[] = {
    {"app_core",            1,     640,   128  },
    {"sys_event",           7,     256,   0    },
    {"btctrler",            4,     512,   256  },
    {"btstack",             3,     768,  256  },
    {"systimer",		    7,	   128,   0		},
#ifdef CONFIG_UPDATA_ENABLE
    {"update",				1,	   320,   0		},
#endif
#if (RCSP_BTMATE_EN)
    {"rcsp_task",		2,		640,	128	},
#endif
    {0, 0},
};

APP_VAR app_var;

void app_var_init(void)
{
    app_var.play_poweron_tone = 1;

    app_var.auto_off_time =  TCFG_AUTO_SHUT_DOWN_TIME;
    app_var.warning_tone_v = 340;
    app_var.poweroff_tone_v = 330;
}

void app_main()
{
    struct intent it;

#ifdef CONFIG_UPDATA_ENABLE
    int update = 0;
    update = update_result_deal();
#endif

    printf(">>>>>>>>>>>>>>>>>app_main...\n");
    init_intent(&it);

#if (CONFIG_APP_MOUSE)
    it.name = "mouse";
    it.action = ACTION_MOUSE_MAIN;
#elif(CONFIG_APP_KEYFOB)
    it.name = "keyfob";
    it.action = ACTION_KEYFOB;
#elif(CONFIG_APP_KEYBOARD)
    it.name = "hid_key";
    it.action = ACTION_HID_MAIN;
#elif(CONFIG_APP_STANDARD_KEYBOARD)
    it.name = "hid_key";
    it.action = ACTION_HID_MAIN;
#elif(CONFIG_APP_KEYPAGE)
    it.name = "keypage";
    it.action = ACTION_KEYPAGE;
#else
    ASSERT(0, "no app!!!");
#endif

    log_info("run app>>>%s", it.name);

    start_app(&it);
}

/*
 * app模式切换
 */
void app_switch(const char *name, int action)
{
    struct intent it;
    struct application *app;

    log_info("app_exit\n");

    init_intent(&it);
    app = get_current_app();
    if (app) {
        /*
         * 退出当前app, 会执行state_machine()函数中APP_STA_STOP 和 APP_STA_DESTORY
         */
        it.name = app->name;
        it.action = ACTION_BACK;
        start_app(&it);
    }

    /*
     * 切换到app (name)并执行action分支
     */
    it.name = name;
    it.action = action;
    start_app(&it);
}

int eSystemConfirmStopStatus(void)
{
    /* 系统进入在未来时间里，无任务超时唤醒，可根据用户选择系统停止，或者系统定时唤醒(100ms) */
    //1:Endless Sleep
    //0:100 ms wakeup
    /* log_info("100ms wakeup"); */
    return 1;
}


