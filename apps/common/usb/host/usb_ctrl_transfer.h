#ifndef __USB_CTRL_TRANSFER_H__
#define __USB_CTRL_TRANSFER_H__

#include "usb/ch9.h"
#include "usb/usb_phy.h"
#include "device/device.h"
#include "usb_config.h"


/*
 * USB Packet IDs (PIDs)
 */
#define USB_PID_EXT			0xf0	/* USB 2.0 LPM ECN */
#define USB_PID_OUT			0xe1
#define USB_PID_ACK			0xd2
#define USB_PID_DATA0			0xc3
#define USB_PID_PING			0xb4	/* USB 2.0 */
#define USB_PID_SOF			0xa5
#define USB_PID_NYET			0x96	/* USB 2.0 */
#define USB_PID_DATA2			0x87	/* USB 2.0 */
#define USB_PID_SPLIT			0x78	/* USB 2.0 */
#define USB_PID_IN			0x69
#define USB_PID_NAK			0x5a
#define USB_PID_DATA1			0x4b
#define USB_PID_PREAMBLE		0x3c	/* Token mode */
#define USB_PID_ERR			0x3c	/* USB 2.0: handshake mode */
#define USB_PID_SETUP			0x2d
#define USB_PID_STALL			0x1e
#define USB_PID_MDATA			0x0f	/* USB 2.0 */



struct ctlXfer {
    struct usb_ctrlrequest setup;
    void *buffer;
    u8  stage;
};


#if USB_INTERRUPT_ENABLE
void usb_ctl_isr_init(const usb_dev usb_id, u32 ep);
#endif
int usb_clear_feature(struct usb_host_device *usb_dev, u32 ep);
int set_address(struct usb_host_device *usb_dev, u8 devnum);
int usb_get_device_descriptor(struct usb_host_device *usb_dev, struct usb_device_descriptor *desc);
int usb_get_string_descriptor(struct usb_host_device *usb_dev, struct usb_device_descriptor *desc);
int set_configuration(struct usb_host_device *usb_dev);
int set_configuration_add_value(struct usb_host_device *host_dev, u16 value);
int get_config_descriptor(struct usb_host_device *usb_dev, void *cfg_desc, u32 len);
int get_config_descriptor_add_value_l(struct usb_host_device *host_dev, void *cfg_desc, u32 len, u8 value_l);
int get_msd_max_lun(struct usb_host_device *usb_dev, void *lun);
int set_msd_reset(struct usb_host_device *usb_dev);
int hid_set_idle(struct usb_host_device *usb_dev, u32 id);
int hid_get_report(struct usb_host_device *usb_dev, u8 *report, u8 report_id, u16 report_len);
int hid_set_output_report(struct usb_host_device *usb_dev, u8 *report, u8 report_id, u8 report_len);
int usb_set_remote_wakeup(struct usb_host_device *usb_dev);
int get_device_status(struct usb_host_device *usb_dev);
int usb_get_device_qualifier(struct usb_host_device *usb_dev, u8 *buffer);

int usb_get_aoa_version(struct usb_host_device *host_dev, u16 *version);
int usb_set_credentials(struct usb_host_device *host_dev, const char *string, int index);
int usb_switch2aoa(struct usb_host_device *host_dev);
int usb_switch2slave(struct usb_host_device *host_dev);
int usb_aoa_register_hid(struct usb_host_device *host_dev, u16 value, u16 index);
int usb_aoa_set_hid_report_desc(struct usb_host_device *host_dev, u16 value, u16 offset, const char *pbuf, u32 len);
int usb_aoa_send_hid_event(struct usb_host_device *host_dev, u16 value, const u8 *pbuf, u32 len);
int get_ms_extended_compat_id(struct usb_host_device *host_dev,  u8 *buffer);

int usb_set_interface(struct usb_host_device *host_dev, u8 interface, u8 alternateSetting);
int usb_audio_sampling_frequency_control(struct usb_host_device *host_dev, u32 ep, u32 sampe_rate);
int usb_audio_volume_control(struct usb_host_device *host_dev, u8 feature_id, u8 channel_num, u16 volume);
int usb_audio_mute_control(struct usb_host_device *host_dev, u8 feature_id, u8 mute);

#endif
