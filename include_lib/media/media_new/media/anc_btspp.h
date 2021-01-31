#ifndef _ANC_BTSPP_H_
#define _ANC_BTSPP_H_

#include "generic/typedef.h"
#include "asm/anc.h"

//前期训练 建议先做喇叭测试，再做MIC测试。这样内部可以通过喇叭判断MIC好不好
//-------------------ANC SPP 协议 -------------------//

//父命令
#define	ANC_COMMAND_RESPO			0x00		//命令回复
#define ANC_TRAIN_NOMA_MODE			0x01 		//普通训练模式
#define ANC_TRAIN_FAST_MODE			0x02 		//快速训练模式

//训练工具子命令1
#define ANC_CHANGE_COMMAND			0x00		//父命令模式切换
#define	ANC_MUTE_TARIN				0x01		//静音训练
#define	ANC_NOISE_TARIN   			0x02 		//噪声训练
#define ANC_TARIN_AGAIN    			0x03		//继续训练
#define ANC_MODE_OFF				0x04		//ANC关
#define ANC_MODE_ON					0x05		//ANC降噪模式
#define ANC_TRAIN_EXIT				0x06		//ANC训练结束

#define ANC_PASS_MODE_ON			0x07		//ANC通透模式
#define ANC_TRAIN_STEP_1			0x08		//ANC训练步进1
#define ANC_TRAIN_STEP_2			0x09		//ANC训练步进2

#define ANC_REF_MIC_GAIN_SET		0x11		//设置参考MIC的增益
#define ANC_ERR_MIC_GAIN_SET		0x12		//设置误差MIC的增益
#define ANC_DAC_GAIN_SET			0x13		//设置DAC的模拟增益
#define ANC_FFGAIN_SET				0x14		//设置ANC的数字增益

#define ANC_TRAIN_STEP_SET          0x15		//设置ANC训练步进
#define ANC_SZ_LOW_THR_SET          0x16        //设置SZ MIC下限能量阈值
#define ANC_FZ_LOW_THR_SET          0x17        //设置FZ MIC下限能量阈值
#define ANC_SZ_ADAP_THR_SET			0x18		//设置SZ 自适应收敛阈值
#define ANC_FZ_ADAP_THR_SET			0x19		//设置FZ 自适应收敛阈值
#define ANC_AUTO_TS_EN_SET			0x1a		//设置步进自适应微调使能

#define ANC_NONADAP_TIME_SET		0x1b		//设置非自适应收敛时间
#define ANC_SZ_ADAP_TIME_SET		0x1c		//设置SZ自适应收敛时间
#define ANC_FZ_ADAP_TIME_SET		0x1d		//设置FZ自适应收敛时间
#define ANC_WZ_TRAIN_TIME_SET		0x1e		//设置WZ训练时间

#define ANC_FBGAIN_SET				0x1f		//设置ANC的FBGAIN增益

//以下为开发者测试指令
#define ANC_DEVELOPER_MODE			0x20		//开发者模式
#define ANC_TEST_NADAP				0x21		//关闭ANC自适应收敛
#define ANC_TEST_ADAP				0x22		//开启ANC自适应收敛
#define ANC_TEST_WZ_BREAK			0x23		//退出噪声训练
#define ANC_CHANGE_MODE				0x24		//模式设置指令

#define ANC_TRAIN_STEP_CONNUTE      0x30        //继续找步进

//----------------耳机反馈子命令1---------------------//
#define	ANC_EXEC_SUCC				0x01		//执行成功
#define ANC_EXEC_FAIL				0x02		//执行失败
#define ANC_TRIM_MIC_WORK_FAIL		0x80|BIT(0)	//MIC1工作不正常
#define ANC_MIC_WORK_FAIL   		0x80|BIT(1)	//MIC0工作不正常
#define ANC_TRIM_MIC_SNR_FAIL   	0x80|BIT(2)	//MIC1信噪比差
#define ANC_MIC_SNR_FAIL   			0x80|BIT(3) //MIC0信噪比差
#define ANC_TRIM_MIC_NOISE_FAIL  	0x80|BIT(4) //MIC1底噪高
#define ANC_MIC_NOISE_FAIL   		0x80|BIT(5) //MIC0底噪高
//多种错误信息组合为|的形式,如MIC0/MIC1工作都不正常，则返回0x80|BIT(0)|BIT(1)


#define ANC_TRANS_EN				BIT(0)      //通透训练使能
#define ANC_FF_EN					BIT(1)		//FF训练使能
#define ANC_FB_EN					BIT(2)		//FB训练使能
#define ANC_HYBRID_EN  			 	BIT(3)		//Hybrid 训练使能
//注意FF/FB/Hybrid/只能选择一种

#define A_MIC0					BIT(0)		//模拟MIC0
#define A_MIC1        			BIT(1)		//模拟MIC1
#define D_MIC0       			BIT(2)		//数字MIC0
#define D_MIC1       		 	BIT(3)		//数字MIC1

#define ANC_SPP_PACK_NUM		10		//数据包长度

#define ANC_SPP_MAGIC			0x55AA

enum {
    ANC_SZ_MUTE_STATUS = 0,					//SZ非自适应（DAC静音）状态
    ANC_SZ_NADAP_STATUS,					//SZ非自适应状态
    ANC_SZ_ADAP_STATUS,						//SZ自适应状态
    ANC_FZ_MUTE_STATUS,				    	//FZ非自适应（DAC静音）状态
    ANC_FZ_NADAP_STATUS,					//FZ非自适应状态
    ANC_FZ_ADAP_STATUS,						//FZ自适应状态
    ANC_WZ_NADAP_STATUS,					//WZ非自适应状态
    ANC_WZ_ADAP_STATUS,						//WZ自适应状态
    ANC_WZ_END_STATUS,						//WZ训练结束
};

enum {
    ANC_TRAIN_SZ = 0,
    ANC_TRAIN_FZ
};

typedef struct {
    u16 magic;
    u16 crc;
    u16 len;
    u8 dat[0];
} anc_spp_data_t;

typedef struct {
    u8  mode;			//当前训练步骤
    u8  train_busy;		//训练繁忙位 0-空闲 1-训练中 2-训练结束测试中
    u8	train_step;		//训练系数
    u8  ret_step;		//自适应训练系数值
    u8  noise_level;	//静音训练白噪等级，等级越高，白噪声音越小。与DAC 模拟增益设置成反比
    u8  auto_ts_en;		//自动步进微调使能位
    /*
         DAC GAIN <10    noise_level 1;
     10< DAC GAIN <12    noise_level 2;
         DAC GAIN >12    noise_level 3;
     */
    u8  enablebit; 			//训练模式使能标志位 前馈|混合馈|通透
    u16 timerout_id;		//训练超时定时器ID
    s16 fb0_gain;
    s16 fb0sz_dly_en;
    u16 fb0sz_dly_num;
    int sz_gain;			//静音训练sz 增益
    u32 sz_lower_thr;		//误差MIC下限能量阈值
    u32 fz_lower_thr;   	//参考MIC下限能量阈值
    u32 sz_noise_thr;		//误差MIC底噪阈值
    u32 fz_noise_thr;   	//参考MIC底噪阈值
    u32 sz_adaptive_thr;	//误差MIC自适应收敛阈值
    u32 fz_adaptive_thr;	//参考MIC自适应收敛阈值
    u32 wz_train_thr;		//噪声训练阈值
    u32 non_adaptive_time;  //非自适应收敛时间
    u32 sz_adaptive_time;   //误差MIC自适应收敛时间
    u32 fz_adaptive_time;   //参考MIC自适应收敛时间
    u32 wz_train_time; 		//噪声训练时间
} anc_train_para_t;

typedef struct {
    anc_train_para_t *para;
    u8 developer_mode;		//开发者模式
    u8 noise_exit;			//退出噪声训练
    u8 pow_dat[10];
    u8 status;
    u8 train_step_num;
    int anc_gain;
    int anc_fbgain;
    anc_spp_data_t rx_buf;
    anc_spp_data_t tx_buf;

} anc_spp_t;

typedef struct {
    u8 mode;
    u8 ff_en;
    u8 fb_en;
    u16 fb0_gain;
    u16 fb1_gain;
} anc_train_reg_t;

typedef struct {
    u8 mic_errmsg;
    u8 status;
    u32 dat[4];  //ff_num/ff_dat/fb_num/fb_dat
    u32 pow;
    u32 temp_pow;
    u32 mute_pow;
} anc_ack_msg_t;

void anc_spp_init(anc_train_para_t *para);
void anc_spp_uninit(void);
int anc_spp_event_deal(u8 *dat);
void anc_train_api_set(u8 cmd, u32 data, anc_train_para_t *para);
int anc_spp_rx_packet(u8 *dat, u8 len);
int anc_spp_tx_packet(u8 mode, u8 command, u16 command_dat);
u8 anc_powdat_analysis(u32 pow);
void anc_coeff_max_get(s32 *coeff, s32 *dat, u16 len, u8 type);

void anc_btspp_display_pow(anc_ack_msg_t *anc_ack);
void anc_btspp_status_set(u8 status);

#endif/*_ANC_BTSPP_H_*/

