#include "gSensor_manage.h"
#include "device/device.h"
#include "app_config.h"
#include "debug.h"
#include "key_event_deal.h"
#include "btstack/avctp_user.h"
#include "app_main.h"
#include "tone_player.h"
#include "user_cfg.h"
#include "system/os/os_api.h"
#include "Motion_api.h"
/* #include "audio_config.h" */
/* #include "app_power_manage.h" */
/* #include "system/timer.h" */
/* #include "event.h" */

#if (TCFG_GSENSOR_ENABLE && (TCFG_SC7A20_EN || TCFG_SC7A20_E_EN || TCFG_MSA310_EN))

spinlock_t iic_lock;

/* #define LOG_TAG             "[GSENSOR]" */
/* #define LOG_ERROR_ENABLE */
/* #define LOG_DEBUG_ENABLE */
/* #define LOG_INFO_ENABLE */
/* #define LOG_DUMP_ENABLE */
/* #define LOG_CLI_ENABLE */
/* #include "debug.h" */

#if GSENSOR_PRINTF_ENABLE
#define log_info(x, ...)  printf("[GSENSOR_MAN]" x "\r\n", ## __VA_ARGS__)
#define log_info_hexdump  put_buf

#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

static const struct gsensor_platform_data *platform_data;
G_SENSOR_INTERFACE *gSensor_hdl = NULL;
G_SENSOR_INFO  __gSensor_info = {.iic_delay = 10};
#define gSensor_info (&__gSensor_info)

extern spinlock_t sensor_iic;
extern u8 sensor_iic_init_status;

#if TCFG_GSENOR_USER_IIC_TYPE
#define iic_init(iic)                       hw_iic_init(iic)
#define iic_uninit(iic)                     hw_iic_uninit(iic)
#define iic_start(iic)                      hw_iic_start(iic)
#define iic_stop(iic)                       hw_iic_stop(iic)
#define iic_tx_byte(iic, byte)              hw_iic_tx_byte(iic, byte)
#define iic_rx_byte(iic, ack)               hw_iic_rx_byte(iic, ack)
#define iic_read_buf(iic, buf, len)         hw_iic_read_buf(iic, buf, len)
#define iic_write_buf(iic, buf, len)        hw_iic_write_buf(iic, buf, len)
#define iic_suspend(iic)                    hw_iic_suspend(iic)
#define iic_resume(iic)                     hw_iic_resume(iic)
#else
#define iic_init(iic)                       soft_iic_init(iic)
#define iic_uninit(iic)                     soft_iic_uninit(iic)
#define iic_start(iic)                      soft_iic_start(iic)
#define iic_stop(iic)                       soft_iic_stop(iic)
#define iic_tx_byte(iic, byte)              soft_iic_tx_byte(iic, byte)
#define iic_rx_byte(iic, ack)               soft_iic_rx_byte(iic, ack)
#define iic_read_buf(iic, buf, len)         soft_iic_read_buf(iic, buf, len)
#define iic_write_buf(iic, buf, len)        soft_iic_write_buf(iic, buf, len)
#define iic_suspend(iic)                    soft_iic_suspend(iic)
#define iic_resume(iic)                     soft_iic_resume(iic)
#endif

//输出三轴数组和数据长度
int get_gSensor_data(short *buf)
{
    axis_info_t accel_data[32];
    int axis_info_len = gSensor_hdl->gravity_sensor_ctl(READ_GSENSOR_DATA, accel_data);
    for (int i = 0; i < axis_info_len; i++) {
        buf[i * 3] = accel_data[i].x;
        buf[i * 3 + 1] = accel_data[i].y;
        buf[i * 3 + 2] = accel_data[i].z;
    }
    return axis_info_len;
}

u8 gravity_sensor_command(u8 w_chip_id, u8 register_address, u8 function_command)
{
    spin_lock(&sensor_iic);
    u8 ret = 1;
    iic_start(gSensor_info->iic_hdl);
    if (0 == iic_tx_byte(gSensor_info->iic_hdl, w_chip_id)) {
        ret = 0;
        log_info("\n gsen iic wr err 0");
        goto __gcend;
    }

    delay(gSensor_info->iic_delay);

    if (0 == iic_tx_byte(gSensor_info->iic_hdl, register_address)) {
        ret = 0;
        log_info("\n gsen iic wr err 1");
        goto __gcend;
    }

    delay(gSensor_info->iic_delay);

    if (0 == iic_tx_byte(gSensor_info->iic_hdl, function_command)) {
        ret = 0;
        log_info("\n gsen iic wr err 2\n");
        goto __gcend;
    }

__gcend:
    iic_stop(gSensor_info->iic_hdl);
    spin_unlock(&sensor_iic);
    return ret;
}

u8 _gravity_sensor_get_ndata(u8 r_chip_id, u8 register_address, u8 *buf, u8 data_len)
{
    spin_lock(&sensor_iic);
    u8 read_len = 0;

    iic_start(gSensor_info->iic_hdl);
    if (0 == iic_tx_byte(gSensor_info->iic_hdl, r_chip_id - 1)) {
        log_info("\n gsen iic rd err 0");
        read_len = 0;
        goto __gdend;
    }


    delay(gSensor_info->iic_delay);
    if (0 == iic_tx_byte(gSensor_info->iic_hdl, register_address)) {
        log_info("\n gsen iic rd err 1");
        read_len = 0;
        goto __gdend;
    }

    iic_start(gSensor_info->iic_hdl);
    if (0 == iic_tx_byte(gSensor_info->iic_hdl, r_chip_id)) {
        log_info("\n gsen iic rd err 2");
        read_len = 0;
        goto __gdend;
    }

    delay(gSensor_info->iic_delay);

    for (; data_len > 1; data_len--) {
        *buf++ = iic_rx_byte(gSensor_info->iic_hdl, 1);
        read_len ++;
    }

    *buf = iic_rx_byte(gSensor_info->iic_hdl, 0);
    read_len ++;

__gdend:

    iic_stop(gSensor_info->iic_hdl);
    delay(gSensor_info->iic_delay);
    spin_unlock(&sensor_iic);

    return read_len;
}

void gsensor_io_ctl(u8 cmd, void *arg)
{
    gSensor_hdl->gravity_sensor_ctl(cmd, arg);
}

int gravity_sensor_init(void *_data)
{
    if (sensor_iic_init_status == 0) {
        spin_lock_init(&sensor_iic);
        sensor_iic_init_status = 1;
    }
    gSensor_info->init_flag  = 0;

    int retval = 0;
    platform_data = (const struct gsensor_platform_data *)_data;
    gSensor_info->iic_hdl = platform_data->iic;
    retval = iic_init(gSensor_info->iic_hdl);

    /* log_info("\n  gravity_sensor_init\n"); */

    if (retval < 0) {
        log_info("\n  open iic for gsensor err");
        return retval;
    } else {
        log_info("iic open succ");
    }

    retval = -EINVAL;
    list_for_each_gsensor(gSensor_hdl) {
        if (!memcmp(gSensor_hdl->logo, platform_data->gSensor_name, strlen(platform_data->gSensor_name))) {
            retval = 0;
            break;
        }
    }

    if (retval < 0) {
        log_info(">>>gSensor_hdl logo err");
        return retval;
    }

    if (gSensor_hdl->gravity_sensor_init()) {
        log_info(">>>>gSensor_Int ERROR");
    } else {
        log_info(">>>>gSensor_Int SUCC");
        gSensor_info->init_flag  = 1;
    }
    return 0;
}


int gsensor_disable(void)
{
    if (gSensor_info->init_flag  == 1) {
        int valid = 0;
        gSensor_hdl->gravity_sensor_ctl(GSENSOR_DISABLE, &valid);
        if (valid == 0) {
            log_info("gsensor_disable_succeed");
            return 0;
        }
    }
    return -1;
}

#endif
