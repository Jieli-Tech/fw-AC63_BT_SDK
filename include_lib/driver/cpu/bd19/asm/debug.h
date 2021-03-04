#ifndef __DEBUG_H__
#define __DEBUG_H__




#define CDBG_IDx(n, id) ((1<<(n+4)) | (id<<(n*8+8)))
#define CDBG_INV         (1<<7)
#define CDBG_PEN         (1<<3)
#define CDBG_XEN         (1<<2)
#define CDBG_WEN         (1<<1)
#define CDBG_REN         (1<<0)

void debug_init();
void exception_analyze();

/********************************** DUBUG SFR *****************************************/

u32 get_dev_id(char *name);

void mpu_set(int idx, u32 begin, u32 end, u32 inv, const char *format, ...);


#endif


