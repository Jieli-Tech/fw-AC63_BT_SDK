#include "asm/includes.h"
#include "system/includes.h"
#include "app_config.h"
#include "chargebox.h"

#if (TCFG_CHARGE_ENABLE && TCFG_HANDSHAKE_ENABLE)

static int hs_timer;
static u8 hs_repeat_times;
void (*handshake_complete_cb)(void);
extern void handshake_timer_delay_us(u8 us);
extern void handshake_timer_delay_ms(u8 ms);

static struct _hs_hdl hs_ctrl = {
    .port0 = TCFG_HANDSHAKE_IO_DATA1,         //初始化IO
    .port1 = TCFG_HANDSHAKE_IO_DATA2,         //初始化IO
    .send_delay_us = handshake_timer_delay_us,//注册延时函数
};

static void handshake_app_init(void)
{
    static u8 hs_init_flag = 0;
    if (hs_init_flag == 0) {
        hs_init_flag = 1;
        handshake_ctrl_init(&hs_ctrl);
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    lighting检测是否为快充线
   @param    无
   @return   无
   @note     无
*/
/*------------------------------------------------------------------------------------*/
static void handshake_app_check_fast_charge(void)
{
    u8 ret;
    local_irq_disable();
    ret = handshake_check_fast_charge(10);//检测10ms
    local_irq_enable();
}

/*------------------------------------------------------------------------------------*/
/**@brief    lighting握手
   @param    无
   @return   无
   @note     注意：为了精确的时间，会关闭其他中断
*/
/*------------------------------------------------------------------------------------*/
static void handshake_app_run(void)
{
    u32 cur_clock = 0;
    cur_clock = clk_get("sys");
    //转成48m
    if (cur_clock != 48000000) {
        clk_set("sys", 48 * 1000000L);
    }
    local_irq_disable();
    handshake_timer_delay_ms(2);
    /* handshake_send_app(HS_CMD0); */
    /* handshake_timer_delay_ms(2); */
    /* handshake_send_app(HS_CMD1); */
    /* handshake_timer_delay_ms(2); */
    /* handshake_send_app(HS_CMD2); */
    /* handshake_timer_delay_ms(2); */
    handshake_send_app(HS_CMD3);
    local_irq_enable();
    //恢复时钟
    if (cur_clock != 48000000) {
        clk_set("sys", cur_clock);
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    lighting重复握手
   @param    null
   @return   无
   @note     用定时器实现重复握手增加握手成功几率
*/
/*------------------------------------------------------------------------------------*/
static void handshake_app_deal(void *priv)
{
    handshake_app_run();
    hs_repeat_times--;
    if (hs_repeat_times == 0) {
        sys_timer_del(hs_timer);
        hs_timer = 0;
        if (handshake_complete_cb) {
            handshake_complete_cb();
            handshake_complete_cb = NULL;
        }
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    lighting握手开始
   @param    times:重复次数
   @return   无
   @note     初始化多次握手的次数
*/
/*------------------------------------------------------------------------------------*/
void handshake_app_start(u8 times, void (*complete_cb)(void))
{
    handshake_app_init();
    if (hs_repeat_times == 0) {
        handshake_app_check_fast_charge();
        handshake_app_run();
        hs_repeat_times = times;
        handshake_complete_cb = complete_cb;
        if (hs_repeat_times) {
            if (hs_timer == 0) {
                hs_timer = sys_timer_add(NULL, handshake_app_deal, 200);
            }
        } else {
            if (complete_cb) {
                complete_cb();
            }
        }
    }
}

/*------------------------------------------------------------------------------------*/
/**@brief    lighting握手停止
   @param    无
   @return   无
   @note     中途停止握手时调用
*/
/*------------------------------------------------------------------------------------*/
void handshake_app_stop(void)
{
    if (hs_timer) {
        sys_timer_del(hs_timer);
        hs_timer = 0;
    }
    handshake_complete_cb = NULL;
    hs_repeat_times = 0;
}

#endif

