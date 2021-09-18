#include "system/app_core.h"
#include "system/includes.h"
#include "server/server_core.h"
#include "app_config.h"
#include "app_action.h"
#include "vm.h"
#include "update_loader_download.h"

#if(CONFIG_APP_GAMEBOX)

#include "gamebox.h"

#define LOG_TAG_CONST       GAMEBOX
#define LOG_TAG             "[GAMBOX]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE

#include "debug.h"

extern u8 get_jl_update_flag(void);

static void uart_recv_packet();
static void uart_write(const u8 *buffer, u32 len);
static void uart_comm_init();

#define     UART_RX_EVENT          0x55410001
#define     HEARTBEAT_EVENT        0x55410002
#define     HOT_KEY_EVENT          0x55410003
#define     TOUCH_POINT
const char hid_report_desc[HID_REPORT_SIZE] = {
    0x05, 0x0D,        // Usage Page (Digitizer)
    0x09, 0x04,        // Usage (Touch Screen)
    0xA1, 0x01,        // Collection (Application)
    0x85, TOUCH_SCREEN_ID,        //   Report ID (1)
    0x09, 0x22,        //   Usage (Finger)
    0xA1, 0x02,        //   Collection (Logical)
    0x09, 0x42,        //     Usage (Tip Switch)
    0x09, 0x32,        //     Usage (In Range)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x75, 0x01,        //     Report Size (1)
    0x95, 0x02,        //     Report Count (2)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x02,        //     Report Count (2)
    0x81, 0x01,        //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x51,        //     Usage (Contact Identifier)
    0x75, 0x04,        //     Report Size (4)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x75, 0x10,        //     Report Size (16)
    0x95, 0x01,        //     Report Count (1)
    0x09, 0x30,        //     Usage (X)
    0x26, 0xA8, 0x0C,  //     Logical Maximum (3240)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x31,        //     Usage (Y)
    0x26, 0x80, 0x16,  //     Logical Maximum (5760)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0xA1, 0x02,        //   Collection (Logical)
    0x05, 0x0D,        //     Usage Page (Digitizer)
    0x09, 0x42,        //     Usage (Tip Switch)
    0x09, 0x32,        //     Usage (In Range)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x75, 0x01,        //     Report Size (1)
    0x95, 0x02,        //     Report Count (2)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x02,        //     Report Count (2)
    0x81, 0x01,        //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x51,        //     Usage (Contact Identifier)
    0x75, 0x04,        //     Report Size (4)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x75, 0x10,        //     Report Size (16)
    0x95, 0x01,        //     Report Count (1)
    0x09, 0x30,        //     Usage (X)
    0x26, 0xA8, 0x0C,  //     Logical Maximum (3240)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x31,        //     Usage (Y)
    0x26, 0x80, 0x16,  //     Logical Maximum (5760)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0xC0,              // End Collection

    0x05, 0x0D,        // Usage Page (Digitizer)
    0x09, 0x02,        // Usage (Pen)
    0xA1, 0x01,        // Collection (Application)
    0x85, MOUSE_POINT_ID,        //   Report ID (2)
    0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x05, 0x09,        //     Usage Page (Button)
    0x19, 0x01,        //     Usage Minimum (0x01)
    0x29, 0x03,        //     Usage Maximum (0x03)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x75, 0x01,        //     Report Size (1)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x05,        //     Report Size (5)
    0x95, 0x01,        //     Report Count (1)
    0x81, 0x01,        //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x09, 0x38,        //     Usage (Wheel)
    0x15, 0x81,        //     Logical Minimum (-127)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x03,        //     Report Count (3)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0xC0,              // End Collection

// 167 bytes

};
#define     TASK_NAME   "gamebox"

u8 mouse_data_send ;
u8 touch_data_send;
struct mouse_point_t mouse_data;

static u8 is_mouse_point_mode = FALSE;
static u32 cur_work_mode = BT_MODE;
static u32 need_pre_reset = 0;
u32 get_run_mode()
{
    /* return UT_DEBUG_MODE; */
    /* return BT_MODE; */
    /* return USB_MODE; */
    /* return UART_MODE; */
    return cur_work_mode;
}
void set_run_mode(u32 mode)
{
    cur_work_mode = mode;
}
extern u32 get_jl_rcsp_update_status();
static u32 check_ota_mode()
{
    if (UPDATE_MODULE_IS_SUPPORT(UPDATE_APP_EN)) {
#if RCSP_UPDATE_EN
        if (get_jl_rcsp_update_status()) {
            r_printf("OTA ing");
            set_run_mode(OTA_MODE);
            usb_sie_close_all();//关闭usb
            JL_UART1->CON0 = BIT(13) | BIT(12) | BIT(10);//关闭串口
            return 1;
        }
#endif
    }
    return 0;
}

static void device_insert(const char *dev_name)
{
    u32 id = dev_name[strlen(dev_name) - 1] - '0';
    log_info("device_insert %s id: %x", dev_name, id);
    if (strncmp(dev_name, "hid", 3) == 0) {
        hid_process(id);
        need_pre_reset = 0;
    } else {

#if TCFG_AOA_ENABLE && TCFG_ADB_ENABLE
        if (strncmp(dev_name, "adb", 3) == 0) {
            if (get_run_mode() == BT_MODE) {
                adb_process();
            } else {
                adb_switch_aoa(id);
            }
        } else if (strncmp(dev_name, "aoa", 3) == 0) {
            u32 succ = aoa_process(1, id);
            if (succ) {
                set_phone_connect_status(USB_MODE);
            } else {
                r_printf("aoa error");
            }
        }
#else
        if (adb_process()) {
            usb_h_force_reset(id);
            usb_otg_suspend(id, OTG_UNINSTALL);
            usb_otg_resume(id);
        }
#endif
    }
}

void heartbeat(void *p)
{
    int err = os_taskq_post_msg(TASK_NAME, 1, HEARTBEAT_EVENT);
}

static void gamebox_task(void *arg)
{
    int ret = 0;
    int msg[16];
    u8 heartbeat_packet[1] = {0};

    key_list_init();
    set_run_mode(UT_DEBUG_MODE);
    uart_comm_init();

    u16 timer_id = sys_timer_add(NULL, heartbeat, 500);

    while (1) {
        ret = os_taskq_pend("taskq", msg, ARRAY_SIZE(msg));
        if (ret != OS_TASKQ) {
            continue;
        }
        if (msg[0] != Q_MSG) {
            continue;
        }
        switch (msg[1]) {
        case DEVICE_EVENT_IN:
            if (get_run_mode() != OTA_MODE) {
                device_insert((const char *)msg[2]);
            }
            break;

        case UART_RX_EVENT:
            /* uart_recv_packet(); */
            break;
        case HEARTBEAT_EVENT:
            heartbeat_packet[0]++;
            /* uart_write(heartbeat_packet,1); */
            if (check_ota_mode()) {
#if TCFG_PC_ENABLE
                usb_stop(0);
#endif
                r_printf("%s()", __func__);
                sys_timer_del(timer_id);
            }
            break;
        case HOT_KEY_EVENT:
            //switch IOS or Android
            log_info("hot key %x", msg[2]);
            break;

        default:
            break;
        }
    }
}
static void usb_event_handler(struct sys_event *event, void *priv)
{
    const char *usb_msg;
    usb_dev usb_id;

    switch ((u32)event->arg) {
    case DEVICE_EVENT_FROM_OTG:
        usb_msg = (const char *)event->u.dev.value;
        usb_id = usb_msg[2] - '0';

        log_debug("usb event : %d DEVICE_EVENT_FROM_OTG %s",
                  event->u.dev.event, usb_msg);

        if (usb_msg[0] == 'h') {
            if (event->u.dev.event == DEVICE_EVENT_IN) {
                log_info("usb %c online", usb_msg[2]);
                if (usb_host_mount(usb_id, 3, 20, 250)) {
                    usb_h_force_reset(usb_id);
                    usb_otg_suspend(usb_id, OTG_UNINSTALL);
                    usb_otg_resume(usb_id);
                }
            } else if (event->u.dev.event == DEVICE_EVENT_OUT) {
                log_info("usb %c offline", usb_msg[2]);
                set_phone_connect_status(0);
                usb_host_unmount(usb_id);
            }
        } else if (usb_msg[0] == 's') {
#if TCFG_PC_ENABLE
            if (event->u.dev.event == DEVICE_EVENT_IN) {
                usb_start(usb_id);
            } else {
                usb_stop(usb_id);
            }
#endif
        }
        break;
    case DEVICE_EVENT_FROM_USB_HOST:
        log_debug("host_event %x", event->u.dev.event);
        if ((event->u.dev.event == DEVICE_EVENT_IN) ||
            (event->u.dev.event == DEVICE_EVENT_CHANGE)) {
            int err = os_taskq_post_msg(TASK_NAME, 2, DEVICE_EVENT_IN, event->u.dev.value);
            if (err) {
                r_printf("err %x ", err);
            }
        } else if (event->u.dev.event == DEVICE_EVENT_OUT) {
            log_error("device out %x", event->u.dev.value);
        }
        break;
    }
}

static u16 sys_event_id;
void gamebox_init()
{
    register_sys_event_handler(SYS_DEVICE_EVENT, 0, 2, usb_event_handler);
    int err = task_create(gamebox_task, NULL, TASK_NAME);
}

struct ut_packet {
    char id[4];

    u16 type;//USB_CLASS_HID_KEYBOARD : Keyboard MOUSE_POINT_MODE: mouse

    union {
        struct mouse_data_t m;
        struct keyboard_data_t k;
    } d;

    u16 crc;

} _GNU_PACKED_;

static u8 uart_dma_buffer[256] __attribute__((aligned(4)));
static u8 uart_rxbuf[32];
static u8 uart_txbuf[32] __attribute__((aligned(4)));

static void send2uart(u32 type, const void *p)
{
    struct ut_packet *packet = (struct ut_packet *)uart_txbuf;
    strcpy(packet[0].id, "HID");
    packet[0].type = type;
    memcpy(&(packet[0].d), p, sizeof(packet[0].d));
    packet[0].crc = CRC16(&packet[0], sizeof(packet[0]) - 2);

    packet[1] = packet[0];

    /* printf_buf(packet,2*sizeof(*packet)); */
    uart_write((u8 *)packet, 2 * sizeof(*packet));
}

static u32 mouse_filter(struct mouse_data_t *p)
{
    if (check_ota_mode()) {
        return 0;
    }

    if (p->btn & BIT(2)) {     //middle button
        is_mouse_point_mode = !is_mouse_point_mode;
        point_list_empty();
        p->x = 1;
        p->y = 1;
        p->btn = 0;
        return 1;
    } else {
        return 1; // as mouse event
    }
}
void mouse_route(const struct mouse_data_t *p)
{
    if (get_run_mode() != UART_MODE) {
        if (mouse_filter((void *)p) == 0) {
            return;
        }
    }
    /* log_info("btn: %x x-y %d %d wheel %d ac_pan %d",  */
    /*         p->btn, p->x, p->y, p->wheel, p->ac_pan); */
    switch (get_run_mode()) {
    case UART_MODE ://在USB中断函数调用
        send2uart(MOUSE_POINT_MODE, p);
        break;
    case BT_MODE ://在uart中断 或者usb中断函数调用
    case USB_MODE://在串口中断调用
        if (is_mouse_point_mode) {
            send2phone(MOUSE_POINT_MODE, p);
        } else {
            mouse_mapping(p);
            send2phone(TOUCH_SCREEN_MODE, p);
        }
        break;
    case MAPPING_MODE:
        send2phone(MOUSE_POINT_MODE + 1, p);
        break;
    default :
        log_info("btn: %x x-y %d %d wheel %d ac_pan %d",
                 p->btn, p->x, p->y, p->wheel, p->ac_pan);
        break;
    }
}

u32 keyboard_filter(struct keyboard_data_t *k)
{
    if (check_ota_mode()) {
        return 0;
    }
    if ((k->fun_key & _KEY_MOD_LMETA) &&
        (k->Keypad[0] == _KEY_F1)) {
        os_taskq_post_msg(TASK_NAME, 2, HOT_KEY_EVENT, _KEY_F1);
        return 0; //hook this msg
    }
    return 2;
}
void keyboard_route(const u8 *p)
{
    /* log_info("keyboard:"); */
    /* printf_buf(p, 8); */
    if (keyboard_filter((struct keyboard_data_t *)p) == 0) {
        return;
    }

    switch (get_run_mode()) {
    case UART_MODE ://在USB中断函数调用
        send2uart(KEYBOARD_MODE, p);
        break;
    case BT_MODE ://在uart接收事件 或者usb中断函数调用
    case USB_MODE://在串口事件调用
        key_mapping((const void *)p);
        send2phone(TOUCH_SCREEN_MODE, p);
        break;
    case MAPPING_MODE:
        send2phone(KEYBOARD_MODE, p);
        break;
    default :
        printf_buf((u8 *)p, 8);
        break;
    }
}

static volatile u32 tx_idle;
static KFIFO uart_fifo;

SET_INTERRUPT
static void uart_isr(void)
{
    u32 rx_len;
    if ((JL_UART1->CON0 & BIT(2)) && (JL_UART1->CON0 & BIT(15))) {
        JL_UART1->CON0 |= BIT(13);
        tx_idle = 1;
    }
    if ((JL_UART1->CON0 & BIT(3)) && (JL_UART1->CON0 & BIT(14))) {
        JL_UART1->CON0 |= BIT(12);           //清RX PND
    }

    if ((JL_UART1->CON0 & BIT(5)) && (JL_UART1->CON0 & BIT(11))) {
        //OTCNT PND
        JL_UART1->CON0 |= BIT(7);                     //DMA模式
        JL_UART1->CON0 |= BIT(10);                    //清OTCNT PND
        JL_UART1->CON0 |= BIT(12);                    //清RX PND(这里的顺序不能改变，这里要清一次)
        rx_len = JL_UART1->HRXCNT;             //读当前串口接收数据的个数

        if (rx_len) {
            uart_fifo.buf_in += rx_len;
            uart_recv_packet();
        }
    }
}

static void uart_comm_init()
{
    request_irq(IRQ_UART1_IDX, 2, uart_isr, 0);//优先级不能比usb低

    uart_fifo.buffer = uart_dma_buffer;
    uart_fifo.buf_size = sizeof(uart_dma_buffer);
    uart_fifo.buf_in = uart_fifo.buf_out = 0;

    gpio_set_uart1(-1);

    gpio_output_channle(IO_PORTB_08, CH1_UT1_TX);
    gpio_uart_rx_input(IO_PORTB_09, 1, 1);
    gpio_set_pull_up(IO_PORTB_08, 1);

    JL_UART1->CON0 = BIT(13) | BIT(12) | BIT(10);

    JL_UART1->RXSADR = (u32)uart_fifo.buffer;
    JL_UART1->RXEADR = (u32)(uart_fifo.buffer + uart_fifo.buf_size);
    JL_UART1->RXCNT = uart_fifo.buf_size / 2;
    JL_UART1->OTCNT = 100 * (clk_get("lsb") / 1000000);

    JL_UART1->BAUD = (clk_get("uart") / 2000000) / 4 - 1;

    JL_UART1->CON0 |= BIT(6) | BIT(5) | BIT(3);
    JL_UART1->CON0 |= BIT(0);

    memset(uart_txbuf, cur_work_mode, 32);
}

static u32 kfifo_get(KFIFO *kfifo, u8 *buffer, u32 len)
{
    unsigned int i;
    len = MIN(len, kfifo->buf_in - kfifo->buf_out);

    i = MIN(len, kfifo->buf_size - (kfifo->buf_out & (kfifo->buf_size - 1)));

    memcpy(buffer, kfifo->buffer + (kfifo->buf_out & (kfifo->buf_size - 1)), i);

    memcpy(buffer + i, kfifo->buffer, len - i);

    kfifo->buf_out += len;
    return len;
}

static u32 uart_read(u8 *buffer, u32 len)
{
    return kfifo_get(&uart_fifo, buffer, len);
}

static void uart_write(const u8 *buffer, u32 len)
{
    tx_idle = 0;
    JL_UART1->CON0 |= BIT(13);
    JL_UART1->CON0 |= BIT(2);
    JL_UART1->TXADR = (u32)buffer;
    JL_UART1->TXCNT = len;
    while (!tx_idle) {
        asm("idle");
    }

}

static void uart_recv_packet()
{
    struct ut_packet *p;
    memset(uart_rxbuf, 0, sizeof(uart_rxbuf));

    u8 *u = uart_rxbuf;
    u8 len;

__r0:
    len = uart_read(u, sizeof(uart_rxbuf));
    if (len < sizeof(*p)) {
        return;
    }
__r1:
    p = (struct ut_packet *)strstr((const char *)u, "HID");
    if (p) {
        if (p->crc == CRC16(p, sizeof(*p) - 2)) {
            if (p->type == MOUSE_POINT_MODE) {
                mouse_route(&p->d.m);
            } else if (p->type == KEYBOARD_MODE) {
                keyboard_route((const u8 *)&p->d.m);
            }
            goto __r0;
            /* return; */
        }
    } else {
        p = (struct ut_packet *)strstr((const char *)u, "UPDATE");//reset 进入uboot模式
    }

    if (u < &uart_rxbuf[sizeof(*p)]) {
        u += sizeof(*p);
        goto __r1;
    }
    goto __r0;
}

#endif
