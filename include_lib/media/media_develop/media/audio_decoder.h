#ifndef AUDIO_DECODER_H
#define AUDIO_DECODER_H

#include "generic/typedef.h"
#include "generic/list.h"
#include "media/audio_base.h"
#include "media/audio_stream.h"
#include "os/os_api.h"



// 解码事件
enum {
    AUDIO_DEC_EVENT_CURR_TIME = 0x20,
    AUDIO_DEC_EVENT_END,
    AUDIO_DEC_EVENT_ERR,
    AUDIO_DEC_EVENT_SUSPEND,
    AUDIO_DEC_EVENT_RESUME,
    AUDIO_DEC_EVENT_START,
};

// 资源回调事件
enum {
    AUDIO_RES_GET,
    AUDIO_RES_PUT,
};

// 解码状态
enum {
    DEC_STA_TRY_START,
    DEC_STA_START,
    DEC_STA_WAIT_STOP,
    DEC_STA_WAIT_SUSPEND,
    DEC_STA_WAIT_RESUME,
    DEC_STA_WAIT_PAUSE,
};

// AB点复读模式
enum {
    AB_REPEAT_MODE_BP_A = 0x01,
    AB_REPEAT_MODE_BP_B,
    AB_REPEAT_MODE_CUR,
};

// 解码器命令
enum {
    AUDIO_IOCTRL_CMD_SET_BREAKPOINT_A = 0x08,	// 设置复读A点
    AUDIO_IOCTRL_CMD_SET_BREAKPOINT_B,			// 设置复读B点
    AUDIO_IOCTRL_CMD_SET_BREAKPOINT_MODE,		// 设置AB点复读模式
    AUDIO_IOCTRL_CMD_SET_AB_CALLBACK,           //设置AB点复读回调

    AUDIO_IOCTRL_CMD_REPEAT_PLAY = 0x90,		// 设置循环播放
    AUDIO_IOCTRL_CMD_SET_DEST_PLAYPOS = 0x93,	// 设置指定位置播放
    AUDIO_IOCTRL_CMD_GET_PLAYPOS = 0x94,		// 获取毫秒级时间
};

/*
 * 指定位置播放
 * 设置后跳到start_time开始播放，
 * 播放到dest_time后如果callback_func存在，则调用callback_func，
 * 可以在callback_func回调中实现对应需要的动作
 */
struct audio_dest_time_play_param {
    u32 start_time;	// 要跳转过去播放的起始时间。单位：ms
    u32 dest_time;	// 要跳转过去播放的目标时间。单位：ms
    u32(*callback_func)(void *priv);	// 到达目标时间后回调
    void *callback_priv;	// 回调参数
};

/*
 * 获取毫秒级解码时间
 */
struct audio_get_play_ms_time_param {
    u32 time_ms;
};

/*
 * ab点复读模式结构体
 */
struct audio_ab_repeat_mode_param {
    u32 value;
    int (*callback)(void *priv, int mode);
    void *callback_priv;
};

/*
 * 循环播放fifo
 */
struct fixphase_repair_obj {
    short fifo_buf[18 + 12][32][2];
};

/*
 * 循环播放结构体
 * 每次播放会根据headcut_frame和tailcut_frame来砍掉一些不需要的帧
 * 在repeat_callback()回调中可以自主控制播放次数，返回非0即结束循环
 */
struct audio_repeat_mode_param {
    int flag;
    int headcut_frame;	// 砍掉前面几帧，仅mp3格式有效
    int tailcut_frame;	// 砍掉后面几帧，仅mp3格式有效
    int (*repeat_callback)(void *);	// 循环播放回调，返回0-正常循环；返回非0-结束循环
    void *callback_priv;
    struct fixphase_repair_obj *repair_buf;
};

/*
 *解码任务cpu占用率跟踪:
 *跟踪周期trace_period内，如果解码任务空闲时间统计少于
 *期望值(idle_expect),就会触发饱和保护机制。默认是挂起自
 *身pend_time.如果想要自己做处理，只需注册trace_hdl即可
 */
struct audio_decoder_occupy {
    u8 pend_time;		//挂起时间,os_time_dly(pend_time)
    u16 idle_expect;	//跟踪周期内期望idle时间
    u16 trace_period;	//跟踪周期，单位：ms
    int (*trace_hdl)(void *priv, u32 idle_total); //跟踪回调函数
};

/*
 * 资源处理结构体
 * res会先挂载到链表中，然后依据以下规律依次执行：
 * protect置1时，马上开始播放（叠加播放），不会结束其他的res
 * preemption置1时，马上开始播放（抢占播放），会结束当前非protect模式的res
 * protect和preemption为0时，依据priority优先级播放
 * 按优先级排序的情况下，由snatch_same_prio决定是否放在同优先级的前面
 * preemption方式和priority优先级方式，同一时刻只会有一个解码
 * preemption方式的全部播放完后才开始播放按priority优先级排序的
 */
struct audio_res_wait {
    struct list_head list_entry;	// 链表。用于多个解码抢占/排序等
    u8 priority;		// 优先级
    u8 preemption : 1;	// 抢占
    u8 protect : 1;		// 保护（叠加）
    u8 only_del : 1; 	// 仅删除
    u8 snatch_same_prio : 1; // 放在同优先级的前面
    u8 is_work : 1; 	// 已经运行
    u32 format;			// 格式锁
    int (*handler)(struct audio_res_wait *, int event); // 回调
};

/*
 * 解码任务结构体
 * 链接所有的解码，按list链表循环执行
 */
struct audio_decoder_task {
    struct list_head head;	// 用于解码任务中的轮询处理
    struct list_head wait;	// 用于多个解码抢占/排序等
    struct audio_decoder_occupy occupy;//
    const char *name;	// 解码任务名称
    int wakeup_timer;	// 定时唤醒
    int fmt_lock;		// 格式锁
    u16 is_add_wait : 1;	// 正在添加res资源
    u16 step_cnt : 8;
    u16 step_timer;
    void *out_task;		// 使用单独任务做输出
};


/*
 * 解码断点结构体
 */
struct audio_dec_breakpoint {
    int len;
    u32 fptr;
    int data_len;
    u8 data[0];
    // u8 data[128];
};

struct audio_decoder;


/*
 * 解码输入接口
 * 用于获取音频数据，分为音频文件和音频流
 */
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

/*
 * 解码处理过程中的回调
 */
struct audio_dec_handler {
    int (*dec_probe)(struct audio_decoder *);	// 预处理。返回非0值不跑后续的解码run
    // 解码输出。解码会自动输出到数据流中，上层不必要实现该函数
    int (*dec_output)(struct audio_decoder *, s16 *data, int len, void *priv);
    int (*dec_post)(struct audio_decoder *);	// 后处理
    int (*dec_stop)(struct audio_decoder *);	// 解码结束
};

/*
 * 数据流解码信息
 */
struct stream_codec_info {
    int  time;
    int  frame_num;
    u32  frame_len;
    int  frame_points;
    int  sequence_number;
    u32  sample_rate;
    u8   channel;
};

/*
 * 解码器结构体
 * 所有挂载到解码的解码器都需要该结构体
 * 至少需要实现open/start/run/close等函数
 */
struct audio_decoder_ops {
    u32 coding_type;
    void *(*open)(void *priv);	// 打开解码器，获取句柄
    int (*start)(void *);		// 启动解码
    int (*get_fmt)(void *, struct audio_fmt *fmt);			// 获取解码信息
    int (*set_output_channel)(void *, enum audio_channel);	// 设置输出声道
    int (*get_play_time)(void *);				// 获取当前解码时间
    int (*fast_forward)(void *, int step_s);	// 快进，step单位：秒
    int (*fast_rewind)(void *, int step_s);		// 快退，step单位：秒
    int (*get_breakpoint)(void *, struct audio_dec_breakpoint *);	// 获取断点信息
    int (*set_breakpoint)(void *, struct audio_dec_breakpoint *);	// 设置断点buf
    int (*stream_info_scan)(void *, struct stream_codec_info *info, void *data, int len);	// 数据流信息检测
    int (*set_tws_mode)(void *, int);			// 设置tws解码模式
    int (*run)(void *, u8 *);					// 运行解码
    int (*stop)(void *);						// 停止解码
    int (*close)(void *);						// 关闭解码，释放空间
    int (*reset)(void *);						// 重启解码
    int (*ioctrl)(void *, u32 cmd, void *parm);	// 解码控制。如设置AB点复读等
};

#define REGISTER_AUDIO_DECODER(ops) \
        const struct audio_decoder_ops ops SEC(.audio_decoder)

extern const struct audio_decoder_ops audio_decoder_begin[];
extern const struct audio_decoder_ops audio_decoder_end[];

#define list_for_each_audio_decoder(p) \
    for (p = audio_decoder_begin; p < audio_decoder_end; p++)


/*
 * 解码通道
 */
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

// 解码输出声道类型
#define AUDIO_DEC_ORIG_CH       AUDIO_CH_LR
#define AUDIO_DEC_L_CH          AUDIO_CH_L
#define AUDIO_DEC_R_CH          AUDIO_CH_R
#define AUDIO_DEC_MONO_LR_CH    AUDIO_CH_DIFF
#define AUDIO_DEC_DUAL_L_CH     AUDIO_CH_DUAL_L
#define AUDIO_DEC_DUAL_R_CH     AUDIO_CH_DUAL_R
#define AUDIO_DEC_DUAL_LR_CH    AUDIO_CH_DUAL_LR

#define AUDIO_DEC_IS_MONO(ch)	(((ch)==AUDIO_DEC_L_CH) || ((ch)==AUDIO_DEC_R_CH) || ((ch)==AUDIO_DEC_MONO_LR_CH))

/*
*********************************************************************
*                  Audio Decoder Task Create
* Description: 创建解码任务
* Arguments  : *task	任务句柄
*              *name	任务名
* Return	 : 0		成功
* Note(s)    : None.
*********************************************************************
*/
int audio_decoder_task_create(struct audio_decoder_task *task, const char *name);

/*
*********************************************************************
*                  Audio Decoder Task Add ResWait
* Description: 解码任务增加解码通道资源
* Arguments  : *task	解码任务句柄
*              *wait	解码资源句柄
* Return	 : 0		成功
* Note(s)    : 加入链表后，按*wait配置信息播放
*********************************************************************
*/
int audio_decoder_task_add_wait(struct audio_decoder_task *task, struct audio_res_wait *wait);

/*
*********************************************************************
*                  Audio Decoder Task Delete ResWait
* Description: 解码任务删除解码通道资源
* Arguments  : *task	解码任务句柄
*              *wait	解码资源句柄
* Return	 : None.
* Note(s)    : 当前的删除后，如果链表中还有res，按规则继续播放
*********************************************************************
*/
void audio_decoder_task_del_wait(struct audio_decoder_task *task, struct audio_res_wait *wait);

/*
*********************************************************************
*                  Audio Decoder Get Task ResWait state
* Description: 获取解码任务链表中的资源数量
* Arguments  : *task	解码任务句柄
* Return	 : ResWait数量
* Note(s)    : None.
*********************************************************************
*/
int audio_decoder_task_wait_state(struct audio_decoder_task *task);

/*
*********************************************************************
*                  Audio Decoder Resume All Decoder Channel
* Description: 激活解码任务链表中的所有解码
* Arguments  : *task	解码任务句柄
* Return	 : 0		成功
* Note(s)    : None.
*********************************************************************
*/
int audio_decoder_resume_all(struct audio_decoder_task *task);

/*
*********************************************************************
*                  Audio Decoder Add Format Lock
* Description: 解码任务资源添加格式锁
* Arguments  : *task	解码任务句柄
*              fmt		解码格式
* Return	 : 0		成功
* Note(s)    : 锁定后，后续的解码将不能使用该解码
*********************************************************************
*/
int audio_decoder_fmt_lock(struct audio_decoder_task *task, int fmt);

/*
*********************************************************************
*                  Audio Decoder Add Format Unlock
* Description: 解码任务资源格式锁解锁
* Arguments  : *task	解码任务句柄
*              fmt		解码格式
* Return	 : 0		成功
* Note(s)    : 解锁后，后续的解码可以使用该解码
*********************************************************************
*/
int audio_decoder_fmt_unlock(struct audio_decoder_task *task, int fmt);


/*
*********************************************************************
*                  Audio Decoder Output
* Description: 解码输出
* Arguments  : *_dec	解码句柄
*              *buf		输出buf
*              len		输出长度
*              *priv	私有参数
* Return	 : 输出了多少数据
* Note(s)    : 解码输出不了会被挂起，需要调用resume激活该解码
*********************************************************************
*/
int audio_decoder_put_output_buff(void *_dec, void *buff, int len, void *priv);

/*
*********************************************************************
*                  Audio Decoder Read Data
* Description: 解码读数
* Arguments  : *_dec	解码句柄
*              *buf		输出buf
*              len		输出长度
*              offset	读数偏移
* Return	 : 读到了多长（错误返回负数）
* Note(s)    : 仅文件解码可用
*********************************************************************
*/
int audio_decoder_read_data(void *_dec, u8 *data, int len, u32 offset);

/*
*********************************************************************
*                  Audio Decoder Get Input Len
* Description: 获取解码文件长度
* Arguments  : *_dec	解码句柄
* Return	 : 文件长度
* Note(s)    : 仅文件解码可用
*********************************************************************
*/
int audio_decoder_get_input_data_len(void *_dec);

/*
*********************************************************************
*                  Audio Decoder Get Frame Data
* Description: 获取流数据帧内容
* Arguments  : *_dec	解码句柄
*              **frame	返回帧信息
* Return	 : 帧长度
* Note(s)    : 仅流数据解码可用
*********************************************************************
*/
int audio_decoder_get_frame(void *_dec, u8 **frame);

/*
*********************************************************************
*                  Audio Decoder Fetch Frame
* Description: 检查流数据帧内容
* Arguments  : *_dec	解码句柄
*              **frame	返回帧信息
* Return	 : 帧长度
* Note(s)    : 仅流数据解码可用。数据没有读走，仅查看内容
*********************************************************************
*/
int audio_decoder_fetch_frame(void *_dec, u8 **frame);

/*
*********************************************************************
*                  Audio Decoder Put Frame
* Description: 流数据帧使用完
* Arguments  : *_dec	解码句柄
*              *frame	帧信息
* Return	 : None.
* Note(s)    : 仅流数据解码可用
*********************************************************************
*/
void audio_decoder_put_frame(void *_dec, u8 *frame);

/*
*********************************************************************
*                  Audio Decoder Open Channel
* Description: 打开一个解码通道
* Arguments  : *dec		解码句柄
*              *input: 解码输入参数信息
*              *task: 解码任务句柄
* Return	 : 0		成功
* Note(s)    : None.
*********************************************************************
*/
int audio_decoder_open(struct audio_decoder *dec, const struct audio_dec_input *input,
                       struct audio_decoder_task *task);

/*
*********************************************************************
*                  Audio Decoder Get Data Type
* Description: 获取配置的解码类型
* Arguments  : *_dec	解码句柄
* Return	 : 解码类型AUDIO_INPUT_FILE或者AUDIO_INPUT_FRAME
* Note(s)    : None.
*********************************************************************
*/
int audio_decoder_data_type(void *_dec);

/*
*********************************************************************
*                  Audio Decoder Get Format
* Description: 获取解码器格式信息
* Arguments  : *_dec	解码句柄
*              **fmt	获取到的格式信息
* Return	 : 0		成功
* Note(s)    : 解码器没有打开时，尝试打开解码器
*********************************************************************
*/
int audio_decoder_get_fmt(struct audio_decoder *dec, struct audio_fmt **fmt);

/*
*********************************************************************
*                  Audio Decoder Set Format
* Description: 设置解码的格式信息
* Arguments  : *_dec	解码句柄
*              *fmt		格式信息
* Return	 : 0		成功
* Note(s)    : 直接指定解码格式，如sbc解码等
*********************************************************************
*/
int audio_decoder_set_fmt(struct audio_decoder *dec, struct audio_fmt *fmt);

/*
*********************************************************************
*                  Audio Decoder Get Format Info
* Description: 实时获取解码器格式信息
* Arguments  : *dec		解码句柄
*              *fmt		获取到的格式信息
* Return	 : 0		成功
* Note(s)    : 需要在解码成功后才能调用。函数中调用解码器get_fmt()接口
*********************************************************************
*/
int audio_decoder_get_fmt_info(struct audio_decoder *dec, struct audio_fmt *fmt);

/*
*********************************************************************
*                  Audio Decoder Set Deal Handler
* Description: 设置解码处理handler
* Arguments  : *dec		解码句柄
*              *handler	解码处理handler
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_decoder_set_handler(struct audio_decoder *dec, const struct audio_dec_handler *handler);


/*
*********************************************************************
*                  Audio Decoder Set Event Handler
* Description: 设置解码事件回调
* Arguments  : *dec		解码句柄
*              *handler	回调函数
*              maigc	私有标识
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_decoder_set_event_handler(struct audio_decoder *dec,
                                     void (*handler)(struct audio_decoder *, int, int *), u32 magic);


/*
*********************************************************************
*                  Audio Decoder Set Output Channel
* Description: 设置解码输出声道类型
* Arguments  : *dec		解码句柄
*              channel	声道类型
* Return	 : 0		成功
* Note(s)    : 函数中调用解码器set_output_channel()接口
*********************************************************************
*/
int audio_decoder_set_output_channel(struct audio_decoder *dec, enum audio_channel);

/*
*********************************************************************
*                  Audio Decoder Start
* Description: 启动解码
* Arguments  : *dec		解码句柄
* Return	 : 0		成功
* Note(s)    : 函数中调用解码器start()接口
*********************************************************************
*/
int audio_decoder_start(struct audio_decoder *dec);

/*
*********************************************************************
*                  Audio Decoder Stop
* Description: 停止解码
* Arguments  : *dec		解码句柄
* Return	 : 0		成功
* Note(s)    : None
*********************************************************************
*/
int audio_decoder_stop(struct audio_decoder *dec);

/*
*********************************************************************
*                  Audio Decoder Pause
* Description: 解码暂停
* Arguments  : *dec		解码句柄
* Return	 : 0		成功
* Note(s)    : None
*********************************************************************
*/
int audio_decoder_pause(struct audio_decoder *dec);

/*
*********************************************************************
*                  Audio Decoder Suspend
* Description: 解码挂起
* Arguments  : *dec		解码句柄
* Return	 : 0		成功
* Note(s)    : None
*********************************************************************
*/
int audio_decoder_suspend(struct audio_decoder *dec);

/*
*********************************************************************
*                  Audio Decoder Resume
* Description: 解码挂起激活
* Arguments  : *dec		解码句柄
* Return	 : 0		成功
* Note(s)    : None
*********************************************************************
*/
int audio_decoder_resume(struct audio_decoder *dec);

/*
*********************************************************************
*                  Audio Decoder Close
* Description: 关闭解码
* Arguments  : *dec		解码句柄
* Return	 : 0		成功
* Note(s)    : None
*********************************************************************
*/
int audio_decoder_close(struct audio_decoder *dec);

/*
*********************************************************************
*                  Audio Decoder Reset
* Description: 解码器reset
* Arguments  : *dec		解码句柄
* Return	 : 0		成功
* Note(s)    : 函数中调用解码器reset()接口
*********************************************************************
*/
int audio_decoder_reset(struct audio_decoder *dec);

/*
*********************************************************************
*                  Audio Decoder Set Breakpoint Handler
* Description: 设置解码断点句柄
* Arguments  : *dec		解码句柄
*              *bp		断点句柄
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_decoder_set_breakpoint(struct audio_decoder *dec, struct audio_dec_breakpoint *bp);

/*
*********************************************************************
*                  Audio Decoder Get Breakpoint
* Description: 获取解码断点信息
* Arguments  : *dec		解码句柄
*              *bp		断点句柄
* Return	 : 0		成功
* Note(s)    : None.
*********************************************************************
*/
int audio_decoder_get_breakpoint(struct audio_decoder *dec, struct audio_dec_breakpoint *bp);

/*
*********************************************************************
*                  Audio Decoder Forward
* Description: 解码快进
* Arguments  : *dec		解码句柄
*              step_s	快进步伐（按秒计数）
* Return	 : 0		成功
* Note(s)    : 函数中调用解码器fast_forward()接口
*********************************************************************
*/
int audio_decoder_forward(struct audio_decoder *dec, int step_s);

/*
*********************************************************************
*                  Audio Decoder Rewind
* Description: 解码快退
* Arguments  : *dec		解码句柄
*              step_s	快退步伐（按秒计数）
* Return	 : 0		成功
* Note(s)    : 函数中调用解码器fast_rewind()接口
*********************************************************************
*/
int audio_decoder_rewind(struct audio_decoder *dec, int step_s);

/*
*********************************************************************
*                  Audio Decoder Get Total Time
* Description: 获取解码总时间
* Arguments  : *dec		解码句柄
* Return	 : 解码总时间
* Note(s)    : None.
*********************************************************************
*/
int audio_decoder_get_total_time(struct audio_decoder *dec);

/*
*********************************************************************
*                  Audio Decoder Get Play Time
* Description: 获取解码当前时间
* Arguments  : *dec		解码句柄
* Return	 : 解码当前时间
* Note(s)    : 函数中调用解码器get_play_time()接口
*********************************************************************
*/
int audio_decoder_get_play_time(struct audio_decoder *dec);

/*
*********************************************************************
*                  Audio Decoder Set Pick Status
* Description: 设置解码拆包模式
* Arguments  : *dec		解码句柄
*              pick		拆包使能
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_decoder_set_pick_stu(struct audio_decoder *dec, u8 pick);

/*
*********************************************************************
*                  Audio Decoder Get Pick Status
* Description: 获取解码拆包模式
* Arguments  : *dec		解码句柄
* Return	 : 1		拆包
* Note(s)    : None.
*********************************************************************
*/
int audio_decoder_get_pick_stu(struct audio_decoder *dec);

/*
*********************************************************************
*                  Audio Decoder Set Tws Status
* Description: 设置解码tws模式
* Arguments  : *dec		解码句柄
*              tws		tws模式使能
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_decoder_set_tws_stu(struct audio_decoder *dec, u8 tws);

/*
*********************************************************************
*                  Audio Decoder Get Tws Status
* Description: 获取解码tws模式
* Arguments  : *dec		解码句柄
* Return	 : 1		tws模式
* Note(s)    : None.
*********************************************************************
*/
int audio_decoder_get_tws_stu(struct audio_decoder *dec);

/*
*********************************************************************
*                  Audio Decoder Set Max Run Number
* Description: 设置解码每次轮询可执行的最大次数
* Arguments  : *dec		解码句柄
*              run_max	最大次数
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_decoder_set_run_max(struct audio_decoder *dec, u8 run_max);

/*
*********************************************************************
*                  Audio Decoder Dual Channel Switch To Other
* Description: 双声道转换为其他声道
* Arguments  : ch_type	输出声道类型
*              half_lr	合并声道是否除2
*              *data	数据
*              len		数据长度
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_decoder_dual_switch(u8 ch_type, u8 half_lr, s16 *data, int len);

/*
*********************************************************************
*                  Audio Decoder Find Coding Type Have Used
* Description: 检查解码器是否已经使用
* Arguments  : *task	解码任务句柄
*              coding_type	解码器类型
* Return	 : 1		已经使用
* Note(s)    : None.
*********************************************************************
*/
int audio_decoder_find_coding_type(struct audio_decoder_task *task, u32 coding_type);

/*
*********************************************************************
*                  Audio Decoder Check Running Decoder Number
* Description: 统计正在运行的解码数
* Arguments  : *task	解码任务句柄
* Return	 : 正在运行的解码数
* Note(s)    : 统计解码状态为DEC_STA_START或者DEC_STA_WAIT_RESUME
*********************************************************************
*/
int audio_decoder_running_number(struct audio_decoder_task *task);

/*
*********************************************************************
*                  Audio Decoder Ioctrl
* Description: 解码器控制
* Arguments  : *dec		解码句柄
*              cmd		控制命令
*              *parm	控制参数
* Return	 : 0		成功
* Note(s)    : None.
*********************************************************************
*/
int audio_decoder_ioctrl(struct audio_decoder *dec, u32 cmd, void *parm);


/*
*********************************************************************
*                  Audio Decoder Output Task Create
* Description: 创建解码输出任务
* Arguments  : *task	任务句柄
*              *out_task_name	解码输出任务名
* Return	 : 0		成功
* Note(s)    : None.
*********************************************************************
*/
int audio_decoder_out_task_create(struct audio_decoder_task *task, const char *out_task_name);

/*
*********************************************************************
*                  Audio Decoder Output Task Destroy
* Description: 删除解码输出任务
* Arguments  : *task	任务句柄
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_decoder_out_task_destroy(struct audio_decoder_task *task);

/*
*********************************************************************
*                  Audio Decoder Output Task Channel Enable
* Description: 设置解码输出任务使能
* Arguments  : *dec		解码句柄
*              len		输出缓存长度
* Return	 : 0		成功
* Note(s)    : 当解码输出长度大于设置的长度时，会关闭该通道输出任务使能
*********************************************************************
*/
int audio_decoder_out_task_ch_enable(struct audio_decoder *dec, int len);

/*
*********************************************************************
*                  Audio Decoder Resume Output Task
* Description: 激活解码输出任务
* Arguments  : *dec		解码句柄
* Return	 : None.
* Note(s)    : None.
*********************************************************************
*/
void audio_decoder_resume_out_task(struct audio_decoder *dec);

/*
*********************************************************************
*                  Audio Decoder Get sample_rate
* Description: 获取配置的解码采样率
* Arguments  : *_dec	解码句柄
* Return	 : sample_rate
* Note(s)    : None.
*********************************************************************
*/
int audio_decoder_get_sample_rate(void *_dec);
/*
*********************************************************************
*                  Audio Decoder Get bit rate
* Description: 获取配置的比特率
* Arguments  : *_dec	解码句柄
* Return	 : bit rate
* Note(s)    : None.
*********************************************************************
*/
int audio_decoder_get_bit_rate(void *_dec);


/*
*********************************************************************
*                  Audio Decoder Get sample_rate
* Description: 获取配置的解码帧长
* Arguments  : *_dec	解码句柄
* Return	 : sample_rate
* Note(s)    : None.
*********************************************************************
*/
int audio_decoder_get_frame_len(void *_dec);

/*
*********************************************************************
*                  Audio Decoder set dec out ch_num
* Description: 配置解码通道数
* Arguments  : *_dec	解码句柄  ch_num解码输出数据通道数
* Return	 :
* Note(s)    : None.
*********************************************************************
*/
void audio_decoder_set_channel(void *_dec, u8 ch_num);
int audio_decoder_get_channel(void *_dec);

#endif

