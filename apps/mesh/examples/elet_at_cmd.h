#ifndef __ELET_AT_CMD__
#define __ELET_AT_CMD__

#include <stdint.h>

int at_send_uart_data(uint8_t *packet, int16_t size, int16_t post_event);
void elet_at_cmd_post_sem(void);
void elet_at_cmd_init(void);

#endif //__ELET_AT_CMD__
