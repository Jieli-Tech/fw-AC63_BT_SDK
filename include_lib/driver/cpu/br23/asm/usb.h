#ifndef	_USB_H_
#define _USB_H_
#include "typedef.h"
#include "generic/ioctl.h"


#ifndef min
#define min(a,b) ((a)<(b) ? (a) : (b))
#endif
#ifndef USB_DIR_OUT
#define USB_DIR_OUT			0		/* to device */
#endif
#ifndef USB_DIR_IN
#define USB_DIR_IN			0x80		/* to host */
#endif

#define     FUSB_MODE               1
#define     EP0_SETUP_LEN           0x40
#define USB_MAX_HW_EPNUM    5


//USB_CON0
#define PHY_ON          0
#define LOW_SPEED       1
#define USB_NRST        2
#define TM1             3
#define CID             4
#define VBUS            5
#define USB_TEST        6
#define PDCHKDP         9
#define SOFIE           10
#define SIEIE           11
#define CLR_SOF_PND     12
#define SOF_PND         13
#define SIE_PND         14
#define CHKDPO          15
#define DM_SE           16
#define DP_SE           17
#define LOWP_MD_        18
#define EP1_DISABLE     20
#define EP2_DEIABLE     21
#define EP3_DISABLE     22
#define EP4_DISABLE     23

//USB_CON1
#define MC_RNW          14
#define MACK            15

//USB_IO_CON0
#define DPOUT           0
#define DMOUT           1
#define DPIE            2
#define DMIE            3
#define DPPD            4
#define DMPD            5
#define DPPU            6
#define DMPU            7
#define IO_PU_MODE      8
#define DPDIE           9
#define DMDIE           10
#define IO_MODE         11
#define SR              12
#define DPDIEH          13
#define DMDIEH          14

enum {
    USB0,
};

#define USB_MAX_HW_NUM      1


struct usb_ep_addr_t {
    u32 ep0_addr;
    u32 ep_usage ;
    u32 ep_taddr[4];
    u32 ep_dual_taddr[4];
    u32 ep_raddr[4];
    u32 ep_dual_raddr[4];
    u32 ep_tsize[4];
    u32 ep_rsize[4];
} __attribute__((aligned(4)));


typedef u8 usb_dev;

u16 musb_read_sofframe(const usb_dev id);

/**@brief 通过USB_CON1寄存器，对USB寄存器进行读操作
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  addr  读取的地址
  * @return     寄存器的数据
  * @par    示例：
  * @code
  * musb_read_usb(usb_id,addr);
  * @encode
  */
u32 musb_read_usb(const usb_dev usb_id, u32 addr);

/**@brief 通过USB_CON1寄存器，对USB寄存器进行写操作
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  addr  写入的地址
  * @param[in]  data  写入的数据
  * @return     无
  * @par    示例：
  * @code
  * musb_write_usb(usb_id,addr);
  * @encode
  */
void musb_write_usb(const usb_dev usb_id, u32 addr, u32 data);

/**@brief 读取指定USB的USB_CON0寄存器的值
  * @param[in]  usb_id  USB接口的id号
  * @return     USB_CON0寄存器值
  * @par    示例：
  * @code
  * usb_dev_con0(usb_id);
  * @encode
  */
u32 usb_dev_con0(const usb_dev usb_id);
//****************************************************************************************
/**@brief 读取USB_CON0寄存器 PHY_ON的值
  * @param[in]  usb_id  USB接口的id号
  * @return     PHY_ON的值   0:禁用   1:使能
  * @par    示例：
  * @code
  * usb_read_CON0_PHY_ON(usb_id);
  * @encode
  */
u32 usb_read_CON0_PHY_ON(const usb_dev usb_id);

/**@brief 读取USB_CON0寄存器 LOW_SPEED的值，低速USB_DMA
  * @param[in]  usb_id  USB接口的id号
  * @return     LOW_SPEED的值   0:禁用   1:使能
  * @par    示例：
  * @code
  * usb_read_CON0_LOW_SPEED(usb_id);
  * @encode
  */
u32 usb_read_CON0_LOW_SPEED(const usb_dev usb_id);

/**@brief 读取USB_CON0寄存器 USB_NRST的值，USB模块复位
  * @param[in]  usb_id  USB接口的id号
  * @return     USB_NRST的值   0:复位   1:释放复位
  * @par    示例：
  * @code
  * usb_read_CON0_USB_NRST(usb_id);
  * @encode
  */
u32 usb_read_CON0_USB_NRST(const usb_dev usb_id);

/**@brief 读取USB_CON0寄存器 TM1的值，用于缩短检测连接时间
  * @param[in]  usb_id  USB接口的id号
  * @return     TM1的值   0:禁用   1:使能
  * @par    示例：
  * @code
  * usb_read_CON0_TM1(usb_id);
  * @encode
  */
u32 usb_read_CON0_TM1(const usb_dev usb_id);

/**@brief 读取USB_CON0寄存器 CID的值，USB的工作模式
  * @param[in]  usb_id  USB接口的id号
  * @return     CID的值   0:主机模式   1:从机模式
  * @par    示例：
  * @code
  * usb_read_CON0_CID(usb_id);
  * @encode
  */
u32 usb_read_CON0_CID(const usb_dev usb_id);

/**@brief 读取USB_CON0寄存器 VBUS的值，USB电源（相当于USB总开关）
  * @param[in]  usb_id  USB接口的id号
  * @return     VBUS的值   0:关闭电源   1:打开电源
  * @par    示例：
  * @code
  * usb_read_CON0_VBUS(usb_id);
  * @encode
  */
u32 usb_read_CON0_VBUS(const usb_dev usb_id);

/**@brief 读取USB_CON0寄存器 USB_TEST的值，USB测试模式
  * @param[in]  usb_id  USB接口的id号
  * @return     USB_TEST的值   0:禁用   1:使能
  * @par    示例：
  * @code
  * usb_read_CON0_USB_TEST(usb_id);
  * @encode
  */
u32 usb_read_CON0_USB_TEST(const usb_dev usb_id);

/**@brief 读取USB_CON0寄存器 PDCHKDP的值，DP外接下拉检查使能
  * @param[in]  usb_id  USB接口的id号
  * @return     PDCHKDP的值   0:禁用   1:使能
  * @par    示例：
  * @code
  * usb_read_CON0_PDCHKDP(usb_id);
  * @encode
  */
u32 usb_read_CON0_PDCHKDP(const usb_dev usb_id);

/**@brief 读取USB_CON0寄存器 SOFIE的值，SOF中断使能
  * @param[in]  usb_id  USB接口的id号
  * @return     SOFIE的值   0:禁用   1:使能
  * @par    示例：
  * @code
  * usb_read_CON0_SOFIE(usb_id);
  * @encode
  */
u32 usb_read_CON0_SOFIE(const usb_dev usb_id);

/**@brief 读取USB_CON0寄存器 SIEIE的值，SIE中断使能
  * @param[in]  usb_id  USB接口的id号
  * @return     SIEIE的值   0:禁用   1:使能
  * @par    示例：
  * @code
  * usb_read_CON0_SIEIE(usb_id);
  * @encode
  */
u32 usb_read_CON0_SIEIE(const usb_dev usb_id);

/**@brief 读取USB_CON0寄存器 SOF_PND的值，SOF中断请求标志
  * @param[in]  usb_id  USB接口的id号
  * @return     SOF_PND的值   0:无请求   1:有请求
  * @par    示例：
  * @code
  * usb_read_CON0_SOF_PND(usb_id);
  * @encode
  */
u32 usb_read_CON0_SOF_PND(const usb_dev usb_id);

/**@brief 读取USB_CON0寄存器 SIE_PND的值，SIE中断请求标志
  * @param[in]  usb_id  USB接口的id号
  * @return     SIE_PND的值   0:无请求   1:有请求
  * @par    示例：
  * @code
  * usb_read_CON0_SIE_PND(usb_id);
  * @encode
  */
u32 usb_read_CON0_SIE_PND(const usb_dev usb_id);

/**@brief 读取USB_CON0寄存器 CHKDPO的值，DP外接下拉检查。   当PDCHKDP = 0,CHKDPO = 1
  * @param[in]  usb_id  USB接口的id号
  * @return     CHKDPO的值   0:低电平   1:高电平
  * @par    示例：
  * @code
  * usb_read_CON0_CHKDPO(usb_id);
  * @encode
  */
u32 usb_read_CON0_CHKDPO(const usb_dev usb_id);

/**@brief 读取USB_CON0寄存器 DM_SE的值，DM输入的电平
  * @param[in]  usb_id  USB接口的id号
  * @return     DM_SE的值   0:低电平   1:高电平
  * @par    示例：
  * @code
  * usb_read_CON0_DM_SE(usb_id);
  * @encode
  */
u32 usb_read_CON0_DM_SE(const usb_dev usb_id);

/**@brief 读取USB_CON0寄存器 DP_SE的值，DP输入的电平
  * @param[in]  usb_id  USB接口的id号
  * @return     DP_SE的值   0:低电平   1:高电平
  * @par    示例：
  * @code
  * usb_read_CON0_DP_SE(usb_id);
  * @encode
  */
u32 usb_read_CON0_DP_SE(const usb_dev usb_id);

/**@brief 读取USB_CON0寄存器 LOWP_MD_的值，低功耗模式选择
  * @param[in]  usb_id  USB接口的id号
  * @return     LOWP_MD_的值   0:使用低功耗模式，系统时钟可低于48M   1:不适用低功耗模式，使用USB模块时系统时钟大于或等于48M
  * @par    示例：
  * @code
  * usb_read_CON0_LOWP_MD_(usb_id);
  * @encode
  */
u32 usb_read_CON0_LOWP_MD_(const usb_dev usb_id);

/**@brief 读取USB_CON0寄存器 EPx_DISABLE的值，x为端点号，可选（1~4）;关闭端点x，不会返回ack,nak.stall
  * @param[in]  usb_id  USB接口的id号
  * @return     EPx_DISABLE的值   0:打开   1:关闭
  * @par    示例：
  * @code
  * usb_read_CON0_EPx_DISABLE(usb_id);
  * @encode
  */
u32 usb_read_CON0_EPx_DISABLE(const usb_dev usb_id, u32 ep);
//****************************************************************************************

/**@brief USB SIE中断使能
  * @param[in]  usb_id  USB接口的id号
  * @return     无
  * @par    示例：
  * @code
  * usb_sie_enable(usb_id);
  * @encode
  */
void usb_sie_enable(const usb_dev usb_id);

/**@brief USB SIE中断禁用
  * @param[in]  usb_id  USB接口的id号
  * @return     无
  * @par    示例：
  * @code
  * usb_sie_disable(usb_id);
  * @encode
  */
void usb_sie_disable(const usb_dev id);

/**@brief 记录端点发送数据个数，单位Byte
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  ep  端点
  * @param[in]  len  数据长度
  * @return     发送地址
  * @par    示例：
  * @code
  * usb_write_ep_cnt(usb_id,ep,len);
  * @encode
  */
void usb_write_ep_cnt(const usb_dev usb_id, u32 ep, u32 len);

/**@brief USB从机模式设备状态
  * @param[in]  usb_id  USB接口的id号
  * @return     0:有设备连接   1:无设备连接
  * @par    示例：
  * @code
  * usb_g_dev_status(usb_id);
  * @encode
  */
u32 usb_g_dev_status(const usb_dev usb_id);

/**@brief USB主机模式设备状态
  * @param[in]  usb_id  USB接口的id号
  * @return     0:有设备连接   1:无设备连接
  * @par    示例：
  * @code
  * usb_h_dev_status(usb_id);
  * @encode
  */
u32 usb_h_dev_status(const usb_dev usb_id);

/**@brief USB低速模式设置
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  0:禁用   1:使能
  * @return     无
  * @par    示例：
  * @code
  * usb_set_low_speed(usb_id,flag);
  * @encode
  */
void usb_set_low_speed(const usb_dev usb_id, u8 flag);

/**@brief 对端点0进行写操作
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  *ptr  数据存放地址
  * @param[in]  len  数据长度
  * @return     无
  * @par    示例：
  * @code
  * usb_write_ep0(usb_id,ptr,len);
  * @encode
  */
void usb_write_ep0(const usb_dev usb_id, const u8 *ptr, u32 len);

/**@brief 对端点0进行读操作
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  *ptr  数据存放地址
  * @param[in]  len  数据长度
  * @return     无
  * @par    示例：
  * @code
  * usb_read_ep0(usb_id,ptr,len);
  * @encode
  */
void usb_read_ep0(const usb_dev usb_id, u8 *ptr, u32 len);

/**@brief 获取dma发送地址
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  ep  端点
  * @return     无
  * @par    示例：
  * @code
  * usb_get_dma_taddr(usb_id,ep);
  * @encode
  */
void *usb_get_dma_taddr(const usb_dev usb_id, u32 ep);
u32 usb_get_dma_size(const usb_dev usb_id, u32 ep);
void usb_set_dma_tsize(const usb_dev usb_id, u32 ep, u32 size);
void usb_set_dma_rsize(const usb_dev usb_id, u32 ep, u32 size);

/**@brief 设置dma发送地址
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  ep  端点
  * @param[in]  *ptr  地址
  * @return     无
  * @par    示例：
  * @code
  * usb_set_dma_taddr(usb_id,ep,ptr);
  * @encode
  */
void usb_set_dma_taddr(const usb_dev usb_id, u32 ep, void *ptr);

/**@brief 获取dma接收地址
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  ep  端点
  * @return     无
  * @par    示例：
  * @code
  * usb_get_dma_raddr(usb_id,ep);
  * @encode
  */
void *usb_get_dma_raddr(const usb_dev usb_id, u32 ep);

/**@brief 设置dma接收地址
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  ep  端点
  * @param[in]  *ptr  地址
  * @return     无
  * @par    示例：
  * @code
  * usb_set_dma_raddr(usb_id,ep,ptr);
  * @encode
  */
void usb_set_dma_raddr(const usb_dev usb_id, u32 ep, void *ptr);
void usb_set_dma_dual_raddr(const usb_dev usb_id, u32 ep, void *ptr);
void musb_write_index(const usb_dev usb_id, u32 endpoint);
void usb_write_power(const usb_dev usb_id, u32 value);
u32 usb_read_power(const usb_dev usb_id);

/**@brief 对DEVCTL寄存器进行读操作
  * @param[in]  usb_id  USB接口的id号
  * @return     DEVCTL寄存器的值
  * @par    示例：
  * @code
  * usb_read_devctl(usb_id);
  * @encode
  */
u32 usb_read_devctl(const usb_dev usb_id);

/**@brief 对DEVCTL寄存器进行写操作
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  value  写入的数据
  * @return     无
  * @par    示例：
  * @code
  * usb_write_devctl(usb_id,value);
  * @encode
  */
void usb_write_devctl(const usb_dev usb_id, u32 value);

/**@brief 对CSR0寄存器进行读操作
  * @param[in]  usb_id  USB接口的id号
  * @return     寄存器CSR0的值
  * @par    示例：
  * @code
  * usb_read_csr0(usb_id);
  * @encode
  */
u32 usb_read_csr0(const usb_dev usb_id);

/**@brief 对CSR0寄存器进行写操作
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  csr0  写入的数据
  * @return     无
  * @par    示例：
  * @code
  * usb_write_csr0(usb_id,csr0);
  * @encode
  */
void usb_write_csr0(const usb_dev usb_id, u32 csr0);

/**@brief 将CSR0寄存器中 ClrRxPktRdy(D6位) 置1
  * @param[in]  usb_id  USB接口的id号
  * @return     无
  * @par    示例：
  * @code
  * usb_ep0_ClrRxPktRdy(usb_id);
  * @encode
  */
void usb_ep0_ClrRxPktRdy(const usb_dev usb_id);

/**@brief 将CSR0寄存器中 TxPktRdy(D1位) 和 DataEnd(D3位) 置1
  * @param[in]  usb_id  USB接口的id号
  * @return     无
  * @par    示例：
  * @code
  * usb_ep0_TxPktEnd(usb_id);
  * @encode
  */
void usb_ep0_TxPktEnd(const usb_dev usb_id);

/**@brief 将CSR0寄存器中 ClrRxPktRdy(D6位) 和 DataEnd(D3位) 置1
  * @param[in]  usb_id  USB接口的id号
  * @return     无
  * @par    示例：
  * @code
  * usb_ep0_RxPktEnd(usb_id);
  * @encode
  */
void usb_ep0_RxPktEnd(const usb_dev usb_id);

/**@brief 将CSR0寄存器中 ClrRxPktRdy(D6位) 和 SendStall(D5位) 置1
  * @param[in]  usb_id  USB接口的id号
  * @return     无
  * @par    示例：
  * @code
  * usb_ep0_Set_Stall(usb_id);
  * @encode
  */
void usb_ep0_Set_Stall(const usb_dev usb_id);
u32 usb_read_count0(const usb_dev usb_id);
void usb_read_intre(const usb_dev usb_id,
                    u32 *const intr_usbe,
                    u32 *const intr_txe,
                    u32 *const intr_rxe);

void usb_read_intr(const usb_dev usb_id,
                   u32 *const intr_usb,
                   u32 *const intr_tx,
                   u32 *const intr_rx);
void usb_write_intr_usbe(const usb_dev usb_id, u32 intr_usbe);

/**@brief USB设置发送中断使能
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  ep  端点号
  * @return     无
  * @par    示例：
  * @code
  * usb_set_intr_txe(usb_id,ep);
  * @encode
  */
void usb_set_intr_txe(const usb_dev usb_id, const u32 ep);

/**@brief USB清除发送中断使能
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  ep  端点号
  * @return     无
  * @par    示例：
  * @code
  * usb_clr_intr_txe(usb_id,ep);
  * @encode
  */
void usb_clr_intr_txe(const usb_dev usb_id, const u32 ep);

/**@brief USB设置接收中断使能
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  ep  端点号
  * @return     无
  * @par    示例：
  * @code
  * usb_set_intr_rxe(usb_id,ep);
  * @encode
  */
void usb_set_intr_rxe(const usb_dev usb_id, const u32 ep);

/**@brief USB清除接收中断使能
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  ep  端点号
  * @return     无
  * @par    示例：
  * @code
  * usb_clr_intr_rxe(usb_id,ep);
  * @encode
  */
void usb_clr_intr_rxe(const usb_dev usb_id, const u32 ep);

/**@brief 对FADDR寄存器进行写操作
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  addr  写入FADDR寄存器的值
  * @return     无
  * @par    示例：
  * @code
  * usb_writ_faddr(usb_id,addr);
  * @encode
  */
void usb_write_faddr(const usb_dev usb_id, u32 addr);

/**@brief 对txcsr寄存器进行写操作
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  ep  端点号
  * @param[in]  txcsr  写入的值
  * @return     无
  * @par    示例：
  * @code
  * usb_write_txcsr(usb_id,ep,txcsr);
  * @encode
  */
void usb_write_txcsr(const usb_dev usb_id, const u32 ep, u32 txcsr);

/**@brief 对txcsr寄存器进行读操作
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  ep  端点号
  * @return     txcsr寄存器的值
  * @par    示例：
  * @code
  * usb_read_txcsr(usb_id,ep);
  * @encode
  */
u32 usb_read_txcsr(const usb_dev usb_id, const u32 ep);

/**@brief 对rxcsr寄存器进行写操作
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  ep  端点号
  * @param[in]  rxcsr  写入的值
  * @return     无
  * @par    示例：
  * @code
  * usb_write_rxcsr(usb_id,ep,rxcsr);
  * @encode
  */
void usb_write_rxcsr(const usb_dev usb_id, const u32 ep, u32 rxcsr);

/**@brief 对rxcsr寄存器进行读操作
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  ep  端点号
  * @return     rxcsr寄存器的值
  * @par    示例：
  * @code
  * usb_read_rxcsr(usb_id,ep);
  * @encode
  */
u32 usb_read_rxcsr(const usb_dev usb_id, const u32 ep);
void usb_write_rxmaxp(const usb_dev usb_id, const u32 ep, u32 value);
void usb_write_txmaxp(const usb_dev usb_id, const u32 ep, u32 value);
void usb_write_rxtype(const usb_dev usb_id, const u32 ep, u32 value);
void usb_write_txtype(const usb_dev usb_id, const u32 ep, u32 value);
u32 usb_read_rxcount(const usb_dev usb_id, u32 ep);
u32 usb_g_ep_config(const usb_dev usb_id, const u32 ep, u32 type, u32 ie, u8 *ptr, u32 dma_size);
u32 usb_g_ep_read64byte_fast(const usb_dev usb_id, const u32 ep, u8 *ptr, u32 len);
u32 usb_g_ep_read(const usb_dev usb_id, const u32 ep, u8 *ptr, u32 len, u32 block);
u32 usb_g_ep_write(const usb_dev usb_id, u32 ep, const u8 *ptr, u32 len);
u32 usb_g_ep_config(const usb_dev usb_id, u32 ep, u32 type, u32 ie, u8 *ptr, u32 dma_size);

/**@brief USB从机模式SIE初始化
  * @param[in]  usb_id  USB接口的id号
  * @return     无
  * @par    示例：
  * @code
  * usb_g_sie_init(usb_id);
  * @encode
  */
void usb_g_sie_init(const usb_dev usb_id);

/**@brief USB从机模式保持
  * @param[in]  usb_id  USB接口的id号
  * @return     无
  * @par    示例：
  * @code
  * usb_g_hold(usb_id);
  * @encode
  */
void usb_g_hold(const usb_dev usb_id);
u32 usb_get_ep_num(const usb_dev usb_id, u32 ep_dir, u32 type);
u32 usb_h_ep_config(const usb_dev usb_id, u32 ep, u32 type, u32 ie, u32 interval, u8 *ptr, u32 dma_size);
void usb_mdelay(unsigned int ms);

/**@brief USB主机模式对端点写操作
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  host_ep  主机端点
  * @param[in]  txmaxp  发送端点最大包长
  * @param[in]  target_ep  目标端点
  * @param[in]  *ptr  数据存放的地址
  * @param[in]  len  数据长度
  * @param[in]  xfer  端点传输类型
  * @return     正数：数据长度   负数：出错
  * @par    示例：
  * @code
  * usb_h_ep_write(usb_id,host_ep,txmaxp,target_ep,ptr,len,xfer);
  * @encode
  */
u32 usb_h_ep_write(const usb_dev usb_id, u8 host_ep, u16 txmaxp, u8 target_ep, const u8 *ptr, u32 len, u32 xfer);

/**@brief USB主机模式对端点写操作（异步模式）
  * @param[in]  id  USB接口的id号
  * @param[in]  host_ep  主机端点
  * @param[in]  txmaxp  发送端点最大包长
  * @param[in]  target_ep  目标端点
  * @param[in]  *ptr  数据存放的地址
  * @param[in]  len  数据长度
  * @param[in]  xfer  端点传输类型
  * @param[in]  kstart 一个标志位。0:写数据   1:发起OUT请求
  * @return     正数：数据长度   负数：出错
  * @par    示例：
  * @code
  * usb_h_ep_write_async(usb_id,host_ep,txmaxp,target_ep,ptr,len,xfer,kstart);
  * @encode
  */
int usb_h_ep_write_async(const usb_dev id, u8 host_ep, u16 txmaxp, u8 target_ep, const u8 *ptr, u32 len, u32 xfer, u32 kstart);

/**@brief USB主机模式对端点读操作
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  host_ep  主机端点
  * @param[in]  target_ep  目标端点
  * @param[in]  *ptr  数据存放的地址
  * @param[in]  len  数据长度
  * @param[in]  xfer  端点传输类型
  * @return     正数：数据长度   负数：出错
  * @par    示例：
  * @code
  * usb_h_ep_read(usb_id,host_ep,target_ep,ptr,len,xfer);
  * @encode
  */
u32 usb_h_ep_read(const usb_dev usb_id, u8 host_ep, u16 rxmaxp, u8 target_ep, u8 *ptr, u32 len, u32 xfer);

/**@brief USB主机模式对端点读操作（异步模式）
  * @param[in]  id  USB接口的id号
  * @param[in]  host_ep  主机端点
  * @param[in]  target_ep  目标端点
  * @param[in]  *ptr  数据存放的地址
  * @param[in]  len  数据长度
  * @param[in]  xfer  端点传输类型
  * @param[in]  kstart 一个标志位。0:读数据   1:发起IN请求
  * @return     正数：数据长度   负数：出错
  * @par    示例：
  * @code
  * usb_h_ep_read_async(usb_id,host_ep,target_ep,ptr,len,xfer,kstart);
  * @encode
  */
int usb_h_ep_read_async(const usb_dev id, u8 host_ep, u8 target_ep, u8 *ptr, u32 len, u32 xfer, u32 kstart);
void usb_h_sie_init(const usb_dev usb_id);
void usb_h_sie_close(const usb_dev usb_id);
void usb_h_sie_reset(const usb_dev usb_id);

/**@brief 禁用USB热拔插功能
  * @param[in]  usb_id  USB接口的id号
  * @return     无
  * @par    示例：
  * @code
  * usb_hotplug_disable(usb_id);
  * @encode
  */
void usb_hotplug_disable(const usb_dev usb_id);

/**@brief 使能USB热拔插功能
  * @param[in]  usb_id  USB接口的id号
  * @return     无
  * @par    示例：
  * @code
  * usb_hotplug_enable(usb_id);
  * @encode
  */
void usb_hotplug_enable(const usb_dev usb_id, u32 mode);

/**@brief 指定某个USB SIE关闭
  * @param[in]  usb_id  USB接口的id号
  * @return     无
  * @par    示例：
  * @code
  * usb_sie_close(usb_id);
  * @encode
  */
void usb_sie_close(const usb_dev usb_id);

/**@brief 所有USB的SIE关闭
  * @param[in]  无
  * @return     无
  * @par    示例：
  * @code
  * usb_sie_close_all();
  * @encode
  */
void usb_sie_close_all(void);
void usb_io_reset(const usb_dev usb_id);
void usb_var_init(const usb_dev usb_id, void *ptr);
void usb_var_release(const usb_dev usb_id);
void usb_enable_ep(const usb_dev usb_id, u32 eps);
u32 usb_get_ep_status(const usb_dev usb_id, u32 epx);

/**@brief 禁用某一个端点
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  eps  端点号
  * @return     无
  * @par    示例：
  * @code
  * usb_disable_ep(usb_id,ep);
  * @encode
  */
void usb_disable_ep(const usb_dev usb_id, u32 eps);

void usb_sofie_enable(const usb_dev id);
void usb_sofie_disable(const usb_dev id);

/**@brief 清除某个USB SOF中断请求标志
  * @param[in]  usb_id  USB接口的id号
  * @return     无
  * @par    示例：
  * @code
  * usb_sof_clr_pnd(usb_id);
  * @encode
  */
void usb_sof_clr_pnd(const usb_dev id);

/**@brief 将端点0设置为忽略状态
  * @param[in]  usb_id  USB接口的id号
  * @param[in]  addr  地址
  * @return     无
  * @par    示例：
  * @code
  * usb_ep0_Set_ignore(usb_id,addr);
  * @encode
  */
void usb_ep0_Set_ignore(const usb_dev id, u32 addr);

/**@brief 将I/O口恢复成的USB模式
  * @param[in]  usb_id  USB接口的id号
  * @return     无
  * @par    示例：
  * @code
  * usb_recover_io_status(usb_id);
  * @encode
  */
void usb_recover_io_status(const usb_dev id);
#endif
