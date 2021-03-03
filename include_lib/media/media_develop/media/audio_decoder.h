#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#include "generic/typedef.h"
#include "generic/list.h"
#include "media/audio_base.h"
#include "media/audio_stream.h"
#include "media/prevent_task_fill.h"
#include "os/os_api.h"



enum {
    AUDIO_DEC_EVENT_CURR_TIME = 0x20,
    AUDIO_DEC_EVENT_END,
    AUDIO_DEC_EVENT_ERR,
    AUDIO_DEC_EVENT_SUSPEND,
    AUDIO_DEC_EVENT_RESUME,
    AUDIO_DEC_EVENT_START,
};

enum {
    AUDIO_PLAY_EVENT_CURR_TIME = 0x20,
    AUDIO_PLAY_EVENT_END,
    AUDIO_PLAY_EVENT_ERR,
    AUDIO_PLAY_EVENT_SUSPEND,
    AUDIO_PLAY_EVENT_RESUME,
};

enum {
    AUDIO_RES_GET,
    AUDIO_RES_PUT,
};

enum {
    DEC_STA_TRY_START,
    DEC_STA_START,
    DEC_STA_WAIT_STOP,
    DEC_STA_WAIT_SUSPEND,
    DEC_STA_WAIT_RESUME,
    DEC_STA_WAIT_PAUSE,
};

enum {
    AB_REPEAT_MODE_BP_A = 0x01,
    AB_REPEAT_MODE_BP_B,
    AB_REPEAT_MODE_CUR,
};

enum {
    AUDIO_IOCTRL_CMD_SET_BREAKPOINT_A = 0x08,
    AUDIO_IOCTRL_CMD_SET_BREAKPOINT_B,
    AUDIO_IOCTRL_CMD_SET_BREAKPOINT_MODE,

    AUDIO_IOCTRL_CMD_REPEAT_PLAY = 0x90,
};

struct audio_ab_repeat_mode_param {
    u32 value;
};

struct fixphase_repair_obj {
    short fifo_buf[18 + 12][32][2];
};

struct audio_repeat_mode_param {
    int flag;
    int headcut_frame;
    int tailcut_frame;
    int (*repeat_callback)(void *);
    void *callback_priv;
    struct fixphase_repair_obj *repair_buf;
};

struct audio_res_wait {
    struct list_head list_entry;	// 链表。用于多个解码抢占/排序等
    u8 priority;		// 优先级
    u8 preemption : 1;	// 抢占
    u8 protect : 1;		// 保护（叠加）
    u8 only_del : 1; 	// 仅删除
    u8 snatch_same_prio : 1; // 放在同优先级的前面
    u8 is_work : 1; 	// 已经运行
    u32 format;			// 格式锁
    int (*handler)(struct audio_res_wait *, int event);
};

struct audio_decoder_task {
    struct list_head head;	// 用于解码任务中的轮询处理
    struct list_head wait;	// 用于多个解码抢占/排序等
    struct prevent_task_fill_ch *prevent_fill;	// 防止解码任务一直占满cpu
    const char *name;	// 解码任务名称
    int wakeup_timer;	// 定时唤醒
    int fmt_lock;		// 格式锁
    u16 is_add_wait : 1;	// 正在添加res资源
    u16 step_cnt : 8;
    u16 step_timer;
    void *out_task;		// 使用单独任务做输出
};


struct audio_dec_breakpoint {
    int len;
    u32 fptr;
    int data_len;
    u8 data[0];
    // u8 data[128];
};

struct audio_decoder;


struct audio_dec_input {
    u32 coding_type;
    // 定义在p_more_coding_type数组中的解码器会依照顺序依次检测
    // 先检测coding_type再检测p_more_coding_type
    u32 *p_more_coding_type;
    u32 data_type : 8;
    union {
        struct { // data_type == AUDIO_INPUT_FILE
            int (*fread)(struct audio_decoder *, void *buf, u32 len);
            int (*fseek)(struct audio_decoder *, u32 offset, int seek_mode);
            int (*ftell)(struct audio_decoder *);
            int (*flen)(struct audio_decoder *);
        } file;

        struct { // data_type == AUDIO_INPUT_FRAME
            int (*fget)(struct audio_decoder *, u8 **frame);
            void (*fput)(struct audio_decoder *, u8 *frame);
            int (*ffetch)(struct audio_decoder *, u8 **frame);
        } frame;
    } ops;
};

struct audio_dec_handler {
    int (*dec_probe)(struct audio_decoder *);	// 预处理
    // 解码输出。解码会自动输出到数据流中，上层不必要实现该函数
    int (*dec_output)(struct audio_decoder *, s16 *data, int len, void *priv);
    int (*dec_post)(struct audio_decoder *);	// 后处理
    int (*dec_stop)(struct audio_decoder *);	// 解码结束
};
struct stream_codec_info {
    int  time;
    int  frame_num;
    u32  frame_len;
    int  frame_points;
    int  sequence_number;
    u32  sample_rate;
    u8   channel;
};
struct audio_decoder_ops {
    u32 coding_type;
    void *(*open)(void *priv);
    int (*start)(void *);
    int (*get_fmt)(void *, struct audio_fmt *fmt);
    int (*set_output_channel)(void *, enum audio_channel);
    int (*get_play_time)(void *);
    int (*fast_forward)(void *, int step_s);
    int (*fast_rewind)(void *, int step_s);
    int (*get_breakpoint)(void *, struct audio_dec_breakpoint *);
    int (*set_breakpoint)(void *, struct audio_dec_breakpoint *);
    int (*stream_info_scan)(void *, struct stream_codec_info *info, void *data, int len);
    int (*set_tws_mode)(void *, int);
    int (*run)(void *, u8 *);
    int (*stop)(void *);
    int (*close)(void *);
    int (*reset)(void *);
    int (*ioctrl)(void *, u32 cmd, void *parm);
};

#define REGISTER_AUDIO_DECODER(ops) \
        const struct audio_decoder_ops ops SEC(.audio_decoder)

extern const struct audio_decoder_ops audio_decoder_begin[];
extern const struct audio_decoder_ops audio_decoder_end[];

#define list_for_each_audio_decoder(p) \
    for (p = audio_decoder_begin; p < audio_decoder_end; p++)




struct audio_decoder {
    struct list_head list_entry;		// 链表。用于解码任务中轮询处理
    struct audio_decoder_task *task;	// 解码任务
    struct audio_fmt fmt;				// 解码格式
    const char *evt_owner;				// 用于接受消息的任务名称
    const struct audio_dec_input *input;			// 解码输入接口
    const struct audio_decoder_ops *dec_ops;		// 解码器接口
    const struct audio_dec_handler *dec_handler;	// 解码处理回调
    void (*evt_handler)(struct audio_decoder *dec, int, int *);	// 事件回调
    void *dec_priv;		// 解码器对应句柄
    void *bp;			// 断点
    // u16 id;			// ID号
    u16 pick : 1;		// 本地拆包解码标记
    u16 tws : 1;		// 本地tws解码标记
    u16 output_err : 1;	// 解码输出错误
    u16 read_err : 1;	// 解码读取错误
    u16 open_step : 4;
    u8 run_max;			// 正常解码最大次数
    // u8 output_channel;	// 输出通道
    u8 state;			// 解码状态
    u8 err;				// 解码结束错误类型标记
    u8 remain;			// 解码输出完成标记
    volatile u8 resume_flag;// 解码激活标记
    u32 magic;			// 事件回调的私有标记
    u32 process_len;	// 数据流处理长度
    struct audio_stream_entry entry;	// 音频流入口
    void *out_task_ch;	// 使用单独任务做输出
};

#define AUDIO_DEC_ORIG_CH       AUDIO_CH_LR
#define AUDIO_DEC_L_CH          AUDIO_CH_L
#define AUDIO_DEC_R_CH          AUDIO_CH_R
#define AUDIO_DEC_MONO_LR_CH    AUDIO_CH_DIFF
#define AUDIO_DEC_DUAL_L_CH     AUDIO_CH_DUAL_L
#define AUDIO_DEC_DUAL_R_CH     AUDIO_CH_DUAL_R
#define AUDIO_DEC_DUAL_LR_CH    AUDIO_CH_DUAL_LR

#define AUDIO_DEC_IS_MONO(ch)	(((ch)==AUDIO_DEC_L_CH) || ((ch)==AUDIO_DEC_R_CH) || ((ch)==AUDIO_DEC_MONO_LR_CH))

// 创建解码任务
int audio_decoder_task_create(struct audio_decoder_task *task, const char *name);

// 解码任务增加解码通道资源（加入链表后，按配置信息播放）
int audio_decoder_task_add_wait(struct audio_decoder_task *, struct audio_res_wait *);
// 解码任务删除解码通道资源
void audio_decoder_task_del_wait(struct audio_decoder_task *, struct audio_res_wait *);
// 获取解码任务链表中的资源数量
int audio_decoder_task_wait_state(struct audio_decoder_task *task);

// 激活解码任务链表中的所有解码
int audio_decoder_resume_all(struct audio_decoder_task *task);

// 解码任务资源添加格式锁
int audio_decoder_fmt_lock(struct audio_decoder_task *task, int fmt);
// 解码任务资源格式锁解锁
int audio_decoder_fmt_unlock(struct audio_decoder_task *task, int fmt);

// void *audio_decoder_get_output_buff(void *_dec, int *len);

// 解码输出
int audio_decoder_put_output_buff(void *_dec, void *buff, int len, void *priv);

// 解码读数
int audio_decoder_read_data(void *_dec, u8 *data, int len, u32 offset);

// 获取解码文件长度（仅文件解码可用）
int audio_decoder_get_input_data_len(void *_dec);

// 获取流数据帧内容（仅流数据解码可用）
int audio_decoder_get_frame(void *_dec, u8 **frame);

// 检查流数据帧内容（仅流数据解码可用）
int audio_decoder_fetch_frame(void *_dec, u8 **frame);

// 流数据帧使用完（仅流数据解码可用）
void audio_decoder_put_frame(void *_dec, u8 *frame);

// 打开一个解码通道
int audio_decoder_open(struct audio_decoder *dec, const struct audio_dec_input *input,
                       struct audio_decoder_task *task);

// 获取配置的解码类型
int audio_decoder_data_type(void *_dec);

// 获取解码器格式信息（解码器没有打开时，尝试打开解码器）
int audio_decoder_get_fmt(struct audio_decoder *dec, struct audio_fmt **fmt);

// 设置解码的格式信息
int audio_decoder_set_fmt(struct audio_decoder *dec, struct audio_fmt *fmt);

// 实时获取解码器格式信息
int audio_decoder_get_fmt_info(struct audio_decoder *dec, struct audio_fmt *fmt);

// 设置解码处理handler
void audio_decoder_set_handler(struct audio_decoder *dec, const struct audio_dec_handler *handler);


// 设置解码事件回调
void audio_decoder_set_event_handler(struct audio_decoder *dec,
                                     void (*handler)(struct audio_decoder *, int, int *), u32 magic);

// void audio_decoder_set_input_buff(struct audio_decoder *dec, u8 *buff, u16 buff_size);

// void audio_decoder_set_output_buffs(struct audio_decoder *dec, s16 *buffs,
// u16 buff_size, u8 buff_num);

// 设置解码输出声道类型
int audio_decoder_set_output_channel(struct audio_decoder *dec, enum audio_channel);

// 启动解码
int audio_decoder_start(struct audio_decoder *dec);

// 停止解码
int audio_decoder_stop(struct audio_decoder *dec);

// 解码暂停
int audio_decoder_pause(struct audio_decoder *dec);

// 解码挂起
int audio_decoder_suspend(struct audio_decoder *dec);

// 解码挂起激活
int audio_decoder_resume(struct audio_decoder *dec);

// 关闭解码
int audio_decoder_close(struct audio_decoder *dec);
// 解码器reset
int audio_decoder_reset(struct audio_decoder *dec);

// 设置解码断点句柄
void audio_decoder_set_breakpoint(struct audio_decoder *dec, struct audio_dec_breakpoint *bp);

// 获取解码断点信息
int audio_decoder_get_breakpoint(struct audio_decoder *dec, struct audio_dec_breakpoint *bp);

// 解码快进
int audio_decoder_forward(struct audio_decoder *dec, int step_s);

// 解码快退
int audio_decoder_rewind(struct audio_decoder *dec, int step_s);

// 获取解码总时间
int audio_decoder_get_total_time(struct audio_decoder *dec);
// 获取解码当前时间
int audio_decoder_get_play_time(struct audio_decoder *dec);

// 设置解码拆包模式
void audio_decoder_set_pick_stu(struct audio_decoder *dec, u8 pick);
// 获取解码拆包模式
int audio_decoder_get_pick_stu(struct audio_decoder *dec);

// 设置解码tws模式
void audio_decoder_set_tws_stu(struct audio_decoder *dec, u8 tws);
// 获取解码tws模式
int audio_decoder_get_tws_stu(struct audio_decoder *dec);

// 设置解码每次轮询可执行的最大次数
void audio_decoder_set_run_max(struct audio_decoder *dec, u8 run_max);

// 双声道转换为其他声道
void audio_decoder_dual_switch(u8 ch_type, u8 half_lr, s16 *data, int len);

// 统计正在运行的解码数
int audio_decoder_running_number(struct audio_decoder_task *task);

// 解码器控制
int audio_decoder_ioctrl(struct audio_decoder *dec, u32 cmd, void *parm);


// 创建解码输出任务
int audio_decoder_out_task_create(struct audio_decoder_task *task, const char *out_task_name);
// 激活解码输出任务。0: 成功
int audio_decoder_out_task_ch_enable(struct audio_decoder *dec);
// 激活解码输出任务
void audio_decoder_resume_out_task(struct audio_decoder *dec);


#endif

