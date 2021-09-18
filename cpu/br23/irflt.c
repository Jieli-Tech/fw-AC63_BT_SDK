
#include "asm/includes.h"
#include "asm/irflt.h"
#include "timer.h"
#include "generic/gpio.h"

#define ir_log 		log_d

//红外定时器定义
#define IR_TIMER                    TIMER3
#define IR_IRQ_TIME_IDX             IRQ_TIME3_IDX
#define IR_TIME_REG                 JL_TIMER3

IR_CODE  ir_code;       ///<红外遥控信息
u16 timer1_pad;

static const u16 timer_div[] = {
    /*0000*/    1,
    /*0001*/    4,
    /*0010*/    16,
    /*0011*/    64,
    /*0100*/    2,
    /*0101*/    8,
    /*0110*/    32,
    /*0111*/    128,
    /*1000*/    256,
    /*1001*/    4 * 256,
    /*1010*/    16 * 256,
    /*1011*/    64 * 256,
    /*1100*/    2 * 256,
    /*1101*/    8 * 256,
    /*1110*/    32 * 256,
    /*1111*/    128 * 256,
};


#define IRFLT_OUTPUT_TIMER_SEL(x)		SFR(JL_IOMAP->CON0, 8, 3, x)


/*----------------------------------------------------------------------------*/
/**@brief   time1红外中断服务函数
   @param   void
   @param   void
   @return  void
   @note    void timer1_ir_isr(void)
*/
/*----------------------------------------------------------------------------*/
___interrupt
void timer_ir_isr(void)
{
    u16 bCap1;
    u8 cap = 0;

    static u8 cnt = 0;

    IR_TIME_REG->CON |= BIT(14);

    bCap1 = IR_TIME_REG->PRD;
    IR_TIME_REG->CNT = 0;
    cap = bCap1 / ir_code.timer_pad;

    /* ir_log("cnt = %d, cap = 0x%x", cnt++, cap); */
    if (cap <= 1) {
        ir_code.wData >>= 1;
        ir_code.bState++;
        ir_code.boverflow = 0;
    } else if (cap == 2) {
        ir_code.wData >>= 1;
        ir_code.bState++;
        ir_code.wData |= 0x8000;
        ir_code.boverflow = 0;
    }
    /*13ms-Sync*/
    /*
    else if ((cap == 13) || (cap < 8) || (cap > 110))
    {
        ir_code.bState = 0;
    }
    else
    {
        ir_code.boverflow = 0;
    }
    */
    else if ((cap == 13) && (ir_code.boverflow < 8)) {
        ir_code.bState = 0;
    } else if ((cap < 8) && (ir_code.boverflow < 5)) {
        ir_code.bState = 0;
    } else if ((cap > 110) && (ir_code.boverflow > 53)) {
        ir_code.bState = 0;
    } else if ((cap > 20) && (ir_code.boverflow > 53)) { //溢出情况下 （12M 48M）
        ir_code.bState = 0;
    } else {
        ir_code.boverflow = 0;
    }
    if (ir_code.bState == 16) {
        ir_code.wUserCode = ir_code.wData;
    }
    if (ir_code.bState == 32) {
        /* printf("[0x%X]",ir_code.wData); */
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   ir按键初始化
   @param   void
   @param   void
   @return  void
   @note    void set_ir_clk(void)

   ((cnt - 1)* 分频数)/lsb_clk = 1ms
*/
/*----------------------------------------------------------------------------*/
/* #define APP_TIMER_CLK           clk_get("timer")//选osc时钟源 */
#define APP_TIMER_CLK           12000000L //pll12m
#define TIMER_UNIT_MS           1
#define MAX_TIME_CNT 0x07ff //分频准确范围，更具实际情况调整
#define MIN_TIME_CNT 0x0030
void set_ir_clk(void)
{
    u32 clk;
    u32 prd_cnt;
    u8 index;


    for (index = 0; index < (sizeof(timer_div) / sizeof(timer_div[0])); index++) {
        prd_cnt = TIMER_UNIT_MS * (APP_TIMER_CLK / 1000) / timer_div[index];
        if (prd_cnt > MIN_TIME_CNT && prd_cnt < MAX_TIME_CNT) {
            break;
        }
    }

    ir_code.timer_pad = prd_cnt;
    /* IR_TIME_REG->CON = ((index << 4) | BIT(3) | BIT(1) | BIT(0));//选择osc时钟 */
    JL_IOMAP->CON1 |= BIT(27);//这里已选了timer3,时钟源选io信号里的pll_12m,不是所有的timer都可选pll,修改请看文档
    IR_TIME_REG->CON = ((index << 4) | BIT(2) | BIT(1) | BIT(0));

}


/*----------------------------------------------------------------------------*/
/**@brief   获取ir按键值
   @param   void
   @param   void
   @return  void
   @note    void get_irkey_value(void)
*/
/*----------------------------------------------------------------------------*/
u8 get_irflt_value(void)
{
    u8 tkey = 0xff;
    if (ir_code.bState != 32) {
        return tkey;
    }

    /* printf("(0x%X_%x)",ir_code.wUserCode,ir_code.wData); */
    if ((((u8 *)&ir_code.wData)[0] ^ ((u8 *)&ir_code.wData)[1]) == 0xff) {
        /* if (ir_code.wUserCode == 0xFF00) */
        {
            /* printf("<%d>",(u8)ir_code.wData); */
            tkey = (u8)ir_code.wData;
        }
    } else {
        ir_code.bState = 0;
    }

    return tkey;
}

void ir_input_io_sel(u8 port)
{
    //选择input channel1输入
    // IOMC2[13 : 8]
    // 0 ~ 15: 		PA0 ~ PA15
    //16 ~ 31: 		PB0 ~ PB15
    //32 ~ 47: 		PC0 ~ PC15
    //48 ~ 55: 		PD0 ~ PD7
    gpio_irflt_in(port);
    gpio_set_direction(port, 1);
    gpio_set_die(port, 1);
}

void ir_output_timer_sel()
{
    IRFLT_OUTPUT_TIMER_SEL(IR_TIMER);
    request_irq(IR_IRQ_TIME_IDX, 5, timer_ir_isr, 0);
}

static void ir_timeout(void *priv)
{
    ir_code.boverflow++;
    if (ir_code.boverflow > 56) { //56*2ms ~= 112ms
        ir_code.bState = 0;
    }
}

void ir_timeout_set(void)
{
    sys_s_hi_timer_add(NULL, ir_timeout, 2); //2ms
}

void irflt_config()
{
    JL_IR->RFLT_CON = 0;
    /* JL_IR->RFLT_CON |= BIT(7);		//256 div */
    /* JL_IR->RFLT_CON |= BIT(3);		//osc 24m */
    JL_IR->RFLT_CON |= BIT(7) | BIT(4);		//512 div
    JL_IR->RFLT_CON |= BIT(3) | BIT(2);		//PLL_48m（兼容省晶振）
    JL_IR->RFLT_CON |= BIT(0);		//irflt enable

    set_ir_clk();
}

void log_irflt_info()
{
    ir_log("IOMC0 = 0x%x", JL_IOMAP->CON0);
    ir_log("IOMC2 = 0x%x", JL_IOMAP->CON2);
    ir_log("RFLT_CON = 0x%x", JL_IR->RFLT_CON);
    ir_log("IR_TIME_REG = 0x%x", IR_TIME_REG->CON);
}

