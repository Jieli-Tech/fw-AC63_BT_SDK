#include "system/includes.h"



static const u8 SPI1_DO[2] = {
    IO_PORTB_01,//'A'
    IO_PORTC_05 //'B'
};
static const u8 SPI2_DO[2] = {
    IO_PORTB_10,//'A'
    IO_PORT_DM  //'B'
};
#define LED_SPI                 JL_SPI1
#define LED_SPI_PORT            'B'
#define LED_SPI_DAT_BAUD        8000000
#define LED_SPI_REST_BAUD       1000000
#define LED_SPI_CLOCK_BASE		clk_get("lsb")

static OS_SEM led_spi_sem;
static u32 spi_do = 0;
static u8 led_spi_busy = 0;
static u8 led_spi_sus = 0;

___interrupt
void led_spi_isr()
{
    LED_SPI->CON &= ~BIT(13);   //关闭中断
    LED_SPI->CON |=  BIT(14);   //清pnding
    os_sem_post(&led_spi_sem);
    led_spi_busy = 0;
}

void led_spi_init(void)
{
    if ((u32)LED_SPI == (u32)JL_SPI1) {
        spi_do = SPI1_DO[LED_SPI_PORT - 'A'];
        SFR(JL_IOMAP->CON1, 4, 1, LED_SPI_PORT - 'A');
        request_irq(IRQ_SPI1_IDX, 0, led_spi_isr, 0);
    } else {
        spi_do = SPI2_DO[LED_SPI_PORT - 'A'];
        SFR(JL_IOMAP->CON1, 16, 1, LED_SPI_PORT - 'A');
        request_irq(IRQ_SPI2_IDX, 0, led_spi_isr, 0);
    }
    gpio_set_die(spi_do, 1);
    gpio_set_direction(spi_do, 0);
    gpio_set_pull_up(spi_do, 0);
    gpio_set_pull_down(spi_do, 0);
    gpio_write(spi_do, 0);

    os_sem_create(&led_spi_sem, 1);

    LED_SPI->CON = 0x4021;
}

void led_spi_rgb_to_24byte(u8 r, u8 g, u8 b, u8 *buf, int idx)
{
    buf = buf + idx * 24;
    u32 dat = ((g << 16) | (r << 8) | b);
    for (u8 i = 0; i < 24; i ++) {
        if (dat & BIT(23 - i)) {
            *(buf + i) = 0x7c;
        } else {
            *(buf + i) = 0x60;
        }
    }
}

void led_spi_rest()
{
    u8 tmp_buf[16] = {0};
    LED_SPI->BAUD = LED_SPI_CLOCK_BASE / LED_SPI_REST_BAUD - 1;
    LED_SPI->CON |= BIT(14);
    LED_SPI->ADR = (u32)tmp_buf;
    LED_SPI->CNT = 16;
    while (!(LED_SPI->CON & BIT(15)));
    LED_SPI->CON |= BIT(14);
}

void led_spi_send_rgbbuf(u8 *rgb_buf, u16 led_num) //rgb_buf的大小 至少要等于 led_num * 24
{
    if (!led_num) {
        return;
    }
    while (led_spi_sus) {
        os_time_dly(1);
    }
    led_spi_busy = 1;
    led_spi_rest();
    LED_SPI->BAUD = LED_SPI_CLOCK_BASE / LED_SPI_DAT_BAUD - 1;
    LED_SPI->CON |= BIT(14);
    LED_SPI->ADR = (u32)rgb_buf;
    LED_SPI->CNT = led_num * 24;
    while (!(LED_SPI->CON & BIT(15)));
    LED_SPI->CON |= BIT(14);
    led_spi_busy = 0;
}

void led_spi_send_rgbbuf_isr(u8 *rgb_buf, u16 led_num) //rgb_buf的大小 至少要等于 led_num * 24
{
    if (!led_num) {
        return;
    }
    while (led_spi_sus) {
        os_time_dly(1);
    }
    led_spi_busy = 1;
    os_sem_pend(&led_spi_sem, 0);
    led_spi_rest();
    LED_SPI->BAUD = LED_SPI_CLOCK_BASE / LED_SPI_DAT_BAUD - 1;
    LED_SPI->CON |= BIT(14);
    LED_SPI->ADR = (u32)rgb_buf;
    LED_SPI->CNT = led_num * 24;
    LED_SPI->CON |= BIT(13);//打开中断
}

u8 led_spi_suspend(void)
{
    if (led_spi_sus) {
        return 1;
    }
    if (led_spi_busy) {
        return 1;
    }
    LED_SPI->CON |=  BIT(14);
    LED_SPI->CON &= ~BIT(0);
    led_spi_sus = 1;
    return 0;
}

u8 led_spi_resume(void)
{
    if (!led_spi_sus) {
        return 0;
    }
    gpio_set_die(spi_do, 1);
    gpio_set_direction(spi_do, 0);
    gpio_set_pull_up(spi_do, 0);
    gpio_set_pull_down(spi_do, 0);
    gpio_write(spi_do, 0);
    LED_SPI->CON = 0x4021;
    led_spi_sus = 0;
    return 0;
}

static u8 spi_dat_buf[24 * 2] __attribute__((aligned(4)));
extern void wdt_clear();
void led_spi_test(void)
{
    printf("******************  led spi test  *******************\n");
    led_spi_init();
    u8 cnt = 0;
    u8 pulse = 0;
    while (1) {
        cnt ++;
        led_spi_rgb_to_24byte(cnt, 255 - cnt, 0, spi_dat_buf, 0);
        led_spi_rgb_to_24byte(0, 0, cnt, spi_dat_buf, 1);
#if 1
        led_spi_send_rgbbuf(spi_dat_buf, 2);        //等待的方式，建议用在发的数据量小的场合
#else
        led_spi_send_rgbbuf_isr(spi_dat_buf, 2);    //中断的方式，建议用在发的数据量大的场合
#endif
        os_time_dly(2);
        wdt_clear();
    }
}

