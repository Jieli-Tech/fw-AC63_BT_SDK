#ifndef COEFF_CALCULATE_API_H
#define COEFF_CALCULATE_API_H


int get_coeff_calculate_buf(int section);
void coeff_calculate_init(void *ptr,  int section, int fs);
void bpf_filts(void *ptr, int *in_buf_fc, int *qf);

typedef struct spectrum {
    int section;
    int fs;
    int *SOSMatrix;
    int pool[0];
} spec;

#if 0
coeff_calculate.a库说明：
该库根据给定的参数计算带通滤波器的系数；

调用流程：
bufsize = int get_coeff_calculate_buf(section); //获取buf大小
section：段数（可根据并联的需要进行配置）

workbuf = malloc(bufsize); //申请内存

coeff_calculate_init(workbuf, section, fs); //初始化
fs:
采样频率

bpf_filts(workbuf, in_buf, qf); //运行
in_buf ：输入的fc数据（定点24bit），与段数匹配
qf：输入的q因子数据（定点24bit），与段数匹配

输出：每段5个系数（定点22bit）对应br23模型，系数存储在缓存SOSMatrix中；
#endif

#endif

