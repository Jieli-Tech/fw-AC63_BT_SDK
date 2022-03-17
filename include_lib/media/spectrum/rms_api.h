#ifndef RMS_API_H
#define RMS_API_H

int get_rms_buf(int section, int time, int fs, int switch_1);
void rms_init(void *ptr, int section, int fs, int time, int switch_1, float attackFactor, float releaseFactor, int D_time, float alpha);
int rms(void *ptr, short *in_buf, int len);


typedef struct __spectrum_rms {
    int switch_1;
    float attackFactor;//平滑因子
    float releaseFactor;
    float alpha;//最值的平滑因子
    int section;
    int fs;
    short *index;//索引
    short *cache;//缓存地址
    float *DB;//记录增益
    int time;//缓存时间，ms为单位
    int T_num;//缓存时间内的点数
    int  D_time;//跟新db的时间
    int counter;//计数器
    int *max;//记录最大值
    int T;//缓存长度
    long long *add;//求和累加
    int _T;
    int pool[0];
} spec_rms;

#if 0
rms.a库说明：
该库计算的是经过滤波后的信号的db值，其中包括打开rms计算db和关闭rms计算db两种模式；

调用说明：
buf_size = get_rms_buf(section, time, fs, switch_1); //获取buf大小：
section:
段数（可配置），
time： 缓存时间(单位毫秒，整形输入)，
fs:
采样频率，
switch_1：模式开关（1打开rms，0关闭rms），


work_buf = malloc(buf_size); //申请内存


rms_init(work_buf, channel, section, fs, time, switch_1, attackFactor, releaseFactor);
初始化：
channel：声道数，
attackFactor, releaseFactor：下降因子和上升因子（范围0 - 1，建议attackFactor取较大值）
attackFactor 1 releaseFactor 1不能设置成1, 设置成1永远都是0
attackFactor取大，db值降的慢，releaseFactor取大，db值升的慢

rms(work_buf, in_buf, out_buf, len);//运行
in_buf 输入数据，16bit
out_buf 输出数据，16bit
len 数据长度(其中一段滤波器的点数)
数据输出为单通道并行输出
#endif


#endif
