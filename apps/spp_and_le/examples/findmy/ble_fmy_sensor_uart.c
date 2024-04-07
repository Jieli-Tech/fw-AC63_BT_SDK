/*********************************************************************************************
    *   Filename        : ble_fmy_sensor_uart.c

    *   Description     :

    *   Author          : JM

    *   Email           : zh-jieli.com

    *   Last modifiled  : 2024-03-05 15:14

    *   Copyright:(c)JIELI  2011-2026  @ , All Rights Reserved.
*********************************************************************************************/

#include "system/includes.h"
#include "app_config.h"
#include "gSensor/fmy/gsensor_api.h"
#include "ble_fmy_sensor_uart.h"
#include "debug.h"

#if FMY_DEBUG_SENSOR_TO_UART_ENBALE

#if LE_DEBUG_PRINT_EN
#define log_info(x, ...)  printf("[SENSOR_UART]" x "\r\n", ## __VA_ARGS__)
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

#define UART_RX_SIZE 0x40
#define UART_TX_SIZE 0x40
#define UART_DB_SIZE 0x40
#define UART_BAUD_RATE 1000000

static uint8_t pRxBuffer_static[UART_RX_SIZE] __attribute__((aligned(4))); // rx memory
static uint8_t pTxBuffer_static[UART_TX_SIZE] __attribute__((aligned(4))); // tx memory
static uint8_t devBuffer_static[UART_DB_SIZE] __attribute__((aligned(4))); // dev DMA memory

struct sensor_uart {
    uint32_t baudrate;
    int flowcontrol;
    const char *dev_name;
};

struct uart_hdl {
    struct sensor_uart config;
    uart_bus_t *udev;
    void *dbuf;
    void *pRxBuffer;
    void *pTxBuffer;
    uint16_t data_length;
};

static struct uart_hdl hdl;
#define __this (&hdl)

/*************************************************************************************************/
/*!
 *  \brief     发送一个字节
 *
 *  \param      [in] a
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void ct_uart_putbyte(char a)
{
    if (__this->udev) {
        __this->udev->putbyte(a);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      发送包数据长度
 *
 *  \param      [in]buf, len
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void ct_uart_write(char *buf, uint16_t len)
{
    if (__this->udev) {
        __this->udev->write(buf, len);
    }
}

/*************************************************************************************************/
/*!
 *  \brief      发送包数据长度
 *
 *  \param      [in]buf, len
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void uart_data_send(char *buf, uint16_t len)
{
    ct_uart_write(buf, len);
}

/*************************************************************************************************/
/*!
 *  \brief      uart 中断接收回调
 *
 *  \param      [in] ut_bus,status
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void ct_uart_isr_cb(void *ut_bus, uint32_t status)
{
    struct sys_event e;
    /* printf("{##%d}"); */

    if (status == UT_RX || status == UT_RX_OT) {
        e.type = SYS_DEVICE_EVENT;
        e.arg = (void *)DEVICE_EVENT_FROM_AT_UART;
        e.u.dev.event = 0;
        e.u.dev.value = 0;
        sys_event_notify(&e);
    }
}

/*************************************************************************************************/
/*!
 *  \brief     uart 设备打开
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static int ct_dev_open(void)
{
    const uart_bus_t *uart_bus;
    struct uart_platform_data_t u_arg = {0};
    u_arg.tx_pin = UART_DB_TX_PIN;
    u_arg.rx_pin = UART_DB_RX_PIN;
    u_arg.rx_cbuf = devBuffer_static;
    u_arg.rx_cbuf_size = UART_DB_SIZE;
    u_arg.frame_length = UART_DB_SIZE;
    u_arg.rx_timeout = 6; // ms,兼容波特率较低
    u_arg.isr_cbfun = ct_uart_isr_cb;
    u_arg.baud = UART_BAUD_RATE;
    u_arg.is_9bit = 0;

    uart_bus = uart_dev_open(&u_arg);
    if (uart_bus != NULL) {
        log_info("uart_dev_open() success\n");
        __this->udev = uart_bus;
        return 0;
    }
    return -1;
}

/*************************************************************************************************/
/*!
 *  \brief      设备初始化
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
static void ct_dev_init(void)
{

#ifdef HAVE_MALLOC
    __this = malloc(sizeof(struct uart_hdl));
    ASSERT(__this, "Fatal error");

    memset(__this, 0x0, sizeof(struct uart_hdl));

    __this->pRxBuffer = malloc(UART_RX_SIZE);
    ASSERT(__this->pRxBuffer, "Fatal error");

    __this->pTxBuffer = malloc(UART_TX_SIZE);
    ASSERT(__this->pTxBuffer, "Fatal error");
#else
    log_info("Static");
    __this->pRxBuffer = pRxBuffer_static;
    __this->pTxBuffer = pTxBuffer_static;
#endif


    __this->config.baudrate = UART_BAUD_RATE;
    __this->config.flowcontrol = 0;
    __this->config.dev_name = "sensor-uart";

    __this->udev = 0;
    __this->data_length = 0;
    __this->dbuf = devBuffer_static;
    log_info("ct_dev_init!!");
}

/*************************************************************************************************/
/*!
 *  \brief     sensor_uart 初始化
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
int sensor_uart_init(void)
{
    ct_dev_init();
    uint8_t ret = ct_dev_open();
    if (ret == -1) {
        log_info("uart_dev_open() fail\n");
    }
    return ret;
}

// sensor uart发送数据，每次发送一组陀螺仪数据(六个字节), 取多组数据的平均值
// 数据格式：x（hex hex） y（hex hex） z（hex hex）
// example:x=709, y=-659, z=160 ----> C5 02 6D FD A0 00
/* void send_sensor_avg_data(void) */
/* { */
/*     axis_info_t axis_avg = {0}; */
/*     get_sensor_avg(&axis_avg); */
/*     ct_uart_write((char *)&axis_avg, 6); */
/*    //  log_info_hexdump(&axis_avg, 6); */
/* } */

/*************************************************************************************************/
/*!
 *  \brief      sensor_uart发送数据，每次发送多组数据,最多32组
 *
 *  \param      [in]
 *
 *  \return
 *
 *  \note
 */
/*************************************************************************************************/
void sensor_uart_send_data(void)
{
    axis_info_t axis_buffer[32] = {0};
    int data_len = sensor_data_get(axis_buffer);
    ct_uart_write((char *)axis_buffer, data_len * 6);
}

#endif
