
#ifndef MIDI_DEC_API_h__
#define MIDI_DEC_API_h__
#include "w2s_handle_api.h"

#define  test_or32_midi_n     0
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

//sdk添加const int MAX_PLAYER_CNT = 18；同时发声可配置的最大数[1,32]

typedef struct _MIDI_CONFIG_PARM_ {

    unsigned int spi_pos;		//	sdk添加 const int  MIDI_TONE_MODE = 0;   这个变量控制是0用音色地址，还是1读音色文件。
    unsigned char sample_rate;	//	采样率0 - 8 对应采样率表{48000,44100,32000,24000,22050,16000,12000,11025,8000}
    short player_t;				//  最多同时发声的key数,最大值为MAX_PLAYER_CNT

    int(*fread)(void *file, void *buf, u32 len);
    int(*fseek)(void *file, u32 offset, int seek_mode);
} MIDI_CONFIG_PARM;



#define  VOL_Norm_Bit                12

#define  CTRL_CHANNEL_NUM             16

typedef struct _EX_CH_VOL_PARM_ {
    unsigned short  cc_vol[CTRL_CHANNEL_NUM];                 //16个通道或轨道的音量      <=>4096等于原音量
    unsigned char ex_vol_use_chn;							  //ex_vol_use_chn = 0 轨道音量；ex_vol_use_chn = 1 通道音量
} EX_CH_VOL_PARM;


typedef  struct _EX_INFO_STRUCT_ {
    void *priv;
    u32(*mark_trigger)(void *priv, u8 *val, u8 len);			//标记回调，val 标记名称  len 字节数
} EX_INFO_STRUCT;


typedef  struct _EX_TmDIV_STRUCT_ {					 //小节回调
    void *priv;
    u32(*timeDiv_trigger)(void *priv);
} EX_TmDIV_STRUCT;

typedef  struct _EX_BeatTrig_STRUCT_ {				//节拍回调/*val1 一节多少拍,  val2每拍多少分音符*/
    void *priv;
    u32(*beat_trigger)(void *priv, u8 val1, u8 val2);
} EX_BeatTrig_STRUCT;

typedef  struct _EX_MELODY_STRUCT_ {				//主旋律音符回调 vel 为按键力度
    void *priv;
    u32(*melody_trigger)(void *priv, u8 key, u8 vel);
} EX_MELODY_STRUCT;


typedef struct _EX_MELODY_STOP_STRUCT_ {			//主旋律音符停止回调
    void *priv;
    u32(*melody_stop_trigger)(void *priv, u8 key);
} EX_MELODY_STOP_STRUCT;

#define  CMD_MODE4_PLAY_END          0x09

enum {
    CMD_MIDI_SEEK_BACK_N = 0xa0,		//小节回调								  MIDI_SEEK_BACK_STRUCT 变量
    CMD_MIDI_SET_CHN_PROG,			//配置主通道乐器或者所有的通道乐器	  MIDI_PROG_CTRL_STRUCT 变量
    CMD_MIDI_CTRL_TEMPO,			//配置节奏及衰减					  MIDI_PLAY_CTRL_TEMPO 变量
    CMD_MIDI_GOON,					//okon 发声                          参数为空
    CMD_MIDI_CTRL_MODE,				//配置midi模式						  MIDI_PLAY_CTRL_MODE 变量
    CMD_MIDI_SET_SWITCH,			//配置使能		                      MIDI_SET_SWITCH 变量
    CMD_MIDI_SET_EX_VOL,			//配置外部音量						  EX_CH_VOL_PARM 变量
    CMD_INIT_CONFIG,				//初始化配置						  MIDI_INIT_STRUCT 变量
    CMD_MIDI_OKON_MODE				//配置OKON模式						  MIDI_OKON_MODE 变量
};

enum {
    CMD_MIDI_CTRL_MODE_0 = 0X00,		 // 正常解码
    CMD_MIDI_CTRL_MODE_1 = 0X01,	//OKON 模式
    CMD_MIDI_CTRL_MODE_2,					// 只推消息，不出声
    CMD_MIDI_CTRL_MODE_W2S                   //外部音源
};

enum {
    CMD_MIDI_OKON_MODE_0 = 0x00,	//主旋 okon
    CMD_MIDI_OKON_MODE_1			//主副旋一起 okon
};

enum {
    CMD_MIDI_MELODY_KEY_0 = 0x00,	//melody_trigger 回调当前音符
    CMD_MIDI_MELODY_KEY_1			// melody_trigger 回调下一个音符
};


typedef  struct _MIDI_PLAY_CTRL_MODE_ {
    u8 mode;
} MIDI_PLAY_CTRL_MODE;				//用于配置midi模式

typedef struct _MIDI_OKON_MODE_ {
    u8 Melody_Key_Mode;
    u8 OKON_Mode;
} MIDI_OKON_MODE;					//用于配置OKON模式

typedef  struct _MIDI_PLAY_CTRL_TEMPO_ {
    u16 tempo_val;
    u16 decay_val[CTRL_CHANNEL_NUM];             //1024 低11bit有效
    u32 mute_threshold;
} MIDI_PLAY_CTRL_TEMPO;			//用于配置节奏及衰减



typedef struct _MIDI_CHNNEL_CTRL_STRUCT_ {
    u8 chn;
} MIDI_CHNNEL_CTRL_STRUCT;

typedef struct _MIDI_PROG_CTRL_STRUCT_ {
    u8 prog;				  //乐器号
    u8 replace_mode;         //replace_mode==1，就是 替换所有通道； 否则替换主通道
    u16 ex_vol;              //1024是跟原来一样大， 变化为原来的(ex_vol/1024)倍
} MIDI_PROG_CTRL_STRUCT;		 //用于配置主通道乐器或者替换所有的通道

typedef struct _MIDI_SEEK_BACK_STRUCT_ {
    s8 seek_back_n;
} MIDI_SEEK_BACK_STRUCT;		//回调多少小节


enum {
    MARK_ENABLE = 0x0001,                  //mark回调的使能
    MELODY_ENABLE = 0x0002,                //主旋律音符回调的使能
    TIM_DIV_ENABLE = 0x0004,               //小节回调的使能
    MUTE_ENABLE = 0x0008,                  //mute住解码的使能
    SAVE_DIV_ENBALE = 0x0010,               //小节保存的使能
    EX_VOL_ENABLE = 0x0020,                 //外部音量控制使能
    SET_PROG_ENABLE = 0x0040,               //主轨道设置成固定乐器使能
    MELODY_PLAY_ENABLE = 0x0080,             //主轨道播放使能
    BEAT_TRIG_ENABLE = 0x0100,                //每拍回调的使能
    MELODY_STOP_ENABLE = 0x200,				//主旋律音符停止回调使能
    MARK_LOOP_ENABLE = 0x400				//使用mark做循环播放使能

};


typedef struct _MIDI_INIT_STRUCT_ {
    MIDI_CONFIG_PARM init_info;               //初始化参数
    MIDI_PLAY_CTRL_MODE mode_info;            //控制模式
    MIDI_PLAY_CTRL_TEMPO  tempo_info;         //节奏参数
    EX_CH_VOL_PARM  vol_info;                 //外部音量控制
    MIDI_PROG_CTRL_STRUCT prog_info;          //主轨道乐器参数
    MIDI_CHNNEL_CTRL_STRUCT  mainTrack_info;  //主轨道设置参数 sdk添加 const int  MAINTRACK_USE_CHN = 0;   这个变量控制是0用track号来区分主通道，还是1用chn的编号来区分主通道。
    EX_INFO_STRUCT  mark_info;                //mark回调函数
    EX_MELODY_STRUCT moledy_info;             //melody回调函数
    EX_TmDIV_STRUCT  tmDiv_info;              //小节回调参数
    EX_BeatTrig_STRUCT beat_info;             //每拍回调参数
    MIDI_OKON_MODE okon_info;				  //OKON参数
    EX_MELODY_STOP_STRUCT moledy_stop_info;	 //melody_stop回调函数
#if 1
    MIDI_W2S_STRUCT    w2s_info;
#endif
    u32   switch_info;              //初始化一些使能位，默认为0
} MIDI_INIT_STRUCT;


#if 0

void init_midi_info_val(MIDI_INIT_STRUCT  *midi_init_info_v)  //midi 配置
{
    //midi初始化表
    midi_init_info_v->init_info.player_t = 8;
    midi_init_info_v->init_info.sample_rate = 5;
    midi_init_info_v->init_info.spi_pos = spi_memory; //音色地址或音色文件

    //midi的模式初始化
    midi_init_info_v->mode_info.mode = 0;

    //midi节奏初始化
    midi_init_info_v->tempo_info.tempo_val = 1024;
    for (int i = 0; i < 16; i++) {
        midi_init_info_v->tempo_info.decay_val[i] = ((u16)31 << 11) | 1024;
    }

    midi_init_info_v->tempo_info.mute_threshold = (u16)1L << 29;

    //midi主轨道初始化
    midi_init_info_v->mainTrack_info.chn = 0;   //把哪个轨道当成主轨道

    //midi外部音量初始化
    {
        u32 tmp_i;
        for (tmp_i = 0; tmp_i < 16; tmp_i++) {
            midi_init_info_v->vol_info.cc_vol[tmp_i] = 4096;   //4096即原来的音量
        }
    }

    //midi的主轨道乐器设置
    midi_init_info_v->prog_info.prog = 0;
    midi_init_info_v->prog_info.ex_vol = 1024;
    midi_init_info_v->prog_info.replace_mode = 0;

    //OKON模式设置
    midi_init_info_v->okon_info.Melody_Key_Mode = CMD_MIDI_MELODY_KEY_0;
    midi_init_info_v->okon_info.OKON_Mode = CMD_MIDI_MELODY_KEY_0;

    //midi的mark控制初始化
    midi_init_info_v->mark_info.priv = &file_mark;
    midi_init_info_v->mark_info.mark_trigger = tmark_trigger;

    //midi的melody控制初始化
    midi_init_info_v->moledy_info.priv = &file_melody;
    midi_init_info_v->moledy_info.melody_trigger = melody_trigger;

    //midi的melody stop控制初始化
    midi_init_info_v->moledy_stop_info.priv = &file_melody_stop;
    midi_init_info_v->moledy_stop_info.melody_stop_trigger = melody_stop_trigger;

    //midi的melody stop控制初始化
    midi_init_info_v->moledy_stop_info.priv = NULL;
    midi_init_info_v->moledy_stop_info.melody_stop_trigger = melody_stop_trigger;

    //midi的小节回调控制初始化
    midi_init_info_v->tmDiv_info.priv = NULL;
    midi_init_info_v->tmDiv_info.timeDiv_trigger = timDiv_trigger;

    //midi的小拍回调控制初始化
    midi_init_info_v->beat_info.priv = NULL;
    midi_init_info_v->beat_info.beat_trigger = beat_trigger;

    //使能位控制
    midi_init_info_v->switch_info = 0;
}

void midi_w2s_parm(MIDI_INIT_STRUCT *midi_init_info_v)
{
#if 1

    FILE *fpt = fopen("parmout.raw", "rb");

    if (fpt != NULL) {
        fread(&midi_init_info_v->w2s_info, 1, sizeof(midi_init_info_v->w2s_info), fpt);//录音预处理后的输出参数

        fclose(fpt);
    }
    midi_init_info_v->w2s_info.key_diff = -7;        //与音高成反比
    midi_init_info_v->w2s_info.rec_data = dataw2s;	//录音预处理后的输出pcm数据地址或录音预处理后输出pcm数据保存的文件地址

#endif

}


{
    MIDI_OKON_MODE okon_mode;
    MIDI_PLAY_CTRL_MODE ctrl_mode_obj;

    ctrl_mode_obj.mode = CMD_MIDI_CTRL_MODE_1;
    okon_mode.OKON_Mode = CMD_MIDI_OKON_MODE_1;
    okon_mode.Melody_Key_Mode = CMD_MIDI_MELODY_KEY_0;

    init_midi_info_val(&midi_init_parm);
    midi_w2s_parm(&midi_init_parm);

    {
        u32 switch_val_in;
        audio_decoder_ops *test_pos = get_midi_ops(); //获取句柄
        buflen = test_pos->need_dcbuf_size();		  //获取midi buf
        buflen = buflen;
        bufptr = malloc(buflen);
        test_pos->open(bufptr, &my_dec_io0, 0);		  // 打开midi

        test_pos->dec_confing(bufptr, CMD_INIT_CONFIG, &midi_init_parm);  //初始化
//		test_pos->dec_confing(bufptr, CMD_MIDI_CTRL_MODE, &ctrl_mode_obj);  //更改midi模式为okon
//		test_pos->dec_confing(bufptr, CMD_MIDI_OKON_MODE, &okon_mode);		//更改okon的模式


//		ctrl_mode_obj.mode = CMD_MIDI_CTRL_MODE_W2S;
//		test_pos->dec_confing(bufptr, CMD_MIDI_CTRL_MODE, &mode_parm);//更改midi模式为外部音源

        int cj = 0;
        if (!test_pos->format_check(bufptr)) { //检查midi文件格式

            testing_cnt = 0;
            while (!retv) {
                retv = test_pos->run(bufptr, 0);

                /*testing_cnt++;
                if (testing_cnt == 800)
                {

                	test_pos->dec_confing(bufptr, CMD_MIDI_GOON, NULL); //okon 模式需要调用CMD_MIDI_GOON
                	testing_cnt = 0;
                }*/
            }
        }


#endif

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

