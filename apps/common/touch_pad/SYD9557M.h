#ifndef __SYD9557M_H__
#define __SYD9557M_H__

#include "asm/iic_hw.h"
#include "asm/iic_soft.h"

#define TOUCHPAD_NO_GESTURE                             0x00
// #define TOUCHPAD_ZOOM_IN                             0x01
// #define TOUCHPAD_ZOOM_OUT                            0x02
// #define TOUCHPAD_LIFT_CLICK                          0x03
// #define TOUCHPAD_RIGHT_CLICK                         0x04
// #define TOUCHPAD_THREE_FINGER_CLICK                  0x05
// #define TOUCHPAD_SWIPE_RIGHT_FROM_THE_LEFT_EDGE      0x06
// #define TOUCHPAD_SWIPE_LEFT_FROM_THE_RIGHT_EDGE      0x07
// #define TOUCHPAD_SWIPE_DOWN_FROM_THE_TOP_EDGE        0x08
// #define TOUCHPAD_SLIDE_UP_THE_BOTTOM_EDGE            0x09
// #define TOUCHPAD_THREE_FINGERS_TO_THE_RIGHT          0x10
// #define TOUCHPAD_THREE_FINGERS_TO_THE_LEFT           0x11
// #define TOUCHPAD_SWEEP_DOWN_WITH_THREE_FINGERS       0x12
#define TOUCHPAD_MAX_GESTURE                            0X17

void syd9557m_init(u8 iic);

#endif
