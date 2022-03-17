/**@file  		power_port.h
* @brief
* @details		电源模块 gpio 相关
* @author
* @date     	2021-11-10
* @version    	V1.0
* @copyright  	Copyright(c)2010-2021  JIELI
 */
#ifndef __POWER_PORT_H__
#define __POWER_PORT_H__

#include "asm/cpu.h"

enum {
    PORTA_GROUP = 0,
    PORTB_GROUP,
    PORTC_GROUP,
    PORTD_GROUP,
};

//flash
#define		SPI0_PWR_A		IO_PORTD_04
#define		SPI0_CS_A		IO_PORTD_03
#define 	SPI0_CLK_A		IO_PORTD_00
#define 	SPI0_DO_D0_A	IO_PORTD_01
#define 	SPI0_DI_D1_A	IO_PORTD_02
#define 	SPI0_WP_D2_A	IO_PORTB_04
#define 	SPI0_HOLD_D3_A	IO_PORTC_04


#define		SPI0_PWR_B		IO_PORTD_04
#define		SPI0_CS_B		IO_PORTC_01
#define 	SPI0_CLK_B		IO_PORTD_00
#define 	SPI0_DO_D0_B	IO_PORTD_01
#define 	SPI0_DI_D1_B	IO_PORTC_02
#define 	SPI0_WP_D2_B	IO_PORTC_03
#define 	SPI0_HOLD_D3_B	IO_PORTC_04

void port_protect(u16 *port_group, u32 port_num);
u32 spi_get_port(void);
void port_init(void);

#endif
