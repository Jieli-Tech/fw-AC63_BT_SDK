#ifndef MathFunc_float_H
#define MathFunc_float_H

#include "MathFunc_fix.h"

__attribute__((noinline))
void sin_float(float a, float *r)
{
    int Q, aQ, rQ, tmp_a, r1, tmp32_1, tmp32_2;
    float tmp1;
    Q = 150;
    rQ = 24;
    tmp32_1 = (int)(a / 2.f);
    tmp1 = (float)tmp32_1;
    a = a - tmp1 * 2.f;
    asm volatile(
        "%0 = %6 \n\t"
        "%1 = uextra(%0, p:0, l:23) \n\t"
        "%1 |= (1<<23) \n\t"
        "%0 = %0 >>> 31 \n\t"
        "ifs(%0 != 0) { \n\t"
        "	%1 = - %1 \n\t"
        "} \n\t"
        "%2 = %1 \n\t"
        "%1 = uextra(%6, p:23, l:8) \n\t"
        " if(%1 == 0){ \n\t"
        " 	%2 = 0 \n\t"
        " } \n\t"
        "%3 = %7 - %1 \n\t"
        //"%3 = %7 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(tmp32_2),
        "=&r"(tmp_a),
        "=&r"(aQ)
        : "0"(tmp32_1),
        "1"(tmp32_2),
        "r"(a),
        "r"(Q)
        :);

    // call fix_function
    sin_fix(tmp_a, aQ, rQ, &r1);

    // int to float
    asm volatile(
        // "%0 = tmp.Data \n\t"
        "%0 = itof(%1) \n\t"
        "%1 = uextra(%0, p:23, l:8) \n\t"
        "%1 = %1 - %5 \n\t"
        "%0 <= insert(%1, p:23, l:8) \n\t"
        "%2 = %0 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(r1),
        "=r"(*r)
        : "0"(tmp32_1),
        "1"(r1),
        "r"(rQ)
        :);
}

__attribute__((noinline))
void cos_float(float a, float *r)
{
    int Q, aQ, rQ, tmp_a, r1, tmp32_1, tmp32_2;
    float tmp1;
    Q = 150;
    rQ = 24;

    tmp32_1 = (int)(a / 2.f);
    tmp1 = (float)tmp32_1;
    a = a - tmp1 * 2.f;
    asm volatile(
        "%0 = %6 \n\t"
        "%1 = uextra(%0, p:0, l:23) \n\t"
        "%1 |= (1<<23) \n\t"
        "%0 = %0 >>> 31 \n\t"
        "ifs(%0 != 0) { \n\t"
        "	%1 = - %1 \n\t"
        "} \n\t"
        "%2 = %1 \n\t"
        "%1 = uextra(%6, p:23, l:8) \n\t"
        " if(%1 == 0){ \n\t"
        " 	%2 = 0 \n\t"
        " } \n\t"
        "%3 = %7 - %1 \n\t"
        //"%3 = %7 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(tmp32_2),
        "=&r"(tmp_a),
        "=&r"(aQ)
        : "0"(tmp32_1),
        "1"(tmp32_2),
        "r"(a),
        "r"(Q)
        :);

    // call fix_function
    cos_fix(tmp_a, aQ, rQ, &r1);

    // int to float
    asm volatile(
        // "%0 = tmp.Data \n\t"
        "%0 = itof(%1) \n\t"
        "%1 = uextra(%0, p:23, l:8) \n\t"
        "%1 = %1 - %5 \n\t"
        "%0 <= insert(%1, p:23, l:8) \n\t"
        "%2 = %0 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(r1),
        "=r"(*r)
        : "0"(tmp32_1),
        "1"(r1),
        "r"(rQ)
        :);
}

__attribute__((noinline))
void arctan_float(float x, float y, float *r)
{
    int Q, xQ, yQ, rQ, tmp_x, tmp_y, r1, tmp32_1, tmp32_2;
    Q = 150;
    rQ = 24;
    asm volatile(
        "%0 = %8 \n\t"
        "%1 = uextra(%0, p:0, l:23) \n\t"
        "%1 |= (1<<23) \n\t"
        "%0 = %0 >>> 31 \n\t"
        "ifs(%0 != 0) { \n\t"
        "	%1 = - %1 \n\t"
        "} \n\t"
        "%2 = %1 \n\t"
        "%1 = uextra(%8, p:23, l:8) \n\t"
        " if(%1 == 0){ \n\t"
        " 	%2 = 0 \n\t"
        " } \n\t"
        "%4 = %10 - %1 \n\t"
        "%0 = %9 \n\t"
        "%1 = uextra(%0, p:0, l:23) \n\t"
        "%1 |= (1<<23) \n\t"
        "%0 = %0 >>> 31 \n\t"
        "ifs(%0 != 0) { \n\t"
        "	%1 = - %1 \n\t"
        "} \n\t"
        "%3 = %1 \n\t"
        "%1 = uextra(%9, p:23, l:8) \n\t"
        " if(%1 == 0){ \n\t"
        " 	%3 = 0 \n\t"
        " } \n\t"
        "%5 = %10 - %1 \n\t"
        //"aQ = %10 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(tmp32_2),
        "=&r"(tmp_x),
        "=&r"(tmp_y),
        "=&r"(xQ),
        "=&r"(yQ)
        : "0"(tmp32_1),
        "1"(tmp32_2),
        "r"(x),
        "r"(y),
        "r"(Q)
        :);

    // call fix_function
    arctan_fix(tmp_x, xQ, tmp_y, yQ, rQ, &r1);

    // int to float
    asm volatile(
        // "%0 = tmp.Data \n\t"
        "%0 = itof(%1) \n\t"
        "%1 = uextra(%0, p:23, l:8) \n\t"
        "%1 = %1 - %5 \n\t"
        "%0 <= insert(%1, p:23, l:8) \n\t"
        "%2 = %0 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(r1),
        "=r"(*r)
        : "0"(tmp32_1),
        "1"(r1),
        "r"(rQ)
        :);
}

__attribute__((noinline))
void arctanh_float(float x, float y, float *r)
{
    int Q, xQ, yQ, rQ, tmp_x, tmp_y, r1, tmp32_1, tmp32_2;
    Q = 150;
    rQ = 24;
    asm volatile(
        "%0 = %8 \n\t"
        "%1 = uextra(%0, p:0, l:23) \n\t"
        "%1 |= (1<<23) \n\t"
        "%0 = %0 >>> 31 \n\t"
        "ifs(%0 != 0) { \n\t"
        "	%1 = - %1 \n\t"
        "} \n\t"
        "%2 = %1 \n\t"
        "%1 = uextra(%8, p:23, l:8) \n\t"
        " if(%1 == 0){ \n\t"
        " 	%2 = 0 \n\t"
        " } \n\t"
        "%4 = %10 - %1 \n\t"
        "%0 = %9 \n\t"
        "%1 = uextra(%0, p:0, l:23) \n\t"
        "%1 |= (1<<23) \n\t"
        "%0 = %0 >>> 31 \n\t"
        "ifs(%0 != 0) { \n\t"
        "	%1 = - %1 \n\t"
        "} \n\t"
        "%3 = %1 \n\t"
        "%1 = uextra(%9, p:23, l:8) \n\t"
        " if(%1 == 0){ \n\t"
        " 	%3 = 0 \n\t"
        " } \n\t"
        "%5 = %10 - %1 \n\t"
        //"aQ = %10 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(tmp32_2),
        "=&r"(tmp_x),
        "=&r"(tmp_y),
        "=&r"(xQ),
        "=&r"(yQ)
        : "0"(tmp32_1),
        "1"(tmp32_2),
        "r"(x),
        "r"(y),
        "r"(Q)
        :);

    // call fix_function
    arctanh_fix(tmp_x, xQ, tmp_y, yQ, rQ, &r1);

    // int to float
    asm volatile(
        // "%0 = tmp.Data \n\t"
        "%0 = itof(%1) \n\t"
        "%1 = uextra(%0, p:23, l:8) \n\t"
        "%1 = %1 - %5 \n\t"
        "%0 <= insert(%1, p:23, l:8) \n\t"
        "%2 = %0 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(r1),
        "=r"(*r)
        : "0"(tmp32_1),
        "1"(r1),
        "r"(rQ)
        :);
}

__attribute__((noinline))
void sinh_float(float a, float *r)
{
    int Q, aQ, rQ, tmp_a, r1, tmp32_1, tmp32_2;
    Q = 150;
    rQ = 24;
    asm volatile(
        "%0 = %6 \n\t"
        "%1 = uextra(%0, p:0, l:23) \n\t"
        "%1 |= (1<<23) \n\t"
        "%0 = %0 >>> 31 \n\t"
        "ifs(%0 != 0) { \n\t"
        "	%1 = - %1 \n\t"
        "} \n\t"
        "%2 = %1 \n\t"
        "%1 = uextra(%6, p:23, l:8) \n\t"
        " if(%1 == 0){ \n\t"
        " 	%2 = 0 \n\t"
        " } \n\t"
        "%3 = %7 - %1 \n\t"
        //"%3 = %7 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(tmp32_2),
        "=&r"(tmp_a),
        "=&r"(aQ)
        : "0"(tmp32_1),
        "1"(tmp32_2),
        "r"(a),
        "r"(Q)
        :);

    // call fix_function
    sinh_fix(tmp_a, aQ, rQ, &r1);

    // int to float
    asm volatile(
        // "%0 = tmp.Data \n\t"
        "%0 = itof(%1) \n\t"
        "%1 = uextra(%0, p:23, l:8) \n\t"
        "%1 = %1 - %5 \n\t"
        "%0 <= insert(%1, p:23, l:8) \n\t"
        "%2 = %0 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(r1),
        "=r"(*r)
        : "0"(tmp32_1),
        "1"(r1),
        "r"(rQ)
        :);
}

__attribute__((noinline))
void cosh_float(float a, float *r)
{
    int Q, aQ, rQ, tmp_a, r1, tmp32_1, tmp32_2;
    Q = 150;
    rQ = 24;
    asm volatile(
        "%0 = %6 \n\t"
        "%1 = uextra(%0, p:0, l:23) \n\t"
        "%1 |= (1<<23) \n\t"
        "%0 = %0 >>> 31 \n\t"
        "ifs(%0 != 0) { \n\t"
        "	%1 = - %1 \n\t"
        "} \n\t"
        "%2 = %1 \n\t"
        "%1 = uextra(%6, p:23, l:8) \n\t"
        " if(%1 == 0){ \n\t"
        " 	%2 = 0 \n\t"
        " } \n\t"
        "%3 = %7 - %1 \n\t"
        //"%3 = %7 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(tmp32_2),
        "=&r"(tmp_a),
        "=&r"(aQ)
        : "0"(tmp32_1),
        "1"(tmp32_2),
        "r"(a),
        "r"(Q)
        :);

    // call fix_function
    cosh_fix(tmp_a, aQ, rQ, &r1);

    // int to float
    asm volatile(
        // "%0 = tmp.Data \n\t"
        "%0 = itof(%1) \n\t"
        "%1 = uextra(%0, p:23, l:8) \n\t"
        "%1 = %1 - %5 \n\t"
        "%0 <= insert(%1, p:23, l:8) \n\t"
        "%2 = %0 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(r1),
        "=r"(*r)
        : "0"(tmp32_1),
        "1"(r1),
        "r"(rQ)
        :);
}

__attribute__((noinline))
void exp_float(float a, float *r)
{
    int Q, aQ, rQ, tmp_a, r1, tmp32_1, tmp32_2;
    Q = 150;
    rQ = 24;
    asm volatile(
        "%0 = %6 \n\t"
        "%1 = uextra(%0, p:0, l:23) \n\t"
        "%1 |= (1<<23) \n\t"
        "%0 = %0 >>> 31 \n\t"
        "ifs(%0 != 0) { \n\t"
        "	%1 = - %1 \n\t"
        "} \n\t"
        "%2 = %1 \n\t"
        "%1 = uextra(%6, p:23, l:8) \n\t"
        " if(%1 == 0){ \n\t"
        " 	%2 = 0 \n\t"
        " } \n\t"
        "%3 = %7 - %1 \n\t"
        //"%3 = %3 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(tmp32_2),
        "=&r"(tmp_a),
        "=&r"(aQ)
        : "0"(tmp32_1),
        "1"(tmp32_2),
        "r"(a),
        "r"(Q)
        :);

    // call fix_function
    exp_fix(tmp_a, aQ, rQ, &r1);

    // int to float
    asm volatile(
        // "%0 = tmp.Data \n\t"
        "%0 = itof(%1) \n\t"
        "%1 = uextra(%0, p:23, l:8) \n\t"
        "%1 = %1 - %5 \n\t"
        "%0 <= insert(%1, p:23, l:8) \n\t"
        "%2 = %0 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(r1),
        "=r"(*r)
        : "0"(tmp32_1),
        "1"(r1),
        "r"(rQ)
        :);
}

__attribute__((noinline))
void log_float(float a, float *r)
{
    if (a == 0) {
        *r = -231785805; //log(1e-6)
        return;
    }

    int Q, aQ, rQ, tmp_a, r1, tmp32_1, tmp32_2;
    Q = 150;
    rQ = 24;
    asm volatile(
        "%0 = %6 \n\t"
        "%1 = uextra(%0, p:0, l:23) \n\t"
        "%1 |= (1<<23) \n\t"
        "%0 = %0 >>> 31 \n\t"
        "ifs(%0 != 0) { \n\t"
        "	%1 = - %1 \n\t"
        "} \n\t"
        "%2 = %1 \n\t"
        "%1 = uextra(%6, p:23, l:8) \n\t"
        " if(%1 == 0){ \n\t"
        " 	%2 = 0 \n\t"
        " } \n\t"
        "%3 = %7 - %1 \n\t"
        //"%3 = %7 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(tmp32_2),
        "=&r"(tmp_a),
        "=&r"(aQ)
        : "0"(tmp32_1),
        "1"(tmp32_2),
        "r"(a),
        "r"(Q)
        :);

    // call fix_function
    log_fix(tmp_a, aQ, rQ, &r1);

    // int to float
    asm volatile(
        "%0 = itof(%1) \n\t"
        "%1 = uextra(%0, p:23, l:8) \n\t"
        "%1 = %1 - %5 \n\t"
        "%0 <= insert(%1, p:23, l:8) \n\t"
        "%2 = %0 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(r1),
        "=r"(*r)
        : "0"(tmp32_1),
        "1"(r1),
        "r"(rQ)
        :);
}

__attribute__((noinline))
void sqrt_float(float a, float *r)
{
    int Q, aQ, rQ, tmp_a, r1, tmp32_1, tmp32_2;
    Q = 150;
    rQ = 24;
    asm volatile(
        "%0 = %6 \n\t"
        "%1 = uextra(%0, p:0, l:23) \n\t"
        "%1 |= (1<<23) \n\t"
        "%0 = %0 >>> 31 \n\t"
        "ifs(%0 != 0) { \n\t"
        "	%1 = - %1 \n\t"
        "} \n\t"
        "%2 = %1 \n\t"
        "%1 = uextra(%6, p:23, l:8) \n\t"
        " if(%1 == 0){ \n\t"
        " 	%2 = 0 \n\t"
        " } \n\t"
        "%3 = %7 - %1 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(tmp32_2),
        "=&r"(tmp_a),
        "=&r"(aQ)
        : "0"(tmp32_1),
        "1"(tmp32_2),
        "r"(a),
        "r"(Q)
        :);

    // call fix_function
    sqrt_fix(tmp_a, aQ, rQ, &r1);

    // int to float
    asm volatile(
        // "%0 = tmp.Data \n\t"
        "%0 = itof(%1) \n\t"
        "%1 = uextra(%0, p:23, l:8) \n\t"
        "%1 = %1 - %5 \n\t"
        "%0 <= insert(%1, p:23, l:8) \n\t"
        "%2 = %0 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(r1),
        "=r"(*r)
        : "0"(tmp32_1),
        "1"(r1),
        "r"(rQ)
        :);
}

__attribute__((noinline))
void SRSS_float(float x, float y, float *r)    //sqrt(x^2 + y^2)
{
    int Q, xQ, yQ, rQ, tmp_x, tmp_y, r1, tmp32_1, tmp32_2;
    Q = 150;
    rQ = 10;
    asm volatile(
        "%0 = %8 \n\t"
        "%1 = uextra(%0, p:0, l:23) \n\t"
        "%1 |= (1<<23) \n\t"
        "%0 = %0 >>> 31 \n\t"
        "ifs(%0 != 0) { \n\t"
        "	%1 = - %1 \n\t"
        "} \n\t"
        "%2 = %1 \n\t"
        "%1 = uextra(%8, p:23, l:8) \n\t"
        " if(%1 == 0){ \n\t"
        " 	%2 = 0 \n\t"
        " } \n\t"
        "%4 = %10 - %1 \n\t"
        "%0 = %9 \n\t"
        "%1 = uextra(%0, p:0, l:23) \n\t"
        "%1 |= (1<<23) \n\t"
        "%0 = %0 >>> 31 \n\t"
        "ifs(%0 != 0) { \n\t"
        "	%1 = - %1 \n\t"
        "} \n\t"
        "%3 = %1 \n\t"
        "%1 = uextra(%9, p:23, l:8) \n\t"
        " if(%1 == 0){ \n\t"
        " 	%3 = 0 \n\t"
        " } \n\t"
        "%5 = %10 - %1 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(tmp32_2),
        "=&r"(tmp_x),
        "=&r"(tmp_y),
        "=&r"(xQ),
        "=&r"(yQ)
        : "0"(tmp32_1),
        "1"(tmp32_2),
        "r"(x),
        "r"(y),
        "r"(Q)
        :);

    // call fix_function
    SRSS_fix(tmp_x, xQ, tmp_y, yQ, rQ, &r1);

    // int to float
    asm volatile(
        // "%0 = tmp.Data \n\t"
        "%0 = itof(%1) \n\t"
        "%1 = uextra(%0, p:23, l:8) \n\t"
        "%1 = %1 - %5 \n\t"
        "%0 <= insert(%1, p:23, l:8) \n\t"
        "%2 = %0 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(r1),
        "=r"(*r)
        : "0"(tmp32_1),
        "1"(r1),
        "r"(rQ)
        :);
}

__attribute__((noinline))
void SDS_float(float x, float y, float *r)  //sqrt(x^2 - y^2)
{
    int Q, xQ, yQ, rQ, tmp_x, tmp_y, r1, tmp32_1, tmp32_2;
    Q = 150;
    rQ = 24;
    asm volatile(
        "%0 = %8 \n\t"
        "%1 = uextra(%0, p:0, l:23) \n\t"
        "%1 |= (1<<23) \n\t"
        "%0 = %0 >>> 31 \n\t"
        "ifs(%0 != 0) { \n\t"
        "	%1 = - %1 \n\t"
        "} \n\t"
        "%2 = %1 \n\t"
        "%1 = uextra(%8, p:23, l:8) \n\t"
        " if(%1 == 0){ \n\t"
        " 	%2 = 0 \n\t"
        " } \n\t"
        "%4 = %10 - %1 \n\t"
        "%0 = %9 \n\t"
        "%1 = uextra(%0, p:0, l:23) \n\t"
        "%1 |= (1<<23) \n\t"
        "%0 = %0 >>> 31 \n\t"
        "ifs(%0 != 0) { \n\t"
        "	%1 = - %1 \n\t"
        "} \n\t"
        "%3 = %1 \n\t"
        "%1 = uextra(%9, p:23, l:8) \n\t"
        " if(%1 == 0){ \n\t"
        " 	%2 = 0 \n\t"
        " } \n\t"
        "%5 = %10 - %1 \n\t"
        //"aQ = %10 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(tmp32_2),
        "=&r"(tmp_x),
        "=&r"(tmp_y),
        "=&r"(xQ),
        "=&r"(yQ)
        : "0"(tmp32_1),
        "1"(tmp32_2),
        "r"(x),
        "r"(y),
        "r"(Q)
        :);

    // call fix_function
    SDS_fix(tmp_x, xQ, tmp_y, yQ, rQ, &r1);

    // int to float
    asm volatile(
        // "%0 = tmp.Data \n\t"
        "%0 = itof(%1) \n\t"
        "%1 = uextra(%0, p:23, l:8) \n\t"
        "%1 = %1 - %5 \n\t"
        "%0 <= insert(%1, p:23, l:8) \n\t"
        "%2 = %0 \n\t"
        : "=&r"(tmp32_1),
        "=&r"(r1),
        "=r"(*r)
        : "0"(tmp32_1),
        "1"(r1),
        "r"(rQ)
        :);
}

__attribute__((noinline))
void arcsin_float(float x, float *r)  //arcsin(x) && x=[-1,1]
{
    float y, y_tmp, pi, rst;
    pi = 3.1415926;
    y_tmp = 1.0 - x * x;
    sqrt_float(y_tmp, &y);
    arctan_float(y, x, &rst);
    *r = rst * pi;
}

#endif/*MathFunc_float_H*/
