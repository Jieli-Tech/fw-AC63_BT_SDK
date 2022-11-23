#include "system/includes.h"
#include "app_config.h"
#include "app_action.h"
#include "app_main.h"
#include "gpio.h"
#include "asm/power/p11.h"

//  可配参数
#define AUDIO_PWM_SOURCE    (1)         // 0:timer    1:mcpwm
#define AUDIO_PWM_MODE      (3)         // 0:single mode   1:diff mode 0   2:diff mode 1   3:diff mode 2
#define FADE_STEP           (100)       // PWM开始和结尾的淡入淡出步进，每 FADE_STEP 个点更新一次占空比单位
#if (AUDIO_PWM_SOURCE == 0)
#define PWM_DEAD_AREA       (50)        // PWM死区时钟数 因为占空比在中断更新，这个时间要大于中断的整个处理时间，但要小于PWM周期
#elif (AUDIO_PWM_SOURCE == 1)
#define PWM_DEAD_AREA       (0)         // PWM死区时钟数 MCPWM 模块有占空比更新时机控制机制，不需要死区
#endif
#define BUF_SIZE            (1024 * 4)  // AUDIO PWM 缓冲buf大小

u32 pwm_io_P = IO_PORTB_08;             // PWM 输出 P 端 IO 选择，单端只关注 P 端即可
u32 pwm_io_N = IO_PORTB_09;             // PWM 输出 N 端 IO 选择，差分需要配置 N 端

// PWM cbuf 缓冲区相关参数
volatile s16 audio_pwm_cbuf[BUF_SIZE] = {0};
volatile u32 rptr = 0;
volatile u32 wptr = 0;

static u8 pwm_is_closed = 1;

// PWM 状态及占空比控制参数
#define STATE_STOP          (0)
#define STATE_FADE_IN       (1)
#define STATE_START         (2)
#define STATE_FADE_OUT      (3)
volatile u8 audio_pwm_state = 0;
volatile u32 audio_pwm_target = 0;
volatile u32 audio_pwm_cur = 0;
volatile u32 audio_pwm_prd = 0;     // 周期时钟数，也是占空比的分辨率
u16 pcm_to_pwm_scale = 0;           // 16bit pcm数据 转成 占空比分辨率范围 的比例值
u32 audio_pwm_zero = 0;             // PCM 数据 0 的时候对应的 PWM 值
u32 half_wave_limit = 0;
volatile u8 pwm_need_resume = 0;
static OS_SEM pwm_need_resume_sem ;
static void (*pwm_resume_handler)(void *) = NULL;
static u8  drop_flag = 0;



void state_printf(void)
{
    printf("state :%d, pwm_h:%d, pwm_l:%d", audio_pwm_state, JL_MCPWM->CH3_CMPH, JL_MCPWM->CH3_CMPL);
}

void set_state(u8 state)
{
    if (state == STATE_STOP) { //要设置pwm 停止需要等cbuffer 里面的数据播完
        u8 err_cnt = 0;
        while (abs(wptr - rptr) > 1) {
            delay(3000);
            err_cnt++;
            if (err_cnt > 500) {
                printf("wptr:%x,rptr:%x!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!", wptr, rptr);
                break;
            }
        }
        audio_pwm_state = state;
        pwm_is_closed = 1;
    } else {
        audio_pwm_state = state;
        pwm_is_closed = 0;
    }
}

#if (AUDIO_PWM_SOURCE == 0) // timer

___interrupt
void usr_pwm_timer_isr(void) __attribute__((section(".audio_pwm_code")))
{
    s32 pcm = 0;
    s32 pcm1 = 0;
    JL_TIMER3->CON |= BIT(14);
    /* JL_PORTA->DIR &= ~BIT(5); */
    /* JL_PORTA->OUT &= ~BIT(5); */

    if (drop_flag == 0) {
        drop_flag = 1;
        return;
    } else {
        drop_flag = 0;
    }

    if (audio_pwm_state == STATE_START) {
        if (rptr != wptr) {
            pcm = (s32)(audio_pwm_cbuf[rptr]);
#if (AUDIO_PWM_MODE == 0)
            if (pcm >= 0) {
                pcm += pcm_to_pwm_scale / 2;
            } else {
                pcm -= pcm_to_pwm_scale / 2;
            }
            pcm /= pcm_to_pwm_scale;
            if (pcm > half_wave_limit) {
                pcm = half_wave_limit;
            } else if (pcm < -half_wave_limit) {
                pcm = -half_wave_limit;
            }
            JL_TIMER3->PWM = (u32)(pcm + audio_pwm_zero);  // P
#elif (AUDIO_PWM_MODE == 1)
            if (pcm >= 0) {
                pcm += pcm_to_pwm_scale / 2;
            } else {
                pcm -= pcm_to_pwm_scale / 2;
            }
            pcm /= pcm_to_pwm_scale;
            if (pcm > half_wave_limit) {
                pcm = half_wave_limit;
            } else if (pcm < -half_wave_limit) {
                pcm = -half_wave_limit;
            }
            JL_TIMER3->PWM = (u32)(pcm + audio_pwm_zero);  // P
            JL_TIMER2->PWM = (u32)(pcm + audio_pwm_zero);  // N
#elif (AUDIO_PWM_MODE == 2)
            if (pcm >= 0) {
                pcm += pcm_to_pwm_scale / 2;
            } else {
                pcm -= pcm_to_pwm_scale / 2;
            }
            pcm /= pcm_to_pwm_scale;
            if (pcm > half_wave_limit) {
                pcm = half_wave_limit;
            } else if (pcm < -half_wave_limit) {
                pcm = -half_wave_limit;
            }
            JL_TIMER3->PWM = (u32)(pcm + audio_pwm_zero);  // P
            JL_TIMER2->PWM = (u32)(-pcm + audio_pwm_zero);  // N
#elif (AUDIO_PWM_MODE == 3)
            if (pcm >= 0) {
                pcm += pcm_to_pwm_scale / 2;
                pcm /= pcm_to_pwm_scale;
                if (pcm > half_wave_limit) {
                    pcm = half_wave_limit;
                }
                JL_TIMER3->PWM = (u32)(PWM_DEAD_AREA + pcm);    // P
                JL_TIMER2->PWM = (u32)(PWM_DEAD_AREA);          // N
            } else {
                pcm = -pcm;
                pcm -= pcm_to_pwm_scale / 2;
                pcm /= pcm_to_pwm_scale;
                if (pcm > half_wave_limit) {
                    pcm = half_wave_limit;
                }
                JL_TIMER3->PWM = (u32)(PWM_DEAD_AREA);          // P
                JL_TIMER2->PWM = (u32)(PWM_DEAD_AREA + pcm);    // N
            }
#endif
            rptr++;
            if (rptr >= BUF_SIZE) {
                rptr = 0;
            }
        } else {
            /* putchar('e'); */
        }
    } else if ((audio_pwm_state == STATE_FADE_IN) || (audio_pwm_state == STATE_FADE_OUT)) {
        if (audio_pwm_cur > audio_pwm_target) {
            audio_pwm_cur--;
            /* printf("{%d", audio_pwm_cur); */
        } else if (audio_pwm_cur < audio_pwm_target) {
            audio_pwm_cur++;
            /* printf("}%d", audio_pwm_cur); */
        } else {
            if (audio_pwm_state == STATE_FADE_IN) {
                audio_pwm_state = STATE_START;
            } else {
                audio_pwm_state = STATE_STOP;
            }
        }
#if (AUDIO_PWM_MODE == 0)
        JL_TIMER3->PWM = audio_pwm_cur / FADE_STEP;
#elif ((AUDIO_PWM_MODE == 1) || (AUDIO_PWM_MODE == 2))
        JL_TIMER3->PWM = audio_pwm_cur / FADE_STEP;
        JL_TIMER2->PWM = audio_pwm_cur / FADE_STEP;
#elif (AUDIO_PWM_MODE == 3) // 这种调制方式不需要淡入淡出
        if (audio_pwm_state == STATE_FADE_IN) {
            audio_pwm_state = STATE_START;
        } else {
            audio_pwm_state = STATE_STOP;
        }
#endif
    } else {
        // do no thing
    }

    /* JL_PORTA->DIR &= ~BIT(5); */
    /* JL_PORTA->OUT |=  BIT(5); */
}

void usr_pwm_timer_init(void)
{
    printf("############ usr_pwm_timer_init ########## \n");
    bit_clr_ie(IRQ_TIME3_IDX);
    request_irq(IRQ_TIME3_IDX, 3, usr_pwm_timer_isr, 0);
    irq_unmask_set(IRQ_TIME3_IDX, 0);
    JL_TIMER3->CON = BIT(14);
    JL_TIMER3->CON |= (6 << 10);          //时钟源选择稳定的STD_24M
    JL_TIMER3->CON |= (0b0000 << 4);      //时钟源再1分频
    JL_TIMER3->CON &= ~BIT(9);			  // PWM_INV 信号反相
    JL_TIMER3->CON |= BIT(8);			  // PWM IO OUT
    JL_TIMER3->CNT = 0;
    JL_TIMER3->PRD = 750;				  // 频率 32kHz
    JL_TIMER3->PWM = 0;
    JL_TIMER3->CON |= BIT(14) | BIT(0);

    audio_pwm_prd = JL_TIMER3->PRD;
#if (AUDIO_PWM_MODE == 3)
    pcm_to_pwm_scale = 32767 / audio_pwm_prd;
    audio_pwm_zero = 0;
    half_wave_limit = audio_pwm_prd;
#else
    pcm_to_pwm_scale = 65536 / (audio_pwm_prd - PWM_DEAD_AREA);
    audio_pwm_zero =  PWM_DEAD_AREA + (audio_pwm_prd - PWM_DEAD_AREA) / 2;
    half_wave_limit = (audio_pwm_prd - PWM_DEAD_AREA) / 2;
#endif

    gpio_set_fun_output_port(pwm_io_P, FO_TMR3_PWM, 0, 1);
    //设置引脚状态
    gpio_set_die(pwm_io_P, 1);
    gpio_set_pull_up(pwm_io_P, 0);
    gpio_set_pull_down(pwm_io_P, 0);
    gpio_set_direction(pwm_io_P, 0);

#if ((AUDIO_PWM_MODE == 1) || (AUDIO_PWM_MODE == 2) || (AUDIO_PWM_MODE == 3))
    JL_TIMER2->CON = 0;
    JL_TIMER2->CON |= (6 << 10);          //时钟源选择稳定的STD_24M
    JL_TIMER2->CON |= (0b0000 << 4);      //时钟源再1分频

#if (AUDIO_PWM_MODE == 1)
    JL_TIMER2->CON |=  BIT(9);			  // PWM_INV 信号反相
#elif ((AUDIO_PWM_MODE == 2) || (AUDIO_PWM_MODE == 3))
    JL_TIMER2->CON &= ~BIT(9);			  // PWM_INV 信号反相
#endif

    JL_TIMER2->CON |= BIT(8);			  // PWM IO OUT
    JL_TIMER2->PRD = 750;				  // 频率 32kHz
    JL_TIMER2->PWM = 0;
    JL_TIMER2->CON |= BIT(0);
    JL_TIMER2->CNT = 0;
    JL_TIMER3->CNT = 0; // sync

    gpio_set_fun_output_port(pwm_io_N, FO_TMR2_PWM, 0, 1);
    //设置引脚状态
    gpio_set_die(pwm_io_N, 1);
    gpio_set_pull_up(pwm_io_N, 0);
    gpio_set_pull_down(pwm_io_N, 0);
    gpio_set_direction(pwm_io_N, 0);
    /* gpio_set_hd(pwm_io_N, 1); */
    /* gpio_set_hd0(pwm_io_N, 1); */
#endif

    audio_pwm_state = STATE_START;
}

void usr_pwm_timer_uninit(void)
{
    JL_TIMER3->CON = 0;
#if ((AUDIO_PWM_MODE == 1) || (AUDIO_PWM_MODE == 2) || (AUDIO_PWM_MODE == 3))
    JL_TIMER2->CON = 0;
#endif
}

#elif (AUDIO_PWM_SOURCE == 1)   // mcpwm

___interrupt
void usr_mcpwm_isr(void) __attribute__((section(".audio_pwm_code")))
{
    s32 pcm = 0;
    s32 pcm1 = 0;
    if (JL_MCPWM->TMR3_CON & BIT(12)) {
        JL_MCPWM->TMR3_CON |= BIT(10);//清中断标志
    }

    if (drop_flag == 0) {
        drop_flag = 1;
        return;
    } else {
        drop_flag = 0;
    }

    /* JL_PORTA->DIR &= ~BIT(5); */
    /* JL_PORTA->OUT &= ~BIT(5); */

    if (audio_pwm_state == STATE_START) {
        if (rptr != wptr) {
            pcm = (s32)(audio_pwm_cbuf[rptr]);
#if (AUDIO_PWM_MODE == 0)
            if (pcm >= 0) {
                pcm += pcm_to_pwm_scale / 2;
            } else {
                pcm -= pcm_to_pwm_scale / 2;
            }
            pcm /= pcm_to_pwm_scale;
            if (pcm > half_wave_limit) {
                pcm = half_wave_limit;
            } else if (pcm < -half_wave_limit) {
                pcm = -half_wave_limit;
            }
            JL_MCPWM->CH3_CMPH = (u32)(pcm + audio_pwm_zero);  // P
#elif (AUDIO_PWM_MODE == 1)
            if (pcm >= 0) {
                pcm += pcm_to_pwm_scale / 2;
            } else {
                pcm -= pcm_to_pwm_scale / 2;
            }
            pcm /= pcm_to_pwm_scale;
            if (pcm > half_wave_limit) {
                pcm = half_wave_limit;
            } else if (pcm < -half_wave_limit) {
                pcm = -half_wave_limit;
            }
            JL_MCPWM->CH3_CMPH = (u32)(pcm + audio_pwm_zero);  // P
            JL_MCPWM->CH3_CMPL = (u32)(pcm + audio_pwm_zero);  // N
#elif (AUDIO_PWM_MODE == 2)
            if (pcm >= 0) {
                pcm += pcm_to_pwm_scale / 2;
            } else {
                pcm -= pcm_to_pwm_scale / 2;
            }
            pcm /= pcm_to_pwm_scale;
            if (pcm > half_wave_limit) {
                pcm = half_wave_limit;
            } else if (pcm < -half_wave_limit) {
                pcm = -half_wave_limit;
            }
            JL_MCPWM->CH3_CMPH = (u32)(pcm + audio_pwm_zero);  // P
            pcm1 = pcm;
            JL_MCPWM->CH3_CMPL = (u32)(-pcm + audio_pwm_zero);  // N
#elif (AUDIO_PWM_MODE == 3)
            if (pcm >= 0) {
                pcm += pcm_to_pwm_scale / 2;
                pcm /= pcm_to_pwm_scale;
                if (pcm > half_wave_limit) {
                    pcm = half_wave_limit;
                }
                JL_MCPWM->CH3_CMPH = (u32)(PWM_DEAD_AREA + pcm);    // P
                JL_MCPWM->CH3_CMPL = PWM_DEAD_AREA;                 // N
            } else {
                pcm = -pcm;
                pcm -= pcm_to_pwm_scale / 2;
                pcm /= pcm_to_pwm_scale;
                if (pcm > half_wave_limit) {
                    pcm = half_wave_limit;
                }
                JL_MCPWM->CH3_CMPH = PWM_DEAD_AREA;                 // P
                JL_MCPWM->CH3_CMPL = (u32)(PWM_DEAD_AREA + pcm);    // N
            }
#endif
            rptr++;
            if (rptr >= BUF_SIZE) {
                rptr = 0;
            }
        } else {
            /* putchar('e'); */
        }

    } else if ((audio_pwm_state == STATE_FADE_IN) || (audio_pwm_state == STATE_FADE_OUT)) {
        if (audio_pwm_cur > audio_pwm_target) {
            audio_pwm_cur--;
            /* printf("{%d", audio_pwm_cur); */
        } else if (audio_pwm_cur < audio_pwm_target) {
            audio_pwm_cur++;
            /* printf("}%d", audio_pwm_cur); */
        } else {
            if (audio_pwm_state == STATE_FADE_IN) {
                audio_pwm_state = STATE_START;
                /* putchar('s'); */
            } else {
                audio_pwm_state = STATE_STOP;
                /* putchar('p'); */
            }
        }
        JL_MCPWM->CH3_CMPH = audio_pwm_cur / FADE_STEP;
#if ((AUDIO_PWM_MODE == 1) || (AUDIO_PWM_MODE == 2))
        JL_MCPWM->CH3_CMPL = audio_pwm_cur / FADE_STEP;
#elif (AUDIO_PWM_MODE == 3) // 这种调制方式不需要淡入淡出
        if (audio_pwm_state == STATE_FADE_IN) {
            audio_pwm_state = STATE_START;
        } else if (audio_pwm_state == STATE_FADE_OUT) {
            audio_pwm_state = STATE_STOP;
        }
#endif
    } else {
        JL_MCPWM->CH3_CMPH = PWM_DEAD_AREA;                 // P
        JL_MCPWM->CH3_CMPL = PWM_DEAD_AREA;                 // N
        // do no thing
    }
    /* JL_PORTA->DIR &= ~BIT(5); */
    /* JL_PORTA->OUT |=  BIT(5); */
}

void usr_mcpwm_init(void)
{
    printf("############ usr_mcpwm_init ########## \n");
    request_irq(IRQ_MCPWM_TIMER, 3, usr_mcpwm_isr, 0);
    irq_unmask_set(IRQ_MCPWM_TIMER, 0);
    JL_MCPWM->MCPWM_CON0 &= ~BIT(8 + 3);
    JL_MCPWM->TMR3_CNT = 0;
    JL_MCPWM->TMR3_PR = clk_get("lsb") / 32000;
    audio_pwm_prd = JL_MCPWM->TMR3_PR;

#if (AUDIO_PWM_MODE == 3)
    pcm_to_pwm_scale = 32767 / audio_pwm_prd;
    audio_pwm_zero = 0;
    half_wave_limit = audio_pwm_prd;
#else
    pcm_to_pwm_scale = 65536 / (audio_pwm_prd - PWM_DEAD_AREA);
    audio_pwm_zero =  PWM_DEAD_AREA + (audio_pwm_prd - PWM_DEAD_AREA) / 2;
    half_wave_limit = (audio_pwm_prd - PWM_DEAD_AREA) / 2;
#endif

    JL_MCPWM->TMR3_CON |= BIT(8);    // IE
    JL_MCPWM->TMR3_CON |= BIT(10) | BIT(0);
    JL_MCPWM->MCPWM_CON0 |= BIT(8 + 3); // MCTIMER3 enable

    gpio_set_fun_output_port(pwm_io_P, FO_MCPWM_CH3H, 0, 1);
    gpio_set_die(pwm_io_P, 1);
    gpio_set_direction(pwm_io_P, 0);
    gpio_set_pull_up(pwm_io_P, 0);
    gpio_set_pull_down(pwm_io_P, 0);

#if ((AUDIO_PWM_MODE == 1) || (AUDIO_PWM_MODE == 2) || (AUDIO_PWM_MODE == 3))
    gpio_set_fun_output_port(pwm_io_N, FO_MCPWM_CH3L, 0, 1);
    gpio_set_die(pwm_io_N, 1);
    gpio_set_direction(pwm_io_N, 0);
    gpio_set_pull_up(pwm_io_N, 0);
    gpio_set_pull_down(pwm_io_N, 0);
#endif

    JL_MCPWM->CH3_CON0 = 0;
    JL_MCPWM->CH3_CON0 |= BIT(2);   // PWM H enable
    JL_MCPWM->CH3_CON0 |= BIT(3);   // PWM L enable

    JL_MCPWM->CH3_CON0 &= ~BIT(4);  // PWM H 信号反相使能
#if (AUDIO_PWM_MODE == 1)
    JL_MCPWM->CH3_CON0 |=  BIT(5);  // PWM L 信号反相使能
#elif ((AUDIO_PWM_MODE == 2) || (AUDIO_PWM_MODE == 3))
    JL_MCPWM->CH3_CON0 &= ~BIT(5);  // PWM L 信号反相使能
#endif

    SFR(JL_MCPWM->CH3_CON0, 0, 2, 2);   //  duty update type select
    SFR(JL_MCPWM->CH3_CON1, 8, 3, 3);   // MCPWM CH3  select MCTIMER3
    JL_MCPWM->MCPWM_CON0 |= BIT(0 + 3); // MCPWM CH3  enable

    audio_pwm_state = STATE_START;
}

void usr_mcpwm_uninit(void)
{

    pwm_is_closed = 1;
}

#endif

static void audio_pwm_task(void *param)
{
    printf(">>>> audio_pwm_task\n");
    u32 buf_points = 0;
    while (1) {
        if (pwm_need_resume) {
            if (rptr < wptr) {
                buf_points = wptr - rptr;
            } else if (rptr == wptr) {
                buf_points = 0;
            } else {
                buf_points = BUF_SIZE - rptr + wptr;
            }
            if (buf_points < 1024) {    //32 ms
                if (pwm_resume_handler != NULL) {
                    pwm_resume_handler(NULL);
                }
                /* putchar('R'); */
            } else {
                /* putchar('N'); */
            }
        } else {
            /* printf("enter audio_pwm.c %d\n",__LINE__); */
            os_sem_pend(&pwm_need_resume_sem, 0); //在不需要跑任务的时候pend住，避免任务频繁起来
            /* printf("enter audio_pwm.c %d\n",__LINE__); */
        }
        os_time_dly(1);
    }
}

void audio_pwm_set_resume(void (*resume)(void *))
{
    pwm_resume_handler = resume;
}

void audio_pwm_open(void)
{
    printf("audio_pwm_open\n");
    clk_set("sys", 96 * 1000000L);
    os_sem_create(&pwm_need_resume_sem, 0);
    os_task_create(audio_pwm_task, NULL, 2, 1024, 128, "audio_pwm_task");
#if (AUDIO_PWM_SOURCE == 0) // timer
    usr_pwm_timer_init();
#elif (AUDIO_PWM_SOURCE == 1) // mcpwm
    usr_mcpwm_init();
#endif  //  timer of mcpwm
    //开机进低功耗要把输出清0，不然有杂声
    JL_MCPWM->CH3_CMPH = PWM_DEAD_AREA;                 // P
    JL_MCPWM->CH3_CMPL = PWM_DEAD_AREA;                 // N
    /* pwm_is_closed = 0; */
}

void audio_pwm_close(void)
{
#if (AUDIO_PWM_SOURCE == 0) // timer
    usr_pwm_timer_uninit();
#elif (AUDIO_PWM_SOURCE == 1) // mcpwm
    usr_mcpwm_uninit();
#endif  //  timer of mcpwm

}

void audio_pwm_start(void)
{
    audio_pwm_state = STATE_FADE_IN;
#if (AUDIO_PWM_SOURCE == 0) // timer
    audio_pwm_cur = JL_TIMER3->PWM * FADE_STEP;
#elif (AUDIO_PWM_SOURCE == 1) // mcpwm
    audio_pwm_cur = JL_MCPWM->CH3_CMPH;
#endif  //  timer of mcpwm

    audio_pwm_target = audio_pwm_zero * FADE_STEP;
}

void audio_pwm_stop(void)
{
    audio_pwm_state = STATE_FADE_OUT;
#if (AUDIO_PWM_SOURCE == 0) // timer
    audio_pwm_cur = JL_TIMER3->PWM * FADE_STEP;
#elif (AUDIO_PWM_SOURCE == 1) // mcpwm
    audio_pwm_cur = JL_MCPWM->CH3_CMPH;
#endif  //  timer of mcpwm

    audio_pwm_target = 0;
}

static int __audio_pwm_write(s16 *data, u32 len)
{
    u32 free_points = 0;
    u32 wpoints = len / 2;
    local_irq_disable();
    if (rptr <= wptr) {
        if (rptr == 0) {  // 不能让 wptr 和 rptr 碰到一起
            free_points = BUF_SIZE - wptr - 1;
        } else {
            free_points = BUF_SIZE - wptr;
        }
    } else {
        free_points = rptr - wptr - 1;
    }

    if (free_points == 0) {
        local_irq_enable();
        return 0;
    }

    if (wpoints > free_points) {
        wpoints = free_points;
    }

    memcpy(&(audio_pwm_cbuf[wptr]), data, wpoints * 2);
    wptr += wpoints;
    if (wptr >= BUF_SIZE) {
        wptr = 0;
    }
    local_irq_enable();
    return wpoints * 2;
}

int audio_pwm_write(s16 *data, u32 len)
{
    u32 wlen = 0;
    wlen = __audio_pwm_write(data, len);
    if (len != wlen) {
        data += wlen / 2;
        wlen += __audio_pwm_write(data, len - wlen);
    }

    if (wlen != len) {
        /* putchar('S'); */
        pwm_need_resume = 1;
        os_sem_post(&pwm_need_resume_sem);
    } else {
        /* putchar('W'); */
        pwm_need_resume = 0;
        os_sem_set(&pwm_need_resume_sem, 0);
    }
    return wlen;
}

static u8 pwm_demo_idle_query()
{
    return pwm_is_closed;
}

REGISTER_LP_TARGET(pwm_demo_lp_target) = {
    .name = "pwm_demo",
    .is_idle = pwm_demo_idle_query,
};


