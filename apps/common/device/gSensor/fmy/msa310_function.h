
/**
  ******************************************************************************
  * @file    msa_app_fixpoint.h
  * @author  memsensing algorithm team
  * @version V1.1.0
  * @date    18- 7 -2017
  * @brief   This file contains definitions for msa_app_fixpoint.c  firmware
  ******************************************************************************
**/

#ifndef _MSA_APP_FIXPOINT_H
#define _MSA_APP_FIXPOINT_H

#if TCFG_MSA310_EN

#define ABS(x)  ( (x)>0?(x):-(x) )
#define MSA_AWAKE				 0
#define MSA_RESTLESS		1
#define MSA_ASLEEP			2
#define MSA_ONTABLE			3

typedef struct {
    int thisDirectionUp;
    int lastDirectionUp;
    int thisDirectionUpCnt;
    int lastDirectionUpCnt;
    long long timeCnt;
    long long timeNow;
    long long lastPeakTime;
    long long thisPeakTime;
    long long lastStepTime;
    long long thisStepTime;
    int timeInterval;
    short int valueNew;
    short int valueOld;
    short	int valuePeak;
    short int valueValley;
    short int valueThreshold;
    short int valueThresholdInitial;
    short int ThresholdCnt;
    short int Threshold[4];
    int 	step_cnt;
    int   step_sum;
    short int stepMode;
    short int stepModeCnt;
    long long stepModeTimeCnt;
    short int staticModeCnt;

} Step_Typedef, *pStep_Typedef;

extern Step_Typedef Step_Structure;

typedef struct {
    unsigned char autoBrightFlag;
    unsigned int enableCnt;
    unsigned int enableCntLimit;
    unsigned int disableCnt;
    unsigned int disableCntLimit;
    unsigned int accSquareSum[2];
    unsigned int accSquareMaxLimit;
    unsigned int accSquareMinLimit;
    unsigned char isLevelRotationCnt;
    unsigned char isLevelCnt;
    unsigned char notLevelCnt;
    unsigned char isLevelCntLimit;
    unsigned char levelRotationEnableCnt;
    unsigned char levelRotationEnableCntLimit;
    unsigned char levelDisableCnt;
    unsigned char levelDisableCntLimit;
    unsigned char autoBrightFromLevelFlag;
    unsigned char lastStatus;
    unsigned char backlightOffCnt;
    unsigned char backlightOffCntSum;
    unsigned char lightOnTimeCnt;
    unsigned char lightOffTimeCnt;
    int yzAngel;
    int xzAngel;
    int xzAngelLimitMin;
    int xzAngelLimitMax;
    int yzAngelLimitMin;
    int yzAngelLimitMax;
} Bright_Typedef, *pBright_Typedef;

extern Bright_Typedef Bright_Structure;

typedef struct {
    unsigned char Sleep_End_Flag;
    unsigned int  accSquareSum[20];
    short int Acc[3][20];
    int  SumX;
    int  SumY;
    int  SumZ;
    int  SumX_Last;
    int  SumY_Last;
    int  SumZ_Last;
    unsigned char Data_Cnt;
    unsigned char A[5];
    unsigned short int Minute_Cnt;
} Sleep_Typedef, *pSleep_Typedef;

extern Sleep_Typedef Sleep_Structure;

void msa_param_init(void);
unsigned int msa_step(short int x, short int y, short int z);
unsigned char msa_light(short int x, short int y, short int z);
int msa_sleep(short int x, short int y, short int z);
void msa_step_clear(void);

#endif  /*TCFG_MSA310_EN*/
#endif  /*_MSA_APP_FIXPOINT_H*/

/************************ (C) COPYRIGHT MEMSENSING *****END OF FILE****/
