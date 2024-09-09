#ifndef __ELET_UART__
#define __ELET_UART__

#include <stdint.h>

uint8_t elet_uart_state_get(void);
uint32_t elet_uart_fifo_get_data_len(void);
void elet_uart_data_read(uint8_t *p_d, uint32_t *data_len);
void elet_uart_change_baud(uint32_t baud);
void elet_uart_write(char *buf, uint16_t len);
int cb_buf_io_uart_tx_peek_data(const void *data, int len, int has_more, void *extra);
void elet_uart_init(void);

#endif // __ELET_UART__
