#include "system/includes.h"
#include "media/includes.h"

#include "app_config.h"
#include "app_action.h"
#include "app_main.h"

#include "audio_config.h"
/*软件数字音量*/
#include "audio_digital_vol.h"
/*硬件数字音量*/
#include "audio_dac_digital_volume.h"
#if TCFG_AUDIO_ANC_ENABLE
#include "audio_anc.h"
#endif/*TCFG_AUDIO_ANC_ENABLE*/

#include "update.h"

#define LOG_TAG             "[APP_AUDIO]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define DEFAULT_DIGTAL_VOLUME   16384

typedef short unaligned_u16 __attribute__((aligned(1)));
struct app_audio_config {
    u8 state;
    u8 prev_state;
    u8 mute_when_vol_zero;
    volatile u8 fade_gain_l;
    volatile u8 fade_gain_r;
    volatile s16 fade_dgain_l;
    volatile s16 fade_dgain_r;
    volatile s16 fade_dgain_step_l;
    volatile s16 fade_dgain_step_r;
    volatile int fade_timer;
    s16 digital_volume;
    u8 analog_volume_l;
    u8 analog_volume_r;
    atomic_t ref;
    s16 max_volume[APP_AUDIO_STATE_WTONE + 1];
    u8 sys_cvol_max;
    u8 call_cvol_max;
    unaligned_u16 *sys_cvol;
    unaligned_u16 *call_cvol;
};
static const char *audio_state[] = {
    "idle",
    "music",
    "call",
    "tone",
    "err",
};

static struct app_audio_config app_audio_cfg = {0};

#define __this      (&app_audio_cfg)
extern struct audio_dac_hdl dac_hdl;
extern struct dac_platform_data dac_data;

/*关闭audio相关模块使能*/
void audio_disable_all(void)
{
    //DAC:DACEN
    JL_AUDIO->DAC_CON &= ~BIT(4);
    //ADC:ADCEN
    JL_AUDIO->ADC_CON &= ~BIT(4);
    //EQ:
    JL_EQ->CON0 &= ~BIT(0);
    //FFT:
    JL_FFT->CON = BIT(1);//置1强制关闭模块，不管是否已经运算完成
    //ANC:anc_en anc_start
    JL_ANC->CON0 &= ~(BIT(1) | BIT(29));

}

REGISTER_UPDATE_TARGET(audio_update_target) = {
    .name = "audio",
    .driver_close = audio_disable_all,
};
/*
 *************************************************************
 *
 *	audio volume fade
 *
 *************************************************************
 */

static void audio_fade_timer(void *priv)
{
    u8 gain_l = dac_hdl.vol_l;
    u8 gain_r = dac_hdl.vol_r;
    //printf("<fade:%d-%d-%d-%d>", gain_l, gain_r, __this->fade_gain_l, __this->fade_gain_r);
    local_irq_disable();
    if ((gain_l == __this->fade_gain_l) && (gain_r == __this->fade_gain_r)) {
        usr_timer_del(__this->fade_timer);
        __this->fade_timer = 0;
        /*音量为0的时候mute住*/
        audio_dac_set_L_digital_vol(&dac_hdl, gain_l ? __this->digital_volume : 0);
        audio_dac_set_R_digital_vol(&dac_hdl, gain_r ? __this->digital_volume : 0);
        if ((gain_l == 0) && (gain_r == 0)) {
            if (__this->mute_when_vol_zero) {
                __this->mute_when_vol_zero = 0;
                audio_dac_mute(&dac_hdl, 1);
            }
        }
        local_irq_enable();
        /*
         *淡入淡出结束，也把当前的模拟音量设置一下，防止因为淡入淡出的音量和保存音量的变量一致，
         *而寄存器已经被清0的情况
         */
        audio_dac_set_analog_vol(&dac_hdl, gain_l);
        /* log_info("dac_fade_end, VOL : 0x%x 0x%x\n", JL_ADDA->DAA_CON1, JL_AUDIO->DAC_VL0); */
        return;
    }
    if (gain_l > __this->fade_gain_l) {
        gain_l--;
    } else if (gain_l < __this->fade_gain_l) {
        gain_l++;
    }

    if (gain_r > __this->fade_gain_r) {
        gain_r--;
    } else if (gain_r < __this->fade_gain_r) {
        gain_r++;
    }
    audio_dac_set_analog_vol(&dac_hdl, gain_l);
    local_irq_enable();
}

static int audio_fade_timer_add(u8 gain_l, u8 gain_r)
{
    /* r_printf("dac_fade_begin:0x%x,gain_l:%d,gain_r:%d\n", __this->fade_timer, gain_l, gain_r); */
    local_irq_disable();
    __this->fade_gain_l = gain_l;
    __this->fade_gain_r = gain_r;
    if (__this->fade_timer == 0) {
        __this->fade_timer = usr_timer_add((void *)0, audio_fade_timer, 2, 1);
        /* y_printf("fade_timer:0x%x", __this->fade_timer); */
    }
    local_irq_enable();

    return 0;
}


#if (SYS_VOL_TYPE == VOL_TYPE_AD)

#define DGAIN_SET_MAX_STEP (300)
#define DGAIN_SET_MIN_STEP (30)

static unsigned short combined_vol_list[17][2] = {
    { 0,     0}, //0: None
    { 0,  2657}, // 1:-45.00 db
    { 0,  3476}, // 2:-42.67 db
    { 0,  4547}, // 3:-40.33 db
    { 0,  5948}, // 4:-38.00 db
    { 0,  7781}, // 5:-35.67 db
    { 0, 10179}, // 6:-33.33 db
    { 0, 13316}, // 7:-31.00 db
    { 1, 14198}, // 8:-28.67 db
    { 2, 14851}, // 9:-26.33 db
    { 3, 15596}, // 10:-24.00 db
    { 5, 13043}, // 11:-21.67 db
    { 6, 13567}, // 12:-19.33 db
    { 7, 14258}, // 13:-17.00 db
    { 8, 14749}, // 14:-14.67 db
    { 9, 15389}, // 15:-12.33 db
    {10, 16144}, // 16:-10.00 db
};

static unsigned short call_combined_vol_list[16][2] = {
    { 0,     0}, //0: None
    { 0,  4725}, // 1:-40.00 db
    { 0,  5948}, // 2:-38.00 db
    { 0,  7488}, // 3:-36.00 db
    { 0,  9427}, // 4:-34.00 db
    { 0, 11868}, // 5:-32.00 db
    { 0, 14941}, // 6:-30.00 db
    { 1, 15331}, // 7:-28.00 db
    { 2, 15432}, // 8:-26.00 db
    { 3, 15596}, // 9:-24.00 db
    { 4, 15788}, // 10:-22.00 db
    { 5, 15802}, // 11:-20.00 db
    { 6, 15817}, // 12:-18.00 db
    { 7, 15998}, // 13:-16.00 db
    { 8, 15926}, // 14:-14.00 db
    { 9, 15991}, // 15:-12.00 db
};

void audio_combined_vol_init(u8 cfg_en)
{
    u16 sys_cvol_len = 0;
    u16 call_cvol_len = 0;
    u8 *sys_cvol  = NULL;
    u8 *call_cvol  = NULL;
    s16 *cvol;

    __this->sys_cvol_max = ARRAY_SIZE(combined_vol_list) - 1;
    __this->sys_cvol = combined_vol_list;
    __this->call_cvol_max = ARRAY_SIZE(call_combined_vol_list) - 1;
    __this->call_cvol = call_combined_vol_list;

    if (cfg_en) {
        sys_cvol  = syscfg_ptr_read(CFG_COMBINE_SYS_VOL_ID, &sys_cvol_len);
        //ASSERT(((u32)sys_cvol & BIT(0)) == 0, "sys_cvol addr unalignd(2):%x\n", sys_cvol);
        if (sys_cvol && sys_cvol_len) {
            __this->sys_cvol = (unaligned_u16 *)sys_cvol;
            __this->sys_cvol_max = sys_cvol_len / 4 - 1;
            //y_printf("read sys_combine_vol succ:%x,len:%d",__this->sys_cvol,sys_cvol_len);
            /* cvol = __this->sys_cvol;
            for(int i = 0,j = 0;i < (sys_cvol_len / 2);j++) {
            	printf("sys_vol %d: %d, %d\n",j,*cvol++,*cvol++);
            	i += 2;
            } */
        } else {
            r_printf("read sys_cvol false:%x,%x\n", sys_cvol, sys_cvol_len);
        }

        call_cvol  = syscfg_ptr_read(CFG_COMBINE_CALL_VOL_ID, &call_cvol_len);
        //ASSERT(((u32)call_cvol & BIT(0)) == 0, "call_cvol addr unalignd(2):%d\n", call_cvol);
        if (call_cvol && call_cvol_len) {
            __this->call_cvol = (unaligned_u16 *)call_cvol;
            __this->call_cvol_max = call_cvol_len / 4 - 1;
            //y_printf("read call_combine_vol succ:%x,len:%d",__this->call_cvol,call_cvol_len);
            /* cvol = __this->call_cvol;
            for(int i = 0,j = 0;i < call_cvol_len / 2;j++) {
            	printf("call_vol %d: %d, %d\n",j,*cvol++,*cvol++);
            	i += 2;
            } */
        } else {
            r_printf("read call_combine_vol false:%x,%x\n", call_cvol, call_cvol_len);
        }
    }

    log_info("sys_cvol_max:%d,call_cvol_max:%d\n", __this->sys_cvol_max, __this->call_cvol_max);
}


static void audio_combined_fade_timer(void *priv)
{
    u8 gain_l = dac_hdl.vol_l;
    u8 gain_r = dac_hdl.vol_r;
    s16 dgain_l = dac_hdl.d_volume[DA_LEFT];
    s16 dgain_r = dac_hdl.d_volume[DA_RIGHT];

    __this->fade_dgain_step_l = __builtin_abs(dgain_l - __this->fade_dgain_l) / \
                                (__builtin_abs(gain_l - __this->fade_gain_l) + 1);
    if (__this->fade_dgain_step_l > DGAIN_SET_MAX_STEP) {
        __this->fade_dgain_step_l = DGAIN_SET_MAX_STEP;
    } else if (__this->fade_dgain_step_l < DGAIN_SET_MIN_STEP) {
        __this->fade_dgain_step_l = DGAIN_SET_MIN_STEP;
    }

    __this->fade_dgain_step_r = __builtin_abs(dgain_r - __this->fade_dgain_r) / \
                                (__builtin_abs(gain_r - __this->fade_gain_r) + 1);
    if (__this->fade_dgain_step_r > DGAIN_SET_MAX_STEP) {
        __this->fade_dgain_step_r = DGAIN_SET_MAX_STEP;
    } else if (__this->fade_dgain_step_r < DGAIN_SET_MIN_STEP) {
        __this->fade_dgain_step_r = DGAIN_SET_MIN_STEP;
    }

    /* log_info("<a:%d-%d-%d-%d d:%d-%d-%d-%d-%d-%d>\n", \ */
    /* gain_l, gain_r, __this->fade_gain_l, __this->fade_gain_r, \ */
    /* dgain_l, dgain_r, __this->fade_dgain_l, __this->fade_dgain_r, \ */
    /* __this->fade_dgain_step_l, __this->fade_dgain_step_r); */

    local_irq_disable();

    if ((gain_l == __this->fade_gain_l) \
        && (gain_r == __this->fade_gain_r) \
        && (dgain_l == __this->fade_dgain_l)\
        && (dgain_r == __this->fade_dgain_r)) {
        usr_timer_del(__this->fade_timer);
        __this->fade_timer = 0;
        /*音量为0的时候mute住*/
        if ((gain_l == 0) && (gain_r == 0)) {
            if (__this->mute_when_vol_zero) {
                __this->mute_when_vol_zero = 0;
                audio_dac_mute(&dac_hdl, 1);
            }
        }

        local_irq_enable();
        /* log_info("dac_fade_end,VOL:0x%x-0x%x-%d-%d-%d-%d\n", \ */
        /* JL_ADDA->DAA_CON1, JL_AUDIO->DAC_VL0,  \ */
        /* __this->fade_gain_l, __this->fade_gain_r, \ */
        /* __this->fade_dgain_l, __this->fade_dgain_r); */
        return;
    }
    if ((gain_l != __this->fade_gain_l) \
        || (gain_r != __this->fade_gain_r)) {
        if (gain_l > __this->fade_gain_l) {
            gain_l--;
        } else if (gain_l < __this->fade_gain_l) {
            gain_l++;
        }

        if (gain_r > __this->fade_gain_r) {
            gain_r--;
        } else if (gain_r < __this->fade_gain_r) {
            gain_r++;
        }

        audio_dac_set_analog_vol(&dac_hdl, gain_l);
        local_irq_enable();//fix : 不同时调模拟和数字可以避免杂音
        return;
    }

    if ((dgain_l != __this->fade_dgain_l) \
        || (dgain_r != __this->fade_dgain_r)) {

        if (gain_l != __this->fade_gain_l) {
            if (dgain_l > __this->fade_dgain_l) {
                if ((dgain_l - __this->fade_dgain_l) >= __this->fade_dgain_step_l) {
                    dgain_l -= __this->fade_dgain_step_l;
                } else {
                    dgain_l = __this->fade_dgain_l;
                }
            } else if (dgain_l < __this->fade_dgain_l) {
                if ((__this->fade_dgain_l - dgain_l) >= __this->fade_dgain_step_l) {
                    dgain_l += __this->fade_dgain_step_l;
                } else {
                    dgain_l = __this->fade_dgain_l;
                }
            }
        } else {
            dgain_l = __this->fade_dgain_l;
        }

        if (gain_r != __this->fade_gain_r) {
            if (dgain_r > __this->fade_dgain_r) {
                if ((dgain_r - __this->fade_dgain_r) >= __this->fade_dgain_step_r) {
                    dgain_r -= __this->fade_dgain_step_r;
                } else {
                    dgain_r = __this->fade_dgain_r;
                }
            } else if (dgain_r < __this->fade_dgain_r) {
                if ((__this->fade_dgain_r - dgain_r) >= __this->fade_dgain_step_r) {
                    dgain_r += __this->fade_dgain_step_r;
                } else {
                    dgain_r = __this->fade_dgain_r;
                }
            }
        } else {
            dgain_r = __this->fade_dgain_r;
        }
        audio_dac_set_digital_vol(&dac_hdl, dgain_l);
    }

    local_irq_enable();
}


static int audio_combined_fade_timer_add(u8 gain_l, u8 gain_r)
{
    u8  gain_max;
    u8  target_again_l = 0;
    u8  target_again_r = 0;
    u16 target_dgain_l = 0;
    u16 target_dgain_r = 0;

    if (__this->state == APP_AUDIO_STATE_CALL) {
        gain_max = __this->call_cvol_max;
        gain_l = (gain_l > gain_max) ? gain_max : gain_l;
        gain_r = (gain_r > gain_max) ? gain_max : gain_r;
        target_again_l = *(&__this->call_cvol[gain_l * 2]);
        target_again_r = *(&__this->call_cvol[gain_r * 2]);
        target_dgain_l = *(&__this->call_cvol[gain_l * 2 + 1]);
        target_dgain_r = *(&__this->call_cvol[gain_r * 2 + 1]);
    } else {
        gain_max = __this->sys_cvol_max;
        gain_l = (gain_l > gain_max) ? gain_max : gain_l;
        gain_r = (gain_r > gain_max) ? gain_max : gain_r;
        target_again_l = *(&__this->sys_cvol[gain_l * 2]);
        target_again_r = *(&__this->sys_cvol[gain_r * 2]);
        target_dgain_l = *(&__this->sys_cvol[gain_l * 2 + 1]);
        target_dgain_r = *(&__this->sys_cvol[gain_r * 2 + 1]);
    }

#if TCFG_AUDIO_ANC_ENABLE
    target_again_l = anc_dac_gain_get(ANC_DAC_CH_L);
    target_again_r = anc_dac_gain_get(ANC_DAC_CH_R);
#endif

    y_printf("[l]v:%d,Av:%d,Dv:%d", gain_l, target_again_l, target_dgain_l);
    //y_printf("[r]v:%d,Av:%d,Dv:%d", gain_r, target_again_r, target_dgain_r);
    /* log_info("dac_com_fade_begin:0x%x\n", __this->fade_timer); */

    local_irq_disable();

    __this->fade_gain_l  = target_again_l;
    __this->fade_gain_r  = target_again_r;
    __this->fade_dgain_l = target_dgain_l;
    __this->fade_dgain_r = target_dgain_r;

#if (RCSP_ADV_EN)
    extern u8 find_device_key_flag_get(void);
    if (find_device_key_flag_get()) {
        __this->fade_gain_l  = 15;
        __this->fade_gain_r  = 15;
        __this->fade_dgain_l = 16384;
        __this->fade_dgain_r = 16384;
    }
#endif

    if (__this->fade_timer == 0) {
        __this->fade_timer = usr_timer_add((void *)0, audio_combined_fade_timer, 2, 1);
        /* log_info("combined_fade_timer:0x%x", __this->fade_timer); */
    }

    local_irq_enable();

    return 0;
}

#endif/*SYS_VOL_TYPE == VOL_TYPE_AD*/


static void set_audio_device_volume(u8 type, s16 vol)
{
    audio_dac_set_analog_vol(&dac_hdl, vol);
}

static int get_audio_device_volume(u8 vol_type)
{
    return 0;
}

void volume_up_down_direct(s8 value)
{

}

void audio_fade_in_fade_out(u8 left_gain, u8 right_gain)
{
    /*根据audio state切换的时候设置的最大音量,限制淡入淡出的最大音量*/
    u8 max_vol_l = __this->max_volume[__this->state];
    u8 max_vol_r = max_vol_l;
    y_printf("[fade]state:%s,max_volume:%d,cur:%d,%d", audio_state[__this->state], max_vol_l, left_gain, left_gain);
    left_gain = left_gain > max_vol_l ? max_vol_l : left_gain;
    right_gain = right_gain > max_vol_r ? max_vol_r : right_gain;



#if TCFG_CALL_USE_DIGITAL_VOLUME
    if (__this->state == APP_AUDIO_STATE_CALL) {
        /*通话使用数字音量的时候，模拟音量默认最大*/
        left_gain = max_vol_l;
        right_gain = max_vol_r;
        //printf("phone_call_use_digital_volume,a_vol max:%d,%d",left_gain,right_gain);
    }
#endif/*TCFG_CALL_USE_DIGITAL_VOLUME*/

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
#if TCFG_AUDIO_ANC_ENABLE
    if (left_gain || (anc_mode_get() != ANC_OFF)) {
#if TCFG_AUDIO_ANC_DYNAMIC_DAC_GAIN
        u8 anc_left_gain = anc_dac_gain_get(ANC_DAC_CH_L);
        u8 anc_right_gain = anc_dac_gain_get(ANC_DAC_CH_R);
        //printf("anc_dac:%d %d,new_dac:%d,%d",anc_left_gain,anc_right_gain,left_gain,right_gain);
        if (left_gain < anc_left_gain) {
            left_gain = anc_left_gain;
        }
        if (right_gain < anc_right_gain) {
            right_gain = anc_right_gain;
        }
#else
        left_gain = anc_dac_gain_get(ANC_DAC_CH_L);
        right_gain = anc_dac_gain_get(ANC_DAC_CH_R);
#endif/*TCFG_AUDIO_ANC_DYNAMIC_DAC_GAIN*/
    }
    audio_fade_timer_add(left_gain, right_gain);
#else
    if (left_gain && right_gain) {
        audio_fade_timer_add(MAX_ANA_VOL, MAX_ANA_VOL);
    } else {
        audio_fade_timer_add(left_gain, right_gain);
    }
#endif/*TCFG_AUDIO_ANC_ENABLE*/


#elif (SYS_VOL_TYPE == VOL_TYPE_ANALOG)
    audio_fade_timer_add(left_gain, right_gain);

#elif (SYS_VOL_TYPE == VOL_TYPE_AD)
#if TCFG_CALL_USE_DIGITAL_VOLUME
    if (__this->state == APP_AUDIO_STATE_CALL) {
        audio_fade_timer_add(left_gain, right_gain);
    } else {
        audio_combined_fade_timer_add(left_gain, right_gain);
    }
#else
    audio_combined_fade_timer_add(left_gain, right_gain);
#endif/*TCFG_CALL_USE_DIGITAL_VOLUME*/

#elif (SYS_VOL_TYPE == VOL_TYPE_DIGITAL_HW)
    __this->digital_volume = left_gain * 16384 / get_max_sys_vol();
    //g_printf("digital_vol:%d",__this->digital_volume);
    audio_fade_timer_add(__this->analog_volume_l, __this->analog_volume_r);
#endif/*SYS_VOL_TYPE*/
}

void app_audio_set_volume(u8 state, s8 volume, u8 fade)
{
    switch (state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
        app_var.music_volume = volume;
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
        extern void a2dp_digital_vol_set(u8 vol);
        a2dp_digital_vol_set(volume);
        return;
#endif
        break;
    case APP_AUDIO_STATE_CALL:
        app_var.call_volume = volume;
#if TCFG_CALL_USE_DIGITAL_VOLUME
        dac_digital_vol_set(volume, volume, 1);
        return;
#endif
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
        extern void esco_digital_vol_set(u8 vol);
        esco_digital_vol_set(volume);
        return;
#endif
        break;
    case APP_AUDIO_STATE_WTONE:
        app_var.wtone_volume = volume;
        break;
    default:
        return;
    }
    printf("set_vol[%s]:%s=%d\n", audio_state[__this->state], audio_state[state], volume);
    if (state == __this->state) {
        audio_dac_set_volume(&dac_hdl, volume);
        if (audio_dac_get_status(&dac_hdl)) {
            if (fade) {
                audio_fade_in_fade_out(volume, volume);
            } else {
                audio_dac_set_analog_vol(&dac_hdl, volume);
            }
        }
    }
}

s8 app_audio_get_volume(u8 state)
{
    s8 volume = 0;
    switch (state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
        volume = app_var.music_volume;
        break;
    case APP_AUDIO_STATE_CALL:
        volume = app_var.call_volume;
        break;
    case APP_AUDIO_STATE_WTONE:
        volume = app_var.wtone_volume;
        if (!volume) {
            volume = app_var.music_volume;
        }
        break;
    case APP_AUDIO_CURRENT_STATE:
        volume = app_audio_get_volume(__this->state);
        break;
    default:
        break;
    }

    return volume;
}


static const char *audio_mute_string[] = {
    "mute_default",
    "unmute_default",
    "mute_L",
    "unmute_L",
    "mute_R",
    "unmute_R",
};

#define AUDIO_MUTE_FADE			1
#define AUDIO_UMMUTE_FADE		1
void app_audio_mute(u8 value)
{
    u8 volume = 0;
    printf("audio_mute:%s", audio_mute_string[value]);
    switch (value) {
    case AUDIO_MUTE_DEFAULT:
#if AUDIO_MUTE_FADE
        audio_fade_in_fade_out(0, 0);
        __this->mute_when_vol_zero = 1;
#else
        audio_dac_set_analog_vol(&dac_hdl, 0);
        audio_dac_mute(&dac_hdl, 1);
#endif
        break;
    case AUDIO_UNMUTE_DEFAULT:
#if AUDIO_UMMUTE_FADE
        audio_dac_mute(&dac_hdl, 0);
        volume = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
        audio_fade_in_fade_out(volume, volume);
#else
        audio_dac_mute(&dac_hdl, 0);
        volume = app_audio_get_volume(APP_AUDIO_CURRENT_STATE);
        audio_dac_set_analog_vol(&dac_hdl, volume);
#endif
        break;
    }
}


void app_audio_volume_up(u8 value)
{
    s16 volume = 0;
    switch (__this->state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
        app_var.music_volume += value;
        if (app_var.music_volume > get_max_sys_vol()) {
            app_var.music_volume = get_max_sys_vol();
        }
        volume = app_var.music_volume;
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
        extern void a2dp_digital_vol_set(u8 vol);
        a2dp_digital_vol_set(volume);
        return;
#endif
        break;
    case APP_AUDIO_STATE_CALL:
        app_var.call_volume += value;
        if (app_var.call_volume > app_var.aec_dac_gain) {
            app_var.call_volume = app_var.aec_dac_gain;
        }
        volume = app_var.call_volume;
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
        extern void esco_digital_vol_set(u8 vol);
        esco_digital_vol_set(volume);
        return;
#endif
#if TCFG_CALL_USE_DIGITAL_VOLUME
        dac_digital_vol_set(volume, volume, 1);
        return;
#endif
        break;
    case APP_AUDIO_STATE_WTONE:
        app_var.wtone_volume += value;
        if (app_var.wtone_volume > get_max_sys_vol()) {
            app_var.wtone_volume = get_max_sys_vol();
        }
        volume = app_var.wtone_volume;
        break;
    default:
        return;
    }
    app_audio_set_volume(__this->state, volume, 1);
}

void app_audio_volume_down(u8 value)
{
    s16 volume = 0;
    switch (__this->state) {
    case APP_AUDIO_STATE_IDLE:
    case APP_AUDIO_STATE_MUSIC:
        app_var.music_volume -= value;
        if (app_var.music_volume < 0) {
            app_var.music_volume = 0;
        }
        volume = app_var.music_volume;
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
        extern void a2dp_digital_vol_set(u8 vol);
        a2dp_digital_vol_set(volume);
        return;
#endif
        break;
    case APP_AUDIO_STATE_CALL:
        app_var.call_volume -= value;
        if (app_var.call_volume < 0) {
            app_var.call_volume = 0;
        }
        volume = app_var.call_volume;
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
        extern void esco_digital_vol_set(u8 vol);
        esco_digital_vol_set(volume);
        return;
#endif
#if TCFG_CALL_USE_DIGITAL_VOLUME
        dac_digital_vol_set(volume, volume, 1);
        return;
#endif
        break;
    case APP_AUDIO_STATE_WTONE:
        app_var.wtone_volume -= value;
        if (app_var.wtone_volume < 0) {
            app_var.wtone_volume = 0;
        }
        volume = app_var.wtone_volume;
        break;
    default:
        return;
    }

    app_audio_set_volume(__this->state, volume, 1);
}

/*level:0~15*/
static const u16 phone_call_dig_vol_tab[] = {
    0,	//0
    111,//1
    161,//2
    234,//3
    338,//4
    490,//5
    708,//6
    1024,//7
    1481,//8
    2142,//9
    3098,//10
    4479,//11
    6477,//12
    9366,//13
    14955,//14
    16384 //15
};

void app_audio_state_switch(u8 state, s16 max_volume)
{
    r_printf("audio state old:%s,new:%s,vol:%d\n", audio_state[__this->state], audio_state[state], max_volume);

    __this->prev_state = __this->state;
    __this->state = state;
    /*限制最大音量*/
#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL_HW)
    __this->analog_volume_l = MAX_ANA_VOL;
    __this->analog_volume_r = MAX_ANA_VOL;
#else
    __this->digital_volume = DEFAULT_DIGTAL_VOLUME;
#endif
    __this->max_volume[state] = max_volume;

#if (SYS_VOL_TYPE == VOL_TYPE_DIGITAL)
    __this->analog_volume_l = MAX_ANA_VOL;
    __this->analog_volume_r = MAX_ANA_VOL;
#if TCFG_AUDIO_ANC_ENABLE
#if TCFG_AUDIO_ANC_DYNAMIC_DAC_GAIN
    __this->max_volume[state] = TCFG_AUDIO_ANC_DYNAMIC_DAC_GAIN;
#endif/*TCFG_AUDIO_ANC_DYNAMIC_DAC_GAIN*/
    __this->analog_volume_l = anc_dac_gain_get(ANC_DAC_CH_L);
    __this->analog_volume_r = anc_dac_gain_get(ANC_DAC_CH_R);
    //g_printf("anc mode,vol_l:%d,vol_r:%d", __this->analog_volume_l, __this->analog_volume_r);
#endif/*TCFG_AUDIO_ANC_ENABLE*/
    __this->digital_volume = DEFAULT_DIGTAL_VOLUME;

#else
    if (__this->state == APP_AUDIO_STATE_CALL) {
#if TCFG_CALL_USE_DIGITAL_VOLUME
        u8 call_volume_level_max = ARRAY_SIZE(phone_call_dig_vol_tab) - 1;
        app_var.call_volume = (app_var.call_volume > call_volume_level_max) ? call_volume_level_max : app_var.call_volume;
        __this->digital_volume = phone_call_dig_vol_tab[app_var.call_volume];
        printf("call_volume:%d,digital_volume:%d", app_var.call_volume, __this->digital_volume);
        dac_digital_vol_open();
        dac_digital_vol_tab_register(phone_call_dig_vol_tab, ARRAY_SIZE(phone_call_dig_vol_tab));
        /*调数字音量的时候，模拟音量定最大*/
        audio_dac_set_analog_vol(&dac_hdl, max_volume);
#elif (SYS_VOL_TYPE == VOL_TYPE_AD)
        /*通话联合音量调节的时候，最大音量为15，和手机的等级一致*/
        __this->max_volume[state] = 15;
#endif/*TCFG_CALL_USE_DIGITAL_VOLUME*/
    }
#endif/*SYS_VOL_TYPE == VOL_TYPE_DIGITAL*/
}



void app_audio_state_exit(u8 state)
{
#if TCFG_CALL_USE_DIGITAL_VOLUME
    if (__this->state == APP_AUDIO_STATE_CALL) {
        dac_digital_vol_close();
    }
#endif
    if (state == __this->state) {
        __this->state = __this->prev_state;
        __this->prev_state = APP_AUDIO_STATE_IDLE;
    } else if (state == __this->prev_state) {
        __this->prev_state = APP_AUDIO_STATE_IDLE;
    }
}

u8 app_audio_get_state(void)
{
    return __this->state;
}

s16 app_audio_get_max_volume(void)
{
    if (__this->state == APP_AUDIO_STATE_IDLE) {
        return get_max_sys_vol();
    }
#if TCFG_CALL_USE_DIGITAL_VOLUME
    if (__this->state == APP_AUDIO_STATE_CALL) {
        return (ARRAY_SIZE(phone_call_dig_vol_tab) - 1);
    }
#endif/*TCFG_CALL_USE_DIGITAL_VOLUME*/
    return __this->max_volume[__this->state];
}

void app_audio_set_mix_volume(u8 front_volume, u8 back_volume)
{
    /*set_audio_device_volume(AUDIO_MIX_FRONT_VOL, front_volume);
    set_audio_device_volume(AUDIO_MIX_BACK_VOL, back_volume);*/
}
#if 0

void audio_vol_test()
{
    app_set_sys_vol(10, 10);
    log_info("sys vol %d %d\n", get_audio_device_volume(AUDIO_SYS_VOL) >> 16, get_audio_device_volume(AUDIO_SYS_VOL) & 0xffff);
    log_info("ana vol %d %d\n", get_audio_device_volume(AUDIO_ANA_VOL) >> 16, get_audio_device_volume(AUDIO_ANA_VOL) & 0xffff);
    log_info("dig vol %d %d\n", get_audio_device_volume(AUDIO_DIG_VOL) >> 16, get_audio_device_volume(AUDIO_DIG_VOL) & 0xffff);
    log_info("max vol %d %d\n", get_audio_device_volume(AUDIO_MAX_VOL) >> 16, get_audio_device_volume(AUDIO_MAX_VOL) & 0xffff);

    app_set_max_vol(30);
    app_set_ana_vol(25, 24);
    app_set_dig_vol(90, 90);

    log_info("sys vol %d %d\n", get_audio_device_volume(AUDIO_SYS_VOL) >> 16, get_audio_device_volume(AUDIO_SYS_VOL) & 0xffff);
    log_info("ana vol %d %d\n", get_audio_device_volume(AUDIO_ANA_VOL) >> 16, get_audio_device_volume(AUDIO_ANA_VOL) & 0xffff);
    log_info("dig vol %d %d\n", get_audio_device_volume(AUDIO_DIG_VOL) >> 16, get_audio_device_volume(AUDIO_DIG_VOL) & 0xffff);
    log_info("max vol %d %d\n", get_audio_device_volume(AUDIO_MAX_VOL) >> 16, get_audio_device_volume(AUDIO_MAX_VOL) & 0xffff);
}
#endif


void dac_power_on(void)
{
    log_info(">>>dac_power_on:%d", __this->ref.counter);
    if (atomic_inc_return(&__this->ref) == 1) {
        audio_dac_open(&dac_hdl);
    }
}

void dac_power_off(void)
{
    /*log_info(">>>dac_power_off:%d", __this->ref.counter);*/
    if (atomic_read(&__this->ref) != 0 && atomic_dec_return(&__this->ref)) {
        return;
    }
#if 0
    app_audio_mute(AUDIO_MUTE_DEFAULT);
    if (dac_hdl.vol_l || dac_hdl.vol_r) {
        u8 fade_time = dac_hdl.vol_l * 2 / 10 + 1;
        os_time_dly(fade_time); //br30进入低功耗无法使用os_time_dly
        printf("fade_time:%d ms", fade_time);
    }
#endif
    audio_dac_close(&dac_hdl);
}

/*
 *dac快速校准
 */
//#define DAC_TRIM_FAST_EN
#ifdef DAC_TRIM_FAST_EN
u8 dac_trim_fast_en()
{
    return 1;
}
#endif/*DAC_TRIM_FAST_EN*/

/*
 *自定义dac上电延时时间，具体延时多久应通过示波器测量
 */
#if 1
void dac_power_on_delay()
{
    os_time_dly(50);
}
#endif

void audio_gain_dump()
{
    u8 dac_again_l = JL_ADDA->DAA_CON1 & 0xF;
    u8 dac_again_r = (JL_ADDA->DAA_CON1 >> 4) & 0xF;
    u32 dac_dgain_l = JL_AUDIO->DAC_VL0 & 0xFFFF;
    u32 dac_dgain_r = (JL_AUDIO->DAC_VL0 >> 16) & 0xFFFF;
    u8 mic0_gain = JL_ADDA->ADA_CON0 & 0x1F;
    u8 mic1_gain = (JL_ADDA->ADA_CON0 >> 5) & 0x1F;
    short anc_gain = JL_ANC->CON5 & 0xFFFF;
    printf("MIC_G:%d,%d,DAC_AG:%d,%d,DAC_DG:%d,%d,ANC_G:%d\n", mic0_gain, mic1_gain, dac_again_l, dac_again_r, dac_dgain_l, dac_dgain_r, anc_gain);

}



