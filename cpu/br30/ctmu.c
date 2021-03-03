#include "app_config.h"
#include "asm/ctmu.h"
#include "asm/gpio.h"


#if TCFG_CTMU_TOUCH_KEY_ENABLE

#define LOG_TAG_CONST       CTMU
#define LOG_TAG             "[ctmu]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


#define CTMU_CON0 		JL_CTM->CON0
#define CTMU_CON1 		JL_CTM->CON1
#define CTMU_ADR 		JL_CTM->ADR

#define CTMU_MAX_CH     16
static u32 ctm_buf[CTMU_MAX_CH * 2] = {0};
static u8 port_index_mapping_talbe[CTMU_KEY_CH_MAX] = {0};
static u32 Touchkey_pre_value[CTMU_KEY_CH_MAX] = {0};
static u32 Touchkey_normal_value[CTMU_KEY_CH_MAX] = {0};

#define CALIBRATE_CYCLE 50//周期大约两秒
static u8 Touchkey_start_cnt[CTMU_KEY_CH_MAX] = {0};
static u32 Touchkey_calibrate_cnt[CTMU_KEY_CH_MAX] = {0};
static u32 Touchkey_calibrate_tmp_value[CTMU_KEY_CH_MAX] = {0};

static u8 Touchkey_state = 0;

static const struct ctmu_touch_key_platform_data *user_data = NULL;


//=================================================================================//
/*
ctmu原理:

  1.三个时钟源:
     1)闸门时钟源(可选), 分频值可以选
     2)放电时钟源(固定是lsb), 分频值可以选
     3)充电时钟源(可选), 分频值不可选

  2.闸门时间内
            _________________________________
         __|                                 |__
      		    __    __    __    __    __
 放电时计数  __|  |__|  |__|  |__|  |__|  |__

  3. 过程: 放电 --> 充电(计数) --> 放电[该过程可选]

  4.分析计数值
 */
//=================================================================================//

//时钟闸门时钟源选择, 可分频
enum {
    GATE_SOURCE_OSC = 0,
    GATE_SOURCE_LRC,
    GATE_SOURCE_PLL128M,
    GATE_SOURCE_PLL192M,
};

enum {
    GATE_SOURCE_PRE_DIV8 = 0,
    GATE_SOURCE_PRE_DIV16,
    GATE_SOURCE_PRE_DIV32,
    GATE_SOURCE_PRE_DIV64,
    GATE_SOURCE_PRE_DIV8192,
    GATE_SOURCE_PRE_DIV16384,
    GATE_SOURCE_PRE_DIV32768,
    GATE_SOURCE_PRE_DIV65536,
};

//放电时钟源硬件上默认是lsb, 可分频
enum {
    DISCHARGE_SOURCE_PRE_DIV1 = 0,
    DISCHARGE_SOURCE_PRE_DIV2,
    DISCHARGE_SOURCE_PRE_DIV4,
    DISCHARGE_SOURCE_PRE_DIV8,
    DISCHARGE_SOURCE_PRE_DIV16,
    DISCHARGE_SOURCE_PRE_DIV32,
    DISCHARGE_SOURCE_PRE_DIV64,
    DISCHARGE_SOURCE_PRE_DIV128,
};

//充电时钟源选择, 不可分频
enum {
    CHARGE_SOURCE_OSC = 0,
    CHARGE_SOURCE_LRC,
    CHARGE_SOURCE_PLL128M,
    CHARGE_SOURCE_PLL192M,
};

static const u8 ctmu_ch_table[] = {
    IO_PORTA_03, IO_PORTA_04, IO_PORTA_05, IO_PORTA_06,
    IO_PORTA_07, IO_PORTA_08, IO_PORTB_05, IO_PORTB_06,
};


static u32 get_ctmu_ch_val(u8 ch_index, u32 *ctm_buf)
{
    if (ch_index >= user_data->num) {
        return 0;
    }
    return ctm_buf[port_index_mapping_talbe[ch_index]];
}

static u32 ctm_flt(u8 ch, u32 value)
{
    if (Touchkey_pre_value[ch] == 0) {
        Touchkey_pre_value[ch] = value;
    }
    if (value >= Touchkey_pre_value[ch]) {
        value = Touchkey_pre_value[ch] + ((value - Touchkey_pre_value[ch]) * 0.2f);
    } else {
        value = Touchkey_pre_value[ch] - ((Touchkey_pre_value[ch] - value) * 0.2f);
    }
    Touchkey_pre_value[ch] = value;
    return value;
}

static void scan_capkey(u8 ch_index, u32 *ctmu_buf)
{
    if (user_data == NULL || user_data->num == 0) {
        return;
    }
    u32 Touchkey_cur_value = get_ctmu_ch_val(ch_index, ctmu_buf);       //当前采到的值
    /* printf("ch: %d  val: %d\n", ch_index, Touchkey_cur_value); */
    u32 Touchkey_flt_value = ctm_flt(ch_index, Touchkey_cur_value);     //滤波后的值
    /* printf("ch: %d  val: %d\n", ch_index, Touchkey_flt_value); */

    //拿滤波后的值做处理
    if (Touchkey_start_cnt[ch_index] < 3) {
        Touchkey_start_cnt[ch_index] ++;
        Touchkey_normal_value[ch_index] = Touchkey_flt_value;
        return;
    }
    if (Touchkey_flt_value > (Touchkey_normal_value[ch_index] + user_data->port_list[ch_index].press_delta)) {
        Touchkey_state |=  BIT(ch_index);
        Touchkey_calibrate_cnt[ch_index] = 0;
    } else {
        Touchkey_state &= ~BIT(ch_index);
        Touchkey_calibrate_cnt[ch_index] ++;
    }
    //定期更新常态下的基准值
    if (Touchkey_calibrate_cnt[ch_index] == (CALIBRATE_CYCLE / 2)) {
        Touchkey_calibrate_tmp_value[ch_index] = Touchkey_flt_value;
    } else if (Touchkey_calibrate_cnt[ch_index] > CALIBRATE_CYCLE) {
        Touchkey_normal_value[ch_index] = Touchkey_calibrate_tmp_value[ch_index];
        Touchkey_calibrate_cnt[ch_index] = 0;
    }
}

___interrupt
static void ctmu_isr_handle(void)
{
    u32 *rbuf = NULL;
    if (CTMU_CON0 & BIT(7)) {
        CTMU_CON0 |= BIT(6);
    }
    if (CTMU_CON0 & BIT(9)) {
        rbuf = (u32 *)(&ctm_buf[0]);
    } else {
        rbuf = (u32 *)(&ctm_buf[CTMU_MAX_CH]);
    }
    for (u8 i = 0; i < user_data->num; i++) {
        scan_capkey(i, rbuf);
    }
}

static void ctmu_port_init(const struct ctmu_key_port *port_list, u8 port_num)
{
    u8 i, j;
    for (i = 0; i < port_num; i++) {
        for (j = 0; j < ARRAY_SIZE(ctmu_ch_table); j++) {
            if (ctmu_ch_table[j] == port_list[i].port) {

                CTMU_CON1 |= (BIT(j) << 0);
                port_index_mapping_talbe[i] = j;

                log_info("ctmu_ch_table[%d] %x", j, ctmu_ch_table[j]);
                gpio_set_pull_down(ctmu_ch_table[j], 0);
                gpio_set_pull_up(ctmu_ch_table[j], 0);
                gpio_set_die(ctmu_ch_table[j], 0);
                gpio_set_direction(ctmu_ch_table[j], 1);

                break;
            }
        }
        if (j > sizeof(ctmu_ch_table)) {
            log_e("port err!!!");
            return;
        }

    }
}

static void ctmu_buf_init(void)
{
    memset((u8 *)&ctm_buf, 0x00, sizeof(ctm_buf));
    CTMU_ADR = (u32)&ctm_buf;
}

static void log_ctmu_info(void)
{
    log_info("CTMU_CON0 = 0x%x", CTMU_CON0);
    log_info("CTMU_CON1 = 0x%x", CTMU_CON1);
}

static void touch_ctmu_init(const struct ctmu_key_port *port, u8 num)
{
    log_info("%s", __func__);

    CTMU_CON0 = 0;
    CTMU_CON1 = 0;

    ctmu_port_init(port, num);
    ctmu_buf_init();

    //充电时钟选择
    CTMU_CON0 |= (CHARGE_SOURCE_PLL192M << 10);
    //放电时钟分频选择, 时钟源固定是lsb
    CTMU_CON0 |= (DISCHARGE_SOURCE_PRE_DIV64 << 12); //要求 > 2uS

    //闸门时钟选择
    CTMU_CON0 |= (GATE_SOURCE_LRC << 4); //24 MHz
    CTMU_CON0 |= (GATE_SOURCE_PRE_DIV64 << 1); //1.36ms

    //充电电流
    CTMU_CON0 |= 3 << 18;  //0 ~ 7
    //comparator reference voltage
    CTMU_CON0 |= 2 << 16;  //0 ~ 3
    //放电模式(在充电完成后是否放电, 这样会放电更彻底)
    CTMU_CON0 |= 1 << 23;

    CTMU_CON0 |= 1 << 6;  // Clear  Pending
}

static void touch_ctmu_enable(u8 en)
{
    if (en) {
        CTMU_CON0 |= BIT(8); //Int Enable
        CTMU_CON0 |= BIT(0); //Moudle Enable
    } else {
        CTMU_CON0 &= ~BIT(8); //Int Disable
        CTMU_CON0 &= ~BIT(0); //Moudle Disable
    }
}

//API:
int ctmu_init(void *_data)
{
    if (_data == NULL) {
        return -1;
    }
    user_data = (const struct ctmu_touch_key_platform_data *)_data;

    if (user_data->num > CTMU_KEY_CH_MAX) {
        log_error("ctm key num config err!!!");
        return -1;
    }

    touch_ctmu_init(user_data->port_list, user_data->num);

    request_irq(IRQ_CTM_IDX, 1, ctmu_isr_handle, 0);

    touch_ctmu_enable(1);

    log_ctmu_info();

    return 0;
}

u8 get_ctmu_value(void)
{
    for (u8 i = 0; i < user_data->num; i++) {
        if (Touchkey_state & BIT(i)) {
            /* printf("i:%d\n", i); */
            return i;
        }
    }
    return 0xff;
}

#endif /* #if TCFG_CTMU_TOUCH_KEY_ENABLE */

