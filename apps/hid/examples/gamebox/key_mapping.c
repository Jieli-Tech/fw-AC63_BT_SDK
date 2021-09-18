#include "system/app_core.h"
#include "system/includes.h"
#include "server/server_core.h"
#include "app_config.h"
#include "app_action.h"
#include "vm.h"

#if(CONFIG_APP_GAMEBOX)

#include "gamebox.h"

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE

#include "debug.h"

static u32 get_key_map(u8 key, struct point_t **t);

struct point_list_t {
    struct list_head entry;
    struct point_t point;
};

#define     POINT_SIZE  (1*32)
#if     POINT_SIZE == 64
#define     BITL(n) (1ULL << (n))
static u64 point_pool_used;
#elif POINT_SIZE == 32
#define     BITL(n) (1UL << (n))
static u32 point_pool_used;
#else
#error "POINT_SIZE muse = 8 16 32 64"
#endif

struct  key_index_t {
    u8 key;
    u8 cnt;
    u16 offset;
} _GNU_PACKED_;

#if 0
struct key_map_t {
    struct point_t awsd_mouse_map_array[10];
    u16 numOfkey;
    struct key_index_t key_index[numOfkey];
    u16 array_size;
    struct point_t map_array[array_size];
};
#endif

#define     AWSD_POINT_CID  1
#define     MOUSE_XY_CID    2
#define     MOUSE_BTN_CID   3
#define     MOUSE_WHEEL_CID 4
#define     KEY_POINT_CID   5
#define     MACRO_POINT_CID   6

#define     AWSD_IDX    0
#define     XY_IDX      1
#define     WHEEL_IDX   2
#define     LEFT_BTN    3
#define     RIGHT_BTN   4
#define     MOUSE_BTN4  5
#define     MOUSE_BTN5  6
#define     MOUSE_BTN6  7
#define     MOUSE_BTN7  8
#define     MOUSE_BTN8  9

static struct point_t mouse_xy_pos;
struct point_t mouse_wheel_pos ;

static u16 numOfkey;
static struct key_index_t *key_index;
static struct point_t *point_map_array;
static struct point_t awsd_mouse_map_array[10];
#define     VM_ITEM_MACRO_MAP 95
#define     VM_ITEM_KEY_MAP 96
static struct point_list_t point_pool[POINT_SIZE];
static struct point_list_t point_list;
static u32 max_list_size = 0;
static u32 list_size = 0;
static u8 shoot_continue_mode = 1;
static u8 shoot_fix = 1;
static u8 shoot_fix_speed = 5;
static u32 auto_shoot_timer_id = 0;
static u16 auto_shoot_speed = 3;
static u32 shoot_cnt = 0;
static u8 auto_shoot_stop = 1;
static u32 mouse_time_out_id = 0;
static u32 wheel_time_out_id = 0;
#define     MOVE_IDLE   0
#define     MOVE_DOWN   1
#define     MOVE_ING    2
#define     MOVE_UP     3

static u8 mouse_xy_new_data = 0;
static u8 mouse_xy_state = 0;
static s16 mouse_x_offset = 0;
static s16 mouse_y_offset = 0;

static u8 mouse_wheel_new_data = 0;
static u8 mouse_wheel_state = 0;
static s16 wheel_offset = 0;
static u32 phone_connect_status;

u32 get_phone_connect_status()
{
    return phone_connect_status;
}

void set_phone_connect_status(u32 status)
{
    if (status) {
        set_run_mode(status);
    } else {
        set_run_mode(UT_DEBUG_MODE);
    }

    phone_connect_status = status;
}
static void *point_mem_alloc()
{
    void *l = NULL;
    local_irq_disable();
    for (int i = 0; i < POINT_SIZE; i++) {
        if ((point_pool_used & BITL(i)) == 0) {
            point_pool_used |= BITL(i);
            l = &point_pool[i];
            break;
        }
    }
    local_irq_enable();
    ASSERT(l != NULL, "mem error %x %d", point_pool_used, max_list_size);
    return l;
}
static void point_mem_free(void *l)
{
    local_irq_disable();
    for (int i = 0; i < POINT_SIZE; i++) {
        if (l == &point_pool[i]) {
            point_pool_used &= ~(BITL(i));
            break;
        }
    }
    local_irq_enable();
}
void point_list_empty()
{
    local_irq_disable();
    point_pool_used = 0;
    INIT_LIST_HEAD(&point_list.entry) ;
    local_irq_enable();
}
static void point_list_push(struct point_t *p, u32 cid)
{
    /* log_d("push cid %x x: %d y: %d", cid, p->x, p->y) */
    struct point_list_t *it = point_mem_alloc();

    memcpy(&it->point, p, sizeof(*p));
    it->point.cid = cid;

    local_irq_disable();
    list_add_tail(&(it->entry), &(point_list.entry));
    list_size++;
    if (list_size > max_list_size) {
        max_list_size = list_size;
    }
    local_irq_enable();
}

u32 point_list_pop(struct touch_screen_t *t)
{
    struct list_head *p, *q;
    struct point_list_t *it = NULL;
    u32 cnt = 0;
    u32 cid = 0;

    local_irq_disable();

    if (mouse_wheel_new_data) {
        cid = MOUSE_WHEEL_CID;
        memcpy(&t->p[0], &mouse_wheel_pos, sizeof(t->p[0]));
        if (mouse_wheel_pos.tip_switch == 0) {
            memcpy(&mouse_wheel_pos, &awsd_mouse_map_array[WHEEL_IDX], sizeof(mouse_wheel_pos));
            mouse_wheel_state = MOVE_IDLE;
        } else {
            mouse_wheel_pos.x -= wheel_offset * 70;
            wheel_offset = 0;
            mouse_wheel_state = MOVE_ING;
        }
        cnt++;
        mouse_wheel_new_data = 0;
    }

    if (mouse_xy_new_data) {
        cid = MOUSE_XY_CID;
        memcpy(&t->p[cnt], &mouse_xy_pos, sizeof(t->p[0]));
        if (mouse_xy_pos.tip_switch == 0) {
            memcpy(&mouse_xy_pos, &awsd_mouse_map_array[XY_IDX], sizeof(mouse_xy_pos));
            mouse_xy_state = MOVE_IDLE;
        } else {
            mouse_xy_pos.x -= mouse_y_offset;
            mouse_xy_pos.y += mouse_x_offset;
            mouse_xy_state = MOVE_ING;

            mouse_y_offset = 0;
            mouse_x_offset = 0;
        }
        cnt++;
        mouse_xy_new_data = 0;
    }


    if (cnt != 2) {
        list_for_each_safe(p, q, &(point_list.entry)) {

            it = list_entry(p, struct point_list_t, entry);

            if (cnt == 1) {
                if (cid == it->point.cid) {
                    break;
                }
            }

            cid = it->point.cid;

            memcpy(&t->p[cnt], &it->point, sizeof(t->p[0]));

            list_del(p);

            point_mem_free(it);

            list_size--;
            cnt++;

            if (cnt == 2) {
                break;
            }
        }
    }

    local_irq_enable();

    if (cnt) {
        /* printf("\ntip: %d cid: %d x:%d y:%d "                          */
        /*        "tip: %d cid: %d x:%d y:%d\n",                          */
        /*        t->p[0].tip_switch, t->p[0].cid, t->p[0].x, t->p[0].y,  */
        /*        t->p[1].tip_switch, t->p[1].cid, t->p[1].x, t->p[1].y); */
        return 1;
    } else {
        return 0;
    }
}
struct macro_point_t {
    u8 end;
    u8 delay_ms;
    struct point_t p;
};
static u8 macro_delay;
struct macro_point_t *macro_list = NULL;
static void macro_timer(void *arg)
{
    if (macro_list && macro_list->end) {
        return;
    }
    if (macro_delay++ == macro_list->delay_ms) {
        point_list_push(&(macro_list->p), MACRO_POINT_CID);
        macro_list++;
    }
}
static struct macro_point_t *get_macro_list(u8 key)
{
    return NULL;
}
void play_macro(u8 key)
{
    macro_list = get_macro_list(key);
    sys_timer_add(NULL, macro_timer, 20);
}
static int fun_key_mapping(char key, u8 is_press)
{
    struct point_t *p;
    if (key >= _KEY_F1 && key <= _KEY_F12) {

        switch (key) {
        case _KEY_F1:
            if (is_press) {
                shoot_continue_mode = ! shoot_continue_mode;
            }
            break;

        case _KEY_F2:
            if (is_press) {
                shoot_fix = ! shoot_fix;
            }
            break;
        case _KEY_F3:
            if (is_press) {
                shoot_fix_speed ++;
            }
            break;

        case _KEY_F4:
            if (is_press) {
                if (shoot_fix_speed > 1) {
                    shoot_fix_speed --;
                }
            }
            break;

        case _KEY_F5:
            if (is_press) {
                auto_shoot_speed++;
            }
            break;

        case _KEY_F6:
            if (is_press) {
                if (auto_shoot_speed > 1) {
                    auto_shoot_speed --;
                }
            }
            break;
        default:
            return 0;
        }
    }
    return 1;
}

/* #define     X_Y(x,y)    LOBYTE((x)),HIBYTE((x)),LOBYTE((y)),HIBYTE((y)) */

#define     AWSD_SIZE   4
#define     X_DIV       160
#define     Y_DIV       300

static u8 last_awsd_key_0 = 0;
static u8 last_awsd_key_1 = 0;
static s32 x_div = 0;
static s32 y_div = 0;
static struct point_t awsd_pos;

static void aswd_timer(void *arg)
{

}

static int aswd_mapping(char key, u8 is_press)
{
    memcpy(&awsd_pos, &awsd_mouse_map_array[AWSD_IDX], sizeof(awsd_pos));

    struct point_t *p = &awsd_pos;
    y_div = 0;
    x_div = 0;
    u32 cnt = 0;
    if (is_press) {
        cnt = AWSD_SIZE - 1;
        switch (key) {
        case _KEY_A:
            y_div = -Y_DIV;
            break;
        case _KEY_S:
            x_div = -X_DIV;
            break;
        case _KEY_D:
            y_div = Y_DIV;
            break;
        case _KEY_W:
            x_div = 3 * X_DIV;
            break;
        default:
            return 0;
        }

        last_awsd_key_0 = last_awsd_key_1;
        last_awsd_key_1 = key;

        if (last_awsd_key_0 && last_awsd_key_1) {
            cnt = 1;
            if ((last_awsd_key_0 == _KEY_A && last_awsd_key_1 == _KEY_W) ||
                (last_awsd_key_0 == _KEY_W && last_awsd_key_1 == _KEY_A)) {
                y_div = -3 * Y_DIV;
                x_div = 3 * X_DIV;
            } else if ((last_awsd_key_0 == _KEY_A && last_awsd_key_1 == _KEY_S) ||
                       (last_awsd_key_0 == _KEY_S && last_awsd_key_1 == _KEY_A)) {
                y_div = -3 * Y_DIV;
                x_div = -3 * X_DIV;
            } else if ((last_awsd_key_0 == _KEY_W && last_awsd_key_1 == _KEY_D) ||
                       (last_awsd_key_0 == _KEY_D && last_awsd_key_1 == _KEY_W)) {
                y_div = 3 * Y_DIV;
                x_div = 3 * X_DIV;
            } else if ((last_awsd_key_0 == _KEY_D && last_awsd_key_1 == _KEY_S) ||
                       (last_awsd_key_0 == _KEY_S && last_awsd_key_1 == _KEY_D)) {
                y_div = 3 * Y_DIV;
                x_div = -3 * X_DIV;
            }
            p->x += x_div;
            p->y += y_div;
        }

        p->tip_switch = 3;
        for (int j = 0; j < cnt ; j++) {
            point_list_push(p, AWSD_POINT_CID);
            p->x += x_div;
            p->y += y_div;
        }

    } else {
        p->tip_switch = 0;
        switch (key) {
        case _KEY_A:
            y_div = -3 * Y_DIV;
            break;
        case _KEY_S:
            x_div = -3 * X_DIV;
            break;
        case _KEY_D:
            y_div = 3 * Y_DIV;
            break;
        case _KEY_W:
            x_div = 3 * X_DIV;
            break;
        default:
            return 0;
        }
        if (last_awsd_key_0 == 0) { //单个按键
            last_awsd_key_1 = 0;
        } else { //两个方向键，释放了其中一个
            if (last_awsd_key_0 == key) {
                key = last_awsd_key_1;
            } else if (last_awsd_key_1 == key) {
                last_awsd_key_1 = last_awsd_key_0;
                key = last_awsd_key_0;
            }

            switch (key) {
            case _KEY_A:
                x_div = 0;
                y_div = -3 * Y_DIV;
                break;
            case _KEY_S:
                y_div = 0;
                x_div = -3 * X_DIV;
                break;
            case _KEY_D:
                x_div = 0;
                y_div = 3 * Y_DIV;
                break;
            case _KEY_W:
                y_div = 0;
                x_div = 6 * X_DIV;
                break;
            }

            last_awsd_key_0 = 0;
            p->tip_switch = 3;
        }

        p->x += x_div;
        p->y += y_div;
        point_list_push(p, AWSD_POINT_CID);
    }

    return 1;
}

static u8 auto_shoot_pause = 0;;
static int num_mapping(char key, u8 is_press)
{
    if (is_press) {
        if (key == _KEY_4) {      //炸弹，取消自动射击和压枪
            auto_shoot_pause = 1;
        } else if (key < _KEY_4) {
            auto_shoot_pause = 0;
        }
    }

    if (key >= _KEY_1 && key <= _KEY_0) {
        struct point_t *p;
        u32 cnt = get_key_map(key, &p);
        for (int i = 0; i < cnt; i++) {
            p->tip_switch = is_press ? 3 : 0;
            point_list_push(p, KEY_POINT_CID);
            p++;
        }
        return cnt;
    }
    return 0;
}
static u8 last_fun_key = 0;
static void gui_fun_key_mapping(char fun_key)
{
    struct point_t *p;
    u8 change_key = last_fun_key ^ fun_key;
    u8 gui_key;
    for (int i = 0; i < 8; i++) {
        if (change_key & BIT(i)) {
            u8 is_press = 0;
            if (fun_key & BIT(i)) { //press
                is_press = 1;
            } else { //release
                /* is_press = 0; */
                continue;
            }

            gui_key = 0xe0 + i;
            u32 cnt = get_key_map(gui_key, &p);
            for (int i = 0; i < cnt; i++) {
                /* p->tip_switch = is_press ? 3 : 0; */
                p->tip_switch = 3;
                point_list_push(p, KEY_POINT_CID);
                p->tip_switch = 0;
                point_list_push(p, KEY_POINT_CID);
                p++;
            }
        }
    }
}


static int misc_key_mapping(char key, u8 is_press)
{
    if (key == _KEY_P) {
        log_i("max_list_size %d", max_list_size);
        log_i("auto shoot %d %d fix %d",
              shoot_continue_mode, auto_shoot_speed, shoot_fix_speed);
        return 0;
    }
    struct point_t *p;
    u32 cnt = 0;
    if (!is_press) {
        return 0;
    }
    cnt = get_key_map(key, &p);
    log_d("cnt %d", cnt);
    for (int j = 0; j < cnt ; j++) {
        p->tip_switch = 3;
        log_d("cid %x x: %d y: %d", p->cid, p->x, p->y);
        point_list_push(p, KEY_POINT_CID);
        p->tip_switch = 0;
        point_list_push(p, KEY_POINT_CID);
        p++;
    }
    return cnt;
}
static u8 last_press_key[6] = {0};
void key_mapping(const struct keyboard_data_t *k)
{
    if (get_phone_connect_status() == 0) {
        return;
    }
    char key;
    char new_key;

    gui_fun_key_mapping(k->fun_key);

    for (int i = 0; i < 6; i++) {
        key = last_press_key[i]        ;
        if (key) {
            if (memchr((const char *)(k->Keypad), key, 6) == NULL) {
                //release key
                do {
                    int r = aswd_mapping(key, 0);
                    if (r) {
                        break;
                    }

                    r = num_mapping(key, 0);
                    if (r) {
                        break;
                    }

                    r = misc_key_mapping(key, 0);
                    if (r) {
                        break;
                    }

                    r = fun_key_mapping(key, 0);
                    if (r) {
                        break;
                    }

                } while (0);
            }
        } else {
            break;
        }
    }

    for (int i = 0; i < 6; i++) {
        new_key = k->Keypad[i];
        if (new_key == 0) {
            break;
        }
        /* printf_buf(last_press_key, 6); */
        if (memchr((const char *)(last_press_key), new_key, 6)) {
            continue;
        }

        //press key
        do {
            int r = aswd_mapping(new_key, 1);
            if (r) {
                break;
            }

            r = num_mapping(new_key, 1);
            if (r) {
                break;
            }

            r = misc_key_mapping(new_key, 1);
            if (r) {
                break;
            }

            r = fun_key_mapping(new_key, 1);
            if (r) {
                break;
            }
        } while (0);

    }

    memcpy(last_press_key, k->Keypad, 6);
}
static void mouse_xy_release(void *p)
{
    if (shoot_cnt) {
        return;
    }
    if (MOVE_ING != mouse_xy_state) {
        return;
    }
    mouse_xy_pos.tip_switch = 0;
    mouse_xy_state = MOVE_UP;
    mouse_xy_new_data = 1;
}
static void mouse_wheel_release(void *p)
{
    if (MOVE_ING != mouse_wheel_state) {
        return;
    }

    mouse_wheel_pos.tip_switch = 0;
    mouse_wheel_state = MOVE_UP;
    mouse_wheel_new_data = 1;
}

static void auto_shoot_timer(void *arg)
{
    struct point_t *p;
    if (auto_shoot_stop == 0) {
        p = &awsd_mouse_map_array[LEFT_BTN];
        if (shoot_cnt % auto_shoot_speed == 0) {
            p->tip_switch = 3;
            point_list_push(p, MOUSE_BTN_CID);
            p->tip_switch = 0;
            point_list_push(p, MOUSE_BTN_CID);
        }
        shoot_cnt++;

        if (shoot_fix) {
            mouse_y_offset = shoot_fix_speed;
            mouse_xy_new_data = 1;
        }
    }
}
static u8 mouse_btn = 0;
void mouse_mapping(const struct mouse_data_t *m)
{
    if (get_phone_connect_status() == 0) {
        return;
    }
    struct point_t *p;
    u8 change_btn = mouse_btn ^ m->btn;
    mouse_btn = m->btn;

    if (change_btn & BIT(0)) {//left btn
        p = &awsd_mouse_map_array[LEFT_BTN];
        if ((auto_shoot_pause == 1) || (shoot_continue_mode == 0)) {
            if (m->btn & BIT(0)) {
                p->tip_switch = 3;
                point_list_push(p, MOUSE_BTN_CID);
            } else {
                p->tip_switch = 0;
                point_list_push(p, MOUSE_BTN_CID);
            }
        } else {
            if (m->btn & BIT(0)) {
                p->tip_switch = 3;
                point_list_push(p, MOUSE_BTN_CID);
                p->tip_switch = 0;
                point_list_push(p, MOUSE_BTN_CID);
                shoot_cnt = 1 ;
                if (auto_shoot_timer_id == 0) {
                    auto_shoot_timer_id = sys_timer_add(NULL, auto_shoot_timer, 20);
                }
                auto_shoot_stop = 0;
            } else {
                auto_shoot_stop = 1;
                shoot_cnt = 0 ;
                mouse_xy_pos.tip_switch = 0;
                mouse_xy_state = MOVE_UP;
                mouse_xy_new_data = 1;
            }
        }
    }


    if (change_btn & BIT(1)) {
        p = &awsd_mouse_map_array[RIGHT_BTN];
        if (m->btn & BIT(1)) { //right btn
            p->tip_switch = 3;
            point_list_push(p, MOUSE_BTN_CID);
            p->tip_switch = 0;
            point_list_push(p, MOUSE_BTN_CID);
        }
    }

    for (int i = 3; i < 8; i++) {
        if (change_btn & BIT(i)) {
            if (m->btn & BIT(i)) {
                p = &awsd_mouse_map_array[i - 3 + MOUSE_BTN4];
                p->tip_switch = 3;
                point_list_push(p, MOUSE_BTN_CID);
                p->tip_switch = 0;
                point_list_push(p, MOUSE_BTN_CID);
            }
        }
    }


    if (m->wheel) {
        wheel_offset += m->wheel;
        mouse_wheel_state = mouse_wheel_state ? MOVE_ING : MOVE_DOWN;
        mouse_wheel_new_data = 1;

        if (wheel_time_out_id) {
            sys_timer_modify(wheel_time_out_id, 400);
        } else {
            wheel_time_out_id = sys_timer_add(NULL, mouse_wheel_release, 400);
        }
    }

    if (m->x || m->y) {
        mouse_x_offset += m->x;
        mouse_y_offset += m->y;
        mouse_xy_state = mouse_xy_state ? MOVE_ING : MOVE_DOWN;
        mouse_xy_new_data = 1;

        if (mouse_time_out_id) {
            sys_timer_modify(mouse_time_out_id, 200);
        } else {
            mouse_time_out_id = sys_timer_add(NULL, mouse_xy_release, 200);
        }
    }

}
#define     USE_DEFAULT_MAP 1
#if USE_DEFAULT_MAP
static volatile u8 default_map[] = {
    0x03, 0x45, 0x03, 0x22, 0x04, 0x03, 0x4B, 0x06, 0xF2, 0x11, 0x03, 0xF3, 0x06, 0xAD, 0x0E, 0x03,
    0x30, 0x06, 0xD8, 0x01, 0x03, 0x21, 0x06, 0x0D, 0x15, 0x03, 0xa3, 0x06, 0x00, 0x14, 0x03, 0x62,
    0x07, 0x6D, 0x15, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x18, 0x00, 0x1E, 0x01, 0x00, 0x00, 0x1F, 0x01, 0x01, 0x00, 0x20, 0x01, 0x02, 0x00,
    0x21, 0x01, 0x03, 0x00, 0x22, 0x01, 0x04, 0x00, 0x05, 0x01, 0x05, 0x00, 0x06, 0x01, 0x06, 0x00,
    0x08, 0x01, 0x07, 0x00, 0x29, 0x02, 0x08, 0x00, 0x09, 0x02, 0x0A, 0x00, 0x0A, 0x01, 0x0C, 0x00,
    0x0B, 0x01, 0x0D, 0x00, 0x14, 0x01, 0x0E, 0x00, 0x15, 0x01, 0x0F, 0x00, 0x17, 0x01, 0x10, 0x00,
    0x19, 0x01, 0x11, 0x00, 0x1B, 0x02, 0x12, 0x00, 0x1D, 0x01, 0x14, 0x00, 0xE0, 0x01, 0x15, 0x00,
    0xE2, 0x01, 0x16, 0x00, 0xE1, 0x01, 0x17, 0x00, 0x2C, 0x01, 0x18, 0x00, 0x2B, 0x01, 0x19, 0x00,
    0x35, 0x01, 0x1A, 0x00, 0x1B, 0x00, 0x03, 0x71, 0x01, 0xBD, 0x09, 0x03, 0x5F, 0x01, 0x50, 0x0C,
    0x03, 0x31, 0x02, 0x85, 0x0D, 0x03, 0xD2, 0x00, 0xE0, 0x0E, 0x03, 0xCF, 0x00, 0x98, 0x07, 0x03,
    0xDD, 0x01, 0x30, 0x0F, 0x03, 0xD2, 0x00, 0xC8, 0x12, 0x03, 0x1C, 0x08, 0xE2, 0x04, 0x03, 0x06,
    0x06, 0x8A, 0x05, 0x03, 0x2A, 0x03, 0x50, 0x0B, 0x03, 0x28, 0x08, 0x45, 0x0F, 0x03, 0x11, 0x04,
    0x82, 0x0F, 0x03, 0x29, 0x07, 0x4A, 0x0F, 0x03, 0xD3, 0x05, 0x4A, 0x0F, 0x03, 0x28, 0x08, 0x4D,
    0x03, 0x03, 0xE7, 0x00, 0x5A, 0x11, 0x03, 0x54, 0x09, 0x18, 0x04, 0x03, 0xDA, 0x01, 0x65, 0x07,
    0x03, 0x8A, 0x09, 0x70, 0x0F, 0x03, 0xE0, 0x04, 0x6A, 0x0B, 0x03, 0x14, 0x01, 0xB5, 0x14, 0x03,
    0xF5, 0x07, 0x30, 0x14, 0x03, 0xCF, 0x00, 0x20, 0x05, 0x03, 0x5A, 0x03, 0xD0, 0x12, 0x03, 0xF3,
    0x03, 0x15, 0x15, 0x03, 0xFF, 0x00, 0x02, 0x02, 0x03, 0xD9, 0x0B, 0x52, 0x15, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x58
};
#endif
u32 key_map_init()
{
    numOfkey = 0;
    u32 item_len = 1024;
    u8 *map_data = malloc(item_len);
    u8 *const _map_data = map_data;
    int ret = syscfg_read(VM_ITEM_KEY_MAP, map_data, item_len);
    if (ret < 0) {
        log_e("vm read error %d %d", ret, VM_WRITE_OVERFLOW);
        free(map_data);
        return  1;
    }
    u16 crc_in_db = 0;
    memcpy(&crc_in_db, &map_data[1024 - 2], 2);
    if (CRC16(map_data, 1024 - 2) != crc_in_db) {
        free(map_data);
        log_e("vm read crc error %x", crc_in_db);
        log_info_hexdump(map_data, 1024);
        return 1;
    }
    log_d("vm read map ok %x", crc_in_db);

    item_len = sizeof(awsd_mouse_map_array);
    memcpy(awsd_mouse_map_array, map_data, item_len);
    map_data += item_len;

    struct point_t *t = NULL;

    awsd_mouse_map_array[AWSD_IDX].cid = AWSD_POINT_CID;
    awsd_mouse_map_array[XY_IDX].cid = MOUSE_XY_CID;
    awsd_mouse_map_array[WHEEL_IDX].cid = MOUSE_WHEEL_CID;
    awsd_mouse_map_array[LEFT_BTN].cid = MOUSE_BTN_CID;
    awsd_mouse_map_array[RIGHT_BTN].cid = MOUSE_BTN_CID;
    awsd_mouse_map_array[MOUSE_BTN4].cid = MOUSE_BTN_CID;
    awsd_mouse_map_array[MOUSE_BTN5].cid = MOUSE_BTN_CID;
    awsd_mouse_map_array[MOUSE_BTN6].cid = MOUSE_BTN_CID;
    awsd_mouse_map_array[MOUSE_BTN7].cid = MOUSE_BTN_CID;
    awsd_mouse_map_array[MOUSE_BTN8].cid = MOUSE_BTN_CID;

    for (int i = 0; i < 10; i++) {
        t =  &awsd_mouse_map_array[i];
        printf("tip: %d x:%d y:%d\n", t->tip_switch, t->x, t->y);
    }

    memcpy(&mouse_xy_pos, &awsd_mouse_map_array[XY_IDX], sizeof(mouse_xy_pos));
    memcpy(&mouse_wheel_pos, &awsd_mouse_map_array[WHEEL_IDX], sizeof(mouse_wheel_pos));

    item_len = 2;
    memcpy(&numOfkey, map_data, item_len);
    map_data += item_len;
    log_i("numOfkey %d ", numOfkey);

    item_len = sizeof(struct key_index_t) * numOfkey;
    key_index = malloc(item_len);
    memcpy(key_index, map_data, item_len);
    map_data += item_len;

    u16 array_size;
    item_len = 2;
    memcpy(&array_size, map_data, item_len);
    map_data += item_len;

    item_len = sizeof(struct point_t) * array_size;
    point_map_array = malloc(item_len);
    memcpy(point_map_array, map_data, item_len);

    free(_map_data);
    for (int i = 0; i < numOfkey; i++) {
        t =  &point_map_array[key_index[i].offset];
        printf("key: %x cnt:%d offset:%d\n", key_index[i].key, key_index[i].cnt, key_index[i].offset);
        printf("tip: %d x:%d y:%d\n", t->tip_switch, t->x, t->y);

    }
    return 0;
}
static void key_map_save(u8 *p, u32 len)
{
    u16 crc_in_db = CRC16(p, 1024 - 2);
    memcpy(&p[1024 - 2], &crc_in_db, 2);
    log_info_hexdump(p, 1024);
    log_d("vm write crc %x", crc_in_db);
    int r = syscfg_write(VM_ITEM_KEY_MAP, p, 1024);
    if (r != 1024) {
        log_e("%s %d", __func__, r);
    }
}

static u32 get_key_map(u8 key, struct point_t **t)
{
    u32 cnt = 0;
    for (int i = 0; i < numOfkey; i++) {
        /* log_d("key %x %x",key,key_index[i].key); */
        if (key == key_index[i].key) {
            cnt = key_index[i].cnt;
            *t =  &point_map_array[key_index[i].offset];
            break;
        }
    }
    return cnt;
}
void key_list_init()
{
    point_list_empty();
    if (key_map_init()) {
#if USE_DEFAULT_MAP
        key_map_save(default_map, 1024);
        key_map_init();
#endif
    }
}
void ble_hid_transfer_channel_recieve(u8 *packet, u16 size)
{
    log_info("transfer_rx(%d):", size);
    /* log_info_hexdump(packet, size); */
    static u8 *map_data = NULL;
    static u32 key_map_offset = 0;
    if (strstr(packet, "kmst") != NULL) {
        map_data = zalloc(1024);
        key_map_offset = 0;
    } else if (strstr(packet, "kmed") != NULL) {
        key_map_save(map_data, 1024);
        free(map_data);
        map_data = NULL;

        if (numOfkey) {
            free(point_map_array);
            free(key_index);
            key_index = NULL;
            point_map_array = NULL;
            numOfkey = 0;
        }
        key_map_init();
        set_run_mode(BT_MODE);
    } else if (map_data) {
        if (key_map_offset + size > 1024) {
            log_e("key_map_offset error", key_map_offset);
        } else {
            memcpy(&map_data[key_map_offset], packet, size);
            key_map_offset += size;
        }
    }
    if (strstr(packet, "mpst") != NULL) {
        set_run_mode(MAPPING_MODE);
    }
}
#endif
