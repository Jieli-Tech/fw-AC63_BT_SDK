
#include "gSensor_manage.h"
#include "app_config.h"
#include "msa310.h"
#include "msa310_function.h"
#include "Motion_api.h"

#if (TCFG_GSENSOR_ENABLE && TCFG_MSA310_EN)

#if GSENSOR_PRINTF_ENABLE
#define log_info(fmt,...)      printf("[MSA310] " fmt "\n",##__VA_ARGS__)
#define log_info_hexdump       put_buf
#else
#define log_info(fmt,...)
#define log_info_hexdump
#endif

// static u8 msa310_is_idle = 1;
spinlock_t sensor_iic;
u8 sensor_iic_init_status = 0;

#define MSA310_W_ADDR (0x62 << 1 | 0x0)
#define MSA310_R_ADDR (0x62 << 1 | 0x1)

uint8_t msa_id = 0;

/*return value: 0: is ok    -1:read is failed*/
int32_t msa_register_read(uint8_t addr, uint8_t *data)
{
    int32_t res = 0;

    res = _gravity_sensor_get_ndata(MSA310_R_ADDR, addr, data, 1); // return:0:err,  1:ok
    if (res) {
        return 0;
    }
    return -1;
}

/*return value: 0: is ok    -2:write is failed*/
int32_t msa_register_write(uint8_t addr, uint8_t data)
{
    int32_t res = 0;

    res = gravity_sensor_command(MSA310_W_ADDR, addr, data); // return:0:err,  1:ok
    if (res) {
        return 0;
    }
    return -2;
}

// return:0:ok,  -1:err
int32_t msa_register_read_continuously(uint8_t addr, uint8_t count, uint8_t *data)
{
    int32_t res = 0;
    int32_t i = 0;

    for (i = 0; i < count; i++) {
        res = msa_register_read(addr + i, data + i);
        if (res) {
            return res;
        }
    }
    return res; // ok
}

// return:0:ok,  -1/-2:err
int32_t msa_register_mask_write(uint8_t addr, uint8_t mask, uint8_t data)
{
    int32_t res = 0;
    uint8_t tmp_data;

    res = msa_register_read(addr, &tmp_data);
    if (res) {
        return res;
    }

    tmp_data &= ~mask;
    tmp_data |= data & mask;
    res = msa_register_write(addr, tmp_data);

    return res;
}

int32_t msa_read_fifo(axis_info_t *raw_accel)
{
    int i = 0;
    unsigned char data_count = 0;
    uint8_t temp_buf[192]; // 32*6
    msa_register_read(0x0d, &data_count);
    data_count = (data_count & 0x7F);
    if (data_count > 32) {
        data_count = 32;
    }
    /* log_info("data_count = %d \r\n",data_count); */
    _gravity_sensor_get_ndata(MSA310_R_ADDR, 0xff, temp_buf, data_count * 6); // return:0:err,  acc_count*6:ok

    for (u8 i = 0; i < data_count; i++) {
        raw_accel[i].x = ((short)(temp_buf[i * 6 + 1] << 8 | temp_buf[i * 6 + 0])) >> 4;
        raw_accel[i].y = ((short)(temp_buf[i * 6 + 3] << 8 | temp_buf[i * 6 + 2])) >> 4;
        raw_accel[i].z = ((short)(temp_buf[i * 6 + 5] << 8 | temp_buf[i * 6 + 4])) >> 4;
        log_info("%d:   x:%d     y:%d    z:%d\r\n", data_count - i, raw_accel[i].x, raw_accel[i].y, raw_accel[i].z);
    }
    return data_count;
}

extern void msa_param_init(void);
/*return value: 0: is ok    other: is failed*/
uint8_t msa310_init(void)
{
    int32_t res = 0;
    uint8_t data = 0;

    msa_register_read(MSA_REG_WHO_AM_I, &msa_id);
    if (msa_id != 0x13) {
        log_info("read msa310 id error");
        return -1;
    }
    log_info("msa310 id:0x%x", msa_id);

    // reset
    res = msa_register_mask_write(MSA_REG_SOFT_RESET, 0x24, 0x24);

    os_time_dly(10); // 10ms

    res |= msa_register_mask_write(MSA_REG_G_RANGE, 0x03, 0x02);	  // 0:2g, 1:4g, 2:8g, 3:16g
    res |= msa_register_mask_write(MSA_REG_POWERMODE_BW, 0xFF, 0x5e); // low power mode, BW:500hz
    /* res |= msa_register_mask_write(MSA_REG_ODR_AXIS_DISABLE, 0xFF, 0x04);//xyz enable, odr:15.63hz */
    /* res |= msa_register_mask_write(MSA_REG_POWERMODE_BW, 0xFF, 0x50);//low power mode, BW:125hz */
    /* res |= msa_register_mask_write(MSA_REG_POWERMODE_BW, 0xFF, 0x4e);//low power mode, BW:62.5hz */
    res |= msa_register_mask_write(MSA_REG_ODR_AXIS_DISABLE, 0xFF, 0x07); // xyz enable, odr:125hz

    // fifo config
    //  bit67:00: bypass mode, 01: FIFO mode, 10: stream mode, 11: trigger mode, WATERMARK_SAMPLES[5:0]:0x20(32)
    res |= msa_register_mask_write(MSA_REG_FIFO_CTRL, 0xFF, 0xa0);
    res |= msa_register_mask_write(MSA_REG_FIFO_CTRL1, 0x03, 0x00); // fifo enable xyz data

    // int config
    res |= msa_register_mask_write(MSA_REG_INTERRUPT_SETTINGS2, 0x78, 0x60); // bit6:OVERRUN_INT_EN, bit5:WATERMARK_INT_EN, bit4:NEW_DATA_INT_EN, bit3:FREEFALL_INT_EN
    res |= msa_register_mask_write(MSA_REG_INTERRUPT_MAPPING2, 0x07, 0x06);	 // bit2:OVERRUN_INT1, bit1:WATERMARK_INT1, bit0:NEW_DATA_INT1
    res |= msa_register_mask_write(MSA_REG_INT_PIN_CONFIG, 0x03, 0x00);		 // bit1:INT1 OD mode, bit0:INT1 LEVEL
    res |= msa_register_mask_write(MSA_REG_INT_LATCH, 0x8f, 0x00);			 // non-latched

    msa_param_init();

    return res;
}
/*return value: 0: is ok    other: is failed*/
uint8_t msa310_stop(void)
{
    int32_t res = 0;

    res |= msa_register_mask_write(MSA_REG_POWERMODE_BW, 0xFF, 0xDE); // lkk modify 0x5E
    res |= msa_register_mask_write(MSA_REG_FIFO_CTRL, 0xFF, 0x00);

    return res;
}

bool is_msa310_chipid(void)
{
    if (msa_id == 0x13) {
        return true;
    } else {
        return false;
    }
}

bool msa310_check(void)
{
    u8 msa310_d = 0;
    msa_register_read(MSA_REG_WHO_AM_I, &msa310_d);
    if (msa310_d == 0x13) {
        return true;
    } else {
        return false;
    }
}

int msa310_ctl(u8 cmd, void *arg)
{
    int res = -1;
    switch (cmd) {
    case GSENSOR_DISABLE:
        res = msa310_stop();
        memcpy(arg, &res, 1);
        break;
    case GSENSOR_RESET_INT:
        res = msa310_init();
        memcpy(arg, &res, 1);
        break;
    case GSENSOR_RESUME_INT:
        break;
    case GSENSOR_INT_DET:
        /* msa310_int_io_detect(*(u8 *)arg); */
        res = 0;
        break;
    case READ_GSENSOR_DATA:
        res = msa_read_fifo((axis_info_t *)arg);
        break;
    case SEARCH_SENSOR:
        res = msa310_check();
        memcpy(arg, &res, 1);
        break;
    default:

        break;
    }
    return res; //>=0:ok,,<0:err
}

// static u8 msa310_idle_query(void)
// {
//     return msa310_is_idle;
// }

REGISTER_GRAVITY_SENSOR(gSensor) = {
    .logo = "msa310",
    .gravity_sensor_init = msa310_init,
    .gravity_sensor_check = NULL, // msa310_click_status,
    .gravity_sensor_ctl = msa310_ctl,
};

// REGISTER_LP_TARGET(msa310_lp_target) = {
//     .name = "msa310",
//     .is_idle = msa310_idle_query,
// };

#endif
