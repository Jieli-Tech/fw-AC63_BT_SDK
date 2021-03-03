#ifndef _OMSM_H_
#define _OMSM_H_

#include "system/includes.h"

#define VECTOR_REVERS(vec)	vec = ~vec; vec++

typedef struct {
    u8 OMSensor_id[20];
    u8 OMSensor_sclk_io;
    u8 OMSensor_data_io;
    u8 OMSensor_int_io;
} OMSENSOR_PLATFORM_DATA;

typedef struct {
    u8 OMSensor_id[20];
    bool(*OMSensor_init)(OMSENSOR_PLATFORM_DATA *);
    bool (*OMSensor_read_motion)(s16 *, s16 *);
    u8(*OMSensor_data_ready)(void);
    u8(*OMSensor_status_dump)(void);
    void (*OMSensor_wakeup)(void);
    void (*OMSensor_led_switch)(u8);
    u16(*OMSensor_set_cpi)(u16 dst_cpi);
} OMSENSOR_INTERFACE;


extern OMSENSOR_INTERFACE OMSensor_dev_begin[];
extern OMSENSOR_INTERFACE OMSensor_dev_end[];

#define REGISTER_OMSENSOR(OMSensor) \
	static OMSENSOR_INTERFACE OMSensor SEC_USED(.omsensor_dev)

#define list_for_each_omsensor(c) \
	for (c=OMSensor_dev_begin; c<OMSensor_dev_end; c++)

#define OMSENSOR_PLATFORM_DATA_BEGIN(data) \
		OMSENSOR_PLATFORM_DATA data = {

#define OMSENSOR_PLATFORM_DATA_END() \
};

bool optical_mouse_sensor_init(OMSENSOR_PLATFORM_DATA *priv);
void optical_mouse_sensor_read_motion_handler(void);
u16 optical_mouse_sensor_set_cpi(u16 dst_cpi);
u8 get_optical_mouse_sensor_status(void);
void optical_mouse_sensor_force_wakeup(void);
void optical_mouse_sensor_led_switch(u8 led_status);

#endif // _OMSM_H_

