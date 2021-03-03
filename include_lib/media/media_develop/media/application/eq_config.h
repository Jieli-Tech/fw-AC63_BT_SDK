#ifndef _EQ_CONFIG_H_
#define _EQ_CONFIG_H_

#include "typedef.h"
#include "asm/hw_eq.h"
#include "spinlock.h"
#include "application/audio_eq.h"
#include "application/audio_drc.h"
#include "math.h"

/*-----------------------------------------------------------*/
/*eq online cmd*/
enum {
    EQ_ONLINE_CMD_SECTION       = 1,
    EQ_ONLINE_CMD_GLOBAL_GAIN,
    EQ_ONLINE_CMD_LIMITER,
    EQ_ONLINE_CMD_INQUIRE,
    EQ_ONLINE_CMD_GETVER,
    EQ_ONLINE_CMD_GET_SOFT_SECTION,//br22专用
    EQ_ONLINE_CMD_GET_SECTION_NUM = 0x7,//工具查询 小机需要的eq段数
    EQ_ONLINE_CMD_GLOBAL_GAIN_SUPPORT_FLOAT = 0x8,


    EQ_ONLINE_CMD_PASSWORD = 0x9,
    EQ_ONLINE_CMD_VERIFY_PASSWORD = 0xA,
    EQ_ONLINE_CMD_FILE_SIZE = 0xB,
    EQ_ONLINE_CMD_FILE = 0xC,
    EQ_EQ_ONLINE_CMD_GET_SECTION_NUM = 0xD,//该命令新加与 0x7功能一样
    EQ_EQ_ONLINE_CMD_CHANGE_MODE = 0xE,//切模式

    EQ_ONLINE_CMD_MODE_COUNT = 0x100,//模式个数a  1
    EQ_ONLINE_CMD_MODE_NAME = 0x101,//模式的名字a eq
    EQ_ONLINE_CMD_MODE_GROUP_COUNT = 0x102,//模式下组的个数,4个字节 1
    EQ_ONLINE_CMD_MODE_GROUP_RANGE = 0x103,//模式下组的id内容  0x11
    EQ_ONLINE_CMD_EQ_GROUP_COUNT = 0x104,//eq组的id个数  1
    EQ_ONLINE_CMD_EQ_GROUP_RANGE = 0x105,//eq组的id内容 0x11
    EQ_ONLINE_CMD_MODE_SEQ_NUMBER = 0x106,//mode的编号  magic num


    EQ_ONLINE_CMD_PARAMETER_SEG = 0x11,
    EQ_ONLINE_CMD_PARAMETER_TOTAL_GAIN,
    EQ_ONLINE_CMD_PARAMETER_LIMITER,
    EQ_ONLINE_CMD_PARAMETER_DRC,
    EQ_ONLINE_CMD_PARAMETER_CHANNEL,//通道切换


    EQ_ONLINE_CMD_SONG_EQ_SEG = 0x2001,
    EQ_ONLINE_CMD_CALL_EQ_SEG = 0x2002,
    EQ_ONLINE_CMD_AEC_EQ_SEG = 0x2003,

    EQ_ONLINE_CMD_SONG_DRC = 0x2101,
//add xx here

    EQ_ONLINE_CMD_MAX,//最后一个
};



/*eq online packet*/
typedef struct {
    int cmd;     			///<EQ_ONLINE_CMD
    int data[45];       	///<data
} EQ_ONLINE_PACKET;


/*EQ_ONLINE_CMD_PARAMETER_SEG*/
typedef struct eq_seg_info EQ_ONLINE_PARAMETER_SEG;

/*-----------------------------------------------------------*/
typedef struct eq_seg_info EQ_CFG_SEG;

typedef struct {
    float global_gain;
    int seg_num;
    int enable_section;
} CFG_PARM;

#ifdef CONFIG_CPU_BR23
#define SECTION_MAX 32
#else
#define SECTION_MAX 10
#endif
// #define MODE_NUM_MAX  6
typedef struct {
    CFG_PARM par;
    // EQ_CFG_SEG *seg;//[EQ_SECTION_MAX];
    EQ_CFG_SEG seg[SECTION_MAX];
} EQ_CFG_PARAMETER;


typedef struct {
    struct drc_ch drc;
} DRC_CFG_PARAMETER;


typedef struct {
    /* unsigned short crc; */
    unsigned short id;
    unsigned short len;
} EQ_FILE_HEAD;

typedef struct {
    EQ_FILE_HEAD head;
    EQ_CFG_PARAMETER parm;
}  EQ_CFG_PARM;

typedef struct {
    EQ_FILE_HEAD head;
    DRC_CFG_PARAMETER parm;
}  DRC_CFG_PARM;

typedef struct {
    EQ_CFG_PARM song_eq_parm;
    DRC_CFG_PARM drc_parm;
    u8 cur_mode;
} EQ_FILE_PARM;

typedef struct {
    u16 tmr;
    u8 *fade_stu;
    u32 *sr/* [eq_mode_num] */;
    // EQ_CFG_SEG seg[MODE_NUM_MAX][SECTION_MAX];
    EQ_CFG_SEG *seg;
} EQ_FADE_CFG;

typedef struct _eq_tool_cfg {
    u8 mode_index;       //模式序号
    u8 *mode_name;       //模式名
    u32 mode_seq;        //模式的seq,用于识别离线文件功能类型
    u8 section;          //每个模式下eq的段数
    u8 fun_num;          //模式下有多少种功能
    u16 fun_type[2];     //模式下拥有哪些功能
} eq_tool_cfg;

typedef struct {
    u32 eq_type : 3;
    u32 *mode_updata;
    u32 *drc_updata;
    u32 *online_updata;
    u32 *design_mask;
    u32 *seg_num;
    EQ_FILE_PARM *cfg_parm;
    u32 *cur_sr;
    spinlock_t lock;
    u8 mode_id;

    u8 mode_num;
    u8 online_en: 1;
    u8 fade_en: 1;
    u8 stero: 1;
    u8 drc: 1;
    u8 tws: 1;
    u8 app: 1;
    u8 limit_zero: 1;
    u8 eq_file_ver_err: 1;
    u8 eq_file_section_err : 1;
    u8 type_num;
    u8 section_max;
    u8 eq_mode;//默认eq系数表的类型
    u8 parse_seq;
    uint8_t password_ok;
    // int eq_coeff_tab[MODE_NUM_MAX][SECTION_MAX * 5];//eq在线多模式系数表
    int *eq_coeff_tab;//eq在线多模式系数表

    void *tws_ci;
    int tws_tmr;
    u16 *tws_pack;
    u8 pack_id;

    EQ_FADE_CFG *fade_cfg;
    int *eq_type_tab;//[EQ_MODE_MAX];
    float *type_gain_tab;//song_eq_mode eq 默认系数表的对应的总增益
    u8 *eq_mode_use_idx;//10段内抽取部分段数
    eq_tool_cfg *eq_tool_tab;

    void *phone_eq_tab;//通话下行 eq系数表
    u8 phone_eq_tab_size;//通话下行eq系数表段数
    void *ul_eq_tab;//通话上行 eq系数表
    u8 ul_eq_tab_size;//通话上行eq系数表段数


} EQ_CFG;

typedef struct _eq_parm {
    u8 mode_num;
    u8 online_en: 1;
    u8 fade_en: 1;
    u8 file_en: 1;
    u8 stero: 1; //是否左右声道做不同eq
    // u8 four: 1; //是否四声道
    u8 drc: 1; //是否有drc
    u8 tws: 1; //是否tws
    u8 app: 1; //是否手机app在线调试
    u8 limit_zero: 1;
    u8 type_num: 4;
    u8 section_max;
    void *eq_type_tab;//eq 默认系数表
    float *type_gain_tab;//song_eq_mode eq 默认系数表的对应的总增益
    u8 *eq_mode_use_idx;//10段内抽取部分段数
    eq_tool_cfg *eq_tool_tab;

    void *phone_eq_tab;//通话下行 eq系数表
    u8 phone_eq_tab_size;//通话下行eq系数表段数
    void *ul_eq_tab;//通话上行 eq系数表
    u8 ul_eq_tab_size;//通话上行eq系数表段数

} eq_adjust_parm;


/*模式序号*/
#define call_eq_mode  		 0
#define call_narrow_eq_mode  1
#define aec_eq_mode          2
#define aec_narrow_eq_mode   3
#define song_eq_mode  		 4
#define fr_eq_mode    		 5
#define rl_eq_mode           6
#define rr_eq_mode           7
#define mic_eq_mode          8

int *get_eq_coeff_tab(EQ_CFG *eq_cfg, u8 mode);
void *get_eq_cfg_hdl();
void eq_seg_limit_zero(EQ_CFG_SEG *seg, u8 seg_num);
float get_glbal_gain(EQ_CFG *eq_cfg, u8 mode);
void eq_coeff_set(EQ_CFG *eq_cfg, u32 sr, u8 mode, EQ_CFG_SEG *seg, int *tar_tab);
void *eq_cfg_open(eq_adjust_parm *parm);
int eq_app_online_parse(u8 *packet, u8 size, u8 *ext_data, u16 ext_size);
void set_global_gain(EQ_CFG *eq_cfg, u8 mode, float global_gain);
float get_glbal_gain(EQ_CFG *eq_cfg, u8 mode);

s8 eq_mode_get_gain(u8 mode, u16 index);
int eq_mode_set_custom_param(u16 index, int gain);
int eq_mode_set_custom_info(u16 index, int freq, int gain);
int eq_mode_set(u8 mode);
int eq_mode_get_cur(void);



s32 eq_file_get_cfg(EQ_CFG *eq_cfg, u8 *path);
int eq_get_filter_info(void *_eq, int sr, struct audio_eq_filter_info *info);
int drc_get_filter_info(void *_drc, struct audio_drc_filter_info *info);
int aec_ul_eq_filter(void *_eq, int sr, struct audio_eq_filter_info *info);
int eq_phone_get_filter_info(void *_eq, int sr, struct audio_eq_filter_info *info);

extern const int config_audio_eq_online_en;
extern const int config_audio_eq_file_en;
extern const int config_audio_eq_file_sw_en;
extern const int config_filter_coeff_fade_en;
extern const int config_filter_coeff_limit_zero;
#endif

