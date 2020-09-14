#include "app_config.h"
#include "asm/ctmu.h"
#include "asm/gpio.h"


#if TCFG_CTMU_TOUCH_KEY_ENABLE

#define TOUCH_CTMU_DEBUG 		0
#if TOUCH_CTMU_DEBUG
#define touch_ctmu_debug(fmt, ...) printf("[CTMU] "fmt, ##__VA_ARGS__)
#else
#define touch_ctmu_debug(fmt, ...)
#endif /* #if TOUCH_CTMU_DEBUG */

#define touch_ctmu_err(fmt, ...) printf("[CTMU ERR] "fmt, ##__VA_ARGS__)

#define CTMU_CON0 		JL_CTM->CON0
#define CTMU_CON1 		JL_CTM->CON1
#define CTMU_ADR 		JL_CTM->ADR

static u32 ctm_buf[16 * 2] = {0};

static sCTMU_KEY_VAR ctm_key_value;
static sCTMU_KEY_VAR *ctm_key_var;

static u8 port_index_mapping_talbe[CTMU_KEY_CH_MAX] = {0};

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
    GATE_SOURCE_PLL240M,
    GATE_SOURCE_PLL480M,
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
    CHARGE_SOURCE_PLL240M,
    CHARGE_SOURCE_PLL480M,
};


static const u8 ctmu_ch_table[] = {
    IO_PORTA_00, IO_PORTA_01, IO_PORTA_02, IO_PORTA_03,
    IO_PORTA_04, IO_PORTA_05, IO_PORTA_06, IO_PORTA_07,
    IO_PORTA_09, IO_PORTA_10, IO_PORTC_00, IO_PORTC_01,
    IO_PORTC_02, IO_PORTC_03, IO_PORTC_04, IO_PORTC_05
};


static u32 get_ctmu_ch_val(u8 ch_index, u32 *ctm_buf)
{
    if (ch_index >= user_data->num) {
        return 0;
    }

    return ctm_buf[port_index_mapping_talbe[ch_index]];
}

static void ctm_irq(u16 ctm_res, u8 ch)
{
    u16 temp_u16_0, temp_u16_1;
    s16 temp_s16_0, temp_s16_1;
    s32 temp_s32_0;
//..............................................................................................
//取计数值/通道判断
//..............................................................................................


    if (ctm_key_var->touch_init_cnt[ch]) {
        ctm_key_var->touch_init_cnt[ch]--;
        ctm_key_var->touch_cnt_buf[ch] = (u32)ctm_res << (ctm_key_var->FLT0CFG + 1);
        ctm_key_var->touch_release_buf[ch] = (u32)ctm_res << (ctm_key_var->FLT1CFG0 + 1);
    }

//..............................................................................................
//当前计数值去抖动滤波器
//..............................................................................................
    temp_u16_0 = ctm_key_var->touch_cnt_buf[ch];
    temp_u16_1 = temp_u16_0;
    temp_u16_1 -= (temp_u16_1 >> ctm_key_var->FLT0CFG);
    temp_u16_1 += ctm_res;
    ctm_key_var->touch_cnt_buf[ch] = temp_u16_1;
    temp_u16_0 += temp_u16_1;
    temp_u16_0 >>= (ctm_key_var->FLT0CFG + 1);


//..............................................................................................
//各通道按键释放计数值滤波器
//..............................................................................................
    temp_s32_0 = ctm_key_var->touch_release_buf[ch];
    temp_u16_1 = temp_s32_0 >> ctm_key_var->FLT1CFG0;	//获得滤波器之后的按键释放值
    temp_s16_0 = temp_u16_0 - temp_u16_1;	//获得和本次检测值的差值，按下按键为负值，释放按键为正值
    temp_s16_1 = temp_s16_0;

//	if(ch == 1)
//	{
//		printf("ch%d: %d  %d", (short)ch, temp_u16_0, temp_s16_1);
//	}

    if (ctm_key_var->touch_key_state & BIT(ch)) {	//如果本通道按键目前是处于释放状态
        if (temp_s16_1 <= 0) {	//当前计数值小于低通值，放大后参与运算
            if (temp_s16_1 > -(ctm_key_var->FLT1CFG2 >> 3)) {
                temp_s16_1 = temp_s16_1 * 8; //temp_s16_1 <<= 3;	//放大后参与运算
            } else {
                temp_s16_1 = -(ctm_key_var->FLT1CFG2);	//饱和，防止某些较大的正偏差导致错判
            }
        } else if (temp_s16_1 <= ctm_key_var->FLT1CFG1) {	//当前计数值小于低通值不多，正常参与运算
        } else {			//当前计数值小于低通值很多，缩小后参与运算
            temp_s16_1 = temp_s16_1 / 8;  //temp_s16_1 >>= 3;(有符号数右移自动扩展符号位???)
        }
    } else {		//如果本通道按键目前是处于按下状态, 缓慢降低释放计数值
        if (temp_s16_1 >= ctm_key_var->RELEASECFG1) {
            temp_s16_1 >>= 3;		//缩小后参与运算
        } else {
            temp_s16_1 = 0;
        }
    }

    temp_s32_0 += (s32)temp_s16_1;
    ctm_key_var->touch_release_buf[ch] = temp_s32_0;

//..............................................................................................
//按键按下与释放检测
//..............................................................................................
    if (temp_s16_0 >= ctm_key_var->PRESSCFG) {			//按键按下
        ctm_key_var->touch_key_state &= ~BIT(ch);
    } else if (temp_s16_0 >= ctm_key_var->RELEASECFG0) {	//按键释放
        ctm_key_var->touch_key_state |= BIT(ch);
    }
}


static void scan_capkey(u8 ch_index, u32 *ctmu_buf)
{
    u16 Touchkey_value_delta;

    if (user_data == NULL || user_data->num == 0) {
        return;
    }

    Touchkey_value_delta = get_ctmu_ch_val(ch_index, ctmu_buf);   ///获取计数值;
    /* g_printf("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa = %d",Touchkey_value_delta); */
    /*调用滤波算法*/
    ctm_irq(Touchkey_value_delta, ch_index);
}


___interrupt
static void ctmu_isr_handle(void)
{
    u32 *rbuf = NULL;
    u8 i, j;
    if (CTMU_CON0 & BIT(7)) {
        CTMU_CON0 |= BIT(6);
    }

    if (CTMU_CON0 & BIT(9)) {
        rbuf = (u32 *)(&ctm_buf[0]);
    } else {
        rbuf = (u32 *)(&ctm_buf[16]);
    }

    for (i = 0; i < user_data->num; i++) {
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
            }
        }
        if (j > sizeof(ctmu_ch_table)) {
            touch_ctmu_err("port err!!!");
            return;
        }

        gpio_set_pull_down(port_list[i].port, 0);
        gpio_set_pull_up(port_list[i].port, 0);
        gpio_set_die(port_list[i].port, 0);
        gpio_set_direction(port_list[i].port, 1);
    }
}

static void ctmu_buf_init(void)
{
    memset((u8 *)&ctm_buf, 0x00, sizeof(ctm_buf));
    CTMU_ADR = (u32)&ctm_buf;
}

static void log_ctmu_info(void)
{
    touch_ctmu_debug("CTMU_CON0 = 0x%x", CTMU_CON0);
    touch_ctmu_debug("CTMU_CON1 = 0x%x", CTMU_CON1);
}

static void touch_ctmu_init(const struct ctmu_key_port *port, u8 num)
{
    touch_ctmu_debug("%s", __func__);
    CTMU_CON0 = 0;
    CTMU_CON1 = 0;
    //充电时钟选择
    CTMU_CON0 |= (CHARGE_SOURCE_PLL480M << 10);
    //放电时钟分频选择, 时钟源固定是lsb
    CTMU_CON0 |= (DISCHARGE_SOURCE_PRE_DIV128 << 12); //要求 > 2uS

    //闸门时钟选择
    CTMU_CON0 |= (GATE_SOURCE_OSC << 4); //24 MHz
    CTMU_CON0 |= (GATE_SOURCE_PRE_DIV32768 << 1); //1.36ms

    //充电电流
    CTMU_CON0 |= 3 << 18;  //0 ~ 7
    //comparator reference voltage
    CTMU_CON0 |= 2 << 16;  //0 ~ 3
    //放电模式(在充电完成后是否放电, 这样会放电更彻底)
    CTMU_CON0 |= 1 << 23;

    CTMU_CON0 |= 1 << 6;  // Clear  Pending

    ctmu_port_init(port, num);
    ctmu_buf_init();

    log_ctmu_info();
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

    memset((u8 *)&ctm_key_value, 0x0, sizeof(sCTMU_KEY_VAR));

    /*触摸按键参数配置*/
    ctm_key_value.FLT0CFG = 2;
    ctm_key_value.FLT1CFG0 = 2;
    ctm_key_value.FLT1CFG1 = 80;
    ctm_key_value.FLT1CFG2 =  10 * 128; //128 = 2^7

    ///调节灵敏度的主要参数
    ctm_key_value.PRESSCFG =  user_data->press_cfg;;
    ctm_key_value.RELEASECFG0 = user_data->release_cfg0;
    ctm_key_value.RELEASECFG1 = user_data->release_cfg1;

    memset((u8 *) & (ctm_key_value.touch_init_cnt[0]), 0x10, CTMU_KEY_CH_MAX);

    ctm_key_value.touch_key_state = 0xffff; //<按键默认释放

    ctm_key_var = &ctm_key_value;

    if (user_data->num > CTMU_KEY_CH_MAX) {
        touch_ctmu_err("ctm key num config err!!!");
        return -1;
    }

    touch_ctmu_init(user_data->port_list, user_data->num);

    request_irq(IRQ_CTM_IDX, 1, ctmu_isr_handle, 0);

    touch_ctmu_enable(1);

    return 0;
}


u8 get_ctmu_value(void)
{
    u8 key = 0xFF;
    u8 i;

    for (i = 0; i < user_data->num; i++) {
        if (!(ctm_key_value.touch_key_state & (u16)(BIT(i)))) {
            break;
        }
    }
    key = (i < user_data->num) ? i : 0xFF;

    return key;
}

#endif /* #if TCFG_CTMU_TOUCH_KEY_ENABLE */

