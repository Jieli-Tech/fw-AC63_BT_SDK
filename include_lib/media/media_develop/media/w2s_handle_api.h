#ifndef w2s_handle_api_h__
#define w2s_handle_api_h__

#ifndef U32

#endif

#define MAX_WORD   10

/* typedef struct _MIDI_W2S_STRUCT_ */
// {
// unsigned int word_cnt;                           //多少个字
// unsigned int data_pos[MAX_WORD + 1];               //数据起始位置
// unsigned int data_len[MAX_WORD + 1];               //数据长度
// unsigned short* rec_data;
// char key_diff;
/* }MIDI_W2S_STRUCT; */

typedef struct _MIDI_W2S_STRUCT_ {
    unsigned int word_cnt;                           //多少个音
    unsigned int data_pos[MAX_WORD + 1];             //每个音起始位置
    unsigned int data_len[MAX_WORD + 1];             //每个音数据长度(short)
    unsigned int rec_data;							 //pcm 数据  MIDI_TONE_MODE变量是0时用pcm数据；是1时读pcm文件地址
    char key_diff;									 //与音高成反比 建议范围[-12,12]
} MIDI_W2S_STRUCT;

typedef struct _REC_W2S_STRUCT_ {
    short *rec_data;
    unsigned int rec_len;
    unsigned char toneadjust_enable;
    unsigned char multi_enable;
    unsigned short energy_thresh;
} REC_W2S_STUCT;


typedef struct _w2s_ana_ops_ {
    unsigned int (*need_buf)();                                       //申请需要的buf
    int (*open)(unsigned char *ptr, REC_W2S_STUCT *ptr_parm_i, MIDI_W2S_STRUCT *ptr_parm_o);      //外面配置的参数
    int (*run)(unsigned char *ptr, short *data, int len);     //处理外，传出处理过的函数结果
} w2s_ana_ops;

extern w2s_ana_ops  *get_w2s_ans_ops();

#endif // w2s_handle_api_h__
