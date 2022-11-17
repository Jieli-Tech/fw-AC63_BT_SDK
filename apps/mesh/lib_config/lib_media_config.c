/*********************************************************************************************
    *   Filename        : lib_driver_config.c

    *   Description     : Optimized Code & RAM (编译优化配置)

    *   Author          : Bingquan

    *   Email           : caibingquan@zh-jieli.com

    *   Last modifiled  : 2019-03-18 14:58

    *   Copyright:(c)JIELI  2011-2019  @ , All Rights Reserved.
*********************************************************************************************/
#include "app_config.h"
#include "system/includes.h"
#include "media/includes.h"


#if (TCFG_AUDIO_DECODER_OCCUPY_TRACE)
const u8 audio_decoder_occupy_trace_enable = 1;
const u8 audio_decoder_occupy_trace_dump = 0;
#else
const u8 audio_decoder_occupy_trace_enable = 0;
const u8 audio_decoder_occupy_trace_dump = 0;
#endif/*TCFG_AUDIO_DECODER_OCCUPY_TRACE*/

const int config_audio_eq_en = 0
#if TCFG_EQ_ENABLE
                               | EQ_EN         //eq使能
#if TCFG_EQ_ONLINE_ENABLE
                               | EQ_ONLINE_EN //在线调试使能
#endif/*TCFG_AUDIO_OUT_EQ_ENABLE*/
#if TCFG_USE_EQ_FILE
                               | EQ_FILE_EN   //使用eq_cfg_hw.bin文件效果
                               //|EQ_FILE_SWITCH_EN   //使能eq_cfg_hw.bin多文件切换，对应旧版config_audio_eq_file_sw_en
#endif/*TCFG_USE_EQ_FILE*/
#if TCFG_AUDIO_OUT_EQ_ENABLE
                               | EQ_HIGH_BASS_EN //高低音接口使能
                               | EQ_HIGH_BASS_FADE_EN  //高低音接口数据更新使用淡入淡出,配合config_audio_eq_fade_step步进使用
#endif/*TCFG_AUDIO_OUT_EQ_ENABLE*/
#if (RCSP_ADV_EN)&&(JL_EARPHONE_APP_EN)&&(TCFG_DRC_ENABLE == 0)
                               /* |EQ_FILTER_COEFF_LIMITER_ZERO_EN */
#endif
#ifndef CONFIG_SOUNDBOX_FLASH_256K
                               | EQ_HW_UPDATE_COEFF_ONLY_EN
                               | EQ_HW_LR_ALONE
#endif/* CONFIG_SOUNDBOX_FLASH_256K */
#if defined(EQ_CORE_V1) && TCFG_DRC_ENABLE
                               | EQ_SUPPORT_MULIT_CHANNEL_EN //eq是否支持多声道（3~8） 打开:支持  否则：仅支持1~2声道*/
                               | EQ_HW_CROSSOVER_TYPE0_EN //硬件分频器用序列进序列出
                               //|EQ_HW_CROSSOVER_TYPE1_EN  //硬件分频器使用块出方式，会增加mem(该方式仅支持单声道处理)
#endif/*(EQ_CORE_V1) && TCFG_DRC_ENABLE*/
#if defined(TCFG_EQ_DIVIDE_ENABLE) && TCFG_EQ_DIVIDE_ENABLE
                               | EQ_LR_DIVIDE_EN
#endif/*TCFG_EQ_DIVIDE_ENABLE*/

#if defined(EQ_LITE_CODE) && EQ_LITE_CODE
                               | EQ_LITE_VER_EN //不支持异步，不支持默认效果切换接口，仅支持文件解析
#if TCFG_DRC_ENABLE
                               | EQ_SUPPORT_32BIT_SYNC_EN //32bit同步方式eq使能,eq_lite_en后，该宏需使能
#endif/*TCFG_DRC_ENABLE*/
#endif/* EQ_LITE_CODE */
                               //|EQ_FILTER_COEFF_FADE_EN//默认系数切换更新时使用淡入淡出
#endif/* TCFG_EQ_ENABLE */
                               | 0; //end

const float config_audio_eq_fade_step = 0.1f;//播歌高低音增益调节步进
const int AUDIO_EQ_CLEAR_MEM_BY_MUTE_TIME_MS = 0;//300 //连续多长时间静音就清除EQ MEM
const int AUDIO_EQ_CLEAR_MEM_BY_MUTE_LIMIT = 0; //静音判断阀值

//wts解码支持采样率可选择，可以同时打开也可以单独打开
const  int  silk_fsN_enable = 1;   //支持8-12k采样率
const  int  silk_fsW_enable = 1;  //支持16-24k采样率


#if TCFG_DRC_ENABLE
const int config_audio_drc_en = 1;
#else
const int config_audio_drc_en = 0;
#endif

#if TCFG_MIC_EFFECT_ENABLE
const int config_audio_dac_mix_enable = 1;
#else
const int config_audio_dac_mix_enable = 0;
#endif

#ifdef CONFIG_SOUNDBOX_FLASH_256K
// mixer模块使能。不使能将关闭大部分功能，mix为直通
const int config_mixer_en = 0;
// mixer变采样使能
const int config_mixer_src_en = 0;

// audio解码资源叠加功能使能。不使能，如果配置了叠加方式，将改成抢占方式
const int config_audio_dec_wait_protect_en = 0;

// audio数据流分支功能使能。
const int config_audio_stream_frame_copy_en = 0;

// audio dec app调用mixer相关函数控制。关闭后需上层设置数据流的输出节点
const int audio_dec_app_mix_en = 0;

#else

// mixer模块使能。不使能将关闭大部分功能，mix为直通
const int config_mixer_en = 1;
// mixer变采样使能
const int config_mixer_src_en = 1;

// audio解码资源叠加功能使能。不使能，如果配置了叠加方式，将改成抢占方式
const int config_audio_dec_wait_protect_en = 1;

// audio数据流分支功能使能。
const int config_audio_stream_frame_copy_en = 1;

// audio dec app调用mixer相关函数控制。关闭后需上层设置数据流的输出节点
const int audio_dec_app_mix_en = 1;

#endif

// audio数据流分支cbuf大小控制
const int config_audio_stream_frame_copy_cbuf_min = 128;
const int config_audio_stream_frame_copy_cbuf_max = 1024;

#if RECORDER_MIX_EN
// 超时等待其他解码unactive步骤完成
const int config_audio_dec_unactive_to = 10;
// audio数据流ioctrl使能
const int config_audio_stream_frame_ioctrl_en = 1;
#else
// 超时等待其他解码unactive步骤完成
const int config_audio_dec_unactive_to = 0;
// audio数据流ioctrl使能
const int config_audio_stream_frame_ioctrl_en = 0;
#endif

#if TCFG_TONE2TWS_ENABLE

// audio dec app tws同步使能
const int audio_dec_app_sync_en = 1;

// wma tws 解码控制
const int WMA_TWSDEC_EN = 1;

#else

// audio dec app tws同步使能
const int audio_dec_app_sync_en = 0;

// wma tws 解码控制
const int WMA_TWSDEC_EN = 0;

#endif


#if TCFG_DEC2TWS_ENABLE || SOUNDCARD_ENABLE

// 解码一次输出点数，1代表32对点，n就是n*32对点
// 超过1时，解码需要使用malloc，如config_mp3_dec_use_malloc=1
const int MP3_OUTPUT_LEN = 4;
const int WMA_OUTPUT_LEN = 4;

#else

// 解码一次输出点数，1代表32对点，n就是n*32对点
// 超过1时，解码需要使用malloc，如config_mp3_dec_use_malloc=1
const int MP3_OUTPUT_LEN = 1;
const int WMA_OUTPUT_LEN = 1;

#endif

// wav最大支持比特率，单位kbps
const int WAV_MAX_BITRATEV = (48 * 2 * 24);

// 解码一次输出点数,建议范围32到900,例如128代表128对点
// 超过128时，解码需要使用malloc，如config_wav_dec_use_malloc=1
const int  WAV_DECODER_PCM_POINTS = 128;

// output超过128时，如果不使用malloc，需要增大对应buf
// 可以看打印中解码器需要的大小，一般输出每增加1长度增加4个字节
int wav_mem_ext[(1336  + 3) / 4] SEC(.wav_mem); //超过128要增加这个数组的大小


const int OPUS_SRINDEX = 0; //选择opus解码文件的帧大小，0代表一帧40字节，1代表一帧80字节，2代表一帧160字节


const  int  WTGV2_STACK2BUF = 0;  //等于1时解码buf会加大760，栈会减小

// mixer在单独任务中输出
const int config_mixer_task = 0;

// tws音频解码自动设置输出声道。
// 单声道：AUDIO_CH_L/AUDIO_CH_R。双声道：AUDIO_CH_DUAL_L/AUDIO_CH_DUAL_R
// 关闭后，按照output_ch_num和output_ch_type/ch_type设置输出声道
const int audio_tws_auto_channel = 1;

// 解码使用单独任务做输出
const int config_audio_dec_out_task_en = 0;


#ifdef CONFIG_256K_FLASH
const char config_audio_mini_enable = 1;
#else
const char config_audio_mini_enable = 0;
#endif



/*省电容mic配置*/
#if TCFG_SUPPORT_MIC_CAPLESS
const u8 const_mic_capless_en = 1;
#else
const u8 const_mic_capless_en = 0;
#endif/*TCFG_SUPPORT_MIC_CAPLESS*/


#if AUDIO_EQUALLOUDNESS_CONFIG
const int const_equall_loundness_en = 1;
#else
const int const_equall_loundness_en = 0;
#endif

#if AUDIO_VBASS_CONFIG
const int const_vbass_en = 1;
#else
const int const_vbass_en = 0;
#endif

const char config_audio_mixer_ch_highlight_enable = 0; //混音器声音突出功能使能

#if AUDIO_SURROUND_CONFIG
const int const_surround_en = 1;
#else
const int const_surround_en = 0;
#endif


const int const_sel_adpcm_type = 1;//1：使用imaen_adpcm,  0:msen_adpcm

#ifdef CONFIG_MIDI_DEC_ADDR
const int MIDI_TONE_MODE = 0;//0是地址访问(仅支持在内置flash,读数快，消耗mips低)，1 是文件访问(内置、外挂flash,sd,u盘均可,读数慢，消耗mips较大)
#else
const int MIDI_TONE_MODE = 1;
#endif


#if TCFG_MEDIA_LIB_USE_MALLOC
const int config_mp3_dec_use_malloc     = 1;
const int config_mp3pick_dec_use_malloc = 1;
const int config_wma_dec_use_malloc     = 1;
const int config_wmapick_dec_use_malloc = 1;
const int config_m4a_dec_use_malloc     = 1;
const int config_m4apick_dec_use_malloc = 1;
const int config_wav_dec_use_malloc     = 1;
const int config_alac_dec_use_malloc    = 1;
const int config_dts_dec_use_malloc     = 1;
const int config_amr_dec_use_malloc     = 1;
const int config_flac_dec_use_malloc    = 1;
const int config_ape_dec_use_malloc     = 1;
const int config_aac_dec_use_malloc     = 1;
const int config_aptx_dec_use_malloc    = 1;
const int config_midi_dec_use_malloc    = 1;
const int config_lc3_dec_use_malloc     = 1;
const int config_speex_dec_use_malloc   = 1;
const int config_opus_dec_use_malloc   = 1;
#else
const int config_mp3_dec_use_malloc     = 0;
const int config_mp3pick_dec_use_malloc = 0;
const int config_wma_dec_use_malloc     = 0;
const int config_wmapick_dec_use_malloc = 0;
const int config_m4a_dec_use_malloc     = 0;
const int config_m4apick_dec_use_malloc = 0;
#if  TCFG_DEC_WAV_ENABLE
const int config_wav_dec_use_malloc     = 1;
#else
const int config_wav_dec_use_malloc     = 0;
#endif
const int config_alac_dec_use_malloc    = 0;
const int config_dts_dec_use_malloc     = 0;
const int config_amr_dec_use_malloc     = 0;
const int config_flac_dec_use_malloc    = 0;
const int config_ape_dec_use_malloc     = 0;
const int config_aac_dec_use_malloc     = 0;
const int config_aptx_dec_use_malloc    = 0;
const int config_midi_dec_use_malloc    = 0;
const int config_lc3_dec_use_malloc     = 1;
const int config_speex_dec_use_malloc   = 0;
const int config_opus_dec_use_malloc   = 0;
#endif

#if (TCFG_ENC_LC3_ENABLE || TCFG_DEC_LC3_ENABLE)
const int   LC3_PLC_EN = 0;   //置1做plc，置0的效果类似补静音包
const  int  LC3_HW_FFT = 0;           //br27/br28置1，其他芯片置0
const int  LC3_INT24bit_INOUT = 0;
const int LC3_SUPPORT_CH = LC3_CODING_CHANNEL;     //lc3解码输入通道数 1：单声道输入， 2:双声道输入(br30可支持2)
const int LC3_DMS_VAL = LC3_CODING_FRAME_LEN;      //单位ms, 【只支持 25,50,100】
//LC3_DMS_FSINDEX配置采样率【只支持0到4】，影响用哪组表以及一次的处理长度(<=8k的时候，配0. <=16k的时候，配1.<=24k的时候，配2.<=32k的时候，配3.<=48k的时候，配4)
#if(LC3_CODING_SAMPLERATE <= 8000)
const int LC3_DMS_FSINDEX = 0;
#elif(LC3_CODING_SAMPLERATE <= 16000)
const int LC3_DMS_FSINDEX = 1;
#elif(LC3_CODING_SAMPLERATE <= 24000)
const int LC3_DMS_FSINDEX = 2;
#elif(LC3_CODING_SAMPLERATE <= 32000)
const int LC3_DMS_FSINDEX = 3;
#elif(LC3_CODING_SAMPLERATE <= 48000)
const int LC3_DMS_FSINDEX = 4;
#endif

const int LC3_QUALTIY_CONFIG = 4;//【范围1到4， 1需要的速度最少，这个默认先配4】

#endif

#if (TCFG_ENC_USBC_ENABLE || TCFG_DEC_USBC_ENABLE)
//参数设置     BITPOOL=26为msbc音质.  BLKS=(4:16)建议值10.
const int BITPOOL = 26;
const int BLKS = 10;
#endif

const int vc_pitchshift_fastmode_flag        = 1;
const  int  vc_pitchshift_downmode_flag = 0;  //变声下采样处理使能
const int howling_pitchshift_fastmode_flag   = 1;
const  int RS_FAST_MODE_QUALITY = 2;	//软件变采样 滤波阶数配置，范围2到8， 8代表16阶的变采样模式 ,速度跟它的大小呈正相关
const int config_howling_enable_pemafrow_mode = 0;
const int config_howling_enable_trap_mode     = 0;//陷波啸叫抑制模式使能
const int config_howling_enable_pitchps_mode  = 1; //移频啸叫抑制模式使能
const  int DOWN_S_FLAG 				= 0; //混响降采样处理使能
const  int  ECHO_INT_VAL_OUT = 0;       //  置1: echo的输出是int 后级需接DRC限幅 功能未实现

#if (AUDIO_OUTPUT_WAY == AUDIO_OUTPUT_WAY_DONGLE)
const int config_mp3_enc_use_layer_3	= 1;
#else
const int config_mp3_enc_use_layer_3	= 0;
#endif

#define FAST_FREQ_restrict				0x01 //限制超过16k的频率不解【一般超出人耳听力范围，但是仪器会测出来】
#define FAST_FILTER_restrict			0x02 //限制滤波器长度【子带滤波器旁瓣加大，边缘不够陡】
#define FAST_CHANNEL_restrict			0x04 //混合左右声道，再解码【如果是左右声道独立性较强的歌曲，会牺牲空间感，特别耳机听会听出来的话】

const int config_mp3_dec_speed_mode 	= 0; //FAST_FREQ_restrict | FAST_FILTER_restrict | FAST_CHANNEL_restrict; //3个开关都置上，是最快的解码模式

// 快进快退到文件end返回结束消息
const int config_decoder_ff_fr_end_return_event_end = 0;



// 解码任务测试
const int audio_decoder_test_en = 0;
// 当audio_decoder_test_en使能时需要实现以下接口
#if 0
void audio_decoder_test_out_before(struct audio_decoder *dec, void *buff, int len) {} ;
void audio_decoder_test_out_after(struct audio_decoder *dec, int wlen) {} ;
void audio_decoder_test_read_before(struct audio_decoder *dec, int len, u32 offset) {} ;
void audio_decoder_test_read_after(struct audio_decoder *dec, u8 *data, int rlen) {} ;
void audio_decoder_test_get_frame_before(struct audio_decoder *dec) {} ;
void audio_decoder_test_get_frame_after(struct audio_decoder *dec, u8 *frame, int rlen) {} ;
void audio_decoder_test_fetch_before(struct audio_decoder *dec) {} ;
void audio_decoder_test_fetch_after(struct audio_decoder *dec, u8 *frame, int rlen) {} ;
void audio_decoder_test_run_before(struct audio_decoder *dec) {} ;
void audio_decoder_test_run_after(struct audio_decoder *dec, int err) {} ;
#else
// 接口实现示例
#include "audio/demo/audio_decoder_test.c"
#endif


// 编码任务测试
const int audio_encoder_test_en = 0;
// 当audio_encoder_test_en使能时需要实现以下接口
#if 0
void audio_encoder_test_out_before(struct audio_encoder *enc, void *buff, int len) {} ;
void audio_encoder_test_out_after(struct audio_encoder *enc, int wlen) {} ;
void audio_encoder_test_get_frame_before(struct audio_encoder *enc, u16 frame_len) {} ;
void audio_encoder_test_get_frame_after(struct audio_encoder *enc, s16 *frame, int rlen) {} ;
void audio_encoder_test_run_before(struct audio_encoder *enc) {} ;
void audio_encoder_test_run_after(struct audio_encoder *enc, int err) {} ;
#else
// 接口实现示例
#include "audio/demo/audio_encoder_test.c"
#endif
//数字音量节点 是否使用汇编优化 不支持的芯片需置0
#if(CONFIG_CPU_BR18)
const int const_config_digvol_use_round = 0;
#else
const int const_config_digvol_use_round = 1;
#endif



/**
 * @brief Log (Verbose/Info/Debug/Warn/Error)
 */
/*-----------------------------------------------------------*/
const char log_tag_const_v_EQ_CFG AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_EQ_CFG AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_EQ_CFG AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_EQ_CFG AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_EQ_CFG AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_EQ_CFG_TOOL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_i_EQ_CFG_TOOL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_d_EQ_CFG_TOOL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(FALSE);
const char log_tag_const_w_EQ_CFG_TOOL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_EQ_CFG_TOOL AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_EQ_APPLY AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_EQ_APPLY AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_EQ_APPLY AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_EQ_APPLY AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_EQ_APPLY AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_DRC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_DRC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_DRC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_DRC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_DRC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_APP_DRC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_APP_DRC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_APP_DRC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_APP_DRC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_APP_DRC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);


const char log_tag_const_v_EQ AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_EQ AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_EQ AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_EQ AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_EQ AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);


const char log_tag_const_v_VBASS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_VBASS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_VBASS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_VBASS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_VBASS AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_AUD_ADC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_AUD_ADC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_AUD_ADC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_AUD_ADC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_AUD_ADC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_AUD_DAC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_AUD_DAC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_AUD_DAC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_AUD_DAC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_AUD_DAC AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);

const char log_tag_const_v_AUD_AUX AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_AUD_AUX AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_AUD_AUX AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_w_AUD_AUX AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_e_AUD_AUX AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);



const char log_tag_const_v_MIXER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_c_MIXER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_MIXER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_MIXER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_MIXER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_AUDIO_STREAM AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_c_AUDIO_STREAM AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_AUDIO_STREAM AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_AUDIO_STREAM AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_AUDIO_STREAM AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_AUDIO_DECODER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_c_AUDIO_DECODER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_AUDIO_DECODER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_AUDIO_DECODER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_AUDIO_DECODER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_AUDIO_ENCODER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_c_AUDIO_ENCODER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_AUDIO_ENCODER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_d_AUDIO_ENCODER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_AUDIO_ENCODER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);

const char log_tag_const_v_AEC_USER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_c_AEC_USER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(0);
const char log_tag_const_i_AEC_USER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(1);
const char log_tag_const_d_AEC_USER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);
const char log_tag_const_e_AEC_USER AT(.LOG_TAG_CONST) = CONFIG_DEBUG_LIB(TRUE);


