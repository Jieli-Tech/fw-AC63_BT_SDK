#include "system/includes.h"
#include "media/includes.h"
#include "tone_player.h"
#include "audio_config.h"
#include "app_main.h"
#include "clock_cfg.h"
#include "audio_dec.h"

#define AUDIO_DEC_TONE_WAIT_USE_PRIO			0 // 采用优先级顺序播放方式

extern struct audio_dac_hdl dac_hdl;
extern struct audio_dac_channel default_dac;
extern const int audio_dec_app_mix_en;

//////////////////////////////////////////////////////////////////////////////
extern int audio_dec_file_app_init_ok(struct audio_dec_file_app_hdl *file_dec);
extern int audio_dec_sine_app_init_ok(struct audio_dec_sine_app_hdl *sine_dec);
extern int audio_dec_file_app_play_end(struct audio_dec_file_app_hdl *file_dec);
extern int audio_dec_sine_app_play_end(struct audio_dec_sine_app_hdl *sine_dec);

static int tone_dec_list_play(struct tone_dec_handle *dec, u8 next);

//////////////////////////////////////////////////////////////////////////////

/*----------------------------------------------------------------------------*/
/**@brief    tone解码数据流推送stop
   @param    *dec: 解码句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void tone_dec_stream_run_stop(struct audio_dec_app_hdl *dec)
{
#if AUDIO_DAC_MULTI_CHANNEL_ENABLE
    struct audio_data_frame frame = {0};
    struct audio_data_frame output = {0};
    frame.stop = 1;
    frame.channel = audio_output_channel_num();
    frame.sample_rate = app_audio_output_samplerate_get();
    /* audio_stream_run(&default_dac.entry, &frame); */
    default_dac.entry.data_handler(&default_dac.entry, &frame, &output);
    audio_stream_del_entry(&default_dac.entry);
#else /*AUDIO_DAC_MULTI_CHANNEL_ENABLE*/
    audio_dac_stop(&dac_hdl);
#endif /*AUDIO_DAC_MULTI_CHANNEL_ENABLE*/
}

#if TONE_DEC_PROTECT_LIST_PLAY
struct tone_dec_list_protect {
    struct audio_res_wait wait;
};
/*----------------------------------------------------------------------------*/
/**@brief    tone链表播放保护用的临时解码资源等待
   @param    *wait: 句柄
   @param    event: 事件
   @return   0：成功
   @note
*/
/*----------------------------------------------------------------------------*/
static int tone_dec_list_protect_res_handler(struct audio_res_wait *wait, int event)
{
    y_printf("tone protect event:%d \n", event);
    return 0;
}
/*----------------------------------------------------------------------------*/
/**@brief    tone链表播放保护用的临时解码释放
   @param    *dec: 解码链表句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void tone_dec_list_protect_release(struct tone_dec_list_handle *dec_list)
{
    if ((!dec_list) || (!dec_list->list_protect)) {
        return ;
    }
    y_printf("tone_dec_list_protect_release \n");
    struct tone_dec_list_protect *prot = dec_list->list_protect;
    audio_decoder_task_del_wait(&decode_task, &prot->wait);
    free(dec_list->list_protect);
    dec_list->list_protect = NULL;
}
/*----------------------------------------------------------------------------*/
/**@brief    tone链表播放保护用的临时解码处理
   @param    *dec_list: 解码链表句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void tone_dec_list_protect_deal(struct tone_dec_list_handle *dec_list)
{
    if ((!dec_list) || (!dec_list->preemption)) {
        return ;
    }
    if ((!dec_list->file_list) || (!dec_list->file_list[1])) {
        return ;
    }
    if (dec_list->list_protect) {
        return ;
    }
    struct tone_dec_list_protect *prot = zalloc(sizeof(struct tone_dec_list_protect));
    if (!prot) {
        return ;
    }
    y_printf("tone_dec_list_protect_deal \n");
    dec_list->list_protect = prot;
#if AUDIO_DEC_TONE_WAIT_USE_PRIO
    prot->wait.priority = 3;
    prot->wait.preemption = 0;
    prot->wait.snatch_same_prio = 1;
#else
    prot->wait.preemption = 1;
#endif
    prot->wait.handler = tone_dec_list_protect_res_handler;
    audio_decoder_task_add_wait(&decode_task, &prot->wait);
}
#endif

/*----------------------------------------------------------------------------*/
/**@brief    获取文件名后缀
   @param    *name: 文件名
   @return   后缀
   @note
*/
/*----------------------------------------------------------------------------*/
static char *get_file_ext_name(char *name)
{
    int len = strlen(name);
    char *ext = (char *)name;

    while (len--) {
        if (*ext++ == '.') {
            break;
        }
    }
    if (len <= 0) {
        ext = name + (strlen(name) - 3);
    }

    return ext;
}

/*----------------------------------------------------------------------------*/
/**@brief    提示音list解码事件
   @param    *dec_list: list句柄
   @param    end_flag: 结束类型
   @return   true: 成功
   @return   false: 成功
   @note
*/
/*----------------------------------------------------------------------------*/
static int tone_dec_list_event_handler(struct tone_dec_list_handle *dec_list, u8 end_flag)
{
    int argv[4];
    if (!dec_list->evt_handler) {
        log_i("evt_handler null\n");
        return false;
    }
    argv[0] = (int)dec_list->evt_handler;
    argv[1] = 2;
    argv[2] = (int)dec_list->evt_priv;
    argv[3] = (int)end_flag; //0正常关闭，1被打断关闭

    int ret = os_taskq_post_type(dec_list->evt_owner, Q_CALLBACK, 4, argv);
    if (ret) {
        return false;
    }
    return true;
}

/*----------------------------------------------------------------------------*/
/**@brief    创建提示音播放句柄
   @param
   @return   提示音句柄
   @note
*/
/*----------------------------------------------------------------------------*/
struct tone_dec_handle *tone_dec_create(void)
{
    struct tone_dec_handle *dec = zalloc(sizeof(struct tone_dec_handle));
    if (!dec) {
        return NULL;
    }
    INIT_LIST_HEAD(&dec->head);
    os_mutex_create(&dec->mutex);
    return dec;
}

/*----------------------------------------------------------------------------*/
/**@brief    设置sine数组获取回调
   @param    *dec: 提示音句柄
   @param    *get_sine: sine数组获取
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void tone_dec_set_sin_get_hdl(struct tone_dec_handle *dec, struct sin_param * (*get_sine)(u8 id, u8 *num))
{
    if (dec) {
        dec->get_sine = get_sine;
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    提示音解码器释放
   @param    *dec: 提示音句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void tone_dec_hdl_release(struct tone_dec_handle *dec)
{
    if (dec->dec_file) {
        audio_dec_file_app_play_end(dec->dec_file);
        dec->dec_file = NULL;
    }
    if (dec->dec_sin) {
        audio_dec_sine_app_play_end(dec->dec_sin);
        dec->dec_sin = NULL;
    }

    clock_remove_set(DEC_TONE_CLK);
}

/*----------------------------------------------------------------------------*/
/**@brief    提示音list释放
   @param    *dec_list: list句柄
   @param    push_event: 普通提示音是否推送消息
   @param    end_flag: 结束类型
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void tone_dec_list_release(struct tone_dec_list_handle *dec_list, u8 push_event, u8 end_flag)
{
    if (dec_list == NULL) {
        return ;
    }
    if (push_event) {
        tone_dec_list_event_handler(dec_list, end_flag);
    }
#if TONE_DEC_PROTECT_LIST_PLAY
    tone_dec_list_protect_release(dec_list);
#endif
    list_del(&dec_list->list_entry);
    if (dec_list->file_list) {
        free(dec_list->file_list);
    }
    free(dec_list);
}

/*----------------------------------------------------------------------------*/
/**@brief    提示音解码结束处理
   @param    *dec: 提示音句柄
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
static void tone_dec_end_ctrl(struct tone_dec_handle *dec)
{
    os_mutex_pend(&dec->mutex, 0);
    // 检查循环播放
    int ret = tone_dec_list_play(dec, 1);
    if (ret == true) {
        os_mutex_post(&dec->mutex);
        return ;
    }
    // 发送播放完成
    tone_dec_list_release(dec->cur_list, 1, TONE_DEC_STOP_NOR);
    dec->cur_list = NULL;

    // 检查链表播放
    struct list_head *list_entry = dec->head.next;
    if (list_entry && (list_entry != &dec->head)) {
        struct tone_dec_list_handle *dec_list = container_of(list_entry, struct tone_dec_list_handle, list_entry);
        dec->cur_list = dec_list;
        tone_dec_list_play(dec, 0);
    }
    os_mutex_post(&dec->mutex);
}

/*----------------------------------------------------------------------------*/
/**@brief    file提示音解码回调
   @param    *priv: 私有句柄
   @param    event: 事件
   @param    *param: 事件参数
   @return   0: 成功
   @note
*/
/*----------------------------------------------------------------------------*/
static int tone_dec_file_app_evt_cb(void *priv, int event, int *param)
{
    /* log_i("audio_dec_file_app_evt_cb, priv:0x%x, event:%d \n", priv, event); */

    struct audio_dec_stream_entries_hdl *entries_hdl = (struct audio_dec_stream_entries_hdl *)param;
    void *dvol_entry = NULL;

    int ret ;
    struct audio_dec_file_app_hdl *file_dec = priv;
    struct tone_dec_handle *dec = file_dec->priv;
    switch (event) {
    case AUDIO_DEC_APP_EVENT_DEC_PROBE:
        break;
    case AUDIO_DEC_APP_EVENT_DEC_OUTPUT:
        // param[0]:*data. param[1]:data_len
        break;
    case AUDIO_DEC_APP_EVENT_DEC_POST:
        break;
    case AUDIO_DEC_APP_EVENT_START_INIT_OK:
        log_i("tone_file start init ok\n");
        if (dec->cur_list->stream_handler) {
            // 删除原有的数据流，需要在回调中重新设置
            if (file_dec->dec->stream) {
                audio_stream_del_entry(&file_dec->dec->mix_ch.entry);
                audio_stream_del_entry(&file_dec->dec->decoder.entry);
                audio_stream_close(file_dec->dec->stream);
                file_dec->dec->stream = NULL;
            }
            dec->cur_list->stream_handler(dec->cur_list->stream_priv, event, file_dec->dec);
        }

        clock_add_set(DEC_TONE_CLK);
        audio_dec_file_app_init_ok(file_dec);
        break;
    case AUDIO_DEC_APP_EVENT_DEC_CLOSE:
        if (!audio_dec_app_mix_en) {
            tone_dec_stream_run_stop(file_dec->dec);
        }
        if (dec->cur_list->stream_handler) {
            dec->cur_list->stream_handler(dec->cur_list->stream_priv, event, file_dec->dec);
        }
        break;
    case AUDIO_DEC_APP_EVENT_START_OK:
        dec->cur_list->dec_ok_cnt++;
        break;
    case AUDIO_DEC_APP_EVENT_START_ERR:
        log_i("tone_file start err\n");
    case AUDIO_DEC_APP_EVENT_PLAY_END:
        log_i("tone_file play end\n");
        tone_dec_end_ctrl(dec);
        break;
    case AUDIO_DEC_APP_EVENT_STREAM_OPEN:
#if SYS_DIGVOL_GROUP_EN
        if (dec->cur_list->stream_handler == NULL) {
            dvol_entry = sys_digvol_group_ch_open("tone_tone", -1, NULL);
            (entries_hdl->entries_addr)[entries_hdl->entries_cnt++] = dvol_entry;
        }
#endif // SYS_DIGVOL_GROUP_EN

        break;
    case AUDIO_DEC_APP_EVENT_STREAM_CLOSE:
#if SYS_DIGVOL_GROUP_EN
        if (dec->cur_list->stream_handler == NULL) {
            sys_digvol_group_ch_close("tone_tone");
        }
#endif // SYS_DIGVOL_GROUP_EN

        break;
    }
    return 0;
}

/*----------------------------------------------------------------------------*/
/**@brief    sine提示音解码回调
   @param    *priv: 私有句柄
   @param    event: 事件
   @param    *param: 事件参数
   @return   0: 成功
   @note
*/
/*----------------------------------------------------------------------------*/
static int tone_dec_sine_app_evt_cb(void *priv, int event, int *param)
{
    /* log_i("audio_dec_sine_app_evt_cb, priv:0x%x, event:%d \n", priv, event); */
    struct audio_dec_sine_app_hdl *sine_dec = priv;
    struct tone_dec_handle *dec = sine_dec->priv;
    switch (event) {
    case AUDIO_DEC_APP_EVENT_DEC_PROBE:
        if (sine_dec->sin_maker) {
            break;
        }
        audio_dec_sine_app_probe(sine_dec);
        if (!sine_dec->sin_maker) {
            return -ENOENT;
        }
        break;
    case AUDIO_DEC_APP_EVENT_START_INIT_OK:
        log_i("tone_sine start init ok\n");
        if (dec->cur_list->stream_handler) {
            // 删除原有的数据流，需要在回调中重新设置
            if (sine_dec->dec->stream) {
                audio_stream_del_entry(&sine_dec->dec->mix_ch.entry);
                audio_stream_del_entry(&sine_dec->dec->decoder.entry);
                audio_stream_close(sine_dec->dec->stream);
                sine_dec->dec->stream = NULL;
            }
            dec->cur_list->stream_handler(dec->cur_list->stream_priv, event, sine_dec->dec);
        }
        clock_add_set(DEC_TONE_CLK);
        audio_dec_sine_app_init_ok(sine_dec);
        break;
    case AUDIO_DEC_APP_EVENT_DEC_CLOSE:
        if (!audio_dec_app_mix_en) {
            tone_dec_stream_run_stop(sine_dec->dec);
        }
        if (dec->cur_list->stream_handler) {
            dec->cur_list->stream_handler(dec->cur_list->stream_priv, event, sine_dec->dec);
        }
        break;
    case AUDIO_DEC_APP_EVENT_START_OK:
        dec->cur_list->dec_ok_cnt++;
        break;
    case AUDIO_DEC_APP_EVENT_START_ERR:
        log_i("tone_file start err\n");
    case AUDIO_DEC_APP_EVENT_PLAY_END:
        log_i("tone_sine play end\n");
        /* audio_dec_sine_app_play_end(sine_dec); */
        tone_dec_end_ctrl(dec);
        break;
    }
    return 0;
}

static void _tone_dec_app_comm_deal(struct audio_dec_app_hdl *dec, struct tone_dec_handle *tone)
{
    dec->sync_confirm_time = tone->cur_list->sync_confirm_time;
    if (dec->dec_type == AUDIO_CODING_SBC) {
        audio_dec_app_set_frame_info(dec, 0x4e, dec->dec_type);
    }
#if AUDIO_DEC_TONE_WAIT_USE_PRIO
    if (dec->dec_mix == 0) {
        dec->wait.priority = 3;
        dec->wait.preemption = 0;
        dec->wait.snatch_same_prio = 1;
    }
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief    提示音list播放
   @param    *dec: 句柄
   @param    next: 播放下一个
   @return   true: 成功
   @return   false: 成功
   @note
*/
/*----------------------------------------------------------------------------*/
static int tone_dec_list_play(struct tone_dec_handle *dec, u8 next)
{
    struct tone_dec_list_handle *dec_list = dec->cur_list;
    if (!dec_list) {
        return false;
    }
#if TONE_DEC_PROTECT_LIST_PLAY
    tone_dec_list_protect_deal(dec_list);
#endif

    tone_dec_hdl_release(dec);

    if (next) {
        dec_list->idx++;
    }
    if (IS_REPEAT_END(dec_list->file_list[dec_list->idx])) {
        //log_i("repeat_loop:%d",dec_list->loop);
        if (!dec_list->dec_ok_cnt) {
            log_i("repeat dec err \n");
            return false;
        }
        if (dec_list->loop) {
            dec_list->loop--;
            dec_list->idx = dec_list->repeat_begin;
        } else {
            dec_list->idx++;
            if (!dec_list->file_list[dec_list->idx]) {
                log_i("repeat end 2:idx end");
                return false;
            }
        }
    }

    if (IS_REPEAT_BEGIN(dec_list->file_list[dec_list->idx])) {
        if (dec_list->loop == 0) {
            dec_list->loop = TONE_REPEAT_COUNT(dec_list->file_list[dec_list->idx]);
            log_i("repeat begin:%d", dec_list->loop);
        }
        dec_list->idx++;
        dec_list->repeat_begin = dec_list->idx;
        dec_list->dec_ok_cnt = 0;
    }

    if (dec_list->file_list[dec_list->idx] == NULL) {
        return false;
    }

    if (IS_DEFAULT_SINE(dec_list->file_list[dec_list->idx])) {
        // is sine idx
        if (dec->get_sine == NULL) {
            log_e("get_sine is NULL ");
            return false;
        }
        u8 num = 0;
        struct sin_param *sin = dec->get_sine(DEFAULT_SINE_ID(dec_list->file_list[dec_list->idx]), &num);
        if (!sin) {
            log_e("sine is NULL ");
            return false;
        }
        dec->dec_sin = audio_dec_sine_app_create_by_parm(sin, num, !dec_list->preemption);
        if (dec->dec_sin == NULL) {
            return false;
        }
        _tone_dec_app_comm_deal(dec->dec_sin->dec, dec);
        dec->dec_sin->dec->evt_cb = tone_dec_sine_app_evt_cb;
        dec->dec_sin->priv = dec;
        audio_dec_sine_app_open(dec->dec_sin);
        return true;
    }
    u8 file_name[16];
    char *format = NULL;
    FILE *file = fopen(dec_list->file_list[dec_list->idx], "r");
    if (!file) {
        return false;
    }
    fget_name(file, file_name, 16);
    format = get_file_ext_name((char *)file_name);
    fclose(file);
    if (ASCII_StrCmpNoCase(format, "sin", 3) == 0) {
        // is sine file
        if (file) {
            fclose(file);
        }
        dec->dec_sin = audio_dec_sine_app_create(dec_list->file_list[dec_list->idx], !dec_list->preemption);
        if (dec->dec_sin == NULL) {
            return false;
        }
        _tone_dec_app_comm_deal(dec->dec_sin->dec, dec);
        dec->dec_sin->dec->evt_cb = tone_dec_sine_app_evt_cb;
        dec->dec_sin->priv = dec;
        audio_dec_sine_app_open(dec->dec_sin);
        return true;
    }

    // is file
    dec->dec_file = audio_dec_file_app_create(dec_list->file_list[dec_list->idx], !dec_list->preemption);
    if (dec->dec_file == NULL) {
        return false;
    }
    _tone_dec_app_comm_deal(dec->dec_file->dec, dec);
    dec->dec_file->dec->evt_cb = tone_dec_file_app_evt_cb;
    dec->dec_file->priv = dec;
    audio_dec_file_app_open(dec->dec_file);
    return true;
}


/*----------------------------------------------------------------------------*/
/**@brief    创建提示音播放list句柄
   @param    *dec: 提示音句柄
   @param    **file_list: 文件名
   @param    preemption: 打断标记
   @param    *evt_handler: 事件回调接口
   @param    *evt_priv: 事件回调私有句柄
   @param    *stream_handler: tone数据流设置回调
   @param    *stream_priv: tone数据流设置回调私有句柄
   @return   list句柄
   @note
*/
/*----------------------------------------------------------------------------*/
struct tone_dec_list_handle *tone_dec_list_create(struct tone_dec_handle *dec,
        const char **file_list,
        u8 preemption,
        void (*evt_handler)(void *priv, int flag),
        void *evt_priv,
        void (*stream_handler)(void *priv, int event, struct audio_dec_app_hdl *app_dec),
        void *stream_priv)
{
    if (!dec || !file_list) {
        return NULL;
    }
    struct tone_dec_list_handle *dec_list = zalloc(sizeof(struct tone_dec_list_handle));
    if (!dec_list) {
        return NULL;
    }

    int i = 0;
    while (file_list[i] != NULL) {
        i++;
    }
    dec_list->file_list = malloc(4 * (i + 1));
    if (!dec_list->file_list) {
        free(dec_list);
        return NULL;
    }
    memcpy(dec_list->file_list, file_list, 4 * (i + 1));
    dec_list->preemption = preemption;
    dec_list->evt_handler = evt_handler;
    dec_list->evt_priv = evt_priv;
    dec_list->evt_owner = os_current_task();
    dec_list->stream_handler = stream_handler;
    dec_list->stream_priv = stream_priv;
    return dec_list;
}

/*----------------------------------------------------------------------------*/
/**@brief    提示音list开始播放
   @param    *dec: 提示音句柄
   @param    *dec_list: list句柄
   @return   true: 成功
   @return   false: 成功
   @note     当前没有播放，马上开始播放。当前有播放，挂载到链表后面等待播放
*/
/*----------------------------------------------------------------------------*/
int tone_dec_list_add_play(struct tone_dec_handle *dec, struct tone_dec_list_handle *dec_list)
{
    int ret = false;
    if (!dec || !dec_list) {
        return false;
    }

    os_mutex_pend(&dec->mutex, 0);

    list_add_tail(&dec_list->list_entry, &dec->head);

    if (dec->cur_list == NULL) {
        // 当前没有播放，开始播放
        dec->cur_list = dec_list;
        ret = tone_dec_list_play(dec, 0);
    } else {
        // 当前有播放，等播放完自动播放
        ret = true;
    }

    os_mutex_post(&dec->mutex);

    return ret;
}


/*----------------------------------------------------------------------------*/
/**@brief    提示音播放停止
   @param    **ppdec: 提示音句柄
   @param    push_event: 普通提示音是否推送消息
   @param    end_flag: 结束类型
   @return
   @note
*/
/*----------------------------------------------------------------------------*/
void tone_dec_stop(struct tone_dec_handle **ppdec,
                   u8 push_event,
                   u8 end_flag)
{
    if (!ppdec || !*ppdec) {
        return ;
    }
    struct tone_dec_handle *dec = *ppdec;
    struct tone_dec_list_handle *p, *n;

    os_mutex_pend(&dec->mutex, 0);

    tone_dec_hdl_release(dec);

    list_for_each_entry_safe(p, n, &dec->head, list_entry) {
        tone_dec_list_release(p, push_event, end_flag);
    }

    os_mutex_post(&dec->mutex);

    free(dec);

    *ppdec = NULL;
}


/*----------------------------------------------------------------------------*/
/**@brief    指定提示音播放停止
   @param    **ppdec: 提示音句柄
   @param    push_event: 普通提示音是否推送消息
   @param    end_flag: 结束类型
   @return
   @note     如果该提示音正在播，停止播放并且播放下一个。如果不在播放，只从链表中删除
*/
/*----------------------------------------------------------------------------*/
void tone_dec_stop_spec_file(struct tone_dec_handle **ppdec,
                             char *file_name,
                             u8 push_event,
                             u8 end_flag)
{
    if (!ppdec || !*ppdec || !file_name) {
        return ;
    }
    struct tone_dec_handle *dec = *ppdec;
    struct tone_dec_list_handle *p, *n;

    os_mutex_pend(&dec->mutex, 0);
    if (dec->cur_list) {
        do {
            if (IS_DEFAULT_SINE(file_name)) {
                if (dec->cur_list->file_list[0] != file_name) {
                    break;
                }
            } else if (IS_DEFAULT_SINE(dec->cur_list->file_list[0])) {
                break;
            } else if (strcmp(dec->cur_list->file_list[0], file_name)) {
                break;
            }
            // 当前正在播放，停止播放
            tone_dec_hdl_release(dec);
            tone_dec_list_release(dec->cur_list, push_event, end_flag);
            dec->cur_list = NULL;
            // 检查链表播放
            struct list_head *list_entry = dec->head.next;
            if (list_entry && (list_entry != &dec->head)) {
                // 如果还有，则继续播放
                struct tone_dec_list_handle *dec_list = container_of(list_entry, struct tone_dec_list_handle, list_entry);
                dec->cur_list = dec_list;
                tone_dec_list_play(dec, 0);
                os_mutex_post(&dec->mutex);
            } else {
                // 如果没有，释放
                os_mutex_post(&dec->mutex);
                free(dec);
                *ppdec = NULL;
            }
            return ;
        } while (0);
    }

    // 从链表中删除，不停止解码
    list_for_each_entry_safe(p, n, &dec->head, list_entry) {
        do {
            if (IS_DEFAULT_SINE(file_name)) {
                if (dec->cur_list->file_list[0] != file_name) {
                    break;
                }
            } else if (IS_DEFAULT_SINE(dec->cur_list->file_list[0])) {
                break;
            } else if (strcmp(dec->cur_list->file_list[0], file_name)) {
                break;
            }
            tone_dec_list_release(p, push_event, end_flag);
            os_mutex_post(&dec->mutex);
            return ;
        } while (0);
    }

    os_mutex_post(&dec->mutex);
}


#if 0
static struct tone_dec_handle *test_tone_dec = NULL;
static struct audio_eq_drc *test_tone_eq_drc = NULL;

extern void *file_eq_drc_open(u16 sample_rate, u8 ch_num);
extern void file_eq_drc_close(struct audio_eq_drc *eq_drc);

static void tone_test_stream_resume(void *p)
{
    struct audio_dec_app_hdl *app_dec = p;
    audio_decoder_resume(&app_dec->decoder);
}

static void tone_test_stream_handler(void *priv, int event, struct audio_dec_app_hdl *app_dec)
{
    switch (event) {
    case AUDIO_DEC_APP_EVENT_START_INIT_OK:
        y_printf("AUDIO_DEC_APP_EVENT_START_INIT_OK \n");
        struct audio_stream_entry *entries[8] = {NULL};
        u8 entry_cnt = 0;
        entries[entry_cnt++] = &app_dec->decoder.entry;
        {
            // add eq
            test_tone_eq_drc = file_eq_drc_open(app_dec->sample_rate, app_dec->ch_num);
            entries[entry_cnt++] = &test_tone_eq_drc->entry;
        }

        entries[entry_cnt++] = &app_dec->mix_ch.entry;
        app_dec->stream = audio_stream_open(app_dec, tone_test_stream_resume);
        audio_stream_add_list(app_dec->stream, entries, entry_cnt);
        break;
    case AUDIO_DEC_APP_EVENT_DEC_CLOSE:
        y_printf("AUDIO_DEC_APP_EVENT_DEC_CLOSE \n");
        {
            // del eq
            if (test_tone_eq_drc) {
                file_eq_drc_close(test_tone_eq_drc);
                test_tone_eq_drc = NULL;
            }
        }
        break;
    }
}

static void tone_test_play_close(void)
{
    tone_dec_stop(&test_tone_dec, 0, 0);
}

void tone_test_play(void)
{
    tone_test_play_close();
    test_tone_dec = tone_dec_create();
    static char *single_file[2] = {NULL};
    single_file[0] = (char *)TONE_POWER_OFF;
    single_file[1] = NULL;
    struct tone_dec_list_handle *dec_list = tone_dec_list_create(test_tone_dec, single_file, 1, NULL, NULL, tone_test_stream_handler, NULL);
    tone_dec_list_add_play(test_tone_dec, dec_list);
}
#endif


