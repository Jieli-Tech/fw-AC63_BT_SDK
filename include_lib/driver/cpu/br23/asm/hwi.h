#ifndef HWI_H
#define HWI_H


#define IRQ_EMUEXCPT_IDX   0		//0
#define IRQ_EXCEPTION_IDX  1		//0
#define IRQ_SYSCALL_IDX    2		//0
#define IRQ_TICK_TMR_IDX   3		//0

#define IRQ_TIME0_IDX      4   	//0
#define IRQ_TIME1_IDX      5   	//0
#define IRQ_TIME2_IDX      6   	//0
#define IRQ_TIME3_IDX      7   	//0
#define IRQ_USB_SOF_IDX    8   	//1
#define IRQ_USB_CTRL_IDX   9  	//1
#define IRQ_RTC_WDT_IDX    10  	//0
#define IRQ_ALINK0_IDX     11  	//1
#define IRQ_AUDIO_IDX      12		//1
#define IRQ_PORT_IDX       13		//0
#define IRQ_SPI0_IDX       14		//0
#define IRQ_SPI1_IDX       15		//0
#define IRQ_SD0_IDX        16		//0
#define IRQ_SD1_IDX        17		//0
#define IRQ_UART0_IDX      18		//0
#define IRQ_UART1_IDX      19		//0
#define IRQ_UART2_IDX      20		//0
#define IRQ_PAP_IDX        21		//0
#define IRQ_IIC_IDX        22		//0
#define IRQ_SARADC_IDX     23		//0
#define IRQ_PDM_LINK_IDX   24		//1
#define IRQ_RDEC0_IDX      25		//1
#define IRQ_LRCT_IDX       26       //1
#define IRQ_BREDR_IDX      27		//2
#define IRQ_BT_CLKN_IDX    28		//2
#define IRQ_BT_DBG_IDX     29		//1
#define IRQ_WL_LOFC_IDX    30		//2
#define IRQ_SRC_IDX        31		//1
#define IRQ_FFT_IDX        32 		//1
#define IRQ_EQ_IDX         33		//1
#define IRQ_LP_TIMER0_IDX  34
#define IRQ_LP_TIMER1_IDX  35
#define IRQ_ALINK1_IDX     36
#define IRQ_OSA_IDX        37
#define IRQ_BLE_RX_IDX     38		//2
#define IRQ_BLE_EVENT_IDX  39		//1
#define IRQ_AES_IDX        40
#define IRQ_MCTMRX_IDX 	   41
#define IRQ_CHX_PWM_IDX    42
#define IRQ_FMRX_IDX       43
#define IRQ_SPI2_IDX       44
#define IRQ_SBC_IDX		   45		//1
#define IRQ_GPC_IDX		   46		//1
#define IRQ_FMTX_IDX	   47		//1
#define IRQ_DCP_IDX	       48		//1
#define IRQ_RDEC1_IDX      49		//1
#define IRQ_RDEC2_IDX      50		//1
#define IRQ_SPDIF_IDX      51		//1
#define IRQ_PWM_LED_IDX    52		//1
#define IRQ_CTM_IDX        53		//1
#define IRQ_SOFT0_IDX      60
#define IRQ_SOFT1_IDX      61
#define IRQ_SOFT2_IDX      62
#define IRQ_SOFT3_IDX      63

#define IRQ_MEM_ADDR        0x2BF00
#define MAX_IRQ_ENTRY_NUM   64

void interrupt_init();

void request_irq(u8 index, u8 priority, void (*handler)(void), u8 cpu_id);

void unrequest_irq(u8 index);

void bit_clr_ie(unsigned char index);
void bit_set_ie(unsigned char index);
bool irq_read(u32 index);

#define irq_disable(x)  bit_clr_ie(x)
#define irq_enable(x)   bit_set_ie(x)

#ifdef IRQ_TIME_COUNT_EN
void irq_handler_enter(int irq);

void irq_handler_exit(int irq);

void irq_handler_times_dump();
#else

#define irq_handler_enter(irq)      do { }while(0)
#define irq_handler_exit(irq)       do { }while(0)
#define irq_handler_times_dump()    do { }while(0)

#endif


#endif

