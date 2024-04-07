#ifndef __SENSOR_API_H__
#define __SENSOR_API_H__
#include "gSensor_manage.h"

int sensor_init();
int sensor_data_get();
void get_sensor_avg(axis_info_t *axis_avg);
int sensor_deinit();
bool sensor_motion_detection(void);

typedef enum {
    DETECTION_PASSIVE = 0,
    DETECTION_ACTIVE
} DETECTION_STATES;

static DETECTION_STATES detection_status = DETECTION_PASSIVE;

#endif
