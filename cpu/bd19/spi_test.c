#include "system/includes.h"
/* #include "media/includes.h" */
#include "asm/spi.h"
#include "generic/log.h"

#if 0
/*
    [[[ README ]]]
    1. 本spi测试demo提供了spi.c的API使用例程，测试方式为两个spi的环回测试。
    spi1设置为主机模式，spi2设置为从机模式，spi1发送数据到spi2，然后接收spi2
    原样返回的数据，然后比较发送出去的数据与接收的数据是否一致，一致则说明
    验证通过。
    2. 本demo涉及BYTE收发测试及DMA收发测试，通过宏SPI_TEST_MODE选择。另外
    demo还涉及到spi中断中调用可用于中断的spi API的使用。
    3. spi.c的API不包含CS引脚，CS由API以外控制。
    4. 请在board_xxx.c中定义配置结构体，例如用到spi1，需要定义spi1_p_data，
    否则编译出错。
    5. spi的DMA地址需要4字节对齐。
    6. 虽然spi.c的API带有spi0，但在有挂spi flash的芯片上使用可能会出问题，
    避免使用spi0。

*/

#define SPI1_CS_OUT() \
    do { \
        JL_PORTB->DIR &= ~BIT(4); \
        JL_PORTB->DIE |= BIT(4); \
        JL_PORTB->PU &= ~BIT(4); \
        JL_PORTB->PD &= ~BIT(4); \
    } while(0)
#define SPI1_CS_L()     (JL_PORTB->OUT &= ~BIT(4))
#define SPI1_CS_H()     (JL_PORTB->OUT |= BIT(4))

#define SPI2_CS_IN() \
    do { \
        JL_PORTA->DIR |= BIT(3); \
        JL_PORTA->DIE |= BIT(3); \
        JL_PORTA->PU &= ~BIT(3); \
        JL_PORTA->PD &= ~BIT(3); \
    } while (0)
#define SPI2_READ_CS()     (JL_PORTA->IN & BIT(3))

static u8 slave_dir = 1;
static u8 spi1_send_buf[100] __attribute__((aligned(4)));
static u8 spi1_recv_buf[100] __attribute__((aligned(4)));
static u8 spi2_send_buf[100] __attribute__((aligned(4)));
static u8 spi2_recv_buf[100] __attribute__((aligned(4)));

static spi_dev spi1_hdl = 1;
static spi_dev spi2_hdl = 2;

#define SPI_TEST_BYTE_MODE      0x01
#define SPI_TEST_DMA_MODE       0x02
//测试模式选择
#define SPI_TEST_MODE           SPI_TEST_BYTE_MODE

static void my_put_u8hex(u8 b)
{
    u8 dat;
    dat = b / 16;
    if (dat >= 0 && dat <= 9) {
        putchar('0' + dat);
    } else {
        putchar('A' + dat - 10);
    }
    dat = b % 16;
    if (dat >= 0 && dat <= 9) {
        putchar('0' + dat);
    } else {
        putchar('A' + dat - 10);
    }
    putchar(' ');
}

//中断函数，需以下特殊声明
__attribute__((interrupt("")))
static void spi2_isr()
{
    static int i = 0;
    if (spi_get_pending(spi2_hdl)) {
        spi_clear_pending(spi2_hdl);
        if (SPI2_READ_CS()) {
            return;
        }
#if SPI_TEST_MODE == SPI_TEST_BYTE_MODE
        if (slave_dir == 1) {
            spi2_recv_buf[i] = spi_recv_byte_for_isr(spi2_hdl);
            spi_send_byte_for_isr(spi2_hdl, spi2_recv_buf[i]);
            i >= 100 ? i = 0 : i++;
            slave_dir = 0;
        } else {
            slave_dir = 1;
        }
#elif SPI_TEST_MODE == SPI_TEST_DMA_MODE
        if (slave_dir == 1) {
            spi_dma_set_addr_for_isr(spi2_hdl, spi2_recv_buf, 100, 0);
            slave_dir = 0;
        } else {
            slave_dir = 1;
        }
#endif
    }
}

#if 1  //仅用于spi demo，正式工程请放到board_xxx.c文件中

const struct spi_platform_data spi1_p_data = {
    .port = {
        IO_PORTB_06,    //CLK
        IO_PORTB_07,    //DO
        IO_PORTB_05,    //DI
    },
    .mode = SPI_MODE_BIDIR_1BIT,
    .clk = 1000000,
    .role = SPI_ROLE_MASTER,
};

const struct spi_platform_data spi2_p_data = {
    .port = {
        IO_PORTB_02,    //CLK
        IO_PORTB_03,    //DO
        IO_PORTB_01,    //DI
    },
    .mode = SPI_MODE_BIDIR_1BIT,
    .clk = 1000000,
    .role = SPI_ROLE_SLAVE,
};
#endif


void spi_test_main()
{
    int i;
    int err;

    spi_open(spi1_hdl);
    spi_open(spi2_hdl);
    spi_set_ie(spi2_hdl, 1);
    //配置中断优先级，中断函数
    request_irq(IRQ_SPI2_IDX, 3, spi2_isr, 0);

    SPI1_CS_OUT();
    SPI2_CS_IN();
    SPI1_CS_H();
    for (i = 0; i < 100; i++) {
        spi1_send_buf[i] = i % 26 + 'A';
        spi1_recv_buf[i] = 0;
    }
    puts(">>> spi test start\n");
#if SPI_TEST_MODE == SPI_TEST_BYTE_MODE
    SPI1_CS_L();
    for (i = 0; i < 100; i++) {
        err = spi_send_byte(spi1_hdl, spi1_send_buf[i]);
        if (err) {
            puts("spi1 byte send timeout\n");
            break;
        }
        delay(100);
        spi1_recv_buf[i] = spi_recv_byte(spi1_hdl, &err);
        if (err) {
            puts("spi1 byte recv timeout\n");
            break;
        }
        delay(100);
    }
    SPI1_CS_H();
#elif SPI_TEST_MODE == SPI_TEST_DMA_MODE
    spi_dma_set_addr_for_isr(spi2_hdl, spi2_recv_buf, 100, 1);
    SPI1_CS_L();
    err = spi_dma_send(spi1_hdl, spi1_send_buf, 100);
    if (err < 0) {
        puts("spi1 dma send timeout\n");
        goto __out_dma;
    }
    //delay(100);
    err = spi_dma_recv(spi1_hdl, spi1_recv_buf, 100);
    if (err < 0) {
        puts("spi1 dma recv timeout\n");
        goto __out_dma;
    }
    //delay(100);
__out_dma:
    SPI1_CS_H();
#endif
    puts("<<< spi test end\n");

    puts("\nspi master receivce buffer:\n");
    for (i = 0; i < 100; i++) {
        //my_put_u8hex(spi1_recv_buf[i]);
        putchar(spi1_recv_buf[i]), putchar(0x20);
        if (i % 16 == 15) {
            putchar('\n');
        }
    }
    if (i % 16) {
        putchar('\n');
    }

    if (!memcmp(spi1_send_buf, spi1_recv_buf, 100)) {
        puts("\nspi test pass\n");
    } else {
        puts("\nspi test fail\n");
    }

    spi_close(spi1_hdl);
    spi_close(spi2_hdl);
}

#else

#if 0
/* 配置要使用的 CS 脚 */
/* 为了省IO其实可以不用CS脚也能被ellisys分辨出来，或者在ellisys端将CS脚接地 */
#define SPI1_PORT       JL_PORTA
#define SPI1_PORT_BIT   1
#define SPI1_CS_OUT() \
    do { \
        SPI1_PORT->DIR &= ~BIT(SPI1_PORT_BIT); \
        SPI1_PORT->DIE |=  BIT(SPI1_PORT_BIT); \
        SPI1_PORT->PU  &= ~BIT(SPI1_PORT_BIT); \
        SPI1_PORT->PD  &= ~BIT(SPI1_PORT_BIT); \
    } while(0)
#define SPI1_CS_L()     (SPI1_PORT->DIR &= ~BIT(SPI1_PORT_BIT),SPI1_PORT->OUT &= ~BIT(SPI1_PORT_BIT))
#define SPI1_CS_H()     (SPI1_PORT->DIR &= ~BIT(SPI1_PORT_BIT),SPI1_PORT->OUT |=  BIT(SPI1_PORT_BIT))

typedef const int spi_dev;

extern int spi_open(spi_dev spi);
extern int spi_dma_send(spi_dev spi, const void *buf, u32 len);

const struct spi_platform_data spi2_p_data = {
    .port = {
        IO_PORTA_02,    //CLK
        IO_PORTA_03,    //DO
        IO_PORTA_01,    //DI
    },
    .mode = SPI_MODE_BIDIR_1BIT,
    .clk = 5000000,
    .role = SPI_ROLE_MASTER,
};

const struct spi_platform_data spi1_p_data = {
    .port = {
        /* IO_PORTC_00,    // CLK */
        /* IO_PORTC_01,    // DO，对应ellisys的DI */
        /* IO_PORTC_02,    // DI，对应ellisys的DO */

        IO_PORTA_06,    //
        IO_PORTA_07,    //
        IO_PORTA_08,    //
    },
    .mode = SPI_MODE_BIDIR_1BIT,
    .clk = 5000000,                /* 经测试 ellisys 能分辨到这个速度 */
    .role = SPI_ROLE_MASTER,
};

static spi_dev spi1_hdl = 1;
static u8 spi1_send_buf[512] ALIGNED(4);

void bb_spi_init(void)
{
    r_printf("spi_open:%08x\n", spi_open);
    spi_open(spi1_hdl);
    /* SPI1_CS_OUT(); */
    /* SPI1_CS_H(); */
}

void bb_spi_send(const void const *buf, size_t len)
{
    memcpy(spi1_send_buf, buf, len);
    /* SPI1_CS_L(); */
    spi_dma_send(spi1_hdl, spi1_send_buf, len);
    /* SPI1_CS_H(); */
}

#endif
#endif
