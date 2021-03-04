//*********************************************************************************//
// Module name : cache.h                                                           //
// Description : j32CPU_icu cache control head file                                    //
// By Designer : zequan_liu                                                        //
// Dat changed :                                                                   //
//*********************************************************************************//

#ifndef __j32CPU_icu_CACHE__
#define __j32CPU_icu_CACHE__

//------------------------------------------------------//
// icache function
//------------------------------------------------------//

void IcuEnable(void);
void IcuDisable(void);
void IcuInitial(void);

//void IcuFlushAll(void);
void IcuFlushinvAll(void);
//void IcuFlushRegion(int *beg, int len);                // note len!=0
void IcuFlushinvRegion(int *beg, int len);             // note len!=0

void IcuUnlockAll(void);
void IcuUnlockRegion(int *beg, int len);               // note len!=0
void IcuPfetchRegion(int *beg, int len);               // note len!=0
void IcuLockRegion(int *beg, int len);                 // note len!=0

void IcuReportEnable(void);
void IcuReportDisable(void);
void IcuReportPrintf(void);
void IcuReportClear(void);

void IcuEmuEnable(void);
void IcuEmuDisable(void);
void IcuEmuMessage(void);

#define WAIT_ICACHE_IDLE    do{asm volatile("csync"); \
                                 while(!(j32CPU_icu(core_num())->CON & BIT(31)));}  while(0);

//------------------------------------------------------//
// dcache function
//------------------------------------------------------//

void DcuEnable(void);
void DcuDisable(void);
void DcuInitial(void);

//void DcuFlushAll(void);
void DcuFlushinvAll(void);
//void DcuFlushRegion(int *beg, int len);                // note len!=0
void DcuFlushinvRegion(int *beg, int len);             // note len!=0

void DcuUnlockAll(void);
void DcuUnlockRegion(int *beg, int len);               // note len!=0
void DcuPfetchRegion(int *beg, int len);               // note len!=0
void DcuLockRegion(int *beg, int len);                 // note len!=0

void DcuReportEnable(void);
void DcuReportDisable(void);
void DcuReportPrintf(void);
void DcuReportClear(void);

void DcuEmuEnable(void);
void DcuEmuDisable(void);
void DcuEmuMessage(void);

#define WAIT_DCACHE_IDLE    do{asm volatile("csync"); while(!(JL_DCU->CON & BIT(31)));} while(0);

#endif

//*********************************************************************************//
//                                                                                 //
//                               end of this module                                //
//                                                                                 //
//*********************************************************************************//
