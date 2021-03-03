#include "asm/clock.h"
#include "timer.h"
#include "asm/power/p33.h"
/* #include "asm/usb_key.h" */
#include "device/device.h"
#include "asm/power_interface.h"
#include "system/event.h"
#include "asm/efuse.h"
#include "gpio.h"
#include "os/os_api.h"
#include "app_config.h"

#define USB_TIMERx              JL_TIMER4
#define IRQ_TIMEX_IDX           IRQ_TIME4_IDX
#define RX_BIT                  ((JL_USB->CON0>>16)&0x01)

static volatile u8 usbkey_active = 0;
static u8 usbkey_idle_query(void)
{
    return (!usbkey_active);
}

REGISTER_LP_TARGET(usbkey_lp_target) = {
    .name = "usb_key",
    .is_idle = usbkey_idle_query,
};

___interrupt
static void usb_key_isr(void)
{
    static u16 usb_key = 0;
    USB_TIMERx->CON |= BIT(14);
    usb_key = (usb_key << 1) | RX_BIT;
    if (usb_key == 0x16ef) {
        cpu_reset();
    }
}

//用于检测下载工具插入
void usb_key_check_entry(u32 timeout)
{
    u32 usb_con0, usb_io_con0;
    usbkey_active = 1;
    USB_TIMERx->CON = BIT(14);
    usb_con0 = JL_USB->CON0;
    usb_io_con0 = JL_USB_IO->CON0;
    gpio_direction_input(IO_PORT_DP);
    gpio_direction_input(IO_PORT_DM);
    gpio_set_die(IO_PORT_DP, 1);
    gpio_set_die(IO_PORT_DM, 1);
    gpio_set_pull_up(IO_PORT_DP, 0);
    gpio_set_pull_up(IO_PORT_DM, 0);
    gpio_set_pull_down(IO_PORT_DP, 0);
    gpio_set_pull_down(IO_PORT_DM, 0);
    gpio_irflt_in(IO_PORT_DP);
    gpio_irflt_to_timer(4);
    request_irq(IRQ_TIMEX_IDX, 3, usb_key_isr, 0);
    USB_TIMERx->CON = (0b0011 << 4) | BIT(1);
    os_time_dly(timeout);
    USB_TIMERx->CON = 0;
    JL_USB->CON0 = usb_con0;
    JL_USB_IO->CON0 = usb_io_con0;
    usbkey_active = 0;
}


