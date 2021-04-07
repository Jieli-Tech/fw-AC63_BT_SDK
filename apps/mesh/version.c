#include "system/includes.h"
#include "generic/log.h"

int lib_media_version(void)
{
    return 0;
}

extern char __VERSION_BEGIN[];
extern char __VERSION_END[];

_INLINE_
int app_version_check()
{
    char *version;

    puts("=================Version===============\n");
    for (version = __VERSION_BEGIN; version < __VERSION_END;) {
        version += 4;
        printf("%s\n", version);
        version += strlen(version) + 1;
    }
    puts("=======================================\n");

    return 0;
}

