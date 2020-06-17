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

static int idle_event_handler(struct application *app, struct sys_event *event)
{
    switch (event->type) {
    case SYS_KEY_EVENT:
        return 0;
    case SYS_BT_EVENT:
        return 0;
    case SYS_DEVICE_EVENT:
        return 0;
    default:
        return false;
    }
}

static

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
            os_taskq_flush();
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


