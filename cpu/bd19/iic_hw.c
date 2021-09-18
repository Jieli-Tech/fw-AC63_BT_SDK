#include "asm/iic_hw.h"
#include "system/generic/gpio.h"
#include "system/generic/log.h"
#include "asm/clock.h"
#include "asm/cpu.h"

#if 0
/*
    [[  注意!!!  ]]
    * 适用于带cfg_done的硬件IIC，另一种硬件IIC另作说明
    * 硬件IIC的START / ACK(NACK)必须在发送或接收字节cfg_done前设置，且不能
      接cfg_done单独发送；而STOP则应在发送或接收字节cfg_done后设置，必须接
      cfg_done单独发送
*/

/* const struct hw_iic_config hw_iic_cfg_test[] = { */
/*     //iic0 data */
/*     { */
/*         //         SCL          SDA */
/*         .port = {IO_PORTA_06, IO_PORTA_07}, */
/*         .baudrate = 100000,      //IIC通讯波特率 */
/*         .hdrive = 0,             //是否打开IO口强驱 */
/*         .io_filter = 1,          //是否打开滤波器（去纹波） */
/*         .io_pu = 1,              //是否打开上拉电阻，如果外部电路没有焊接上拉电阻需要置1 */
/*         .role = IIC_MASTER, */
/*     }, */
/* }; */
static JL_IIC_TypeDef *const iic_regs[IIC_HW_NUM] = {
    JL_IIC,
};

#define iic_get_id(iic)         (iic)

#define iic_info_port(iic, x)   (hw_iic_cfg[iic_get_id(iic)].port[x])
#define iic_info_baud(iic)      (hw_iic_cfg[iic_get_id(iic)].baudrate)
#define iic_info_hdrive(iic)    (hw_iic_cfg[iic_get_id(iic)].hdrive)
#define iic_info_io_filt(iic)   (hw_iic_cfg[iic_get_id(iic)].io_filter)
#define iic_info_io_pu(iic)     (hw_iic_cfg[iic_get_id(iic)].io_pu)
#define iic_info_role(iic)      (hw_iic_cfg[iic_get_id(iic)].role)

static inline u32 iic_get_scl(hw_iic_dev iic)
{
    u8 port = iic_info_port(iic, 0);
    return port;
}

static inline u32 iic_get_sda(hw_iic_dev iic)
{
    u8 port = iic_info_port(iic, 1);
    return port;
}

static int iic_port_init(hw_iic_dev iic)
{
    u32 reg;
    int ret = 0;
    u8 id = iic_get_id(iic);
    u32 scl, sda;
    scl = iic_get_scl(iic);
    sda = iic_get_sda(iic);
    if (id == 0) {
        gpio_set_fun_output_port(scl, FO_IIC_SCL, 1, 1);
        gpio_set_fun_output_port(sda, FO_IIC_SDA, 1, 1);
        gpio_set_fun_input_port(scl, PFI_IIC_SCL);
        gpio_set_fun_input_port(sda, PFI_IIC_SDA);
        if (scl >= IO_PORT_DP || sda >= IO_PORT_DP) {
            usb_iomode(1);
        }
        if (iic_info_hdrive(iic)) {
            gpio_set_hd(scl, 1);
            gpio_set_hd(sda, 1);
        } else {
            gpio_set_hd(scl, 0);
            gpio_set_hd(sda, 0);
        }
        if (iic_info_io_pu(iic)) {
            gpio_set_pull_up(scl, 1);
            gpio_set_pull_down(scl, 0);
            gpio_set_pull_up(sda, 1);
            gpio_set_pull_down(sda, 0);
        } else {
            gpio_set_pull_up(scl, 0);
            gpio_set_pull_down(scl, 0);
            gpio_set_pull_up(sda, 0);
            gpio_set_pull_down(sda, 0);
        }
    } else {
        ret = -EINVAL;
    }
    return ret;
}

int hw_iic_set_baud(hw_iic_dev iic, u32 baud)
{
    //f_iic = f_sys / (IIC_BAUD * 2)
    //=> IIC_BAUD = f_sys / (2 * f_iic)
    u32 sysclk;
    u8 id = iic_get_id(iic);

    sysclk = clk_get("lsb");
    /* printf("lsb clk:%d",sysclk); */
    if (sysclk < 2 * baud) {
        return -EINVAL;
    }
    iic_baud_reg(iic_regs[id]) = sysclk / (2 * baud);
    return 0;
}

static void hw_iic_set_die(hw_iic_dev iic, u8 en)
{
    u8 id = iic_get_id(iic);
    u32 scl, sda;
    scl = iic_get_scl(iic);
    sda = iic_get_sda(iic);
    if (id == 0) {
        gpio_set_die(scl, en);
        gpio_set_die(sda, en);
    } else {
        //undefined
    }
}

void hw_iic_suspend(hw_iic_dev iic)
{
    hw_iic_set_die(iic, 0);
}

void hw_iic_resume(hw_iic_dev iic)
{
    hw_iic_set_die(iic, 1);
}

int hw_iic_init(hw_iic_dev iic)
{
    int ret;
    u8 id = iic_get_id(iic);

    if ((ret = iic_port_init(iic))) {
        printf("invalid hardware iic port\n");
        return ret;
    }
    hw_iic_set_die(iic, 1);
    if (iic_info_role(iic) == IIC_MASTER) {
        iic_host_mode(iic_regs[id]);
        if ((ret = hw_iic_set_baud(iic, iic_info_baud(iic)))) {
            printf("iic baudrate is invalid\n");
            return ret ;
        }
    } else {
        iic_slave_mode(iic_regs[id]);
        iic_slave_scl_pull_down_enble(iic_regs[id]); //在收到/在发送数据时把SCL拉低
        iic_dir_in(iic_regs[id]);
    }
    if (iic_info_io_filt(iic)) {
        iic_isel_filter(iic_regs[id]);
    } else {
        iic_isel_direct(iic_regs[id]);
    }

    iic_auto_ack(iic_regs[id]);
    iic_int_disable(iic_regs[id]);
    iic_pnding_clr(iic_regs[id]);
    iic_enable(iic_regs[id]);
    /* iic_disable(iic_regs[id]); */
#if 0
    printf("info->scl = %d\n", iic_get_scl(iic));
    printf("info->sda = %d\n", iic_get_sda(iic));
    printf("info->baudrate = %d\n", iic_info_baud(iic));
    printf("info->hdrive = %d\n", iic_info_hdrive(iic));
    printf("info->io_filter = %d\n", iic_info_io_filt(iic));
    printf("info->io_pu = %d\n", iic_info_io_pu(iic));
    printf("info->role = %d\n", iic_info_role(iic));
    printf("IIC_CON0 0x%04x\n", iic_regs[id]->CON0);
    printf("IIC_CON1 0x%04x\n", iic_regs[id]->CON1);
    printf("IIC_BAUD 0x%02x\n", iic_regs[id]->BAUD);
    //printf("IIC_BUF %02x\n", iic_regs[id]->BUF);
    printf("IOMC1 0x%08x\n", JL_IOMAP->CON1);
#endif
    return 0;
}

void hw_iic_uninit(hw_iic_dev iic)
{
    u8 id = iic_get_id(iic);
    u32 scl, sda;

    scl = iic_get_scl(iic);
    sda = iic_get_sda(iic);
    hw_iic_set_die(iic, 0);
    if (id == 0) {
        gpio_set_hd(scl, 0);
        gpio_set_hd(sda, 0);
        gpio_set_pull_up(scl, 0);
        gpio_set_pull_up(sda, 0);
        if (scl >= IO_PORT_DP || sda >= IO_PORT_DP) {
            usb_iomode(0);
        }
    }
    iic_disable(iic_regs[id]);
}
//必要的延时,两次通信之间至少20us
static u8 start_signal = 0;
static u8 end_signal = 0;
void hw_iic_start(hw_iic_dev iic)
{
    u8 id = iic_get_id(iic);
    /* iic_enable(iic_regs[id]); */
    start_signal |= BIT(7);
    start_signal++;
    if ((start_signal & 0x7f) == 2) {
        start_signal &= ~BIT(7);
    }
}
//必要的延时,两次通信之间至少20us
void hw_iic_stop(hw_iic_dev iic)
{
    u8 id = iic_get_id(iic);
    /* iic_disable(iic_regs[id]); */
    iic_host_send_stop(iic_regs[id]); //stop singal
    start_signal = 0;
    while (!iic_host_is_stop_pending(iic_regs[id]));
}

u8 hw_iic_tx_byte(hw_iic_dev iic, u8 byte)
{
    u8 ack = 0;

    u8 id = iic_get_id(iic);

    iic_dir_out(iic_regs[id]);

    if ((start_signal & 0x7f) >= 2) { //连续两次起始信号则进入发送地址并接收模式
        iic_dir_in(iic_regs[id]);
        iic_host_receive_continue_byte(iic_regs[id]);
        iic_restart(iic_regs[id]);
        /* printf("rst/\n"); */
    }

    iic_buf_reg(iic_regs[id]) = byte;

    if (start_signal & BIT(7)) {
        iic_kick_start(iic_regs[id]); //kick start
        start_signal &= ~BIT(7);
        /* printf("st/\n"); */
    }

    while (!iic_is_pnding(iic_regs[id]));

    if (!iic_host_send_is_ack(iic_regs[id])) {
        ack = 1;
    }
    iic_pnding_clr(iic_regs[id]);


    return ack;
}

u8 hw_iic_rx_byte(hw_iic_dev iic, u8 ack)
{
    u8 data = 0;
    u8 id = iic_get_id(iic);
    iic_dir_in(iic_regs[id]);

    iic_host_receive_continue_byte(iic_regs[id]);

    iic_host_read_kick_start(iic_regs[id]);

    if (ack) {
    } else {
        iic_host_nack_auto_stop(iic_regs[id]); //硬件检测到nack(这个nack是我们硬件发出的), 自动长生stop信号
        iic_host_receive_continue_byte_stop(iic_regs[id]); //最后1byte, 完成后自动nack;
    }
    while (!iic_is_pnding(iic_regs[id]));
    data = iic_buf_reg(iic_regs[id]);
    iic_pnding_clr(iic_regs[id]);

    return data;
}

int hw_iic_write_buf(hw_iic_dev iic, const void *buf, int len)
{
    u8 id = iic_get_id(iic);
    int i = 0;

    if (!buf || !len) {
        return -1;
    }
    iic_dir_out(iic_regs[id]);

    for (i = 0; i < len; i++) {
        iic_buf_reg(iic_regs[id]) = ((u8 *)buf)[i];
        /* if (i == 0) { */
        /*     iic_kick_start(iic_regs[id]); //kick start */
        /* } */
        while (!iic_is_pnding(iic_regs[id]));

        if (iic_host_send_is_ack(iic_regs[id])) {
            iic_pnding_clr(iic_regs[id]);
            break;
        }
        iic_pnding_clr(iic_regs[id]);
    }


    return i;
}

int hw_iic_read_buf(hw_iic_dev iic, void *buf, int len)
{
    u8 id = iic_get_id(iic);
    int i;
    if (!buf || !len) {
        return -1;
    }
    iic_dir_in(iic_regs[id]);
    iic_host_receive_continue_byte(iic_regs[id]);

    for (i = 0; i < len; i++) {
        iic_host_read_kick_start(iic_regs[id]);
        if (i == len - 1) {
            iic_host_nack_auto_stop(iic_regs[id]); //硬件检测到nack(这个nack是我们硬件发出的), 自动长生stop信号
            iic_host_receive_continue_byte_stop(iic_regs[id]); //最后1byte, 完成后自动nack;
        }
        while (!iic_is_pnding(iic_regs[id]));
        iic_pnding_clr(iic_regs[id]);

        ((u8 *)buf)[i] = iic_buf_reg(iic_regs[id]);
    }
    return len;
}






void iic_disable_for_ota()
{
    JL_IIC->CON0 = 0;
}
#endif
