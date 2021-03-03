/*
 ****************************************************************************
 *							Audio Mic_Capless Module
 *省电容mic模块
 *即直接将mic的信号输出接到芯片的MICIN引脚(PA1/PB8)
 ****************************************************************************
 */
#include "system/includes.h"
#include "media/includes.h"
#include "app_config.h"
#include "app_action.h"
#include "app_main.h"
#include "audio_config.h"

#define LOG_TAG             "[APP_AUDIO]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define TCFG_AUDIO_MC_ENABLE	1	//mic_capless_enable

#define MC_LOG_ENABLE
#ifdef MC_LOG_ENABLE
#define MC_LOG_I	g_printf
#define MC_LOG_E	r_printf
#else
#define MC_LOG_E(...)
#define MC_LOG_I(...)
#endif/*MC_LOG_ENABLE*/

#if TCFG_AUDIO_MC_ENABLE
extern struct adc_platform_data adc_data;
extern void delay_2ms(int cnt);
extern void wdt_clear(void);
typedef struct {
    u8 save_bias_res;	/*trim结果是否保存*/
    u8 trim;			/*trim状态*/
    u8 cur_bias_res;
    u8 cur_bias1_res;
    OS_SEM sem;
} MicCapless_trim_t;
MicCapless_trim_t *mc_trim = NULL;

#define MC_TRIM_P	BIT(0)
#define MC_TRIM_N	BIT(1)

#define LADC_CAPLESS_ADJUST_SAVE
#ifdef LADC_CAPLESS_ADJUST_SAVE
#define DIFF_RANGE		50
#define CFG_DIFF_RANGE	100
#define CHECK_INTERVAL  10
#endif/*LADC_CAPLESS_ADJUST_SAVE*/

#define DACDTB_RANGE_MAX	8100
#define DACDTB_RANGE_MIN	6600

#define MIC_CAPLESS_ADJUST_BUD_DEFAULT	0
#define MIC_CAPLESS_ADJUST_BUD			100

#define MC_TRIM_SUCC	0
#define MC_TRIM_ERR		1

#define MC_TRIM_TIMEOUT				300

static const u8 mic_bias_tab[] = {0, 20, 12, 28, 4, 18, 10, 26, 2, 22, 14, 30, 17, 21, 6, 25, 29, 27, 31, 5, 3, 7};
#define MIC_BIAS_RSEL(x) 	SFR(JL_ADDA->ADA_CON1, 2, 5, x)
#define MIC1_BIAS_RSEL(x) 	SFR(JL_ADDA->ADA_CON2, 2, 5, x)

/*不支持自动校准，使用快速收敛*/
#if TCFG_MC_BIAS_AUTO_ADJUST
u8	mic_capless_adjust_bud = MIC_CAPLESS_ADJUST_BUD_DEFAULT;
#else
u8	mic_capless_adjust_bud = MIC_CAPLESS_ADJUST_BUD;
#endif/*TCFG_MC_BIAS_AUTO_ADJUST*/

u32 read_capless_DTB(void)
{
    u32 dacr32 = 0;
    int ret = syscfg_read(CFG_DAC_DTB, &dacr32, sizeof(dacr32));
    if (ret != sizeof(dacr32)) {
        /*
         *没有记忆值,使用默认值
         *没有收敛值的时候，使用快速收敛
         */
        //printf("DAC_DTB NULL,use fast feedback");
        mic_capless_adjust_bud = MIC_CAPLESS_ADJUST_BUD;
        return (DACR32_DEFAULT | (DACR32_DEFAULT << 16));
    }
    y_printf("cfg DTB0:%d,DTB1:%d,DTB:%x,ret = %d\n", (dacr32 & 0xFFFF), (dacr32 >> 16), dacr32, ret);
    return dacr32;
}

static u32 read_vm_capless_DTB(void)
{
    u32 vm_dacr32 = 0;
    int ret = syscfg_read(CFG_DAC_DTB, &vm_dacr32, sizeof(vm_dacr32));
    if (ret != sizeof(vm_dacr32)) {
        return (DACR32_DEFAULT | (DACR32_DEFAULT << 16));
    }
    printf("vm DTB0:%d,DTB1:%d,DTB:%x,ret = %d\n", (vm_dacr32 & 0xFFFF), (vm_dacr32 >> 16), vm_dacr32, ret);
    return vm_dacr32;
}

typedef struct {
    u8 adjust_complete;
    u8 dtb_step_limit;	//限制dtb收敛步进
    u16 save_dtb;
    u16 save_dtb1;
} mc_var_t;
mc_var_t mc_var = {
    .save_dtb = DACR32_DEFAULT,
    .save_dtb1 = DACR32_DEFAULT,
    .adjust_complete = 0,
    .dtb_step_limit = 0,
};

#if TCFG_MC_DTB_STEP_LIMIT
/*获取省电容mic收敛信息配置*/
int get_mc_dtb_step_limit(void)
{
    return mc_var.dtb_step_limit;
}
#endif /*TCFG_MC_DTB_STEP_LIMIT*/

void save_capless_DTB()
{
    s16 diff;
    u32 save_dac_dtb = 0;
    MC_LOG_I("save_capless_DTB\n");
    /*
     *(1)dtb0或者dtb1有一个不是默认值
     *(2)收敛成功
     */
    if (((mc_var.save_dtb != DACR32_DEFAULT) || (mc_var.save_dtb1 != DACR32_DEFAULT)) && mc_var.adjust_complete) {
        /*读取记忆值，比较是否需要更新配置*/
        u32 cfg_dacr32 = read_vm_capless_DTB();
        /*分离dtb0和dtb1*/
        u16 cfg_dtb0 = (cfg_dacr32 & 0xFFFF);
        u16 cfg_dtb1 = (cfg_dacr32 >> 16);
        mc_var.adjust_complete = 0;
        log_info("save_dtb0:%d,save_dtb1:%d\n", mc_var.save_dtb, mc_var.save_dtb1);
        log_info("cfg_dtb:0x%x,cfg_dtb0:%d,cfg_dtb1:%d\n", cfg_dacr32, cfg_dtb0, cfg_dtb1);

        diff = mc_var.save_dtb - cfg_dtb0;
        if ((mc_var.save_dtb != DACR32_DEFAULT) && ((cfg_dtb0 == DACR32_DEFAULT) || ((diff < -CFG_DIFF_RANGE) || (diff > CFG_DIFF_RANGE)))) {
            save_dac_dtb = mc_var.save_dtb;
            log_info("dtb0 update:%d,0x%x,save_dac_dtb:%x\n", mc_var.save_dtb, mc_var.save_dtb, save_dac_dtb);
        } else {
            save_dac_dtb = cfg_dtb0;
            log_info("dtb0 need't update:%d,diff:%d\n", mc_var.save_dtb, diff);
        }

        diff = mc_var.save_dtb1 - cfg_dtb1;
        if ((mc_var.save_dtb1 != DACR32_DEFAULT) && ((cfg_dtb1 == DACR32_DEFAULT) || ((diff < -CFG_DIFF_RANGE) || (diff > CFG_DIFF_RANGE)))) {
            save_dac_dtb |= (mc_var.save_dtb1 << 16);
            log_info("dtb1 update:%d,0x%x,save_dac_dtb:%x\n", mc_var.save_dtb1, mc_var.save_dtb1, save_dac_dtb);
        } else {
            save_dac_dtb |= (cfg_dtb1 << 16);
            log_info("dtb1 need't update:%d,diff:%d\n", mc_var.save_dtb1, diff);
        }

        log_info("dtb0:0x%x,dtb1:0x%x,dtb:0x%x\n", mc_var.save_dtb, mc_var.save_dtb1, save_dac_dtb);
        if (save_dac_dtb != cfg_dacr32) {
            MC_LOG_I("store mc dtb:%x\n", save_dac_dtb);
            syscfg_write(CFG_DAC_DTB, &save_dac_dtb, 4);
            /* u32 tmp_dacr32 = 0;
            syscfg_read(CFG_DAC_DTB,&tmp_dacr32,4);
            MC_LOG_I("read mc dtb:%x\n",tmp_dacr32); */
        }
    } else {
        log_info("dacr32 adjust uncomplete:%d,%d,complete:%d\n", mc_var.save_dtb, mc_var.save_dtb1, mc_var.adjust_complete);
    }
}

/*
 *收敛快调慢调边界
 */
u16 get_ladc_capless_bud(void)
{
    //printf("mc_bud:%d",mic_capless_adjust_bud);
    return mic_capless_adjust_bud;
}

extern void audio_adc_mic_trim_open(u8 mic_idx, u8 gain);
extern void audio_adc_mic_trim_close(u8 mic_idx);

static u8 audio_mc_idle_query(void)
{
    return (mc_trim ? 0 : 1);
}
REGISTER_LP_TARGET(audio_mc_device_lp_target) = {
    .name = "audio_mc_device",
    .is_idle = audio_mc_idle_query,
};

void mic_capless_trim_init(u8 mic_idx)
{
    log_info("mic_capless_trim_init:%d\n", mic_idx);
    mc_trim = zalloc(sizeof(MicCapless_trim_t));
    if (mc_trim) {
        if (mic_idx == LADC_CH_MIC_L) {
            mc_trim->cur_bias_res = adc_data.mic_bias_res;
        } else {
            mc_trim->cur_bias1_res = adc_data.mic1_bias_res;
        }
        os_sem_create(&mc_trim->sem, 0);
        mic_capless_adjust_bud = MIC_CAPLESS_ADJUST_BUD;
        audio_adc_mic_trim_open(TCFG_AUDIO_ADC_MIC_CHA, app_var.aec_mic_gain);
        log_info("mic_capless_trim_init succ\n");
    } else {
        log_info("mic_capless_trim_init err\n");
    }
}

void mic_capless_trim_exit(u8 trim_result)
{
    audio_adc_mic_trim_close(TCFG_AUDIO_ADC_MIC_CHA);

    if (trim_result) {
        log_info("save trim value,bias_res:%d,dtb0:%d,dtb1:%d\n", adc_data.mic_bias_res, mc_var.save_dtb, mc_var.save_dtb1);
        log_info("save trim value,save_bias:%d,complete:%d\n", mc_trim->save_bias_res, mc_var.adjust_complete);
        if (mc_trim->save_bias_res) {
            int ret = syscfg_write(CFG_MC_BIAS, &adc_data.mic_bias_res, 1);
        }
        if (mc_var.adjust_complete) {
            save_capless_DTB();
        }
        mic_capless_adjust_bud = MIC_CAPLESS_ADJUST_BUD_DEFAULT;
        MC_LOG_E("mc_trim succ,use default adjust bud:%d\n", mic_capless_adjust_bud);
    } else {
        mic_capless_adjust_bud = MIC_CAPLESS_ADJUST_BUD;
        MC_LOG_E("mc_trim err,use fast adjust bud:%d\n", mic_capless_adjust_bud);
    }

    free(mc_trim);
    mc_trim = NULL;
    log_info("mic_capless_bias_adjust_exit\n");
}

void mic_capless_trim_result(u8 result)
{
    if (mc_trim) {
        /*预收敛*/
        log_info("mc_pre_auto_adjust_result:%d\n", result);
        if (mc_trim->trim && (mc_trim->trim < (MC_TRIM_P | MC_TRIM_N))) {
            mc_trim->save_bias_res = 1;
            adc_data.mic_bias_res = mc_trim->cur_bias_res;
        }
        os_sem_post(&mc_trim->sem);
    } else {
        /*运行过程收敛*/
        log_info("mc_run_auto_adjust_result:%d\n", result);
    }
}

#define POWER_RESET_VDDIO_POR	BIT(0)	/*上电reset*/
#define POWER_RESET_PPINR		BIT(3)	/*长按reset*/
extern u8 power_reset_flag;
/*
 *检查mic偏置是否需要校准,以下情况需要重新校准：
 *1、power on reset
 *2、vm被擦除
 */
u8 mc_bias_adjust_check(void)
{
    u8 por_flag = 0;
#if (TCFG_MC_BIAS_AUTO_ADJUST == MC_BIAS_ADJUST_ALWAYS)
    return 1;
#endif
    int ret = syscfg_read(CFG_POR_FLAG, &por_flag, 1);
    if (ret == 1) {
        if (por_flag == 0xA5) {
            log_info("power on reset 1");
            por_flag = 0;
            ret = syscfg_write(CFG_POR_FLAG, &por_flag, 1);
            return 1;
        }
    }
    if (power_reset_flag & POWER_RESET_VDDIO_POR) {
        log_info("power on reset 2");
        return 1;
    }
    if (read_vm_capless_DTB() == (DACR32_DEFAULT | (DACR32_DEFAULT << 16))) {
        log_info("vm format");
        return 1;
    }

    return 0;
}

void mc_trim_init(int update)
{
    u8 por_flag = 0;
    u8 cur_por_flag = 0;
    if (adc_data.mic_capless == 0) {
        return;
    }
    /*
     *1.update
     *2.power_on_reset
     *3.pin reset
     */
    if (update || (power_reset_flag & POWER_RESET_VDDIO_POR) || (power_reset_flag & POWER_RESET_PPINR)) {
        log_info("reset_flag:0x%x", power_reset_flag);
        cur_por_flag = 0xA5;
    }
    int ret = syscfg_read(CFG_POR_FLAG, &por_flag, 1);
    log_info("POR flag:%x-%x", cur_por_flag, por_flag);
    if ((cur_por_flag == 0xA5) && (por_flag != cur_por_flag)) {
        //log_info("update POR flag");
        /*需要保存上电状态时因为开机有可能进入soft poweroff，下次开机的时候需要知道上次的开机状态*/
        ret = syscfg_write(CFG_POR_FLAG, &cur_por_flag, 1);
    }
}

/*
 * 省电容mic预收敛
 */
void mic_trim_run()
{
    int ret = 0;

#if (!(TCFG_AUDIO_ADC_MIC_CHA & (LADC_CH_MIC_L | LADC_CH_MIC_R)))
    return;
#endif
    if (adc_data.mic_capless == 0) {
        return;
    }

    log_info("mic_trim_run\n");
#if TCFG_MC_BIAS_AUTO_ADJUST
    if (mc_bias_adjust_check()) {
        //printf("MC_BIAS_ADJUST...");
        /*
         *预收敛条件：
         *1、开机检查发现mic的偏置非法，则校准回来，同时重新收敛,比如中途更换mic头的情况
         *2、收敛值丢失（vm被擦除），重新收敛一次(前提是校准成功)
         */
        mc_var.dtb_step_limit = 0;
        mic_capless_trim_init(TCFG_AUDIO_ADC_MIC_CHA);
        ret = os_sem_pend(&mc_trim->sem, MC_TRIM_TIMEOUT);
        if (ret == OS_TIMEOUT) {
            log_info("mc_trim timeout!\n");
            mic_capless_trim_exit(0);
        } else {
            log_info("mc_trim ok\n");
            mc_var.dtb_step_limit = TCFG_MC_DTB_STEP_LIMIT;
            mic_capless_trim_exit(1);
        }
    } else {
        log_info("mc_need't trim");
        mc_var.dtb_step_limit = TCFG_MC_DTB_STEP_LIMIT;
    }
#endif/*TCFG_MC_BIAS_AUTO_ADJUST*/
}

void ladc_capless_adjust_post(s32 dacr32, u8 begin)
{
    static s32 last_dacr32 = 0;
    static u8 check_cnt = 0;
    u8 bias_rsel;
    s32 dacr32_diff;

    /*adjust_begin,clear*/
    if (begin) {
        last_dacr32 = 0;
        check_cnt = 0;
        mc_var.adjust_complete = 0;
        mc_var.save_dtb = DACR32_DEFAULT;
        return;
    }
#if TCFG_MC_CONVERGE_TRACE
    printf("<%d>", dacr32);
#endif/*TCFG_MC_CONVERGE_TRACE*/
    if (mc_var.adjust_complete == 0) {
        if (++check_cnt > CHECK_INTERVAL) {
            check_cnt = 0;
#if TCFG_MC_BIAS_AUTO_ADJUST
            if (mc_trim) {
                if (dacr32 > DACDTB_RANGE_MAX) {
                    if (mc_trim->cur_bias_res < 21) {
                        mc_trim->cur_bias_res++;
                    } else {
                        printf("[Assert]mic_bias_rsel err 1!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                        ASSERT(0, "mic_bias_rsel err 1\n");
                    }
                    bias_rsel = mic_bias_tab[mc_trim->cur_bias_res];
                    y_printf("[-]mic_bias_rsel:%d = %d", mc_trim->cur_bias_res, bias_rsel);
                    MIC_BIAS_RSEL(bias_rsel);
                    mc_trim->trim |= MC_TRIM_P;
                    return;
                } else if (dacr32 < DACDTB_RANGE_MIN) {
                    if (mc_trim->cur_bias_res > 1) {
                        mc_trim->cur_bias_res--;
                    } else {
                        printf("[Assert]mic_bias_rsel err 2!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
                        ASSERT(0, "mic_bias_rsel err 2\n");
                    }
                    bias_rsel = mic_bias_tab[mc_trim->cur_bias_res];
                    y_printf("[+]mic_bias_rsel:%d = %d", mc_trim->cur_bias_res, bias_rsel);
                    MIC_BIAS_RSEL(bias_rsel);
                    mc_trim->trim |= MC_TRIM_N;
                    return;
                }

                if (mc_trim->trim == (MC_TRIM_P | MC_TRIM_N)) {
                    mic_capless_trim_result(MC_TRIM_ERR);
                }
            }
#endif/*TCFG_MC_BIAS_AUTO_ADJUST*/

            dacr32_diff = dacr32 - last_dacr32;
            //printf("[capless:%d-%d-%d]",dacr32,last_dacr32,dacr32_diff);
            last_dacr32 = dacr32;
            if (mc_var.adjust_complete == 0) {
                mc_var.save_dtb = dacr32;
            }

            /*调整稳定*/
            if ((dacr32_diff > -DIFF_RANGE) && (dacr32_diff < DIFF_RANGE)) {
                log_info("adjust_OK:%d\n", dacr32);
                mc_var.adjust_complete = 1;
#if TCFG_MC_BIAS_AUTO_ADJUST
                mic_capless_trim_result(MC_TRIM_SUCC);
#endif/*TCFG_MC_BIAS_AUTO_ADJUST*/
            }
        }
    }
}
#endif/*TCFG_AUDIO_MC_ENABLE*/

