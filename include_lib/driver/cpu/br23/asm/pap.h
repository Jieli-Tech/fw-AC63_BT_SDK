#ifndef _PAP_H_
#define _PAP_H_

#include "system/generic/typedef.h"

//PAP_IO:
//      PAP_RD:PA11
//      PAP_WR:PA12
//      PAPD0 :PA9
//      PAPD1 :PA10
//      PAPD2~PAPD7: PC0~PC5
//PAP_IOMAP:
//      PAP_DEN:IOMAPCON0 bit14(Dx)
//      PAP_REN:IOMAPCON0 bit13(RD)
//      PAP_WEN:IOMAPCON0 bit12(WR)

#define PAP_HW_NUM                  1

#define pap_enable(reg)                   (reg->CON |= BIT(0))
#define pap_disable(reg)                  (reg->CON &= ~BIT(0))
#define pap_dir_in(reg)                   (reg->CON |= BIT(1))
#define pap_dir_out(reg)                  (reg->CON &= ~BIT(1))
#define pap_write_en_polarity_h(reg)      (reg->CON |= BIT(2))//空闲为1,有效为0
#define pap_write_en_polarity_l(reg)      (reg->CON &=~ BIT(2))//空闲为0,有效为1
#define pap_read_en_polarity_h(reg)       (reg->CON |= BIT(3))//空闲为1,有效为0
#define pap_read_en_polarity_l(reg)       (reg->CON &=~ BIT(3))//空闲为0,有效为1
#define pap_data_width_16bit(reg)         (reg->CON |= BIT(4))// N/A
#define pap_data_width_8bit(reg)          (reg->CON &=~ BIT(4))//8bit mode
#define pap_data_big_endian(reg)          (reg->CON |= BIT(5))//数据至端口高位
#define pap_data_little_endian(reg)       (reg->CON &=~ BIT(5))//
#define pap_rw_en_time_width(reg,x)       SFR(reg->CON,8,4, x)//TW:x system clock width(x=0:16 system clock width)
#define pap_data_hold_time_width(reg,x)   SFR(reg->CON,12,2, x)//TH:>=x system clock width
#define pap_data_build_time_width(reg,x)  SFR(reg->CON,14,2, x)//TS:x system clock width
#define pap_data_ext_enable(reg)          (reg->CON |= BIT(16))//数据扩展模式
#define pap_data_ext_disable(reg)         (reg->CON &=~ BIT(16))//
#define pap_data_ext_msb2lsb(reg)         (reg->CON |= BIT(17))//数据扩展模式下,从MSB到LSB逐位检查原始数据
#define pap_data_ext_lsb2msb(reg)         (reg->CON &=~ BIT(17))//LSB-->MSB

#define pap_set_ie(reg)             (reg->CON |= BIT(18))
#define pap_clr_ie(reg)             (reg->CON &= ~BIT(18))
#define pap_pnd(reg)                (reg->CON & BIT(7))
#define pap_pnd_clr(reg)            (reg->CON |= BIT(6))

#define pap_buf_reg(reg)            (reg->BUF)
#define pap_dma_addr_reg(reg)       (reg->ADR)
#define pap_dma_cnt_reg(reg)        (reg->CNT)
#define pap_ext_data0_reg(reg)      (reg->DAT0)
#define pap_ext_data1_reg(reg)      (reg->DAT1)


typedef const u8 hw_pap_dev;


struct hw_pap_config {
    u32 baudrate;
    u8 hdrive;
    u8 io_pu;
};

extern const struct hw_pap_config hw_pap_cfg[];

int hw_pap_init(hw_pap_dev pap);
void hw_pap_uninit(hw_pap_dev pap);
void pap_port_dir_set(u8 dx_dir);//pap d0~d7 输入输出状态
u8 hw_pap_tx_byte(hw_pap_dev pap, u16 byte);//支持扩展模式
u16 hw_pap_rx_byte(hw_pap_dev pap);//不支持扩展模式
int hw_pap_read_buf(hw_pap_dev pap, void *buf, int len);//dma rx
int hw_pap_write_buf(hw_pap_dev pap, const void *buf, int len);//dma tx
void hw_pap_set_data_reg(hw_pap_dev pap, u16 data0, u16 data1);
void hw_pap_set_ie(hw_pap_dev pap, u8 en);
u8 hw_pap_get_pnd(hw_pap_dev pap);
void hw_pap_clr_pnd(hw_pap_dev pap);

#endif

