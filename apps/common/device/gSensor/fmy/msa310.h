
#ifndef __MSA310_H__
#define __MSA310_H__

#if TCFG_MSA310_EN
#include "gSensor_manage.h"

//msa310 max fre:400Khz(iic)
#define MSA_REG_SOFT_RESET              0x00
#define MSA_REG_WHO_AM_I                0x01
#define MSA_REG_ACC_X_LSB               0x02
#define MSA_REG_ACC_X_MSB               0x03
#define MSA_REG_ACC_Y_LSB               0x04
#define MSA_REG_ACC_Y_MSB               0x05
#define MSA_REG_ACC_Z_LSB               0x06
#define MSA_REG_ACC_Z_MSB               0x07

#define MSA_REG_MOTION_FLAG							0x09
#define MSA_REG_DATA_INT_FLAG							0x0a
#define MSA_REG_FIFO_SAMPLE_CNT         0x0d
#define MSA_REG_G_RANGE                 0x0f
#define MSA_REG_ODR_AXIS_DISABLE        0x10
#define MSA_REG_POWERMODE_BW            0x11
#define MSA_REG_SWAP_POLARITY           0x12
#define MSA_REG_FIFO_CTRL               0x14
#define MSA_REG_FIFO_CTRL1               0x15
#define MSA_REG_INTERRUPT_SETTINGS1     0x16
#define MSA_REG_INTERRUPT_SETTINGS2     0x17
#define MSA_REG_INTERRUPT_MAPPING1      0x19
#define MSA_REG_INTERRUPT_MAPPING2      0x1a
#define MSA_REG_INT_PIN_CONFIG          0x20
#define MSA_REG_INT_LATCH               0x21
#define MSA_REG_ACTIVE_DURATION         0x27
#define MSA_REG_ACTIVE_THRESHOLD        0x28

typedef unsigned char   uint8_t;
typedef signed char	int8_t;
typedef unsigned short  uint16_t;
typedef signed short	int16_t;

int32_t msa_read_data(int16_t *x, int16_t *y, int16_t *z);
int32_t msa_register_read_continuously(uint8_t addr, uint8_t count, uint8_t *data);
//int32_t i2c_write_byte_data( uint8_t addr, uint8_t data);
bool msa_WriteBytes(uint8_t RegAddr, uint8_t Data);
bool msa_ReadBytes(uint8_t *Data, uint8_t RegAddr);
// int32_t msa_read_fifo(int16_t *xBuf, int16_t *yBuf, int16_t *zBuf, int len);
int32_t msa_read_fifo(axis_info_t *raw_accel);
uint8_t msa_ReadBytes_ACKEND(uint8_t *Data, uint8_t RegAddr);
uint8_t msa310_init(void);

#endif
#endif
