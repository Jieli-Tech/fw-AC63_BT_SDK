#include "os/os_api.h"
#include "usb/device/usb_stack.h"
#include "usb/device/hid.h"
#include "usb_config.h"

#include "app_config.h"

#define LOG_TAG_CONST       USB
#define LOG_TAG             "[USB]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"
#if TCFG_USB_CUSTOM_HID_ENABLE
typedef void (*hid_rx_handle_t)(void *hdl, u8 *buffer, u32 len);
struct custom_hid_hdl {
    u8 cfg_done;
    void *priv_hdl;
    hid_rx_handle_t hid_rx_hook;
};
static struct custom_hid_hdl *custom_hid_info;
#if USB_MALLOC_ENABLE
#else
static struct custom_hid_hdl _custom_hid_info;
#endif

static const u8 sHIDDescriptor[] = {
//HID
    //InterfaceDeszcriptor:
    USB_DT_INTERFACE_SIZE,     // Length
    USB_DT_INTERFACE,          // DescriptorType
    0x00,                       // bInterface number
    0x00,                      // AlternateSetting
    0x02,                      // NumEndpoint
    USB_CLASS_HID,             // Class = Human Interface Device
    0x00,                      // Subclass, 0 No subclass, 1 Boot Interface subclass
    0x00,                      // Procotol, 0 None, 1 Keyboard, 2 Mouse
    0x00,                      // Interface Name

    //HIDDescriptor:
    0x09,                      // bLength
    USB_HID_DT_HID,            // bDescriptorType, HID Descriptor
    0x01, 0x02,                // bcdHID, HID Class Specification release NO.
    0x00,                      // bCuntryCode, Country localization (=none)
    0x01,                       // bNumDescriptors, Number of descriptors to follow
    0x22,                       // bDescriptorType, Report Desc. 0x22, Physical Desc. 0x23
    0, 0,                       // wDescriptorLength

    //EndpointDescriptor:
    USB_DT_ENDPOINT_SIZE,       // bLength
    USB_DT_ENDPOINT,            // bDescriptorType, Type
    USB_DIR_IN | CUSTOM_HID_EP_IN,     // bEndpointAddress
    USB_ENDPOINT_XFER_INT,      // Interrupt
    LOBYTE(MAXP_SIZE_CUSTOM_HIDIN), HIBYTE(MAXP_SIZE_CUSTOM_HIDIN),// Maximum packet size
    0x01,                       // bInterval, for high speed 2^(n-1) * 125us, for full/low speed n * 1ms

    //Endpoint Descriptor:
    USB_DT_ENDPOINT_SIZE,       // bLength
    USB_DT_ENDPOINT,            // bDescriptorType, Type
    CUSTOM_HID_EP_OUT,   // bEndpointAddress
    USB_ENDPOINT_XFER_INT,      // Interrupt
    LOBYTE(MAXP_SIZE_CUSTOM_HIDOUT), HIBYTE(MAXP_SIZE_CUSTOM_HIDOUT),// Maximum packet size
    0x01,                       // bInterval, for high speed 2^(n-1) * 125us, for full/low speed n * 1ms
};


static const u8 sHIDReportDesc[] = {
    USAGE_PAGE(2, 0x02, 0xff),
    USAGE(1, 0x02),
    COLLECTION(1, APPLICATION),
    USAGE(1, 0x03),
    LOGICAL_MIN(1, 0x00),
    LOGICAL_MAX(2, 0xff, 0x00),
    REPORT_SIZE(1, 8),
    REPORT_COUNT(1, 0x40),
    INPUT(1, 0x00),
    USAGE(1, 0x04),
    LOGICAL_MIN(1, 0x00),
    LOGICAL_MAX(2, 0xff, 0x00),
    REPORT_SIZE(1, 0x08),
    REPORT_COUNT(1, 0X40),
    OUTPUT(1, 0x00),
    END_COLLECTION,
};

static u32 get_hid_report_desc_len(u32 index)
{
    u32 len = 0;
    len = sizeof(sHIDReportDesc);
    return len;
}
static void *get_hid_report_desc(u32 index)
{
    u8 *ptr  = NULL;
    ptr = (u8 *)sHIDReportDesc;
    return ptr;
}

u32 custom_hid_tx_data(const usb_dev usb_id, const u8 *buffer, u32 len)
{
    if (len > MAXP_SIZE_CUSTOM_HIDIN) {
        len = MAXP_SIZE_CUSTOM_HIDIN;
    }
    if (custom_hid_info == NULL || custom_hid_info->cfg_done == 0) {
        return 0;
    }
    return usb_g_intr_write(usb_id, CUSTOM_HID_EP_IN, buffer, len);
}

static void custom_hid_rx_data(struct usb_device_t *usb_device, u32 ep)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u8 rx_buffer[64] = {0};
    u32 rx_len = usb_g_intr_read(usb_id, CUSTOM_HID_EP_OUT, rx_buffer, 64, 0);
    if (custom_hid_info && custom_hid_info->hid_rx_hook) {
        custom_hid_info->hid_rx_hook(custom_hid_info->priv_hdl, rx_buffer, rx_len);
    }
}

int custom_hid_get_ready(const usb_dev usb_id)
{
    if (custom_hid_info && custom_hid_info->cfg_done) {
        return 1;
    }
    return 0;
}

void custom_hid_set_rx_hook(void *priv, void (*rx_hook)(void *priv, u8 *buf, u32 len))
{
    if (custom_hid_info) {
        custom_hid_info->priv_hdl = priv;
        custom_hid_info->hid_rx_hook = rx_hook;
    }
}

static u8 *custom_hid_ep_in_dma;
static u8 *custom_hid_ep_out_dma;
static void custom_hid_endpoint_init(struct usb_device_t *usb_device, u32 itf)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    //u8 *ep_buffer = usb_alloc_ep_dmabuffer(usb_id, CUSTOM_HID_EP_IN | USB_DIR_IN, MAXP_SIZE_CUSTOM_HIDIN);
    usb_g_ep_config(usb_id, CUSTOM_HID_EP_IN | USB_DIR_IN, USB_ENDPOINT_XFER_INT, 0, custom_hid_ep_in_dma, MAXP_SIZE_CUSTOM_HIDIN);

    //ep_buffer = usb_alloc_ep_dmabuffer(usb_id, CUSTOM_HID_EP_OUT, MAXP_SIZE_CUSTOM_HIDOUT);
    usb_g_set_intr_hander(usb_id, CUSTOM_HID_EP_OUT, custom_hid_rx_data);
    usb_g_ep_config(usb_id, CUSTOM_HID_EP_OUT, USB_ENDPOINT_XFER_INT, 1, custom_hid_ep_out_dma, MAXP_SIZE_CUSTOM_HIDOUT);
    usb_enable_ep(usb_id, CUSTOM_HID_EP_OUT);
}

static void custom_hid_reset(struct usb_device_t *usb_device, u32 itf)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    log_debug("%s", __func__);
#if USB_ROOT2
    usb_disable_ep(usb_id, HID_EP_OUT);
#else
    custom_hid_endpoint_init(usb_device, itf);
#endif
}

static u32 custom_hid_itf_hander(struct usb_device_t *usb_device, struct usb_ctrlrequest *req)
{
    const usb_dev usb_id = usb_device2id(usb_device);
    u32 tx_len;
    u8 *tx_payload = usb_get_setup_buffer(usb_device);
    u32 bRequestType = req->bRequestType & USB_TYPE_MASK;
    switch (bRequestType) {
    case USB_TYPE_STANDARD:
        switch (req->bRequest) {
        case USB_REQ_GET_DESCRIPTOR:
            switch (HIBYTE(req->wValue)) {
            case USB_HID_DT_HID:
                tx_payload = (u8 *)sHIDDescriptor + USB_DT_INTERFACE_SIZE;
                tx_len = 9;
                tx_payload = usb_set_data_payload(usb_device, req, tx_payload, tx_len);
                tx_payload[7] = LOBYTE(get_hid_report_desc_len(req->wIndex));
                tx_payload[8] = HIBYTE(get_hid_report_desc_len(req->wIndex));
                break;
            case USB_HID_DT_REPORT:
                tx_len = get_hid_report_desc_len(req->wIndex);
                tx_payload = get_hid_report_desc(req->wIndex);
                usb_set_data_payload(usb_device, req, tx_payload, tx_len);
                break;
            default:
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }// USB_REQ_GET_DESCRIPTOR
            break;
        case USB_REQ_SET_DESCRIPTOR:
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            break;
        case USB_REQ_SET_INTERFACE:
            if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
                //只有一个interface 没有Alternate
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
            break;
        case USB_REQ_GET_INTERFACE:
            if (req->wLength) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_CONFIGURED) {
                tx_len = 1;
                tx_payload[0] = 0x00;
                usb_set_data_payload(usb_device, req, tx_payload, tx_len);
            }
            break;
        case USB_REQ_GET_STATUS:
            if (usb_device->bDeviceStates == USB_DEFAULT) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else if (usb_device->bDeviceStates == USB_ADDRESS) {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            } else {
                usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
            }
            break;
        default:
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        }//bRequest @ USB_TYPE_STANDARD
        break;

    case USB_TYPE_CLASS:
        switch (req->bRequest) {
        case USB_REQ_SET_IDLE:
            custom_hid_endpoint_init(usb_device, LOBYTE(req->wIndex));
            usb_set_setup_phase(usb_device, USB_EP0_STAGE_SETUP);
            custom_hid_info->cfg_done = 1;
            break;
        case USB_REQ_GET_IDLE:
            tx_len = 1;
            tx_payload[0] = 0;
            usb_set_data_payload(usb_device, req, tx_payload, tx_len);
            break;
        default:
            usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
        }//bRequest @ USB_TYPE_CLASS
        break;

    default:
        usb_set_setup_phase(usb_device, USB_EP0_SET_STALL);
    }
    return 0;
}

u32 custom_hid_desc_config(const usb_dev usb_id, u8 *ptr, u32 *cur_itf_num)
{
    log_debug("custom hid interface num:%d\n", *cur_itf_num);
    memcpy(ptr, sHIDDescriptor, sizeof(sHIDDescriptor));
    ptr[2] = *cur_itf_num;
    ptr[USB_DT_INTERFACE_SIZE + 7] = LOBYTE(get_hid_report_desc_len(0));
    ptr[USB_DT_INTERFACE_SIZE + 8] = HIBYTE(get_hid_report_desc_len(0));

    if (usb_set_interface_hander(usb_id, *cur_itf_num, custom_hid_itf_hander) != *cur_itf_num) {
        ASSERT(0, "custom hid set interface_hander fail");
    }
    if (usb_set_reset_hander(usb_id, *cur_itf_num, custom_hid_reset) != *cur_itf_num) {
        ASSERT(0, "custom hid set interface_reset_hander fail");
    }

    *cur_itf_num = *cur_itf_num + 1;
    return sizeof(sHIDDescriptor);
}

u32 custom_hid_register(usb_dev usb_id)
{
    if (custom_hid_info) {
        return 0;
    }
#if USB_MALLOC_ENABLE
    custom_hid_info = malloc(sizeof(struct custom_hid_hdl));
    if (!custom_hid_info) {
        log_error("custom hid allocates memory fail 1\n");
        return -1;
    }
#else
    custom_hid_info = &_custom_hid_info;
#endif
    memset(custom_hid_info, 0, sizeof(struct custom_hid_hdl));
    custom_hid_ep_in_dma = usb_alloc_ep_dmabuffer(usb_id, CUSTOM_HID_EP_IN | USB_DIR_IN, MAXP_SIZE_CUSTOM_HIDIN);
    custom_hid_ep_out_dma = usb_alloc_ep_dmabuffer(usb_id, CUSTOM_HID_EP_OUT, MAXP_SIZE_CUSTOM_HIDOUT);
    return 0;
}

void custom_hid_release()
{
    if (custom_hid_info == NULL) {
        return;
    }
#if USB_MALLOC_ENABLE
    free(custom_hid_info);
#else
    memset(custom_hid_info, 0, sizeof(struct custom_hid_hdl));
#endif
    custom_hid_info = NULL;
}



#endif
