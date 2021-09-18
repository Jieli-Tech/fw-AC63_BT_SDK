#include "system/app_core.h"
#include "system/includes.h"
#include "server/server_core.h"
#include "app_config.h"
#include "app_action.h"

#define LOG_TAG_CONST       APP_IDLE
#define LOG_TAG             "[APP_IDLE]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


#if CONFIG_APP_IDLE || CONFIG_APP_SPP_LE_TO_IDLE

#if CONFIG_APP_IDLE
#if TCFG_USER_EDR_ENABLE || TCFG_USER_BLE_ENABLE
//默认应用不需要打开配置,有需要自己添加
#error " confirm, need disable !!!!!!"
#endif
#endif
static volatile u8 timer_wakeup_id;
extern void app_switch(const char *name, int action);
static void app_key_event_handler(struct sys_event *event)
{
    /* u16 cpi = 0; */
    u8 event_type = 0;
    u8 key_value = 0;
    if (event->arg == (void *)DEVICE_EVENT_FROM_KEY) {
        event_type = event->u.key.event;
        key_value = event->u.key.value;
        log_info("idle_key_evnet: %d,%d\n", event_type, key_value);
#if CONFIG_APP_SPP_LE_TO_IDLE
        if (event_type == KEY_EVENT_CLICK && key_value == TCFG_ADKEY_VALUE1) {
            log_info(">>>hold key1:change case to spp_le\n");
            //change case to spp_le test
            app_switch("spp_le", ACTION_SPPLE_MAIN);
        }
#endif
    }
}
static void idle_timer_handle_test(void)
{
    log_info("wakeup");
}

static void idle_app_start()
{
    log_info("=======================================");
    log_info("---------idle demo---------");
    log_info("=======================================");

    log_info("app_file: %s", __FILE__);

    clk_set("sys", BT_NORMAL_HZ);

    timer_wakeup_id = sys_timer_add(NULL, idle_timer_handle_test, 2000);

}

static int idle_event_handler(struct application *app, struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        app_key_event_handler(event);
        return 0;
    case SYS_BT_EVENT:
        return 0;
    case SYS_DEVICE_EVENT:
        return 0;
    default:
        return false;
    }
}


static int idle_state_machine(struct application *app, enum app_state state,
                              struct intent *it)
{
    switch (state) {
    case APP_STA_CREATE:
        break;
    case APP_STA_START:
        if (!it) {
            break;
        }
        switch (it->action) {
        case ACTION_IDLE_MAIN:
            log_info("ACTION_IDLE_MAIN\n");
            /* os_taskq_flush(); */
            idle_app_start();
            break;
        }
        break;
    case APP_STA_PAUSE:
        break;
    case APP_STA_RESUME:
        break;
    case APP_STA_STOP:
        break;
    case APP_STA_DESTROY:
        sys_timer_del(timer_wakeup_id);// 关闭idle定时器
        break;
    }

    return 0;
}

static const struct application_operation app_idle_ops = {
    .state_machine  = idle_state_machine,
    .event_handler 	= idle_event_handler,
};

REGISTER_APPLICATION(app_app_idle) = {
    .name 	= "idle",
    .action	= ACTION_IDLE_MAIN,
    .ops 	= &app_idle_ops,
    .state  = APP_STA_DESTROY,
};

#endif

