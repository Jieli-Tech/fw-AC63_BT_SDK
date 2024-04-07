#ifndef SENSOR_UART_H
#define SENSOR_UART_H

// @brief 初始化sensor数据打印串口
// @parm NULL
// @return 0 成功，-1 失败
int sensor_uart_init(void);

// @brief 打印串口数据
// @parm NULL
// @return NULL
void sensor_uart_send_data(void);
#endif

