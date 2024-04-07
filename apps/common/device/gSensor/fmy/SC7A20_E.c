#include "app_config.h"
#include "gSensor_manage.h"
#include "SC7A20_TR.h"
#include "includes.h"
#include "Motion_api.h"


#if (TCFG_GSENSOR_ENABLE && TCFG_SC7A20_E_EN)

#if GSENSOR_PRINTF_ENABLE
#define LOG(fmt,...)      printf("[SC7A20_E] " fmt "\n",##__VA_ARGS__)
#define log_info_hexdump  put_buf
#else
#define LOG(fmt,...)
#define log_info_hexdump
#endif

spinlock_t sensor_iic;
u8 sensor_iic_init_status = 0;

#define ADDR_R   ((0x19U<<1) | 1)
#define ADDR_W   ((0x19U<<1) | 0)

typedef struct {
    u8 reg;
    u8 value;
} regs_opt_t;

static const regs_opt_t config_sc7a20e[] = {
    {0x68, 0xa5}, //soft reset
    {0x57, 0x0C}, //disable sdo_pu/i2c_pu
    {0x0e, 0x00}, // disable register auto and  enable iic
    {0x1F, 0x08}, //low power mode
    {0x20, 0x37}, //odr 1.5hz
    {0x21, 0x00}, //disable high pass
    {0x23, 0x10}, //range:+-4g,disable low pass
    {0x24, 0x40}, //enable fifo
    {0x2E, 0x9F}, //stream mode
};


static unsigned char sl_write_regs(unsigned char register_address, unsigned char value)
{
    return gravity_sensor_command(ADDR_W, register_address, value);
}

static unsigned int sl_read_regs(unsigned char register_address, unsigned char *destination, unsigned char number_of_bytes)
{
    return _gravity_sensor_get_ndata(ADDR_R, register_address, destination, number_of_bytes);
}

static u8 sl_device_id_get(void)
{
    u8 value = 0;
    sl_read_regs(0x0f, &value, 1);
    return value;
}

// extern void mdelay(unsigned int ms);
static u8 sl_config_register(void)
{
    u8 ret = 0;

    for (u8 i = 0; i < sizeof(config_sc7a20e) / sizeof(regs_opt_t); i++) {
        if (sl_write_regs(config_sc7a20e[i].reg, config_sc7a20e[i].value) != 1) {
            ret = -1;
            break;
        }
        delay(1);
    }
    return ret;
}

static u8 sl_fifo_length(void)
{
    u8 value = 0;
    sl_read_regs(0x2F, &value, 1);
    return value & 0x1F;
}

static u8 sl_fifo_get(axis_info_t *acceler_buf)
{
    u8 fifo_data_num  = sl_fifo_length();
    LOG("the length of fifo is %d", fifo_data_num);
    u8 regs_data[100] = {0};

    if (fifo_data_num > 0) {
        sl_read_regs(0x69,  regs_data, fifo_data_num * 3);
    }
    for (int i = 0; i < fifo_data_num; i++) {
        acceler_buf->x = (signed char)regs_data[i * 3 + 2];
        acceler_buf->y = (signed char)regs_data[i * 3 + 0];
        acceler_buf->z = (signed char)regs_data[i * 3 + 1];

        acceler_buf->x = acceler_buf->x << 5;
        acceler_buf->y = acceler_buf->y << 5;
        acceler_buf->z = acceler_buf->z << 5;

        LOG("7a20e fifo[%d],%d,%d,%d", i, acceler_buf->x, acceler_buf->y, acceler_buf->z);
        acceler_buf++;
    }
    return fifo_data_num;
}

static void sl_restart_fifo(void)
{
    /*after the FIFO data is read, need to switch the FIFO mode to BY-PASS mode,
    and then switch back to FIFO mode
    */
    sl_write_regs(0x2e, 0);
    sl_write_regs(0x2e, 0x9F);
}


u8 sl_device_check(void)
{
    u8 id = sl_device_id_get();
    LOG("id=0x%02x", id);
    return id == 0x11;
}

u8 sl_device_init(void)
{
    // gsensor采用IO口供电的时需要增加上电时间来保证其内部电路稳定
#ifdef GSENSOR_POWER_IO
    os_time_dly(1); // 10ms
#endif
    sl_device_check();
    return sl_config_register();
}

u8 sl_device_disable(void)
{
    return sl_write_regs(0x20, 0);
}


s32 sl_device_ctl(u8 cmd, void *arg)
{
    char res;
    s32 ret = 0;

    switch (cmd) {
    case GSENSOR_DISABLE:
        res = sl_device_disable();
        memcpy(arg, &res, 1);
        break;
    case GSENSOR_RESET_INT:
        res = sl_device_init();
        memcpy(arg, &res, 1);
        break;
    case GSENSOR_RESUME_INT:
        break;
    case GSENSOR_INT_DET:
        break;
    case READ_GSENSOR_DATA:
        ret = sl_fifo_get((axis_info_t *)arg);
        sl_restart_fifo();
        break;
    case SEARCH_SENSOR:
        res = sl_device_check();
        memcpy(arg, &res, 1);
        break;
    default:
        break;
    }
    return ret;
}


REGISTER_GRAVITY_SENSOR(gSensor) = {
    .logo = "sc7a20e",
    .gravity_sensor_init  = sl_device_init,
    .gravity_sensor_check = NULL,
    .gravity_sensor_ctl   = sl_device_ctl,
};

#endif
