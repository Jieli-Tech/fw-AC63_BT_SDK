#ifndef __ELECTROCAR_H__
#define __ELECTROCAR_H__


#if CONFIG_APP_ELECTROCAR
#define GET_MINUS(a, b)             ((a) > (b)) ? ((a) - (b)) : ((b) - (a))

#define ELECTROCAR_DATA_LONG        4//data len

//cmd
#define APP_TO_DEVICE_IGNITION       0x01
#define APP_TO_DEVICE_SHUTDOWM       0x00
#define APP_TO_DEVICE_LOCK           0x00
#define APP_TO_DEVICE_UNLOCK         0x01

//notify
#define DEVICE_GET_MASSAGE_NOTIFY    	   (('G' << 24) | ('S' << 16) | ('N' << 8) | '\0')
#define DEVICE_433_MASSAGE_NOTIFY    	   (('4' << 24) | ('N' << 16) | ('F' << 8) | '\0')
#define DEVICE_NFC_MASSAGE_NOTIFY    	   (('N' << 24) | ('N' << 16) | ('F' << 8) | '\0')
#define DEVICE_ONEPA_MASSAGE_NOTIFY    	   (('O' << 24) | ('N' << 16) | ('F' << 8) | '\0')

// consumer key
#define CONSUMER_VOLUME_INC             0x0001
#define CONSUMER_VOLUME_DEC             0x0002
#define CONSUMER_PLAY_PAUSE             0x0004
#define CONSUMER_MUTE                   0x0008
#define CONSUMER_SCAN_PREV_TRACK        0x0010
#define CONSUMER_SCAN_NEXT_TRACK        0x0020
#define CONSUMER_SCAN_FRAME_FORWARD     0x0040
#define CONSUMER_SCAN_FRAME_BACK        0x0080

// IO Control
#define LOCK_MOTOR_PIN                  IO_PORTB_04//out---change
#define LOCK_LED_PIN                    IO_PORTB_05

//

typedef enum {
    HID_MODE_NULL = 0,
    HID_MODE_EDR,
    HID_MODE_BLE,
    HID_MODE_INIT = 0xff
} bt_mode_e;

typedef struct {
    u16 head_tag;
    u8  mode;
} hid_vm_cfg_t;

#endif

#endif//__ELECTROCAR_H_

