#include "includes.h"
#include "asm/plcnt.h"

#define PLCNT_CTM_DEBUG 		0

#if PLCNT_CTM_DEBUG
#define plcnt_ctm_debug(fmt, ...) printf("[PLCNT] "fmt, ##__VA_ARGS__)
#else
#define plcnt_ctm_debug(fmt, ...)
#endif

u8 g_touch_len;        //触摸按键数
u8  bCapState;          //电容充放电状态
u8  bCap_ch = 0;          //触摸通道
u16 Touchkey_value_new = 0;
u16 Touchkey_value_old = 0;
sCTM_KEY_VAR ctm_key_value;


#define PLCNT_CLOCK_SEL(x)				SFR(JL_PCNT->CON, 2, 2, x)
#define INPUT_CHANNLE2_SRC_SEL(x)		SFR(JL_IOMAP->CON2, 16, 6, x)

static const struct touch_key_platform_data *user_data = NULL;


sCTM_KEY_VAR *ctm_key_var;


static u8 plcnt_channel2port(u8 chan)
{
    return user_data->port_list[chan].port;
}


static void set_port_out_H(u8 chan)
{
    gpio_direction_output(plcnt_channel2port(chan), 1);
}

static void set_port_pd(u8 chan)
{
    gpio_set_pull_down(plcnt_channel2port(chan), 1);
}

static void set_port_die(u8 chan)
{
    gpio_set_die(plcnt_channel2port(chan), 1);
}


static void set_port_in(u8 chan)
{
    gpio_direction_input(plcnt_channel2port(chan)); //输入
}

static void set_touch_io(u8 chan)
{
    INPUT_CHANNLE2_SRC_SEL(plcnt_channel2port(chan));
    set_port_in(chan);  //设置为输入
}


static void ctm_key_var_init(sCTM_KEY_VAR *ptr)
{
    ctm_key_var = ptr;
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
//		touch_cnt_buf[ch] = rvalue << FLT0CFG;
//		touch_release_buf[ch] = (long)(rvalue) << FLT1CFG0;
        ctm_key_var->touch_cnt_buf[ch] = (u32)ctm_res << ctm_key_var->FLT0CFG;
        ctm_key_var->touch_release_buf[ch] = (u32)ctm_res << ctm_key_var->FLT1CFG0;
    }

//..............................................................................................
//当前计数值去抖动滤波器
//..............................................................................................
    temp_u16_0 = ctm_key_var->touch_cnt_buf[ch];
    temp_u16_1 = temp_u16_0;
    temp_u16_1 -= (temp_u16_1 >> ctm_key_var->FLT0CFG);
    temp_u16_1 += ctm_res;//temp_u16_1 += rvalue;
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
        if (temp_s16_1 >= 0) {	//当前计数值大于低通值，放大后参与运算
            if (temp_s16_1 < (ctm_key_var->FLT1CFG2 >> 3)) {
                temp_s16_1 <<= 3;	//放大后参与运算
            } else {
                temp_s16_1 = ctm_key_var->FLT1CFG2;	//饱和，防止某些较大的正偏差导致错判
            }
        } else if (temp_s16_1 >= ctm_key_var->FLT1CFG1) {	//当前计数值小于低通值不多，正常参与运算
        } else {			//当前计数值小于低通值很多，缩小后参与运算 (有符号数右移自动扩展符号位???)
            temp_s16_1 >>= 3;
        }
    } else {		//如果本通道按键目前是处于按下状态, 缓慢降低释放计数值
        if (temp_s16_1 <= ctm_key_var->RELEASECFG1) {
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
    if (temp_s16_0 <= ctm_key_var->PRESSCFG) {			//按键按下
        ctm_key_var->touch_key_state &= ~BIT(ch);
    } else if (temp_s16_0 >= ctm_key_var->RELEASECFG0) {	//按键释放
        ctm_key_var->touch_key_state |= BIT(ch);
    }
}


static void scan_capkey(void *priv)
{
    u16 temp;
    u16 Touchkey_value_delta;

    if (0 == g_touch_len) {
        return;
    }

    if (bCapState == 0) {
        bCapState = 1;
        Touchkey_value_new = JL_PCNT->VAL;   ///获取计数值

        set_port_out_H(bCap_ch);//set output H
        ///////***wait IO steady for pulse counter

        if (Touchkey_value_old > Touchkey_value_new) {
            Touchkey_value_new += 0x10000;
        }

        Touchkey_value_delta = Touchkey_value_new - Touchkey_value_old;
        //plcnt_ctm_debug("old: %x   new: %x", Touchkey_value_old, Touchkey_value_new);
        Touchkey_value_old = Touchkey_value_new;	///记录旧值
        temp = 6800L - Touchkey_value_delta * user_data->change_gain;    ///1变化增大倍数

        /* if(bCap_ch == 0){ */
        /* plcnt_ctm_debug("value: %x\n", Touchkey_value_delta); */
        /* } */

        //plcnt_ctm_debug("delta: %d, ch: %d", Touchkey_value_delta, bCap_ch);
        /* if(bCap_ch == 0) */
        /* { */
        /* plcnt_ctm_debug("temp: %d\n", Touchkey_value_delta); */
        /* } */

        /*调用滤波算法*/
        ctm_irq(temp, bCap_ch);

        /*清除前一个通道状态，锁定PLL CNT*/
        //set_port_out(bCap_ch); //设为输出
        /*切换通道，开始充电，PLL CNT 输入Mux 切换*/
        bCap_ch++;
        bCap_ch %= g_touch_len;
        //bCap_ch = (bCap_ch >= g_touch_len) ? 0 : bCap_ch;


        ////////***make sure IO is steady (IO output high voltage) for pulse counter

        set_port_out_H(bCap_ch);
        //set_port_out(bCap_ch);

    } else {
        bCapState = 0;
        set_touch_io(bCap_ch); //设为输入开始计数
    }
}

//API:
u8 get_plcnt_value(void)
{
    u8 key;
    u8 i;

    for (i = 0; i < g_touch_len; i++) {
        if (!(ctm_key_value.touch_key_state & (u8)(BIT(i)))) {
            break;
        }
    }
    key = (i < g_touch_len) ? i : NO_KEY;

    if (key != NO_KEY) {
        //plcnt_ctm_debug("tch %x", key);
    }

    return key;
}


//API:
int plcnt_init(void *_data)
{
    if (_data == NULL) {
        return -1;
    }
    user_data = (const struct touch_key_platform_data *)_data;


    ctm_key_var_init(&ctm_key_value);

    memset((u8 *)&ctm_key_value, 0x0, sizeof(sCTM_KEY_VAR));

    /*触摸按键参数配置*/
    ctm_key_value.FLT0CFG = 0;
    ctm_key_value.FLT1CFG0 = 7;
    ctm_key_value.FLT1CFG1 = -80;
    ctm_key_value.FLT1CFG2 = (-(-10)) << 7; //1280

    ///调节灵敏度的主要参数
    /* ctm_key_value.PRESSCFG = -10; */
    /* ctm_key_value.RELEASECFG0 = -50; */
    /* ctm_key_value.RELEASECFG1 = -80;//-81; */

    ctm_key_value.PRESSCFG = user_data->press_cfg;
    ctm_key_value.RELEASECFG0 = user_data->release_cfg0;
    ctm_key_value.RELEASECFG1 = user_data->release_cfg1;

    memset((u8 *) & (ctm_key_value.touch_init_cnt[0]), 0x10, TOUCH_KEY_CH_MAX);

    ctm_key_value.touch_key_state = 0xffff; //<按键默认释放

    //初始化计数器配置:
    PLCNT_CLOCK_SEL(user_data->clock);

    JL_PCNT->CON |= BIT(1);	//使能计数器

    if (user_data->num > TOUCH_KEY_CH_MAX) {
        printf("plcnt_ctm key num config err!!!");
        return -1;
    }

    g_touch_len = user_data->num;
    Touchkey_value_old = JL_PCNT->VAL;   ///获取计数值

    set_port_out_H(bCap_ch);

#if 1       //如果外部有下拉电阻，可不是用芯片内部下拉
    for (u8 i = 0; i < g_touch_len; i++) {
        set_port_pd(i);
        set_port_die(i);
    }
#endif

    plcnt_ctm_debug("touch_key_init , plcnt_con = 0x%x, val = 0x%x", JL_PCNT->CON, JL_PCNT->VAL);

    sys_s_hi_timer_add(NULL, scan_capkey, 2); //2ms

    return 0;
}



