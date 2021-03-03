/*****************************************************************
>file name : audio_sync.h
>author : lichao
>create time : Fri 29 May 2020 11:11:23 AM CST
*****************************************************************/
#ifndef _ASM_AUDIO_SYNC_H_
#define _ASM_AUDIO_SYNC_H_
#include "media/audio_stream.h"

#define AUDIO_SYNC_OUTPUT_IDLE                  0
#define AUDIO_SYNC_OUTPUT_PROBE                 1 //输出前处理
#define AUDIO_SYNC_OUTPUT_START                 2 //输出开始
#define AUDIO_SYNC_OUTPUT_MISS_DATA             3 //输出采样丢失样点
#define AUDIO_SYNC_OUTPUT_ALIGN_COMPLETE        4 //输出对齐完成

#define AUDIO_SYNC_DAC                          0
#define AUDIO_SYNC_IIS                          1

#define PCM_PHASE_BIT                           8

#define AUDIO_STANDARD_STREAM                   1
#define AUDIO_OUTPUT_STREAM                     2


#define SAMP_STREAM_FRAME_OUTPUT                0
#define SAMP_STREAM_CBUF_OUTPUT                 1

#define SAMPLE_SYNC_DEVICE_IDLE                 0
#define SAMPLE_SYNC_DEVICE_START                1

#define BT_AHB_BASE_ADR             0x1c0000L

#define SAMPLECLK                   (*(volatile unsigned long *)(BT_AHB_BASE_ADR + 0x7 *4))
#define SLOTCLK                     (*(volatile unsigned long *)(BT_AHB_BASE_ADR + 0x8 *4))
#define FINETIMECNT                 (*(volatile unsigned long *)(BT_AHB_BASE_ADR + 0x9 *4))

#define BT_SLOT_CLKN()              ({u32 slot_clk;SAMPLECLK = 1;while(SAMPLECLK);slot_clk = SLOTCLK;slot_clk;})
#define BT_FINETIME_CNT()           (FINETIMECNT)

struct audio_bt_position {
    u32 sample_counter;
    u32 pcm_position;
    u32 bt_time;
    u32 clkoffset;
    u16 bt_time_phase;
    s16 bitoff;
    u32 buf_num;
};

struct audio_output_position {
    u32 out_samples;            //输出样点个数
    u32 bt_time;                //蓝牙时间
    u16 bt_time_phase;          //蓝牙时间相位
};

struct audio_input_position {
    u32 buf_num;            //输出buf当前缓冲样点个数
    u32 bt_time;            //蓝牙时间
    u32 clkoffset;          //与主机的偏差
    int bitoff;             //与主机偏差值的相位
    u16 bt_time_phase;      //蓝牙时间的相位
    u32 pcm_position;       //样点输入位置
};

struct resample_output_cbuf {
    s16 *(*write_alloc)(void *buffer, int *len);
    int (*write_update)(void *buffer, int len);
};

struct resample_output_frame {
    u8 free;
    void *addr;
    int len;
    void *remain_addr;
    int remain_len;
    int (*output)(void *buffer, void *data, int len);
};

struct sample_sync_device_ops {
    int (*data_len)(void *priv);            /*设备音频缓冲数据长度*/
    int (*state_query)(void *priv);         /*设备输出状态查询*/
};

struct audio_sample_output {
    u8 mode;                                /*输出模式*/
    void *priv;
    /*
    union {
        struct resample_output_cbuf cbuf;
        struct resample_output_frame frame;
    } u;
    */
    u8 free;                                /*是否关闭时释放*/
    void *addr;                             /*输出缓冲地址*/
    int len;                                /*输出缓冲长度*/
    void *remain_addr;                      /*输出剩余缓冲地址*/
    int remain_len;                         /*输出剩余缓冲长度*/
    struct sample_sync_device_ops device;   /*输出设备管理*/
};

/*************************************************************************
 * 音频变采样同步结构
 *
 * HISTORY :
 *      2020/6/18 by LC.
 *=======================================================================*/
struct audio_sample_sync {
    u8 start;                   /*模块start标志*/
    u8 bt_time_enable;          /*蓝牙时间设置起始使能(待删除)*/
    u8 fixed_sample_rate;       /*固定输出采样率*/
    u8 channels;                /*变采样音频声道数*/
    u8 quick_align;             /*快速调整对齐*/
    u8 position_num;            /*位置信息个数*/
    u8 block_start;             /*阻塞起始(待删除*/
    s8 index;                   /*位置保存id*/
    int quick_output_count;     /*快速调整变采样输出样点数*/
    int sample_rate;            /*采样率*/
    int input_sample_rate;      /*输入采样率*/
    u16 in_rate;                /*保存的变采样in采样率*/
    u16 out_rate;               /*保存的变采样out采样率*/
    u32 pcm_position;           /*已变采样的输入位置*/
    u32 input_num;              /*样点输入（包含未变采样）个数*/
    u32 sample_counter;         /*变采样输出计数器*/
    u32 output_counter;         /*输出设备样点计数器*/
    u32 bt_clkn;                /*蓝牙时钟*/
    s16 bt_clkn_phase;          /*蓝牙时钟相位*/
    struct audio_sample_output output;                          /*变采样输出管理*/
    struct audio_bt_position *position;                         /*蓝牙音频位置信息*/
    void *event_priv;                                           /*事件处理私有数据*/
    int (*event_handler)(void *priv, void *output, u8 event);   /*事件处理函数*/
    struct audio_stream_entry entry;                            /*audio stream节点*/
    struct audio_src_base_handle *src;                          /*SRC模块指针*/
    void *irq_priv;                                             /*中断响应私有数据*/
    void (*irq_callback)(void *priv);                           /*中断响应回调*/
};

/*************************************************************************
 * 音频变采样同步打开
 * INPUT    :   stream_mode - 数据流模式(暂时无意义)
 * OUTPUT   :   非NULL - 打开成功，NULL - 打开失败.
 * WARNINGS :   无.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
struct audio_sample_sync *audio_sample_sync_open(u8 stream_mode);

/*************************************************************************
 * 音频变采样同步初始化变采样信息
 * INPUT    :   s                   - 同步变采样结构指针
 *              input_sample_rate   - 输入采样率
 *              sample_rate         - 输出采样率(标准)
 *              data_channels       - 数据流声道数
 *              position_num        - 位置信息个数
 * OUTPUT   :   0 - 初始化成功，非0 - 初始化失败.
 * WARNINGS :   无.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_sample_sync_init_resample(struct audio_sample_sync *s,
                                    int input_sample_rate,
                                    int sample_rate,
                                    u8 data_channels,
                                    u8 position_num);

/*************************************************************************
 * 音频变采样同步模块关闭
 * INPUT    :   s - 同步变采样结构指针
 * OUTPUT   :   无.
 * WARNINGS :   无.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
void audio_sample_sync_close(struct audio_sample_sync *s);

/*************************************************************************
 * 音频变采样同步模fifo输出设置
 * INPUT    :   s           - 同步变采样结构指针
 *              priv        - fifo私有数据
 *              fifo        - fifo输出结构
 * OUTPUT   :   无.
 * WARNINGS :   待开发和验证.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_sample_sync_set_fifo_output(struct audio_sample_sync *s,
                                      void *priv,
                                      struct resample_output_cbuf *fifo);

/*************************************************************************
 * 音频变采样同步关联输出设备
 * INPUT    :   s           - 同步变采样结构指针
 *              priv        - 设备私有数据
 *              device      - 关联的设备操作函数
 * OUTPUT   :   0 -成功，非0 - 失败.
 * WARNINGS :   设置变采样同步的必要函数之一.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_sample_sync_set_device(struct audio_sample_sync *s,
                                 void *priv,
                                 struct sample_sync_device_ops *device);

/*************************************************************************
 * 音频变采样同步关联输出设备
 * INPUT    :   s           - 同步变采样结构指针
 *              priv        - 设备私有数据
 *              device      - 关联的设备操作函数
 * OUTPUT   :   0 -成功，非0 - 失败.
 * WARNINGS :   设置变采样同步的必要函数之一.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
void audio_sample_sync_set_event_handler(struct audio_sample_sync *s,
        void *priv,
        int (*handler)(void *priv, void *ch, u8 event));

/*************************************************************************
 * 音频变采样同步中断响应处理设置
 * INPUT    :   s           - 同步变采样结构指针
 *              priv        - 中断响应私有数据
 *              handler     - 中断响应操作函数
 * OUTPUT   :   无.
 * WARNINGS :   Audio stream结构下使用, 用于非节点的数据流唤醒类的注册.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
void audio_sample_sync_set_irq_handler(struct audio_sample_sync *s, void *priv, void (*handler)(void *));
#if 0
/*AUDIO 样点同步设置普通buffer的输出*/
int audio_sample_sync_set_output_handler(struct audio_sample_sync *s,
        void *priv,
        int (*data_len)(void *priv);
        int (*output)(void *priv, void *data, int len));
#endif
/*************************************************************************
 * 音频变采样同步刷新数据输出
 * INPUT    :   s - 同步变采样结构指针
 * OUTPUT   :   0 - 刷出成功，非0 - 未成功刷出(不重要).
 * WARNINGS :   不推荐在audio stream节点下使用，若有需求请告知开发人员.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_sample_sync_flush_data(struct audio_sample_sync *s);

//int audio_sample_sync_start(struct audio_sample_sync *s);

/*************************************************************************
 * 音频变采样同步停止
 * INPUT    :   s - 同步变采样结构指针
 * OUTPUT   :   无.
 * WARNINGS :   无.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_sample_sync_stop(struct audio_sample_sync *s);

/*AUDIO 样点同步写函数，用于接入到数据流程中*/
//int audio_sample_sync_write(struct audio_sample_sync *s, void *buf, int len);

/*************************************************************************
 * 音频变采样同步设置蓝牙起始时间
 * INPUT    :   s           - 同步变采样结构指针
 *              bt_clkn     - 蓝牙时钟
 *              phase       - 时钟相位
 * OUTPUT   :   无.
 * WARNINGS :   此函数通过同步控制模块进行设置，最终由输出设备来查询.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_sample_sync_set_bt_time(struct audio_sample_sync *s, u32 bt_clkn, int phase);

/*************************************************************************
 * 音频变采样同步根据蓝牙时间确定起始
 * INPUT    :   s           - 同步变采样结构指针
 *              priv        - 回调私有数据
 *              callback    - 回调函数
 * OUTPUT   :   无.
 * WARNINGS :   此函数通过输出设备调用，非外部可用接口.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_sample_start_by_sync_time(struct audio_sample_sync *s, void *priv, void (*callback)(void *));

/*************************************************************************
 * 音频变采样同步根据蓝牙时间确定起始
 * INPUT    :   s - 同步变采样结构指针
 * OUTPUT   :   距离蓝牙起始时间的距离(us).
 * WARNINGS :   此函数通过输出设备调用，非外部可用接口.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_sample_sync_us_time_distance(struct audio_sample_sync *s);

/*************************************************************************
 * 音频变采样同步蓝牙起始时间使能关闭
 * INPUT    :   s - 同步变采样结构指针
 * OUTPUT   :   无.
 * WARNINGS :   无.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_sample_sync_bt_time_disable(struct audio_sample_sync *s);

/*************************************************************************
 * 音频变采样同步更新输出样点
 * INPUT    :   s       - 同步变采样结构指针
 *              points  - 样点个数
 * OUTPUT   :   无.
 * WARNINGS :   由输出设备调用该接口更新缓冲样点个数.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_sample_sync_update_count(struct audio_sample_sync *s, int points);

/*AUDIO 样点同步查询输入位置(某个时间和对应具体某个样点位置)*/
/*************************************************************************
 * 音频变采样同步获取当前变采样输入位置(已变采样)
 * INPUT    :   s       - 同步变采样结构指针
 *              pos     - 输入位置结构指针
 * OUTPUT   :   0 - 获取成功， 非0 - 获取失败(目前不存在失败情况).
 * WARNINGS :   无.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_sample_sync_get_in_position(struct audio_sample_sync *s, struct audio_input_position *pos);

/*************************************************************************
 * 音频变采样同步查询设备输出位置
 * INPUT    :   s       - 同步变采样结构指针
 *              pos     - 输出位置结构指针
 * OUTPUT   :   0 - 获取成功， 非0 - 获取失败(目前不存在失败情况).
 * WARNINGS :   无.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_sample_sync_get_out_position(struct audio_sample_sync *s, struct audio_output_position *pos);

/*************************************************************************
 * 音频变采样同步缓冲剩余长度
 * INPUT    :   s - 同步变采样结构指针
 * OUTPUT   :   长度.
 * WARNINGS :   无.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_sample_sync_out_remain_len(struct audio_sample_sync *s);

/*************************************************************************
 * 音频变采样同步输出设备采样起始
 * INPUT    :   s           - 同步变采样结构指针
 *              samples     - 起始缓冲样点个数
 *              sample_rate - 输出采样率
 * OUTPUT   :   无.
 * WARNINGS :   无.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_sample_sync_output_begin(struct audio_sample_sync *s, int samples, int sample_rate);

/*************************************************************************
 * 音频变采样同步输出采样率查询
 * INPUT    :   s - 同步变采样结构指针
 * OUTPUT   :   采样率.
 * WARNINGS :   无.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_sample_sync_output_rate(struct audio_sample_sync *s);

/*************************************************************************
 * 音频变采样同步输出设备丢失样点
 * INPUT    :   s - 同步变采样结构指针
 * OUTPUT   :   无.
 * WARNINGS :   调用处用来通知变采样同步漏样点.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_sample_sync_output_miss_data(struct audio_sample_sync *s);

/*************************************************************************
 * 音频变采样同步位置修正
 * INPUT    :   s       - 同步变采样结构指针
 *              num     - 修正样点个数
 * OUTPUT   :   无.
 * WARNINGS :   关系到具体的位置不可随意使用.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
void audio_sample_sync_position_correct(struct audio_sample_sync *s, int num);

/*************************************************************************
 * 音频变采样同步位置调整对齐
 * INPUT    :   s           - 同步变采样结构指针
 *              in_rate     - 调整阶段输入采样率
 *              out_rate    - 调整阶段输出采样率
 *              points      - 调整阶段输出样点个数
 *              phase_diff  - 相位差(暂时不使用)
 * OUTPUT   :   无.
 * WARNINGS :   关系到具体的位置不可随意使用.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_sample_sync_align_control(struct audio_sample_sync *s, int in_rate, int out_rate, int points, s16 phase_diff);

/*************************************************************************
 * 音频变采样同步速度调整
 * INPUT    :   s           - 同步变采样结构指针
 *              in_rate     - 调整输入采样率
 *              out_rate    - 调整输出采样率
 * OUTPUT   :   无.
 * WARNINGS :   无.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_sample_sync_rate_control(struct audio_sample_sync *s, int in_rate, int out_rate);

/*************************************************************************
 * 中断模式设备更新中断消耗样点位置
 * INPUT    :   s               - 同步变采样结构指针
 *              irq_sample_num  - 中断样点个数
 * OUTPUT   :   无.
 * WARNINGS :   无.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_irq_update_sample_sync_position(struct audio_sample_sync *s, int irq_sample_num);


/*************************************************************************
 * 音频变采样同步蓝牙目标时间距离获取(ms)
 * INPUT    :   s - 同步变采样结构指针
 * OUTPUT   :   距离时间(ms).
 * WARNINGS :   无.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_sample_sync_time_distance(struct audio_sample_sync *s);

/*************************************************************************
 * 音频变采样同步关联设备输出状态查询
 * INPUT    :   s - 同步变采样结构指针
 * OUTPUT   :   SAMPLE_SYNC_DEVICE_IDLE     - 输出空闲
 *              SAMPLE_SYNC_DEVICE_START    - 输出开启
 * WARNINGS :   无.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_sample_sync_output_query(struct audio_sample_sync *s);

/*************************************************************************
 * 音频变采样同步是否在工作
 * INPUT    :   s - 同步变采样结构指针
 * OUTPUT   :   1 - 是,  0 - 否
 * WARNINGS :   无.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
int audio_sample_sync_is_working(struct audio_sample_sync *s);

/*************************************************************************
 * 音频变采样同步已输入的整点个数
 * INPUT    :   s - 同步变采样结构指针
 * OUTPUT   :   样点个数.
 * WARNINGS :   无.
 * HISTORY  :   2020/6/18 by LC.
 *=======================================================================*/
u32 audio_sample_sync_input_pcm_num(struct audio_sample_sync *s);
#endif
