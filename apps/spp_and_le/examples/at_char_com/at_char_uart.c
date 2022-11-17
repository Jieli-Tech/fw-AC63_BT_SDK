#include "system/app_core.h"
#include "system/includes.h"
#include "server/server_core.h"
#include "app_config.h"
#include "app_action.h"
#include "os/os_api.h"
#include "btcontroller_config.h"
#include "btctrler/btctrler_task.h"
#include "config/config_transport.h"
#include "btstack/avctp_user.h"
#include "btstack/btstack_task.h"
#include "bt_common.h"
#include "at.h"

/* #include "system/includes.h" */
/* #include "config/config_transport.h" */
/* #include "system/event.h" */
/* #include "asm/uart_dev.h" */
/* #include "app_config.h" */
/* #include "at.h" */
/* #include "bt_common.h" */


#if  CONFIG_APP_AT_CHAR_COM

#define LOG_TAG_CONST       AT_CHAR_COM
/* #define LOG_TAG             "[AT_CHAR_UART]" */
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

struct at_uart {
    u32 baudrate;
    int flowcontrol;
    const char *dev_name;
};

struct uart_hdl {
    struct at_uart config;
    uart_bus_t *udev;
    void *dbuf;
    void *pRxBuffer;
    void *pTxBuffer;
    void (*packet_handler)(const u8 *packet, int size);
    u16 data_length;
    u8  ucRxIndex;
};


typedef struct {
    //head
    u16 preamble;
    u8  type;
    u16 length;
    u8  crc8;
    u16 crc16;
    u8 payload[0];
} _GNU_PACKED_	uart_packet_t;

#define UART_FORMAT_HEAD  sizeof(uart_packet_t)

extern u8 cur_atcom_cid;

extern u16 crc_get_16bit(const void *src, u32 len);

extern void uart1_flow_ctl_init(u8 rts_io, u8 cts_io);
extern void uart1_flow_ctl_rts_suspend(void);
extern void uart1_flow_ctl_rts_resume(void);

static void dummy_handler(const u8 *packet, int size);




#define UART_PREAMBLE         0xBED6

#define UART_CBUF_SIZE          0x400  //串口驱动缓存大小(单次数据超过cbuf, 则驱动会丢失数据)
#define UART_FRAM_SIZE          0x100   //单次数据包最大值(每次cbuf缓存达到fram或串口收到一次数据, 就会起一次中断)

#define UART_RX_SIZE          UART_CBUF_SIZE  //接收串口驱动数据的缓存
#define UART_TX_SIZE          0x400  //


#define UART_BAUD_RATE        115200



#ifdef HAVE_MALLOC
static struct uart_hdl *hdl;
#define __this      (hdl)
#else
static struct uart_hdl hdl;
#define __this      (&hdl)

static u8 pRxBuffer_static[UART_RX_SIZE] __attribute__((aligned(4)));       //rx memory
static u8 pTxBuffer_static[UART_TX_SIZE] __attribute__((aligned(4)));       //tx memory
static u8 devBuffer_static[UART_CBUF_SIZE] __attribute__((aligned(4)));       //dev DMA memory
#endif

const char at_change_channel_cmd[] = "AT>";

void wdt_clear(void);
#define WDT_CLEAR_TIMS 20  //每20个中断清一次狗(待定)
void at_cmd_rx_handler(void)
{
    static u8 clear_dog_flag = 0;
    u8  *p_data = __this->pRxBuffer;


    clear_dog_flag++;
    if (clear_dog_flag > WDT_CLEAR_TIMS) {
        clear_dog_flag = 0;
        wdt_clear();
    }

    //    log_info("uart_len[%d]",__this->udev->get_data_len());

#if FLOW_CONTROL
    if (__this->udev->get_data_len() > UART_CBUF_SIZE) {
        log_error("\n uart overflow, Data loss!!!");
    }
#endif


    if (__this->udev) {
        __this->data_length = __this->udev->read(p_data, UART_RX_SIZE, 0);
    }

    if (__this->data_length > UART_RX_SIZE) {
        log_error("cmd overflow");
        __this->data_length = 0;
        goto __cmd_rx_end;
    }

    log_info("rx[%d]", __this->data_length);
    log_info_hexdump(p_data, __this->data_length);
    if (__this->data_length > 3 && 0 == memcmp(at_change_channel_cmd, p_data, 3)) {
        cur_atcom_cid += 9;  //收到TA>命令后, 强行进入解析
        goto check_at_cmd;
    }

    if (cur_atcom_cid < 9) {
        log_info("rx_data[%d]:", __this->data_length, p_data);
        //        put_buf(p_data,__this->data_length);
        __this->packet_handler(p_data, __this->data_length);
        __this->data_length = 0;

        goto __cmd_rx_end;
    }

check_at_cmd:
    if (__this->data_length > 1) {
        u8 get_cmd = 0;
        if (p_data[__this->data_length - 1] == '\r') {
            p_data[__this->data_length] = 0;
        } else if (p_data[__this->data_length - 2] == '\r') {
            __this->data_length--;
            p_data[__this->data_length] = 0;
        } else {
            goto __cmd_rx_end;
        }

        log_info("rx_cmd[%d]:%s", __this->data_length, p_data);
        __this->packet_handler(p_data, __this->data_length);
        __this->data_length = 0;
    }

__cmd_rx_end:
#if FLOW_CONTROL
    uart1_flow_ctl_rts_resume();
#endif
    ;

}

static void ct_uart_isr_cb(void *ut_bus, u32 status)
{
    struct sys_event e;
    /* printf("{##%d}"); */

    if (status == UT_RX || status == UT_RX_OT) {

#if FLOW_CONTROL
        uart1_flow_ctl_rts_suspend();
#endif
        e.type = SYS_DEVICE_EVENT;
        e.arg  = (void *)DEVICE_EVENT_FROM_AT_UART;
        e.u.dev.event = 0;
        e.u.dev.value = 0;
        sys_event_notify(&e);
    }
}

int ct_uart_init(u32 baud)
{
    const uart_bus_t *uart_bus;
    struct uart_platform_data_t u_arg = {0};
    u_arg.tx_pin = UART_DB_TX_PIN;
    u_arg.rx_pin = UART_DB_RX_PIN;
    u_arg.rx_cbuf = devBuffer_static;
    u_arg.rx_cbuf_size = UART_CBUF_SIZE;  //>=
    u_arg.frame_length = UART_FRAM_SIZE;  //协议数据包
    u_arg.rx_timeout = 6;  //ms,兼容波特率较低
    u_arg.isr_cbfun = ct_uart_isr_cb;
    u_arg.baud = baud;
    u_arg.is_9bit = 0;

    uart_bus = uart_dev_open(&u_arg);

    if (uart_bus != NULL) {
        log_info("uart_dev_open() success\n");
        __this->udev = uart_bus;

#if FLOW_CONTROL
        uart1_flow_ctl_init(UART_DB_RTS_PIN, UART_DB_CTS_PIN);
#endif
//rts_io 1:忙
//
        return 0;
    }
    return -1;
}

void ct_uart_change_baud(u32 baud)
{
    if (__this->udev) {
        __this->udev->set_baud(baud);
    }
}

static void ct_uart_putbyte(char a)
{
    if (__this->udev) {
        __this->udev->putbyte(a);
    }
}

static void ct_uart_write(char *buf, u16 len)
{
    if (__this->udev) {
        __this->udev->write(buf, len);
    }
}

int ct_uart_send_packet(const u8 *packet, int size)
{
    log_info("ct_uart_send_packet:%d", size);
    /* log_info_hexdump(packet, size); */

#if 0
    int i = 0;
    while (size--) {
        ct_uart_putbyte(packet[i++]);
    }
#else
    ct_uart_write((void *)packet, size);
#endif
    /* log_info("end"); */
    return 0;

}

static void dummy_handler(const u8 *packet, int size)
{
    log_error("Dummy");
}

static void clock_critical_enter(void)
{

}
static void clock_critical_exit(void)
{
    if (__this->udev) {
        __this->udev->set_baud(__this->config.baudrate);
    }
}
CLOCK_CRITICAL_HANDLE_REG(ct, clock_critical_enter, clock_critical_exit)

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

    __this->packet_handler = dummy_handler;

    __this->config.baudrate     = UART_BAUD_RATE;
    __this->config.flowcontrol  = 0;
    __this->config.dev_name     = "at-uart";

    __this->udev = 0;
    __this->data_length = 0;
    __this->dbuf = devBuffer_static;

}

static int ct_dev_open(void)
{
    extern u32 uart_baud;
    ct_uart_init(uart_baud);
    return 0;
}

static int ct_dev_close(void)
{
    if (__this->udev) {
        log_info("uart_dev_close\n");
        uart_dev_close(__this->udev);
    }
    return 0;
}

static void ct_dev_register_packet_handler(void (*handler)(const u8 *packet, int size))
{
    __this->packet_handler = handler;
}

void at_uart_init(void *packet_handler)
{
    ct_dev_init();
    ct_dev_open();
    ct_dev_register_packet_handler(packet_handler);
}

extern void power_wakeup_add_io(int io);
void set_at_uart_wakeup(void)
{
    ct_dev_close();
    /* power_wakeup_add_io(UART_DB_RX_PIN);	 */
}

#endif




