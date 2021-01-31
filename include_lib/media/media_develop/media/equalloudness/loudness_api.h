#ifndef EQUAL_LOUDNESS_API_H
#define EQUAL_LOUDNESS_API_H
int getEqualLoudnessBuf();
void EqualLoudnessInit(void *workBuf, float alpha, int sampleRate, int channel);
int EqulLoudnessRun(void *workBuf, short *in, short *out, int len);
#endif // !EQUAL_LOUDNESS_API_H

