#include "system/includes.h"
#include "code_switch.h"
#include "app_config.h"
#include "stdlib.h"

#if TCFG_CODE_SWITCH_ENABLE

#if 1
#define log_debug          printf
#else
#define log_debug(...)
#endif

#define INPUT_CHANNLE6_SRC_SEL(x)			SFR(JL_IOMAP->CON4, 16, 6, x)
#define INPUT_CHANNLE7_SRC_SEL(x)			SFR(JL_IOMAP->CON4, 24, 6, x)

static SW_PLATFORM_DATA *sw_pdata = NULL;
static u8 code_switch_active = 0;

static void code_switch_event_to_usr(u8 event, s8 sw_val);
static void get_code_value_handler(void *priv);
/* static void get_code_value_handler(void); */
static u8 code_switch_idle_query(void);


static void code_switch_event_to_usr(u8 event, s8 sw_val)
{
    struct sys_event e;
    e.type                = SYS_DEVICE_EVENT;
    e.arg                 = "code_switch";
    e.u.codesw.event      = event;
    e.u.codesw.value      = sw_val;
    sys_event_notify(&e);
}

/* ___interrupt */
/* static void get_code_value_handler(void) */
/* { */
/* 	if (JL_RDEC->CON & 0x80) */
/* 	{ */
/* 		mouse_code_switch_event_to_usr(0, JL_RDEC->DAT); */
/* 		JL_RDEC->CON |= BIT(6); */
/* 	} */
/* }	 */
/*  */

u8 code_postive_list[] = {
    ((0x00 << 4) | (0x02 << 2) | (0x03 << 0)), \
    ((0x00 << 4) | (0x02 << 2) | (0x01 << 0)), \
    ((0x00 << 4) | (0x03 << 2) | (0x01 << 0)), \

    ((0x02 << 4) | (0x03 << 2) | (0x01 << 0)), \
    ((0x02 << 4) | (0x03 << 2) | (0x00 << 0)), \
    ((0x02 << 4) | (0x01 << 2) | (0x00 << 0)), \

    ((0x03 << 4) | (0x01 << 2) | (0x00 << 0)), \
    ((0x03 << 4) | (0x01 << 2) | (0x02 << 0)), \
    ((0x03 << 4) | (0x00 << 2) | (0x02 << 0)), \

    ((0x01 << 4) | (0x00 << 2) | (0x02 << 0)), \
    ((0x01 << 4) | (0x00 << 2) | (0x03 << 0)), \
    ((0x01 << 4) | (0x02 << 2) | (0x03 << 0))
};

u8 code_negative_list[] = {
    ((0x00 << 4) | (0x01 << 2) | (0x03 << 0)), \
    ((0x00 << 4) | (0x01 << 2) | (0x02 << 0)), \
    ((0x00 << 4) | (0x03 << 2) | (0x02 << 0)), \

    ((0x01 << 4) | (0x03 << 2) | (0x02 << 0)), \
    ((0x01 << 4) | (0x03 << 2) | (0x00 << 0)), \
    ((0x01 << 4) | (0x02 << 2) | (0x00 << 0)), \

    ((0x03 << 4) | (0x02 << 2) | (0x00 << 0)), \
    ((0x03 << 4) | (0x02 << 2) | (0x01 << 0)), \
    ((0x03 << 4) | (0x00 << 2) | (0x01 << 0)), \

    ((0x02 << 4) | (0x00 << 2) | (0x01 << 0)), \
    ((0x02 << 4) | (0x00 << 2) | (0x03 << 0)), \
    ((0x02 << 4) | (0x01 << 2) | (0x03 << 0))
};

//功能：相位纠正
static void correct_code(u8 *code_data, u8 *list_data, u8 idx)
{
    code_data[2] = (list_data[(idx / 3) * 3] & (0x03 << 4)) >> 4;
    code_data[1] = (list_data[(idx / 3) * 3] & (0x03 << 2)) >> 2;
    code_data[0] = (list_data[(idx / 3) * 3] & (0x03 << 0)) >> 0;

    log_debug("lost_phase_recover: %d  %d  %d\n", code_data[2], code_data[1], code_data[0]);
}

//功能：相位遍历
static u8 traverse_phase_list(u8 *code_data, u8 *list_data, u8 size)
{
    u8 code_num = 0, idx = 0;

    code_num = code_data[2] << 4 | code_data[1] << 2 | code_data[0] << 0;

    for (idx = 0; idx < size; idx++) {
        if (code_num == list_data[idx]) {
            if (idx % 3) {
                /* correct_code(code_data, list_data, idx); */
            }
            break;
        }
    }

    return idx;
}

//功能：消抖
static bool code_phase_debounce(u8 code_val)
{
    static u8 filter_value = 0, filter_cnt = 0;
    const u8 filter_time = 2;

    //当前值与上一次值如果不相等, 重新消抖处理, 注意filter_time != 0;
    if (code_val != filter_value && filter_time) {
        filter_cnt = 0; 		//消抖次数清0, 重新开始消抖
        filter_value = code_val;//记录上一次的值
        return false; 		    //第一次检测, 返回不做处理
    }

    //当前值与上一次值相等, filter_cnt开始累加;
    if (filter_cnt < filter_time) {
        filter_cnt++;
        return false;
    }

    return true;
}

//功能：获取相位
static bool get_code_phase(u8 *code_data)
{
    static u8 code_now = 0, code_last = 0;
    static u8 debug_count = 0;

    //获取相位值
    code_last = code_now;
    code_now = (gpio_read(sw_pdata->a_phase_io) ? 0x02 : 0x00) + (gpio_read(sw_pdata->b_phase_io) ? 0x01 : 0x00);

    //相位状态出现变化，将相位值存入队列
    if (code_last != code_now && code_now != code_data[0]) {
        code_data[2] = code_data[1];
        code_data[1] = code_data[0];
        code_data[0] = code_now;

        code_switch_active = 200;//编码开关工作状态标志位

        /* log_debug(">>>%d<<<%d  %d  %d\n", debug_count, code_data[2], code_data[1], code_data[0]); */
        debug_count++;
        return true;
    }

    //相位状态无变化，编码开关工作状态标志位递减
    else {
        if (code_switch_active) {
            code_switch_active--;
        }
    }

    return false;
}

//功能：获取编码值
u8 get_code_value(u8 *code_data)
{
    u8 idx = 0;   //相位序列号
    static u8 code_val = 0;
    static u8 move_now = 0, move_last = 0;
    static u8 idx_pos_last = 0, idx_neg_last = 0;
    enum {
        MOVE_POS,
        MOVE_NEG,
    };

    //正相位遍历，获取相位序列号
    idx = traverse_phase_list(code_data, &code_postive_list[0], ARRAY_SIZE(code_postive_list));

    //相位序列号有效，则出现正向滚动动作
    if (idx < ARRAY_SIZE(code_postive_list)) {
        move_last = move_now;
        move_now = MOVE_POS;

        //相位区域限制，避免重复检测
        if ((idx >= 0 && idx <= 2) || (idx >= 6 && idx <= 8) || (move_now != move_last))
            /* if (abs(idx_pos_last/3-idx/3)>=2 || (move_now != move_last)) */
        {
            code_val++; //正向滚动，编码值+1
            idx_pos_last = idx;
            /* log_debug(">>>>>>>>>>>>>>>>>>>>>>>idx = %d  R\n", idx); */
            /* log_debug("\n"); */
            return code_val;
        }
    }


    //负相位遍历，获取相位序列号
    idx = traverse_phase_list(code_data, &code_negative_list[0], ARRAY_SIZE(code_negative_list));

    //相位序列号有效，则出现负向滚动
    if (idx < ARRAY_SIZE(code_negative_list)) {
        move_last = move_now;
        move_now = MOVE_NEG;

        //相位区域限制，避免重复检测
        if ((idx >= 3 && idx <= 5) || (idx >= 9 && idx <= 11) || (move_now != move_last))
            /* if (abs(idx_neg_last/3-idx/3)>=2 || (move_now != move_last)) */
        {
            code_val--; //负向滚动，编码值-1
            idx_neg_last = idx;
            /* log_debug(">>>>>>>>>>>>>>>>>>>>>>>idx = %d  L\n", idx); */
            /* log_debug("\n"); */

            return code_val;
        }
    }

    return code_val;
}

//功能：旋转编码开关检测
static u8 code_switch_detector(void)
{
    static u8 code_table[3] = {0}; //相位值存储队列
    static u8 code_val = 0;

    //相位有变化，进行相位遍历，获取编码值
    if (get_code_phase(&code_table[0])) {
        code_val = get_code_value(&code_table[0]);
    }

    //相位无变化，编码值保持不变
    else {
        code_val = code_val;
    }

    return code_val;
}


//功能：旋转编码开关数据采集
static void code_switch_handler(void *priv)
{
    static u8 code_val_last = 0;
    static u8 code_val_now  = 0;

    code_val_last = code_val_now;
    code_val_now = code_switch_detector();

    if (code_val_last != code_val_now) {
        code_switch_event_to_usr(0, code_val_now - code_val_last);
    }

    /* if (JL_RDEC->DAT != code) */
    /* { */
    /* 	code_switch_event_to_usr(0, JL_RDEC->DAT - code); */
    /*     code_switch_active = 10; */
    /* } */
    /*  */
    /* else */
    /* { */
    /*     code_switch_active--; */
    /* } */
    /*  */
    /* code = JL_RDEC->DAT; */


    /* pnd = JL_RDEC->CON & 0x80; */
    /* if (pnd) */
    /* { */
    /* code_switch_event_to_usr(0, JL_RDEC->DAT); */
    /* JL_RDEC->CON |= BIT(6); */
    /* putchar('Y'); */
    /* CODE_IO_DEBUG_TOGGLE(A,2) */
    /* } */
}

void code_switch_init(SW_PLATFORM_DATA *priv)
{
    sw_pdata = priv;

    /* JL_RDEC->CON |= BIT(0); //Enable RDEC */
    /* JL_RDEC->CON &= ~BIT(1);//pull up */
    /* JL_RDEC->CON |= 15<<2; */

    /* JL_IOMAP->CON1 &= ~BIT(12);  //RDES_IOSO = PB4 */
    /* JL_IOMAP->CON1 &= ~BIT(13);  //RDES_IOS1 = PB5 */

    /* JL_IOMAP->CON1 |= BIT(12); */
    /* JL_IOMAP->CON1 |= BIT(13); */

    /* INPUT_CHANNLE6_SRC_SEL(sw_pdata->a_phase_io); */
    /* INPUT_CHANNLE7_SRC_SEL(sw_pdata->b_phase_io); */

    gpio_set_die(sw_pdata->a_phase_io, 1);
    gpio_set_dieh(sw_pdata->a_phase_io, 1);
    gpio_set_direction(sw_pdata->a_phase_io, 1);
    gpio_set_pull_up(sw_pdata->a_phase_io, 0);
    gpio_set_pull_down(sw_pdata->a_phase_io, 0);

    gpio_set_die(sw_pdata->b_phase_io, 1);
    gpio_set_dieh(sw_pdata->b_phase_io, 1);
    gpio_set_direction(sw_pdata->b_phase_io, 1);
    gpio_set_pull_up(sw_pdata->b_phase_io, 0);
    gpio_set_pull_down(sw_pdata->b_phase_io, 0);

    /* request_irq(IRQ_RDEC_IDX, 3, code_switch_handler, 0); */
    sys_s_hi_timer_add(NULL, code_switch_handler, 2); //10ms
}


static u8 code_switch_idle_query(void)
{
    /* return !(JL_RDEC->CON & 0x80); */
    return (!code_switch_active);
}

REGISTER_LP_TARGET(code_switch_lp_target) = {
    .name = "code_switch",
    .is_idle = code_switch_idle_query,
};

#endif   /* CODE_SWITCH_ENABLE */
