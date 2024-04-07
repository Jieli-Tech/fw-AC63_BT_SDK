#include "app_config.h"
#include "gSensor_manage.h"
#include "SC7A20_TR.h"
#include "includes.h"
#include "math.h"
#include "bank_switch.h"
#include "gpio.h"
#include "Motion_api.h"
/* #pragma bss_seg(".gsport_dev_bss") */
/* #pragma const_seg(".gsport_dev_const") */
/* #pragma code_seg(".gsport_dev_code") */
/*  */

#if (TCFG_GSENSOR_ENABLE && TCFG_SC7A20_EN)

#if GSENSOR_PRINTF_ENABLE
#define LOG(fmt,...)      printf("[SC7A20] " fmt "\n",##__VA_ARGS__)
#define log_info_hexdump  put_buf
#else
#define LOG(fmt,...)
#define log_info_hexdump
#endif

#define SL_SC7A20H_FIFO_CTRL_REG   (unsigned char)0X2E
#define SL_SC7A20H_FIFO_SRC_REG    (unsigned char)0X2F
#define SL_SC7A20H_SPI_OUT_X_L     (unsigned char)0X27
#define SL_SC7A20H_IIC_OUT_X_L     (unsigned char)0XA8

static unsigned char  SL_SPI_IIC_INTERFACE         = 0;

/***使用驱动前请根据实际接线情况配置（7bit）IIC地址******/
/**SC7A20的SDO 脚接地：            0x18****************/
/**SC7A20的SDO 脚接电源：           0x19****************/
#define SC7A20_W_ADDR          (0x19U << 1 | 0x0)
#define SC7A20_R_ADDR          (0x19U << 1 | 0x1)
/*******************************************************/

#define SL_SC7A20H_INIT_REG1_NUM 8
static unsigned char SL_SC7A20H_INIT_REG1[SL_SC7A20H_INIT_REG1_NUM * 3] = {
    0x24, 0x80, 0x00,
    0x2E, 0x00, 0x00,
    0x1f, 0x00, 0x00,
    0x20, 0x37, 0x00, // 25hz 1s获取25组数据
    0x21, 0x70, 0x00,
    0x23, 0x10, 0x00,
    0x24, 0x40, 0x00, // 使能fifo
    0x2E, 0x9F, 0x00, // 将fifo设置为stream模式
};

spinlock_t sensor_iic;
u8 sensor_iic_init_status = 0;


static unsigned char SL_SC7A20H_I2c_Spi_Write(unsigned char sl_spi_iic, unsigned char reg, unsigned char data)
{
    return gravity_sensor_command(SC7A20_W_ADDR, reg, data);
}

static unsigned char SL_SC7A20H_I2c_Spi_Read(unsigned char sl_spi_iic, unsigned char reg, unsigned char len, unsigned char *buf)
{
    return _gravity_sensor_get_ndata(SC7A20_R_ADDR, reg, buf, len);
}

static unsigned char  SL_SC7A20H_Check(void)
{
    unsigned char id1 = 0;
    unsigned char id2 = 0;

    SL_SC7A20H_I2c_Spi_Read(SL_SPI_IIC_INTERFACE, 0x0f, 1, &id1);
    SL_SC7A20H_I2c_Spi_Read(SL_SPI_IIC_INTERFACE, 0x70, 1, &id2);

    LOG("id1=0x%02x id2=0x%02x", id1, id2);
    return id1 == 0x11 && (id2 == 0x28 || id2 == 0x11);
}

static signed char SL_SC7A20H_Driver_Init(unsigned char Sl_spi_iic_init, unsigned char Sl_pull_up_mode)
{
    LOG("SL_SC7A20H_Driver_Init");
    unsigned char i = 0;

    if (Sl_spi_iic_init == 0) {
        SL_SPI_IIC_INTERFACE  = 0;    //spi
    } else {
        SL_SPI_IIC_INTERFACE  = 1;    //iic
    }
    // gsensor采用IO口供电时需要增加上电时间来保证其内部电路稳定
#ifdef GSENSOR_POWER_IO
    os_time_dly(1); // 10ms
#endif

    if (!SL_SC7A20H_Check()) {
        return -1;
    } else {
        LOG("read chip sc7a20's id is ok!");
    }

    SL_SC7A20H_I2c_Spi_Write(SL_SPI_IIC_INTERFACE, 0x57, Sl_pull_up_mode);

    for (i = 0; i < SL_SC7A20H_INIT_REG1_NUM; i++) {
        SL_SC7A20H_I2c_Spi_Write(SL_SPI_IIC_INTERFACE, SL_SC7A20H_INIT_REG1[3 * i], SL_SC7A20H_INIT_REG1[3 * i + 1]);
    }

    for (i = 0; i < SL_SC7A20H_INIT_REG1_NUM; i++) {
        SL_SC7A20H_I2c_Spi_Read(SL_SPI_IIC_INTERFACE, SL_SC7A20H_INIT_REG1[3 * i], 1, &SL_SC7A20H_INIT_REG1[3 * i + 2]);
    }

    for (i = 2; i < SL_SC7A20H_INIT_REG1_NUM; i++) {
        if (SL_SC7A20H_INIT_REG1[3 * i + 1] != SL_SC7A20H_INIT_REG1[3 * i + 2]) {
            break;
        }
    }

    if (i != SL_SC7A20H_INIT_REG1_NUM) {
        if (SL_SPI_IIC_INTERFACE == 0) {
            return -1;    //reg write and read error by SPI
        } else {
            return -2;    //reg write and read error by IIC
        }
    }

    return 0;
}

static unsigned char SL_SC7A20H_Init(void)
{
    return SL_SC7A20H_Driver_Init(1, 0);
}

// 使用FIFO模式读取数据，每次读取缓冲区多个数据（最多32组）
static unsigned char SL_SC7A20H_Read_FIFO_Buf(axis_info_t *accel)
{
    unsigned char  i = 0;
    unsigned char  sc7a20_data[7];
    unsigned char  SL_FIFO_ACCEL_NUM;
    short x, y, z;
    short x_sum = 0, y_sum = 0, z_sum = 0;
    SL_SC7A20H_I2c_Spi_Read(SL_SPI_IIC_INTERFACE, SL_SC7A20H_FIFO_SRC_REG, 1, &SL_FIFO_ACCEL_NUM);
    if (SL_FIFO_ACCEL_NUM & 0x40) {
        SL_FIFO_ACCEL_NUM = 32;
    } else {
        SL_FIFO_ACCEL_NUM = SL_FIFO_ACCEL_NUM & 0x1f;
    }

    LOG("send data len is %d\n", SL_FIFO_ACCEL_NUM);
    if (SL_FIFO_ACCEL_NUM == 0) {
        SL_SC7A20H_Init();
        return 0;
    }
    /* spin_lock() */
    for (i = 0; i < SL_FIFO_ACCEL_NUM; i++) {

        if (SL_SPI_IIC_INTERFACE == 0) {
            SL_SC7A20H_I2c_Spi_Read(SL_SPI_IIC_INTERFACE, SL_SC7A20H_SPI_OUT_X_L, 7, &sc7a20_data[0]);
        } else {
            SL_SC7A20H_I2c_Spi_Read(SL_SPI_IIC_INTERFACE, SL_SC7A20H_IIC_OUT_X_L, 6, &sc7a20_data[1]);
        }

        x = (signed short int)(((unsigned char)sc7a20_data[2] * 256) + (unsigned char)sc7a20_data[1]);
        y = (signed short int)(((unsigned char)sc7a20_data[4] * 256) + (unsigned char)sc7a20_data[3]);
        z = (signed short int)(((unsigned char)sc7a20_data[6] * 256) + (unsigned char)sc7a20_data[5]);

        accel[i].x = x >> 3;
        accel[i].y = y >> 3;
        accel[i].z = z >> 3;

        /* LOG("\nxyz(%d,%d): %d %d %d", SL_FIFO_ACCEL_NUM, i, accel[i].x, accel[i].y,  accel[i].z); */
    }

    return SL_FIFO_ACCEL_NUM;
}


static unsigned char SL_SC7A20H_Disable(void)
{
    return SL_SC7A20H_I2c_Spi_Write(SL_SPI_IIC_INTERFACE, 0x20, 0);
}


s32 SL_SC7A20H_CTL(u8 cmd, void *arg)
{
    char res;
    s32 ret = 0;

    switch (cmd) {
    case GSENSOR_DISABLE:
        res = SL_SC7A20H_Disable();
        memcpy(arg, &ret, 1);
        break;
    case GSENSOR_RESET_INT:
        res = SL_SC7A20H_Init();
        memcpy(arg, &res, 1);
        break;
    case GSENSOR_RESUME_INT:
        break;
    case GSENSOR_INT_DET:
        break;
    case READ_GSENSOR_DATA:
        ret = SL_SC7A20H_Read_FIFO_Buf((axis_info_t *)arg);
        break;
    case SEARCH_SENSOR:
        res = SL_SC7A20H_Check();
        memcpy(arg, &res, 1);
        break;
    default:
        break;
    }
    return ret;
}


REGISTER_GRAVITY_SENSOR(gSensor) = {
    .logo = "sc7a20",
    .gravity_sensor_init  = SL_SC7A20H_Init,
    .gravity_sensor_check = NULL,
    .gravity_sensor_ctl   = SL_SC7A20H_CTL,
};

#endif

