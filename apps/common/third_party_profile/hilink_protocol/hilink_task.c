#include "hilink_task.h"
#include "os/os_api.h"
#include "hilink_protocol.h"

#if 1
extern void printf_buf(u8 *buf, u32 len);
/* #define log_info          printf */
#define log_info(x, ...)  printf("[LE_TASK]" x " ", ## __VA_ARGS__)
#define log_info_hexdump  printf_buf
#else
#define log_info(...)
#define log_info_hexdump(...)
#endif

HILINK_PACKET_CONTROL HILINK_packet_c;

#define __this      (&HILINK_packet_c)

extern void hilink_msg_handle(u8 *buf, u16 len);

static void hilink_task(void *arg)
{
    HILINK_PACKET_HEAD_T hilink_packet_head;
    u8 *buffer = NULL;
    u8 result = 0;
    while (1) {
        os_sem_pend(&(__this->hilink_sem), 0);
        if (cbuf_get_data_len(&(__this->cbuf)) > HILINK_PACKET_HEAD_LEN) {
            cbuf_read(&(__this->cbuf), &hilink_packet_head, HILINK_PACKET_HEAD_LEN);
            buffer = malloc(hilink_packet_head.len);
            if (buffer == NULL) {
                log_info("buf malloc err\n");
                break;
            }
            cbuf_read(&(__this->cbuf), buffer, hilink_packet_head.len);
            switch (hilink_packet_head.packet_channel) {
            case HILINK_DEVICE_INFO_MSG_CH:
                hilink_msg_handle(buffer, hilink_packet_head.len);
                break;
            case HILINK_OTA_CTL_MSG_CH:
                hilink_ota_ctl_deal(buffer, hilink_packet_head.len);
                break;
            case HILINK_OTA_DATA_MSG_CH:
                hilink_ota_data_deal(buffer, hilink_packet_head.len);
                break;
            }
            free(buffer);
        }
    }
}

int hilink_task_init(void)
{
    os_sem_create(&(__this->hilink_sem), 0);
    u32 malloc_size = HILINK_MSG_POOL_SIZE;
    __this->hilink_buf = malloc(malloc_size);
    memset(__this->hilink_buf, 0x0, malloc_size);
    cbuf_init(&(__this->cbuf), __this->hilink_buf, malloc_size);
    int ret = task_create(hilink_task, 0, "hilink_task");
    if (ret) {
        log_info("hilink task create error:%d", ret);
    }
    return 0;
}

void hilink_packet_recieve(void *buf, u16 len)
{
    if (cbuf_is_write_able(&(__this->cbuf), len) >= len) {
        cbuf_write(&(__this->cbuf), buf, len);
    } else {
        log_info("[L]\n");
    }
    os_sem_post(&(__this->hilink_sem));
}

