/*
 ****************************************************************
 *File : audio_dec_midi_file.c
 *Note :
 *
 ****************************************************************
 */
//////////////////////////////////////////////////////////////////////////////
#include "asm/includes.h"
#include "media/includes.h"
#include "system/includes.h"
#include "effectrs_sync.h"
#include "app_config.h"
#include "audio_config.h"
#include "audio_dec.h"
#include "app_config.h"
#include "app_main.h"
#include "MIDI_DEC_API.h"
#include "audio_dec_file.h"
#include "lfwordana_enc_api.h"

#if defined(TCFG_DEC_MIDI_ENABLE) && TCFG_DEC_MIDI_ENABLE
//midi文件播放时，对应的音色文件路径(用户可修改路径)
#define MIDI_FILE_PATH  SDFILE_RES_ROOT_PATH"MIDI.bin\0"
//替换主旋律的音色库，由录音得到(用户可修改路径)
int *MIDI_W2S_FILE_PATH = "storage/sd0/C/W2S/W2S.raw\0";
//替换主旋律音色库对应的参数文件,录音时生成(用户可修改路径)
#define MIDI_W2S_PARM_PATH "storage/sd0/C/W2S/parmout.raw\0"

static const u16 midi_samplerate_tab[9] = {
    48000,
    44100,
    32000,
    24000,
    22050,
    16000,
    12000,
    11025,
    8000
};
static u32 get_sr_tab_num(u32 sr)
{
    int i = 0;
    for (i = 0; i < ARRAY_SIZE(midi_samplerate_tab); i++) {
        if (sr == midi_samplerate_tab[i]) {
            return i;
        }
    }
    return 4;
}
u32 tmark_trigger(void *priv, u8 *val, u8 len)
{
    return 0;
}

u32 melody_trigger(void *priv, u8 key, u8 vel)
{
    return 0;
}

u32 melody_stop_trigger(void *priv, u8 key)
{
    return 0;
}

u32 timDiv_trigger(void *priv)
{
    return 0;
}

u32 beat_trigger(void *priv, u8 val1/*一节多少拍*/, u8 val2/*每拍多少分音符*/)
{
    return 0;
}
/*----------------------------------------------------------------------------*/
/**@brief    midi音色文件读
   @param
   @return
   @note     内部调用
*/
/*----------------------------------------------------------------------------*/
int midi_fread(void *file, void *buf, u32 len)
{
#ifndef CONFIG_MIDI_DEC_ADDR
    FILE *hd = (FILE *)file;
    if (hd) {
        len = fread(hd, buf, len);
    }
#endif
    return len;
}
/*----------------------------------------------------------------------------*/
/**@brief    midi音色文件seek
   @param
   @return
   @note     内部调用
*/
/*----------------------------------------------------------------------------*/
int midi_fseek(void *file, u32 offset, int seek_mode)
{
#ifndef CONFIG_MIDI_DEC_ADDR
    FILE *hd = (FILE *)file;
    if (hd) {
        fseek(hd, offset, seek_mode);
    }
#endif
    return 0;
}
/*----------------------------------------------------------------------------*/
/**@brief    midi初始化函数，由midi_init调用
   @param    midi_init_info_v:midi信息
   @param    addr:音色文件句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void init_midi_info_val(MIDI_INIT_STRUCT  *midi_init_info_v, void *addr)
{
    //midi初始化表
    midi_init_info_v->init_info.player_t = 8;
    midi_init_info_v->init_info.sample_rate = get_sr_tab_num(22050);
    midi_init_info_v->init_info.spi_pos = (u32)addr;
    midi_init_info_v->init_info.fread = midi_fread;
    midi_init_info_v->init_info.fseek = midi_fseek;


    //midi的模式初始化
    midi_init_info_v->mode_info.mode = CMD_MIDI_CTRL_MODE_0; //CMD_MIDI_CTRL_MODE_2;

    //midi节奏初始化
    midi_init_info_v->tempo_info.tempo_val = 1042;

    for (u32 tmp_i = 0; tmp_i < 16; tmp_i++) {
        midi_init_info_v->tempo_info.decay_val[tmp_i] = ((u16)31 << 11) | 1024;
    }
    midi_init_info_v->tempo_info.mute_threshold = (u16)1L << 29;

    //midi主轨道初始化
    midi_init_info_v->mainTrack_info.chn = 17; //把哪个轨道当成主轨道 , 17:库内自动分配

    //midi外部音量初始化
    for (u32 tmp_i = 0; tmp_i < 16; tmp_i++) {
        midi_init_info_v->vol_info.cc_vol[tmp_i] = 4096; //4096即原来的音量
    }
    midi_init_info_v->vol_info.ex_vol_use_chn = 1;

    //midi的主轨道乐器设置
    midi_init_info_v->prog_info.prog = 0;
    midi_init_info_v->prog_info.ex_vol = 1024;
    midi_init_info_v->prog_info.replace_mode = 0;

    //OKON 模式设置
    midi_init_info_v->okon_info.Melody_Key_Mode = CMD_MIDI_MELODY_KEY_0;
    midi_init_info_v->okon_info.OKON_Mode = CMD_MIDI_OKON_MODE_0;


    //midi的mark控制初始化
    midi_init_info_v->mark_info.priv = NULL; //&file_mark;
    midi_init_info_v->mark_info.mark_trigger = tmark_trigger;

    //midi的melody控制初始化
    midi_init_info_v->moledy_info.priv = NULL; //&file_melody;
    midi_init_info_v->moledy_info.melody_trigger = melody_trigger;

    //midi的melody stop控制初始化
    midi_init_info_v->moledy_stop_info.priv = NULL;
    midi_init_info_v->moledy_stop_info.melody_stop_trigger = melody_stop_trigger;//主旋律音符停止播放回调

    //midi的小节回调控制初始化
    midi_init_info_v->tmDiv_info.priv = NULL;
    midi_init_info_v->tmDiv_info.timeDiv_trigger = timDiv_trigger;

    //midi的小拍回调控制初始化
    midi_init_info_v->beat_info.priv = NULL;
    midi_init_info_v->beat_info.beat_trigger = beat_trigger;

    //使能位控制
    midi_init_info_v->switch_info = MELODY_PLAY_ENABLE;// | MELODY_ENABLE | EX_VOL_ENABLE;            //主轨道播放使能

#if defined(MIDI_SUPPORT_W2S) && MIDI_SUPPORT_W2S
    //设置替换主轨道的声音的音源
    void *cache_addr;
    extern int midi_w2s_get_cfg_addr(void **addr);
    if (midi_w2s_get_cfg_addr(&cache_addr)) {
        log_e("get midi w2s addr err\n");
        return;
    }
    midi_w2s_parm_get(&midi_init_info_v->w2s_info);
    midi_init_info_v->w2s_info.key_diff = -7;////与音高成反比
    midi_init_info_v->w2s_info.rec_data = (u32)cache_addr;
#endif/*MIDI_SUPPORT_W2S*/
    return;
}

FILE  *midi_w2s_file = NULL;
int midi_w2s_get_cfg_addr(void **addr)
{
#ifndef CONFIG_MIDI_DEC_ADDR
    //音色文件支持在外部存储卡或者外挂flash,sdk默认使用本方式
    //获取音色文件
    FILE  *file = fopen(MIDI_W2S_FILE_PATH, "r");
    if (!file) {
        log_e("MIDI.bin open err\n");
        return -1;
    }
    *addr = (void *)file;
    midi_w2s_file = file;
    log_i("midi_w2s_file %x\n", midi_w2s_file);
#else
    //音色文件仅支持在内置flash
    FILE  *file = fopen(MIDI_W2S_FILE_PATH, "r");
    if (!file) {
        log_e("MIDI.bin open err\n");
        return -1;
    }

    struct vfs_attr attr = {0};
    fget_attrs(file, &attr);
    *addr = (void *)attr.sclust;
    fclose(file);
#endif

    return 0;
}


FILE  *midi_file = NULL;
int midi_get_cfg_addr(void **addr)
{
#ifndef CONFIG_MIDI_DEC_ADDR
    //音色文件支持在外部存储卡或者外挂flash,sdk默认使用本方式
    //获取音色文件
    FILE  *file = fopen(MIDI_FILE_PATH, "r");
    if (!file) {
        log_e("MIDI.bin open err\n");
        return -1;
    }
    *addr = (void *)file;
    midi_file = file;
    log_i("midi_file %x\n", midi_file);
#else
    //音色文件仅支持在内置flash
    FILE  *file = fopen(MIDI_FILE_PATH, "r");
    if (!file) {
        log_e("MIDI.bin open err\n");
        return -1;
    }

    struct vfs_attr attr = {0};
    fget_attrs(file, &attr);
    *addr = (void *)attr.sclust;
    fclose(file);
#endif

    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    midi音色文件关闭
   @param
   @return
   @note     该函数在midi关闭时调用,该函数是弱定义函数，不可修改定义
*/
/*----------------------------------------------------------------------------*/
int midi_uninit()
{
#ifndef CONFIG_MIDI_DEC_ADDR
    if (midi_file) {
        fclose(midi_file);
        midi_file = NULL;
    }

    if (midi_w2s_file) {
        fclose(midi_w2s_file);
        midi_w2s_file = NULL;
    }
#endif
    return 0;
}
/*----------------------------------------------------------------------------*/
/**@brief    midi初始化函数，该函数由库调用
   @param    参数返回地址
   @return   0
   @note     该函数是弱定义函数，不可修改定义
*/
/*----------------------------------------------------------------------------*/
int midi_init(void *info)
{
    void *cache_addr;
    if (midi_get_cfg_addr(&cache_addr)) {
        log_e("get midi addr err\n");
        return -1;
    }
    //初始化midi参数
    init_midi_info_val(info, cache_addr);  //需要外部写
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    midi控制函数
	@param   cmd:
		     CMD_MIDI_SET_CHN_PROG,  //更改乐器,参数结构对应 MIDI_PROG_CTRL_STRUCT
		     CMD_MIDI_CTRL_TEMPO,    //改变节奏,参数结构对应 MIDI_PLAY_CTRL_TEMPO
		     CMD_MIDI_GOON,          //one key one note的时候接着播放使用,参数为空
		     CMD_MIDI_CTRL_MODE,     //改变模式,参数结构对应 MIDI_PLAY_CTRL_MO
		     CMD_MIDI_SET_SWITCH,    //配置开关使能，要不要替换乐器，使用外部音量,参数对应 MIDI_SET_SWITCH
		     CMD_MIDI_SET_EX_VOL,    //设置外部声道音量,参数结构对应 EX_CH_VOL_PARM
   @param    priv:对应cmd的参数地址
   @return   0
   @note    midi解码控制例程
*/
/*----------------------------------------------------------------------------*/
void midi_ioctrl(u32 cmd, void *priv)
{
    struct file_dec_hdl *dec = get_file_dec_hdl();	// 文件解码句柄
    if (dec) {
        log_e("file dec NULL\n");
        return ;
    }

    int status = file_dec_get_status();
    if (status == FILE_DEC_STATUS_STOP) {
        if (tone_get_dec_status()) {
            audio_decoder_ioctrl(get_tone_dec_file_decoder(), cmd, priv);
        } else {
            log_w("file dec is stop\n");
        }
        return ;
    }

    log_i("midi cmd %x", cmd);
    audio_decoder_ioctrl(&dec->file_dec.decoder, cmd, priv);
}
#if 0
//测试例子
void *ex_vol_test()
{
    static int val = 4096;
    EX_CH_VOL_PARM ex_vol;
    for (int test_ci = 0; test_ci < CTRL_CHANNEL_NUM; test_ci++) {
        ex_vol.cc_vol[test_ci] = val;
    }
    val -= 64;
    if (val <= 0) {
        val = 4096;
    }
    midi_ioctrl(CMD_MIDI_SET_EX_VOL, &ex_vol);
    return NULL;
}
#endif


#if defined(MIDI_SUPPORT_W2S) && MIDI_SUPPORT_W2S
int midi_w2s_init(void *info)
{
    struct _AUDIO_DECODE_PARA {
        u32 mode;
    };
    struct _AUDIO_DECODE_PARA *mode_parm = (struct _AUDIO_DECODE_PARA *)info;
    mode_parm->mode = CMD_MIDI_CTRL_MODE_W2S;
    return 0;
}


/*----------------------------------------------------------------------------*/
/**@brief    midi主旋律是否用录音文件替换主旋律
   @param    flag:1(替换主旋律)  0(不替换主旋律)
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_mode_change(u8 flag)
{
    struct _AUDIO_DECODE_PARA {
        u32 mode;
    };
    struct _AUDIO_DECODE_PARA mode_parm = {0};
    if (flag) {
        mode_parm.mode = CMD_MIDI_CTRL_MODE_W2S;
    } else {
        mode_parm.mode = CMD_MIDI_CTRL_MODE_0;
    }
    midi_ioctrl(CMD_MIDI_CTRL_MODE, &mode_parm);
}

void midi_mode_test(void *p)
{
    static u8 flag = 0;
    midi_mode_change(flag);
    flag = !flag;
}
#endif/*MIDI_SUPPORT_W2S*/

/*----------------------------------------------------------------------------*/
/**@brief    midi预处理的音色，对应的输出的参数
   @param    data:参数地址
   @param    len:参数长度
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_w2s_parm_set(void *data, u32 len)
{
    MIDI_W2S_STRUCT *w2s_parmOut;
    void *fp = fopen(MIDI_W2S_PARM_PATH, "wb");
    if (fp) {
        fwrite(fp, data, len);
        fclose(fp);
        fp = NULL;
    }
}
/*----------------------------------------------------------------------------*/
/**@brief    获取midi预处理的音色，对应的参数
   @param    parm:参数返回地址
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_w2s_parm_get(void *parm)
{
    void *fp = fopen(MIDI_W2S_PARM_PATH, "r");
    if (fp) {
        fread(fp, parm, sizeof(MIDI_W2S_STRUCT));
        fclose(fp);
        fp = NULL;
    }
}


#endif /*TCFG_DEC_MIDI_ENABLE*/
