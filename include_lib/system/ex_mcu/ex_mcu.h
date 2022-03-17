#ifndef __EX_MCU_H__
#define __EX_MCU_H__

#include "system/includes.h"
#include "asm/crc16.h"
// #include "debug.h"
#include "boot.h"

//=======================================================================//
//                     扩展注意!!                                        //
//=======================================================================//
#define  TCFG_EX_MCU_UART_ENABLE  1
#define  TCFG_EX_MC_USE_FLASH   1

//是否使用系统延时接口进行os调度和任务切换，1为用os调度 0为裸板工程
#define TCFG_EX_MCU_OS_ENABLE   1
//是否在使用完串口传输代码之后，继续保留此串口传输数据
#define TCFG_EX_MCU_UART_TO_USE 1

struct ex_mcu_platform_data {
    u32   io_port;/*!<收发一线串口对应的引脚*/
    u32   hand_baudrate;/*!<和从机握手时候的波特率，一般固定*/
    u32   tran_baudrate;/*!<发送代码文件时候的波特率，可改,但需要和uboot的波特率一致*/

#if TCFG_EX_MCU_OS_ENABLE
    OS_SEM *sem_tx_rx; /*!<控制os调度后接收数据的同步*/
    u8    *ex_mcu_buf; /*!<串口中断接收数据buf*/
    // u8    *ex_mcu_buf_2; [>!<串口中断接收数据buf<]
    u32(*rx_init)(u8 *buf, u32 len);/*!<串口中断接收数据准备初始化*/
    u32(*rx_buf)(void); /*!<串口中断接收数据*/
    u32(*rx_buf_client)(u8 *uart_dma_buf, u32 dma_buf_len, u32 timeout); /*!<os调度切换为裸板串口接收数据*/
    u32(*tx_buf_client)(u8 *uart_dma_buf, u32 dma_buf_len); /*!<os调度切换为裸板串口发送数据*/
#else
    u32(*rx_buf)(u8 *buf, u32 len, u32 timeout);  /*!<串口轮询接收数据*/
#endif

    u8(*reset)(void);      /*!<复位从机进行推代码*/

    //*********发送接收函数族********//
    u32(*tx_rx_init)(const struct ex_mcu_platform_data *);  /*!<收发一线串口初始化函数*/
    void (*tx_buf)(u8 *buf, u32 len);/*!<串口使用dma发送数据*/
    void (*set_baudrate)(u32 baudrate);/*!<设置波特率*/

    //*********文件操作函数族***//
    void *(*open)(const char *path);  /*!<返回一个文件操作句柄提供给read和seek */
    u32(*read)(void *file, void *buf, u32 len);      /*!<读取bin文件的内容*/
    u32(*seek)(void *file, int offset);      /*!<文件操作指针偏移 */
    u8(*exit)(void);        /*!<退出释放资源操作*/

    //******系统延时******//
    void (*delay)(unsigned int t);/*!<os调度的延时或者裸机定时器延时,单位10ms*/
};

struct ex_mcu_handle {
    struct ex_mcu_platform_data *data;
    void *file_cpu_acess_address;/*!<flash中bin文件的操作指针*/
    u32 file_cpu_acess_begin;/*!<flash中bin文件的起始地址*/
};



u8 ucEx_mcu_init(struct ex_mcu_platform_data *data);
void reset_ex_mcu_register(void (*reset_mcu));
/* ---------------------------------------------------------------------------- */
/**
 * @brief 向其他芯片传输程序文件并启动程序
 *
 * @param baud 进行传输程序数据的时候的波特率设置
 * @param retry 等待从机回应，建立主从通信的重试次数。
 * @param retry_timeout 等待从机回应的最长等待时间，单位10ms。
 * @param timeout 从机接收成功后,主机等待从机回应接收成功的最长等待时间，单位ms，20k代码一般100ms。从机只有完整接收完成程序文件才回应，如果发送的程序越大，从机接收程序越慢，则timeout相应需要设置更长。
 *
 * @return 成功返回1，失败返回0。
 */
/* ---------------------------------------------------------------------------- */
//TODO mode参数待扩展，目前仅支持串口升级
u8 ucEx_mcu_app_file_download(const char *file_app, /*u8 mode,*/ u32 retry, u32 retry_timeout, u32 file_timeout);

#endif /* #ifndef __EX_MCU_H__ */
