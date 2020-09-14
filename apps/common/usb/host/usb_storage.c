/** * @file usb_storage.c
 * @brief
 * @author chenrixin@zh-jieli.com
 * @version 1.00
 * @date 2017-02-09
 */

#include "asm/includes.h"
#include "system/timer.h"
#include "device/ioctl_cmds.h"
#include "usb/host/usb_host.h"
#include "usb_ctrl_transfer.h"
#include "usb_bulk_transfer.h"
#include "device_drive.h"
#include "usb_storage.h"
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

#if TCFG_UDISK_ENABLE

static int set_stor_power(struct usb_host_device *host_dev, u32 value);
static int get_stor_power(struct usb_host_device *host_dev, u32 value);

const struct interface_ctrl udisk_ops = {
    .interface_class = USB_CLASS_MASS_STORAGE,
    .set_power = set_stor_power,
    .get_power = get_stor_power,
    .ioctl = NULL,
};

static struct mass_storage mass_stor ;//SEC(.usb_h_udisk);

static const struct usb_interface_info udisk_inf = {
    .ctrl = (struct interface_ctrl *) &udisk_ops,
    .dev.disk = &mass_stor,
};

static struct device udisk_device ;//SEC(.usb_h_udisk);
static struct udisk_end_desc udisk_ep ;//SEC(.usb_h_udisk);

#define     host_device2disk(host_dev)      (udisk_inf.dev.disk)

/**
 * @brief usb_stor_force_reset
 *
 * @param device
 *
 * @return
 */
static int usb_stor_force_reset(const usb_dev usb_id)
{
    /* struct usb_host_device *host_dev = device_to_usbdev(device);
    const usb_dev usb_id = host_device2id(host_dev); */
    log_error("============usb_stor_force_reset===============\n");
    return usb_h_force_reset(usb_id);
}
static void usb_stor_set_curlun(struct mass_storage *disk, u32 i)
{
#if MULTI_DISK
    disk->curlun = i;
#endif
}
static int usb_stor_get_curlun(struct mass_storage *disk)
{
#if MULTI_DISK
    return disk->curlun;
#else
    return 0;
#endif
}
static int usb_stor_txmaxp(struct mass_storage *disk)
{
#if HUSB_MODE
    return udisk_ep.txmaxp;
#else
    return 0x40;
#endif
}
static int usb_stor_rxmaxp(struct mass_storage *disk)
{
#if HUSB_MODE
    return udisk_ep.rxmaxp;
#else
    return 0x40;
#endif
}
#define     UDISK_MUTEX_TIMEOUT     5000/10 //5s
static int usb_h_mutex_pend(struct usb_host_device *host_dev)
{
    struct mass_storage *disk = host_device2disk(host_dev);
    /* putchar('{'); */
    /* putchar('^'); */
    /* putchar('['); */

    int r = os_mutex_pend(&disk->mutex, UDISK_MUTEX_TIMEOUT);
    if (r) {
        log_error("-----------------%d -------------------", r);
    }
    if (r == OS_TIMEOUT) {
        return OS_TIMEOUT;
    }
    return r;
}
static int usb_h_mutex_post(struct usb_host_device *host_dev)
{
    struct mass_storage *disk = host_device2disk(host_dev);
    /* putchar(']'); */
    /* putchar('^'); */
    /* putchar('}'); */

    int r = os_mutex_post(&disk->mutex);
    return r;
}
static int set_stor_power(struct usb_host_device *host_dev, u32 value)
{
    struct mass_storage *disk = host_device2disk(host_dev);
    log_debug("%s() %d ", __func__, disk->dev_status);
    if (disk == NULL || disk->dev_status == DEV_IDLE) {
        return 0;
    }
    int r = usb_h_mutex_pend(host_dev);

    if (disk->dev_status == DEV_READ || disk->dev_status == DEV_WRITE) {
        if (value == -1) {
            log_error("%s disk busy", __func__);
        }
    }

    disk->dev_status = DEV_CLOSE;

    log_debug("%s() %d ", __func__, disk->dev_status);

    if (r == OS_TIMEOUT) {
        return OS_TIMEOUT;
    }
    usb_h_mutex_post(host_dev);
    return DEV_ERR_NONE;
}

static int get_stor_power(struct usb_host_device *host_dev, u32 value)
{
    return DEV_ERR_NONE;
}

/**
 * @brief usb_init_cbw
 *
 * @param device
 */
static void usb_init_cbw(struct device *device, u32 dir, u32 opcode, u32 length)
{
    struct mass_storage *disk = host_device2disk(host_dev);

    memset(&disk->cbw, 0x00, sizeof(struct usb_scsi_cbw));
#if MULTI_DISK
    u32 curlun = usb_stor_get_curlun(disk);
#else
    u32 curlun = disk->cur_available_lun;
#endif
    disk->cbw.dCBWSignature = CBW_SIGNATURE;
    disk->cbw.dCBWTag = rand32();
    disk->cbw.bCBWLUN = curlun;
    disk->cbw.lun = curlun << 5;
    disk->cbw.bmCBWFlags = dir;
    disk->cbw.operationCode = opcode;
    disk->cbw.dCBWDataTransferLength = cpu_to_le32(length);
}

static int usb_stor_check_status(struct usb_host_device *host_dev)
{
    struct mass_storage *disk = host_device2disk(host_dev);

    if (!host_dev) {
        return -DEV_ERR_NOT_MOUNT;
    }
    if (!disk) {
        return -DEV_ERR_NOT_MOUNT;
    }
    if (disk->dev_status == DEV_IDLE) {
        return -DEV_ERR_NOT_MOUNT;
    } else if (disk->dev_status == DEV_CLOSE) {
        return -DEV_ERR_NOT_MOUNT;
    }
    return 0;
}

static int usb_stor_reset(struct device *device)
{
    struct usb_host_device *host_dev = device_to_usbdev(device);
    struct mass_storage *disk = host_device2disk(host_dev);
    int ret = DEV_ERR_NONE;

    if (disk == NULL) {
        return -DEV_ERR_NOT_MOUNT;
    }
    //mass storage reset request
    ret = set_msd_reset(host_dev);
    return ret;
}

static int usb_stor_resume(struct device *device)
{
    struct usb_device_descriptor device_desc;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    const usb_dev usb_id = host_device2id(host_dev);
    u8 devnum = host_dev->private_data.devnum;
    int ret = DEV_ERR_NONE;
    ret = usb_stor_check_status(host_dev);
    if (ret) {
        return ret;
    }

    usb_h_resume(usb_id);

    ret = usb_get_device_descriptor(host_dev, &device_desc);
    if (ret == 0) {
        return ret;
    }
    usb_h_force_reset(usb_id);
    ret = set_address(host_dev, devnum);
    if (ret) {
        log_error("%s() @ %d %x\n", __func__, __LINE__, ret);
        usb_sie_close(usb_id);
        return -ret;
    }
    ret = usb_get_device_descriptor(host_dev, &device_desc);
    if (ret) {
        log_error("%s() @ %d %x\n", __func__, __LINE__, ret);
        usb_sie_close(usb_id);
        return -ret;
    }
    return ret;
}
/**
 * @brief usb_stor_inquiry
 *
 * @param device
 *
 * @return
 */
static int _usb_stor_inquiry(struct device *device)
{
    struct usb_host_device *host_dev = device_to_usbdev(device);
    struct mass_storage *disk = host_device2disk(host_dev);
    const usb_dev usb_id = host_device2id(host_dev);

    const u32 txmaxp = usb_stor_txmaxp(disk);
    const u32 rxmaxp = usb_stor_rxmaxp(disk);

    int ret = DEV_ERR_NONE;
    struct inquiry_data inquiry;

    log_info("usb_stor_inquiry disk =%x", (u32)disk);

    u32 total_lun = 1;//disk->lun;

    for (int i = 0; i <= total_lun; i ++) {
        usb_stor_set_curlun(disk, i);
        usb_init_cbw(device, USB_DIR_IN, INQUIRY, sizeof(struct inquiry_data));
        disk->cbw.bCBWLength = 0x06;
        disk->cbw.lba[2] = sizeof(struct inquiry_data);

        //cbw
        ret = usb_bulk_only_send(device,
                                 udisk_ep.host_epout,
                                 txmaxp,
                                 udisk_ep.target_epout,
                                 (u8 *)&disk->cbw,
                                 sizeof(struct usb_scsi_cbw));

        if (ret < DEV_ERR_NONE) {
            log_error("%s:%d\n", __func__, __LINE__);
            goto __exit;
        }

        //data
        ret = usb_bulk_only_receive(device,
                                    udisk_ep.host_epin,
                                    rxmaxp,
                                    udisk_ep.target_epin,
                                    (u8 *)&inquiry,
                                    sizeof(struct inquiry_data));

        if (ret < DEV_ERR_NONE) {
            log_error("%s:%d\n", __func__, __LINE__);
            goto __exit;
        } else if (ret != sizeof(struct inquiry_data)) {
            ret = -DEV_ERR_UNKNOW;
            log_error("%s:%d\n", __func__, __LINE__);
            goto __exit;
        }

        /* printf_buf(&inquiry, sizeof(struct INQUIRY_DATA)); */
        //csw
        ret = usb_bulk_only_receive(device,
                                    udisk_ep.host_epin,
                                    rxmaxp,
                                    udisk_ep.target_epin,
                                    (u8 *)&disk->csw,
                                    sizeof(struct usb_scsi_csw));

        if (ret < DEV_ERR_NONE) {
            log_error("%s:%d\n", __func__, __LINE__);
            goto __exit;
        } else if (ret != sizeof(struct usb_scsi_csw)) {
            ret = -DEV_ERR_UNKNOW;
            log_error("%s:%d\n", __func__, __LINE__);
            goto __exit;
        }
        if (inquiry.PeripheralDeviceType) {
            continue;
        } else {
            break;
        }
    }

    return DEV_ERR_NONE;
__exit:
    log_error("%s---%d\n", __func__, __LINE__);
    return ret;
}

static int usb_stor_inquiry(struct device *device)
{
    int ret;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    struct mass_storage *disk = host_device2disk(host_dev);
    ret = usb_stor_check_status(host_dev);
    if (ret) {
        return ret;
    }
    ret = usb_h_mutex_pend(host_dev);
    if (ret == OS_TIMEOUT) {
        return -OS_TIMEOUT;
    }

    ret = _usb_stor_inquiry(device);
    usb_h_mutex_post(host_dev);
    return ret;
}
/**
 * @brief usb_stor_test_unit_ready
 *
 * @param device
 *
 * @return
 */
static int _usb_stor_test_unit_ready(struct device *device)
{
    int ret = DEV_ERR_NONE;

    struct usb_host_device *host_dev = device_to_usbdev(device);
    struct mass_storage *disk = host_device2disk(host_dev);
    const usb_dev usb_id = host_device2id(host_dev);

    const u32 txmaxp = usb_stor_txmaxp(disk);
    const u32 rxmaxp = usb_stor_rxmaxp(disk);


    usb_init_cbw(device, 0, TEST_UNIT_READY, 0);
    disk->cbw.bCBWLength = 6;
    //cbw
    ret = usb_bulk_only_send(device,
                             udisk_ep.host_epout,
                             txmaxp,
                             udisk_ep.target_epout,
                             (u8 *)&disk->cbw,
                             sizeof(struct usb_scsi_cbw));

    if (ret < DEV_ERR_NONE) {
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }

    //csw
    ret = usb_bulk_only_receive(device,
                                udisk_ep.host_epin,
                                rxmaxp,
                                udisk_ep.target_epin,
                                (u8 *)&disk->csw,
                                sizeof(struct usb_scsi_csw));

    if (ret < DEV_ERR_NONE) {
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    } else if (ret != sizeof(struct usb_scsi_csw)) {
        ret = -DEV_ERR_UNKNOW;
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }

    return DEV_ERR_NONE;
__exit:
    log_error("%s---%d\n", __func__, __LINE__);
    return ret;
}
static int usb_stor_test_unit_ready(struct device *device)
{
    int ret;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    struct mass_storage *disk = host_device2disk(host_dev);
    ret = usb_stor_check_status(host_dev);
    if (ret) {
        return ret;
    }
    ret = usb_h_mutex_pend(host_dev);
    if (ret == OS_TIMEOUT) {
        return -OS_TIMEOUT;
    }
    ret = _usb_stor_test_unit_ready(device);
    usb_h_mutex_post(host_dev);
    return ret;
}

/**
 * @brief usb_stor_mode_sense6
 *
 * @param device
 *
 * @return
 *
 * @brief disk write protect inquiry
 */
static int _usb_stor_mode_sense6(struct device *device)
{
    struct usb_host_device *host_dev = device_to_usbdev(device);
    struct mass_storage *disk = host_device2disk(host_dev);
    const usb_dev usb_id = host_device2id(host_dev);
    u8 *cdb;
    int ret = DEV_ERR_NONE;
    u8 data_buf[4];


    const u32 txmaxp = usb_stor_txmaxp(disk);
    const u32 rxmaxp = usb_stor_rxmaxp(disk);

    usb_init_cbw(device, USB_DIR_IN, MODE_SENSE, 4);
    disk->cbw.bCBWLUN = 6;
    cdb = (u8 *)&disk->cbw.operationCode;
    cdb[1] = 0;  // PC | DBD
    cdb[2] = 0x1c;  //page code
    cdb[3] = 0;  //subpage code
    cdb[4] = 192;  //allocation length
    cdb[5] = 0;  //control;
    //cbw
    ret = usb_bulk_only_send(device,
                             udisk_ep.host_epout,
                             txmaxp,
                             udisk_ep.target_epout,
                             (u8 *)&disk->cbw,
                             sizeof(struct usb_scsi_cbw));
    if (ret < DEV_ERR_NONE) {
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }

    //data
    ret = usb_bulk_only_receive(device,
                                udisk_ep.host_epin,
                                rxmaxp, udisk_ep.target_epin,
                                data_buf,
                                sizeof(data_buf));
    if (ret < DEV_ERR_NONE) {
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }
    disk->read_only = data_buf[2] == 0x80 ? 1 : 0;  //write protect

    //csw
    ret = usb_bulk_only_receive(device,
                                udisk_ep.host_epin,
                                rxmaxp,
                                udisk_ep.target_epin,
                                (u8 *)&disk->csw,
                                sizeof(struct usb_scsi_csw));
    if (ret < DEV_ERR_NONE) {
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    } else if (ret != sizeof(struct usb_scsi_csw)) {
        ret = -DEV_ERR_NONE;
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }

    return DEV_ERR_NONE;
__exit:
    log_error("%s---%d\n", __func__, __LINE__);
    return ret;
}
static int usb_stor_mode_sense6(struct device *device)
{
    int ret;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    struct mass_storage *disk = host_device2disk(host_dev);
    ret = usb_stor_check_status(host_dev);
    if (ret) {
        return ret;
    }
    ret = usb_h_mutex_pend(host_dev);
    if (ret == OS_TIMEOUT) {
        return -OS_TIMEOUT;
    }
    ret = _usb_stor_mode_sense6(device);
    usb_h_mutex_post(host_dev);
    return ret;

}
/**
 * @brief usb_stor_request_sense
 *
 * @param device
 *
 * @return
 */
static int _usb_stor_request_sense(struct device *device)
{
    struct usb_host_device *host_dev = device_to_usbdev(device);
    struct mass_storage *disk = host_device2disk(host_dev);
    const usb_dev usb_id = host_device2id(host_dev);
    int ret = DEV_ERR_NONE;


    const u32 txmaxp = usb_stor_txmaxp(disk);
    const u32 rxmaxp = usb_stor_rxmaxp(disk);

    usb_init_cbw(device, USB_DIR_IN, REQUEST_SENSE, sizeof(struct request_sense_data));
    disk->cbw.bCBWLength = 0xC;
    disk->cbw.lba[2] = sizeof(struct request_sense_data);
    //cbw
    ret = usb_bulk_only_send(device,
                             udisk_ep.host_epout,
                             txmaxp,
                             udisk_ep.target_epout,
                             (u8 *)&disk->cbw,
                             sizeof(struct usb_scsi_cbw));

    if (ret < DEV_ERR_NONE) {
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }

    //data
    ret = usb_bulk_only_receive(device,
                                udisk_ep.host_epin,
                                rxmaxp,
                                udisk_ep.target_epin,
                                (u8 *)&disk->sense,
                                sizeof(struct request_sense_data));

    if (ret < DEV_ERR_NONE) {
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    } else if (ret != sizeof(struct request_sense_data)) {
        ret = -DEV_ERR_NONE;
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }

    /* printf_buf(&private_data->dev.disk->sense, sizeof(struct REQUEST_SENSE_DATA)); */
    //csw
    ret = usb_bulk_only_receive(device,
                                udisk_ep.host_epin,
                                rxmaxp,
                                udisk_ep.target_epin,
                                (u8 *)&disk->csw,
                                sizeof(struct usb_scsi_csw));

    if (ret < DEV_ERR_NONE) {
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    } else if (ret != sizeof(struct usb_scsi_csw)) {
        ret = -DEV_ERR_UNKNOW;
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }

    return DEV_ERR_NONE;
__exit:
    log_error("%s---%d\n", __func__, __LINE__);
    return ret;
}
static int usb_stor_request_sense(struct device *device)
{
    int ret;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    struct mass_storage *disk = host_device2disk(host_dev);
    ret = usb_stor_check_status(host_dev);
    if (ret) {
        return ret;
    }
    ret = usb_h_mutex_pend(host_dev);
    if (ret == OS_TIMEOUT) {
        return -OS_TIMEOUT;
    }
    ret = _usb_stor_request_sense(device);
    usb_h_mutex_post(host_dev);
    return ret;
}

/**
 * @brief usb_stor_read_capacity
 *
 * @param device
 *
 * @return
 */
static int _usb_stor_read_capacity(struct device *device)
{
    struct usb_host_device *host_dev = device_to_usbdev(device);
    struct mass_storage *disk = host_device2disk(host_dev);
    const usb_dev usb_id = host_device2id(host_dev);
    int ret = DEV_ERR_NONE;

    if (disk == NULL) {
        return -DEV_ERR_NOT_MOUNT;
    }

    const u32 txmaxp = usb_stor_txmaxp(disk);
    const u32 rxmaxp = usb_stor_rxmaxp(disk);

    u32 curlun = usb_stor_get_curlun(disk);

    usb_init_cbw(device, USB_DIR_IN, READ_CAPACITY, 8);
    disk->cbw.bCBWLength = 0xA;
    disk->cbw.bmCBWFlags = USB_DIR_IN;
    disk->cbw.operationCode = READ_CAPACITY;
    disk->capacity[curlun].block_size = 0;
    disk->capacity[curlun].block_num = 0;

    //cbw
    ret = usb_bulk_only_send(device,
                             udisk_ep.host_epout,
                             txmaxp,
                             udisk_ep.target_epout,
                             (u8 *)&disk->cbw,
                             sizeof(struct usb_scsi_cbw));


    if (ret < DEV_ERR_NONE) {
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }

    //data
    ret = usb_bulk_only_receive(device,
                                udisk_ep.host_epin,
                                rxmaxp,
                                udisk_ep.target_epin,
                                (u8 *)&disk->capacity[curlun],
                                sizeof(struct read_capacity_data));

    if (ret < DEV_ERR_NONE) {
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    } else if (ret != sizeof(struct read_capacity_data)) {
        ret = -DEV_ERR_UNKNOW;
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }

    disk->capacity[curlun].block_size = be32_to_cpu(disk->capacity[curlun].block_size);
    disk->capacity[curlun].block_num = be32_to_cpu(disk->capacity[curlun].block_num) + 1;
    log_info("block_size=%d", disk->capacity[curlun].block_size);
    log_info("block_num=%d", disk->capacity[curlun].block_num);
    //csw
    ret = usb_bulk_only_receive(device,
                                udisk_ep.host_epin,
                                rxmaxp,
                                udisk_ep.target_epin,
                                (u8 *)&disk->csw,
                                sizeof(struct usb_scsi_csw));

    if (ret < DEV_ERR_NONE) {
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    } else if (ret != sizeof(struct usb_scsi_csw)) {
        ret = -DEV_ERR_UNKNOW;
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }

    return DEV_ERR_NONE;
__exit:
    log_error("%s---%d\n", __func__, __LINE__);
    return ret;
}
static int usb_stor_read_capacity(struct device *device)
{
    int ret;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    struct mass_storage *disk = host_device2disk(host_dev);
    ret = usb_stor_check_status(host_dev);
    if (ret) {
        return ret;
    }
    ret = usb_h_mutex_pend(host_dev);
    if (ret == OS_TIMEOUT) {
        return -OS_TIMEOUT;
    }
    ret = _usb_stor_read_capacity(device);
    usb_h_mutex_post(host_dev);
    return ret;
}


/**
 * @brief usb_stor_read 从U盘的lba扇区读取num_lba个扇区
 *
 * @param device 设备句柄
 * @param pBuf 读buffer缓冲区，芯片所有memory都可以
 * @param lba 需要读取的扇区地址
 * @param num_lba 需要读取的扇区数量
 *
 * @return 负数表示失败
 * 			大于0的表示读取的字节数(Byte)
 */
static int _usb_stor_read(struct device *device, void *pBuf, u32 num_lba, u32 lba)
{
    struct usb_host_device *host_dev = device_to_usbdev(device);
    struct mass_storage *disk = host_device2disk(host_dev);
    const usb_dev usb_id = host_device2id(host_dev);

    const u32 txmaxp = usb_stor_txmaxp(disk);
    const u32 rxmaxp = usb_stor_rxmaxp(disk);
    const u32 curlun = usb_stor_get_curlun(disk);

    u8 read_retry = 0;
    int ret;
    u32 rx_cnt;
    u32 rx_len = num_lba * disk->capacity[curlun].block_size;

    if (lba + num_lba/* - 1*/ >= disk->capacity[curlun].block_num) {
        return -DEV_ERR_OVER_CAPACITY;
    }

    disk->dev_status = DEV_READ;
    disk->suspend_cnt = 0;
    usb_init_cbw(device, USB_DIR_IN, READ_10, rx_len);
    disk->cbw.bCBWLength = 0xA;

    lba = cpu_to_be32(lba);
    memcpy(disk->cbw.lba, &lba, sizeof(lba));

    disk->cbw.LengthH = HIBYTE(num_lba);
    disk->cbw.LengthL = LOBYTE(num_lba);

    //cbw
    ret = usb_bulk_only_send(device,
                             udisk_ep.host_epout,
                             txmaxp,
                             udisk_ep.target_epout,
                             (u8 *)&disk->cbw,
                             sizeof(struct usb_scsi_cbw));

    if (ret < DEV_ERR_NONE) {
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }

    //data
    ret = usb_bulk_only_receive(device,
                                udisk_ep.host_epin,
                                rxmaxp,
                                udisk_ep.target_epin,
                                pBuf,
                                rx_len);
    if (ret < DEV_ERR_NONE) {
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }

    //csw
    ret = usb_bulk_only_receive(device,
                                udisk_ep.host_epin,
                                rxmaxp,
                                udisk_ep.target_epin,
                                (u8 *)&disk->csw,
                                sizeof(struct usb_scsi_csw));

    if (ret < DEV_ERR_NONE) {
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    } else if (ret != sizeof(struct usb_scsi_csw)) {
        ret = -DEV_ERR_UNKNOW;
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }

    disk->dev_status = DEV_OPEN;
    return num_lba;

__exit:
    if (disk->dev_status != DEV_CLOSE) {
        disk->dev_status = DEV_OPEN;
    }
    log_error("%s---%d", __func__, __LINE__);
    return 0;
}
static int usb_stor_read(struct device *device, void *pBuf, u32 num_lba, u32 lba)
{
    int ret;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    struct mass_storage *disk = host_device2disk(host_dev);
    ret = usb_stor_check_status(host_dev);
    if (ret) {
        return ret;
    }
    ret = usb_h_mutex_pend(host_dev);
    if (ret == OS_TIMEOUT) {
        return -OS_TIMEOUT;
    }
    ret = _usb_stor_read(device, pBuf, num_lba, lba);
    usb_h_mutex_post(host_dev);
    return ret;
}

/**
 * @brief usb_stor_write 写数据到U盘
 *
 * @param device
 * @param pBuf 数据buffer
 * @param lba   需要写入的扇区地址
 * @param num_lba 需要写入的扇区数量
 *
 * @return 负数表示写入失败
 * 		正数为写入的字节数 (Byte)
 */
static int _usb_stor_write(struct device *device, void *pBuf,  u32 num_lba, u32 lba)
{
    struct usb_host_device *host_dev = device_to_usbdev(device);
    struct mass_storage *disk = host_device2disk(host_dev);
    const usb_dev usb_id = host_device2id(host_dev);
    u32 ret;

    const u32 txmaxp = usb_stor_txmaxp(disk);
    const u32 rxmaxp = usb_stor_rxmaxp(disk);
    const u32 curlun = usb_stor_get_curlun(disk);

    if (lba + num_lba/* - 1*/ >= disk->capacity[curlun].block_num) {
        log_error("%s %d", __func__, __LINE__);
        return -DEV_ERR_OVER_CAPACITY;
    }

    if (disk->read_only) {
        log_error("%s %d", __func__, __LINE__);
        return -DEV_ERR_NO_WRITE;
    }

    u32 tx_len = num_lba * disk->capacity[curlun].block_size;


    disk->dev_status = DEV_WRITE;
    disk->suspend_cnt = 0;
    usb_init_cbw(device, USB_DIR_OUT, WRITE_10, tx_len);

    disk->cbw.bCBWLength = 0xA;

    lba = cpu_to_be32(lba);
    memcpy(disk->cbw.lba, &lba, sizeof(lba));


    disk->cbw.LengthH = HIBYTE(num_lba);
    disk->cbw.LengthL = LOBYTE(num_lba);

    //cbw
    ret = usb_bulk_only_send(device,
                             udisk_ep.host_epout,
                             txmaxp,
                             udisk_ep.target_epout,
                             (u8 *)&disk->cbw,
                             sizeof(struct usb_scsi_cbw));

    if (ret < DEV_ERR_NONE) {
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }

    //data
    ret = usb_bulk_only_send(device,
                             udisk_ep.host_epout,
                             txmaxp,
                             udisk_ep.target_epout,
                             pBuf,
                             tx_len);

    if (ret < DEV_ERR_NONE) {
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    }

    //csw
    ret = usb_bulk_only_receive(device,
                                udisk_ep.host_epin,
                                rxmaxp,
                                udisk_ep.target_epin,
                                (u8 *)&disk->csw,
                                sizeof(struct usb_scsi_csw));

    if (ret < DEV_ERR_NONE) {
        log_error("%s:%d\n", __func__, __LINE__);
        goto __exit;
    } else if (ret != sizeof(struct usb_scsi_csw)) {
        ret = -DEV_ERR_UNKNOW;
        goto __exit;
    }

    if (disk->csw.bCSWStatus) {
        ret = usb_stor_request_sense(device);
        if (ret) {
            ret = -DEV_ERR_OFFLINE;
        }

        if (ret == -DEV_ERR_OFFLINE) {
            log_error("%s:%d\n", __func__, __LINE__);
            goto __exit;
        }
        log_info("usb_stor_sense =%x", disk->sense.SenseKey);
        return 0;
    }

    disk->dev_status = DEV_OPEN;
    return  num_lba;//DEV_ERR_NONE;

__exit:
    if (disk->dev_status != DEV_CLOSE) {
        disk->dev_status = DEV_OPEN;
    }
    log_error("%s---%d\n", __func__, __LINE__);
    /* usb_stor_force_reset(usb_id); */
    return 0;
}
static int usb_stor_write(struct device *device, void *pBuf,  u32 num_lba, u32 lba)
{
    int ret;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    struct mass_storage *disk = host_device2disk(host_dev);
    ret = usb_stor_check_status(host_dev);
    if (ret) {
        return ret;
    }
    ret = usb_h_mutex_pend(host_dev);
    if (ret == OS_TIMEOUT) {
        return -OS_TIMEOUT;
    }
    ret = _usb_stor_write(device, pBuf, num_lba, lba);
    usb_h_mutex_post(host_dev);
    return ret;
}
#if ENABLE_DISK_HOTPLUG
static void usb_stor_tick_handle(void *arg)
{
    struct device *device = (struct device *)arg;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    struct mass_storage *disk = host_device2disk(host_dev);//(device);
    const usb_dev usb_id = host_device2id(host_dev);

    int ret;

    r_printf("A");
    if (disk == NULL) {
        return;
    }
    r_printf("B");
    if (disk->dev_status == DEV_OPEN) {
        r_printf("C");
        ret = usb_stor_read_capacity(device);
        r_printf("D");
    }

#if 0
#if 1
    disk->suspend_cnt++;
    if ((disk->suspend_cnt > 6) && (disk->dev_status == DEV_OPEN)) {
        usb_h_entry_suspend(usb_id);
        disk->suspend_cnt = 0;
        disk->dev_status = DEV_SUSPEND;
    }
#else
    ret = usb_stor_test_unit_ready(device);
    if (ret < DEV_ERR_NONE) {
        return ;
    }
    if (disk->csw.bCSWStatus) {
        usb_stor_request_sense(device);
        /* puts("request sense\n"); */
        /* put_buf((u8 *)&disk->sense, sizeof(disk->sense)); */
        if ((disk->sense.SenseKey & 0x0f) == NOT_READY) {
            if (disk->sense.ASC == 0x3a && disk->sense.ASCQ == 0x00) {
                disk->media_sta_cur = 0;
            }
        } else if ((disk->sense.SenseKey & 0x0f) == UNIT_ATTENTION) {
            if (disk->sense.ASC == 0x28 && disk->sense.ASCQ == 0x00) {
                disk->media_sta_cur = 1;
            }
        }
    } else {
        disk->media_sta_cur = 1;
    }

    if (disk->media_sta_cur != disk->media_sta_prev) {
        if (disk->media_sta_cur) {
            log_info("udisk media insert");
            usb_stor_read_capacity(device);
            usb_stor_mode_sense6(device);
        } else {
            log_info("udisk media eject");
        }
    }
    disk->media_sta_prev = disk->media_sta_cur;
#endif
#endif
}
#endif

/**
 * @brief usb_stor_init
 *
 * @param device
 *
 * @return
 */
int usb_stor_init(struct device *device)
{
    log_debug("%s---%d\n", __func__, __LINE__);
    int ret = DEV_ERR_NONE;
    u32 state = 0;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    struct mass_storage *disk = host_device2disk(host_dev);
    log_error("%s() disk = %x", __func__, disk);

    if (!host_dev_status(host_dev)) {
        return -DEV_ERR_OFFLINE;
    }

    const usb_dev usb_id = host_device2id(host_dev);
    if (host_dev->private_data.status == 0) {
        return -ENODEV;
    }

    if (!disk) {
        log_error("%s not enough memory");
        return -DEV_ERR_OUTOFMEMORY;
    }

    disk->dev_status = DEV_INIT;

    const u32 txmaxp = usb_stor_txmaxp(disk);
    const u32 rxmaxp = usb_stor_rxmaxp(disk);

    u8 lun;
    ret = get_msd_max_lun(host_dev, &lun);
    if (ret != DEV_ERR_NONE) {
        disk->dev_status = DEV_IDLE;
        usb_stor_force_reset(usb_id);
        log_error("ret = %d\n", ret);
        return ret;
    }

#if MULTI_DISK
    disk->lun = lun;
#else
    disk->cur_available_lun = 0;
#endif

    int retry = 5;
    while (retry) {
        retry --;
        if (ret == -DEV_ERR_OFFLINE) {
            log_error("disk offline");
            break;
        }
        switch (state) {
        case 0:
            log_info("--- inquery ---");
            ret = usb_stor_inquiry(device);
            if (ret < DEV_ERR_NONE) {
                os_time_dly(retry);
                if (retry == 3) {
                    ret = -DEV_ERR_OFFLINE;
                }
            } else {
                state++;
            }
            break;

        case 1:
            log_info("--- test unit ready ---");
            ret = usb_stor_test_unit_ready(device);
            if (ret < DEV_ERR_NONE) {
                os_time_dly(50);
            } else if (disk->csw.bCSWStatus) {
                os_time_dly(50);
                ret = usb_stor_request_sense(device);
                if ((disk->sense.SenseKey & 0x0f) !=  NO_SENSE) {
                    if (disk->sense.SenseKey == NOT_READY) {
                        os_time_dly(30);
                        if (disk->cur_available_lun++ == lun) {
                            disk->cur_available_lun = 0;
                        }
                    }
                }
            } else {
                state++;
            }
            break;

        case 2:
            log_info("--- read capacity ---");
            ret = usb_stor_read_capacity(device);
            if (ret < DEV_ERR_NONE) {
                os_time_dly(50);
            } else if (disk->csw.bCSWStatus) {
                os_time_dly(50);
                ret = usb_stor_request_sense(device);
                if ((disk->sense.SenseKey & 0x0f) !=  NO_SENSE) {
                    os_time_dly(30);
                }
            } else {
                log_info("udisk mount succ");
                state = 0xff;
            }
            break;

        default:
            retry = 0;
            break;
        }
    }
    state = 0;

    if (ret != DEV_ERR_NONE) {
        disk->dev_status = DEV_IDLE;
        usb_stor_force_reset(usb_id);
        log_error("ret = %d\n", ret);
    } else {
        /* disk->test_unit_ready_tick = sys_timer_add(device,usb_stor_tick_handle,200); */
#if ENABLE_DISK_HOTPLUG
        disk->media_sta_cur = 1;
        disk->media_sta_prev = 1;
#endif
    }
    disk->suspend_cnt = 0;
    return ret;
}

/**
 * @brief usb_stor_ioctrl
 *
 * @param device
 * @param cmd 支持下面的命令
 * 			|命令|功能|
 * 			|---|---|
 * 			|IOCTL_GET_ID|U盘位于总线的地址0表示不在线|
 * 			|IOCTL_GET_BLOCK_SIZE|获取扇区大小|
 * 			|IOCTL_GET_CAPACITY|获取磁盘容量|
 * 			|IOCTL_GET_MAX_LUN|获取最大磁盘数量<最大支持两个磁盘>|
 * 			|IOCTL_GET_CUR_LUN|获取当前的磁盘号|
 * 			|IOCTL_SET_CUR_LUN|设置当前的磁盘号|
 * @param arg
 *
 * @return 0----成功
 * 		  其他值---失败-
 */
static int usb_stor_ioctrl(struct device *device, u32 cmd, u32 arg)
{
    int ret = DEV_ERR_NONE;
    struct usb_host_device *host_dev = device_to_usbdev(device);
    struct mass_storage *disk = host_device2disk(host_dev);
    const usb_dev usb_id = host_device2id(host_dev);
    u32 devnum = host_dev->private_data.devnum;
    u32 curlun = usb_stor_get_curlun(disk);

    if (disk == NULL) {
        return -ENOTTY;
    }

    switch (cmd) {
    case IOCTL_GET_ID:
        *(u32 *)arg = devnum;
        break;

    case IOCTL_GET_BLOCK_SIZE:
        *(u32 *)arg = disk->capacity[curlun].block_size;
        break;

    case IOCTL_GET_BLOCK_NUMBER:
        *(u32 *)arg = disk->capacity[curlun].block_num;
        break;

    case IOCTL_GET_CAPACITY:
        *(u32 *)arg = disk->capacity[curlun].block_num;
        break;

    case IOCTL_GET_MAX_LUN:
#if MULTI_DISK
        *(u32 *)arg = disk->lun;
#else
        *(u32 *)arg = 1;
#endif
        break;

    case IOCTL_GET_CUR_LUN:
        *(u32 *)arg = curlun;
        break;

    case IOCTL_SET_CUR_LUN:
#if MULTI_DISK
        if (arg > disk->lun) {
            return -EINVAL;
        }
        usb_stor_set_curlun(arg);
        ret = usb_stor_read_capacity(device);
        if (ret < 0) {
            log_error("usb disk unit%d is not ready", curlun);
            return -EFAULT;
        } else if (disk->csw.bCSWStatus) {
            usb_stor_request_sense(device);
            if (disk->sense.SenseKey != NO_SENSE) {
                log_error("usb disk unit%d is not ready", curlun);
            }
            return -EFAULT;
        }
        usb_stor_mode_sense6(device);
#else
#endif
        break;

    case IOCTL_CHECK_WRITE_PROTECT:
        if (disk->read_only) {
            *(u32 *)arg = 1;
        } else {
            *(u32 *)arg = 0;
        }
        break;

    case IOCTL_GET_STATUS:
#if ENABLE_DISK_HOTPLUG
        if (disk->media_sta_cur) {
#else
        if (1) {
#endif
            log_info("usb disk unit%d is online", curlun);
            *(u32 *)arg = 1;
        } else {
            log_info("usb disk unit%d is offline", curlun);
            *(u32 *)arg = 0;
        }
        break;

    case IOCTL_SET_FORCE_RESET:
        ret = usb_stor_force_reset(usb_id);
        break;

    default:
        return -ENOTTY;
    }

    return ret;
}

/**
 * @brief usb_msd_parser
 *
 * @param device
 * @param pBuf
 *
 * @return
 */
int usb_msd_parser(struct usb_host_device *host_dev, u8 interface_num, const u8 *pBuf)
{
    struct usb_interface_descriptor *interface = (struct usb_interface_descriptor *)pBuf;
    pBuf += sizeof(struct usb_interface_descriptor);
    int len = 0;
    const usb_dev usb_id = host_device2id(host_dev);

    udisk_device.private_data = host_dev;

    host_dev->interface_info[interface_num] = &udisk_inf;

    for (int endnum = 0; endnum < interface->bNumEndpoints; endnum++) {
        struct usb_endpoint_descriptor *end_desc = (struct usb_endpoint_descriptor *)pBuf;

        if (end_desc->bDescriptorType != USB_DT_ENDPOINT ||
            end_desc->bLength != USB_DT_ENDPOINT_SIZE) {
            log_error("ep bDescriptorType = %d bLength = %d", end_desc->bDescriptorType, end_desc->bLength);
            return -USB_DT_ENDPOINT;
        }

        len += USB_DT_ENDPOINT_SIZE;
        pBuf += USB_DT_ENDPOINT_SIZE;

        if ((end_desc->bmAttributes & USB_ENDPOINT_XFERTYPE_MASK) == USB_ENDPOINT_XFER_BULK) {
            if (end_desc->bEndpointAddress & USB_DIR_IN) {
                udisk_ep.host_epin = usb_get_ep_num(usb_id, USB_DIR_IN, USB_ENDPOINT_XFER_BULK);
                udisk_ep.target_epin = end_desc->bEndpointAddress & 0x0f;
#if HUSB_MODE
                udisk_ep.rxmaxp = end_desc->wMaxPacketSize;
#endif
                log_debug("D(%d)->H(%d)", udisk_ep.target_epin, udisk_ep.host_epin);
            } else {
                udisk_ep.host_epout = usb_get_ep_num(usb_id, USB_DIR_OUT, USB_ENDPOINT_XFER_BULK);
                udisk_ep.target_epout = end_desc->bEndpointAddress & 0x0f;
#if HUSB_MODE
                udisk_ep.txmaxp = end_desc->wMaxPacketSize;
#endif
                log_debug("H(%d)->D(%d)",  udisk_ep.host_epout, udisk_ep.target_epout);
            }
        }
    }
    u8 *ep_buffer = usb_h_get_ep_buffer(usb_id, udisk_ep.host_epin | USB_DIR_IN);
    usb_h_ep_config(usb_id, udisk_ep.host_epin | USB_DIR_IN, USB_ENDPOINT_XFER_BULK, 0, 0, ep_buffer, 64);


    ep_buffer = usb_h_get_ep_buffer(usb_id, udisk_ep.host_epout | USB_DIR_OUT);
    usb_h_ep_config(usb_id, udisk_ep.host_epout | USB_DIR_OUT, USB_ENDPOINT_XFER_BULK, 0, 0, ep_buffer, 64);
    return len;
}
/**
 * @brief usb_stor_open
 *
 * @param node
 * @param device
 * @param arg
 *
 * @return
 */
static int usb_stor_open(const char *name, struct device **device, void *arg)
{
    int ret;
    *device = &udisk_device;
    struct usb_host_device *host_dev = device_to_usbdev(*device);
    struct mass_storage *disk = host_device2disk(host_dev);
    log_debug("=====================usb_stor_open===================== %d", disk->dev_status);
    if (disk->dev_status != DEV_IDLE) {
        if (disk->dev_status == DEV_CLOSE) {
            log_error("%s() fail, disconnect\n", __func__);
            return -DEV_ERR_NOT_MOUNT;
        } else {
            return DEV_ERR_NONE;
        }
    }
    memset(&disk->mutex, 0, sizeof(disk->mutex));
    os_mutex_create(&disk->mutex);
    ret = usb_stor_init(*device);
    if (ret) {
        log_error("usb_stor_init err %d\n", ret);
        return -ENODEV;
    }
    log_debug("device %x", (u32)*device);
    usb_h_mutex_pend(host_dev);
    if (*device) {
        log_error("mass storage dev name %s", name);
        disk->dev_status = DEV_OPEN;
        usb_h_mutex_post(host_dev);
        return DEV_ERR_NONE;
    } else {
        usb_h_mutex_post(host_dev);
        return -ENODEV;
    }
}

/**
 * @brief usb_stor_close
 *
 * @param device
 *
 * @return
 */
static int usb_stor_close(struct device *device)
{
    log_debug("%s---%d\n", __func__, __LINE__);
    struct usb_host_device *host_dev = device_to_usbdev(device);
    struct mass_storage *disk = host_device2disk(host_dev);
    int ret;
    if (disk->dev_status == DEV_IDLE) {
        return 0;
    }
    log_info("=====================usb_stor_close=====================");
    if (disk) {
        log_info("=====================usb_stor_close===================== 333");
        ret = usb_h_mutex_pend(host_dev);

        disk->dev_status = DEV_IDLE;

        if (ret != OS_ERR_NONE) {
            log_error("disk close pend timeout!!!\n");
        } else {
            usb_h_mutex_post(host_dev);
        }
        log_info("=====================usb_stor_close===================== 1222 ");

        os_mutex_del(&disk->mutex, 0);
        memset(&disk->mutex, 0, sizeof(disk->mutex));
    }
    return DEV_ERR_NONE;
}

static bool usb_stor_online(const struct dev_node *node)
{
    struct device *device = &udisk_device;
    struct mass_storage *disk = udisk_inf.dev.disk;;
    if (disk) {
        return true;//disk->media_sta_cur ? true : false;
    }
    return false;
}
/* void usb_stor_test()
{

   struct device *device;// = (struct device *)zalloc(sizeof(struct device));
   usb_stor_open("udisk",&device,NULL);

   usb_stor_write(device,buf,1,15);
   usb_stor_read(device,buf,1,15);
   extern void printf_buf(u8 *buf, u32 len);
   printf_buf(buf,512);
} */
const struct device_operations mass_storage_ops = {
    .init = NULL,
    .online = usb_stor_online,
    .open = usb_stor_open,
    .read = usb_stor_read,
    .write = usb_stor_write,
    .ioctl = usb_stor_ioctrl,
    .close = usb_stor_close,
};

static u8 usb_stor_idle_query(void)
{
    struct device *device = &udisk_device;
    struct mass_storage *disk = host_device2disk(host_dev);
    if (disk) {
        return (disk->dev_status == DEV_IDLE) ? true : false;
    }
    return true;
}

REGISTER_LP_TARGET(usb_stor_lp_target) = {
    .name = "udisk",
    .is_idle = usb_stor_idle_query,
};
#endif
