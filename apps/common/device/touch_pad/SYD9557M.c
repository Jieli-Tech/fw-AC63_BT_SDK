#include "app_config.h"
#include "SYD9557M.h"
#include "asm/gpio.h"
#include "timer.h"
#include "event.h"
#include "asm/power_interface.h"

#if TCFG_TOUCHPAD_ENABLE
#if 0
#define iic_init(iic)                       hw_iic_init(iic)
#define iic_uninit(iic)                     hw_iic_uninit(iic)
#define iic_start(iic)                      hw_iic_start(iic)
#define iic_stop(iic)                       hw_iic_stop(iic)
#define iic_tx_byte(iic, byte)              hw_iic_tx_byte(iic, byte)
#define iic_rx_byte(iic, ack)               hw_iic_rx_byte(iic, ack)
#define iic_read_buf(iic, buf, len)         hw_iic_read_buf(iic, buf, len)
#define iic_write_buf(iic, buf, len)        hw_iic_write_buf(iic, buf, len)
#define iic_suspend(iic)                    hw_iic_suspend(iic)
#define iic_resume(iic)                     hw_iic_resume(iic)
#else
#define iic_init(iic)                       soft_iic_init(iic)
#define iic_uninit(iic)                     soft_iic_uninit(iic)
#define iic_start(iic)                      soft_iic_start(iic)
#define iic_stop(iic)                       soft_iic_stop(iic)
#define iic_tx_byte(iic, byte)              soft_iic_tx_byte(iic, byte)
#define iic_rx_byte(iic, ack)               soft_iic_rx_byte(iic, ack)
#define iic_read_buf(iic, buf, len)         soft_iic_read_buf(iic, buf, len)
#define iic_write_buf(iic, buf, len)        soft_iic_write_buf(iic, buf, len)
#define iic_suspend(iic)                    soft_iic_suspend(iic)
#define iic_resume(iic)                     soft_iic_resume(iic)
#endif

#define INT_IO          IO_PORTB_04
#define SYD9557M_READ_ADDR    0x49
#define SYD9557M_WRITE_ADDR   0x48

static u8 iic_hdl = 0;

u8 syd9557m_get_ndata(u8 r_chip_id, u8 *buf, u8 data_len)
{
    u8 read_len = 0;

    iic_start(iic_hdl);
    if (0 == iic_tx_byte(iic_hdl, SYD9557M_WRITE_ADDR)) {
        log_e("\n gsen iic rd err 0\n");
        read_len = 0;
        goto __gdend;
    }


    delay(50);


    if (0 == iic_tx_byte(iic_hdl, 0x01)) {
        log_e("\n gsen iic rd err 2\n");
        read_len = 0;
        goto __gdend;
    }
    delay(50);

    iic_start(iic_hdl);
    if (0 == iic_tx_byte(iic_hdl, SYD9557M_READ_ADDR)) {
        log_e("\n gsen iic rd err 3\n");
        read_len = 0;
        goto __gdend;
    }

    for (; data_len > 1; data_len--) {
        *buf++ = iic_rx_byte(iic_hdl, 1);
        read_len ++;
    }

    *buf = iic_rx_byte(iic_hdl, 0);
    read_len ++;

__gdend:

    iic_stop(iic_hdl);
    delay(50);

    return read_len;
}

void syd9557_timer_hdl(void *arg)
{
    u8 i = 0;
    u8 data[5] = {0};
    struct sys_event e;
    if (gpio_read(INT_IO) == 0) {
        syd9557m_get_ndata(SYD9557M_READ_ADDR, data, 5);
        //put_buf(data, 5);
        memset(&e, 0x0, sizeof(e));
        e.type = SYS_TOUCHPAD_EVENT;
        if (data[0] != TOUCHPAD_NO_GESTURE && data[0] < TOUCHPAD_MAX_GESTURE) {     //手势事件优先
            e.u.touchpad.gesture_event = data[0];
        }
        e.u.touchpad.x = data[2];
        e.u.touchpad.y = data[3];
        sys_event_notify(&e);
    }
}

void syd9557m_init(u8 iic)
{
    int i = 0;
    iic_hdl = iic;
    iic_init(iic);
    gpio_set_direction(INT_IO, 1);
    gpio_set_die(INT_IO, 1);
    gpio_set_pull_up(INT_IO, 1);
    gpio_set_pull_down(INT_IO, 0);
    sys_s_hi_timer_add(NULL, syd9557_timer_hdl, 2);
    lvd_extern_wakeup_enable();             //要根据封装来选择是否可以使用LVD唤醒， 6531C这个封装LVD是PB4
}
#endif
