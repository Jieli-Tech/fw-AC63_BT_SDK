/**
 * @file usb_bulk_transfer.c
 * @brief bulk transfer driver
 * @author chenrixin@zh-jieli.com
 * @version 1.00
 * @date 2017-02-09
 */

#include <string.h>
#include "jiffies.h"
#include "usb_config.h"
#include "usb_bulk_transfer.h"
#include "usb_ctrl_transfer.h"
#include "usb_storage.h"
#include "usb/host/usb_host.h"
#include "usb/usb_phy.h"
#include "app_config.h"
#include "device_drive.h"

#if USB_HOST_ENABLE
#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

struct usb_request_block {
    void *ptr;
    u32 len;
    u32 target_ep;
    u16 rxmap;
    u16 txmap;
    u32 msg;
};
static struct usb_request_block urb;

static void usb_bulk_rx_isr(struct usb_host_device *host_dev, u32 ep)
{
    usb_dev usb_id = host_device2id(host_dev);
    int l  = min(urb.len, urb.rxmap);
    l = usb_h_ep_read_async(usb_id, ep, urb.target_ep, urb.ptr, l, USB_ENDPOINT_XFER_BULK, 0);
    /* g_printf("%s() %d %d", __func__, l, urb.len); */
    if (l > 0) {
        urb.len -= l;
        urb.ptr += l;
        urb.msg = 0;
    } else {
        urb.msg = l;
        urb.len = 0;
    }

    if (urb.len == 0) {
        usb_sem_post(host_dev);
    } else {
        usb_h_ep_read_async(usb_id, ep,  urb.target_ep, urb.ptr, urb.len, USB_ENDPOINT_XFER_BULK, 1);
    }
}
s32 usb_bulk_only_receive_async(struct device *device, u8 host_ep, u16 rxmaxp, u8 target_ep, u8 *pBuf, u32 len)
{
    struct usb_host_device *host_dev = device_to_usbdev(device);
    const usb_dev usb_id = host_device2id(host_dev);

    urb.ptr = pBuf;
    urb.len = len;
    urb.target_ep = target_ep;

#ifdef CONFIG_USB_SUPPORT_MRX_TX
    urb.rxmap = 1 * 1024;
#else
    urb.rxmap = 0x40;
#endif

    urb.msg = -DEV_ERR_OFFLINE;

    usb_h_set_ep_isr(host_dev, host_ep | USB_DIR_IN, usb_bulk_rx_isr, host_dev);
    usb_set_intr_rxe(usb_id, host_ep);

    int ret = usb_h_ep_read_async(usb_id, host_ep, target_ep, urb.ptr, len, USB_ENDPOINT_XFER_BULK, 1);
    if (ret < 0) {
        return ret;
    }
    ret = usb_sem_pend(host_dev, 250);

    usb_clr_intr_rxe(usb_id, host_ep);
    usb_h_set_ep_isr(host_dev, host_ep | USB_DIR_IN, NULL, host_dev);
    if (ret) {
        return -DEV_ERR_TIMEOUT;
    }
    return urb.msg ? urb.msg : len;
}
static s32 _usb_bulk_only_receive(struct device *device, u8 host_ep, u16 rxmaxp, u8 target_ep, u8 *pBuf, u32 len)
{
#if 0
    struct usb_host_device *host_dev = device_to_usbdev(device);
    const usb_dev usb_id = host_device2id(host_dev);
    return usb_h_bulk_read(usb_id, host_ep, rxmaxp, target_ep, pBuf, len);
#else
    return usb_bulk_only_receive_async(device, host_ep, rxmaxp, target_ep, pBuf, len);
#endif
}
s32 usb_bulk_only_receive(struct device *device, u8 host_ep, u16 rxmaxp, u8 target_ep, u8 *pBuf, u32 len)
{

    struct usb_host_device *host_dev = device_to_usbdev(device);
    int ret = _usb_bulk_only_receive(device, host_ep, rxmaxp, target_ep, pBuf, len);
    if (ret == -DEV_ERR_RXSTALL) {
        ret = usb_clear_feature(host_dev, target_ep | USB_DIR_IN);
    }
    return ret;
}
static void usb_bulk_tx_isr(struct usb_host_device *host_dev, u32 ep)
{
    usb_dev usb_id = host_device2id(host_dev);
    int l  = min(urb.len, urb.txmap);
    l = usb_h_ep_write_async(usb_id, ep, urb.txmap, urb.target_ep, urb.ptr, l, USB_ENDPOINT_XFER_BULK, 0);

    if (l > 0) {
        urb.len -= l;
        urb.ptr += l;
        urb.msg = 0;
    } else {
        urb.msg = l;
        urb.len = 0;
    }

    if (urb.len == 0) {
        if (urb.msg || l == 0) {
            usb_sem_post(host_dev);
        }
    }
}
s32 usb_bulk_only_send_async(struct device *device, u8 host_ep, u16 txmaxp, u8 target_ep, const u8 *pBuf, u32 len)
{
    struct usb_host_device *host_dev = device_to_usbdev(device);
    const usb_dev usb_id = host_device2id(host_dev);

    urb.target_ep = target_ep;
#ifdef CONFIG_USB_SUPPORT_MRX_TX
    urb.txmap = 8 * 1024;
#else
    urb.txmap = 0x40;
#endif


    urb.msg = -DEV_ERR_OFFLINE;
    urb.len = len - min(len, urb.txmap);
    urb.ptr = (u8 *)pBuf + min(len, urb.txmap);


    usb_h_set_ep_isr(host_dev, host_ep, usb_bulk_tx_isr, host_dev);
    usb_set_intr_txe(usb_id, host_ep);

    int ret = usb_h_ep_write_async(usb_id, host_ep, txmaxp, target_ep, pBuf, min(len, urb.txmap), USB_ENDPOINT_XFER_BULK, 1);
    if (ret < 0) {
        return ret;
    }
    ret = usb_sem_pend(host_dev, 250);

    usb_clr_intr_txe(usb_id, host_ep);
    usb_h_set_ep_isr(host_dev, host_ep, NULL, host_dev);

    if (ret) {
        r_printf("ret %d", ret);
        return -DEV_ERR_TIMEOUT;
    }
    /* g_printf("%s() %d %d", __func__, urb.len, urb.msg); */
    return urb.msg ? urb.msg : len;
}
/**
 * @brief usb_bulk_only_send
 *
 * @param device
 * @param host_ep   主机的端点号
 * @param target_ep 目标设备的端点号
 * @param pBuf
 * @param len
 *
 * @return 负数失败
 * 		正数发送的字节数
 */
static s32 _usb_bulk_only_send(struct device *device, u8 host_ep, u16 txmaxp, u8 target_ep, const u8 *pBuf, u32 len)
{
#if 0
    struct usb_host_device *host_dev = device_to_usbdev(device);
    const usb_dev usb_id = host_device2id(host_dev);
    return usb_h_bulk_write(usb_id, host_ep, txmaxp, target_ep, pBuf, len);
#elif 0
    if (len < 512) {
        struct usb_host_device *host_dev = device_to_usbdev(device);
        const usb_dev usb_id = host_device2id(host_dev);
        return usb_h_bulk_write(usb_id, host_ep, txmaxp, target_ep, pBuf, len);
    } else {
        return usb_bulk_only_send_async(device, host_ep, txmaxp, target_ep, pBuf, len);
    }
#else
    return usb_bulk_only_send_async(device, host_ep, txmaxp, target_ep, pBuf, len);
#endif
}
s32 usb_bulk_only_send(struct device *device, u8 host_ep, u16 txmaxp, u8 target_ep, const u8 *pBuf, u32 len)
{
    struct usb_host_device *host_dev = device_to_usbdev(device);
    int ret = _usb_bulk_only_send(device, host_ep, txmaxp, target_ep, pBuf, len);

    if (ret == -DEV_ERR_TXSTALL) {
        ret = usb_clear_feature(host_dev, target_ep);
    }
    return ret;

}
#endif
