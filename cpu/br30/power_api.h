#ifndef __POWER_API_H__
#define __POWER_API_H__

#include "typedef.h"

/**************************************
 *  MACRO
 *************************************/
enum {
    PWR_WK_REASON_PLUSE_CNT_OVERFLOW = 0,   /*!< 脉冲计数溢出 */
    PWR_WK_REASON_EDGE_INDEX0,      /* 边沿唤醒 索引 0*/
    PWR_WK_REASON_EDGE_INDEX1,      /* 边沿唤醒 索引 1*/
    PWR_WK_REASON_EDGE_INDEX2,      /* 边沿唤醒 索引 2*/
    PWR_WK_REASON_EDGE_INDEX3,      /* 边沿唤醒 索引 3*/
    PWR_WK_REASON_EDGE_INDEX4,      /* 边沿唤醒 索引 4*/
    PWR_WK_REASON_EDGE_INDEX5,      /* 边沿唤醒 索引 5*/
    PWR_WK_REASON_EDGE_INDEX6,      /* 边沿唤醒 索引 6*/
    PWR_WK_REASON_EDGE_INDEX7,      /* 边沿唤醒 索引 7*/
};

#endif
