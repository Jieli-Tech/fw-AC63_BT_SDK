/*
*********************************************************************
*                  		Audio AEC APIs

* Description:AEC用户调用接口
* Note(s)    :ANS等级和AEC滤波器长度可根据实际需要进行配置
*			  (1)CONST_ANS_MODE:ANS降噪等级配置
*			  (2)AEC_TAIL_LENGTH:AEC滤波器长度配置
*********************************************************************
*/
#include "system/includes.h"
#include "app_config.h"
#include "audio_config.h"
#include "aec_user.h"
#include "media/includes.h"
#include "application/audio_eq_drc_apply.h"
#include "circular_buf.h"
#include "clock_cfg.h"

#define LOG_TAG_CONST       AEC_USER
#define LOG_TAG             "[AEC_USER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define AEC_MALLOC_ENABLE	1

/*AEC_TOGGLE:AEC模块使能开关，Disable则数据完全不经过处理，AEC模块不占用资源*/
#if (TCFG_AEC_ENABLE)
#define AEC_TOGGLE			1
#else
#define AEC_TOGGLE			0
#endif/*TCFG_AEC_ENABLE*/

#if (TCFG_EQ_ENABLE == 1)
#define AEC_DCCS_EN			1 /*mic去直流滤波eq*/
#define AEC_UL_EQ_EN		1 /*mic 普通eq*/
#else
#define AEC_DCCS_EN			0
#define AEC_UL_EQ_EN		0
#endif/*TCFG_EQ_ENABLE*/

#ifdef CONFIG_FPGA_ENABLE
const u8 CONST_AEC_ENABLE = 0;
#else
const u8 CONST_AEC_ENABLE = 1;
#endif/*CONFIG_FPGA_ENABLE*/

#ifdef AUDIO_PCM_DEBUG
/*AEC串口数据导出*/
const u8 CONST_AEC_EXPORT = 1;
#else
const u8 CONST_AEC_EXPORT = 0;
#endif/*AUDIO_PCM_DEBUG*/

/*ANS等级:0 or 1,等级1比等级0多6k左右的ram*/
const u8 CONST_ANS_MODE = 1;

/*参考数据变采样处理*/
const u8 CONST_REF_SRC = 0;

/*
 *ANS版本配置
 *ANS_V100:传统降噪
 **/
const u8 CONST_ANS_VERSION = ANS_V100;

/*
 *延时估计使能
 *点烟器/单工模式需要做延时估计
 *其他的暂时不需要做
 */
#if ((AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_FM) || TCFG_AEC_SIMPLEX)
const u8 CONST_AEC_DLY_EST = 1;
#else
const u8 CONST_AEC_DLY_EST = 0;
#endif

/*
 *AEC复杂等级，等级越高，ram和mips越大，适应性越好
 *回音路径不定/回音失真等情况才需要比较高的等级
 *音箱建议使用等级:5
 *耳机建议使用等级:2
 */
#define AEC_TAIL_LENGTH			5 /*range:2~10,default:5*/

//////////////////Simplex Parameters(单工调试参数)/////////////////////
#if TCFG_AEC_SIMPLEX
const u8 CONST_AEC_SIMPLEX = 1;
#else
const u8 CONST_AEC_SIMPLEX = 0;
#endif/*TCFG_AEC_SIMPLEX*/
/*单工连续清0的帧数*/
#define AEC_SIMPLEX_TAIL 		15
/**远端数据大于CONST_AEC_SIMPLEX_THR,即清零近端数据
 *越小，回音限制得越好，同时也就越容易卡*/
#define AEC_SIMPLEX_THR			100000	/*default:260000*/
///////////////////////////////////////////////////////////////////////

#define AEC_OUT_DUMP_PACKET		15	/*数据输出开头丢掉的数据包数*/
#define AEC_IN_DUMP_PACKET		1	/*数据输出开头丢掉的数据包数*/

/*AEC默认使能模块:AEC_MODE_REDUCE or AEC_MODE_ADVANCE */
#define AEC_MODULE_BIT			AEC_MODE_ADVANCE

extern struct adc_platform_data adc_data;

__attribute__((weak))u32 usb_mic_is_running()
{
    return 0;
}



extern void mem_state_dump();

/*复用lmp rx buf(一般通话的时候复用)
 *rx_buf概率产生碎片，导致alloc失败，因此默认配0
 */
#define MALLOC_MULTIPLEX_EN		0
extern void *lmp_malloc(int);
extern void lmp_free(void *);
void *zalloc_mux(int size)
{
#if MALLOC_MULTIPLEX_EN
    void bredr_rx_bulk_state();
    bredr_rx_bulk_state();
    void *p = NULL;
    do {
        p = lmp_malloc(size);
        if (p) {
            break;
        }
        printf("aec_malloc wait...\n");
        os_time_dly(2);
    } while (1);
    if (p) {
        memset(p, 0, size);
    }
    printf("[malloc_mux]p = 0x%x,size = %d\n", p, size);
    return p;
#else
    return zalloc(size);
#endif
}

void free_mux(void *p)
{
#if MALLOC_MULTIPLEX_EN
    printf("[free_mux]p = 0x%x\n", p);
    lmp_free(p);
#else
    free(p);
#endif
}

void aec_param_dump(struct aec_s_attr *param)
{
    log_info("===========dump aec param==================\n");
    log_info("toggle:%d\n", param->toggle);
    log_info("EnableBit:%x\n", param->EnableBit);
    log_info("ul_eq_en:%x\n", param->ul_eq_en);
    //log_info("AGC_fade:%d\n", (int)(param->AGC_gain_step * 100));
    log_info("AGC_NDT_max_gain:%d\n", (int)(param->AGC_NDT_max_gain * 100));
    log_info("AGC_NDT_min_gain:%d\n", (int)(param->AGC_NDT_min_gain * 100));
    log_info("AGC_NDT_speech_thr:%d\n", (int)(param->AGC_NDT_speech_thr * 100));
    log_info("AGC_DT_max_gain:%d\n", (int)(param->AGC_DT_max_gain * 100));
    log_info("AGC_DT_min_gain:%d\n", (int)(param->AGC_DT_min_gain * 100));
    log_info("AGC_DT_speech_thr:%d\n", (int)(param->AGC_DT_speech_thr * 100));
    log_info("AGC_echo_present_thr:%d\n", (int)(param->AGC_echo_present_thr * 100));
    log_info("AEC_DT_AggressiveFactor:%d\n", (int)(param->AEC_DT_AggressiveFactor * 100));
    log_info("AEC_RefEngThr:%d\n", (int)(param->AEC_RefEngThr * 100));
    log_info("ES_AggressFactor:%d\n", (int)(param->ES_AggressFactor * 100));
    log_info("ES_MinSuppress:%d\n", (int)(param->ES_MinSuppress * 100));
    log_info("ANS_AggressFactor:%d\n", (int)(param->ANS_AggressFactor * 100));
    log_info("ANS_MinSuppress:%d\n", (int)(param->ANS_MinSuppress * 100));
    log_info("=================END=======================\n");
}

struct audio_aec_hdl {
    u8 start;				//aec模块状态
    u8 inbuf_clear_cnt;		//aec输入数据丢掉
    u8 output_fade_in;		//aec输出淡入使能
    u8 output_fade_in_gain;	//aec输出淡入增益
#if AEC_UL_EQ_EN
    struct audio_eq *ul_eq;	//上行数据eq处理
#endif/*AEC_UL_EQ_EN*/
#if AEC_DCCS_EN
    struct audio_eq *dccs_eq;//省电容mic去直流滤波
#endif/*AEC_DCCS_EN*/
    u16 dump_packet;		//前面如果有杂音，丢掉几包
    u8 output_buf[1000];	//aec数据输出缓存
    cbuffer_t output_cbuf;
    struct aec_s_attr attr;	//aec模块参数属性
};
#if AEC_MALLOC_ENABLE
struct audio_aec_hdl *aec_hdl = NULL;
#else
struct audio_aec_hdl aec_handle;
#endif

extern int esco_adc_mic_en();
void audio_aec_ref_start(u8 en)
{
    if (aec_hdl && (aec_hdl->attr.fm_tx_start == 0)) {
        if (esco_adc_mic_en() == 0) {
            aec_hdl->attr.fm_tx_start = en;
            y_printf("fm_tx_start:%d\n", en);
        }
    }
}

#if AEC_DCCS_EN
const int DCCS_8k_Coeff[5] = {
    (943718 << 2),	-(856687 << 2),	(1048576 << 2),	(1887437 << 2),	-(2097152 << 2)
};
const int DCCS_16k_Coeff[5] = {
    (1006633 << 2),	-(967542 << 2),	(1048576 << 2),	(2013266 << 2),	-(2097152 << 2)
};
int aec_dccs_eq_filter(void *eq, int sr, struct audio_eq_filter_info *info)
{
    //r_printf("dccs_eq sr:%d\n", sr);
    if (sr == 16000) {
        info->L_coeff = (void *)DCCS_16k_Coeff;
        info->R_coeff = (void *)DCCS_16k_Coeff;
    } else {
        info->L_coeff = (void *)DCCS_8k_Coeff;
        info->R_coeff = (void *)DCCS_8k_Coeff;
    }
    info->L_gain = 0;
    info->R_gain = 0;
    info->nsection = 1;
    return 0;
}

static int dccs_eq_output(void *priv, void *data, u32 len)
{
    return 0;
}
#endif/*AEC_DCCS_EN*/

#if AEC_UL_EQ_EN

static int ul_eq_output(void *priv, void *data, u32 len)
{
    return 0;
}
#endif/*AEC_UL_EQ_EN*/

/*
*********************************************************************
*                  Audio AEC Process_Probe
* Description: AEC模块数据前处理回调
* Arguments  : data 数据地址
*			   len	数据长度
* Return	 : 0 成功 其他 失败
* Note(s)    : 在源数据经过AEC模块前，可以增加自定义处理
*********************************************************************
*/
static int audio_aec_probe(s16 *data, u16 len)
{
#if AEC_DCCS_EN
    if (aec_hdl->dccs_eq) {
        audio_eq_run(aec_hdl->dccs_eq, data, len);
    }
#endif/*AEC_DCCS_EN*/
    return 0;
}

/*
*********************************************************************
*                  Audio AEC Process_Post
* Description: AEC模块数据后处理回调
* Arguments  : data 数据地址
*			   len	数据长度
* Return	 : 0 成功 其他 失败
* Note(s)    : 在数据处理完毕，可以增加自定义后处理
*********************************************************************
*/
static int audio_aec_post(s16 *data, u16 len)
{
#if AEC_UL_EQ_EN
    if (aec_hdl->ul_eq) {
        audio_eq_run(aec_hdl->ul_eq, data, len);
    }
#endif/*AEC_UL_EQ_EN*/
    return 0;
}

/*
*********************************************************************
*                  Audio AEC Output Handle
* Description: AEC模块数据输出回调
* Arguments  : data 输出数据地址
*			   len	输出数据长度
* Return	 : 数据输出消耗长度
* Note(s)    : None.
*********************************************************************
*/
extern void esco_enc_resume(void);
static int audio_aec_output(s16 *data, u16 len)
{
    u16 outlen = len;
    s16 *outdat = data;

    /*数据清0处理*/
    if (aec_hdl->dump_packet) {
        aec_hdl->dump_packet--;
        memset(data, 0, len);
    } else  {
        /*数据淡入处理*/
        if (aec_hdl->output_fade_in) {
            s32 tmp_data;
            //printf("fade:%d\n",aec_hdl->output_fade_in_gain);
            for (int i = 0; i < len / 2; i++) {
                tmp_data = data[i];
                data[i] = tmp_data * aec_hdl->output_fade_in_gain >> 7;
            }
            aec_hdl->output_fade_in_gain += 12;
            if (aec_hdl->output_fade_in_gain >= 128) {
                aec_hdl->output_fade_in = 0;
            }
        }
    }

    u16 wlen = cbuf_write(&aec_hdl->output_cbuf, outdat, outlen);
    /* printf("wlen:%d-%d-%d-%d\n",wlen, outlen, len,aec_hdl->output_cbuf.data_len); */

    esco_enc_resume();
#if 1
    static u32 aec_output_max = 0;
    if (aec_output_max < aec_hdl->output_cbuf.data_len) {
        aec_output_max = aec_hdl->output_cbuf.data_len;
        y_printf("o_max:%d", aec_output_max);
    }
#endif
    if (wlen != outlen) {
        putchar('F');
    }
    return len;
}

/*
*********************************************************************
*                  Audio AEC Output Query
* Description: 查询aec模块的输出数据缓存大小
* Arguments  : None.
* Return	 : 数据缓存大小
* Note(s)    : None.
*********************************************************************
*/
int audio_aec_output_data_size(void)
{
    local_irq_disable();
    if (!aec_hdl || !aec_hdl->start) {
        printf("audio_aec close now");
        local_irq_enable();
        return -EINVAL;
    }
    int len = cbuf_get_data_size(&aec_hdl->output_cbuf);
    local_irq_enable();
    return len;
}

/*
*********************************************************************
*                  Audio AEC Output Read
* Description: 读取aec模块的输出数据
* Arguments  : buf  读取数据存放地址
*			   len	读取数据长度
* Return	 : 数据读取长度
* Note(s)    : None.
*********************************************************************
*/
int audio_aec_output_read(s16 *buf, u16 len)
{
    /* printf("rlen:%d-%d\n",len,aec_hdl->output_cbuf.data_len); */
    local_irq_disable();
    if (!aec_hdl || !aec_hdl->start) {
        printf("audio_aec close now");
        local_irq_enable();
        return -EINVAL;
    }
    u16 rlen = cbuf_read(&aec_hdl->output_cbuf, buf, len);
    if (rlen == 0) {
        //putchar('N');
    }
    local_irq_enable();
    return rlen;
}

/*
*********************************************************************
*                  Audio AEC Parameters
* Description: AEC模块配置参数
* Arguments  : p	参数指针
* Return	 : None.
* Note(s)    : 读取配置文件成功，则使用配置文件的参数配置，否则使用默
*			   认参数配置
*********************************************************************
*/
static void audio_aec_param_read_config(struct aec_s_attr *p)
{
    AEC_CONFIG cfg;
    int ret = syscfg_read(CFG_AEC_ID, &cfg, sizeof(AEC_CONFIG));
    if (ret == sizeof(AEC_CONFIG)) {
        log_info("audio_aec read config ok\n");
        p->AGC_NDT_fade_in_step = cfg.ndt_fade_in;
        p->AGC_NDT_fade_out_step = cfg.ndt_fade_out;
        p->AGC_DT_fade_in_step = cfg.dt_fade_in;
        p->AGC_DT_fade_out_step = cfg.dt_fade_out;
        p->AGC_NDT_max_gain = cfg.ndt_max_gain;
        p->AGC_NDT_min_gain = cfg.ndt_min_gain;
        p->AGC_NDT_speech_thr = cfg.ndt_speech_thr;
        p->AGC_DT_max_gain = cfg.dt_max_gain;
        p->AGC_DT_min_gain = cfg.dt_min_gain;
        p->AGC_DT_speech_thr = cfg.dt_speech_thr;
        p->AGC_echo_present_thr = cfg.echo_present_thr;
        p->AEC_DT_AggressiveFactor = cfg.aec_dt_aggress;
        p->AEC_RefEngThr = cfg.aec_refengthr;
        p->ES_AggressFactor = cfg.es_aggress_factor;
        p->ES_MinSuppress = cfg.es_min_suppress;
        p->ES_Unconverge_OverDrive = cfg.es_min_suppress;
        p->ANS_AggressFactor = cfg.ans_aggress;
        p->ANS_MinSuppress = cfg.ans_suppress;

        if (cfg.aec_mode == 0) {
            p->toggle = 0;
            p->EnableBit = 0;
        } else if (cfg.aec_mode == 1) {
            p->toggle = 1;
            p->EnableBit = AEC_MODE_REDUCE;
        } else if (cfg.aec_mode == 2) {
            p->toggle = 1;
            p->EnableBit = AEC_MODE_ADVANCE;
        }
        p->ul_eq_en = cfg.ul_eq_en;
        //aec_param_dump(p);
    } else {
        log_error("read audio_aec param err:%x", ret);
    }
}

/*
*********************************************************************
*                  Audio AEC Open
* Description: 初始化AEC模块
* Arguments  : sr 			采样率(8000/16000)
*			   enablebit	使能模块(AEC/NLP/AGC/ANS...)
*			   out_hdl		自定义回调函数，NULL则用默认的回调
* Return	 : 0 成功 其他 失败
* Note(s)    : 该接口是对audio_aec_init的扩展，支持自定义使能模块以及
*			   数据输出回调函数
*********************************************************************
*/
int audio_aec_open(u16 sample_rate, s16 enablebit, int (*out_hdl)(s16 *data, u16 len))
{
    struct aec_s_attr *aec_param;
    printf("audio_aec_open,sr = %d\n", sample_rate);
#if AEC_MALLOC_ENABLE
    aec_hdl = zalloc(sizeof(struct audio_aec_hdl));
    if (aec_hdl == NULL) {
        log_error("aec_hdl malloc failed");
        return -ENOMEM;
    }
#else
    aec_hdl = &aec_handle;
#endif/*AEC_MALLOC_ENABLE*/
    cbuf_init(&aec_hdl->output_cbuf, aec_hdl->output_buf, sizeof(aec_hdl->output_buf));
    aec_hdl->start = 1;
    aec_hdl->dump_packet = AEC_OUT_DUMP_PACKET;
    aec_hdl->inbuf_clear_cnt = AEC_IN_DUMP_PACKET;
    aec_hdl->output_fade_in = 1;
    aec_hdl->output_fade_in_gain = 0;
    aec_param = &aec_hdl->attr;

#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_DAC)
    aec_param->output_way = 0;
#elif (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_FM)
    aec_param->output_way = 1;
#endif

    aec_param->toggle = 1;
    aec_param->EnableBit = AEC_MODULE_BIT;
    aec_param->agc_en = 1;
    aec_param->wideband = 0;
    aec_param->ul_eq_en = 1;
    aec_param->packet_dump = 50;/*0~255(u8)*/

    aec_param->AGC_NDT_fade_in_step = 1.3f;
    aec_param->AGC_NDT_fade_out_step = 0.9f;
    aec_param->AGC_DT_fade_in_step = 1.3f;
    aec_param->AGC_DT_fade_out_step = 0.9f;
    aec_param->AGC_NDT_max_gain = 12.f;
    aec_param->AGC_NDT_min_gain = 0.f;
    aec_param->AGC_NDT_speech_thr = -50.f;
    aec_param->AGC_DT_max_gain = 12.f;
    aec_param->AGC_DT_min_gain = 0.f;
    aec_param->AGC_DT_speech_thr = -40.f;
    aec_param->AGC_echo_look_ahead = 100;
    aec_param->AGC_echo_present_thr = -70.f;
    aec_param->AGC_echo_hold = 400;

    /*AEC*/
    aec_param->AEC_DT_AggressiveFactor = 1.f;	/*范围：1~5，越大追踪越好，但会不稳定,如破音*/
    aec_param->AEC_RefEngThr = -70.f;

    /*ES*/
    aec_param->ES_AggressFactor = -3.0f;
    aec_param->ES_MinSuppress = 4.f;
    aec_param->ES_Unconverge_OverDrive = aec_param->ES_MinSuppress;

    /*ANS*/
    aec_param->ANS_mode = CONST_ANS_MODE;
    aec_param->ANS_AggressFactor = 1.25f;	/*范围：1~2,动态调整,越大越强(1.25f)*/
    aec_param->ANS_MinSuppress = 0.04f;	/*范围：0~1,静态定死最小调整,越小越强(0.09f)*/
    aec_param->ANS_NoiseLevel = 2.2e4f;

#if TCFG_AEC_SIMPLEX
    aec_param->wn_en = 1;
#else
    aec_param->wn_en = 0;
#endif/*TCFG_AEC_SIMPLEX*/

    aec_param->aec_tail_length = AEC_TAIL_LENGTH;
    aec_param->wn_gain = 331;
    aec_param->SimplexTail = AEC_SIMPLEX_TAIL;
    aec_param->SimplexThr = AEC_SIMPLEX_THR;
#if ((AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_FM) || TCFG_AEC_SIMPLEX)
    aec_param->dly_est = 1;
#else
    aec_param->dly_est = 0;
#endif/*DLY_EST config*/

    aec_param->dst_delay = 50;
    aec_param->aec_probe = audio_aec_probe;
    aec_param->aec_post = audio_aec_post;
    aec_param->output_handle = audio_aec_output;
    aec_param->ref_sr  = usb_mic_is_running();
    if (aec_param->ref_sr == 0) {
        aec_param->ref_sr = sample_rate;
    }


    audio_aec_param_read_config(aec_param);
    if (enablebit >= 0) {
        aec_param->EnableBit = enablebit;
    }
    if (out_hdl) {
        aec_param->output_handle = out_hdl;
    }

    if (sample_rate == 16000) {
        aec_param->wideband = 1;
        aec_param->hw_delay_offset = 50;
        if (aec_param->EnableBit == AEC_MODE_ADVANCE) {
            clock_add(AEC16K_ADV_CLK);
        } else {
            clock_add(AEC16K_CLK);
        }
    } else {
        aec_param->wideband = 0;
        aec_param->hw_delay_offset = 75;
        if (aec_param->EnableBit == AEC_MODE_ADVANCE) {
            clock_add(AEC8K_ADV_CLK);
        } else {
            clock_add(AEC8K_CLK);
        }
    }
    clock_set_cur();
#if TCFG_AEC_SIMPLEX
    aec_param->EnableBit = AEC_MODE_SIMPLEX;
    if (sample_rate == 8000) {
        aec_param->SimplexTail = aec_param->SimplexTail / 2;
    }
#endif/*TCFG_AEC_SIMPLEX*/


#if AEC_UL_EQ_EN
    if (aec_param->ul_eq_en) {
        /* memset(&aec_hdl->ul_eq, 0, sizeof(struct audio_eq)); */
        /* memset(&aec_hdl->ul_eq_ch, 0, sizeof(struct hw_eq_ch)); */
        /* aec_hdl->ul_eq.eq_ch = &aec_hdl->ul_eq_ch; */
        struct audio_eq_param ul_eq_param = {0};
        ul_eq_param.sr = sample_rate;
        ul_eq_param.channels = 1;
        ul_eq_param.online_en = 1;
        ul_eq_param.mode_en = 0;
        ul_eq_param.remain_en = 0;
        ul_eq_param.max_nsection = EQ_SECTION_MAX;
        ul_eq_param.cb = aec_ul_eq_filter;
        ul_eq_param.eq_name = aec_eq_mode;
        aec_hdl->ul_eq = audio_dec_eq_open(&ul_eq_param);
        /* audio_eq_open(&aec_hdl->ul_eq, &ul_eq_param); */
        /* audio_eq_set_samplerate(&aec_hdl->ul_eq, sample_rate); */
        /* audio_eq_set_output_handle(&aec_hdl->ul_eq, ul_eq_output, NULL); */
        /* audio_eq_start(&aec_hdl->ul_eq); */
    }
#endif/*AEC_UL_EQ_EN*/

#if AEC_DCCS_EN
    if (adc_data.mic_capless) {
        /* memset(&aec_hdl->dccs_eq, 0, sizeof(struct audio_eq)); */
        /* memset(&aec_hdl->dccs_eq_ch, 0, sizeof(struct hw_eq_ch)); */

        /* .eq_ch = &aec_hdl->dccs_eq_ch; */
        struct audio_eq_param dccs_eq_param = {0};
        dccs_eq_param.sr = sample_rate;
        dccs_eq_param.channels = 1;
        dccs_eq_param.online_en = 0;
        dccs_eq_param.mode_en = 0;
        dccs_eq_param.remain_en = 0;
        dccs_eq_param.max_nsection = EQ_SECTION_MAX;
        dccs_eq_param.cb = aec_dccs_eq_filter;
        aec_hdl->dccs_eq = audio_dec_eq_open(&dccs_eq_param);
        /* audio_eq_open(&aec_hdl->dccs_eq, &dccs_eq_param); */
        /* audio_eq_set_samplerate(&aec_hdl->dccs_eq, sample_rate); */
        /* audio_eq_set_output_handle(&aec_hdl->dccs_eq, dccs_eq_output, NULL); */
        /* audio_eq_start(&aec_hdl->dccs_eq); */
    }
#endif/*AEC_DCCS_EN*/

    //aec_param->toggle = 1;
    //aec_param->EnableBit = AEC_MODULE_BIT;
    //aec_param_dump(aec_param);

#if AEC_TOGGLE
    aec_open(aec_param);
#endif
    return 0;
}

/*
*********************************************************************
*                  Audio AEC Init
* Description: 初始化AEC模块
* Arguments  : sr 采样率(8000/16000)
* Return	 : 0 成功 其他 失败
* Note(s)    : None.
*********************************************************************
*/
int audio_aec_init(u16 sample_rate)
{
    return audio_aec_open(sample_rate, -1, NULL);
}

/*
*********************************************************************
*                  Audio AEC Close
* Description: 关闭AEC模块
* Arguments  : None.
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_aec_close(void)
{
    printf("audio_aec_close:%x", (u32)aec_hdl);
    struct aec_s_attr *aec_param;
    if (aec_hdl) {
        aec_hdl->start = 0;

        aec_param = &aec_hdl->attr;
        if (aec_param->wideband) {
            if (aec_param->EnableBit == AEC_MODE_ADVANCE) {
                clock_remove(AEC16K_ADV_CLK);
            } else {
                clock_remove(AEC16K_CLK);
            }
        } else {
            if (aec_param->EnableBit == AEC_MODE_ADVANCE) {
                clock_remove(AEC8K_ADV_CLK);
            } else {
                clock_remove(AEC8K_CLK);
            }
        }
        clock_set_cur();

#if AEC_TOGGLE
        aec_close();
#endif/*AEC_TOGGLE*/
#if AEC_DCCS_EN
        if (aec_hdl->dccs_eq) {
            audio_dec_eq_close(aec_hdl->dccs_eq);
        }
#endif/*AEC_DCCS_EN*/
#if AEC_UL_EQ_EN
        if (aec_hdl->ul_eq) {
            audio_dec_eq_close(aec_hdl->ul_eq);
        }
#endif/*AEC_UL_EQ_EN*/
        local_irq_disable();
#if AEC_MALLOC_ENABLE
        free(aec_hdl);
#endif/*AEC_MALLOC_ENABLE*/
        aec_hdl = NULL;
        local_irq_enable();
    }
    printf("audio_aec_close ok\n");
}

/*
*********************************************************************
*                  Audio AEC Input
* Description: AEC源数据输入
* Arguments  : buf	输入源数据地址
*			   len	输入源数据长度
* Return	 : None.
* Note(s)    : 输入一帧数据，唤醒一次运行任务处理数据，默认帧长256点
*********************************************************************
*/
void audio_aec_inbuf(s16 *buf, u16 len)
{
    if (aec_hdl && aec_hdl->start) {
#if AEC_TOGGLE
        if (aec_hdl->inbuf_clear_cnt) {
            aec_hdl->inbuf_clear_cnt--;
            memset(buf, 0, len);
        }
        int ret = aec_in_data((u8 *)buf, len);
        if (ret == -1) {
#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_DAC)
            /* log_info("fill dac data\n");
            u8 tmp_buf[64] = {0};
            for (u8 i = 0; i < 512 / sizeof(tmp_buf); i++) {
                app_audio_output_write(tmp_buf, sizeof(tmp_buf));
            } */
#endif
        } else if (ret == -2) {
            log_error("aec inbuf full\n");
        }
#else
        audio_aec_output(buf, len);
#endif/*AEC_TOGGLE*/
    }
}

/*
*********************************************************************
*                  Audio AEC Reference
* Description: AEC模块参考数据输入
* Arguments  : buf	输入参考数据地址
*			   len	输入参考数据长度
* Return	 : None.
* Note(s)    : 声卡设备是DAC，默认不用外部提供参考数据
*********************************************************************
*/
void audio_aec_refbuf(s16 *buf, u16 len)
{
    if (aec_hdl && aec_hdl->start) {
#if AEC_TOGGLE
        aec_ref_data((u8 *)buf, len);
#endif/*AEC_TOGGLE*/
    }
}

/* void aec_estimate_dump(int DlyEst)
{
	printf("DlyEst:%d\n",DlyEst);
} */
