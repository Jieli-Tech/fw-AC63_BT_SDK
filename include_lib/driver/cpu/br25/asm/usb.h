#ifndef	_USB_H_
#define _USB_H_
#include "typedef.h"
#include "generic/ioctl.h"

/* #define USB_FPGA*/

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

//.............  Full Speed USB ...................
#define MUSB_FADDR                   0x00
#define MUSB_POWER                   0x01
#define MUSB_INTRTX1                 0x02
#define MUSB_INTRTX2                 0x03
#define MUSB_INTRRX1                 0x04
#define MUSB_INTRRX2                 0x05
#define MUSB_INTRUSB                 0x06
#define MUSB_INTRTX1E                0x07
#define MUSB_INTRTX2E                0x08
#define MUSB_INTRRX1E                0x09
#define MUSB_INTRRX2E                0x0a
#define MUSB_INTRUSBE                0x0b
#define MUSB_FRAME1                  0x0c
#define MUSB_FRAME2                  0x0d
#define MUSB_INDEX                   0x0e
#define MUSB_DEVCTL                  0x0f
#define MUSB_TXMAXP                  0x10
#define MUSB_CSR0                    0x11
#define MUSB_TXCSR1                  0x11
#define MUSB_TXCSR2                  0x12
#define MUSB_RXMAXP                  0x13
#define MUSB_RXCSR1                  0x14
#define MUSB_RXCSR2                  0x15
#define MUSB_COUNT0                  0x16
#define MUSB_RXCOUNT1                0x16
#define MUSB_RXCOUNT2                0x17
#define MUSB_TXTYPE                  0x18
#define MUSB_TXINTERVAL              0x19
#define MUSB_RXTYPE                  0x1a
#define MUSB_RXINTERVAL              0x1b

/*****MUSB SFR BitMap******/
/*INTRUSB mode*/
#define INTRUSB_SUSPEND             BIT(0)
#define INTRUSB_RESUME              BIT(1)
#define INTRUSB_RESET_BABBLE        BIT(2)
#define INTRUSB_SOF                 BIT(3)
#define INTRUSB_CONNECT             BIT(4)
#define INTRUSB_DISCONNECT          BIT(5)
#define INTRUSB_SESS_REQ            BIT(6)
#define INTRUSB_VBUS_ERROR          BIT(7)

/*CSR0 peripheral mode*/
#define CSR0P_RxPktRdy           0x01
#define CSR0P_TxPktRdy           0x02
#define CSR0P_SentStall          0x04
#define CSR0P_DataEnd            0x08
#define CSR0P_SetupEnd           0x10
#define CSR0P_SendStall          0x20
#define CSR0P_ClrRxPktRdy        0x40
#define CSR0P_ClrSetupEnd        0x80


/*TXCSR1 peripheral mode*/
#define TXCSRP_TxPktRdy          0x01
#define TXCSRP_FIFONotEmpty      0x02
#define TXCSRP_UnderRun          0x04
#define TXCSRP_FlushFIFO         0x08
#define TXCSRP_SendStall         0x10
#define TXCSRP_SentStall         0x20
#define TXCSRP_ClrDataTog        0x40
#define TXCSRP_DIR               (BIT(13))
#define TXCSRP_ISOCHRONOUS       (BIT(14))

/*RXCSR1 peripheral mode*/
#define RXCSRP_RxPktRdy          0x01
#define RXCSRP_FIFOFull          0x02
#define RXCSRP_OverRun           0x04
#define RXCSRP_DataError         0x08
#define RXCSRP_FlushFIFO         0x10
#define RXCSRP_SendStall         0x20
#define RXCSRP_SentStall         0x40
#define RXCSRP_ClrDataTog        0x80
#define RXCSRP_IncompRx          (BIT(8))
#define RXCSRP_ISOCHRONOUS       (BIT(14))

/*CSR0 host mode*/
#define CSR0H_RxPktRdy           0x01
#define CSR0H_TxPktRdy           0x02
#define CSR0H_RxStall            0x04
#define CSR0H_SetupPkt           0x08
#define CSR0H_Error              0x10
#define CSR0H_ReqPkt             0x20
#define CSR0H_StatusPkt          0x40
#define CSR0H_DISPING            (BIT(11))

/*TXCSR1 host mode*/
#define TXCSRH_TxPktRdy          0x01
#define TXCSRH_FIFONotEmpty      0x02
#define TXCSRH_Error             0x04
#define TXCSRH_FlushFIFO         0x08
#define TXCSRH_RxStall           0x20
#define TXCSRH_ClrDataTog        0x40
#define TXCSRH_NAK               0x80

/*RXCSR1 host mode*/
#define RXCSRH_RxPktRdy          0x01
#define RXCSRH_FIFOFull          0x02
#define RXCSRH_Error             0x04
#define RXCSRH_DataError         0x08
#define RXCSRH_FlushFIFO         0x10
#define RXCSRH_ReqPkt            0x20
#define RXCSRH_RxStall           0x40
#define RXCSRH_ClrDataTog        0x80
#define RXCSRH_IncompRx          BIT(8)
#define RXCSRH_PIDError          BIT(12)


///USB Slave 控制传输各阶段
#define USB_EP0_STAGE_SETUP       0
#define USB_EP0_STAGE_IN          1
#define USB_EP0_STAGE_OUT         2
#define USB_EP0_SET_STALL         3
#define USB_EP0_IGNORE            4

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
    USB_MAX_HW_NUM,
};


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

u32 musb_read_usb(const usb_dev usb_id, u32 addr);
void musb_write_usb(const usb_dev usb_id, u32 addr, u32 data);
u32 usb_dev_con0(const usb_dev usb_id);
void usb_sie_enable(const usb_dev usb_id);
void usb_sie_disable(const usb_dev id);
void usb_write_ep_cnt(const usb_dev usb_id, u32 ep, u32 len);
u32 usb_g_dev_status(const usb_dev usb_id);
u32 usb_h_dev_status(const usb_dev usb_id);
void usb_set_low_speed(const usb_dev usb_id, u8 flag);
void usb_write_ep0(const usb_dev usb_id, const u8 *ptr, u32 len);
void usb_read_ep0(const usb_dev usb_id, u8 *ptr, u32 len);
void *usb_get_dma_taddr(const usb_dev usb_id, u32 ep);
u32 usb_get_dma_size(const usb_dev usb_id, u32 ep);
void usb_set_dma_tsize(const usb_dev usb_id, u32 ep, u32 size);
void usb_set_dma_rsize(const usb_dev usb_id, u32 ep, u32 size);
void usb_set_dma_taddr(const usb_dev usb_id, u32 ep, void *ptr);
void *usb_get_dma_raddr(const usb_dev usb_id, u32 ep);
void usb_set_dma_raddr(const usb_dev usb_id, u32 ep, void *ptr);
void usb_set_dma_dual_raddr(const usb_dev usb_id, u32 ep, void *ptr);
void musb_write_index(const usb_dev usb_id, u32 endpoint);
void usb_write_power(const usb_dev usb_id, u32 value);
u32 usb_read_power(const usb_dev usb_id);
u32 usb_read_devctl(const usb_dev usb_id);
void usb_write_devctl(const usb_dev usb_id, u32 value);
u32 usb_read_csr0(const usb_dev usb_id);
void usb_write_csr0(const usb_dev usb_id, u32 csr0);
void usb_ep0_ClrRxPktRdy(const usb_dev usb_id);
void usb_ep0_TxPktEnd(const usb_dev usb_id);
void usb_ep0_RxPktEnd(const usb_dev usb_id);
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
void usb_set_intr_txe(const usb_dev usb_id, const u32 ep);
void usb_clr_intr_txe(const usb_dev usb_id, const u32 ep);
void usb_set_intr_rxe(const usb_dev usb_id, const u32 ep);
void usb_clr_intr_rxe(const usb_dev usb_id, const u32 ep);
void usb_write_faddr(const usb_dev usb_id, u32 addr);
void usb_write_txcsr(const usb_dev usb_id, const u32 ep, u32 txcsr);
u32 usb_read_txcsr(const usb_dev usb_id, const u32 ep);
void usb_write_rxcsr(const usb_dev usb_id, const u32 ep, u32 rxcsr);
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
void usb_g_sie_init(const usb_dev usb_id);
void usb_g_hold(const usb_dev usb_id);
u32 usb_get_ep_num(const usb_dev usb_id, u32 ep_dir, u32 type);
u32 usb_h_ep_config(const usb_dev usb_id, u32 ep, u32 type, u32 ie, u32 interval, u8 *ptr, u32 dma_szie);
void usb_mdelay(unsigned int ms);
u32 usb_h_ep_write(const usb_dev usb_id, u8 host_ep, u16 txmaxp, u8 target_ep, const u8 *ptr, u32 len, u32 xfer);
int usb_h_ep_write_async(const usb_dev id, u8 host_ep, u16 txmaxp, u8 target_ep, const u8 *ptr, u32 len, u32 xfer, u32 kstart);
u32 usb_h_ep_read(const usb_dev usb_id, u8 host_ep, u16 rxmaxp, u8 target_ep, u8 *ptr, u32 len, u32 xfer);
int usb_h_ep_read_async(const usb_dev id, u8 host_ep, u8 target_ep, u8 *ptr, u32 len, u32 xfer, u32 kstart);
void usb_h_sie_init(const usb_dev usb_id);
void usb_h_sie_close(const usb_dev usb_id);
void usb_h_sie_reset(const usb_dev usb_id);
void usb_hotplug_disable(const usb_dev usb_id);
void usb_hotplug_enable(const usb_dev usb_id, u32 mode);
void usb_sie_close(const usb_dev usb_id);
void usb_sie_close_all(void);
void usb_io_reset(const usb_dev usb_id);
void usb_var_init(const usb_dev usb_id, void *ptr);
void usb_var_release(const usb_dev usb_id);
void usb_enable_ep(const usb_dev usb_id, u32 eps);
void usb_disable_ep(const usb_dev usb_id, u32 eps);

void usb_sofie_enable(const usb_dev id);
void usb_sofie_disable(const usb_dev id);
void usb_sof_clr_pnd(const usb_dev id);
void usb_ep0_Set_ignore(const usb_dev id, u32 addr);
void usb_recover_io_status(const usb_dev id);
#endif
