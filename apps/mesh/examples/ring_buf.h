#ifndef _ring_buf_h
#define _ring_buf_h

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 ****************************************************************************************
 * @brief A simple yet efficient ring buffer
 *
 * Thread safety:
 *
 * Call `ring_buf_write_data` only in thread A; `ring_buf_peek_data` in
 * thread B. A and B can be the same or different.
 *
 ****************************************************************************************
 */

#define RING_BUF_OBJ_SIZE   (32)

typedef struct ring_buf
{
	void (*highwater_cb)(struct ring_buf *buf);
	void (*lowwater_cb)(struct ring_buf *buf);
	uint32_t          total_size;
	uint32_t          free_size_low;
	uint32_t          free_size_high;
	uint32_t          write_next;
	uint32_t          read_next;
	uint32_t		  watermark_event;
	uint8_t           buffer[1];
} ring_buf_t;

uint8_t ring_buf_is_empty(struct ring_buf *buf);
uint16_t ring_buf_num_items(struct ring_buf *buf);
uint32_t ring_buf_free_items(struct ring_buf *buf);

/**
 ****************************************************************************************
 * @brief Create a ring buffer
 *
 * Note: a `struct ring_buf` variable of `RING_BUF_OBJ_SIZE` bytes occupies the beginning
 *       of `buf`.
 *
 * @param[in] buf           buffer
 * @param[in] total_size    total size fo buffer in bytes
 * @return                  `ring_buf` object
 ****************************************************************************************
 */
struct ring_buf *ring_buf_init(void *buf, int total_size, void (*highwater_cb)(struct ring_buf *buf), void (*lowwater_cb)(struct ring_buf *buf));

/**
 ****************************************************************************************
 * @brief Reset a ring buffer
 *
 * @param[in] buf           `ring_buf` object
 ****************************************************************************************
 */
void ring_buf_reset(struct ring_buf *buf);

typedef int (*f_ring_peek_data)(const void *data, int len, int has_more, void *extra);

/**
 ****************************************************************************************
 * @brief Peek data from a ring buffer
 *
 * @param[in] buf           `ring_buf` object
 * @param[in] peek_data     callback function
 * @param[in] param         extra param for `peek_data`
 * @return                  total bytes peeked by callback function
 ****************************************************************************************
 */
int ring_buf_peek_data(struct ring_buf *buf, f_ring_peek_data peek_data, void *extra);

/**
 ****************************************************************************************
 * @brief Write data into a ring buffer
 *
 * @param[in] buf           `ring_buf` object
 * @param[in] data          data to be written
 * @param[in] len           data length
 * @return                  number of bytes actually written
 ****************************************************************************************
 */
int ring_buf_write_data(struct ring_buf *buf, const void *data, int len);

#ifdef __cplusplus
}
#endif

#endif
