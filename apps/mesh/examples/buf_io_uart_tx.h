#ifndef __BUF_IO_UART_TX__
#define __BUF_IO_UART_TX__

#include <stdint.h>

void    buf_io_uart_tx_send_event(void);
uint8_t buf_io_uart_tx_highwater_state_get(void);
void    buf_io_uart_tx_init_buffer(void);
int8_t  buf_io_uart_tx_send_data(const uint8_t *data, int len, int flush);
void    buf_io_uart_tx_do_write(int flush);

#endif // __BUF_IO_UART_TX__
