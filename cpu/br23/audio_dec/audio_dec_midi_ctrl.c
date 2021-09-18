#include "audio_dec_midi_ctrl.h"

#if AUDIO_MIDI_CTRL_CONFIG
/*
 *如遇到多按琴键同时按下或重复快速按某琴键出现卡顿的情况
 *处理方法：1、减少同时发声MIDI_KEY_NUM 个数
 *          2、提高外挂flash的 波特率（60M、90M）,调整spi读线宽的为2线模式或4线
 *          3、提高系统时钟
 * */
extern struct audio_dac_hdl dac_hdl;

#define MIDI_KEY_NUM  (10)//(支持多少个key同时发声(1~18)， 越大，需要的时钟越大)

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

struct _midi_obj {
    u8 channel;
    u32 sample_rate;
    u32 id;				// 唯一标识符，随机值
    u32 start : 1;		// 正在解码
    char *path;         //音色文件路径
    struct audio_res_wait wait;		// 资源等待句柄
    struct midi_ctrl_decoder midi_ctrl_dec;
    struct audio_mixer_ch mix_ch;	// 叠加句柄
    struct audio_stream *stream;		// 音频流
};

struct _midi_obj *midi_ctrl_dec_hdl = NULL;

void midi_ctrl_ioctrl(u32 cmd, void *priv);
/*----------------------------------------------------------------------------*/
/**@brief    midi ctrl解码释放
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_dec_relaese()
{
    if (midi_ctrl_dec_hdl) {
        audio_decoder_task_del_wait(&decode_task, &midi_ctrl_dec_hdl->wait);
        clock_remove(DEC_MIDI_CLK);
        local_irq_disable();
        free(midi_ctrl_dec_hdl);
        midi_ctrl_dec_hdl = NULL;
        local_irq_enable();
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    midi音色文件获取
   @param
   @return
   @note     内部调用
*/
/*----------------------------------------------------------------------------*/
static int midi_get_cfg_addr(u8 **addr)
{
    if (!midi_ctrl_dec_hdl) {
        return -1;
    }

#ifndef CONFIG_MIDI_DEC_ADDR
    //音色文件支持在外部存储卡或者外挂flash,sdk默认使用本方式
    /* FILE  *file = fopen("storage/sd0/C/MIDI.bin\0", "r"); */
    FILE  *file = fopen(midi_ctrl_dec_hdl->path, "r");

    /* FILE  *file = fopen(SDFILE_RES_ROOT_PATH"MIDI.bin\0", "r"); */
    if (!file) {
        log_e("MIDI.bin open err\n");
        return -1;
    }
    *addr = (u8 *)file;
    log_i("midi_file %x\n", file);
#else
    //音色文件仅支持在内置flash
    /* FILE  *file = fopen(SDFILE_RES_ROOT_PATH"MIDI.bin\0", "r"); */
    FILE  *file = fopen(midi_ctrl_dec_hdl->path, "r");
    if (!file) {
        log_e("MIDI.bin open err\n");
        return -1;
    }

    struct vfs_attr attr = {0};
    fget_attrs(file, &attr);
    *addr = (u8 *)attr.sclust;
    fclose(file);
#endif

    return 0;
}



/*----------------------------------------------------------------------------*/
/**@brief    midi音色文件读
   @param
   @return
   @note     内部调用
*/
/*----------------------------------------------------------------------------*/
static int midi_fread(void *file, void *buf, u32 len)
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
static int midi_fseek(void *file, u32 offset, int seek_mode)
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
/**@brief    midi ctrl初始化函数，该函数由库调用
   @param    参数返回地址
   @return   0
   @note     该函数是弱定义函数，不可修改定义
*/
/*----------------------------------------------------------------------------*/
int midi_ctrl_init(void *info)
{
    if (!midi_ctrl_dec_hdl) {
        return -1;
    }
    u8 *cache_addr;
    if (midi_get_cfg_addr(&cache_addr)) {
        return -1;
    }
    midi_ctrl_open_parm *parm = (midi_ctrl_open_parm *)info;
    parm->ctrl_parm.tempo = 1024;//解码会改变播放速度,1024是正常值
    parm->ctrl_parm.track_num = 1;//支持音轨的最大个数0~15

    parm->sample_rate = midi_ctrl_dec_hdl->midi_ctrl_dec.sample_rate;//midi_samplerate_tab[5];
    parm->cfg_parm.player_t = MIDI_KEY_NUM; //(支持多少个key同时发声,用户可修改)
    parm->cfg_parm.spi_pos = (unsigned int)cache_addr;
    parm->cfg_parm.fread = midi_fread;
    parm->cfg_parm.fseek = midi_fseek;

    for (int i = 0; i < ARRAY_SIZE(midi_samplerate_tab); i++) {
        if (parm->sample_rate == midi_samplerate_tab[i]) {
            parm->cfg_parm.sample_rate = i;
            break;
        }
    }
    return 0;
}
/*----------------------------------------------------------------------------*/
/**@brief    midi ctrl音色文件关闭
   @param
   @return
   @note     该函数在midi关闭时调用,该函数是弱定义函数，不可修改定义
*/
/*----------------------------------------------------------------------------*/
int midi_ctrl_uninit(void *priv)
{
#ifndef CONFIG_MIDI_DEC_ADDR
    if (priv) {
        fclose((FILE *)priv);
        priv = NULL;
    }
#endif
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief   乐器更新
   @param   prog:乐器号
   @param   trk_num :音轨 (0~15)
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_set_porg(u8 prog, u8 trk_num)
{
    struct set_prog_parm parm = {0};
    parm.prog = prog;
    parm.trk_num = trk_num;
    midi_ctrl_ioctrl(MIDI_CTRL_SET_PROG, &parm);
}

/*----------------------------------------------------------------------------*/
/**@brief   按键按下
   @param   nkey:按键序号（0~127）
   @param   nvel:按键力度（0~127）
   @param   chn :通道(0~15)
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_note_on(u8 nkey, u8 nvel, u8 chn)
{
    struct note_on_parm parm = {0};
    parm.nkey = nkey;
    parm.nvel = nvel;
    parm.chn = chn;
    midi_ctrl_ioctrl(MIDI_CTRL_NOTE_ON, &parm);
}
/*----------------------------------------------------------------------------*/
/**@brief   按键松开
   @param   nkey:按键序号（0~127）
   @param   chn :通道(0~15)
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_note_off(u8 nkey, u8 chn)
{
    struct note_off_parm parm = {0};
    parm.nkey = nkey;
    parm.chn = chn;
    midi_ctrl_ioctrl(MIDI_CTRL_NOTE_OFF, &parm);
}
/*----------------------------------------------------------------------------*/
/**@brief  midi 配置接口
   @param   cmd:命令
   @param   priv:对应cmd的结构体
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_confing(u32 cmd, void *priv)
{
    midi_ctrl_ioctrl(cmd, priv);
}

/*----------------------------------------------------------------------------*/
/**@brief   midi keyboard 设置按键按下音符发声的衰减系数
   @param   val:((衰减系数&0x7fff) | (延时系数&0x1f <<11))
   @return
   @note    低11bit为调节衰减系数，值越小，衰减越快， 1024为默认值， 范围：0~1024
            大于11bit为延时系数，节尾音长度的，值越大拉德越长，0为默认值,31延长1s,范围：0~31(延时系数无效)
   @example midi_ctrl_confing_set_melody_decay((衰减系数&0x7fff) | (延时系数&0x1f <<11));
			u16 val = ((1024&0x7ff) | (31&0x1f << 11));
			midi_ctrl_confing_set_melody_decay(val);
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_confing_set_melody_decay(u16 val)
{
    u32 cmd = CMD_MIDI_CTRL_TEMPO;
    MIDI_PLAY_CTRL_TEMPO tempo = {0};
    for (int i = 0; i < 16; i++) {
        tempo.decay_val[i] = val;
    }
    tempo.tempo_val = 1024;//设置为固定1024即可
    midi_ctrl_confing(cmd, (void *)&tempo);
}
/*----------------------------------------------------------------------------*/
/**@brief   midi keyboard 设置每个通道按下音符发声的衰减系数
   @param   val:16个值的数组，每个值组成组成结构((衰减系数&0x7fff) | (延时系数&0x1f <<11))
   @return
   @note    低11bit为调节衰减系数，值越小，衰减越快， 1024为默认值， 范围：0~1024
            大于11bit为延时系数，节尾音长度的，值越大拉德越长，0为默认值,31延长1s,范围：0~31(延时系数无效)
   @example u16 val[16];
   			for (int i = 0; i< 16; i++){
			    val[i] = ((1024&0x7ff) | (31&0x1f << 11));
			}
   			midi_ctrl_confing_set_melody_decay(val);
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_confing_set_melody_decay_each_chn(u16 *val)
{
    u32 cmd = CMD_MIDI_CTRL_TEMPO;
    MIDI_PLAY_CTRL_TEMPO tempo = {0};
    if (val) {
        memcpy(tempo.decay_val, val, sizeof(tempo.decay_val));
    }
    tempo.tempo_val = 1024;//设置为固定1024即可
    midi_ctrl_confing(cmd, (void *)&tempo);
}

/*----------------------------------------------------------------------------*/
/**@brief  弯音轮配置
   @param   pitch_val:弯音轮值,1 - 65535 ；256是正常值,对音高有作用
   @param   chn :通道(0~15)
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_pitch_bend(u16 pitch_val, u8 chn)
{
    struct pitch_bend_parm parm = {0};
    parm.pitch_val = pitch_val;
    parm.chn = chn;
    midi_ctrl_ioctrl(MIDI_CTRL_PITCH_BEND, &parm);
}



/*----------------------------------------------------------------------------*/
/**@brief    midi ctrl解码数据流激活
   @param    *p: 私有句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void midi_ctrl_dec_out_stream_resume(void *p)
{
    struct _midi_obj *dec = p;
#if FILE_DEC_USE_OUT_TASK
    if (dec->midi_ctrl_dec.dec_no_out_sound == 0) {
        audio_decoder_resume_out_task(&dec->midi_ctrl_dec.decoder);
        return ;
    }
#endif
    audio_decoder_resume(&dec->midi_ctrl_dec.decoder);
}


/*----------------------------------------------------------------------------*/
/**@brief    midi ctrl解码事件处理
   @param    *decoder: 解码器句柄
   @param    argc: 参数个数
   @param    *argv: 参数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void midi_ctrl_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_DEC_EVENT_END:
        if (!midi_ctrl_dec_hdl) {
            log_i("midi_ctrl_dec_hdl handle err ");
            break;
        }

        if (midi_ctrl_dec_hdl->id != argv[1]) {
            log_w("midi_ctrl_dec_hdl id err : 0x%x, 0x%x \n", midi_ctrl_dec_hdl->id, argv[1]);
            break;
        }

        midi_ctrl_dec_close();
        //audio_decoder_resume_all(&decode_task);
        break;
    }
}
/*----------------------------------------------------------------------------*/
/**@brief    midi ctrl解码开始
   @param
   @return   0：成功
   @return   非0：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int midi_ctrl_dec_start()
{
    int err;
    struct _midi_obj *dec = midi_ctrl_dec_hdl;

    struct audio_mixer *p_mixer = &mixer;

    if (!dec) {
        return -EINVAL;
    }

    log_i("midi_ctrl dec start: in\n");
// 打开midi ctrl解码器
    err = midi_ctrl_decoder_open(&dec->midi_ctrl_dec, &decode_task);
    if (err) {
        goto __err1;
    }

    midi_ctrl_decoder_set_event_handler(&dec->midi_ctrl_dec, midi_ctrl_dec_event_handler, dec->id);

// 设置叠加功能
    audio_mixer_ch_open_head(&dec->mix_ch, p_mixer);
    audio_mixer_ch_set_src(&dec->mix_ch, 1, 0);

#if FILE_DEC_USE_OUT_TASK
    if (dec->midi_ctrl_dec.dec_no_out_sound == 0) {
        audio_decoder_out_task_ch_enable(&dec->midi_ctrl_dec.decoder);
    }
#endif


// 数据流串联
    struct audio_stream_entry *entries[8] = {NULL};
    u8 entry_cnt = 0;

    entries[entry_cnt++] = &dec->midi_ctrl_dec.decoder.entry;
    entries[entry_cnt++] = &dec->mix_ch.entry;

    // 创建数据流，把所有节点连接起来

    dec->stream = audio_stream_open(dec, midi_ctrl_dec_out_stream_resume);
    audio_stream_add_list(dec->stream, entries, entry_cnt);

// 设置音频输出音量
    audio_output_set_start_volume(APP_AUDIO_STATE_MUSIC);
// 设置时钟
    clock_set_cur();

    // 开始解码
    dec->midi_ctrl_dec.status = FILE_DEC_STATUS_PLAY;
    err = audio_decoder_start(&dec->midi_ctrl_dec.decoder);
    dec->start = 1;
    if (err) {
        goto __err3;
    }
    return 0;
__err3:
    audio_mixer_ch_close(&dec->mix_ch);

    midi_ctrl_decoder_close(&dec->midi_ctrl_dec);

    if (dec->stream) {
        audio_stream_close(dec->stream);
        dec->stream = NULL;
    }
__err1:
    log_w(" start err close\n");
    midi_ctrl_dec_relaese();
    return -1;
}
/*----------------------------------------------------------------------------*/
/**@brief    midi ctrl解码关闭
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void __midi_ctrl_dec_close(void)
{
    if (midi_ctrl_dec_hdl && midi_ctrl_dec_hdl->start) {
        midi_ctrl_dec_hdl->start = 0;

        midi_ctrl_decoder_close(&midi_ctrl_dec_hdl->midi_ctrl_dec);

        audio_mixer_ch_close(&midi_ctrl_dec_hdl->mix_ch);

        // 先关闭各个节点，最后才close数据流
        if (midi_ctrl_dec_hdl->stream) {
            audio_stream_close(midi_ctrl_dec_hdl->stream);
            midi_ctrl_dec_hdl->stream = NULL;
        }

    }
}


/*----------------------------------------------------------------------------*/
/**@brief    关闭midi ctrl解码
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_dec_close(void)
{
    if (!midi_ctrl_dec_hdl) {
        return;
    }
    __midi_ctrl_dec_close();
    midi_ctrl_dec_relaese();
    clock_set_cur();
    log_i("midi ctrl dec close \n\n ");
}

/*----------------------------------------------------------------------------*/
/**@brief    midi ctrl解码资源等待
   @param    *wait: 句柄
   @param    event: 事件
   @return   0：成功
   @note     用于多解码打断处理
*/
/*----------------------------------------------------------------------------*/
static void __midi_ctrl_dec_close(void);
static int midi_ctrl_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;
    log_i("midi_ctrl_wait_res_handler, event:%d\n", event);
    if (event == AUDIO_RES_GET) {
        // 启动解码
        err = midi_ctrl_dec_start();
    } else if (event == AUDIO_RES_PUT) {
        // 被打断
        __midi_ctrl_dec_close();
    }

    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    打开midi ctrl解码
   @param    sample_rate: 采样率
   @param    *path:音色文件路径
   @return   0：成功
   @return   非0：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int midi_ctrl_dec_open(u32 sample_rate, char *path)
{
    int err = 0;
    int i = 0;
    struct _midi_obj *dec;
    dec = zalloc(sizeof(*dec));
    if (!dec) {
        return -ENOMEM;
    }
    midi_ctrl_dec_hdl = dec;
    dec->id = rand32();
    dec->path = path;

    dec->midi_ctrl_dec.ch_num = 2;

    dec->midi_ctrl_dec.output_ch_num = audio_output_channel_num();
    dec->midi_ctrl_dec.output_ch_type = audio_output_channel_type();
#if TCFG_MIC_EFFECT_ENABLE
    dec->midi_ctrl_dec.sample_rate = MIC_EFFECT_SAMPLERATE;
#else
    for (i = 0; i < ARRAY_SIZE(midi_samplerate_tab); i++) {
        if (sample_rate == midi_samplerate_tab[i]) {
            dec->midi_ctrl_dec.sample_rate = midi_samplerate_tab[i];
            break;
        }
    }
    if (i >= ARRAY_SIZE(midi_samplerate_tab)) {
        dec->midi_ctrl_dec.sample_rate = 16000;
        log_e("midi sample_rate check err ,will set default 16000 Hz\n");
    }
#endif
    dec->wait.priority = 2;
    dec->wait.preemption = 0;
    dec->wait.snatch_same_prio = 1;
    /* dec->wait.protect = 1; */
    dec->wait.handler = midi_ctrl_wait_res_handler;
    clock_add(DEC_MIDI_CLK);


    err = audio_decoder_task_add_wait(&decode_task, &dec->wait);
    return err;
}
/*----------------------------------------------------------------------------*/
/**@brief    midi ctrl控制函数
	@param   cmd:
			 MIDI_CTRL_NOTE_ON,     //按键按下，参数结构对应struct note_on_parm
			 MIDI_CTRL_NOTE_OFF,    //按键松开，参数结构对应struct note_off_parm
			 MIDI_CTRL_SET_PROG,    //更改乐器，参数结构对应struct set_prog_parm
			 MIDI_CTRL_PITCH_BEND,  //弯音轮，参数结构对应struct pitch_bend_parm
		     CMD_MIDI_CTRL_TEMPO,    //改变节奏,参数结构对应 MIDI_PLAY_CTRL_TEMPO
   @param    priv:对应cmd的参数地址
   @return   0
   @note    midi解码控制例程
*/
/*----------------------------------------------------------------------------*/
void midi_ctrl_ioctrl(u32 cmd, void *priv)
{
    struct _midi_obj *dec = midi_ctrl_dec_hdl;
    if (!dec) {
        log_e("midi ctrl dec NULL\n");
        return ;
    }

    log_i("midi ctrl cmd %x", cmd);
    audio_decoder_ioctrl(&dec->midi_ctrl_dec.decoder, cmd, priv);
}

#if 0
/*----------------------------------------------------------------------------*/
/**@brief   midi key 测试样例
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void midi_paly_test(u32 key)
{
    static u8 open_close = 0;
    static u8 change_prog = 0;
    static u8 note_on_off = 0;
    switch (key) {
    case KEY_IR_NUM_0:
        if (!open_close) {
            /* midi_ctrl_dec_open(16000);//启动midi key */
            /* midi_ctrl_dec_open(16000, "storage/sd0/C/MIDI.bin\0");//启动midi key */
            midi_ctrl_dec_open(16000, SDFILE_RES_ROOT_PATH"MIDI.bin\0");//启动midi key

        } else {
            midi_ctrl_dec_close();//关闭midi key
        }
        open_close = !open_close;
        break;
    case KEY_IR_NUM_1:
        if (!change_prog) {
            midi_ctrl_set_porg(0, 0);//设置0号乐器，音轨0
        } else {
            midi_ctrl_set_porg(22, 0);//设置22号乐器，音轨0
        }
        change_prog = !change_prog;
        break;
    case KEY_IR_NUM_2:
        if (!note_on_off) {
            //模拟按键57、58、59、60、61、62,以力度127，通道0，按下测试
            midi_ctrl_note_on(57, 127, 0);
            midi_ctrl_note_on(58, 127, 0);
            midi_ctrl_note_on(59, 127, 0);
            midi_ctrl_note_on(60, 127, 0);
            midi_ctrl_note_on(61, 127, 0);
            midi_ctrl_note_on(62, 127, 0);
        } else {
            //模拟按键57、58、59、60、61、62松开测试
            midi_ctrl_note_off(57,  0);
            midi_ctrl_note_off(58,  0);
            midi_ctrl_note_off(59,  0);
            midi_ctrl_note_off(60,  0);
            midi_ctrl_note_off(61,  0);
            midi_ctrl_note_off(62,  0);
        }
        note_on_off = !note_on_off;
        break;
    default:
        break;
    }
}
#endif
#endif
