/*
 ****************************************************************
 *							AUDIO SMS(SingleMic System)
 * File  : audio_aec_sms.c
 * By    :
 * Notes : AEC回音消除 + 单mic降噪
 *
 ****************************************************************
 */
#include "aec_user.h"
#include "system/includes.h"
#include "media/includes.h"
#include "application/audio_eq.h"
#include "circular_buf.h"
#include "overlay_code.h"
//#include "audio_aec_online.h"
//#include "audio_aec_debug.c"

#if (TCFG_AUDIO_DUAL_MIC_ENABLE == 0)
#include "commproc.h"


#ifdef SUPPORT_MS_EXTENSIONS
#pragma const_seg(  ".app_aec_const")
#pragma code_seg(   ".app_aec_code")
#endif


#define LOG_TAG_CONST       AEC_USER
#define LOG_TAG             "[AEC_USER]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

#define AEC_USER_MALLOC_ENABLE	1
#define AEC_TOGGLE				1
#if TCFG_EQ_ENABLE
/*mic去直流滤波eq*/
#define AEC_DCCS_EN				TCFG_AEC_DCCS_EQ_ENABLE
/*mic普通eq*/
#define AEC_UL_EQ_EN			TCFG_AEC_UL_EQ_ENABLE
#else
#define AEC_DCCS_EN			0
#define AEC_UL_EQ_EN		0
#endif

extern struct adc_platform_data adc_data;
extern struct audio_dac_hdl dac_hdl;

#ifdef CONFIG_FPGA_ENABLE
const u8 CONST_AEC_ENABLE = 1;
#else
#if CONFIG_LITE_AEC_ENABLE
const u8 CONST_AEC_ENABLE = 0;
const u8 CONST_ANS_MODE = 0;	/*ANS模式:0 or 1*/
#else
const u8 CONST_AEC_ENABLE = 1;
const u8 CONST_ANS_MODE = 1;	/*ANS模式:0 or 1*/
#endif/*CONFIG_LITE_AEC_ENABLE*/
#endif/*CONFIG_FPGA_ENABLE*/

/*ANS版本配置*/
const u8 CONST_ANS_VERSION = ANS_V100;

/*参考数据变采样处理*/
const u8 CONST_REF_SRC = 0;

#ifdef AUDIO_PCM_DEBUG
/*AEC串口数据导出*/
const u8 CONST_AEC_EXPORT = 1;
#else
const u8 CONST_AEC_EXPORT = 0;
#endif/*AUDIO_PCM_DEBUG*/

/*
 *延时估计使能
 *点烟器需要做延时估计
 *其他的暂时不需要做
 */
const u8 CONST_AEC_DLY_EST = 0;


/*Splittingfilter模式：0 or 1
 *mode = 0:运算量和ram小，高频会跌下来
 *mode = 1:运算量和ram大，频响正常（过认证，选择模式1）
 */
const u8 CONST_SPLIT_FILTER_MODE = 0;

#if TCFG_AEC_SIMPLEX
/*限幅器-噪声门限*/
const u8 CONST_AEC_SIMPLEX = 1;
#else
const u8 CONST_AEC_SIMPLEX = 0;
#endif/*TCFG_AEC_SIMPLEX*/

/*单工连续清0的帧数*/
#define AEC_SIMPLEX_TAIL 	15
/*
 *远端数据大于CONST_AEC_SIMPLEX_THR,即清零近端数据
 *越小，回音限制得越好，同时也就越容易卡
 */
#define AEC_SIMPLEX_THR		100000	/*default:260000*/

/*数据输出开头丢掉的数据包数*/
#define AEC_OUT_DUMP_PACKET		15
/*数据输出开头丢掉的数据包数*/
#define AEC_IN_DUMP_PACKET		1

/*
 *复用lmp rx buf(一般通话的时候复用)
 *rx_buf概率产生碎片，导致alloc失败，因此默认配0
 */
#define MALLOC_MULTIPLEX_EN		0
extern void *lmp_malloc(int);
extern void lmp_free(void *);
void *zalloc_mux(int size)
{
#if MALLOC_MULTIPLEX_EN
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

void aec_cfg_fill(AEC_CONFIG *cfg)
{
}

struct audio_aec_hdl {
    u8 start;
    u8 inbuf_clear_cnt;
    u8 output_fade_in;
    u8 output_fade_in_gain;
    u8 EnableBit;
    u16 dump_packet;/*前面如果有杂音，丢掉几包*/
    u8 output_buf[1000];
    cbuffer_t output_cbuf;
#if AEC_UL_EQ_EN
    struct audio_eq ul_eq;
    struct hw_eq_ch ul_eq_ch;
#endif/*AEC_UL_EQ_EN*/
#if AEC_DCCS_EN
    struct audio_eq dccs_eq;
    struct hw_eq_ch dccs_eq_ch;
#endif/*AEC_DCCS_EN*/
    struct aec_s_attr attr;
};
#if AEC_USER_MALLOC_ENABLE
struct audio_aec_hdl *aec_hdl = NULL;
#else
struct audio_aec_hdl aec_handle;
struct audio_aec_hdl *aec_hdl = &aec_handle;
#endif/*AEC_USER_MALLOC_ENABLE*/

#if AEC_DCCS_EN
//一段高通滤波器 可调中心截止频率、带宽
// 默认   /*freq:100*/   /*quality:0.7f*/
//
const struct eq_seg_info dccs_eq_tab_8k[] = {
    {0, EQ_IIR_TYPE_HIGH_PASS, 100,   0 << 20, (int)(0.7f * (1 << 24))},
};
const struct eq_seg_info dccs_eq_tab_16k[] = {
    {0, EQ_IIR_TYPE_HIGH_PASS, 100,   0 << 20, (int)(0.7f * (1 << 24))},
};
static int dccs_tab[5];
int aec_dccs_eq_filter(int sr, struct audio_eq_filter_info *info)
{
    if (!sr) {
        sr = 16000;
    }
    u8 nsection = ARRAY_SIZE(dccs_eq_tab_8k);
    local_irq_disable();
    if (sr == 16000) {
        for (int i = 0; i < nsection; i++) {
            eq_seg_design((struct eq_seg_info *)&dccs_eq_tab_16k[i], sr, &dccs_tab[5 * i]);
        }
    } else {
        for (int i = 0; i < nsection; i++) {
            eq_seg_design((struct eq_seg_info *)&dccs_eq_tab_8k[i], sr, &dccs_tab[5 * i]);
        }
    }
    local_irq_enable();

    info->L_coeff = (int *)dccs_tab;
    info->R_coeff = (int *)dccs_tab;
    info->L_gain = 0;
    info->R_gain = 0;
    info->nsection = nsection;
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


static int audio_aec_probe(s16 *data, u16 len)
{
#if AEC_DCCS_EN
    if (aec_hdl->dccs_eq.start) {
        audio_eq_run(&aec_hdl->dccs_eq, data, len);
    }
#endif/*AEC_DCCS_EN*/
    return 0;
}

static int audio_aec_post(s16 *data, u16 len)
{
#if AEC_UL_EQ_EN
    if (aec_hdl->ul_eq.start) {
        audio_eq_run(&aec_hdl->ul_eq, data, len);
    }
#endif/*AEC_UL_EQ_EN*/
    return 0;
}

static int audio_aec_update(u8 EnableBit)
{
    y_printf("aec_update:%d,%d", aec_hdl->attr.wideband, EnableBit);
    if (aec_hdl->attr.wideband) {
        if ((EnableBit & 0x7) == AEC_MODE_ADVANCE) {
            clk_set("sys", BT_CALL_16k_ADVANCE_HZ);
        } else {
            clk_set("sys", BT_CALL_16k_HZ);
        }
    } else {
        if ((EnableBit & 0x7) == AEC_MODE_ADVANCE) {
            clk_set("sys", BT_CALL_ADVANCE_HZ);
        } else {
            clk_set("sys", BT_CALL_HZ);
        }
    }
    return 0;
}

extern void esco_enc_resume(void);
#include "app_main.h"
static int audio_aec_output(s16 *data, u16 len)
{
#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
    //Voice Recognition get mic data here
    extern void kws_aec_data_output(void *priv, s16 * data, int len);
    kws_aec_data_output(NULL, data, len);

#endif/*TCFG_KWS_VOICE_RECOGNITION_ENABLE*/

    if (aec_hdl->dump_packet) {
        aec_hdl->dump_packet--;
        memset(data, 0, len);
    } else  {
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
    u16 wlen = cbuf_write(&aec_hdl->output_cbuf, data, len);
    //printf("wlen:%d-%d\n",len,aec_hdl.output_cbuf.data_len);
    esco_enc_resume();
#if 1
    static u32 aec_output_max = 0;
    if (aec_output_max < aec_hdl->output_cbuf.data_len) {
        aec_output_max = aec_hdl->output_cbuf.data_len;
        y_printf("o_max:%d", aec_output_max);
    }
#endif
    if (wlen != len) {
        putchar('f');
    }
    return wlen;
}

int audio_aec_output_read(s16 *buf, u16 len)
{
    //printf("rlen:%d-%d\n",len,aec_hdl.output_cbuf.data_len);
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


static void audio_aec_param_init(struct aec_s_attr *p)
{
    int ret = 0;
    AEC_CONFIG cfg;
#if TCFG_AEC_TOOL_ONLINE_ENABLE
    ret = aec_cfg_online_update_fill(&cfg, sizeof(AEC_CONFIG));
#endif/*TCFG_AEC_TOOL_ONLINE_ENABLE*/
    if (ret == 0) {
        ret = syscfg_read(CFG_AEC_ID, &cfg, sizeof(AEC_CONFIG));
    }
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
        p->toggle = 1;
        p->EnableBit = AEC_MODE_REDUCE;
        p->wideband = 0;
        p->ul_eq_en = 1;

        p->AGC_NDT_fade_in_step = 1.3f;
        p->AGC_NDT_fade_out_step = 0.9f;
        p->AGC_DT_fade_in_step = 1.3f;
        p->AGC_DT_fade_out_step = 0.9f;
        p->AGC_NDT_max_gain = 12.f;
        p->AGC_NDT_min_gain = 0.f;
        p->AGC_NDT_speech_thr = -50.f;
        p->AGC_DT_max_gain = 12.f;
        p->AGC_DT_min_gain = 0.f;
        p->AGC_DT_speech_thr = -40.f;
        p->AGC_echo_present_thr = -70.f;

        /*AEC*/
        p->AEC_DT_AggressiveFactor = 1.f;	/*范围：1~5，越大追踪越好，但会不稳定,如破音*/
        p->AEC_RefEngThr = -70.f;

        /*ES*/
        p->ES_AggressFactor = -3.0f;
        p->ES_MinSuppress = 4.f;
        p->ES_Unconverge_OverDrive = p->ES_MinSuppress;

        /*ANS*/
        p->ANS_AggressFactor = 1.25f;	/*范围：1~2,动态调整,越大越强(1.25f)*/
        p->ANS_MinSuppress = 0.04f;	/*范围：0~1,静态定死最小调整,越小越强(0.09f)*/
    }
    p->ANS_mode = 1;
    p->ANS_NoiseLevel = 2.2e4f;
    p->wn_gain = 331;
    p->SimplexTail = AEC_SIMPLEX_TAIL;
    p->SimplexThr = AEC_SIMPLEX_THR;
    p->dly_est = 0;
    p->dst_delay = 50;
    p->agc_en = 1;
    p->AGC_echo_look_ahead = 0;
    p->AGC_echo_hold = 0;
    p->packet_dump = 50;/*0~255(u8)*/
}

int audio_aec_init(u16 sample_rate)
{
    struct aec_s_attr *aec_param;
#if AEC_USER_MALLOC_ENABLE
    aec_hdl = zalloc(sizeof(struct audio_aec_hdl));
    if (aec_hdl == NULL) {
        log_error("aec_hdl malloc failed");
        return -ENOMEM;
    }
#endif/*AEC_USER_MALLOC_ENABLE*/

#if 0
    extern u32 aec_addr[], aec_begin[], aec_size[];
    memcpy(aec_addr, aec_begin, (u32)aec_size) ;
#else
    audio_overlay_load_code(OVERLAY_AEC);
#endif

    cbuf_init(&aec_hdl->output_cbuf, aec_hdl->output_buf, sizeof(aec_hdl->output_buf));
    aec_hdl->dump_packet = AEC_OUT_DUMP_PACKET;
    aec_hdl->inbuf_clear_cnt = AEC_IN_DUMP_PACKET;
    aec_hdl->output_fade_in = 1;
    aec_hdl->output_fade_in_gain = 0;
    aec_param = &aec_hdl->attr;
    aec_param->aec_probe = audio_aec_probe;
    aec_param->aec_post = audio_aec_post;
#if TCFG_AEC_TOOL_ONLINE_ENABLE
    aec_param->aec_update = audio_aec_update;
#endif/*TCFG_AEC_TOOL_ONLINE_ENABLE*/
    aec_param->output_handle = audio_aec_output;
    aec_param->far_noise_gate = 10;
    aec_param->ref_sr = sample_rate;

    audio_aec_param_init(aec_param);

#if TCFG_AEC_SIMPLEX
    aec_param->wn_en = 1;
    aec_param.EnableBit = AEC_MODE_SIMPLEX;
    if (sr == 8000) {
        aec_param.SimplexTail = aec_param.SimplexTail / 2;
    }
#else
    aec_param->wn_en = 0;
#endif/*TCFG_AEC_SIMPLEX*/

    if (sample_rate == 16000) {
        aec_param->wideband = 1;
        aec_param->hw_delay_offset = 60;
        if (aec_param->EnableBit == AEC_MODE_ADVANCE) {
            clk_set("sys", BT_CALL_16k_ADVANCE_HZ);
        } else {
            clk_set("sys", BT_CALL_16k_HZ);
        }
    } else {
        aec_param->wideband = 0;
        aec_param->hw_delay_offset = 55;
        if (aec_param->EnableBit == AEC_MODE_ADVANCE) {
            clk_set("sys", BT_CALL_ADVANCE_HZ);
        } else {
            clk_set("sys", BT_CALL_HZ);
        }
    }

#if AEC_UL_EQ_EN
    if (aec_param->ul_eq_en) {
        memset(&aec_hdl->ul_eq, 0, sizeof(struct audio_eq));
        memset(&aec_hdl->ul_eq_ch, 0, sizeof(struct hw_eq_ch));
        aec_hdl->ul_eq.eq_ch = &aec_hdl->ul_eq_ch;
        struct audio_eq_param ul_eq_param;
        ul_eq_param.channels = 1;
        ul_eq_param.online_en = 1;
        ul_eq_param.mode_en = 0;
        ul_eq_param.remain_en = 0;
        ul_eq_param.max_nsection = EQ_SECTION_MAX;
        ul_eq_param.cb = aec_ul_eq_filter;
        ul_eq_param.eq_name = AUDIO_AEC_EQ_NAME;
        audio_eq_open(&aec_hdl->ul_eq, &ul_eq_param);
        audio_eq_set_samplerate(&aec_hdl->ul_eq, sample_rate);
        audio_eq_set_output_handle(&aec_hdl->ul_eq, ul_eq_output, NULL);
        audio_eq_start(&aec_hdl->ul_eq);
    }
#endif

#if AEC_DCCS_EN
    if (adc_data.mic_capless) {
        memset(&aec_hdl->dccs_eq, 0, sizeof(struct audio_eq));
        memset(&aec_hdl->dccs_eq_ch, 0, sizeof(struct hw_eq_ch));
        aec_hdl->dccs_eq.eq_ch = &aec_hdl->dccs_eq_ch;
        struct audio_eq_param dccs_eq_param;
        dccs_eq_param.channels = 1;
        dccs_eq_param.online_en = 0;
        dccs_eq_param.mode_en = 0;
        dccs_eq_param.remain_en = 0;
        dccs_eq_param.max_nsection = EQ_SECTION_MAX;
        dccs_eq_param.cb = aec_dccs_eq_filter;
        audio_eq_open(&aec_hdl->dccs_eq, &dccs_eq_param);
        audio_eq_set_samplerate(&aec_hdl->dccs_eq, sample_rate);
        audio_eq_set_output_handle(&aec_hdl->dccs_eq, dccs_eq_output, NULL);
        audio_eq_start(&aec_hdl->dccs_eq);
    }
#endif

    //aec_param_dump(aec_param);
    aec_hdl->EnableBit = aec_param->EnableBit;
#if TCFG_KWS_VOICE_RECOGNITION_ENABLE
    extern u8 kws_get_state(void);
    if (kws_get_state()) {
        aec_param->EnableBit = AEC_EN;
        aec_param->agc_en = 0;
        printf("kws open,aec_enablebit=%x", aec_param->EnableBit);
        //临时关闭aec, 对比测试
        //aec_param->toggle = 0;
    }
#endif/*TCFG_KWS_VOICE_RECOGNITION_ENABLE*/

#if AEC_TOGGLE
    int ret = aec_open(aec_param);
    ASSERT(ret != -EPERM, "Chip not support aec mode!!");
#endif
    aec_hdl->start = 1;
    return 0;
}

void audio_aec_reboot(u8 reduce)
{
    if (aec_hdl) {
        printf("audio_aec_reboot:%x,%x,start:%d", aec_hdl->EnableBit, aec_hdl->attr.EnableBit, aec_hdl->start);
        if (aec_hdl->start) {
            if (reduce) {
                aec_hdl->attr.EnableBit = AEC_EN;
                aec_hdl->attr.agc_en = 0;
                aec_reboot(aec_hdl->attr.EnableBit);
            } else {
                if (aec_hdl->EnableBit != aec_hdl->attr.EnableBit) {
                    aec_hdl->attr.agc_en = 1;
                    aec_reboot(aec_hdl->EnableBit);
                }
            }
        }
    } else {
        printf("audio_aec close now\n");
    }
}

void audio_aec_close(void)
{
    printf("audio_aec_close:%x", (u32)aec_hdl);
    if (aec_hdl) {
        aec_hdl->start = 0;
#if AEC_TOGGLE
        aec_close();
#endif
#if AEC_DCCS_EN
        if (aec_hdl->dccs_eq.start) {
            audio_eq_close(&aec_hdl->dccs_eq);
        }
#endif/*AEC_DCCS_EN*/
#if AEC_UL_EQ_EN
        if (aec_hdl->ul_eq.start) {
            audio_eq_close(&aec_hdl->ul_eq);
        }
#endif/*AEC_UL_EQ_EN*/
        local_irq_disable();
#if AEC_USER_MALLOC_ENABLE
        free(aec_hdl);
#endif/*AEC_USER_MALLOC_ENABLE*/
        aec_hdl = NULL;
        local_irq_enable();
    }
}

u8 audio_aec_status(void)
{
    if (aec_hdl) {
        return aec_hdl->start;
    }
    return 0;
}

//pbg profile use it,don't delete
static u8 aec_input_clear_flag = 0;
void aec_input_clear_enable(u8 enable)
{
    aec_input_clear_flag = enable;
    log_info("aec_input_clear_enable= %d\n", enable);
}

void audio_aec_inbuf(s16 *buf, u16 len)
{
    if (aec_input_clear_flag) {
        memset(buf, 0, len);
    }

    if (aec_hdl && aec_hdl->start) {
#if AEC_TOGGLE
        if (aec_hdl->inbuf_clear_cnt) {
            aec_hdl->inbuf_clear_cnt--;
            memset(buf, 0, len);
        }
        int ret = aec_in_data(buf, len);
        if (ret == -1) {
        } else if (ret == -2) {
            log_error("aec inbuf full\n");
        }
#else
        audio_aec_output(buf, len);
#endif/*AEC_TOGGLE*/
    }
}

void audio_aec_inbuf_ref(s16 *buf, u16 len)
{
    if (aec_hdl && aec_hdl->start) {
        aec_in_data_ref(buf, len);
    }
}

void audio_aec_refbuf(s16 *buf, u16 len)
{
    if (aec_hdl && aec_hdl->start) {
#if AEC_TOGGLE
        aec_ref_data(buf, len);
#endif/*AEC_TOGGLE*/
    }
}

#endif/*TCFG_AUDIO_DUAL_MIC_ENABLE == 0*/

