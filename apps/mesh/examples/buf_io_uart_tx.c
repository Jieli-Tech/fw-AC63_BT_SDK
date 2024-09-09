#include "buf_io_uart_tx.h"
#include "ring_buf.h"
#include "elet_uart.h"

#include "bt_common.h"


#define BLOCK_SIZE          (1024u)
#define RX_BUFFER_SIZE      (BLOCK_SIZE * 2 + RING_BUF_OBJ_SIZE)

static uint8_t ring_buff_storage[RX_BUFFER_SIZE] __attribute__((aligned(4)));
static struct ring_buf *ring_buffer;
static uint8_t ring_buf_highwater_flag = 0;

#include "board_config.h"
#include "system/includes.h"
#include "asm/power_interface.h"
#include "app_config.h"
#undef _DEBUG_H_
#define LOG_TAG_CONST       UART_TX
#define LOG_TAG             "[UART_TX]"
#include "debug.h"
const char log_tag_const_v_UART_TX AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_UART_TX AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_d_UART_TX AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_UART_TX AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_UART_TX AT(.LOG_TAG_CONST) = 1;

static OS_SEM buf_io_uart_tx_sem;
static void buf_io_uart_tx_task(void *p)
{
	int ret = 0;
	u32 rlen = 0;
	static u32 r_len_total = 0;

	while (1)
	{
		clr_wdt();
		ret = os_sem_pend(&buf_io_uart_tx_sem, 0);
		log_debug("%s[os_sem_pend -> ret:%d]", __func__, ret);
		buf_io_uart_tx_do_write(1);
	}
}

void buf_io_uart_tx_send_event(void)
{
	int ret = OS_NO_ERR;
	ret = os_sem_post(&buf_io_uart_tx_sem);

	if (ret) log_error("%s[ret:%d]", __func__, ret);

	return;
}

static int buf_io_uart_tx_task_init(void)
{
	int ret = OS_NO_ERR;
	os_sem_create(&buf_io_uart_tx_sem, 0);
	os_sem_set(&buf_io_uart_tx_sem, 0);
	ret = os_task_create(buf_io_uart_tx_task, NULL, 7, 256, 0, "uart_tx");

	if (ret != OS_NO_ERR) log_error("%s %s create fail 0x%x", __func__, "uart_tx", ret);

	return ret;
}

uint8_t buf_io_uart_tx_highwater_state_get(void)
{
	return ring_buf_highwater_flag;
}

static void buf_io_uart_tx_highwater_cb(struct ring_buf *buf)
{
	log_debug("%s", __func__);
	ring_buf_highwater_flag = 1;

	buf_io_uart_tx_send_event();
}

static void buf_io_uart_tx_lowwater_cb(struct ring_buf *buf)
{
	log_debug("%s", __func__);
	ring_buf_highwater_flag = 0;
}

uint8_t buf_io_uart_tx_is_empty(void)
{
	return ring_buf_is_empty(ring_buffer);
}

uint32_t buf_io_uart_tx_num_items(void)
{
	return ring_buf_num_items(ring_buffer);
}

void buf_io_uart_tx_reset(void)
{
	ring_buf_reset(ring_buffer);
}

void buf_io_uart_tx_init_buffer(void)
{
	ring_buffer = ring_buf_init(ring_buff_storage, sizeof(ring_buff_storage), buf_io_uart_tx_highwater_cb, buf_io_uart_tx_lowwater_cb);
	buf_io_uart_tx_task_init();
}

int8_t buf_io_uart_tx_send_data(const uint8_t *data, int len, int flush)
{
	int written = ring_buf_write_data(ring_buffer, data, len);

	if (written < len)
		log_error("data lost: %d!!!\n", len - written);

	if (flush)
	{
		buf_io_uart_tx_send_event();
	}

	return 0;
}

void buf_io_uart_tx_do_write(int flush)
{
	// log_debug("buf_io_uart_tx_do_write %d\n", flush);
	ring_buf_peek_data(ring_buffer, cb_buf_io_uart_tx_peek_data, (void *)flush);
}

