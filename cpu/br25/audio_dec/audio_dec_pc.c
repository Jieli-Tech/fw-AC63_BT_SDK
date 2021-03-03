/*****************************************************************
>file name : audio_dec_pc.c
>author : lichao
>create time : Wed 22 May 2019 10:36:22 AM CST
*****************************************************************/
#include "uac_stream.h"
#include "app_config.h"
#include "audio_decoder.h"
#include "media/includes.h"
#include "audio_config.h"
#include "system/includes.h"
#include "audio_enc.h"
#include "application/audio_eq_drc_apply.h"
#include "app_config.h"
#include "audio_config.h"
#include "audio_dec.h"
#include "app_main.h"
#include "clock_cfg.h"
#include "media/pcm_decoder.h"
#include "bt_tws.h"

#if TCFG_UI_ENABLE
#include "ui/ui_api.h"
#endif


#if TCFG_APP_PC_EN

//////////////////////////////////////////////////////////////////////////////

struct uac_dec_hdl {
    struct audio_stream *stream;	// 音频流
    struct pcm_decoder pcm_dec;		// pcm解码句柄
    struct audio_res_wait wait;		// 资源等待句柄
    struct audio_mixer_ch mix_ch;	// 叠加句柄
    struct audio_eq_drc *eq_drc;//eq drc句柄
    u32 id;				// 唯一标识符，随机值
    u32 start : 1;		// 正在解码
    u32 source : 8;		// 音频源
    int check_data_timer;
};

//////////////////////////////////////////////////////////////////////////////

static struct uac_dec_hdl *uac_dec = NULL;	// pc解码句柄

static u16 sys_event_id = 0;

//////////////////////////////////////////////////////////////////////////////

extern u16 uac_get_cur_vol(const u8 id, u16 *l_vol, u16 *r_vol);
extern u8 uac_get_mute(void);
extern int usb_audio_mic_open(void *_info);
extern int usb_audio_mic_close(void *arg);
extern void usb_audio_mic_set_gain(int gain);

void *pc_eq_drc_open(u16 sample_rate, u8 ch_num);
void pc_eq_drc_close(struct audio_eq_drc *eq_drc);

//////////////////////////////////////////////////////////////////////////////

/*----------------------------------------------------------------------------*/
/**@brief    pc音量值转换
   @param    vol: pc音量
   @return   转换后的系统音量
   @note
*/
/*----------------------------------------------------------------------------*/
int uac_vol_switch(int vol)
{
    u16 valsum = vol * (SYS_MAX_VOL + 1) / 100;

    if (valsum > SYS_MAX_VOL) {
        valsum = SYS_MAX_VOL;
    }
    return valsum;
}

/*----------------------------------------------------------------------------*/
/**@brief    uac解码释放
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void uac_dec_relaese()
{
    if (!uac_dec) {
        return;
    }
    audio_decoder_task_del_wait(&decode_task, &uac_dec->wait);

    clock_remove(DEC_PCM_CLK);

    local_irq_disable();
    free(uac_dec);
    uac_dec = NULL;
    local_irq_enable();
}


/*----------------------------------------------------------------------------*/
/**@brief    uac解码数据流激活
   @param    *p: 私有句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void uac_dec_out_stream_resume(void *p)
{
    struct uac_dec_hdl *dec = p;
    audio_decoder_resume(&dec->pcm_dec.decoder);
}

/*----------------------------------------------------------------------------*/
/**@brief    uac解码激活
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void uac_dec_resume(void)
{
    if (uac_dec) {
        audio_decoder_resume(&uac_dec->pcm_dec.decoder);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    uac获取buf总长度
   @param    *priv: 私有参数
   @return   总长度
   @note
*/
/*----------------------------------------------------------------------------*/
static int uac_dec_get_total(void *priv)
{
    return uac_speaker_stream_length();
}

/*----------------------------------------------------------------------------*/
/**@brief    uac获取数据长度
   @param    *priv: 私有参数
   @return   数据长度
   @note
*/
/*----------------------------------------------------------------------------*/
static int uac_dec_get_size(void *priv)
{
    return uac_speaker_stream_size();
}

/*----------------------------------------------------------------------------*/
/**@brief    pc解码关闭
   @param
   @return   0：成功
   @note
*/
/*----------------------------------------------------------------------------*/
static int uac_audio_close(void)
{
    if (!uac_dec || !uac_dec->start) {
        return 0;
    }

    uac_dec->start = 0;
    if (uac_dec->check_data_timer) {
        sys_hi_timer_del(uac_dec->check_data_timer);
    }
    pcm_decoder_close(&uac_dec->pcm_dec);
    pc_eq_drc_close(uac_dec->eq_drc);
    audio_mixer_ch_close(&uac_dec->mix_ch);
    app_audio_state_exit(APP_AUDIO_STATE_MUSIC);

#if TCFG_PCM_ENC2TWS_ENABLE
    if (uac_dec->pcm_dec.dec_no_out_sound) {
        uac_dec->pcm_dec.dec_no_out_sound = 0;
        localtws_enc_api_close();
    }
#endif

    // 先关闭各个节点，最后才close数据流
    if (uac_dec->stream) {
        audio_stream_close(uac_dec->stream);
        uac_dec->stream = NULL;
    }

    clock_set_cur();
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    uac解码事件处理
   @param    *decoder: 解码器句柄
   @param    argc: 参数个数
   @param    *argv: 参数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void uac_dec_event_handler(struct audio_decoder *decoder, int argc, int *argv)
{
    switch (argv[0]) {
    case AUDIO_DEC_EVENT_END:
        puts("USB AUDIO_DEC_EVENT_END\n");
        if (!uac_dec) {
            log_i("uac_dec handle err ");
            break;
        }

        if (uac_dec->id != argv[1]) {
            log_w("uac_dec id err : 0x%x, 0x%x \n", uac_dec->id, argv[1]);
            break;
        }

        /*uac_audio_close();*/
        break;
    }
}
/*----------------------------------------------------------------------------*/
/**@brief    检测uac是否收到数据，没数据时暂停mix_ch
   @param    *priv: 私有参数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void audio_pc_check_timer(void *priv)
{
    static u8 cnt = 0;
    if (uac_speaker_stream_size(NULL) == 0) {
        if (cnt < 6) {
            cnt++;
        }
        if (cnt == 5) {
#if TCFG_DEC2TWS_ENABLE
            if (uac_dec->pcm_dec.dec_no_out_sound) {
                localtws_decoder_pause(1);
            }
#endif
            audio_mixer_ch_pause(&uac_dec->mix_ch, 1);
            audio_decoder_resume_all(&decode_task);
        }
    } else {
        if (cnt > 4) {
#if TCFG_DEC2TWS_ENABLE
            if (uac_dec->pcm_dec.dec_no_out_sound) {
                localtws_decoder_pause(0);
            }
#endif
            audio_mixer_ch_pause(&uac_dec->mix_ch, 0);
            audio_decoder_resume_all(&decode_task);
        }
        cnt = 0;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    计算pc输入采样率
   @param
   @return   采样率
   @note
*/
/*----------------------------------------------------------------------------*/
static int audio_pc_input_sample_rate(void *priv)
{
    struct uac_dec_hdl *dec = (struct uac_dec_hdl *)priv;
    int sample_rate = uac_speaker_stream_sample_rate();
    int buf_size = uac_speaker_stream_size();
#if TCFG_PCM_ENC2TWS_ENABLE
    if (dec->pcm_dec.dec_no_out_sound) {
        /*TWS的上限在uac输入buffer，下限在tws push buffer*/
        if (buf_size >= uac_speaker_stream_length() / 2) {
            sample_rate += (sample_rate * 1 / 10000);
        } else if (tws_api_local_media_trans_check_ready_total() < 1024) {
            sample_rate -= (sample_rate * 1 / 10000);
        }
        return sample_rate;
    }
#endif
    if (buf_size >= (uac_speaker_stream_length() * 3 / 4)) {
        sample_rate += (sample_rate * 1 / 10000);
    }
    if (buf_size <= (uac_speaker_stream_length() / 4)) {
        sample_rate -= (sample_rate * 1 / 10000);
    }
    return sample_rate;
}

/*----------------------------------------------------------------------------*/
/**@brief    uac解码开始
   @param
   @return   0：成功
   @return   非0：失败
   @note
*/
/*----------------------------------------------------------------------------*/
static int uac_audio_start(void)
{
    int err;
    struct uac_dec_hdl *dec = uac_dec;
    struct audio_mixer *p_mixer = &mixer;

    if (!uac_dec) {
        return -EINVAL;
    }

    // 打开pcm解码器
    err = pcm_decoder_open(&dec->pcm_dec, &decode_task);
    if (err) {
        goto __err1;
    }

    pcm_decoder_set_event_handler(&dec->pcm_dec, uac_dec_event_handler, dec->id);
    pcm_decoder_set_read_data(&dec->pcm_dec, uac_speaker_read, NULL);


#if TCFG_PCM_ENC2TWS_ENABLE
    {
        // localtws使用sbc等编码转发
        struct audio_fmt enc_f;
        memcpy(&enc_f, &dec->pcm_dec.decoder.fmt, sizeof(struct audio_fmt));
        enc_f.coding_type = AUDIO_CODING_SBC;
        int ret = localtws_enc_api_open(&enc_f, LOCALTWS_ENC_FLAG_STREAM);
        if (ret == true) {
            dec->pcm_dec.dec_no_out_sound = 1;
            // 重定向mixer
            p_mixer = &g_localtws.mixer;
            // 关闭资源等待。最终会在localtws解码处等待
            audio_decoder_task_del_wait(&decode_task, &dec->wait);
        }
    }
#endif

    if (!dec->pcm_dec.dec_no_out_sound) {
        audio_mode_main_dec_open(AUDIO_MODE_MAIN_STATE_DEC_PC);
    }

    // 设置叠加功能
    audio_mixer_ch_open_head(&dec->mix_ch, p_mixer);
    audio_mixer_ch_follow_resample_enable(&dec->mix_ch, dec, audio_pc_input_sample_rate);
    dec->eq_drc = pc_eq_drc_open(dec->pcm_dec.sample_rate, dec->pcm_dec.output_ch_num);
    // 数据流串联
    struct audio_stream_entry *entries[8] = {NULL};
    u8 entry_cnt = 0;
    entries[entry_cnt++] = &dec->pcm_dec.decoder.entry;
#if TCFG_EQ_ENABLE && TCFG_PC_MODE_EQ_ENABLE
    if (dec->eq_drc) {
        entries[entry_cnt++] = &dec->eq_drc->entry;
    }
#endif
    entries[entry_cnt++] = &dec->mix_ch.entry;
    // 创建数据流，把所有节点连接起来
    dec->stream = audio_stream_open(dec, uac_dec_out_stream_resume);
    audio_stream_add_list(dec->stream, entries, entry_cnt);


    // 设置音量
    audio_output_set_start_volume(APP_AUDIO_STATE_MUSIC);
    u16 l_volume, r_volume;
    uac_get_cur_vol(0, &l_volume, &r_volume);
    u8 vol = uac_vol_switch(l_volume);
    app_audio_set_volume(APP_AUDIO_STATE_MUSIC, vol, 0);
#if (TCFG_DEC2TWS_ENABLE)
    bt_tws_sync_volume();
#endif

    // 开始解码
    dec->start = 1;
    err = audio_decoder_start(&dec->pcm_dec.decoder);
    if (err) {
        goto __err3;
    }
    /*dec->check_data_timer = sys_hi_timer_add(NULL, audio_pc_check_timer, 5);*/
    clock_set_cur();
    return 0;

__err3:
    dec->start = 0;
    pc_eq_drc_close(dec->eq_drc);
    audio_mixer_ch_close(&dec->mix_ch);
#if TCFG_PCM_ENC2TWS_ENABLE
    if (dec->pcm_dec.dec_no_out_sound) {
        dec->pcm_dec.dec_no_out_sound = 0;
        localtws_enc_api_close();
    }
#endif

    if (dec->stream) {
        audio_stream_close(dec->stream);
        dec->stream = NULL;
    }
    pcm_decoder_close(&dec->pcm_dec);
__err1:
    uac_dec_relaese();
    return err;
}



/*----------------------------------------------------------------------------*/
/**@brief    uac解码资源等待
   @param    *wait: 句柄
   @param    event: 事件
   @return   0：成功
   @note     用于多解码打断处理
*/
/*----------------------------------------------------------------------------*/
static int uac_wait_res_handler(struct audio_res_wait *wait, int event)
{
    int err = 0;

    if (event == AUDIO_RES_GET) {
        // 启动解码
        err = uac_audio_start();
    } else if (event == AUDIO_RES_PUT) {
        // 被打断
        uac_audio_close();
    }

    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    uac收到数据处理
   @param    event: 事件
   @param    *data: 数据
   @param    len: 数据长度
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void uac_speaker_stream_rx_handler(int event, void *data, int len)
{
    /* putchar('A'); */
    uac_dec_resume();
}

/*----------------------------------------------------------------------------*/
/**@brief    打开pc解码
   @param    *_info: pc信息
   @return   0：成功
   @note
*/
/*----------------------------------------------------------------------------*/
static int usb_audio_play_open(void *_info)
{
    struct uac_dec_hdl *dec;

    if (uac_dec) {
        return 0;
    }

    dec = zalloc(sizeof(*dec));
    if (!dec) {
        return -ENOMEM;
    }

    uac_dec = dec;

#if TCFG_DEC2TWS_ENABLE
    // 设置localtws重播接口
    localtws_globle_set_dec_restart(uac_dec_push_restart);
#endif

    dec->pcm_dec.sample_rate = (u32)_info & 0xFFFFFF;
    dec->pcm_dec.ch_num = (u32)_info >> 24;
    dec->pcm_dec.output_ch_num = audio_output_channel_num();
    dec->pcm_dec.output_ch_type = audio_output_channel_type();
    printf("usb_audio_play_open sr:%d ch:%d\n", dec->pcm_dec.sample_rate, dec->pcm_dec.ch_num);
    set_uac_speaker_rx_handler(dec, uac_speaker_stream_rx_handler);
    dec->wait.priority = 2;
    dec->wait.preemption = 0;
    dec->wait.snatch_same_prio = 1;
    dec->wait.handler = uac_wait_res_handler;
    audio_decoder_task_add_wait(&decode_task, &dec->wait);

    clock_add(DEC_PCM_CLK);

    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    关闭pc解码
   @param    *arg: 参数
   @return   0：成功
   @note
*/
/*----------------------------------------------------------------------------*/
static int usb_audio_play_close(void *arg)
{
    int err = 0;

    if (!uac_dec) {
        return 0;
    }

    if (uac_dec->start) {
        uac_audio_close();
    }

    uac_dec_relaese();

    return 0;
}


/*----------------------------------------------------------------------------*/
/**@brief    pc解码重新开始
   @param    id: 文件解码id
   @return   0：成功
   @return   非0：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int uac_dec_restart(int id)
{
    if ((!uac_dec) || (id != uac_dec->id)) {
        return -1;
    }
    int _info = (uac_dec->pcm_dec.ch_num << 24) | uac_dec->pcm_dec.sample_rate;
    usb_audio_play_close(NULL);
    int err = usb_audio_play_open((void *)_info);
    return err;
}

/*----------------------------------------------------------------------------*/
/**@brief    推送pc解码重新开始命令
   @param
   @return   true：成功
   @return   false：失败
   @note
*/
/*----------------------------------------------------------------------------*/
int uac_dec_push_restart(void)
{
    if (!uac_dec) {
        return false;
    }
    int argv[3];
    argv[0] = (int)uac_dec_restart;
    argv[1] = 1;
    argv[2] = (int)uac_dec->id;
    os_taskq_post_type(os_current_task(), Q_CALLBACK, ARRAY_SIZE(argv), argv);
    return true;
}


/*----------------------------------------------------------------------------*/
/**@brief    pc事件处理
   @param    event: 事件
   @param    value: 参数
   @return   0：成功
   @note
*/
/*----------------------------------------------------------------------------*/
static int usb_device_event_handler(u8 event, int value)
{
    switch (event) {
    case USB_AUDIO_PLAY_OPEN:
        /*tone_play_stop();*/
        usb_audio_play_open((void *)value);
        break;
    case USB_AUDIO_PLAY_CLOSE:
        usb_audio_play_close((void *)value);
        break;
    case USB_AUDIO_MIC_OPEN:
        usb_audio_mic_open((void *)value);
        break;
    case USB_AUDIO_MIC_CLOSE:
        usb_audio_mic_close((void *)value);
        break;
    case USB_AUDIO_SET_MIC_VOL:
        usb_audio_mic_set_gain(value);
        break;
    case USB_AUDIO_SET_PLAY_VOL:
        value = uac_vol_switch(value & 0xffff);  //left ch volume
        app_audio_set_volume(APP_AUDIO_STATE_MUSIC, value, 1);
#if (TCFG_DEC2TWS_ENABLE)
        bt_tws_sync_volume();
#endif
#if TCFG_UI_ENABLE
        u8 vol = app_audio_get_volume(APP_AUDIO_STATE_MUSIC);
        UI_SHOW_MENU(MENU_MAIN_VOL, 1000, vol, NULL);
#endif //TCFG_UI_ENABLE
        break;
    default:
        break;
    }
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    pc系统事件处理
   @param    *event: 事件
   @param    *priv: 参数
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void usb_audio_event_handler(struct sys_event *event)
{
    log_d("usb device event : %d %x\n", event->u.dev.event, event->u.dev.value);
    usb_device_event_handler(event->u.dev.event, event->u.dev.value);
}

/*----------------------------------------------------------------------------*/
/**@brief    音频设备初始化
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
__attribute__((weak))int audio_dev_init()
{
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    usb音频初始化
   @param
   @return   0：成功
   @note
*/
/*----------------------------------------------------------------------------*/
int usb_audio_demo_init(void)
{
    int err = 0;

    audio_dev_init();
    register_sys_event_handler(SYS_DEVICE_EVENT, DEVICE_EVENT_FROM_UAC, 2,
                               usb_audio_event_handler);

    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    usb音频退出
   @param
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void usb_audio_demo_exit(void)
{
    unregister_sys_event_handler(usb_audio_event_handler);
    usb_audio_play_close(NULL);
    usb_audio_mic_close(NULL);
}
/*----------------------------------------------------------------------------*/
/**@brief    pc模式 eq drc 打开
   @param    sample_rate:采样率
   @param    ch_num:通道个数
   @return   句柄
   @note
*/
/*----------------------------------------------------------------------------*/
void *pc_eq_drc_open(u16 sample_rate, u8 ch_num)
{

#if TCFG_EQ_ENABLE
    struct audio_eq_drc *eq_drc = NULL;
    struct audio_eq_drc_parm effect_parm = {0};

#if TCFG_PC_MODE_EQ_ENABLE
    effect_parm.eq_en = 1;

#if TCFG_PC_MODE_DRC_ENABLE
    effect_parm.drc_en = 1;
    effect_parm.drc_cb = drc_get_filter_info;
#endif


    if (effect_parm.eq_en) {
        effect_parm.async_en = 1;
        effect_parm.out_32bit = 1;
        effect_parm.online_en = 1;
        effect_parm.mode_en = 1;
    }

    effect_parm.eq_name = song_eq_mode;
#if TCFG_EQ_DIVIDE_ENABLE
    effect_parm.divide_en = 1;
#endif


    effect_parm.ch_num = ch_num;
    effect_parm.sr = sample_rate;
    effect_parm.eq_cb = eq_get_filter_info;
    eq_drc = audio_eq_drc_open(&effect_parm);

    clock_add(EQ_CLK);
    if (effect_parm.drc_en) {
        clock_add(EQ_DRC_CLK);
    }
#endif
    return eq_drc;
#endif
    return NULL;
}

/*----------------------------------------------------------------------------*/
/**@brief    pc模式 eq drc 关闭
   @param    句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void pc_eq_drc_close(struct audio_eq_drc *eq_drc)
{
#if TCFG_EQ_ENABLE
#if TCFG_PC_MODE_EQ_ENABLE
    if (eq_drc) {
        audio_eq_drc_close(eq_drc);
        eq_drc = NULL;
        clock_remove(EQ_CLK);
#if TCFG_DRC_ENABLE
#if TCFG_PC_MODE_DRC_ENABLE
        clock_remove(EQ_DRC_CLK);
#endif
#endif
    }
#endif
#endif
    return;
}

#endif /* TCFG_APP_PC_EN */

