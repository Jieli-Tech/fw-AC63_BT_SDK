#ifndef _SL_SC7A20H_DRV_H_
#define _SL_SC7A20H_DRV_H_

#if TCFG_SC7A20_EN

static  unsigned char SL_SC7A20H_I2c_Spi_Write(unsigned char sl_spi_iic, unsigned char reg, unsigned char data);
static  unsigned char SL_SC7A20H_I2c_Spi_Read(unsigned char sl_spi_iic, unsigned char reg, unsigned char len, unsigned char *buf);
/**SL_SC7A20H_I2c_Spi_Write 函数中， sl_spi_iic:0=spi  1=i2c  Reg：寄存器地址   data：寄存器的配置值******************/
/**SL_SC7A20H_I2c_Spi_Write 函数 是一个单次写的函数******************************************/
/***SL_SC7A20H_I2c_Spi_Read 函数中， sl_spi_iic:0=spi  1=i2c Reg 同上，len:读取数据长度，buf:存储数据首地址（指针）**/
/***SL_SC7A20H_I2c_Spi_Read 函数 是可以进行单次读或多次连续读取的函数************************/

/*************驱动初始化函数**************/
static signed char SL_SC7A20H_Driver_Init(unsigned char Sl_spi_iic_init, unsigned char Sl_pull_up_mode);
/***输入参数:1,Sl_spi_iic_init:0-1***2,PULL_UP_MODE:0x00 0x08 0x04 0x0c********/
/****Sl_spi_iic_init=0：SPI MODE, Sl_pull_up_mode config failed****************/
/****Sl_spi_iic_init=1：IIC MODE***********************************************/
/****Sl_pull_up_mode=0x00: SDO  I2C  pull up***********************************/
/****Sl_pull_up_mode=0x08: I2C  pull up and  SDO  open drain*******************/
/****Sl_pull_up_mode=0x04: SDO  pull up and  I2C  open drain*******************/
/****Sl_pull_up_mode=0x0C: SDO  I2C  open drain********************************/

/*************返回数据情况如下**************/
/**return : 0x28 表示CHIP ID 正常***********/
/**return : 0    表示CHIP ID 异常***********/
/**return :-1;   SPI 通信问题***************/
/**return :-2;   IIC 通信问题***************/

#endif

#endif // TCFG_SC7A20_EN


