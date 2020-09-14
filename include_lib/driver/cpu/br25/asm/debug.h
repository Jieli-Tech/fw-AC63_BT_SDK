#ifndef __DEBUG_H__
#define __DEBUG_H__

#define DEBUG_WR_SFR_EN 	do{JL_DEBUG->WR_EN = 0xE7;}while(0)

#define _DSP_BF_CON				JL_DEBUG->DSP_BF_CON
#define _WR_EN					JL_DEBUG->WR_EN
#define _DEBUG_MSG				JL_DEBUG->MSG
#define _DEBUG_MSG_CLR			JL_DEBUG->MSG_CLR
#define _DSP_PC_LIML0			JL_DEBUG->CPU_PC_LIML0
#define _DSP_PC_LIMH0			JL_DEBUG->CPU_PC_LIMH0
#define _DSP_PC_LIML1			JL_DEBUG->CPU_PC_LIML1
#define _DSP_PC_LIMH1			JL_DEBUG->CPU_PC_LIMH1
#define _DSP_EX_LIML			JL_DEBUG->CPU_WR_LIML
#define _DSP_EX_LIMH			JL_DEBUG->CPU_WR_LIMH
#define _PRP_EX_LIML			JL_DEBUG->PRP_WR_LIML
#define _PRP_EX_LIMH			JL_DEBUG->PRP_WR_LIMH
#define _PRP_MMU_MSG			JL_DEBUG->PRP_MMU_MSG
#define _LSB_MMU_MSG_CH			JL_DEBUG->LSB_MMU_MSG_CH
#define _PRP_WR_LIMIT_MSG		JL_DEBUG->PRP_WR_LIMIT_MSG
#define _LSB_WR_LIMIT_CH		JL_DEBUG->LSB_WR_LIMIT_CH
#define _PRP_SRM_INV_MSG		JL_DEBUG->PRP_SRM_INV_MSG
#define _LSB_SRM_INV_CH        JL_DEBUG->LSB_SRM_INV_CH
#define _DSPCON					JL_DSP->CON
#define _EMU_CON				q32DSP(0)->EMU_CON
#define _EMU_MSG				q32DSP(0)->EMU_MSG
#define _EMU_SSP_H              q32DSP(0)->EMU_SSP_H
#define _EMU_SSP_L              q32DSP(0)->EMU_SSP_L
#define _EMU_USP_H              q32DSP(0)->EMU_USP_H
#define _EMU_USP_L              q32DSP(0)->EMU_USP_L
#define _ETM_CON                q32DSP(0)->ETM_CON


void ram_protect_close(void);
void debug_init();
void exception_analyze();

void emu_stack_limit_set(u8 mode, u32 limit_l, u32 limit_h);
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


