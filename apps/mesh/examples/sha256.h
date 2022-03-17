#ifndef SHA256_H
#define SHA256_H

#define SHA256_DIGEST_SIZE  32
#define SHA256_BLOCK_SIZE   64

extern int sha256Compute(const void *data, int length, unsigned char *digest);

#endif
