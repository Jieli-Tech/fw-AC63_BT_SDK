
/**
  ******************************************************************************
  * @file    msa300_app_fixpoint.c
  * @author  memsensing algorithm team
  * @version V1.1.0
  * @date    16- 8 -2018
  * @brief   This file provides pedometer function of G-sensor MSA30x application
  ******************************************************************************
**/
#include "gSensor_manage.h"
#include "msa310_function.h"

#if TCFG_MSA310_EN

Step_Typedef Step_Structure;
Bright_Typedef Bright_Structure;
Sleep_Typedef Sleep_Structure;

// void sum_Calculate(void);
// void A_Calculate(void);
// void sleep_Mode_Calculate(void);
// void isTurn_Judge(void);
void find_Turn(void);
void sleep_Para_Clear(void);


static int a1[7] = {16384, -79111, 170817, -209099, 152847, -63295, 11720};
static int b1[7] = {805, -2623, 2884, 0, -2884, 2623, -806};
static int a2[7] = {16384, -64148, 119340, -133733, 95028, -40165, 8056};
static int b2[7] = {1630, -3977, 3285, 0, -3285, 3977, -1630};
static int beforeFilter[7];
static int afterFilter[7];

void msa_step_clear(void)//计步清零
{
    Step_Structure.step_cnt = 0;
    Step_Structure.step_sum = 0;
    Step_Structure.timeCnt 	= 0;
    Step_Structure.timeNow  = 0;
    Step_Structure.lastPeakTime = 0;
    Step_Structure.thisPeakTime = 0;
    Step_Structure.lastStepTime = 0;
    Step_Structure.thisStepTime = 0;
    Step_Structure.ThresholdCnt = 0;
    Step_Structure.stepMode = 1;
    Step_Structure.stepModeCnt = 0;
    Step_Structure.staticModeCnt = 0;
}

void msa_param_init(void)
{
    //Pedometer Structure initial
    Step_Structure.thisDirectionUp = 0;
    Step_Structure.timeInterval  = 5;
    Step_Structure.valueThreshold = 40;
    Step_Structure.valueThresholdInitial = 25;
    Step_Structure.stepMode = 1;
    Step_Structure.stepModeCnt = 0;

    Bright_Structure.autoBrightFlag = 0;
    Bright_Structure.accSquareSum[0] = 0;
    Bright_Structure.accSquareSum[1] = 0;
    Bright_Structure.accSquareMaxLimit = 98304;
    Bright_Structure.accSquareMinLimit = 23909;//43909
    Bright_Structure.enableCnt = 0;
    Bright_Structure.enableCntLimit = 4;
    Bright_Structure.disableCnt = 0;
    Bright_Structure.disableCntLimit = 12;
    Bright_Structure.xzAngel = 0;
    Bright_Structure.yzAngel = 0;
    Bright_Structure.xzAngelLimitMax = 35;
    Bright_Structure.xzAngelLimitMin = -35;
    Bright_Structure.yzAngelLimitMax = 90;
    Bright_Structure.yzAngelLimitMin = 5;//25
    Bright_Structure.lastStatus = 1;
    Bright_Structure.isLevelRotationCnt = 0;
    Bright_Structure.autoBrightFromLevelFlag = 0;
    Bright_Structure.levelRotationEnableCntLimit = 4;
    Bright_Structure.levelDisableCntLimit = 40;
    Bright_Structure.isLevelCntLimit = 4;

    Sleep_Structure.Data_Cnt = 0;
    Sleep_Structure.Minute_Cnt = 0;
}

static int peak_Search(short int newData, short int oldData)
{
    Step_Structure.lastDirectionUp = Step_Structure.thisDirectionUp;

    if (newData > oldData) {
        Step_Structure.thisDirectionUp = 1;
        Step_Structure.thisDirectionUpCnt++;
    }

    else if (newData < oldData) {
        Step_Structure.lastDirectionUpCnt = Step_Structure.thisDirectionUpCnt;
        Step_Structure.thisDirectionUpCnt = 0;
        Step_Structure.thisDirectionUp = 0;
    }

    if (!Step_Structure.thisDirectionUp && \
        Step_Structure.lastDirectionUp && \
        Step_Structure.lastDirectionUpCnt	>= 2 && \
        oldData >= 0) {
        Step_Structure.valuePeak = oldData;
        return 1;
    } else if (Step_Structure.thisDirectionUp && \
               !Step_Structure.lastDirectionUp) {
        Step_Structure.valueValley = oldData;
        return 0;
    } else {
        return 0;
    }
}

static void step_Count(void)
{
    Step_Structure.lastStepTime =  Step_Structure.thisStepTime;
    Step_Structure.thisStepTime =  Step_Structure.timeCnt;
    //2s
    if (Step_Structure.thisStepTime - Step_Structure.lastStepTime <= 20) { //40
        if (Step_Structure.step_cnt < 20) { //12
            Step_Structure.step_cnt++;
        } else if (Step_Structure.step_cnt == 20) { //12
            Step_Structure.step_cnt++;
            Step_Structure.step_sum += Step_Structure.step_cnt;
            Step_Structure.valueThresholdInitial = 20;
        } else {
            Step_Structure.step_sum++;
        }
    } else {
        Step_Structure.step_cnt = 1;
        Step_Structure.valueThresholdInitial = 40;
        Step_Structure.valueThreshold = 40;
    }
}

static void valueThreshold_Refresh(void)
{
    short int average;
    if (Step_Structure.ThresholdCnt < 4) {
        Step_Structure.Threshold[Step_Structure.ThresholdCnt] = Step_Structure.valuePeak - Step_Structure.valueValley;
        Step_Structure.ThresholdCnt++;
    } else {
        Step_Structure.Threshold[0] = Step_Structure.Threshold[1];
        Step_Structure.Threshold[1] = Step_Structure.Threshold[2];
        Step_Structure.Threshold[2] = Step_Structure.Threshold[3];
        Step_Structure.Threshold[3] = Step_Structure.valuePeak - Step_Structure.valueValley;
        average = (Step_Structure.Threshold[0] + Step_Structure.Threshold[1] + \
                   Step_Structure.Threshold[2] + Step_Structure.Threshold[3]);

        average /= 4;

        if (average > 150) {
            Step_Structure.valueThreshold = 70;
        } else if (average > 120) {
            Step_Structure.valueThreshold = 55;
        } else if (average > 90) {
            Step_Structure.valueThreshold = 40;
        } else if (average > 75) {
            Step_Structure.valueThreshold = 30;
        } else if (average > 60) {
            Step_Structure.valueThreshold = 25;
        } else {
            Step_Structure.valueThreshold = 16;
        }
    }
}

void USER_Pedometer(void)
{
    if (Step_Structure.valueOld == 0) {
        Step_Structure.valueOld = Step_Structure.valueNew;
    } else {
        if (peak_Search(Step_Structure.valueNew, Step_Structure.valueOld)) {
            Step_Structure.lastPeakTime = Step_Structure.thisPeakTime;
            Step_Structure.timeNow = Step_Structure.timeCnt;
            if (((Step_Structure.timeNow - Step_Structure.lastPeakTime) >= Step_Structure.timeInterval) && \
                ((Step_Structure.valuePeak - Step_Structure.valueValley) >= Step_Structure.valueThreshold)) {
                Step_Structure.thisPeakTime = Step_Structure.timeNow;
                step_Count();
            }
            if (((Step_Structure.timeNow - Step_Structure.lastPeakTime) >= Step_Structure.timeInterval) && \
                ((Step_Structure.valuePeak - Step_Structure.valueValley) >= Step_Structure.valueThresholdInitial)) {
                valueThreshold_Refresh();
            }
        }
    }
    Step_Structure.valueOld = Step_Structure.valueNew;
}


static int msa_filterWalk(int data)
{

    int ret = 0;

    beforeFilter[0] = data * 5;

    for (int i = 0; i < 7; i++) {
        ret += b1[i] * beforeFilter[i];
    }

    for (int i = 1; i < 7; i++) {
        ret -= a1[i] * afterFilter[i];
    }

    ret /= 16384;

    afterFilter[0] = ret;

    for (int i = 6; i > 0; i--) {
        afterFilter[i]  = afterFilter[i - 1];
        beforeFilter[i] = beforeFilter[i - 1];
    }

    return ret;
}

static int msa_filterRun(int data)
{
    int ret = 0;

    beforeFilter[0] = data * 5;

    for (int i = 0; i < 7; i++) {
        ret += b2[i] * beforeFilter[i];
    }

    for (int i = 1; i < 7; i++) {
        ret -= a2[i] * afterFilter[i];
    }

    ret /= 16384;

    afterFilter[0] = ret;

    for (int i = 6; i > 0; i--) {
        afterFilter[i]  = afterFilter[i - 1];
        beforeFilter[i] = beforeFilter[i - 1];
    }

    return ret;
}

//æ±sqrt
static int msa_sqrt(int val)
{
    int r = 0;
    int shift;

    if (val < 0) {
        return 0;
    }

    for (shift = 0; shift < 32; shift += 2) {
        int x = 0x40000000l >> shift;
        if (x + r <= val) {
            val -= x + r;
            r = (r >> 1) | x;
        } else {
            r = r >> 1;
        }
    }
    return r;
}

/*
 * 计步器函数，msa_step(),返回的为实际步数，15步开始计数，不满15步停止视为无效步数，不清零一直累加，需要手动清零。//msa_app.c , msa310.c
50毫秒的timer调用一次msa_get_step，关闭时停止timer。使用FIFO数据(数据长度与ODR相关)。 //未找到 添加到msa310.c
例如500MS，10组数据，就循环调用10次算法。
 */
unsigned int msa_step(short int x, short int y, short int z)
{
    Step_Structure.timeCnt++;
    Step_Structure.valueNew = msa_sqrt(x * x + y * y + z * z);

    if (Step_Structure.stepMode == 1 && Step_Structure.valueNew > 200 && Step_Structure.valueNew < 295) {
        Step_Structure.staticModeCnt ++;
        if (Step_Structure.staticModeCnt > 10) {
            Step_Structure.stepMode = 0;
        }
    } else {
        Step_Structure.staticModeCnt = 0;
    }

    if (Step_Structure.stepMode == 0 && Step_Structure.valueNew > 295) {
        Step_Structure.stepMode = 1;
    }

    if (Step_Structure.stepMode == 1 && Step_Structure.valueNew > 620) {
        if (Step_Structure.stepModeCnt == 0) {
            Step_Structure.stepModeTimeCnt = Step_Structure.timeCnt;
        }
        Step_Structure.stepModeCnt ++;

        if (Step_Structure.stepModeCnt > 5) {
            if ((Step_Structure.timeCnt - Step_Structure.stepModeTimeCnt) < 60) {
                Step_Structure.stepMode = 2;
                Step_Structure.stepModeCnt = 0;
            } else {
                Step_Structure.stepModeCnt = 0;
            }
        }
    }
    if (Step_Structure.stepMode == 2) {
        if (Step_Structure.valueNew < 620) {
            Step_Structure.stepModeCnt ++;
            if (Step_Structure.stepModeCnt > 50) {
                Step_Structure.stepMode = 1;
                Step_Structure.stepModeCnt = 0;
            }
        } else {
            Step_Structure.stepModeCnt = 0;
        }
    }

    if (Step_Structure.stepMode == 1) {
        Step_Structure.valueNew = msa_filterWalk(Step_Structure.valueNew);
    } else if (Step_Structure.stepMode == 2) {
        Step_Structure.valueNew = msa_filterRun(Step_Structure.valueNew);
    }

    if (Step_Structure.stepMode != 0) {
        USER_Pedometer();
    }
    return Step_Structure.step_sum;
}

static unsigned char isStatic(int val)
{
    if (val < Bright_Structure.accSquareMaxLimit && \
        val > Bright_Structure.accSquareMinLimit) {
        return 1;
    }

    return 0;
}

static unsigned char light_off_detect(short int x, short int y, short int z)
{
    if (Bright_Structure.lightOffTimeCnt) {
        Bright_Structure.lightOffTimeCnt++;
        if (Bright_Structure.lightOffTimeCnt >= 3) {
            Bright_Structure.lightOffTimeCnt = 0;
            Bright_Structure.lastStatus = 0;
            return 2;
        }
    }

    if (Bright_Structure.lastStatus == 1) {
#ifdef KCT_GSETURE_WAKEUP

        if (isStatic(Bright_Structure.accSquareSum[0]) && \
            (ABS(x) > 110 || y < -80)) {
            Bright_Structure.backlightOffCnt = 0;

            if (!Bright_Structure.lightOffTimeCnt) {
                Bright_Structure.lightOffTimeCnt = 1;
            }
            //return 2;
        }
#endif

        if (isStatic(Bright_Structure.accSquareSum[0]) && \
            ABS(x) > 162) {
            Bright_Structure.backlightOffCnt = 0;

            if (!Bright_Structure.lightOffTimeCnt) {
                Bright_Structure.lightOffTimeCnt = 1;
            }
        }
        Bright_Structure.backlightOffCnt++;

    }

    return 0;
}

static unsigned char isLevel_detect(short int x, short int y, short int z)
{
    if (ABS(x) < 76 && ABS(y) < 76 && z > 200 && z < 281) {
        return 1;
    }

    return 0;
}

static int msa_atan(int x, int y)
{
    const int angle[] = {11520, 6801, 3593, 1824, 916, 458, 229, 115, 57, 29, 14, 7, 4, 2, 1};

    int i = 0;
    int x_new, y_new;
    int angleSum = 0;

    x *= 1024;
    y *= 1024;

    for (i = 0; i < 15; i++) {
        if (y > 0) {
            x_new = x + (y >> i);
            y_new = y - (x >> i);
            x = x_new;
            y = y_new;
            angleSum += angle[i];
        } else {
            x_new = x - (y >> i);
            y_new = y + (x >> i);
            x = x_new;
            y = y_new;
            angleSum -= angle[i];
        }
    }
    return angleSum;
}



static unsigned char light_on_detect(short int x, short int y, short int z)
{
#ifdef KCT_GSETURE_WAKEUP

    if (Bright_Structure.lightOnTimeCnt) {
        Bright_Structure.lightOnTimeCnt++;
        if (Bright_Structure.lightOnTimeCnt >= 4) {
            Bright_Structure.lightOnTimeCnt = 0;
            Bright_Structure.lastStatus = 1;
            return 1;
        }
    }

    if (Bright_Structure.autoBrightFromLevelFlag && \
        isStatic(Bright_Structure.accSquareSum[0]) && \
        (ABS(x) < 75 && y > 90 && y < 200 && z > 75)
       ) {
        Bright_Structure.levelRotationEnableCnt ++;
        if (Bright_Structure.levelRotationEnableCnt >= Bright_Structure.levelRotationEnableCntLimit) {
            Bright_Structure.levelDisableCnt = 0;
            Bright_Structure.levelRotationEnableCnt = 0;
            Bright_Structure.autoBrightFromLevelFlag = 0;
            Bright_Structure.accSquareSum[1] = Bright_Structure.accSquareSum[0];
            if (!Bright_Structure.lightOnTimeCnt) {
                Bright_Structure.lightOnTimeCnt = 1;
            }
            //return 1;
        }
    } else {
        Bright_Structure.levelRotationEnableCnt = 0;
    }

    Bright_Structure.levelDisableCnt++;

    if (Bright_Structure.levelDisableCnt >= Bright_Structure.levelDisableCntLimit) {
        Bright_Structure.levelDisableCnt = 0;
        Bright_Structure.autoBrightFromLevelFlag = 0;
    }
#endif

#ifdef  KCT_GSETURE_WAKEUP
    if (isLevel_detect(x, y, z)) {
        Bright_Structure.isLevelCnt++;
        if (Bright_Structure.isLevelCnt >= Bright_Structure.isLevelCntLimit) {
            if (Bright_Structure.notLevelCnt >= 5) {
                Bright_Structure.accSquareSum[1] = Bright_Structure.accSquareSum[0];
                Bright_Structure.lastStatus = 1;
                Bright_Structure.isLevelCnt = 0;
                Bright_Structure.notLevelCnt = 0;
                Bright_Structure.lightOnTimeCnt = 1;
                //return 1;
            } else {
                Bright_Structure.isLevelCnt = 0;
                Bright_Structure.notLevelCnt = 0;
            }
        }
    } else {
        Bright_Structure.notLevelCnt++;
        if (Bright_Structure.notLevelCnt > 20) {
            Bright_Structure.notLevelCnt = 20;
        }
        Bright_Structure.isLevelCnt = 0;
    }
#endif

    if (!isStatic(Bright_Structure.accSquareSum[0])) {
        Bright_Structure.enableCnt = 0;
    }

    if (isStatic(Bright_Structure.accSquareSum[0]) && \
        Bright_Structure.accSquareSum[1] > 0 && \
        !isStatic(Bright_Structure.accSquareSum[1])) {
        Bright_Structure.autoBrightFlag = 1;
    }

    if (Bright_Structure.autoBrightFlag) {
        Bright_Structure.disableCnt ++;
    }
    if (Bright_Structure.disableCnt >= Bright_Structure.disableCntLimit) {
        Bright_Structure.disableCnt = 0;
        Bright_Structure.autoBrightFlag = 0;
    }

    if (Bright_Structure.autoBrightFlag && y > 0 &&  z > 0 && \
        isStatic(Bright_Structure.accSquareSum[0])) {
        Bright_Structure.enableCnt ++;

        if (Bright_Structure.enableCnt >= Bright_Structure.enableCntLimit) {
            if (z == 0) {
                z = 1 ;
            }
            Bright_Structure.xzAngel = msa_atan(z, x) / 256;
            Bright_Structure.yzAngel = msa_atan(z, y) / 256;

            if (z >= 38) {
                if (Bright_Structure.xzAngel > Bright_Structure.xzAngelLimitMin && \
                    Bright_Structure.xzAngel < Bright_Structure.xzAngelLimitMax && \
                    Bright_Structure.yzAngel > Bright_Structure.yzAngelLimitMin && \
                    Bright_Structure.yzAngel < Bright_Structure.yzAngelLimitMax) {
                    Bright_Structure.enableCnt = 0;
                    Bright_Structure.disableCnt = 0;
                    Bright_Structure.autoBrightFlag = 0;
                    Bright_Structure.accSquareSum[1] = Bright_Structure.accSquareSum[0];
                    Bright_Structure.lastStatus = 1;
                    return 1;
                }
            } else if (y > 212) {
                Bright_Structure.enableCnt = 0;
                Bright_Structure.disableCnt = 0;
                Bright_Structure.autoBrightFlag = 0;
                Bright_Structure.accSquareSum[1] = Bright_Structure.accSquareSum[0];
                Bright_Structure.lastStatus = 1;
                return 1;
            }
        }
    } else {
        Bright_Structure.enableCnt = 0;
    }
    return 0;
}


/*50毫秒的timer调用一次msa_light，关闭时停止timer。    //msa_app.c , msa310.c
   函数返回值: 1 : 亮屏   2:灭屏  0:无动作
   使用FIFO数据(数据长度与ODR相关)。
   例如500MS，10组数据，识别有一次抬手动作，开启亮屏，其他无效。
*/
unsigned char msa_light(short int x, short int y, short int z)
{
    unsigned char ret;
    Bright_Structure.accSquareSum[0] = x * x + y * y + z * z ;

#ifdef KCT_GSETURE_WAKEUP

    if (!isLevel_detect(x, y, z) && Bright_Structure.isLevelRotationCnt > 9) {
        Bright_Structure.autoBrightFromLevelFlag = 1;
    }

#endif

    ret = light_off_detect(x, y, z);
    if (ret) {
        return ret;
    }

    ret = light_on_detect(x, y, z);
    if (ret) {
        return ret;
    }

#ifdef KCT_GSETURE_WAKEUP
    if (isLevel_detect(x, y, z)) {
        Bright_Structure.isLevelRotationCnt++;
        if (Bright_Structure.isLevelRotationCnt++ > 20) {
            Bright_Structure.isLevelRotationCnt = 20;
        }
    } else {
        if (Bright_Structure.isLevelRotationCnt > 10) {
            Bright_Structure.isLevelRotationCnt -= 5;
        } else {
            Bright_Structure.isLevelRotationCnt = 0;
        }
    }
#endif

    Bright_Structure.accSquareSum[1] = Bright_Structure.accSquareSum[0];
    return 0;
}



static char m_cnt = 20;
static int sleep_time_cnt = 0;
static int sleep_this_status = -1;
static int sleep_ontable_cnt = 0;
static void sum_Calculate(void)
{
    int i = 0;
    int SumX = 0;
    int SumY = 0;
    int SumZ = 0;

    if (Sleep_Structure.Minute_Cnt > 0) {
        Sleep_Structure.SumX_Last = Sleep_Structure.SumX;
        Sleep_Structure.SumY_Last = Sleep_Structure.SumY;
        Sleep_Structure.SumZ_Last = Sleep_Structure.SumZ;
    }

    for (i = 0; i < m_cnt; i++) {
        SumX += Sleep_Structure.Acc[0][i];
        SumY += Sleep_Structure.Acc[1][i];
        SumZ += Sleep_Structure.Acc[2][i];
    }

    Sleep_Structure.SumX = SumX;
    Sleep_Structure.SumY = SumY;
    Sleep_Structure.SumZ = SumZ;
}

static void isTurn_Judge(void)
{
    int P = 0;
    int AvgX = 0;
    int AvgY = 0;
    int AvgZ = 0;
    int Mx = 0;
    int My = 0;
    int Mz = 0;
    int i = 0;

    sum_Calculate();

    if (Sleep_Structure.Minute_Cnt > 0) {
        P = ((Sleep_Structure.SumX - Sleep_Structure.SumX_Last) / m_cnt) * ((Sleep_Structure.SumX - Sleep_Structure.SumX_Last) / m_cnt) + \
            ((Sleep_Structure.SumY - Sleep_Structure.SumY_Last) / m_cnt) * ((Sleep_Structure.SumY - Sleep_Structure.SumY_Last) / m_cnt) + \
            ((Sleep_Structure.SumZ - Sleep_Structure.SumZ_Last) / m_cnt) * ((Sleep_Structure.SumZ - Sleep_Structure.SumZ_Last) / m_cnt);

        AvgX = Sleep_Structure.SumX / m_cnt;
        AvgY = Sleep_Structure.SumY / m_cnt;
        AvgZ = Sleep_Structure.SumZ / m_cnt;
        for (i = 0; i < m_cnt; i++) {
            Mx += ABS(Sleep_Structure.Acc[0][i] - AvgX);
            My += ABS(Sleep_Structure.Acc[1][i] - AvgY);
            Mz += ABS(Sleep_Structure.Acc[2][i] - AvgZ);
        }
        Mx /= m_cnt;
        My /= m_cnt;
        Mz /= m_cnt;
    }

    if ((P > 7864) && ((Mx > 92) || (My > 92) || (Mz > 92))) {
        sleep_time_cnt = 0;
    }
}

static void A_Calculate(void)
{
    int i = 0;

    for (i = 0; i < 4; i++) {
        Sleep_Structure.A[i] = Sleep_Structure.A[i + 1];
    }
    Sleep_Structure.A[4] = 0;
    for (i = 0; i < m_cnt; i++) {
        /*	if(Sleep_Structure.accSquareSum[i] > 1386741 || \
        		 Sleep_Structure.accSquareSum[i] < 757596)
        	*/
        if (Sleep_Structure.accSquareSum[i] > 1270000 || \
            Sleep_Structure.accSquareSum[i] < 840000) {
            Sleep_Structure.A[4]++;
        }
    }
    Sleep_Structure.A[4] *= 2;
}

static void sleep_Mode_Calculate(void)
{
    int S  = 0;
    int P  = 92;
    int X1 = 6;
    int X2 = 24;
    int X3 = 100;
    int X4 = 22;
    int X5 = 4;

    S = (Sleep_Structure.A[0] * X1 + \
         Sleep_Structure.A[1] * X2 + \
         Sleep_Structure.A[2] * X3 + \
         Sleep_Structure.A[3] * X4 + \
         Sleep_Structure.A[4] * X5);

    S *= P;
    if (S < 100000) {
        sleep_time_cnt ++;
        if (sleep_time_cnt > 2) {
            sleep_this_status = MSA_RESTLESS;
        }
        if (sleep_time_cnt >= 15) {
            sleep_this_status = MSA_ASLEEP;
        }
    } else {
        sleep_time_cnt = 0;
        sleep_this_status = MSA_AWAKE;
    }

    if (S == 0) {
        sleep_ontable_cnt ++;
        if (sleep_ontable_cnt > 30) {
            sleep_this_status = MSA_ONTABLE;
        }
    } else {
        sleep_ontable_cnt = 0;
    }

}

/*
3S采样一次数据给算法msa_sleep，1分钟统计一次数据。
   开启睡眠检测，7分钟后，每一分中统计一次，7-15分钟浅睡，15分钟以后深睡，否则为清醒，具体参考算法。
   如果检测到平放，并且超过半个小时，则认为未佩戴
   函数返回值:  -1: 睡眠采样阶段  0:睡眠质量差   1:睡眠质量一般   2:睡眠质量佳  3:未佩戴
*/
static int sleep_last_status = MSA_AWAKE;
int msa_sleep(short int x, short int y, short int z)
{
    x *= 4;
    y *= 4;
    z *= 4;

    Sleep_Structure.accSquareSum[Sleep_Structure.Data_Cnt] = x * x + y * y + z * z;
    Sleep_Structure.Acc[0][Sleep_Structure.Data_Cnt] = x;
    Sleep_Structure.Acc[1][Sleep_Structure.Data_Cnt] = y;
    Sleep_Structure.Acc[2][Sleep_Structure.Data_Cnt] = z;

    if (Sleep_Structure.accSquareSum[Sleep_Structure.Data_Cnt] > 3397386) {
        sleep_time_cnt = 0;
        sleep_this_status = MSA_AWAKE;
        return sleep_this_status;
    }

    Sleep_Structure.Data_Cnt++;

    if (Sleep_Structure.Data_Cnt == m_cnt) {
        Sleep_Structure.Data_Cnt = 0;
        isTurn_Judge();
        A_Calculate();
        Sleep_Structure.Minute_Cnt++;
        if (Sleep_Structure.Minute_Cnt >= 1000) {
            Sleep_Structure.Minute_Cnt = 0;
        }

        if (Sleep_Structure.Minute_Cnt > 4) {
            sleep_last_status = sleep_this_status;
            sleep_Mode_Calculate();
            if (sleep_last_status != sleep_this_status) {
                return sleep_this_status;
            }
        }
    }
    return sleep_this_status;
}


void sleep_Para_Clear(void)
{
    Sleep_Structure.Sleep_End_Flag = 0;
    Sleep_Structure.Data_Cnt = 0;
    Sleep_Structure.Minute_Cnt = 0;
    memset(Sleep_Structure.accSquareSum, 0, sizeof(Sleep_Structure.accSquareSum));
    memset(Sleep_Structure.A, 0, sizeof(Sleep_Structure.A));

}

/************************ (C) COPYRIGHT MEMSENSING *****END OF FILE****/
#endif

