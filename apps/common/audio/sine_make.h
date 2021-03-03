#ifndef __SINE_MAKE_H_
#define __SINE_MAKE_H_

#include "generic/typedef.h"

#define DEFAULT_SINE_SAMPLE_RATE 16000
#define SINE_TOTAL_VOLUME        26843546//16106128//20132660 //26843546

struct sin_param {
    //int idx_increment;
    int freq;
    int points;
    int win;
    int decay;
};

int sin_tone_make(void *_maker, void *data, int len);
void *sin_tone_open(const struct sin_param *param, int num, u8 channel, u8 repeat);
int sin_tone_points(void *_maker);
void sin_tone_close(void *_maker);

#endif/*__SINE_MAKE_H_*/
