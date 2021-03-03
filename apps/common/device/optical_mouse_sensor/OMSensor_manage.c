#include "OMSensor_manage.h"
#include "system/includes.h"
/* #include "asm/port_wkup.h" */
#include "app_config.h"

#if TCFG_OMSENSOR_ENABLE

#define LOG_TAG             "[OMSENSOR]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"

static const OMSENSOR_INTERFACE *OMSensor_hdl = NULL;

/* static void optical_mouse_sensor_read_motion_handler(void); */
/* static void optical_mouse_sensor_read_motion_handler(void *priv); */

static s16 avg_filter(s16 *pdata, u8 num)
{
    u8 i = 0;
    s16 sum = 0;

    for (i = 0; i < num; i++) {
        sum += pdata[i];
    }

    return (sum / num);
}

/* __attribute__((interrupt(""))) */
/* static void port_wkup_isr() */
/* { */
/* 	if ((JL_WAKEUP->CON0 & BIT(3)) && (JL_WAKEUP->CON3 & BIT(3))) */
/*     { */
/* 		JL_WAKEUP->CON2 |= BIT(3); */
/* 		#<{(| log_info("optical interrupt\n"); |)}># */
/* 		#<{(| optical_mouse_sensor_read_motion_handler(); |)}># */
/* 	} */
/* } */

static void optical_mouse_sensor_event_to_usr(u8 event, s16 x, s16 y)
{
    struct sys_event e;
    e.type = SYS_DEVICE_EVENT;
    e.arg = "omsensor_axis";
    e.u.axis.event = event;
    e.u.axis.x = x;
    e.u.axis.y = y;
    sys_event_notify(&e);
}

static u8 optical_mouse_sensor_data_ready(void)
{
    u8 ret = 0;
    if (OMSensor_hdl->OMSensor_data_ready) {
        ret = OMSensor_hdl->OMSensor_data_ready();
    } else {
        ret = 0;
    }

    return ret;
}

void optical_mouse_sensor_read_motion_handler(void)
/* static void optical_mouse_sensor_read_motion_handler(void) */
{
    s16 x = 0, y = 0;
    s16 temp = 0;

    if (optical_mouse_sensor_data_ready()) {
        if (OMSensor_hdl->OMSensor_read_motion) {
            OMSensor_hdl->OMSensor_read_motion(&x, &y);
        }

        temp = x;
        x = y;
        y = temp;

        VECTOR_REVERS(x);
        VECTOR_REVERS(y);

#if 0
        static s16 x_data[3] = {0}, y_data[3] = {0};
        static u8 count = 0;
        u8 i = 0;

        if (count < ARRAY_SIZE(x_data)) {
            x_data[count] = x;
            y_data[count] = y;
        } else {
            for (i = ARRAY_SIZE(x_data) - 1; i > 0; i--) {
                x_data[i] = x_data[i - 1];
            }
            x_data[0] = x;

            for (i = ARRAY_SIZE(y_data) - 1; i > 0; i--) {
                y_data[i] = y_data[i - 1];
            }
            y_data[0] = y;
        }

        if (count < ARRAY_SIZE(x_data)) {
            count++;
        }

        x = avg_filter(&x_data[0], ARRAY_SIZE(x_data));
        y = avg_filter(&y_data[0], ARRAY_SIZE(y_data));
#endif

        optical_mouse_sensor_event_to_usr(0, x, y);
    }
}


u16 optical_mouse_sensor_set_cpi(u16 dst_cpi)
{
    u16 cpi = 0;

    if (OMSensor_hdl->OMSensor_set_cpi) {
        cpi = OMSensor_hdl->OMSensor_set_cpi(dst_cpi);
    }

    return cpi;
}


u8 get_optical_mouse_sensor_status(void)
{
    u8 ret = 0;

    if (OMSensor_hdl->OMSensor_status_dump) {
        ret =  OMSensor_hdl->OMSensor_status_dump();
    }

    return ret;
}

void optical_mouse_sensor_force_wakeup(void)
{
    if (OMSensor_hdl->OMSensor_wakeup) {
        OMSensor_hdl->OMSensor_wakeup();
    }
}

void optical_mouse_sensor_led_switch(u8 led_status)
{
    if (OMSensor_hdl->OMSensor_led_switch) {
        OMSensor_hdl->OMSensor_led_switch(led_status);
    }
}

bool optical_mouse_sensor_init(OMSENSOR_PLATFORM_DATA *priv)
{
    int retval = false;

    //查询驱动列表，匹配设备
    list_for_each_omsensor(OMSensor_hdl) {
        if (!memcmp(OMSensor_hdl->OMSensor_id, priv->OMSensor_id, strlen(priv->OMSensor_id))) {
            retval = true;
            break;
        }
    }

    //匹配失败
    if (retval == false) {
        log_error("no match optical mouse sensor driver");
        return retval;
    }

    //初始化设备驱动
    retval = false;
    if (OMSensor_hdl->OMSensor_init) {
        retval = OMSensor_hdl->OMSensor_init(priv);
    }

    //设置optical mouse sensor的采样率
    if (retval == true) {
        sys_s_hi_timer_add(NULL, optical_mouse_sensor_read_motion_handler, OPTICAL_SENSOR_SAMPLE_PERIOD); //10ms

        /* port_wkup_enable(priv->OMSensor_int_io, 1); */
        /* request_irq(IRQ_PORT_IDX, 3, port_wkup_isr, 0); */
    }


    return retval;
}

#endif


