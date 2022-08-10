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

#if CONFIG_APP_AT_COM

#define LOG_TAG_CONST       AT_COM
#define LOG_TAG             "[AT_UART]"
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

extern u16 crc_get_16bit(const void *src, u32 len);

static void dummy_handler(const u8 *packet, int size);

#define UART_PREAMBLE         0xBED6

#define UART_RX_SIZE          0x100
#define UART_TX_SIZE          0x20
#define UART_DB_SIZE          0x100
#define UART_BAUD_RATE        115200

/* #define UART_DB_TX_PIN        IO_PORTC_02 */
/* #define UART_DB_RX_PIN        IO_PORTC_03 */

/* #define UART_DB_TX_PIN        IO_PORTB_04 */
/* #define UART_DB_RX_PIN        IO_PORTB_05 */


#ifdef HAVE_MALLOC
static struct uart_hdl *hdl;
#define __this      (hdl)
#else
static struct uart_hdl hdl;
#define __this      (&hdl)

static u8 pRxBuffer_static[UART_RX_SIZE] __attribute__((aligned(4)));       //rx memory
static u8 pTxBuffer_static[UART_TX_SIZE] __attribute__((aligned(4)));       //tx memory
static u8 devBuffer_static[UART_DB_SIZE] __attribute__((aligned(4)));       //dev DMA memory
#endif

void at_cmd_rx_handler(void)
{
    u16 crc16;
    struct at_format *p;

    if (__this->udev) {
        __this->data_length += __this->udev->read(&__this->pRxBuffer[__this->data_length], -1, 0);
    }

    log_info("AT CMD RX");
    log_info_hexdump(__this->pRxBuffer, __this->data_length);
    if (__this->data_length > UART_RX_SIZE) {
        log_error("Wired");
    }

    if (*((u8 *)__this->pRxBuffer) != AT_PACKET_TYPE_CMD) {
        log_info("IS NOT TYPE_CMD");
        __this->data_length = 0;
        return;
    }

    if (__this->data_length < AT_FORMAT_HEAD) {
        return;
    }

    p = __this->pRxBuffer;

    if (__this->data_length >= p->length + AT_FORMAT_HEAD) {
        /* log_info("Total length : 0x%x / Rx length : 0x%x", __this->data_length, p->length + AT_FORMAT_HEAD); */
        __this->packet_handler(p, p->length + AT_FORMAT_HEAD);
    }

    __this->data_length = 0;
    /* log_info("RX clear"); */
}


static void ct_uart_isr_cb(void *ut_bus, u32 status)
{
    struct sys_event e;
    /* printf("{##%d}"); */

    if (status == UT_RX || status == UT_RX_OT) {
        e.type = SYS_DEVICE_EVENT;
        e.arg  = (void *)DEVICE_EVENT_FROM_AT_UART;
        e.u.dev.event = 0;
        e.u.dev.value = 0;
        sys_event_notify(&e);
    }
}

static int ct_uart_init()
{
    const uart_bus_t *uart_bus;
    struct uart_platform_data_t u_arg = {0};
    u_arg.tx_pin = UART_DB_TX_PIN;
    u_arg.rx_pin = UART_DB_RX_PIN;
    u_arg.rx_cbuf = devBuffer_static;
    u_arg.rx_cbuf_size = UART_DB_SIZE;
    u_arg.frame_length = UART_DB_SIZE;
    u_arg.rx_timeout = 6;  //ms,兼容波特率较低
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
    log_info_hexdump(packet, size);

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

    /*
       int i = 0;
       uart_packet_t *p = (uart_packet_t *)__this->pTxBuffer;

       p->preamble = UART_PREAMBLE;
       p->type     = 0;
       p->length   = size;
       p->crc8     = crc_get_16bit(p, UART_FORMAT_HEAD - 3) & 0xff;
       p->crc16    = crc_get_16bit(packet, size);

       size += UART_FORMAT_HEAD;
       ASSERT(size <= UART_TX_SIZE, "Fatal Error");

       memcpy(p->payload, packet, size);

    #if 0
    while (size--) {
    ct_uart_putbyte(((char *)p)[i++]);
    }
    #else
    ct_uart_write((void*)p, size);
    #endif

    return 0;
     */

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
    ct_uart_init();
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



