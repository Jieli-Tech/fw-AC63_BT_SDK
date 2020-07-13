#ifndef _FILE_OPERATE_H
#define _FILE_OPERATE_H

#include "system/fs/fs.h"
#include "storage_dev/storage_dev.h"

enum {
    FOPR_OK = 0,
    FOPR_ERR_POINT = (('F' << 24) | ('O' << 16) | ('\0' << 8) | '\0'),
    FOPR_ERR_DEV_NO_FIND,
    FOPR_ERR_DEV_MOUNT,
    FOPR_ERR_MALLOC,
    FOPR_ERR_FSCAN,
    FOPR_ERR_FSEL,
    FOPR_ERR_NO_FILE,
};

enum {
    DEV_SEL_CUR,
    DEV_SEL_NEXT,
    DEV_SEL_PREV,
    DEV_SEL_FIRST,
    DEV_SEL_LAST,
    DEV_SEL_SPEC,
    DEV_SEL_LAST_ACTIVE,
};

typedef struct _FILE_OPR_SEL_DEV_ {
    u8 dev_sel;
    int sel_param;
} FILE_OPR_SEL_DEV;

typedef struct _FILE_OPR_SEL_FILE_ {
    u8 file_sel; // FSEL_** /system/fs/fs.h
    u8 cycle_mode; // FCYCLE_** /system/fs/fs.h
    int sel_param;
    void *scan_type;
    char *path;
    int path_len;
} FILE_OPR_SEL_FILE;

typedef struct _FILE_OPERATE_ {
    struct storage_dev *dev;
    struct vfscan *fsn;
    char *fsn_path;
    FILE *file;
    FILE *lrc_file;
    int totalfile;
    u8 is_recfolder;
    u8 sel_flag;//sel dev/file
} FILE_OPERATE;

struct storage_dev *file_opr_available_dev_check(void *logo);
struct storage_dev *file_opr_available_dev_get_last_active(void);
int file_opr_available_dev_total(void);

int app_storage_dev_add(void *logo);
int app_storage_dev_del(void *logo);
int file_opr_dev_total(void);
int file_opr_available_dev_offline(struct storage_dev *p);
int file_opr_available_dev_online(struct storage_dev *p);
struct storage_dev *file_opr_dev_check(void *logo);
struct storage_dev *file_opr_dev_get_last_active(void);

int file_opr_api_scan_init(FILE_OPERATE *fopr, FILE_OPR_SEL_FILE *sel);

int file_opr_api_sel_dev(FILE_OPERATE *fopr, FILE_OPR_SEL_DEV *sel);
int file_opr_api_sel_file(FILE_OPERATE *fopr, FILE_OPR_SEL_FILE *sel);

int file_opr_api_set_cycle_mode(FILE_OPERATE *fopr, u8 cycle_mode);

FILE_OPERATE *file_opr_api_create(void);
void file_opr_api_release(FILE_OPERATE *fopr);

FS_DISP_INFO *file_opr_get_disp_info(void);

const char *evt2dev_map_logo(u32 evt);

void file_opr_api_set_recplay_status(FILE_OPERATE *fopr, u8 status);
void file_opr_api_set_sel_status(FILE_OPERATE *fopr, u8 status);
u8 file_opr_api_get_recplay_status(FILE_OPERATE *fopr);
u8 file_opr_api_get_sel_status(FILE_OPERATE *fopr);

#endif

