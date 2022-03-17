#ifndef __DEBUG_H__
#define __DEBUG_H__




#define CDBG_IDx(n, id) ((1<<(n+4)) | (id<<(n*8+8)))
#define CDBG_INV         (1<<7)
#define CDBG_PEN         (1<<3)
#define CDBG_XEN         (1<<2)
#define CDBG_WEN         (1<<1)
#define CDBG_REN         (1<<0)

void debug_init();
void exception_analyze();

/********************************** DUBUG SFR *****************************************/

#define DBG_REV             0x00
#define DBG_ALNK0           0x01
#define DBG_ALNK1           0x02
#define DBG_AUDIO           0x03
#define DBG_SPDIF_D         0x04
#define DBG_SPDIF_I         0x05
#define DBG_ISP             0x06
#define DBG_PAP             0x07
#define DBG_PLNK            0x08
#define DBG_SBC             0x09
#define DBG_AAC             0x0a
#define DBG_AES             0x0b
#define DBG_SD0             0x0c
#define DBG_SD1             0x0d
#define DBG_SPI0            0x0e
#define DBG_SPI1            0x0f
#define DBG_SPI2            0x10
#define DBG_UART0W          0x11
#define DBG_UART0R          0x12
#define DBG_UART1W          0x13
#define DBG_UART1R          0x14
#define DBG_UART2W          0x15
#define DBG_UART2R          0x16
#define DBG_CTM             0x17
#define DBG_AXI_M0          0x80
#define DBG_AXI_M1          0x81
#define DBG_AXI_M2          0x82
#define DBG_AXI_M3          0x83
#define DBG_AXI_M4          0x84
#define DBG_AXI_M5          0x85
#define DBG_AXI_M6          0x86
#define DBG_AXI_M7          0x87
#define DBG_AXI_M8          0x88
#define DBG_AXI_M9          0x89
#define DBG_AXI_MA          0x8a
#define DBG_AXI_MB          0x8b
#define DBG_AXI_MC          0x8c
#define DBG_AXI_MD          0x8d
#define DBG_AXI_ME          0x8e
#define DBG_AXI_MF          0x8f
#define DBG_USB             0xa0
#define DBG_FM              0xa1
#define DBG_BT              0xa2
#define DBG_FFT             0xa3
#define DBG_EQ              0xa4
#define DBG_FIR             0xa5
#define DBG_CPU0_WR         0xf0
#define DBG_CPU0_RD         0xf1
#define DBG_CPU0_IF         0xf2
#define DBG_SDTAP           0xff
#define MSG_NULL            0xff



/* ---------------------------------------------------------------------------- */
/**
 * @brief Memory权限保护设置
 *
 * @param idx: 保护框索引, 范围: 0 ~ 3, 目前系统默认使用0和3, 用户可用1和2
 * @param begin: Memory开始地址
 * @param end: Memory结束地址
 * @param inv: 0: 保护框内, 1: 保护框外
 * @param format: "Cxwr0rw1rw2rw3rw", CPU:外设0:外设1:外设2:外设3,
 * @param ...: 外设ID号索引, 如: DBG_EQ, 见debug.h
 */
/* ---------------------------------------------------------------------------- */
void mpu_set(int idx, u32 begin, u32 end, u32 inv, const char *format, ...);


/* ---------------------------------------------------------------------------- */
/**
 * @brief 取消指定框的mpu保护
 *
 * @param idx: 保护框索引号
 */
/* ---------------------------------------------------------------------------- */
void mpu_disable_by_index(u8 idx);


/* ---------------------------------------------------------------------------- */
/**
 * @brief :取消所有保护框mpu保护
 */
/* ---------------------------------------------------------------------------- */
void mpu_diasble(void);

#endif


