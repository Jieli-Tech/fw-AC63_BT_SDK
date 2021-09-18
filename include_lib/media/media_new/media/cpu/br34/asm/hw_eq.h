
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
    u16 index;
    u16 iir_type; ///<EQ_IIR_TYPE
    int freq;
    int gain;
    int q;
};

struct eq_coeff_info {
    u16 nsection : 6;
    u16 no_coeff : 1;	//不是滤波系数
#ifdef CONFIG_EQ_NO_USE_COEFF_TABLE
    u16 sr;
#endif
    int *L_coeff;
    int *R_coeff;
    /*     int L_gain; */
    /* int R_gain; */
    float L_gain;
    float R_gain;

};

struct hw_eq_ch;

struct hw_eq {
    struct list_head head;
    OS_MUTEX mutex;
    struct hw_eq_ch *cur_ch;
};

enum {
    HW_EQ_CMD_CLEAR_MEM = 0xffffff00,
    HW_EQ_CMD_CLEAR_MEM_L,
    HW_EQ_CMD_CLEAR_MEM_R,
};

struct hw_eq_handler {
    int (*eq_probe)(struct hw_eq_ch *);
    int (*eq_output)(struct hw_eq_ch *, s16 *, u16);
    int (*eq_post)(struct hw_eq_ch *);
    int (*eq_input)(struct hw_eq_ch *, void **, void **);
};

struct hw_eq_ch {
    unsigned int out_32bit : 1;
    unsigned int updata : 1;
    unsigned int updata_coeff_only : 1;	// 只更新参数，不更新中间数据
    unsigned int no_wait : 1;
    unsigned int channels : 2;
    unsigned int SHI : 4;
    unsigned int countL : 4;
    unsigned int stage : 9;
    unsigned int nsection : 6;
    unsigned int no_coeff : 1;	// 非滤波系数
    volatile unsigned int active : 1;
#ifdef CONFIG_EQ_NO_USE_COEFF_TABLE
        u16 sr;
#endif
        int *L_coeff;
        int *R_coeff;
        int L_gain;
        int R_gain;
        int *eq_LRmem;
        s16 *out_buf;
        s16 *in_buf;
        int in_len;
        void *priv;
        volatile OS_SEM sem;
        struct list_head entry;
        struct hw_eq *eq;
        const struct hw_eq_handler *eq_handler;
    };

//系数计算子函数
    extern void design_lp(int fc, int fs, int quality_factor, int *coeff);
    extern void design_hp(int fc, int fs, int quality_factor, int *coeff);
    extern void design_pe(int fc, int fs, int gain, int quality_factor, int *coeff);
    extern void design_ls(int fc, int fs, int gain, int quality_factor, int *coeff);
    extern void design_hs(int fc, int fs, int gain, int quality_factor, int *coeff);
    extern int eq_stable_check(int *coeff);
    extern void eq_get_AllpassCoeff(void *Coeff);


// eq滤波系数计算
    int eq_seg_design(struct eq_seg_info *seg, int sample_rate, int *coeff);


// 在EQ中断中调用
    void audio_hw_eq_irq_handler(struct hw_eq *eq);

// EQ初始化
    int audio_hw_eq_init(struct hw_eq *eq, u32 eq_section_num);

// 打开一个通道
    int audio_hw_eq_ch_open(struct hw_eq_ch *ch, struct hw_eq *eq);
// 设置回调接口
    int audio_hw_eq_ch_set_handler(struct hw_eq_ch *ch, struct hw_eq_handler *handler);
// 设置通道基础信息
    int audio_hw_eq_ch_set_info(struct hw_eq_ch *ch, u8 channels, u8 out_32bit);

// 设置硬件转换系数
    int audio_hw_eq_ch_set_coeff(struct hw_eq_ch *ch, struct eq_coeff_info *info);

// 启动一次转换
    int audio_hw_eq_ch_start(struct hw_eq_ch *ch, void *input, void *output, int len);

// 关闭一个通道
    int audio_hw_eq_ch_close(struct hw_eq_ch *ch);

// 挤出eq中的数据
    int audio_hw_eq_flush_out(struct hw_eq *eq);

// eq正在运行
    int audio_hw_eq_is_running(struct hw_eq *eq);

#endif /*__HW_EQ_H*/

