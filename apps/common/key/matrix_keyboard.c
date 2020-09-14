#include "matrix_keyboard.h"
#include "key_driver.h"
#include "gpio.h"
#include "system/event.h"
#include "app_config.h"
#include "timer.h"
#include "asm/power_interface.h"


#define MATRIX_NO_KEY           0x1         //高电平表示没有按键按下
#define MATRIX_LONG_TIME        35
#define MATRIX_HOLD_TIME        35+10
#define MATRIX_KEY_FILTER_TIME  2

int (*matrix_key_data_send)(u8 report_id, u8 *data, u16 len) = NULL;

static volatile u8 is_key_active = 0;
static int matrix_key_timer = 0;
static matrix_key_st key_st[ROW_MAX][COL_MAX];
static matrix_key_param *this = NULL;

u8 notify_key_array[8] = {0};
u8 key_map[COL_MAX] = {0};
static u16 key_status_array[6] = {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff};


static void full_key_array(u8 row, u8 col, u8 st)
{
    u8 offset = 0;
    u8 mark = 0, mark_offset = 0;
    //最多推6个按键出来，如果需要推多个按键需要自行修改，每个u16 低八位标识row 高八位标识col
    u16 key_value = (row | col << 8);
    for (offset = 0; offset < sizeof(key_status_array) / sizeof(u16); offset++) {
        if (key_status_array[offset] == key_value && st == MATRIX_KEY_UP) {         //找到列表中有当前按键且按键状态抬起,则将按键移除列表
            mark = 1;                                                               //记录相同键值所在位置
            mark_offset = offset;
            if (mark_offset == sizeof(key_status_array) / sizeof(u16) - 1) {
                key_status_array[mark_offset] = 0xffff;
                break;
            }
        } else if (key_status_array[offset] == 0xffff) {
            if (mark) {
                memcpy(key_status_array + mark_offset, key_status_array + mark_offset + 1, (offset - mark_offset - 1) * 2);
                key_status_array[offset - 1] = 0xffff;
            } else if (st == MATRIX_KEY_SHORT) {                                    //需要状态为短按才会把键值填充到数组中，防止按键满了之后把抬起也填充进去
                key_status_array[offset] = key_value;
            }
            break;
        } else if (mark && (offset == sizeof(key_status_array) / sizeof(u16) - 1)) {
            memcpy(key_status_array + mark_offset, key_status_array + mark_offset + 1, (sizeof(key_status_array) / sizeof(u16) - mark_offset - 1) * 2);
            key_status_array[sizeof(key_status_array) / sizeof(u16) - 1] = 0xffff;
            break;
        }
    }
}


void full_key_map(u8 row, u8 col, u8 st)
{
    if (st) {
        key_map[col] |= BIT(row);
    } else {
        key_map[col] &= ~BIT(row);
    }
}

#if 1
void matrix_key_scan(void)
{
    u8 row, col;
    u8 cur_key_value = MATRIX_NO_KEY, notify = 0;
    for (col = 0; col < this->col_num; col ++) {
        gpio_direction_input(this->col_pin_list[col]);
        gpio_set_die(this->col_pin_list[col], 1);
        gpio_set_pull_up(this->col_pin_list[col], 0);
        gpio_set_pull_down(this->col_pin_list[col], 0);
    }

    for (row = 0; row < this->row_num; row ++) {
        gpio_direction_input(this->row_pin_list[row]);
        gpio_set_die(this->row_pin_list[row], 1);
        gpio_set_pull_up(this->row_pin_list[row], 1);
        gpio_set_pull_down(this->row_pin_list[row], 0);
    }


    for (col = 0; col < this->col_num; col ++) {
        gpio_direction_output(this->col_pin_list[col], 0);
        gpio_set_die(this->col_pin_list[col], 1);
        for (row = 0; row < this->row_num; row ++) {
            cur_key_value = gpio_read(this->row_pin_list[row]);
            /* if (cur_key_value != (key_st[row][col]).filter_value && MATRIX_KEY_FILTER_TIME) {	//当前按键值与上一次按键值如果不相等, 重新消抖处理, 注意filter_time != 0; */
            /*     (key_st[row][col]).filter_cnt = 0; 		//消抖次数清0, 重新开始消抖 */
            /*     (key_st[row][col]).filter_value = cur_key_value;	//记录上一次的按键值 */
            /*     continue; 		//第一次检测, 返回不做处理 */
            /* } 		//当前按键值与上一次按键值相等, filter_cnt开始累加; */
            /* if (key_st[row][col].filter_cnt < MATRIX_KEY_FILTER_TIME) { */
            /*     key_st[row][col].filter_cnt++; */
            /*     continue; */
            /* } */

            if (cur_key_value != (key_st[row][col]).last_st) {
                if (cur_key_value == MATRIX_NO_KEY) { //按键抬起判断
                    (key_st[row][col]).press_cnt = 0;
                    //printf("row:%d  col:%d   [UP]\n", row, col);
                    full_key_map(row, col, 0);
                    //full_key_array(row, col, MATRIX_KEY_UP);
                    notify = 1;
                } else {            //按键初次按下
                    //printf("row:%d  col:%d   [SHORT]\n", row, col);
                    full_key_map(row, col, 1);
                    //full_key_array(row, col, MATRIX_KEY_SHORT);
                    notify = 1;
                    is_key_active = 100;
                }
            } else {        //判断是否按键抬起
                if (cur_key_value != MATRIX_NO_KEY) {
                    (key_st[row][col]).press_cnt ++;
                    if ((key_st[row][col]).press_cnt == MATRIX_LONG_TIME) {
                        /* printf("row:%d  col:%d   [LONG]\n", row, col);     */
                    } else if ((key_st[row][col]).press_cnt == MATRIX_HOLD_TIME) {
                        /* printf("row:%d  col:%d   [HOLD]\n", row, col);     */
                        (key_st[row][col]).press_cnt = MATRIX_LONG_TIME;
                    } else {
                        //press_cnt还未累加够
                    }
                }
            }
            (key_st[row][col]).last_st = cur_key_value;
        }
        gpio_direction_input(this->col_pin_list[col]);
        gpio_set_pull_up(this->col_pin_list[col], 0);
        gpio_set_pull_down(this->col_pin_list[col], 0);
        gpio_set_die(this->col_pin_list[col], 0);
    }

    for (col = 0; col < this->col_num; col ++) {
        gpio_direction_output(this->col_pin_list[col], 0);
    }

    if (notify) {
        //Phantomkey_process();
        struct sys_event e;
        e.type = SYS_MATRIX_KEY_EVENT;
        e.u.matrix_key.map = key_map;
        //memcpy((e.u.matrix_key.args), key_status_array, sizeof(key_status_array));

        e.arg  = (void *)DEVICE_EVENT_FROM_KEY;
        sys_event_notify(&e);
    }
    if (is_key_active) {
        is_key_active--;
    }
}
#else

void matrix_key_scan(void)
{
    u8 row, col;
    u8 cur_key_value = MATRIX_NO_KEY, notify = 0;
    for (row = 0; row < this->row_num; row ++) {
        gpio_direction_input(this->row_pin_list[row]);
        gpio_set_die(this->row_pin_list[row], 1);
        gpio_set_pull_up(this->row_pin_list[row], 1);
        gpio_set_pull_down(this->row_pin_list[row], 0);
    }

    for (col = 0; col < this->col_num; col ++) {
        gpio_direction_output(this->col_pin_list[col], 0);
        gpio_set_die(this->col_pin_list[col], 1);
        for (row = 0; row < this->row_num; row ++) {
            if (gpio_read(this->row_pin_list[row]) == 0) {
                printf("col:%x  row:%x\n", col, row);
            }

        }
        gpio_direction_input(this->col_pin_list[col]);
        gpio_set_pull_up(this->col_pin_list[col], 0);
        gpio_set_pull_up(this->col_pin_list[col], 0);
        gpio_set_die(this->col_pin_list[col], 0);
    }
}
#endif

int matrix_key_init(matrix_key_param *param)
{
    if (!param) {
        return -1;      //参数无效
    }
    this = param;
    u8 row, col;
    for (row = 0; row < this->row_num; row ++) {
        for (col = 0; col < this->col_num; col ++) {
            (key_st[row][col]).last_st = MATRIX_NO_KEY;
            (key_st[row][col]).press_cnt = 0;
        }
    }
    printf("key_row:%d rol:%d\n", this->row_num, this->col_num);
    if (matrix_key_timer) {
        sys_s_hi_timer_del(matrix_key_timer);
    }
    matrix_key_timer = sys_s_hi_timer_add(NULL, matrix_key_scan, 10);
    return 0;
}

void matrix_key_register_data_send_hdl(int (*data_send_hdl)(u8 report_id, u8 *data, u16 len))
{
    matrix_key_data_send = data_send_hdl;
}

static u8 matrix_key_idle_query(void)
{
    return !is_key_active;
}

REGISTER_LP_TARGET(matrix_key_lp_target) = {
    .name = "matrix_key",
    .is_idle = matrix_key_idle_query,
};
