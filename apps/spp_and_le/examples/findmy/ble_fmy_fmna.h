// binary representation
// attribute size in bytes (16), flags(16), handle (16), uuid (16/128), value(...)

#ifndef _BLE_FMY_FMMA_H
#define _BLE_FMY_FMMA_H

#include <stdint.h>
#include "app_config.h"

#define  FMY_DEBUG_TEST_LOW_ADV_TO_FASE          0 //for debug test

#if FMY_DEBUG_TEST_LOW_ADV_TO_FASE
#define  FMY_ADV_SLOW_INTERVAL                   0xa0  //for test
#else
#define  FMY_ADV_SLOW_INTERVAL                   0xC80  //fixed 2 seconds,bqb
#endif

#define  FMY_ADV_PAIR_TIMEOUT_MS                 (10*60*1000L) //10 min

// Separated fast advertising is used for UT finding, e.g. aggressive adv.
#define  FMY_FMNA_SEPARATED_ADV_FAST_INTERVAL    0x30          /**< Fast advertising interval 30ms (in units of 0.625 ms.) */
#define  FMY_FMNA_SEPARATED_ADV_SLOW_INTERVAL    FMY_ADV_SLOW_INTERVAL  /**< Slow advertising interval 2 seconds (in units of 0.625 ms.) */

// Nearby fast advertising is used for leash break.
#define  FMY_FMNA_NEARBY_ADV_FAST_INTERVAL      0x30          /**< Fast advertising interval 30ms (in units of 0.625 ms.) */
#define  FMY_FMNA_NEARBY_ADV_SLOW_INTERVAL      FMY_ADV_SLOW_INTERVAL  /**< Slow advertising interval 2 seconds (in units of 0.625 ms.) */

#define  FMY_FMNA_PAIRING_ADV_FAST_INTERVAL     0x30          /**< Fast advertising interval 30ms (in units of 0.625 ms.) */
#define  FMY_FMNA_PAIRING_ADV_SLOW_INTERVAL     0x30          /**< Slow advertising interval 30ms (in units of 0.625 ms.) */

#define  PORT_DIR_OUPUT                         0
#define  PORT_DIR_INPUT                         1
#define  PORT_VALUE_HIGH                        1
#define  PORT_VALUE_LOW                         0

#define  DEV_OPEN_LED_STATE()                  gpio_write(LED_PAIR_GAPIO_POR,PORT_VALUE_HIGH);
#define  DEV_CLOSE_LED_STATE()                 gpio_write(LED_PAIR_GAPIO_POR,PORT_VALUE_LOW);

#if !SOUND_PASSIVE_BUZZER
#define DEV_SOUND_STATE(ON_OFF)                gpio_write(SOUND_GPIO_PORT, ON_OFF);
#define DEV_SOUND_INIT()                       port_io_init(SOUND_GPIO_PORT, PORT_DIR_OUPUT);
#else
#define DEV_SOUND_STATE(ON_OFF)                fmy_sound_passive_onoff(DEV_SOUND_PWM_CH, ON_OFF);
#define DEV_SOUND_INIT()                       fmy_sound_passive_buzzer_init(SOUND_GPIO_PORT, DEV_SOUND_PWM_CH);
#endif

#define SOUND_TICKS_MS                  (500)
//***********************************************************************************************
void fmy_fmna_adv_cofig_init(void);
void fmy_fmna_init(void);
void fmy_pairing_timeout_start(void);
void fmy_pairing_timeout_stop(void);
void fmy_systerm_reset(uint32_t delay_ticks);
void fmy_ble_key_event_handler(uint8_t event_type, uint8_t key_value);

#endif
