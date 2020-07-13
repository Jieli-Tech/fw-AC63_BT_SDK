#ifndef __BR22_CHARGESTORE_H__
#define __BR22_CHARGESTORE_H__

enum {
    CMD_COMPLETE,
    CMD_RECVDATA,
};

enum {
    MODE_RECVDATA,
    MODE_SENDDATA,
};

enum {
    TYPE_NORMAL,
    TYPE_F95,
};

struct chargestore_platform_data {
    u32 baudrate;
    u32 io_port;
    u8 uart_irq;
    void (*init)(const struct chargestore_platform_data *);
    void (*open)(u8 mode);
    void (*close)(void);
    void (*write)(u8 *, u8);
};

#define CHARGESTORE_PLATFORM_DATA_BEGIN(data) \
    static const struct chargestore_platform_data data = {

#define CHARGESTORE_PLATFORM_DATA_END() \
    .baudrate               = 9600, \
    .init                   = chargestore_init, \
    .open                   = chargestore_open, \
    .close                  = chargestore_close, \
    .write                  = chargestore_write, \
};

extern void chargestore_open(u8 mode);
extern void chargestore_close(void);
extern void chargestore_write(u8 *data, u8 len);
extern void chargestore_init(const struct chargestore_platform_data *);
extern void chargestore_set_update_ram(void);
extern u8 chargestore_get_det_level(u8 chip_type);

//app层使用的接口
extern void chargestore_api_close(void);
extern int chargestore_api_write(u8 *buf, u8 len);
extern void chargestore_api_init(const struct chargestore_platform_data *arg);
extern void chargestore_api_wait_complete(void);

#endif
