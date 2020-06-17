#ifndef _PLCNT_CNT_H_
#define _PLCNT_CNT_H_

/* =========== pclcnt API ============= */
//时钟初始化
void plcnt_clk_init(u8 clk);

//触摸键 IO 初始化
void plcnt_io_init(u8 port);

//获取plcnt计数值
u32 plcnt_delta_cnt_get(u8 port);


#endif


