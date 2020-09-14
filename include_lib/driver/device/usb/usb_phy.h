#ifndef  __USB_PHY_H__
#define  __USB_PHY_H__
#include "typedef.h"
#include "asm/usb.h"

#ifndef min
#define min(a,b) ((a)<(b) ? (a) : (b))
#endif

#define ___ntohl(X)     ((((u16)(X) & 0xff00) >> 8) |(((u16)(X) & 0x00ff) << 8))

#define ___ntohs(X)     ((((u32)(X) & 0xff000000) >> 24) | \
                        (((u32)(X) & 0x00ff0000) >> 8) | \
                        (((u32)(X) & 0x0000ff00) << 8) | \
                        (((u32)(X) & 0x000000ff) << 24))

#if defined(cpu_to_be16) || defined(cpu_to_be32) || defined(be16_to_cpu) || defined(be32_to_cpu)
#error #define cpu_to_be16
#endif

#define cpu_to_be16(v16) ___ntohl(v16)
#define cpu_to_be32(v32) ___ntohs(v32)

#define be16_to_cpu(v16) cpu_to_be16(v16)
#define be32_to_cpu(v32) cpu_to_be32(v32)
#define __le16_to_cpu(v16)     (v16)
#define __le32_to_cpu(v32)     (v32)

#if defined(cpu_to_le16) || defined(cpu_to_le32) || defined(le16_to_cpu) || defined(le32_to_cpu)
#error #define cpu_to_be16
#endif

#define cpu_to_le16(v16) (v16)
#define cpu_to_le32(v32) (v32)

#define le16_to_cpu(v16) cpu_to_le16(v16)
#define le32_to_cpu(v32) cpu_to_le32(v32)

#define LOWORD(l)           ((u16)(l))
#define HIWORD(l)           ((u16)(((u32)(l) >> 16) & 0xFFFF))

#define LOBYTE(w)           ((u8)(w))
#define HIBYTE(w)           ((u8)(((u16)(w) >> 8) & 0xFF))

#define DW1BYTE(dw)         (LOBYTE(LOWORD(dw)))
#define DW2BYTE(dw)         (HIBYTE(LOWORD(dw)))
#define DW3BYTE(dw)         (LOBYTE(HIWORD(dw)))
#define DW4BYTE(dw)         (HIBYTE(HIWORD(dw)))


/*            common api            */
u32 get_jiffies();
u32 usb_host_timeout(u32 ot);
u16 usb_read_sofframe(const usb_dev id);

/*            slave api            */
u32 usb_g_bulk_read64byte_fast(const usb_dev usb_id, u32 ep, u8 *ptr, u32 len);
u32 usb_g_bulk_read(const usb_dev usb_id, u32 ep, u8 *ptr, u32 len, u32 block);
u32 usb_g_bulk_write(const usb_dev usb_id, u32 ep, const u8 *ptr, u32 len);
u32 usb_g_intr_read(const usb_dev usb_id, u32 ep, u8 *ptr, u32 len, u32  block);
u32 usb_g_intr_write(const usb_dev usb_id, u32 ep, const u8 *ptr, u32 len);
u32 usb_g_iso_read(const usb_dev usb_id, u32 ep, u8 *ptr, u32 len, u32 block);
u32 usb_g_iso_write(const usb_dev usb_id, u32 ep, const u8 *ptr, u32 len);
void usb_slave_init(const usb_dev usb_id);

/*            host api            */

u32 usb_h_bulk_read(const usb_dev usb_id, u8 host_ep, u16 rxmaxp, u8 target_ep, u8 *ptr, u32 len);
u32 usb_h_bulk_write(const usb_dev usb_id, u8 host_ep, u16 txmaxp, u8 target_ep, u8 *ptr, u32 len);
u32 usb_h_intr_read(const usb_dev usb_id, u8 host_ep, u16 rxmaxp, u8 target_ep, u8 *ptr, u32 len);
u32 usb_h_intr_write(const usb_dev usb_id, u8 host_ep, u16 txmaxp, u8 target_ep, u8 *ptr, u32 len);
u32 usb_h_iso_read(const usb_dev usb_id, u8 host_ep, u16 rxmaxp, u8 target_ep, u8 *ptr, u32 len);
u32 usb_h_iso_write(const usb_dev usb_id, u8 host_ep, u16 txmaxp, u8 target_ep, u8 *ptr, u32 len);
void usb_h_entry_suspend(const usb_dev usb_id);
void usb_h_resume(const usb_dev usb_id);
u32 usb_host_init(const usb_dev usb_id, u32 reset_delay, u32 timeout);
u32 usb_host_reset(const usb_dev usb_id, u32 reset_delay, u32 timeout);
u32 usb_h_force_reset(const usb_dev usb_id);
// u32 usb_h_sie_init(u32 reset_delay, u32 timeout);
// void usb_h_sie_close();

#endif  /*USB_PHY_H*/
