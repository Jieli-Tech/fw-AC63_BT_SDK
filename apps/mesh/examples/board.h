#ifndef __BOARD_H__
#define __BOARD_H__

#include "gpio.h"

/* ET23 */

#define PIN1_GND
#define PIN2_VDD
#define PIN3_NC
#define PIN4_CDS                    IO_PORTA_08
#define PIN5_SOFT_RESET             IO_PORTA_07
#define PIN6_BT_LINK                IO_PORTA_00
#define PIN7_NC
#define PIN8_NC
#define PIN9_NC

#define PIN18_NC
#define PIN17_UARTA_RXD             IO_PORTB_05
#define PIN16_UARTA_TXD             IO_PORTB_04
#define PIN15_UARTA_CTS             IO_PORTA_01
#define PIN14_UARTA_RTS             IO_PORTA_02
#define PIN13_LED                   IO_PORTB_06
#define PIN12_GPIO                  IO_PORTB_07
#define PIN11_DP
#define PIN10_DM

#endif // __BOARD_H__
