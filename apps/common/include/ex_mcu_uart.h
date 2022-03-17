#ifndef __EX_MCU_UART_H__
#define __EX_MCU_UART_H__

#include "app_config.h"
#if TCFG_EX_MCU_ENABLE

#include "ex_mcu.h"
#include "fs.h"
#include "asm/clock.h"
#include "asm/gpio.h"


#define EX_MCU_APP_FILE_PATH 	SDFILE_RES_ROOT_PATH"ex_mcu.bin"


#if TCFG_EX_MCU_OS_ENABLE
//当前最大发送文件分块的长度为128字节每次
#define EX_MCU_APP_BUF_SIZE 	150
#endif

//ex_mcu_uart_handle 操作句柄
struct ex_mcu_uart_handle {
    struct ex_mcu_platform_data *data;
    JL_UART_TypeDef *UART;
    //当前波特率
    u32 baudrate;
    u32 file_cpu_acess_begin;
};

//=======================================================================//
//                       公共操作部分                                    //
//=======================================================================//
u8 ucEx_mcu_reset(void);
u8 ucEx_mcu_exit(void);
//=======================================================================//
//                       串口操作部分                                    //
//=======================================================================//
u8   ucEx_mcu_tx_rx_init(const struct ex_mcu_platform_data *data);
void vEx_mcu_set_baudrate(u32 baudrate);
void vEx_mcu_uart_tx_buf(u8 *uart_dma_buf, u32 dma_buf_len);
#if TCFG_EX_MCU_OS_ENABLE
u8   ucEx_mcu_uart_rx_init(u8 *uart_dma_buf, u32 dma_buf_len/* , u32 timeout*/);
u32  ulEx_mcu_uart_rx_buf(void);
u32 vEx_mcu_uart_tx_buf_client(u8 *uart_dma_buf, u32 dma_buf_len);
u32 ulEx_mcu_uart_rx_buf_client(u8 *uart_dma_buf, u32 dma_buf_len, u32 timeout);
#else
u32  ulEx_mcu_uart_rx_buf(u8 *uart_dma_buf, u32 dma_buf_len, u32 timeout);
#endif
//=======================================================================//
//                        文件系统操作部分                               //
//=======================================================================//
void *vEx_mcu_file_open(const char *file_path);
u32 ulEx_mcu_file_read(void *file, void *buf, u32 len);
u32 ulEx_mcu_file_seek(void *file, u32 offset);

//=======================================================================//
//                        延时操作部分                                   //
//=======================================================================//
void vEx_mcu_udelay(unsigned int t);
void vEx_mcu_os_delay(unsigned int t);

//=======================================================================//
//                        调用API部分                                    //
//=======================================================================//

u8 ucEx_mcu_uart_app_file_download(const char *file_app, u32 retry, u32 retry_timeout, u32 timeout);

//=======================================================================//
//                        参数初始化部分                                 //
//=======================================================================//

//参数通过board_devices_init的ucEx_mcu_init(&ex_mcu_data);传递到底层ex_mcu.c
#define EX_MCU_PLATFORM_DATA_BEGIN(data) \
    struct ex_mcu_platform_data data = {

#if TCFG_EX_MCU_OS_ENABLE
#define EX_MCU_PLATFORM_DATA_END() \
    .hand_baudrate         = 9600, \
    .reset                 = ucEx_mcu_reset, \
    .open                  = vEx_mcu_file_open, \
    .read                  = ulEx_mcu_file_read, \
    .seek                  = ulEx_mcu_file_seek, \
    .tx_rx_init            = ucEx_mcu_tx_rx_init, \
    .tx_buf                = vEx_mcu_uart_tx_buf, \
    .tx_buf_client         = vEx_mcu_uart_tx_buf_client, \
    .rx_init               = ucEx_mcu_uart_rx_init, \
    .rx_buf                = ulEx_mcu_uart_rx_buf, \
    .rx_buf_client         = ulEx_mcu_uart_rx_buf_client, \
    .set_baudrate          = vEx_mcu_set_baudrate, \
    .exit                  = ucEx_mcu_exit, \
    .delay                 = vEx_mcu_os_delay, \
};
#else
#define EX_MCU_PLATFORM_DATA_END() \
    .hand_baudrate         = 9600,\
    .reset                 = ucEx_mcu_reset, \
    .open                  = vEx_mcu_file_open, \
    .read                  = ulEx_mcu_file_read, \
    .seek                  = ulEx_mcu_file_seek, \
    .tx_rx_init            = ucEx_mcu_tx_rx_init, \
    .tx_buf                = vEx_mcu_uart_tx_buf, \
    .rx_buf                = ulEx_mcu_uart_rx_buf, \
    .set_baudrate          = vEx_mcu_set_baudrate, \
    .delay                 = vEx_mcu_udelay, \
};


#endif



#endif  /* #if TCFG_EX_MCU_ENABLE */
#endif /* #ifndef __EX_MCU_UART_H__ */
