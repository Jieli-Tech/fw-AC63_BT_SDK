/*******************************************************************************************
  File : audio_dig_vol.h
  By   : LinHaibin
  brief: 数据数字音量与分组管理
         mem : sizeof(audio_dig_vol_hdl) + 12 * channels (Byte)
         mips: 6MHz / 44100 points / 2ch / sec (soft)
               3MHz / 44100 points / 2ch / sec (asm)
  Email: linhaibin@zh-jieli.com
  date : Fri, 24 Jul 2020 18:00:39 +0800
********************************************************************************************/
#ifndef _AUDIO_DIG_VOL_H_
#define _AUDIO_DIG_VOL_H_


#define AUDIO_DIG_VOL_CH(x)     (BIT(x))
#define AUDIO_DIG_VOL_ALL_CH    (0xFFFF)

typedef struct _audio_dig_vol_param {
    u8  vol_start;          // 打开数字音量后的起始音量等级
    u8  vol_max;            // 数字音量的最大音量等级
    u8  ch_total;           // 音频数据通道总数
    u8  fade_en;            // 淡入淡出使能
    u16 fade_points_step;   // 淡入淡出点数步长
    u16 fade_gain_step;     // 淡入淡出增益步长
    u16 *vol_list;          // 自定义音量等级增益映射表，NULL则使用默认
} audio_dig_vol_param;

/*******************************************************
* Function name	: audio_dig_vol_entry_get
* Description	: 获取数字音量数据流entry句柄
* Parameter		:
*   @_hdl       	数字音量句柄
* Return        : 返回句柄, NULL：失败
*******************************************************/
extern void *audio_dig_vol_entry_get(void *_hdl);

/*******************************************************
* Function name	: audio_dig_vol_open
* Description	: 新建数字音量句柄
* Parameter		:
*   @param       	配置
* Return        : 返回句柄, NULL：打开失败
*******************************************************/
extern void *audio_dig_vol_open(audio_dig_vol_param *param);

/*******************************************************
* Function name	: audio_dig_vol_run
* Description	: 数字音量计算
* Parameter		:
*   @_hdl       	句柄
*   @data       	数据地址
*   @len       		数据字节长度
* Return        : 0:正常  other:出错
********************************************************/
extern int audio_dig_vol_run(void *hdl, s16 *data, u32 len);

/*******************************************************
* Function name	: audio_dig_vol_set
* Description	: 数字音量音量等级设置
* Parameter		:
*   @_hdl       	句柄
*   @channel       	通道(BIT(0)对应通道0，支持多个同时设置)
*   @vol       		音量等级
* Return        : 0:正常  other:出错
********************************************************/
extern int audio_dig_vol_set(void *hdl, u32 channel, u8 vol);

/*******************************************************
* Function name	: audio_dig_vol_get
* Description	: 数字音量等级获取
* Parameter		:
*   @_hdl       	句柄
*   @channel       	通道(BIT(0)对应通道0，如此类推)
* Return        : 通道的音量等级  -1:出错
********************************************************/
extern int audio_dig_vol_get(void *hdl, u32 channel);

/*******************************************************
* Function name	: audio_dig_max_vol_get
* Description	: 数字音量最大等级获取
* Parameter		:
*   @_hdl       	句柄
* Return        : 数字音量最大等级  -1:出错
********************************************************/
extern int audio_dig_max_vol_get(void *_hdl);

/*******************************************************
* Function name	: audio_dig_vol_skip
* Description	: 数字音量跳过计算（不对数据运算）
* Parameter		:
*   @_hdl       	句柄
*   @channel       	通道(BIT(0)对应通道0，如此类推)
*   @skip_en       	1:跳过 0:正常计算
* Return        : 0:正常  other:出错
********************************************************/
extern int audio_dig_vol_skip(void *hdl, u32 channel, u8 skip_en);

/*******************************************************
* Function name	: audio_dig_vol_fade
* Description	: 数字音量淡入淡出使能
* Parameter		:
*   @_hdl       	句柄
*   @channel       	通道(BIT(0)对应通道0，如此类推)
*   @fade_en       	1:使能 0:不使能
* Return        : 0:正常  other:出错
********************************************************/
extern int audio_dig_vol_fade(void *hdl, u32 channel, u8 fade_en);


/*******************************************************
* Function name	: audio_dig_vol_close
* Description	: 数字音量句柄关闭
* Parameter		:
*   @_hdl       	句柄
* Return        : 0:正常  other:出错
********************************************************/
extern int audio_dig_vol_close(void *_hdl);


/******************************************************************************
                audio digital vol 接入 audio stream demo

INCLUDE:
    #include "application/audio_dig_vol.h"
    void *digvol_last = NULL;
    void *digvol_last_entry = NULL;

OPEN:
    audio_dig_vol_param digvol_last_param = {
        .vol_start = 30,
        .vol_max = 30,
        .ch_total = 2,
        .fade_en = 1,
        .fade_points_step = 5,
        .fade_gain_step = 10,
        .vol_list = NULL,
    };
    digvol_last = audio_dig_vol_open(&digvol_last_param);
    digvol_last_entry = audio_dig_vol_entry_get(digvol_last);
    entries[entry_cnt++] = digvol_last_entry;

SET:
    audio_dig_vol_set(digvol_last, AUDIO_DIG_VOL_CH(0), vol);

CLOSE:
    audio_stream_del_entry(digvol_last_entry);
    audio_dig_vol_close(digvol_last);

******************************************************************************/

/*******************************************************
* Function name	: audio_dig_vol_group_open
* Description	: 新建数字音量组
* Parameter		:
* Return        : 返回句柄, NULL：打开失败
********************************************************/
extern void *audio_dig_vol_group_open(void);

/*******************************************************
* Function name	: audio_dig_vol_group_add
* Description	: 增加一个数字音量成员
* Parameter		:
*   @group_head       	数字音量组句柄
*   @dig_vol_hdl       	数字音量成员的句柄
*   @logo       		成员标识字符串
* Return        : 0:正常  other:出错
********************************************************/
extern int audio_dig_vol_group_add(void *group_head, void *dig_vol_hdl, char *logo);

/*******************************************************
* Function name	: audio_dig_vol_group_del
* Description	: 删除一个数字音量成员
* Parameter		:
*   @group_head     数字音量组句柄
*   @logo       	成员标识字符串
* Return        : 0:正常  other:出错
********************************************************/
extern int audio_dig_vol_group_del(void *group_head, char *logo);

/*******************************************************
* Function name	: audio_dig_vol_group_close
* Description	: 数字音量组解散
* Parameter		:
*   @group_head     数字音量组句柄
* Return        : 0:正常  other:出错
********************************************************/
extern int audio_dig_vol_group_close(void *group_head);


/*******************************************************
* Function name	: audio_dig_vol_group_vol_set
* Description	: 设置数字音量成员的数字音量
* Parameter		:
*   @group_head     数字音量组句柄
*   @logo       	成员标识字符串
*   @channel       	通道(BIT(0)对应通道0，支持多个同时设置)
*   @vol       		音量等级
* Return        : 0:正常  other:出错
********************************************************/
int audio_dig_vol_group_vol_set(void *group_head, char *logo, u32 channel, u8 vol);

/*******************************************************
* Function name	: audio_dig_vol_group_vol_set_by_head
* Description	: 设置数字音量成员的数字音量(匹配logo的前面部分)
* Parameter		:
*   @group_head     数字音量组句柄
*   @head       	成员标识字符串的前面部分
*   @channel       	通道(BIT(0)对应通道0，支持多个同时设置)
*   @vol       		音量等级
* Return        : 0:正常  other:出错
********************************************************/
int audio_dig_vol_group_vol_set_by_head(void *group_head, char *head, u32 channel, u8 vol);

/*******************************************************
* Function name	: audio_dig_vol_group_hdl_get
* Description	: 获取数字音量成员的数字音量句柄
* Parameter		:
*   @group_head     数字音量组句柄
*   @logo       	成员标识字符串
* Return        : 返回句柄  NULL:出错
********************************************************/
extern void *audio_dig_vol_group_hdl_get(void *group_head, char *logo);

/*******************************************************
* Function name	: audio_dig_vol_group_dodge
* Description	: 闪避
* Parameter		:
*   @group_head     数字音量组句柄
*   @logo       	指定成员标识字符串
*   @weight       	指定成员权重
*   @other_weight   其他成员权重
* Return        : 0:正常  other:出错
********************************************************/
extern int audio_dig_vol_group_dodge(void *group_head, char *logo, u8 weight, u8 other_weight);

/******************************************************************************
                audio digital group demo

OPEN:
    void *dig_vol_group1;
    dig_vol_group1 = audio_dig_vol_group_open();

ADD:
    audio_dig_vol_group_add(dig_vol_group1, dig_vol_hdl1 ,"dig_vol_1");
    audio_dig_vol_group_add(dig_vol_group1, dig_vol_hdl2 ,"dig_vol_2");
    audio_dig_vol_group_add(dig_vol_group1, dig_vol_hdl3 ,"dig_vol_3");

DEL:
    audio_dig_vol_group_del(dig_vol_group1, "dig_vol_2");

CLOSE:
    audio_dig_vol_group_close(dig_vol_group1);

DODGE:
    audio_dig_vol_group_dodge(dig_vol_group1, "dig_vol_1", 100, 0);         // dodge
    audio_dig_vol_group_dodge(dig_vol_group1, "dig_vol_1", 100, 100);       // no dodge

******************************************************************************/


/*******************************************************
* Function name	: audio_digital_vol_dual_data_run
* Description	: 双声道数字音量计算
* Parameter		:
*   @*out       	输出数据地址
*   @*in       		输入数据地址
*   @points       	数据点数
*   @vol_l			左声道音量（0-16384）
*   @vol_r			右声道音量（0-16384）
* Return        : 无
********************************************************/
void audio_digital_vol_dual_data_run(s16 *out, s16 *in, u32 points, u16 vol_l, u16 vol_r);

/*******************************************************
* Function name	: audio_digital_vol_single_data_run
* Description	: 单声道数字音量计算
* Parameter		:
*   @*out       	输出数据地址
*   @*in       		输入数据地址
*   @points       	数据点数
*   @vol			音量（0-16384）
* Return        : 无
********************************************************/
void audio_digital_vol_single_data_run(s16 *out, s16 *in, u32 points, u16 vol);

#endif  // #ifndef _AUDIO_DIG_VOL_H_
