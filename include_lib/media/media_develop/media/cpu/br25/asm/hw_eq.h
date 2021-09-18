
#ifndef __HW_EQ_H
#define __HW_EQ_H

#include "generic/typedef.h"
#include "generic/list.h"
#include "os/os_api.h"
#include "application/eq_func_define.h"


/*eq IIR type*/
typedef enum {
    EQ_IIR_TYPE_HIGH_PASS = 0x00,
    EQ_IIR_TYPE_LOW_PASS,
    EQ_IIR_TYPE_BAND_PASS,
    EQ_IIR_TYPE_HIGH_SHELF,
    EQ_IIR_TYPE_LOW_SHELF,
} EQ_IIR_TYPE;

struct eq_seg_info {
    u16 index;            //eq段序号
    u16 iir_type;         //滤波器类型EQ_IIR_TYPE
    int freq;             //中心截止频率
    int gain;             //增益（-12 ~12 db）*(1<<20)
    int q;                //q值（0.3~30）*(1<<24)
};

struct eq_coeff_info {
    u16 nsection : 6;     //eq段数
    u16 no_coeff : 1;	  //不是滤波系数
#ifdef CONFIG_EQ_NO_USE_COEFF_TABLE
    u32 sr;               //采样率
#endif
    int *L_coeff;         //左声道滤波器系数地址
    int *R_coeff;         //右声道滤波器系数地址
    float L_gain;         //左声道总增益(-20~20db)
    float R_gain;         //右声道总增益（-20~20db）
};

struct hw_eq_ch;

struct hw_eq {
    struct list_head head;            //链表头
    OS_MUTEX mutex;                   //互斥锁
    volatile struct hw_eq_ch *cur_ch; //当前需要处理的eq通道
};

enum {
    HW_EQ_CMD_CLEAR_MEM = 0xffffff00,
    HW_EQ_CMD_CLEAR_MEM_L,
    HW_EQ_CMD_CLEAR_MEM_R,
};

struct hw_eq_handler {
    int (*eq_probe)(struct hw_eq_ch *);                   //eq驱动内前处理
    int (*eq_output)(struct hw_eq_ch *, s16 *, u16);      //eq驱动内输出处理回调
    int (*eq_post)(struct hw_eq_ch *);                    //eq驱动内处理后回调
    int (*eq_input)(struct hw_eq_ch *, void **, void **); //eq驱动内输入处理回调
};

struct hw_eq_ch {
    unsigned int out_32bit : 1;          //是否输出32bit位宽的数据  0：16bit 1:32bit
    unsigned int updata : 1;             //更新参数以及中间数据
    unsigned int updata_coeff_only : 1;	 //只更新参数，不更新中间数据
    unsigned int no_wait : 1;            //是否是异步eq处理  0：同步的eq  1：异步的eq
    unsigned int channels : 2;           //通道数（1或2）
    unsigned int SHI : 4;                //eq运算输出数据左移位数控制,记录
    unsigned int countL : 4;             //eq运算输出数据左移位数控制临时记录
    unsigned int stage : 9;              //eq运算开始位置标识
    unsigned int nsection : 6;           //eq段数
    unsigned int no_coeff : 1;	         // 非滤波系数
    volatile unsigned int active : 1;    //已启动eq处理  1：busy  0:处理结束
        volatile unsigned int need_run: 1;//多eq同时使用时，未启动成功的eq，是否需要重新唤醒处理  1：需要  0：否
#ifdef CONFIG_EQ_NO_USE_COEFF_TABLE
        u32 sr;                          //采样率
#endif
        int *L_coeff;                    //输入给左声道系数地址
        int *R_coeff;                    //输入给右声道系数地址
        int L_gain;                      //输入给左声道总增益(-20~20)
        int R_gain;                      //输入给右声道总增益(-20~20)
        int *eq_LRmem;                   //eq系数地址（包含运算的中间数据）
        s16 *out_buf;                    //输出buf地址
        s16 *in_buf;                     //输入buf地址
        int in_len;                      //输入数据长度
        void *priv;                      //保存eq管理层的句柄
        volatile OS_SEM sem;             //信号量，用于通知驱动，当前一次处理完成
        struct list_head entry;          //当前eq通道的节点
        struct hw_eq *eq;                //底层eq操作句柄
        const struct hw_eq_handler *eq_handler;//eq操作的相关回调函数句柄
        void *irq_priv;                  //eq管理层传入的私有指针
        void (*irq_callback)(void *priv);//需要eq中断执行的回调函数
    };

//系数计算子函数
    /*----------------------------------------------------------------------------*/
    /**@brief    低通滤波器
       @param    fc:中心截止频率
       @param    fs:采样率
       @param    quality_factor:q值
       @param    coeff:计算后，系数输出地址
       @return
       @note
    */
    /*----------------------------------------------------------------------------*/
    extern void design_lp(int fc, int fs, int quality_factor, int *coeff);
    /*----------------------------------------------------------------------------*/
    /**@brief    高通滤波器
       @param    fc:中心截止频率
       @param    fs:采样率
       @param    quality_factor:q值
       @param    coeff:计算后，系数输出地址
       @return
       @note
    */
    /*----------------------------------------------------------------------------*/
    extern void design_hp(int fc, int fs, int quality_factor, int *coeff);
    /*----------------------------------------------------------------------------*/
    /**@brief    带通滤波器
       @param    fc:中心截止频率
       @param    fs:采样率
       @param    gain:增益
       @param    quality_factor:q值
       @param    coeff:计算后，系数输出地址
       @return
       @note
    */
    /*----------------------------------------------------------------------------*/
    extern void design_pe(int fc, int fs, int gain, int quality_factor, int *coeff);
    /*----------------------------------------------------------------------------*/
    /**@brief    低频搁架式滤波器
       @param    fc:中心截止频率
       @param    fs:采样率
       @param    gain:增益
       @param    quality_factor:q值
       @param    coeff:计算后，系数输出地址
       @return
       @note
    */
    /*----------------------------------------------------------------------------*/
    extern void design_ls(int fc, int fs, int gain, int quality_factor, int *coeff);
    /*----------------------------------------------------------------------------*/
    /**@brief    高频搁架式滤波器
       @param    fc:中心截止频率
       @param    fs:采样率
       @param    gain:增益
       @param    quality_factor:q值
       @param    coeff:计算后，系数输出地址
       @return
       @note
    */
    /*----------------------------------------------------------------------------*/
    extern void design_hs(int fc, int fs, int gain, int quality_factor, int *coeff);
    /*----------------------------------------------------------------------------*/
    /**@brief    滤波器系数检查
       @param    coeff:滤波器系数
       @return
       @note
    */
    /*----------------------------------------------------------------------------*/
    extern int eq_stable_check(int *coeff);
    /*----------------------------------------------------------------------------*/
    /**@brief    获取直通的滤波器系数
       @param    coeff:滤波器系数
       @return
       @note
    */
    /*----------------------------------------------------------------------------*/
    extern void eq_get_AllpassCoeff(void *Coeff);

    /*----------------------------------------------------------------------------*/
    /**@brief    滤波器计算管理函数
       @param    *seg:提供给滤波器的基本信息
       @param    sample_rate:采样率
       @param    *coeff:计算后，系数输出地址
       @return
       @note
    */
    /*----------------------------------------------------------------------------*/
    int eq_seg_design(struct eq_seg_info *seg, int sample_rate, int *coeff);


    /*----------------------------------------------------------------------------*/
    /**@brief    在EQ中断中调用
       @param    *eq:句柄
       @return
       @note
    */
    /*----------------------------------------------------------------------------*/
    void audio_hw_eq_irq_handler(struct hw_eq *eq);

    /*----------------------------------------------------------------------------*/
    /**@brief    EQ初始化
       @param    *eq:句柄
       @return
       @note
    */
    /*----------------------------------------------------------------------------*/
    int audio_hw_eq_init(struct hw_eq *eq);

    /*----------------------------------------------------------------------------*/
    /**@brief    打开一个通道
       @param    *ch:通道句柄
       @param    *eq:句柄
       @return
       @note
    */
    /*----------------------------------------------------------------------------*/
    int audio_hw_eq_ch_open(struct hw_eq_ch *ch, struct hw_eq *eq);
    /*----------------------------------------------------------------------------*/
    /**@brief    设置回调接口
       @param    *ch:通道句柄
       @param    *handler:回调的句柄
       @return
       @note
    */
    /*----------------------------------------------------------------------------*/
    int audio_hw_eq_ch_set_handler(struct hw_eq_ch *ch, struct hw_eq_handler *handler);
//
    /*----------------------------------------------------------------------------*/
    /**@brief    设置通道基础信息
       @param    *ch:通道句柄
       @param    channels:通道数
       @param    out_32bit:是否输出32bit位宽数据 1：是  0：16bit位宽
       @return
       @note
    */
    /*----------------------------------------------------------------------------*/
    int audio_hw_eq_ch_set_info(struct hw_eq_ch *ch, u8 channels, u8 out_32bit);

    /*----------------------------------------------------------------------------*/
    /**@brief    设置硬件转换系数
       @param    *ch:通道句柄
       @param    *info:系数、增益等信息
       @return
       @note
    */
    /*----------------------------------------------------------------------------*/
    int audio_hw_eq_ch_set_coeff(struct hw_eq_ch *ch, struct eq_coeff_info *info);

    /*----------------------------------------------------------------------------*/
    /**@brief    启动一次转换
       @param   *ch:eq句柄
       @param  *input:输入数据地址
       @param  *output:输出数据地址
       @param  len:输入数据长度
       @return
       @note
    */
    /*----------------------------------------------------------------------------*/
    int audio_hw_eq_ch_start(struct hw_eq_ch *ch, void *input, void *output, int len);

    /*----------------------------------------------------------------------------*/
    /**@brief  关闭一个通道
       @param   *ch:eq句柄
       @return
       @note
    */
    /*----------------------------------------------------------------------------*/
    int audio_hw_eq_ch_close(struct hw_eq_ch *ch);

    /*----------------------------------------------------------------------------*/
    /**@brief  获取eq是否正在运行状态
       @param   *ch:eq句柄
       @return
       @note
    */
    /*----------------------------------------------------------------------------*/
    int audio_hw_eq_is_running(struct hw_eq *eq);

#endif /*__HW_EQ_H*/

