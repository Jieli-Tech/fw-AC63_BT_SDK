#ifndef __DEBUG_H__
#define __DEBUG_H__


void ram_protect_close(void);
void debug_init();
void exception_analyze();

/********************************** DUBUG SFR *****************************************/

//外设写(store)超出设定范围; mode = 1:框内; mode = 0:框外
void prp_store_rang_limit_set(void *low_addr, void *high_addr, u8 mode);

//CPU写(store)超出设定范围; mode = 1:框内; mode = 0:框外
void dsp_store_rang_limit_set(void *low_addr, void *high_addr, u8 mode);

//内部总线地址错误时命中使能, 这个是ex, of, if异常的总开关, 1:使能; 0:关闭
void bus_inv_expt_enable(u8 enable);

//CPU写写总线地址错误时命中使能, 1:使能; 0:关闭
void dsp_ex_inv_enable(u8 enable);

//取操作数地址错误时命中使能, 1:使能; 0:关闭
void dsp_of_inv_enable(u8 enable);

//CPU取指令地址错误时命中使能, 1:使能; 0:关闭
void dsp_if_inv_enable(u8 enable);

//外设读写总线地址错误时命中使能, 1:使能; 0:关闭
void peripheral_bus_inv_enable(u8 enable);


/********************************** EMU *****************************************/
//非对齐访问异常, 不可屏蔽, 默认开启
void emu_misalign_enable(u8 enable);

//非法指令异常, 不可屏蔽, 默认开启
void emu_illeg_enable(u8 enable);

//除0异常使能, 1:使能; 0:关闭
void emu_div0_enable(u8 enable);

//浮点NaN异常使能, 1:使能; 0:关闭
void emu_fpu_inv_enable(u8 enable);

//浮点无穷大异常使能, 1:使能; 0:关闭
void emu_fpu_inf_enable(u8 enable);

//浮点下溢出异常使能, 1:使能; 0:关闭
void emu_fpu_tiny_enable(u8 enable);

//浮点上溢出异常使能, 1:使能; 0:关闭
void emu_fpu_huge_enable(u8 enable);

//浮点不精确异常使能, 1:使能; 0:关闭
void emu_fpu_ine_enable(u8 enable);

//触发相关DEBUG_SFR和EMU异常
void debug_sfr_test();


#endif


