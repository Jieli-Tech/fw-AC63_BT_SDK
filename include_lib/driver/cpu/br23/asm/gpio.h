/**
 * @file gpio.h
 * @brief
 * @author @zh-jieli.com
 * @version 1.0.0
 * @date 2018-10-11
 */

#ifndef  __GPIO_H__
#define  __GPIO_H__
#include "typedef.h"
enum {
    CH0_UT0_TX,
    CH0_UT1_TX,
    CH0_T0_PWM_OUT,
    CH0_T1_PWM_OUT,
    CH0_RTOSH_CLK,
    CH0_BTOSC_CLK,
    CH0_PLL_12M,
    CH0_UT2_TX,
    CH0_CH0_PWM_H,
    CH0_CH0_PWM_L,
    CH0_CH1_PWM_H,
    CH0_CH1_PWM_L,
    CH0_CH2_PWM_H,
    CH0_CH2_PWM_L,
    CH0_WLC_BT_FREQ,
    CH0_T3_PWM_OUT,

    CH1_UT0_TX = 0x10,
    CH1_UT1_TX,
    CH1_T0_PWM_OUT,
    CH1_WLC_BT_PRO,
    CH1_RTOSL_CLK,
    CH1_BTOSC_CLK,
    CH1_SPDIF_DO,
    CH1_UT2_TX,
    CH1_CH0_PWM_H,
    CH1_CH0_PWM_L,
    CH1_CH1_PWM_H,
    CH1_CH1_PWM_L,
    CH1_CH2_PWM_H,
    CH1_CH2_PWM_L,
    CH1_T2_PWM_OUT,
    CH1_T3_PWM_OUT,

    CH2_UT1_RTS = 0x20,
    CH2_UT1_TX,
    CH2_WLC_BT_ACTIVE,
    CH2_T1_PWM_OUT,
    CH2_PLNK_SCLK,
    CH2_BTOSC_CLK,
    CH2_PLL_24M,
    CH2_UT2_TX,
    CH2_CH0_PWM_H,
    CH2_CH0_PWM_L,
    CH2_CH1_PWM_H,
    CH2_CH1_PWM_L,
    CH2_CH2_PWM_H,
    CH2_CH2_PWM_L,
    CH2_T2_PWM_OUT,
    CH2_T3_PWM_OUT,
};


#define IO_GROUP_NUM 		16


#define IO_PORTA_00 				(IO_GROUP_NUM * 0 + 0)
#define IO_PORTA_01 				(IO_GROUP_NUM * 0 + 1)
#define IO_PORTA_02 				(IO_GROUP_NUM * 0 + 2)
#define IO_PORTA_03 				(IO_GROUP_NUM * 0 + 3)
#define IO_PORTA_04 				(IO_GROUP_NUM * 0 + 4)
#define IO_PORTA_05 				(IO_GROUP_NUM * 0 + 5)
#define IO_PORTA_06 				(IO_GROUP_NUM * 0 + 6)
#define IO_PORTA_07 				(IO_GROUP_NUM * 0 + 7)
#define IO_PORTA_08 				(IO_GROUP_NUM * 0 + 8)
#define IO_PORTA_09 				(IO_GROUP_NUM * 0 + 9)
#define IO_PORTA_10 				(IO_GROUP_NUM * 0 + 10)
#define IO_PORTA_11 				(IO_GROUP_NUM * 0 + 11)
#define IO_PORTA_12 				(IO_GROUP_NUM * 0 + 12)
#define IO_PORTA_13 				(IO_GROUP_NUM * 0 + 13)
#define IO_PORTA_14 				(IO_GROUP_NUM * 0 + 14)
#define IO_PORTA_15 				(IO_GROUP_NUM * 0 + 15)

#define IO_PORTB_00 				(IO_GROUP_NUM * 1 + 0)
#define IO_PORTB_01 				(IO_GROUP_NUM * 1 + 1)
#define IO_PORTB_02 				(IO_GROUP_NUM * 1 + 2)
#define IO_PORTB_03 				(IO_GROUP_NUM * 1 + 3)
#define IO_PORTB_04 				(IO_GROUP_NUM * 1 + 4)
#define IO_PORTB_05 				(IO_GROUP_NUM * 1 + 5)
#define IO_PORTB_06 				(IO_GROUP_NUM * 1 + 6)
#define IO_PORTB_07 				(IO_GROUP_NUM * 1 + 7)
#define IO_PORTB_08 				(IO_GROUP_NUM * 1 + 8)
#define IO_PORTB_09 				(IO_GROUP_NUM * 1 + 9)
#define IO_PORTB_10 				(IO_GROUP_NUM * 1 + 10)
#define IO_PORTB_11 				(IO_GROUP_NUM * 1 + 11)
#define IO_PORTB_12 				(IO_GROUP_NUM * 1 + 12)
#define IO_PORTB_13 				(IO_GROUP_NUM * 1 + 13)
#define IO_PORTB_14 				(IO_GROUP_NUM * 1 + 14)
#define IO_PORTB_15 				(IO_GROUP_NUM * 1 + 15)

#define IO_PORTC_00 				(IO_GROUP_NUM * 2 + 0)
#define IO_PORTC_01 				(IO_GROUP_NUM * 2 + 1)
#define IO_PORTC_02 				(IO_GROUP_NUM * 2 + 2)
#define IO_PORTC_03 				(IO_GROUP_NUM * 2 + 3)
#define IO_PORTC_04 				(IO_GROUP_NUM * 2 + 4)
#define IO_PORTC_05 				(IO_GROUP_NUM * 2 + 5)
#define IO_PORTC_06 				(IO_GROUP_NUM * 2 + 6)
#define IO_PORTC_07 				(IO_GROUP_NUM * 2 + 7)
#define IO_PORTC_08 				(IO_GROUP_NUM * 2 + 8)
#define IO_PORTC_09 				(IO_GROUP_NUM * 2 + 9)
#define IO_PORTC_10 				(IO_GROUP_NUM * 2 + 10)
#define IO_PORTC_11 				(IO_GROUP_NUM * 2 + 11)
#define IO_PORTC_12 				(IO_GROUP_NUM * 2 + 12)
#define IO_PORTC_13 				(IO_GROUP_NUM * 2 + 13)
#define IO_PORTC_14 				(IO_GROUP_NUM * 2 + 14)
#define IO_PORTC_15 				(IO_GROUP_NUM * 2 + 15)

#define IO_PORTD_00 				(IO_GROUP_NUM * 3 + 0)
#define IO_PORTD_01 				(IO_GROUP_NUM * 3 + 1)
#define IO_PORTD_02 				(IO_GROUP_NUM * 3 + 2)
#define IO_PORTD_03 				(IO_GROUP_NUM * 3 + 3)
#define IO_PORTD_04 				(IO_GROUP_NUM * 3 + 4)
#define IO_PORTD_05 				(IO_GROUP_NUM * 3 + 5)
#define IO_PORTD_06 				(IO_GROUP_NUM * 3 + 6)
#define IO_PORTD_07 				(IO_GROUP_NUM * 3 + 7)


#define IO_MAX_NUM 					(IO_PORTD_07+1)


#define IO_PORT_PR_00               (IO_MAX_NUM + 0)
#define IO_PORT_PR_01               (IO_MAX_NUM + 1)
#define IO_PORT_PR_02               (IO_MAX_NUM + 2)
#define IO_PORT_PR_03               (IO_MAX_NUM + 3)
#define IO_PORT_PR_04               (IO_MAX_NUM + 4)

#define USB_IO_OFFSET               5
#define IO_PORT_DP                  (IO_MAX_NUM + USB_IO_OFFSET)
#define IO_PORT_DM                  (IO_MAX_NUM + USB_IO_OFFSET + 1)

#define P33_IO_OFFSET               7
#define IO_LDOIN_DET                (IO_MAX_NUM + P33_IO_OFFSET)
#define IO_LVCMP_DET                (IO_MAX_NUM + P33_IO_OFFSET + 1)

#define IO_PORT_MAX					(IO_LVCMP_DET + 1)

#define GPIOA                       (IO_GROUP_NUM * 0)
#define GPIOB                       (IO_GROUP_NUM * 1)
#define GPIOC                       (IO_GROUP_NUM * 2)
#define GPIOD                       (IO_GROUP_NUM * 3)
#define GPIOR                       (IO_MAX_NUM)
#define GPIOUSB                     (IO_MAX_NUM + USB_IO_OFFSET)
#define GPIOP33                     (IO_MAX_NUM + P33_IO_OFFSET)

#define MIC_HW_IO                   IO_PORTC_06

enum {
    INPUT_CH0,
    INPUT_CH1,
    INPUT_CH2,
    INPUT_CH3,
};

enum gpio_op_mode {
    GPIO_SET = 1,
    GPIO_AND,
    GPIO_OR,
    GPIO_XOR,
};
enum gpio_direction {
    GPIO_OUT = 0,
    GPIO_IN = 1,
};
struct gpio_reg {
    volatile unsigned int out;
    volatile unsigned int in;
    volatile unsigned int dir;
    volatile unsigned int die;
    volatile unsigned int pu;
    volatile unsigned int pd;
    volatile unsigned int hd0;
    volatile unsigned int hd;
    volatile unsigned int dieh;
};

struct gpio_platform_data {
    unsigned int gpio;
};

#define GPIO_PLATFORM_DATA_BEGIN(data) \
	static const struct gpio_platform_data data = { \


#define GPIO_PLATFORM_DATA_END() \
	};
#if 0
#define     IO_DEBUG_0(i,x)       {JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT &= ~BIT(x);}
#define     IO_DEBUG_1(i,x)       {JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT |= BIT(x);}
#define     IO_DEBUG_TOGGLE(i,x)  {JL_PORT##i->DIR &= ~BIT(x), JL_PORT##i->OUT ^= BIT(x);}


#else
#define     IO_DEBUG_0(i,x)         {}
#define     IO_DEBUG_1(i,x)         {}
#define     IO_DEBUG_TOGGLE(i,x)    {}

#endif
/**
 * @brief usb_iomode
 *
 * @param enable 1，使能；0，关闭
 */
void usb_iomode(u32 enable);
/**
 * @brief gpio_direction_input
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param dir 1，输入；0，输出
 *
 * @return
 */
int gpio_set_direction(u32 gpio, u32 dir);


/**
 * @brief gpio_direction_input
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param value 1，输出1,  0，输出0
 *
 * @return
 */
int gpio_set_output_value(u32 gpio, u32 dir);

/**
 * @brief gpio_dir
 *
 * @param gpio [GPIOA GPIOB GPIOC GPIOD GPIOR GPIOUSB]
 * @param start [0-15]，GPIOR[0-3]，GPIOUSB[0-1]
 * @param len  [0-15]，GPIOR[0-3]，GPIOUSB[0-1]
 * @param dat 1，输入；0，输出
 *
 * @return
 */
u32 gpio_dir(u32 gpio, u32 start, u32 len, u32 dat, enum gpio_op_mode op);

/**
 * @brief gpio_direction_output
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param value 1，输出1；0，输出0
 *
 * @return
 */
int gpio_direction_output(u32 gpio, int value);

/**
 * @brief gpio_out
 *
 * @param gpio [GPIOA GPIOB GPIOC GPIOD GPIOR GPIOUSB]
 * @param start [0-15]，GPIOR[0-3]，GPIOUSB[0-1]
 * @param len  [0-15]，GPIOR[0-3]，GPIOUSB[0-1]
 * @param dat 1，输入；0，输出
 *
 * @return
 */
u32 gpio_out(u32 gpio, u32 start, u32 len, u32 dat);

/**
 * @brief gpio_set_pull_up
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param value 1，上拉；0，不上拉
 *
 * @return
 */
int gpio_set_pull_up(u32 gpio, int value);


/**
 * @brief gpio_set_pu
 *
 * @param gpio [GPIOA GPIOB GPIOC GPIOD GPIOR GPIOUSB]
 * @param start [0-15]，GPIOR[0-3]，GPIOUSB[0-1]
 * @param len  [0-15]，GPIOR[0-3]，GPIOUSB[0-1]
 * @param dat 1，上拉；0，不上拉
 *
 * @return
 */
u32 gpio_set_pu(u32 gpio, u32 start, u32 len, u32 dat, enum gpio_op_mode op);

/**
 * @brief gpio_set_pull_down
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param value 1，下拉；0，不下拉
 *
 * @return
 */
int gpio_set_pull_down(u32 gpio, int value);

/**
 * @brief gpio_set_pd
 *
 * @param gpio [GPIOA GPIOB GPIOC GPIOD GPIOR GPIOUSB]
 * @param start [0-15]，GPIOR[0-3]，GPIOUSB[0-1]
 * @param len  [0-15]，GPIOR[0-3]，GPIOUSB[0-1]
 * @param dat 1，下拉；0，不下拉
 *
 * @return
 */
u32 gpio_set_pd(u32 gpio, u32 start, u32 len, u32 dat, enum gpio_op_mode op);

/**
 * @brief gpio_set_hd0
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param value 1，增强输出；0，不增强输出
 *
 * @return
 */
u32 gpio_set_hd0(u32 gpio, u32 value);

/**
 * @brief gpio_set_hd
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param value 1，增强输出；0，不增强输出
 *
 * @return
 */
int gpio_set_hd(u32 gpio, int value);

/**
 * @brief gpio_set_die
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param value 1，IO普通输入；0，IO模拟输入
 *
 * @return
 */
int gpio_set_die(u32 gpio, int value);

/**
 * @brief gpio_set_dieh
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param value 1，IO普通输入；0，IO模拟输入
 *
 * @return
 */
u32 gpio_set_dieh(u32 gpio, u32 value);

/**
 * @brief gpio_die
 *
 * @param gpio [GPIOA GPIOB GPIOC GPIOD GPIOR GPIOUSB]
 * @param start [0-15]，GPIOR[0-3]，GPIOUSB[0-1]
 * @param len  [0-15]，GPIOR[0-3]，GPIOUSB[0-1]
 * @param dat 1，IO普通输入；0，IO模拟输入
 *
 * @return
 */
u32 gpio_die(u32 gpio, u32 start, u32 len, u32 dat, enum gpio_op_mode op);

/**
 * @brief gpio_dieh
 *
 * @param gpio [GPIOA GPIOB GPIOC GPIOD GPIOR GPIOUSB]
 * @param start [0-15]，GPIOR[0-3]，GPIOUSB[0-1]
 * @param len  [0-15]，GPIOR[0-3]，GPIOUSB[0-1]
 * @param dat 1，IO普通输入；0，IO模拟输入
 *
 * @return
 */
u32 gpio_dieh(u32 gpio, u32 start, u32 len, u32 dat, enum gpio_op_mode op);

/**
 * @brief gpio_set_output_channle
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param clk 参考枚举CHx_UTx_TX，如CH0_UT0_TX
 *
 * @return
 */
u32 gpio_output_channle(u32 gpio, u32 clk);

/**
 * @brief gpio_read
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 *
 * @return
 */
int gpio_read(u32 gpio);

/**
 * @brief gpio_in
 *
 * @param gpio [GPIOA GPIOB GPIOC GPIOD GPIOR GPIOUSB]
 *
 * @return
 */
u32 gpio_in(u32 gpio);
/**
 * @brief gpio_write
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 * @param value 1，输出1；0，输出0
 *
 * @return
 */
u32 gpio_write(u32 gpio, u32 value);

/**
 * @brief gpio_wakeup0 use IN_CHNL0_SEL
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 *
 * @return
 */
u32 gpio_wakeup0(u32 gpio);

/**
 * @brief gpio_irflt_in use IN_CHNL1_SEL
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 *
 * @return
 */
u32 gpio_irflt_in(u32 gpio);

/**
 * @brief gpio_cap_mux use IN_CHNL2_SEL
 *
 * @param gpio 参考宏IO_PORTx_xx，如IO_PORTA_00
 *
 * @return
 */
u32 gpio_cap_mux(u32 gpio);


/**
 * @brief gpio_uart_rx_input
 *
 * @param gpio
 * @param ut
 * @param ch
 *
 * @return
 */
u32 gpio_uart_rx_input(u32 gpio, u32 ut, u32 ch);

/**
 * @brief
 *
 * @return
 */
u32 gpio_close_uart0(void);

/**
 * @brief
 *
 * @return
 */
u32 gpio_close_uart1(void);

/**
 * @brief
 *
 * @return
 */
u32 gpio_close_uart2(void);

/**
 * @brief gpio_set_uart0
 *
 * @param ch 0:3 选择对应IO br22
 *         |ch|tx|rx|
 *         |- |- |- |
 *         |0|PA5_TX|PA6_RX|
 *         |1|PB7_TX|PB8_RX|
 *         |2|PA7_TX|PA8_RX|
 *         |3|预留|预留|
 *         |-1|关闭对应的IO口串口功能|no|
 *
 * @return
 */
u32 gpio_set_uart0(u32 ch);
/**
 * @brief gpio_set_uart1
 *
 * @param ch 0:3 选择对应IO  br22
 *         |ch|tx|rx|
 *         |- |- |- |
 *         |0|PB5_TX|PB6_RX|
 *         |1|预留|预留|
 *         |2|PA1_TX|PA2_RX|
 *         |3|USBDP |USBDM |
 *         |-1|关闭对应的IO口串口功能|no|
 *
 * @return
 */
u32 gpio_set_uart1(u32 ch);
/**
 * @brief gpio_set_uart2
 *
 * @param ch 0:3 选择对应IO  br22
 *         |ch|tx|rx|
 *         |- |- |- |
 *         |0|PA3_TX|PA4_RX|
 *         |1|预留|预留|
 *         |2|预留|预留|
 *         |3|PA9_TX|PA10_RX|
 *         |-1|关闭对应的IO口串口功能|no|
 *
 * @return
 */
u32 gpio_set_uart2(u32 ch);

enum {
    IRFLT_LSB,
    IRFLT_RC,
    IRFLT_OSC,
    IRFLT_PLL48M,
};
enum {
    IRFLT_DIV1,
    IRFLT_DIV2,
    IRFLT_DIV4,
    IRFLT_DIV8,
    IRFLT_DIV16,
    IRFLT_DIV32,
    IRFLT_DIV64,
    IRFLT_DIV128,
    IRFLT_DIV256,
    IRFLT_DIV512,
    IRFLT_DIV1024,
    IRFLT_DIV2048,
    IRFLT_DIV4096,
    IRFLT_DIV8192,
    IRFLT_DIV16384,
    IRFLT_DIV32768,
};
/* u32 irflt_config(u32 osc, u32 div); */

/**
 * @brief gpio_irflt_to_timer
 *
 * @param t: [0-3]
 *
 * @return
 */
u32 gpio_irflt_to_timer(u32 t);


u32 get_gpio(const char *p);


#endif  /*GPIO_H*/
