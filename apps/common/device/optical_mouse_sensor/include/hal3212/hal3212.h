
//===========================================================================
//	For PixArt Mouse Sensor
//===========================================================================
#ifndef _PAW3212_H_
#define _PAW3212_H_
#include "asm/cpu.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef OMSensor_HAL3212_ENABLE


#define EN_PAW3212
// PixArt Register Addresses
#define pixart_PID0_ADDR         	0x00
#define pixart_PID1_ADDR        	0x01
#define pixart_MOTION_ADDR       	0x02
#define pixart_DELTAX_ADDR     		0x03
#define pixart_DELTAY_ADDR   		0x04
#define pixart_OPMODE_ADDR    		0x05
#define pixart_CONFIG_ADDR      	0x06

#define pixart_WP_ADDR     			0x09
#define pixart_SLP1_ADDR         	0x0A
#define pixart_SLP2_ADDR     		0x0B
#define pixart_SLP3_ADDR    		0x0C
#define pixart_CPI_X_ADDR    		0x0D
#define pixart_CPI_Y_ADDR   		0x0E

#define pixart_Delta_XY_HI_ADDR     0x12
#define pixart_IQC_ADDR     		0x13
#define pixart_Shutter_ADDR    		0x14
#define pixart_Frame_Avg_ADDR    	0x17
#define pixart_Mouse_Option_ADDR   	0x19

#define pixart_POWER_DOWN_ADDR      0x4B


// PixArt Read/Write configuration settings
#define pixart_WRITE      0x80
#define pixart_READ       0x00

#endif //_HAL3212_H_
#endif //_HAL3212_H_

