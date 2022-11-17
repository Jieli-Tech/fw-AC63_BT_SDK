#ifndef _LP_TOUCH_KEY_HW_H_
#define _LP_TOUCH_KEY_HW_H_

enum {
    LP_CTMU_CLK_SEL_32K = 0,
    LP_CTMU_CLK_SEL_250K,
    LP_CTMU_CLK_SEL_P11_SYS_CLK,
};

enum {
    LPCTMU_VL_020V = (0 << 5),
    LPCTMU_VL_025V = (1 << 5),
    LPCTMU_VL_030V = (2 << 5),
    LPCTMU_VL_035V = (3 << 5),
};

enum {
    LPCTMU_VH_065V = (0 << 3),
    LPCTMU_VH_070V = (1 << 3),
    LPCTMU_VH_075V = (2 << 3),
    LPCTMU_VH_080V = (3 << 3),
};

enum {
    LPCTMU_ISEL_008UA = (0 << 0),
    LPCTMU_ISEL_024UA = (1 << 0),
    LPCTMU_ISEL_040UA = (2 << 0),
    LPCTMU_ISEL_056UA = (3 << 0),
    LPCTMU_ISEL_072UA = (4 << 0),
    LPCTMU_ISEL_088UA = (5 << 0),
    LPCTMU_ISEL_104UA = (6 << 0),
    LPCTMU_ISEL_120UA = (7 << 0),
};




//============= 初始化顺序 ===============//
//模块配置顺序: Select ctmu clk --> Enable(BIT0) --> 状态机Reset(bit4 写0) --> 配置参数复位(bit1 写0) --> 配置参数复位释放(bit1写1) --> 开始配置参数 --> Run(bit4 写1)
#define LP_CTMU_MODULE_EN(x) 	SFR(P11_LCTM_CON, 0, 1, x)
#define LP_CTMU_CFG_RESET(x) 	SFR(P11_LCTM_CON, 1, 1, x)
#define LP_CTMU_CLK_SEL(x) 		SFR(P11_LCTM_CON, 2, 2, x)
#define LP_CTMU_RUN(x) 			SFR(P11_LCTM_CON, 4, 1, x)
#define LP_CTMU_CH0_IE(x) 		SFR(P11_LCTM_CON, 5, 1, x)
#define LP_CTMU_CH1_IE(x) 		SFR(P11_LCTM_CON, 6, 1, x)
#define LP_CTMU_WKUP_P11_IE(x)  SFR(P11_LCTM_CON, 7, 1, x)


//--------- 软件触发模块采样 -------//
#define LP_CTMU_SOFT_KICK_MODE(x) 		SFR(LCTM_MOD, 0, 1, x)
#define LP_CTMU_SOFT_KICK_START 		SFR(LCTM_MOD, 7, 1, 1)

//---------  PB1 & PB2 IO 互换 -------//
#define LP_CTMU_IO_INV(x) 				SFR(LCTM_MOD, 1, 1, x)

//--------- 通道切换稳定时间 -------//
#define LP_CTMU_SWITCH_STABLE_TIME_SET() (LCTM_TMR = ((1 << 4) | (1 << 0)))

//---------- 模块时基配置 ------------//
#define LP_CTMU_TIMER_BASE_CONFIG(x) 	do {LCTM_TIME_BASE_H = ((x >> 8) & 0xFFFF); LCTM_TIME_BASE_L = (x & 0xFFFF);} while(0)

//---------- 模块模拟配置 ------------//
#define LP_CTMU_CH0_ANA_CFG(x)     (LCTM_CHL0_ANA = x)

#define LP_CTMU_CH1_ANA_CFG(x)     (LCTM_CHL1_ANA = x)

//---------- 屏蔽第一个上升沿 ------------//
#define LP_CTMU_CH0_FIRST_RISE_MASK(x) 	SFR(LCTM_CHL0_CON0, 6, 1, x) //CH0
#define LP_CTMU_CH1_FIRST_RISE_MASK(x) 	SFR(LCTM_CHL1_CON0, 6, 1, x) //CH1

//---------- 边沿检测温漂致使释放使能 ------------//
#define LP_CTMU_CH0_EDGE_TEMP_EN(x) 	SFR(LCTM_CHL0_CON0, 5, 1, x) //CH0
#define LP_CTMU_CH1_EDGE_TEMP_EN(x) 	SFR(LCTM_CHL1_CON0, 5, 1, x) //CH1

//---------- 边沿检测使能 ------------//
#define LP_CTMU_CH0_EDGE_EN(x) 	SFR(LCTM_CHL0_CON0, 4, 1, x) //CH0
#define LP_CTMU_CH1_EDGE_EN(x) 	SFR(LCTM_CHL1_CON0, 4, 1, x) //CH1

//---------- 滤波级数 ------------//
#define LP_CTMU_CH0_FILTER_LEVEL(x) 	SFR(LCTM_CHL0_CON0, 2, 2, x) //CH0
#define LP_CTMU_CH1_FILTER_LEVEL(x) 	SFR(LCTM_CHL1_CON0, 2, 2, x) //CH1

//---------- 滤波使能 ------------//
#define LP_CTMU_CH0_FILTER_EN(x) 	SFR(LCTM_CHL0_CON0, 1, 1, x) //CH0
#define LP_CTMU_CH1_FILTER_EN(x) 	SFR(LCTM_CHL1_CON0, 1, 1, x) //CH1

//---------- 通道使能 ------------//
#define LP_CTMU_CH0_ENABLE(x) 	SFR(LCTM_CHL0_CON0, 0, 1, x) //CH0
#define LP_CTMU_CH1_ENABLE(x) 	SFR(LCTM_CHL1_CON0, 0, 1, x) //CH1

//---------- 通道下降沿中断标志 ------------//
#define LP_CTMU_CH0_IS_FALL_PENDING() 	 (LCTM_CHL0_CON1 & BIT(7)) //CH0
#define LP_CTMU_CH1_IS_FALL_PENDING() 	 (LCTM_CHL1_CON1 & BIT(7)) //CH1

//---------- 通道下降沿中断标志清除 ------------//
#define LP_CTMU_CH0_FALL_PENDING_CLR() 	 (LCTM_CHL0_CON1 |= BIT(6)) //CH0
#define LP_CTMU_CH1_FALL_PENDING_CLR() 	 (LCTM_CHL1_CON1 |= BIT(6)) //CH1

//---------- 通道下降沿中断标志位选择(下降沿是否于上升沿有关) ------------//
//0: 有关; 1: 无关
#define LP_CTMU_CH0_FALL_PENDING_SEL(x)  SFR(LCTM_CHL0_CON1, 5, 1, x) //CH0
#define LP_CTMU_CH1_FALL_PENDING_SEL(x)  SFR(LCTM_CHL1_CON1, 5, 1, x) //CH1

//---------- 通道下降沿中断使能 ------------//
#define LP_CTMU_CH0_FALL_PENDING_IE(x)  SFR(LCTM_CHL0_CON1, 4, 1, x) //CH0
#define LP_CTMU_CH1_FALL_PENDING_IE(x)  SFR(LCTM_CHL1_CON1, 4, 1, x) //CH1

//---------- 通道上降沿中断标志 ------------//
#define LP_CTMU_CH0_IS_RISE_PENDING() 	 (LCTM_CHL0_CON1 & BIT(3)) //CH0
#define LP_CTMU_CH1_IS_RISE_PENDING() 	 (LCTM_CHL1_CON1 & BIT(3)) //CH1

//---------- 通道上降沿中断标志清除 ------------//
#define LP_CTMU_CH0_RISE_PENDING_CLR() 	 (LCTM_CHL0_CON1 |= BIT(2)) //CH0
#define LP_CTMU_CH1_RISE_PENDING_CLR() 	 (LCTM_CHL1_CON1 |= BIT(2)) //CH1

//---------- 通道上降沿中断标志位选择(上降沿是否于下升沿有关) ------------//
//0: 有关; 1: 无关
#define LP_CTMU_CH0_RISE_PENDING_SEL(x)  SFR(LCTM_CHL0_CON1, 1, 1, x) //CH0
#define LP_CTMU_CH1_RISE_PENDING_SEL(x)  SFR(LCTM_CHL1_CON1, 1, 1, x) //CH1

//---------- 通道上降沿中断使能 ------------//
#define LP_CTMU_CH0_RISE_PENDING_IE(x)  SFR(LCTM_CHL0_CON1, 0, 1, x) //CH0
#define LP_CTMU_CH1_RISE_PENDING_IE(x)  SFR(LCTM_CHL1_CON1, 0, 1, x) //CH1

//---------- 短按模式选择(CH1无) ------------//
//0: 单击模式; 1: 多击模式
#define LP_CTMU_CH0_SHORT_KEY_MODE(x)  	SFR(LCTM_CHL0_CON2, 7, 1, x) //CH0: 短按时间延迟使能(支持多击)
//#define LP_CTMU_CH0_SHORT_KEY_CNT_GET()  ((LCTM_CHL0_CON2 >> 4) & 0x7) //CH0: 多击次数
#define LP_CTMU_CH0_SHORT_KEY_CNT_GET()  (P2M_CTMU_CTMU_KEY_CNT) //CH0: 多击次数

//---------- 快速扫描标志 ------------//
#define LP_CTMU_CH0_FAST_RUN_FLAG_GET()  (LCTM_CHL0_CON2 & BIT(3)) //CH0
#define LP_CTMU_CH1_FAST_RUN_FLAG_GET()  (LCTM_CHL1_CON2 & BIT(3)) //CH1

//---------- 通道采样结果中断标志 ------------//
#define LP_CTMU_CH0_IS_RES_PENDING()  	(LCTM_CHL0_CON2 & BIT(2)) //CH0
#define LP_CTMU_CH1_IS_RES_PENDING()  	(LCTM_CHL1_CON2 & BIT(2)) //CH1

//---------- 通道采样结果中断标志清除 ------------//
#define LP_CTMU_CH0_RES_PENDING_CLR()  	(LCTM_CHL0_CON2 |= BIT(1)) //CH0
#define LP_CTMU_CH1_RES_PENDING_CLR()  	(LCTM_CHL1_CON2 |= BIT(1)) //CH1

//---------- 通道采样结果中断使能 ------------//
#define LP_CTMU_CH0_RES_PENDING_IE(x)  	SFR(LCTM_CHL0_CON2, 0, 1, x) //CH0
#define LP_CTMU_CH1_RES_PENDING_IE(x)  	SFR(LCTM_CHL1_CON2, 0, 1, x) //CH1

//---------- CH0 短按计数识别模式 ------------//
#define LP_CTMU_CH0_SHORT_KEY_TRIGGER(x)  	SFR(LCTM_CHL0_CON3, 7, 1, x) //0: 下降沿计数, 1: 上升沿计数

//---------- CH0 短按中断标志 ------------//
#define LP_CTMU_CH0_IS_SHORT_PENDING()  	(LCTM_CHL0_CON3 & BIT(6))

//---------- CH0 短按中断标志清除 ------------//
#define LP_CTMU_CH0_SHORT_PENDING_CLR()  	(LCTM_CHL0_CON3 |= BIT(5))

//---------- CH0 短按中断使能 ------------//
#define LP_CTMU_CH0_SHORT_PENDING_IE(x)  	SFR(LCTM_CHL0_CON3, 4, 1, x)

//---------- CH0 长按中断标志 ------------//
#define LP_CTMU_CH0_IS_LONG_PENDING()  	(LCTM_CHL0_CON3 & BIT(3))

//---------- CH0 长按中断标志清除 ------------//
#define LP_CTMU_CH0_LONG_PENDING_CLR()  (LCTM_CHL0_CON3 |= BIT(2))

//---------- CH0 长按中断使能 ------------//
#define LP_CTMU_CH0_LONG_PENDING_IE(x)  SFR(LCTM_CHL0_CON3, 1, 1, x)

//---------- CH0 按键识别使能 ------------//
#define LP_CTMU_CH0_KEY_MODE_ENABLE(x)  	SFR(LCTM_CHL0_CON3, 0, 1, x)

//---------- CH0 高速扫描周期 ------------//
#define LP_CTMU_CH0_HIGH_SPEED_PRD(x)  	SFR(LCTM_CHL0_HS_PRD, 0, 8, x)

//---------- CH0 低速扫描周期 ------------//
#define LP_CTMU_CH0_LOW_SPEED_PRD(x)  	SFR(LCTM_CHL0_LS_PRD, 0, 8, x)

//---------- CH1 高速扫描周期 ------------//
#define LP_CTMU_CH1_HIGH_SPEED_PRD(x)  	SFR(LCTM_CHL1_HS_PRD, 0, 8, x)

//---------- CH1 低速扫描周期 ------------//
#define LP_CTMU_CH1_LOW_SPEED_PRD(x)  	SFR(LCTM_CHL1_LS_PRD, 0, 8, x)

//---------- CH0 窗口检测时间 ------------//
#define LP_CTMU_CH0_DET_TIME(x)  	do {LCTM_CHL0_DET_TIME_H = ((x >> 8) & 0xFFFF); LCTM_CHL0_DET_TIME_L = (x & 0xFFFF);} while(0)

//---------- CH1 窗口检测时间 ------------//
#define LP_CTMU_CH1_DET_TIME(x)  	do {LCTM_CHL1_DET_TIME_H = ((x >> 8) & 0xFFFF); LCTM_CHL1_DET_TIME_L = (x & 0xFFFF);} while(0)

//---------- CH0 温漂阈值 ------------//
#define LP_CTMU_CH0_TEMP_TH(x)  	do {LCTM_CHL0_TEMP_H = ((x >> 8) & 0xFFFF); LCTM_CHL0_TEMP_L = (x & 0xFFFF);} while(0)

//---------- CH1 温漂阈值 ------------//
#define LP_CTMU_CH1_TEMP_TH(x)  	do {LCTM_CHL1_TEMP_H = ((x >> 8) & 0xFFFF); LCTM_CHL1_TEMP_L = (x & 0xFFFF);} while(0)

//---------- CH0 稳定阈值 ------------//
#define LP_CTMU_CH0_STABLE_TH(x)  	do {LCTM_CHL0_STA_H = ((x >> 8) & 0xFFFF); LCTM_CHL0_STA_L = (x & 0xFFFF);} while(0)

//---------- CH1 稳定阈值 ------------//
#define LP_CTMU_CH1_STABLE_TH(x)  	do {LCTM_CHL1_STA_H = ((x >> 8) & 0xFFFF); LCTM_CHL1_STA_L = (x & 0xFFFF);} while(0)

//---------- CH0 边沿阈值 ------------//
#define LP_CTMU_CH0_EDGE_TH(x)  	do {LCTM_CHL0_EDGE_H = ((x >> 8) & 0xFFFF); LCTM_CHL0_EDGE_L = (x & 0xFFFF);} while(0)

//---------- CH1 边沿阈值 ------------//
#define LP_CTMU_CH1_EDGE_TH(x)  	do {LCTM_CHL1_EDGE_H = ((x >> 8) & 0xFFFF); LCTM_CHL1_EDGE_L = (x & 0xFFFF);} while(0)

//---------- CH0 短按检测时长(10bit) ------------//
#define LP_CTMU_CH0_SHORT_KEY_TIME(x)  	do {LCTM_CHL0_SHORT_H = ((x >> 8) & 0xFFFF); LCTM_CHL0_SHORT_L = (x & 0xFFFF);} while(0)

//---------- CH0 长按检测时长(10bit) ------------//
#define LP_CTMU_CH0_LONG_KEY_TIME(x)  	do {LCTM_CHL0_LONG_H = ((x >> 8) & 0xFFFF); LCTM_CHL0_LONG_L = (x & 0xFFFF);} while(0)

//---------- CH0 HOLD检测时长(10bit) ------------//
#define LP_CTMU_CH0_HOLD_KEY_TIME(x)  	do {LCTM_CHL0_HOLD_H &= (~0x3); LCTM_CHL0_HOLD_H |= ((x >> 8) & 0x3); LCTM_CHL0_HOLD_L = (x & 0xFFFF);} while(0)
#define LP_CTMU_CH0_HOLD_EN(x) 				SFR(LCTM_CHL0_HOLD_H, 4, 1, x)
//---------- CH0 长按HOLD中断使能 ------------//
#define LP_CTMU_CH0_HOLD_PENDING_IE(x) 		SFR(LCTM_CHL0_HOLD_H, 5, 1, x)
//---------- CH0 长按HOLD中断清除 ------------//
#define LP_CTMU_CH0_HOLD_PENDING_CLR() 		(LCTM_CHL0_HOLD_H |= BIT(6))
//---------- CH0 长按HOLD中断标志 ------------//
#define LP_CTMU_CH0_IS_HOLD_PENDING() 		(LCTM_CHL0_HOLD_H & BIT(7))

//---------- CH0 采样结果 ------------//
//#define LP_CTMU_CH0_RES_GET()  			((LCTM_CHL0_RES_H << 8) | (LCTM_CHL0_RES_L))
#define LP_CTMU_CH0_RES_GET()  			((P2M_CTMU_CH0_H_RES << 8) | (P2M_CTMU_CH0_L_RES))

//---------- CH1 采样结果 ------------//
//#define LP_CTMU_CH1_RES_GET()  			((LCTM_CHL1_RES_H << 8) | (LCTM_CHL1_RES_L))
#define LP_CTMU_CH1_RES_GET()  			((P2M_CTMU_CH1_H_RES << 8) | (P2M_CTMU_CH1_L_RES))


//=======================================================//
//                     P11 通讯定义                  	 //
//=======================================================//
#define CTMU_INIT_CH0_ENABLE 		BIT(0)
#define CTMU_INIT_CH0_DEBUG 		BIT(1)
#define CTMU_INIT_CH0_FALL_ENABLE 	BIT(2)
#define CTMU_INIT_CH0_RAISE_ENABLE 	BIT(3)
#define CTMU_INIT_CH_PORT_INV 		BIT(4)
#define CTMU_INIT_CH0_SHORT_ENABLE 	BIT(5)

#define CTMU_INIT_CH1_ENABLE 		BIT(6)
#define CTMU_INIT_CH1_DEBUG 		BIT(7)

enum CTMU_P2M_EVENT {
    CTMU_P2M_CH0_RES_EVENT = 0x50,
    CTMU_P2M_CH0_SHORT_KEY_EVENT,
    CTMU_P2M_CH0_LONG_KEY_EVENT,
    CTMU_P2M_CH0_HOLD_KEY_EVENT,
    CTMU_P2M_CH0_FALLING_EVENT,
    CTMU_P2M_CH0_RAISING_EVENT,
    CTMU_P2M_CH1_RES_EVENT,
    CTMU_P2M_CH1_IN_EVENT,
    CTMU_P2M_CH1_OUT_EVENT,
};

enum CTMU_M2P_CMD {
    CTMU_M2P_INIT = 0x50,
    CTMU_M2P_DISABLE, 		//模块关闭
    CTMU_M2P_ENABLE,  		//模块使能
    CTMU_M2P_CH0_ENABLE, 	//通道0打开
    CTMU_M2P_CH0_DISABLE, 	//通道0关闭
    CTMU_M2P_CH1_ENABLE, 	//通道1打开
    CTMU_M2P_CH1_DISABLE, 	//通道0关闭
    CTMU_M2P_UPDATE_BASE_TIME, 	//更新时基参数
    CTMU_M2P_CHARGE_ENTER_MODE, //进仓充电模式
    CTMU_M2P_CHARGE_EXIT_MODE,  //退出充电模式
};

enum {
    EPD_IN = 0,
    EPD_OUT = 1,
    EPD_STATE_NO_CHANCE
};

//=======================================================//
//                     TWS 通讯定义                  	 //
//=======================================================//
enum TWS_MSG_TABLE {
    BT_CH0_RES_MSG,
    BT_CH1_RES_MSG,
    BT_EVENT_HW_MSG,
    BT_EVENT_VDDIO,
    BT_EVENT_SW_MSG,
};

//=======================================================//
//                   关于模块内部的计算                  //
//=======================================================//
//#define CTMU_DEFAULT_LRC_FREQ 			32000 			//默认LRC频率(Hz)
#define CTMU_DEFAULT_LRC_FREQ 			(lp_touch_key_get_lrc_hz()) 			//默认LRC频率(Hz)
#define CTMU_DEFAULT_LRC_PRD 			(1000000 / CTMU_DEFAULT_LRC_FREQ)	//默认LRC周期(ms)
#define CTMU_TIME_BASE 					10 	//ms

#define CFG_M2P_CTMU_CH0_SHORT_TIME_VALUE 	0 //((CTMU_SHORT_CLICK_DELAY_TIME / CTMU_TIME_BASE) - 1)
#define CFG_M2P_CTMU_CH0_LONG_TIME_VALUE 	(((CTMU_LONG_KEY_DELAY_TIME - 0) / CTMU_TIME_BASE) - 1)
#define CFG_M2P_CTMU_CH0_HOLD_TIME_VALUE 	(((CTMU_HOLD_CLICK_DELAY_TIME) / CTMU_TIME_BASE) - 1)

//时基计算:
#define CFG_M2P_CTMU_BASE_TIME_PRD 		((CTMU_DEFAULT_LRC_FREQ * CTMU_TIME_BASE) / 1000 - 1)

//采样窗口时间:
#define CFG_CTMU_CH0_WINDOW_TIME 			4000 		//CH0, 单位: us
#define CFG_CTMU_CH0_WINDOW_TIME_VALUE 	 	((CFG_CTMU_CH0_WINDOW_TIME / CTMU_DEFAULT_LRC_PRD) + 2) 	//换算
#define CFG_CTMU_CH1_WINDOW_TIME 			4000 		//CH1, 单位: us
#define CFG_CTMU_CH1_WINDOW_TIME_VALUE 		((CFG_CTMU_CH1_WINDOW_TIME / CTMU_DEFAULT_LRC_PRD) + 2) 	//换算

//采样周期配置
#define CFG_CTMU_CH0_HS_PERIOD_TIME		10 //ms
#define CFG_CTMU_CH0_LS_PERIOD_TIME		20 //ms
#define CFG_CTMU_CH0_HS_PERIOD_VALUE 	((CFG_CTMU_CH0_HS_PERIOD_TIME / CTMU_TIME_BASE - 1) ? (CFG_CTMU_CH0_HS_PERIOD_TIME  / CTMU_TIME_BASE - 1) : 0)
#define CFG_CTMU_CH0_LS_PERIOD_VALUE 	((CFG_CTMU_CH0_LS_PERIOD_TIME / CTMU_TIME_BASE - 1) ? (CFG_CTMU_CH0_LS_PERIOD_TIME   / CTMU_TIME_BASE - 1) : 0)

#define CFG_CTMU_CH1_HS_PERIOD_TIME		30 //ms
#define CFG_CTMU_CH1_LS_PERIOD_TIME		50 //ms
#define CFG_CTMU_CH1_HS_PERIOD_VALUE 	((CFG_CTMU_CH1_HS_PERIOD_TIME / CTMU_TIME_BASE - 1) ? (CFG_CTMU_CH1_HS_PERIOD_TIME / CTMU_TIME_BASE - 1) : 0)
#define CFG_CTMU_CH1_LS_PERIOD_VALUE 	((CFG_CTMU_CH1_LS_PERIOD_TIME / CTMU_TIME_BASE - 1) ? (CFG_CTMU_CH1_LS_PERIOD_TIME / CTMU_TIME_BASE - 1) : 0)

//长按复位时基
#define CTMU_RESET_TIMER_PRD_VALUE 		200 	//(ms)

#endif /* #ifndef _LP_TOUCH_KEY_HW_H_ */

