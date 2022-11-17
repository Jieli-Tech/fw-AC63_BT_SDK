#include <stdio.h>
#include <stdlib.h>

void *calloc(size_t count, size_t size)
{
    return zalloc(count * size);
}

void mbedtls_free(void *arg)
{
    free(arg);
}
void *mbedtls_calloc(int size, int count)
{
    return calloc(size, count);
}
