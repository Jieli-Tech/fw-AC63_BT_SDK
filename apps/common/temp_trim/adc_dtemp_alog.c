#include "typedef.h"
#include "adc_dtemp_alog.h"

#ifdef SUPPORT_MS_EXTENSIONS
/* #pragma bss_seg(	".adc_bss") */
/* #pragma data_seg(	".adc_data") */
/* #pragma const_seg(	".adc_const") */
/* #pragma code_seg(	".adc_code") */
#endif

//*************************  avgfilt  **************************//

static void avgfilt_addSampleValue(AvgfiltData *data, unsigned short input)
{
    data->buf[data->curpos] = input;
    data->curpos = (data->curpos + 1) % data->length;
}

void avgfilt_init(AvgfiltData *data, unsigned short *buf, size_t length, unsigned short init)
{
    data->buf = buf;
    data->length = length;
    data->curpos = 0;
    data->sum  = 0;
    data->count = 0;
    for (int i = 0; i < data->length; i ++) {
        data->buf[i] = init;
    }
}

void avgfilt(AvgfiltData *data, unsigned short input, unsigned short *avg, int *valid)
{
    data->sum += ((int)input - (int)data->buf[data->curpos]);
    avgfilt_addSampleValue(data, input);
    data->count ++;
    if (data->count >= data->length) {
        *valid = 1;
    } else {
        *valid = 0;
    }
    *avg = (data->sum + data->length / 2) / data->length;
}


//**************************  medfilt  **************************//

static void swap(MedfiltNode **a, MedfiltNode **b)
{
    // Swap two node references in the sorted table.
    MedfiltNode *temp = *a;
    *a = *b;
    *b = temp;
    // Preserve index. Used to retrive the node position in the sorted table.
    size_t index = (*a)->index;
    (*a)->index = (*b)->index;
    (*b)->index = index;
}

void medfilt(MedfiltData *data, unsigned short input, unsigned short *median, int *valid)
{
    // New value replaces the oldest
    MedfiltNode *n = data->kernel;
    MedfiltNode *node = data->oldest;
    node->value = input;
    data->oldest = node->parent;

#define VAL(x) (n[x].sorted->value)

    // Sort the kernel
    for (size_t i = node->index; i < data->length - 1 && VAL(i) > VAL(i + 1); i++) {
        swap(&n[i].sorted, &n[i + 1].sorted);
    }
    for (size_t i = node->index; i > 0 && VAL(i) < VAL(i - 1); i--) {
        swap(&n[i].sorted, &n[i - 1].sorted);
    }
    // Get kernel information from sorted entries
    //*min = VAL(0);
    //*max = VAL(data->length - 1);
    *median = n[(data->length / 2) + 1].sorted->value;
    data->count += 1;
    if (data->count >= data->length) {
        *valid = 1;
    } else {
        *valid = 0;
    }

#undef VAL
}

void medfilt_init(MedfiltData *data, MedfiltNode *nodes, size_t length, unsigned short init)
{
    data->kernel = nodes;
    data->length = length;
    data->count = 0;
    // Linked list initialization
    data->oldest = &data->kernel[length - 1];
    for (size_t i = 0; i < length; i++) {
        data->kernel[i] = (MedfiltNode) {
            .value = init,
             .parent = data->oldest,
              .index = i,
               .sorted = &data->kernel[i]
        };
        data->oldest = &data->kernel[i];
    }
}

//*********************  stablefilt  **********************//
#define MY_ABS(a) ((a)<0?-(a):(a))

static void stablefilt_addSampleValue(StablefiltData *data, unsigned short input)
{
    data->buf[data->curpos] = input;
    data->curpos = (data->curpos + 1) % data->length;
}

void stablefilt_init(StablefiltData *data, unsigned short *buf, size_t length, size_t step, int threshold, unsigned short init)
{
    data->buf = buf;
    data->length = length;
    data->step = step;
    data->threshold = threshold;
    data->curpos = 0;
    data->count = 0;
    for (int i = 0; i < (data->length); i++) {
        data->buf[i] = init;
    }
}

void stablefilt(StablefiltData *data, unsigned short input, int *stable, int *valid)
{
    int idx, diff;
    *stable = 1;
    for (int i = 1; i < data->length; i += data->step) {
        idx = data->curpos - i;
        idx = (idx + data->length) % data->length;
        diff = (int)input - (int)data->buf[idx];
        diff = MY_ABS(diff);
        if (diff > data->threshold) {
            *stable = 0;
            break;
        }
    }
    stablefilt_addSampleValue(data, input);
    data->count++;
    if (data->count >= data->length) {
        *valid = 1;
    } else {
        *valid = 0;
    }
}

//*********************  tempsensor  **********************//

TempSensor _tempsensor;

TempSensor *get_tempsensor_pivr(void)
{
    return ((TempSensor *)&_tempsensor);
}

void tempsensor_init(TempSensor *data)
{
    data->stablefilt_th = STABLE_FILT_TH;
    data->stablefilt_step = STABLE_FILT_STEP;

    data->output = 0;
    data->valid = 0;
    data->stable = 0;
    data->delta = 0;
    data->ref_valid = 0;
    data->ref = 0;

    medfilt_init(&data->medfilt, (MedfiltNode *)&data->medfilt_nodes, MEDIAN_FILT_LENGTH, 0);
    avgfilt_init(&data->avgfilt, (unsigned short *)&data->avgfilt_buf, AVG_FILT_LENGTH, 0);
    stablefilt_init(&data->stablefilt, (unsigned short *)&data->stablefilt_buf, STABLE_FILT_LENGTH, data->stablefilt_step, data->stablefilt_th, 0);
}
void tempsensor_process(TempSensor *data, unsigned short input)
{
    unsigned short value = input;
    int _stable = 0;
    int _valid = 0;
    medfilt(&data->medfilt, value, &value, &_valid);
    if (!_valid) {
        goto invalid;
    }
    avgfilt(&data->avgfilt, value, &value, &_valid);
    if (!_valid) {
        goto invalid;
    }
    stablefilt(&data->stablefilt, value, &_stable, &_valid);
    if (!_valid) {
        _stable = 0;
        goto invalid;
    }
    if (!_stable && data->stable && data->ref_valid) {
        data->delta = (int)value - (int)data->ref;
    }
    if (_stable && !data->stable) {
        data->ref = (int)value - (int)data->delta;
        data->ref_valid = 1;
        data->delta = 0;
    }
    if (!data->ref_valid) {
        _valid = 0;
    }

    data->output = value;

invalid:
    data->stable = _stable;
    data->valid = _valid;
    /* printf("in:%d out:%d ref:%d sta:%d val:%d\n", input, data->output, data->ref, data->stable, data->valid); */
}

void tempsensor_update_ref(TempSensor *data, unsigned short new_ref)
{
    if (data->stable) {
        data->ref = new_ref;
    }
}





