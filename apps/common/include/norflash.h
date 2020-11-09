#ifndef _NORFLASH_H
#define _NORFLASH_H

#include "device/device.h"
#include "ioctl_cmds.h"
#include "asm/spi.h"
#include "printf.h"
#include "gpio.h"
#include "device_drive.h"
#include "malloc.h"

/*************************************************/
/*
		COMMAND LIST - WinBond FLASH WX25X
*/
/***************************************************************/
#define WINBOND_WRITE_ENABLE		                        0x06
#define WINBOND_READ_SR1			                        0x05
#define WINBOND_READ_SR2			                        0x35
#define WINBOND_WRITE_SR1			                        0x01
#define WINBOND_WRITE_SR2			                        0x31
#define WINBOND_READ_DATA		                            0x03
#define WINBOND_FAST_READ_DATA		                        0x0b
#define WINBOND_FAST_READ_DUAL_OUTPUT                       0x3b
#define WINBOND_PAGE_PROGRAM	                            0x02
#define WINBOND_PAGE_ERASE                                  0x81
#define WINBOND_SECTOR_ERASE		                        0x20
#define WINBOND_BLOCK_ERASE		                            0xD8
#define WINBOND_CHIP_ERASE		                            0xC7
#define WINBOND_JEDEC_ID                                    0x9F

enum {
    FLASH_PAGE_ERASER,
    FLASH_SECTOR_ERASER,
    FLASH_BLOCK_ERASER,
    FLASH_CHIP_ERASER,
};


struct norflash_dev_platform_data {
    int spi_hw_num;         //只支持SPI1或SPI2
    u32 spi_cs_port;        //cs的引脚
    u32 spi_read_width;     //flash读数据的线宽
    const struct spi_platform_data *spi_pdata;
    u32 start_addr;         //分区起始地址
    u32 size;               //分区大小，若只有1个分区，则这个参数可以忽略
};

#define NORFLASH_DEV_PLATFORM_DATA_BEGIN(data) \
	const struct norflash_dev_platform_data data = {

#define NORFLASH_DEV_PLATFORM_DATA_END()  \
};


extern const struct device_operations norflash_dev_ops;
extern const struct device_operations norfs_dev_ops;

#endif

