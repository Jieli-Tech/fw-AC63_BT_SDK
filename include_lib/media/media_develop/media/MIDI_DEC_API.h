
#ifndef MIDI_DEC_API_h__
#define MIDI_DEC_API_h__


#define  test_or32_midi_n     0
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct _MIDI_CONFIG_PARM_ {

    unsigned int spi_pos;
    unsigned char sample_rate;//数字0~8,采样率对应48000，44100，32000，24000，22050，16000， 12000， 11025，8000
    short player_t;//最大播放轨道0 - 18, 设置的超过18，会强制转为18 (支持多少个key同时发声)

    int(*fread)(void *file, void *buf, u32 len);
    int(*fseek)(void *file, u32 offset, int seek_mode);
} MIDI_CONFIG_PARM;



#define  VOL_Norm_Bit                12

#define  CTRL_CHANNEL_NUM             16

typedef struct _EX_CH_VOL_PARM_ {
    unsigned short  cc_vol[CTRL_CHANNEL_NUM];                 //16个通道的音量      <=>8192等于原音量
} EX_CH_VOL_PARM;


typedef  struct _EX_INFO_STRUCT_ {
    void *priv;
    u32(*mark_trigger)(void *priv, u8 *val, u8 len);
} EX_INFO_STRUCT;


typedef  struct _EX_TmDIV_STRUCT_ {
    void *priv;
    u32(*timeDiv_trigger)(void *priv);
} EX_TmDIV_STRUCT;

typedef  struct _EX_BeatTrig_STRUCT_ {
    void *priv;
    u32(*beat_trigger)(void *priv, u8 val1, u8 val2);
} EX_BeatTrig_STRUCT;

typedef  struct _EX_MELODY_STRUCT_ {
    void *priv;
    u32(*melody_trigger)(void *priv, u8 key, u8 vel);
} EX_MELODY_STRUCT;

#define  CMD_MODE4_PLAY_END          0x09

enum {
    CMD_MIDI_SEEK_BACK_N = 0x00,      //小节回调,参数结构对应 MIDI_SEEK_BACK_STRUCT
    CMD_MIDI_SET_CHN_PROG,            //更改乐器,参数结构对应 MIDI_PROG_CTRL_STRUCT
    CMD_MIDI_CTRL_TEMPO,              //改变节奏,参数结构对应 MIDI_PLAY_CTRL_TEMPO
    CMD_MIDI_GOON,                    //one key one note的时候接着播放使用,参数为空
    CMD_MIDI_CTRL_MODE,               //改变模式,参数结构对应 MIDI_PLAY_CTRL_MO
    CMD_MIDI_SET_SWITCH,              //配置开关使能，要不要替换乐器，使用外部音量,参数对应 MIDI_SET_SWITCH
    CMD_MIDI_SET_EX_VOL,              //设置外部声道音量,参数结构对应 EX_CH_VOL_PARM
    CMD_INIT_CONFIG                   //初始化银色表，合成的采样率，参数结构对应 MIDI_INIT_STRUCT
};

enum {
    //CMD_MIDI_CTRL_MODE_0        =0X00,	//默认模式
    CMD_MIDI_CTRL_MODE_1 = 0X01,			// one key one note
    CMD_MIDI_CTRL_MODE_2,
    CMD_MIDI_CTRL_MODE_W2S                   //外部音源
};



typedef  struct _MIDI_PLAY_CTRL_MODE_ {
    u8 mode;
} MIDI_PLAY_CTRL_MODE;

typedef  struct _MIDI_PLAY_CTRL_TEMPO_ {
    u16 tempo_val;//是放大了10bit
    u16 decay_val;             //1024 decay_val 其实是1024起作用，放大了10bit
    u32 mute_threshold;//是放大了29bit
} MIDI_PLAY_CTRL_TEMPO;



typedef struct _MIDI_CHNNEL_CTRL_STRUCT_ {
    u8 chn;//这个设置主轨道超过大于等于17就是没有设置主轨道
} MIDI_CHNNEL_CTRL_STRUCT;

typedef struct _MIDI_PROG_CTRL_STRUCT_ {
    u8 prog;
    u8 replace_mode;         //replace_mode==1，就是 替换所有通道； 否则替换主通道
    u16 ex_vol;              //1024是跟原来一样大， 变化为原来的(ex_vol/1024)倍,ex_vol是放大了10bit
} MIDI_PROG_CTRL_STRUCT;

typedef struct _MIDI_SEEK_BACK_STRUCT_ {
    s8 seek_back_n;
} MIDI_SEEK_BACK_STRUCT;


#define MAX_WORD   10

typedef struct _MIDI_W2S_STRUCT_ {
    unsigned int word_cnt;                           //多少个字
    unsigned int data_pos[MAX_WORD + 1];               //数据起始位置
    unsigned int data_len[MAX_WORD + 1];               //数据长度
    unsigned short *rec_data;
    char key_diff;
} MIDI_W2S_STRUCT;

typedef enum {
    MARK_ENABLE = 0x0001,                  //mark回调的使能
    MELODY_ENABLE = 0x0002,                //melody回调的使能
    TIM_DIV_ENABLE = 0x0004,               //小节回调的使能
    MUTE_ENABLE = 0x0008,                  //mute住解码的使能
    SAVE_DIV_ENBALE = 0x0010,               //小节保存的使能
    EX_VOL_ENABLE = 0x0020,                 //外部音量控制使能
    SET_PROG_ENABLE = 0x0040,               //主轨道设置成固定乐器使能
    MELODY_PLAY_ENABLE = 0x0080,             //主轨道播放使能
    BEAT_TRIG_ENABLE = 0x0100                //每拍回调的使能
} MIDI_SET_SWITCH;


typedef struct _MIDI_INIT_STRUCT_ {
    MIDI_CONFIG_PARM init_info;               //初始化参数
    MIDI_PLAY_CTRL_MODE mode_info;            //控制模式
    MIDI_PLAY_CTRL_TEMPO  tempo_info;         //节奏参数
    EX_CH_VOL_PARM  vol_info;                 //外部音量控制
    MIDI_PROG_CTRL_STRUCT prog_info;          //主轨道乐器参数
    MIDI_CHNNEL_CTRL_STRUCT  mainTrack_info;  //主轨道设置参数
    EX_INFO_STRUCT  mark_info;                //mark回调函数
    EX_MELODY_STRUCT moledy_info;             //melody回调函数
    EX_TmDIV_STRUCT  tmDiv_info;              //小节回调参数
    EX_BeatTrig_STRUCT beat_info;             //每拍回调参数
#if 1
    MIDI_W2S_STRUCT    w2s_info;
#endif
    u32   switch_info;              //初始化一些使能位，默认为0
} MIDI_INIT_STRUCT;


#if 0
外挂flash baud跑 40M ，midi读数解码 110M.
外挂flash baud跑 60M ，midi读数解码80M.
// midi采样率表sample_rate 0 对应48000, 4对应22050，采样率越大mips消耗越大
const int smpl_rate_tab[9] = {
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

int swtic_cc = MARK_ENABLE | MELODY_ENABLE;
audio_decoder_ioctrl(hdl, CMD_MIDI_SET_SWITCH, &swtic_cc);        //调用接口跟其它解码一致，但是需要调用这个函数


#endif

#endif // MIDI_DEC_API_h__


