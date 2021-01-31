#ifndef _AUDIO_LINK_H_
#define _AUDIO_LINK_H_

//ch_num
#define ALINK_CH0    	0
#define ALINK_CH1    	1
#define ALINK_CH2    	2
#define ALINK_CH3    	3

#define ALNK_BUF_POINTS_NUM 		128

typedef enum {
    ALINK0_PORTA,            //MCLK:PA8 SCLK:PA2  LRCK:PA3 CH0:PA4 CH1:PA5 CH2:PA6 CH3:PA7
    ALINK0_PORTB,            //MCLK:PA15 SCLK:PA9  LRCK:PA10 CH0:PA11 CH1:PA12 CH2:PA13 CH3:PA14
    ALINK1_PORTA,            //MCLK:PB0 SCLK:PC0  LRCK:PC1 CH0:PC2 CH1:PC3 CH2:PC4 CH3:PC5
} ALINK_PORT;

//ch_dir
typedef enum {
    ALINK_DIR_TX	= 0u,
    ALINK_DIR_RX		,
} ALINK_DIR;

typedef enum {
    ALINK_LEN_16BIT = 0u,
    ALINK_LEN_24BIT		, //ALINK_FRAME_MODE需要选择: ALINK_FRAME_64SCLK
} ALINK_DATA_WIDTH;

//ch_mode
typedef enum {
    ALINK_MD_NONE	= 0u,
    ALINK_MD_IIS		,
    ALINK_MD_IIS_LALIGN	,
    ALINK_MD_IIS_RALIGN	,
    ALINK_MD_DSP0		,
    ALINK_MD_DSP1		,
} ALINK_MODE;

//ch_mode
typedef enum {
    ALINK_ROLE_MASTER, //主机
    ALINK_ROLE_SLAVE,  //从机
} ALINK_ROLE;

typedef enum {
    ALINK_CLK_FALL_UPDATE_RAISE_SAMPLE, //下降沿更新数据, 上升沿采样数据
    ALINK_CLK_RAISE_UPDATE_FALL_SAMPLE, //上降沿更新数据, 下升沿采样数据
} ALINK_CLK_MODE;

typedef enum {
    ALINK_FRAME_32SCLK, 	//32 sclk/frame
    ALINK_FRAME_64SCLK, 	//64 sclk/frame
} ALINK_FRAME_MODE;

typedef enum {
    ALINK_SR_48000 = 48000,
    ALINK_SR_44100 = 44100,
    ALINK_SR_32000 = 32000,
    ALINK_SR_24000 = 24000,
    ALINK_SR_22050 = 22050,
    ALINK_SR_16000 = 16000,
    ALINK_SR_12000 = 12000,
    ALINK_SR_11025 = 11025,
    ALINK_SR_8000  = 8000,
} ALINK_SR;

struct alnk_ch_cfg {
    u8 enable;
    ALINK_DIR dir; 				//通道传输数据方向: Tx, Rx
    void *buf;					//dma buf地址
    void (*isr_cb)(u8 ch, s16 *buf, u32 len);
};

//===================================//
//多个通道使用需要注意:
//1.数据位宽需要保持一致
//2.buf长度相同
//===================================//
typedef struct _ALINK_PARM {
    u8 port_select;
    struct alnk_ch_cfg ch_cfg[4];		//通道内部配置
    ALINK_MODE mode; 					//IIS, left, right, dsp0, dsp1
    ALINK_ROLE role; 			//主机/从机
    ALINK_CLK_MODE clk_mode; 			//更新和采样边沿
    ALINK_DATA_WIDTH  bitwide;   //数据位宽16/32bit
    ALINK_FRAME_MODE sclk_per_frame;  	//32/64 sclk/frame
    u16 dma_len; 						//buf长度: byte
    ALINK_SR sample_rate;					//采样率
} ALINK_PARM;

int alink_init(ALINK_PARM *parm);  //iis 初始化
int	alink_start(ALINK_PORT port);             //iis 开启
void alink_channel_init(ALINK_PORT port, u8 ch_idx, u8 dir, void (*handle)(u8 ch, s16 *buf, u32 len));   //iis 打开通道
void alink_channel_close(ALINK_PORT port, u8 ch_idx); //iis 关闭通道
int alink_sr_set(ALINK_PORT port, u16 sr); 			//iis 设置采样率
void alink_uninit(ALINK_PORT port); 			//iis 退出

void audio_link_init(ALINK_PORT port);
void audio_link_uninit(ALINK_PORT port);

#endif
