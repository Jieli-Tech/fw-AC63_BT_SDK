#include "system/includes.h"
#include "generic/log.h"

int app_version_check()
{
    lib_version_check();

#ifdef CONFIG_FATFS_ENBALE
    VERSION_CHECK(fatfs, FATFS_VERSION);
#endif

#ifdef JLFS_VERSION
    VERSION_CHECK(jlfs, JLFS_VERSION);
#endif

    return 0;
}

