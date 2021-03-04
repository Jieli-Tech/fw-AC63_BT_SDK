#ifndef __FS_H__
#define __FS_H__



#include "generic/typedef.h"
#include "generic/list.h"
#include "generic/ioctl.h"
#include "generic/atomic.h"
#include "system/task.h"
#include "system/malloc.h"
#include "system/sys_time.h"
#include "stdarg.h"
#include "fs_file_name.h"

#define SEEK_SET	0	/* Seek from beginning of file.  */
#define SEEK_CUR	1	/* Seek from current position.  */
#define SEEK_END	2	/* Seek from end of file.  */

#include "sdfile.h"

#define F_ATTR_RO       0x01
#define F_ATTR_ARC      0x02
#define F_ATTR_DIR      0x04
#define F_ATTR_VOL      0x08

#if (VFS_ENABLE == 1)

#ifndef FSELECT_MODE
#define FSELECT_MODE
#define    FSEL_FIRST_FILE     			 0
#define    FSEL_LAST_FILE      			 1
#define    FSEL_NEXT_FILE      			 2
#define    FSEL_PREV_FILE      			 3
#define    FSEL_CURR_FILE      			 4
#define    FSEL_BY_NUMBER      			 5
#define    FSEL_BY_SCLUST      			 6
#define    FSEL_AUTO_FILE      			 7
#define    FSEL_NEXT_FOLDER_FILE       	 8
#define    FSEL_PREV_FOLDER_FILE         9
#define    FSEL_BY_PATH                 10
#endif

#ifndef FCYCLE_MODE
#define FCYCLE_MODE
#define    FCYCLE_LIST			0
#define    FCYCLE_ALL			1
#define    FCYCLE_ONE			2
#define    FCYCLE_FOLDER		3
#define    FCYCLE_RANDOM		4
#define    FCYCLE_MAX			5
#endif

enum {
    FS_IOCTL_GET_FILE_NUM,
    FS_IOCTL_FILE_CHECK,
    FS_IOCTL_GET_ERR_CODE,   //暂不支持
    FS_IOCTL_FREE_CACHE,
    FS_IOCTL_SET_NAME_FILTER,  //设置文件过滤
    FS_IOCTL_GET_FOLDER_INFO,  //获取文件夹序号和文件夹内文件数目
    FS_IOCTL_SET_LFN_BUF,	// 512
    FS_IOCTL_SET_LDN_BUF,	// 512

    FS_IOCTL_SET_EXT_TYPE,  //设置后缀类型
    FS_IOCTL_OPEN_DIR,    //打开目录
    FS_IOCTL_ENTER_DIR,   //进入目录
    FS_IOCTL_EXIT_DIR,    //退出
    FS_IOCTL_GET_DIR_INFO,  //获取目录信息

    FS_IOCTL_GETFILE_BYNAME_INDIR, //由歌曲名称获得歌词

    FS_IOCTL_GET_DISP_INFO, //用于长文件名获取

    FS_IOCTL_MK_DIR, //创建文件夹
    FS_IOCTL_GET_ENCFOLDER_INFO, //获取录音文件信息

    FS_IOCTL_GET_OUTFLASH_ADDR, //获取外置flash实际物理地址（暂时用于手表case,特殊fat系统）
};


struct vfs_devinfo;
struct vfscan;
struct vfs_operations;



struct vfs_devinfo {
    void *fd;
    u32 sector_size;
    void *private_data;
};

#define VFS_PART_DIR_MAX 16


struct vfs_partition {
    struct vfs_partition *next;
    u32 offset;
    u32 clust_size;
    u32 total_size;
    u8 fs_attr;
    char dir[VFS_PART_DIR_MAX];
    void *private_data;
};

struct fiter {
    u32 index;
};

struct ffolder {
    u16 fileStart;
    u16 fileTotal;
};



struct imount {
    int fd;
    const char *path;
    struct vfs_operations *ops;
    struct vfs_devinfo dev;
    struct vfs_partition part;
    struct list_head entry;
    atomic_t ref;
    OS_MUTEX mutex;
    u8 avaliable;
    u8 part_num;
};

struct vfs_attr {
    u8 attr;
    u32 fsize;
    u32 sclust;
    struct sys_time crt_time;
    struct sys_time wrt_time;
};

typedef struct {
    struct imount *mt;
    struct vfs_devinfo *dev;
    struct vfs_partition *part;
    void *private_data;
} FILE;


struct vfscan {
    u8 scan_file;
    u8 subpath; //子目录，设置是否只扫描一层
    u8 scan_dir;
    u8 attr;
    u8 cycle_mode;
    char sort;
    char ftype[20 * 3 + 1];
    u16 file_number;
    u16 file_counter;

    u16  dir_totalnumber;          // 文件夹总数
    u16  musicdir_counter;          // 播放文件所在文件夹序号
    u16  fileTotalInDir;           //文件夹下的文件数目

    void *priv;
    struct vfs_devinfo *dev;
    struct vfs_partition *part;
    char  filt_dir[12];
};


struct vfs_operations {
    const char *fs_type;
    int (*mount)(struct imount *, int);
    int (*unmount)(struct imount *);
    int (*format)(struct vfs_devinfo *, struct vfs_partition *);
    int (*fset_vol)(struct vfs_partition *, const char *name);
    int (*fget_free_space)(struct vfs_devinfo *, struct vfs_partition *, u32 *space);
    int (*fopen)(FILE *, const char *path, const char *mode);
    int (*fread)(FILE *, void *buf, u32 len);
    int (*fread_fast)(FILE *, void *buf, u32 len);
    int (*fwrite)(FILE *, void *buf, u32 len);
    int (*fseek)(FILE *, int offset, int);
    int (*fseek_fast)(FILE *, int offset, int);
    int (*flen)(FILE *);
    int (*fpos)(FILE *);
    int (*fcopy)(FILE *, FILE *);
    int (*fget_name)(FILE *, u8 *name, int len);
    int (*fget_path)(FILE *, u8 *name, int len, u8 is_relative_path);
    int (*frename)(FILE *, const char *path);
    int (*fclose)(FILE *);
    int (*fdelete)(FILE *);
    int (*fscan)(struct vfscan *, const char *path, u8 max_deepth);
    int (*fscan_interrupt)(struct vfscan *, const char *path, u8 max_deepth, int (*callback)(void));
    void (*fscan_release)(struct vfscan *);
    int (*fsel)(struct vfscan *, int sel_mode, FILE *, int);
    int (*fget_attr)(FILE *, int *attr);
    int (*fset_attr)(FILE *, int attr);
    int (*fget_attrs)(FILE *, struct vfs_attr *);
    int (*fmove)(FILE *file, const char *path_dst, FILE *, int clr_attr);
    int (*ioctl)(void *, int cmd, int arg);
};

#define REGISTER_VFS_OPERATIONS(ops) \
	const struct vfs_operations ops SEC(.vfs_operations)


static inline struct vfs_partition *vfs_partition_next(struct vfs_partition *p)
{
    struct vfs_partition *n = (struct vfs_partition *)zalloc(sizeof(*n));

    if (n) {
        p->next = n;
    }
    return n;
}


static inline void vfs_partition_free(struct vfs_partition *p)
{
    struct vfs_partition *n = p->next;

    while (n) {
        p = n->next;
        free(n);
        n = p;
    }
}




struct imount *mount(const char *dev_name, const char *path, const char *fs_type,
                     int cache_num, void *dev_arg);   //挂载

int unmount(const char *path);

int f_format(const char *path, const char *fs_type, u32 clust_size); //格式化接口

int f_free_cache(const char *path);

/*----------------------------------------------------------------------------*/
/** @brief:
@param: fopen 扩展功能
@note:     fopen自动打开、创建文件夹和文件。
    说明：
    1. 设备路径+文件，其中文件传入格式:"music/test/1/2/3/pk*.wav"  "JL_REC/AC69****.wav"  "JL_REC/AC690000.wav"
    2. 文件名带*号，带多少个*表示多少个可变数字，最多为8+3的大小，如表示可变数字名称变为XXX0001,XXXX002这样得格式，不带*号则只创建一个文件，写覆盖。
@date: 2020-07-22
*/
/*----------------------------------------------------------------------------*/

FILE *fopen(const char *path, const char *mode);

int fread(FILE *file, void *buf, u32 len);

int fwrite(FILE *file, void *buf, u32 len);

int fseek(FILE *file, int offset, int orig);

int fseek_fast(FILE *file, int offset, int orig);// 快速seek

int fread_fast(FILE *file, void *buf, u32 len); //快速read

int flen(FILE *file);

int fpos(FILE *file);

int fcopy(const char *format, ...);

int fget_name(FILE *file, u8 *name, int len);

int frename(FILE *file, const char *path);

int fclose(FILE *file);

int fdelete(FILE *file);
int fdelete_by_name(const char *fname);

int fget_free_space(const char *path, u32 *space);

int fget_path(FILE *file, u8 *name, int len, u8 is_relative_path);

/* arg:
 * -t  文件类型
 * -r  包含子目录
 * -d  扫描文件夹
 * -a  文件属性 r: 读， /: 非
 * -s  排序方式， t:按时间排序， n:按文件号排序
 */
struct vfscan *fscan(const char *path, const char *arg, u8 max_deepth); //扫描接口，参数配置如上。

struct vfscan *fscan_interrupt(const char *path, const char *arg, u8 max_deepth, int (*callback)(void)); //可打断扫描

struct vfscan *fscan_enterdir(struct vfscan *fs, const char *path);//进入指定子目录，只扫此目录下文件信息

struct vfscan *fscan_exitdir(struct vfscan *fs); //返回上一层

void fscan_release(struct vfscan *fs);

FILE *fselect(struct vfscan *fs, int selt_mode, int arg); //选择指定文件

int fdir_exist(const char *dir); //check 目录是否存在

int fdir(FILE *file, const char *arg, char *name, int len, struct fiter *iter);//暂无此接口

int fget_attr(FILE *file, int *attr); //获取文件属性

int fset_attr(FILE *file, int attr); //设置文件属性

int fget_attrs(FILE *file, struct vfs_attr *attr);//获得文件相关信息如属性、簇号、大小等

struct vfs_partition *fget_partition(const char *path);//获得分区part

int fset_vol(const char *path, const char *name);//设置卷标

int fmove(FILE *file, const char *path_dst, FILE **newFile, int clr_attr);//暂不支持

int fcheck(FILE *file);//暂不支持

int fget_err_code(const char *path); //暂不支持

int fset_name_filter(const char *path, void *name_filter);//暂不支持

int fget_folder(struct vfscan *fs, struct ffolder *arg); //获取文件夹序号和文件夹内文件数目

int fset_lfn_buf(struct vfscan *fs, void *arg);//设置长文件名buf
int fset_ldn_buf(struct vfscan *fs, void *arg);//设置长文件夹名buf

int fset_ext_type(const char *path, void *ext_type);//设置后缀类型
int fopen_dir_info(const char *path, FILE **pp_file, void *dir_dj); //打开目录
int fenter_dir_info(FILE *file, void *dir_dj); //进入目录
int fexit_dir_info(FILE *file); //退出
int fget_dir_info(FILE *file, u32 start_num, u32 total_num, void *buf_info); ////获取目录信息

int fget_fat_outflash_addr(FILE *file, void *buf_info);//获取外置flash实际物理地址（暂时用于手表case,特殊fat系统）

int fget_file_byname_indir(FILE *file, FILE **newFile, void *ext_name); //由歌曲名称获得歌词

int fget_disp_info(FILE *file, void *arg); //用于长文件名获取

int fmk_dir(const char *path, char *folder, u8 mode); //创建目录

int fget_encfolder_info(const char *path, char *folder, char *ext, u32 *last_num, u32 *total_num); //获取录音文件信息

int fname_to_path(char *result, const char *path, const char *fname, int len); //把路径和文件名拼接

int get_last_num(void); //录音获取最后序号。

void set_bp_info(u32 clust, u32 fsize, u32 *flag); //扫描前设置断点参数，需要put_bp_info对应释放buf.
void put_bp_info(void);

#endif  /* VFS_ENABLE */

#endif  /* __FS_H__ */






