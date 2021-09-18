#include "system/includes.h"
#include "media/includes.h"
//#include "device/iic.h"
#include "asm/iic_hw.h"
#include "asm/iic_soft.h"

#if 0
/*
    [[[ README ]]]
    1. iic从机需要赋值hw_iic_cfg.role = IIC_SLAVE，若role没有赋值或赋值成
    IIC_MASTER，则默认是主机模式。
    2. iic从机的demo使用中断-任务的交互方式，避免while(!iic_pnd)的阻塞方式造成的
    CPU浪费及响应不及时。由于IIC没有DMA，每传输1 byte就会触发1次中断，所以IIC主机
    必须每传输完成1字节delay一段时间，以至少等待IIC从机响应中断。iic stop产生的
    end中断被用于计算接收字节数及复位状态机，不要去掉end中断的使能及处理。IIC可能
    会收到意外的end中断，所以end中断的处理必须用start-end包住。
    3. demo的接收/发送使用double buffer，避免处理数据过程中被下次接收/发送的数据
    覆盖。
    4. IIC从机收到IIC_S_RADDR后需要尽快准备需要发送的数据，因为主机会很短时间内发
    送下一个IIC字节时钟以请求数据。
    5. IIC主机的TX的STOP到下个RX的START需要delay一段时间，否则会收多收一个不正确
    的字节。
    6. 未尽事项代码中的注释。
*/

#define IIC_S_RADDR                             0x61
#define IIC_S_WADDR                             0x60

#define IIC_S_DEV                                 0
#define IIC_S_TXBUF_SIZE                        128
#define IIC_S_RXBUF_SIZE                        128

enum {
    IIC_S_MSG_TX,
    IIC_S_MSG_RX,
};

struct iic_s_tx_statemachine {
    u8 *buf[2];
    u32 b_size;
    bool toggle;
    u32 cur_cnt;
    u32 tx_cnt;
};

struct iic_s_rx_statemachine {
    u8 *buf[2];
    u32 b_size;
    bool toggle;
    u32 cur_cnt;
    u32 rx_cnt;
};

struct iic_slave {
    enum {IIC_S_TX, IIC_S_RX} dir;
    struct iic_s_tx_statemachine tx;
    struct iic_s_rx_statemachine rx;
    u8 bus_occupy;
};

static struct iic_slave iic_s;
static u8 iic_s_txbuf[2][IIC_S_TXBUF_SIZE];
static u8 iic_s_rxbuf[2][IIC_S_RXBUF_SIZE];

SET_INTERRUPT
static void iic_slave_isr()
{
    u8 is_addr = 0;
    u8 byte;

    if (hw_iic_get_pnd(IIC_S_DEV)) {
        hw_iic_clr_pnd(IIC_S_DEV);
        putchar('a');
        if (iic_s.dir == IIC_S_RX) {
            byte = hw_iic_slave_rx_byte(IIC_S_DEV, &is_addr);  //判断是否为地址
            if (is_addr) {
                iic_s.rx.cur_cnt = 0;
                iic_s.rx.rx_cnt = 0;
                iic_s.tx.cur_cnt = 0;
                iic_s.tx.tx_cnt = 0;
                if (byte == IIC_S_WADDR) {
                    putchar('b');
                    hw_iic_slave_rx_prepare(IIC_S_DEV, 1);  //触发下1 byte接收，并使能recv ack
                    iic_s.bus_occupy = 1;
                } else if (byte == IIC_S_RADDR) {
                    putchar('c');
                    iic_s.dir = IIC_S_TX;
                    os_taskq_post_msg("iic_slave", 1, IIC_S_MSG_TX);
                    iic_s.bus_occupy = 1;  //包住start-stop，避免处理意外的stop处理
                }
            } else {
                putchar('d');
                if (iic_s.rx.cur_cnt < iic_s.rx.b_size) {
                    iic_s.rx.buf[iic_s.rx.toggle][iic_s.rx.cur_cnt++] = byte;
                }
                hw_iic_slave_rx_prepare(IIC_S_DEV, 1);  //触发下1 byte接收，并使能recv ack
            }
        } else {
            putchar('e');
            //如果主机接收ACK或者是最后1 byte主机NACK，发送下1 byte，否则重发当前byte
            if (hw_iic_slave_tx_check_ack(IIC_S_DEV) ||
                (iic_s.tx.tx_cnt - iic_s.tx.cur_cnt == 1)) {
                iic_s.tx.cur_cnt++;
            }
            if (iic_s.tx.cur_cnt < iic_s.tx.tx_cnt) {
                hw_iic_slave_tx_byte(IIC_S_DEV, iic_s.tx.buf[iic_s.tx.toggle][iic_s.tx.cur_cnt]);
            } else {
                //如果主机请求字节数比实际IIC发送字节数多，则发送0xff，防止主机while(!iic_pnd)阻塞卡死
                hw_iic_slave_tx_byte(IIC_S_DEV, 0xff);
            }
        }
    }
    if (hw_iic_get_end_pnd(IIC_S_DEV)) {
        hw_iic_clr_end_pnd(IIC_S_DEV);
        putchar('f');
        //start-stop包住，收到stop时的处理
        if (iic_s.bus_occupy) {
            iic_s.bus_occupy = 0;
            if (iic_s.dir == IIC_S_RX) {
                putchar('g');
                iic_s.rx.toggle = !iic_s.rx.toggle;
                iic_s.rx.rx_cnt = iic_s.rx.cur_cnt;
                os_taskq_post_msg("iic_slave", 2, IIC_S_MSG_RX, iic_s.rx.rx_cnt);
            } else {
                putchar('h');
                iic_s.tx.toggle = !iic_s.tx.toggle;
            }
            iic_s.dir = IIC_S_RX;
            iic_s.rx.cur_cnt = 0;
            iic_s.tx.cur_cnt = 0;
            iic_s.tx.tx_cnt = 0;
            hw_iic_slave_rx_prepare(IIC_S_DEV, 0);  //触发下1 byte接收，并NACK
        }
    }
}

static void iic_slave_task(void *arg)
{
    int res;
    int msg[8];
    u32 rxlen = 0;
    u32 txlen = 0;
    u8 *addr;

    printf("iic_slave_task run\n");
    memset(&iic_s, 0, sizeof(struct iic_slave));
    iic_s.dir = IIC_S_RX;
    iic_s.rx.buf[0] = iic_s_rxbuf[0];
    iic_s.rx.buf[1] = iic_s_rxbuf[1];
    iic_s.rx.b_size = IIC_S_RXBUF_SIZE;
    iic_s.tx.buf[0] = iic_s_txbuf[0];
    iic_s.tx.buf[1] = iic_s_txbuf[1];
    iic_s.tx.b_size = IIC_S_TXBUF_SIZE;

    hw_iic_init(IIC_S_DEV);
    //设置IIC从机地址，并且使能地址包自动ACK
    hw_iic_slave_set_addr(IIC_S_DEV, IIC_S_WADDR, 1);
    //注册中断isr
    request_irq(IRQ_IIC_IDX, 3, iic_slave_isr, 0);
    //使能byte传输中断
    hw_iic_set_ie(IIC_S_DEV, 1);
    //使能stop中断
    hw_iic_set_end_ie(IIC_S_DEV, 1);
    //请求接收，禁止ACK，避免与地址包自动ACK冲突。bit ACK需要在接收数据前设置，关
    //闭ACK并打开地址包自动ACK是为了挂多个IIC从机时，本IIC从机不会ACK其他IIC设备
    //地址，造成多从机失效。地址包自动ACK只有当从机收到设置的地址才会ACK，否则
    //NACK
    hw_iic_slave_rx_prepare(IIC_S_DEV, 0);
    __asm__ volatile("%0 = icfg" : "=r"(res));
    printf("icfg = %08x\n", res);

    while (1) {
        res = os_taskq_pend("taskq", msg, 8);
        switch (res) {
        case OS_TASKQ:
            switch (msg[0]) {
            case Q_MSG:
                switch (msg[1]) {
                case IIC_S_MSG_RX:
                    puts(">>>>>> iic_s rx msg\n");
                    rxlen = msg[2];
                    addr = iic_s.rx.buf[!iic_s.rx.toggle];
                    printf("rx len: %d\n", rxlen);
                    //put_buf(addr, rxlen);
                    for (int i = 0; i < rxlen; i++) {
                        putchar(*(addr + i));
                        putchar(0x20);
                        if (i % 16 == 15) {
                            putchar('\n');
                        }
                    }
                    break;
                case IIC_S_MSG_TX:
                    puts(">>>>>> iic_s tx msg\n");
                    txlen = rxlen;
                    memcpy(iic_s.tx.buf[!iic_s.tx.toggle], iic_s.rx.buf[!iic_s.rx.toggle], txlen);
                    iic_s.tx.tx_cnt = txlen;
                    hw_iic_slave_tx_byte(IIC_S_WADDR, iic_s.tx.buf[!iic_s.tx.toggle][0]);
                    iic_s.tx.toggle = !iic_s.tx.toggle;
                    printf("tx req len: %d\n", txlen);
                    break;
                }
                break;
            }
            break;
        }
    }
}

void iic_demo_slave_main()
{
    printf("%s() %d\n", __func__, __LINE__);
    os_task_create(iic_slave_task, NULL, 30, 1024, 64, "iic_slave");
}




/*~~~~~~~~~~~~~~~~~~~~~~~~~~~~ iic host below ~~~~~~~~~~~~~~~~~~~~~~~~~~~~*/

#define IIC_H_DEV                                 0
#define IIC_H_TXBUF_SIZE                        128
#define IIC_H_RXBUF_SIZE                        128
#define IIC_H_DELAY                             500

#define iic_h_check_ack(ack, pCnt) \
    if (!(ack)) { \
        printf("nack %d\n", __LINE__); \
        if (*(pCnt) > 0 && --(*(pCnt))) { \
            continue; \
        } else { \
            break; \
        } \
    }

static u8 iic_h_txbuf[IIC_H_TXBUF_SIZE];
static u8 iic_h_rxbuf[IIC_H_RXBUF_SIZE];

void iic_demo_host_main()
{
    int i;
    int ret;
    u32 retry;
    u8 byte;

    printf("%s() %d\n", __func__, __LINE__);
    hw_iic_init(IIC_H_DEV);
    for (i = 0; i < IIC_H_TXBUF_SIZE; i++) {
        iic_h_txbuf[i] = 'A' + i % 26;
    }
    for (u8 times = 0; times < 3; times++) {  //测试次数
        retry = 10;
        do {
            hw_iic_start(IIC_H_DEV);
            putchar('a');
            ret = hw_iic_tx_byte(IIC_H_DEV, IIC_S_WADDR);
            putchar('b');
            iic_h_check_ack(ret, &retry);
            delay(IIC_H_DELAY);
            i = 0;
            while (i < IIC_H_TXBUF_SIZE) {
                ret = hw_iic_tx_byte(IIC_H_DEV, iic_h_txbuf[i]);
                putchar('c');
                iic_h_check_ack(ret, &retry);
                delay(IIC_H_DELAY);
                i++;
            }
            break;
        } while (1);
        hw_iic_stop(IIC_H_DEV);
        delay(IIC_H_DELAY);  //stop后需要delay一段时间后再start

        retry = 10;
        do {
            hw_iic_start(IIC_H_DEV);
            putchar('d');
            ret = hw_iic_tx_byte(IIC_H_DEV, IIC_S_RADDR);
            putchar('e');
            iic_h_check_ack(ret, &retry);
            delay(IIC_H_DELAY);
            i = 0;
            while (i < IIC_H_RXBUF_SIZE - 1) {
                iic_h_rxbuf[i] = hw_iic_rx_byte(IIC_H_DEV, 1);
                putchar('f');
                delay(IIC_H_DELAY);
                i++;
            }
            iic_h_rxbuf[i] = hw_iic_rx_byte(IIC_H_DEV, 0);  //IIC主机接收最后1 byte NACK
            putchar('g');
            delay(IIC_H_DELAY);
            break;
        } while (1);
        hw_iic_stop(IIC_H_DEV);  //stop后需要delay一段时间后再start
        delay(IIC_H_DELAY);
        putchar('\n');

        ret = 0;
        for (i = 0; i < IIC_H_RXBUF_SIZE; i++) {
            putchar(iic_h_rxbuf[i]);
            putchar(0x20);
            if (i % 16 == 15) {
                putchar('\n');
            }
            if (iic_h_txbuf[i] != iic_h_rxbuf[i]) {
                ret = 1;
            }
        }
        if (!ret) {
            puts("iic slave test pass\n");
        } else {
            puts("iic slave test fail\n");
        }
    }
}
#endif
