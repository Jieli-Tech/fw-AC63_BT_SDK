#include "includes.h"
#include "gsensor_api.h"
#include "Motion_api.h"

#if (TCFG_GSENSOR_ENABLE && (TCFG_SC7A20_EN || TCFG_SC7A20_E_EN || TCFG_MSA310_EN))

#if GSENSOR_PRINTF_ENABLE
#define log_info(x, ...)  printf("[GSENSOR_API]" x "\r\n", ## __VA_ARGS__)
#define log_info_hexdump  put_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

#ifdef GSENSOR_POWER_IO
#define  PORT_DIR_OUPUT             0
#define  PORT_DIR_INPUT             1
#define  PORT_VALUE_HIGH            1
#define  PORT_VALUE_LOW             0
#define PORT_IO_INIT(PORT_IO,DIR)       {gpio_set_die(PORT_IO,1);\
                                         gpio_set_pull_down(PORT_IO,0);\
                                         gpio_set_pull_up(PORT_IO,0);\
                                         gpio_set_direction(PORT_IO,DIR);}

#define PORT_IO_OUPUT(PORT_IO,VALUE)     gpio_write(PORT_IO,VALUE);
#endif

#if TCFG_SC7A20_EN
#define GSENSOR_NAME "sc7a20"
#elif TCFG_SC7A20_E_EN
#define GSENSOR_NAME "sc7a20e"
#elif TCFG_MSA310_EN
#define GSENSOR_NAME "msa310"
#endif

GSENSOR_PLATFORM_DATA_BEGIN(gSensor_data)
.iic = 0,
 .gSensor_int_io = -1,
  .gSensor_name = GSENSOR_NAME,
   GSENSOR_PLATFORM_DATA_END();


// motion detection 算法配置
static char *workbuf = NULL;
static const short fs = 25;
static const float thread = 2.0f;



// 获得陀螺仪数据数组
int sensor_data_get(short *buf)
{
    return get_gSensor_data(buf);
}

bool sensor_motion_detection(void)
{
    log_info("run motion detection task!");
    axis_info_t axis_buffer[32] = {0};
    int data_len = sensor_data_get(axis_buffer);
    char flag = run_MotionDetection(workbuf, data_len, axis_buffer);
    if (flag) {
        log_info("is moving!!");
    } else {
        log_info("is static!!");
    }
    return flag;
}

// enable fmy gsensor
int sensor_init(void)
{
    log_info("%s, %s, %d", __FILE__, __FUNCTION__, __LINE__);
#ifdef GSENSOR_POWER_IO
    PORT_IO_INIT(GSENSOR_POWER_IO, PORT_DIR_OUPUT);
    PORT_IO_OUPUT(GSENSOR_POWER_IO, PORT_VALUE_HIGH);
#endif
    int ret = gravity_sensor_init((struct gsensor_platform_data *)&gSensor_data);

    int buff_size = get_DetectionBuf(fs);
    workbuf = (char *)malloc(buff_size);

    if (!workbuf) {
        log_info("fmy gsensor workbuf init fail!!");
        return -1;
    }
    init_MotionDet(workbuf, fs, thread);

    return ret;
}

// disable fmy gsensor
int sensor_deinit(void)
{
    log_info("%s, %s, %d", __FILE__, __FUNCTION__, __LINE__);
    if (!workbuf) {
        log_info("sensor is not init so can not deinit!!");
        return 0;
    }

    int ret = gsensor_disable();

    free(workbuf);
    workbuf = NULL;

    return ret;
}

#endif
