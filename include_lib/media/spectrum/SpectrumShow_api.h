#ifndef SPECTRUMSHOW_API_H
#define SPECTRUMSHOW_API_H

int getSpectrumShowBuf();
void SpectrumShowInit(void *workBuf, float attackFactor, float releaseFactor, int fs, int channel, int mode, unsigned int fft_addr);
int SpectrumShowRun(void *workBuf, short *in, int len);
int getSpectrumNum(void *workBuf);
short *getSpectrumValue(void *workBuf);

#endif // !SPECTRUMSHOW_API_H
