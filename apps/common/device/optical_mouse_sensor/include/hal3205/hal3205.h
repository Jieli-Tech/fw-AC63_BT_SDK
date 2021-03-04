//===========================================================================
//	For PixArt Mouse Sensor		//Hill, 2009.12.11
//===========================================================================
#ifndef _PAW3205_H_
#define _PAW3205_H_
#include "asm/cpu.h"
#include <stdint.h>
#include <stdbool.h>
#ifdef OMSensor_HAL3205_ENABLE

#define EN_PAW3205
// PixArt Register Addresses
#define pixart_PID0_ADDR         	0x00
#define pixart_PID1_ADDR        	0x01
#define pixart_MOTION_ADDR       	0x02
#define pixart_DELTAX_ADDR     		0x03
#define pixart_DELTAY_ADDR   		0x04
#define pixart_OPMODE_ADDR    		0x05
#define pixart_CONFIG_ADDR      	0x06
#define pixart_IMGQA_ADDR     		0x07
#define pixart_OPSTATE_ADDR     	0x08
#define pixart_WP_ADDR     			0x09
#define pixart_SLP1_ADDR         	0x0A
#define pixart_SLPETN_ADDR     		0x0B
#define pixart_SLP2_ADDR    		0x0C
#define pixart_IMGQATH_ADDR  		0x0D
#define pixart_IMGRECOG_ADDR   		0x0E
#define pixart_POWER_DOWN_ADDR          0x4B

// PixArt Read/Write configuration settings
#define pixart_WRITE      0x80
#define pixart_READ       0x00

#endif //_HAL3205_H_
#endif //_HAL3205_H_


