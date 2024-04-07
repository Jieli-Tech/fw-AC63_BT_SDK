#ifndef MOTION_API_H
#define MOTION_API_H

#define GSENSOR_PRINTF_ENABLE             0//contrl debug printf

int get_DetectionBuf(int fs);
void init_MotionDet(void *ptr, short fs, float thread);
char run_MotionDetection(void *ptr, int len, short *data);

#if WIN32
#else
extern inline float add_float(float x, float y);
extern inline float sub_float(float x, float y);
extern inline float mul_float(float x, float y);
extern inline float div_float(float x, float y);
extern inline float complex_abs_float(float x, float y);
#endif


#endif // !MOTION_API_H


