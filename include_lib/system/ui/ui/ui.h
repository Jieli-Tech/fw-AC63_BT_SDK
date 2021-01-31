#ifndef UI_CORE_H
#define UI_CORE_H

#include "window.h"
#include "ui_button.h"
#include "ui_grid.h"
#include "ui_time.h"
#include "ui_camera.h"
#include "ui_pic.h"
#include "ui_text.h"
#include "ui_battery.h"
#include "ui_browser.h"
#include "ui_slider.h"
#include "ui_slider_vert.h"
#include "ui_number.h"
#include "ui_watch.h"
#include "ui_progress.h"
#include "ui_progress_multi.h"
#include "ui_rotate.h"
#include <stdarg.h>

struct uimsg_handl {
    const char *msg;
    int (*handler)(const char *type, u32 args);
};

int ui_framework_init(void *);

int ui_set_style_file(struct ui_style *style);

int ui_style_file_version_compare(int version);

int ui_redraw(int id);

int ui_show(int id);

int ui_hide(int id);

int ui_set_call(int (*func)(int), int param);

int ui_event_onkey(struct element_key_event *e);

int ui_event_ontouch(struct element_touch_event *e);

struct element *ui_get_highlight_child_by_id(int id);
int ui_invert_element_by_id(int id);

int ui_no_highlight_element(struct element *elm);
int ui_no_highlight_element_by_id(int id);
int ui_highlight_element(struct element *elm);
int ui_highlight_element_by_id(int id);

int ui_get_current_window_id();

int ui_register_msg_handler(int id, const struct uimsg_handl *handl);

int ui_message_handler(int id, const char *msg, va_list);

const char *str_substr_iter(const char *str, char delim, int *iter);

int ui_get_child_by_id(int id, int (*event_handler_cb)(void *, int, int));

int ui_set_default_handler(int (*ontouch)(void *, struct element_touch_event *),
                           int (*onkey)(void *, struct element_key_event *),
                           int (*onchange)(void *, enum element_change_event, void *));

/*
 * 锁定元素elm之外的区域，所有的触摸消息都发给elm
 */
void ui_ontouch_lock(void *elm);
void ui_ontouch_unlock(void *elm);

/*
 * 锁定控件的夫图层，先不推向imb显示
 */
int ui_lock_layer(int id);
int ui_unlock_layer(int id);

int ui_get_disp_status_by_id(int id);

#endif

