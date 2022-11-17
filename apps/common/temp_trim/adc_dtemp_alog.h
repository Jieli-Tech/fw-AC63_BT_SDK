
#ifndef __ADC_DTEMP_ALOG_H__
#define __ADC_DTEMP_ALOG_H__

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct {

    unsigned short *buf;
    int curpos;
    size_t length;
    size_t count;
    unsigned int sum;

} AvgfiltData;

void avgfilt_init(AvgfiltData *data, unsigned short *buf, size_t length, unsigned short init);
void avgfilt(AvgfiltData *data, unsigned short input, unsigned short *avg, int *valid);



typedef struct _MedfiltNode {
    unsigned short value;
    size_t index; // Node index in the sorted table
    struct _MedfiltNode *parent;
    struct _MedfiltNode *sorted;
} MedfiltNode;

typedef struct {
    MedfiltNode *kernel; // Working filter memory
    MedfiltNode *oldest; // Reference to the oldest value in the kernel
    size_t length; // Number of nodes
    size_t count;
} MedfiltData;

void medfilt_init(MedfiltData *data, MedfiltNode *nodes, size_t length, unsigned short init);
void medfilt(MedfiltData *data, unsigned short input, unsigned short *median, int *valid);


typedef struct  {
    unsigned short *buf;
    int curpos;
    int threshold;
    size_t length;
    size_t step;
    size_t count;
} StablefiltData;

void stablefilt_init(StablefiltData *data, unsigned short *buf, size_t length, size_t step, int threshold, unsigned short init);
void stablefilt(StablefiltData *data, unsigned short input, int *stable, int *valid);


#define MEDIAN_FILT_LENGTH      (20)
#define AVG_FILT_LENGTH         (20)
#define STABLE_FILT_LENGTH      (25)
#define STABLE_FILT_STEP        (5)
#define STABLE_FILT_TH          (10)
typedef struct  {
    MedfiltData  medfilt;
    AvgfiltData  avgfilt;
    StablefiltData stablefilt;

    MedfiltNode  medfilt_nodes[MEDIAN_FILT_LENGTH];
    unsigned short avgfilt_buf[AVG_FILT_LENGTH];
    unsigned short stablefilt_buf[STABLE_FILT_LENGTH];

    int stablefilt_step;
    int stablefilt_th;

    int output;
    int valid;
    int stable;
    int delta;
    int ref_valid;
    unsigned short ref;

} TempSensor;

TempSensor *get_tempsensor_pivr(void);

void tempsensor_init(TempSensor *data);

//Parameters:
//input:
//  the input sample value
void tempsensor_process(TempSensor *data, unsigned short input);

void tempsensor_update_ref(TempSensor *data, unsigned short new_ref);

#endif



