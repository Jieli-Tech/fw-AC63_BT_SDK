#ifndef MathFunc_fix_H
#define MathFunc_fix_H

#define PRECISION 3
#define FLOAT2INT(x, q) (int)((x) * (1 << q))
static const int INV_AQ = 28;
static const int INV_BQ = 28;

static const int INV_A[4] = {
    FLOAT2INT(1.207484775458747, INV_AQ),
    FLOAT2INT(1.207497010784896, INV_AQ),
    FLOAT2INT(1.207497067013359, INV_AQ),
    FLOAT2INT(1.207497067760144, INV_AQ),
};

static const int INV_B[4] = {
    FLOAT2INT(0.607259112298893, INV_BQ),
    FLOAT2INT(0.607252959138945, INV_BQ),
    FLOAT2INT(0.607252935103139, INV_BQ),
    FLOAT2INT(0.607252935009250, INV_BQ)
};


__attribute__((noinline))
void sin_fix(int a, int aQ, int rQ, int *r)
{
    long long tmp64_1;
    const int sel = (0x00 << 2) | (PRECISION);
    const int InDiffQ = 24 - aQ;
    const int InDiffrQ = rQ - 24;
    //rQ = 24;
    asm volatile(
        " %0.l = %4 >< %6 (sat) \n\t"
        " %0 = copex(%0)(%5) \n\t"
        " %0.l = %0.l >< %7 (sat)\n\t"
        " [%1] = %0.l \n\t"
        : "=&r"(tmp64_1),
        "=&r"(r)
        : "0"(tmp64_1),
        "1"(r),
        "r"(a),
        "i"(sel),
        "r"(InDiffQ),
        "r"(InDiffrQ)
        :
    );
}

__attribute__((noinline))
void cos_fix(int a, int aQ, int rQ, int *r)
{
    long long tmp64_1;
    const int sel = (0x00 << 2) | (PRECISION);
    const int InDiffQ = 24 - aQ;
    const int InDiffrQ = rQ - 24;
    //rQ = 24;
    asm volatile(
        " %0.l = %4 >< %6 (sat) \n\t"
        " %0 = copex(%0)(%5) \n\t"
        " %0.h = %0.h >< %7 (sat)\n\t"
        " [%1] = %0.h \n\t"
        : "=&r"(tmp64_1),
        "=&r"(r)
        : "0"(tmp64_1),
        "1"(r),
        "r"(a),
        "i"(sel),
        "r"(InDiffQ),
        "r"(InDiffrQ)
        :
    );
}

__attribute__((noinline))
void arctan_fix(int x, int xQ, int y, int yQ, int rQ, int *r)
{
    int tmp32_1;
    long long tmp64;
    const int sel = (0x01 << 2) | (PRECISION);
    const int InDiffrQ = rQ - 24;
    //*rQ = 24;

    asm volatile(
        " %1 = %7 - %8 \n\t"
        " %0.h =  %5 >< %1 (sat)\n\t"
        " %0.l = %6  \n\t"
        " %0 = copex(%0)(%9) \n\t"
        " %0.l = %0.l >< %10 (sat) \n\t"
        " %2 = %0.l \n\t"
        : "=&r"(tmp64),
        "=&r"(tmp32_1),
        "=r"(*r)
        : "0"(tmp64),
        "1"(tmp32_1),
        "r"(y),
        "r"(x),
        "r"(xQ),
        "r"(yQ),
        "i"(sel),
        "r"(InDiffrQ)
        :
    );
}

__attribute__((noinline))
void arctanh_fix(int x, int xQ, int y, int yQ, int rQ, int *r)
{
    int tmp32_1;
    long long tmp64;
    const int sel = (0x04 << 2) | (PRECISION);
    const int InDiffrQ = rQ - 24;
    //*rQ = 24;
    asm volatile(
        " %1 = %7 - %8 \n\t"
        " %0.h =  %5 >< %1 \n\t"
        " %0.l = %6  \n\t"
        " %0 = copex(%0)(%9) \n\t"
        " %0.l = %0.l >< %10 (sat)\n\t"
        " %2 = %0.l  \n\t"
        : "=&r"(tmp64),
        "=&r"(tmp32_1),
        "=r"(*r)
        : "0"(tmp64),
        "1"(tmp32_1),
        "r"(y),
        "r"(x),
        "r"(xQ),
        "r"(yQ),
        "i"(sel),
        "r"(InDiffrQ)
        :
    );
}


__attribute__((noinline))
void sinh_fix(int x, int xQ, int rQ, int *r)
{
    long long tmp64;
    const int sel = (0x02 << 2) | (PRECISION);
    const int InDiffQ = 24 - xQ;
    const int InDiffrQ = rQ - 24;
    //*rQ = 24;
    asm volatile(
        " %0.l = %4 >< %7(sat)	\n\t"
        " %0 = copex(%0)(%6)		\n\t"
        " %0.l = %0.l >< %8 (sat)\n\t"
        " [%1] = %0.l	\n\t"
        : "=&r"(tmp64),
        "=&r"(r)
        : "0"(tmp64),
        "1"(r),
        "r"(x),
        "r"(r),
        "i"(sel),
        "r"(InDiffQ),
        "r"(InDiffrQ)
        :
    );
}

__attribute__((noinline))
void cosh_fix(int x, int xQ, int rQ, int *r)
{
    long long tmp64;
    const int sel = (0x02 << 2) | (PRECISION);
    const int InDiffQ = 24 - xQ;
    const int InDiffrQ = rQ - 24;
    //*rQ = 24;
    asm volatile(
        " %0.l = %4 >< %6(sat)	\n\t"
        " %0 = copex(%0)(%5)		\n\t"
        " %0.h = %0.h >< %7 (sat)\n\t"
        " [%1] = %0.h	\n\t"
        : "=&r"(tmp64),
        "=&r"(r)
        : "0"(tmp64),
        "1"(r),
        "r"(x),
        "i"(sel),
        "r"(InDiffQ),
        "r"(InDiffrQ)
        :
    );
}

__attribute__((noinline))
void exp_fix(int a, int aQ, int rQ, int *r)
{
    int tmp32_1, tmp32_2, rQ_1;
    long long tmp64_1;
    const int sel = (0x03 << 2) | (PRECISION);
    const int TargetQ = 28 - 63;
    const int InDiffQ = 24 - aQ;

    asm volatile(
        " %0.l = %10 >< %11 (sat) \n\t"
        " %0.h = 0 \n\t"
        " %0 = copex(%0)(%12) \n\t"
        " %1 = uextra(%0.h,p:26,l:6) \n\t"
        " %0.h = sextra(%0.h,p:0,l:26) \n\t"
        " %0 = %0.l*%0.h (s) \n\t"
        " %2 = clz(%0.h) \n\t"
        " ifs(%2 == 32) { \n\t"
        " 	%3 = clz(%0.l)  \n\t"
        " 	%2 += %3  \n\t"
        " } \n\t"
        " %2 += %13  \n\t"
        " %0 ><= %2 (sat) \n\t"
        " %1 += 24  \n\t"
        " %1 += %2  \n\t"
        " %2 = %9 - %1\n\t"
        " %0.l ><= %2 (sat) \n\t"
        " %4 = %0.l \n\t"
        : "=&r"(tmp64_1),
        "=&r"(rQ_1),
        "=&r"(tmp32_1),
        "=&r"(tmp32_2),
        "=r"(*r)
        : "0"(tmp64_1),
        "1"(rQ_1),
        "2"(tmp32_1),
        "3"(tmp32_2),
        "r"(rQ),
        "r"(a),
        "r"(InDiffQ),
        "i"(sel),
        "i"(TargetQ)
        :
    );
}

__attribute__((noinline))
void log_fix(int a, int aQ, int rQ, int *r)
{
    if (a == 0) {
        *r = -231785805; //log(1e-6)
        return;
    }
    long long tmp64_1;
    const int sel = (0x05 << 2) | (PRECISION);
    const int InDiffQ = 24 - aQ;
    const int InDiffrQ = rQ - 24;
    //*rQ = 24;
    asm volatile(
        " %0.h = %4 \n\t"
        " %0.l = %3 \n\t"
        " %0 = copex(%0)(%6) \n\t"
        " %0.l = %0.l >< %5 (sat) \n\t"
        " %1 = %0.l \n\t"
        : "=&r"(tmp64_1),
        "=r"(*r)
        : "0"(tmp64_1),
        "r"(a),
        "r"(InDiffQ),
        "r"(InDiffrQ),
        "i"(sel)
        :
    );
}

__attribute__((noinline))
void sqrt_fix(int a, int aQ, int rQ, int *r)
{
    int tmp32_1, rQ_1;
    const int sqrt2Q = 28;
    const int sqrt2 = (int)(0.7071f * (1 << sqrt2Q));
    long long tmp64_1;
    const int sel = (0x06 << 2) | (PRECISION);
    const int invA = INV_A[PRECISION];
    const int cordic_sqrt_outQ = 15;

    asm volatile(
        " %0.l = %12 \n\t"
        " %1 = %13 \n\t"
        " ifs(%1<0) { \n\t"
        " 	%1 = -%1 \n\t"
        " } \n\t"
        " %0 = copex(%0)(%11) \n\t"
        " if((%1 & 0x1)==0) goto 1f \n\t"
        " ;{ \n\t"
        " 	%0 = %0.l * %6(s) \n\t"
        " 	%0.l = %0 >> %7 (s) \n\t"
        " ;} \n\t"
        " 1: \n\t"
        " %0 = %0.l*%8(s) \n\t"
        " %1 = %13 >>> 1 \n\t"
        " %3 = %1 + %10 \n\t"
        " %1 = %14 - %3\n\t"
        " %2 = %0 >> %9 (s)\n\t"
        " %2 = %2 ><  %1 (sat) \n\t"

        : "=&r"(tmp64_1),
        "=&r"(tmp32_1),
        "=r"(*r),
        "=r"(rQ_1)
        : "0"(tmp64_1),
        "1"(tmp32_1),
        "r"(sqrt2),
        "i"(sqrt2Q),
        "r"(invA),
        "i"(INV_AQ),
        "i"(cordic_sqrt_outQ),
        "i"(sel),
        "r"(a),
        "r"(aQ),
        "r"(rQ)
        :
    );
}

__attribute__((noinline))
void SRSS_fix(int x, int xQ, int y, int yQ, int rQ, int *r)
{
    int tmp32;
    long long tmp64;
    const int sel = (0x01 << 2) | (PRECISION);
    const int invB = INV_B[PRECISION];

    asm volatile(
        " %1 = %8 - %9  \n\t"
        " %1 = %7 >< %1(sat) \n\t"
        " %0.h = %1 \n\t"
        " %0.l = %6 \n\t"
        " %0 = copex(%0)(%12) \n\t"
        " %1 = %10 - %8 \n\t"
        " %0 = %0.h * %11(s) \n\t"
        " %2 = %0 >> %13(s) \n\t"
        " %2 = %2 >< %1 (sat) \n\t"
        : "=&r"(tmp64),
        "=&r"(tmp32),
        "=&r"(*r)
        : "0"(tmp64),
        "1"(tmp32),
        "2"(*r),
        "r"(x),
        "r"(y),
        "r"(xQ),
        "r"(yQ),
        "r"(rQ),
        "r"(invB),
        "i"(sel),
        "i"(INV_BQ)
        :
    );
}

__attribute__((noinline))
void SDS_fix(int x, int xQ, int y, int yQ, int rQ, int *r)
{
    int tmp32;
    long long tmp64;
    const int sel = (0x04 << 2) | (PRECISION);
    const int invA = INV_A[PRECISION];
    const int InDiffxQ = 24 - xQ;
    const int InDiffyQ = 24 - yQ;
    const int InDiffrQ = rQ - 24;
    //*rQ = 24;

    asm volatile(
        "%1 = %5 >< %7(sat) \n\t"
        "%0.l = %1 \n\t"
        "%1 = %6 >< %8(sat)	\n\t"
        "%0.h = %1 \n\t"
        "%0 = copex(%0)(%11) \n\t"
        "%0 = %0.h * %10(s) \n\t"
        "%2 = %0 >> %12(s) \n\t"
        "%2 = %2 >< %9 (sat) \n\t"
        : "=&r"(tmp64),
        "=&r"(tmp32),
        "=r"(*r)
        : "0"(tmp64),
        "1"(tmp32),
        "r"(x),
        "r"(y),
        "r"(InDiffxQ),
        "r"(InDiffyQ),
        "r"(InDiffrQ),
        "r"(invA),
        "i"(sel),
        "i"(INV_AQ)
        :
    );
}

#endif/*MathFunc_fix_H*/
