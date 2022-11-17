#ifndef	_LED_API_H_
#define _LED_API_H_

#if LED_UI_EN
// #include "config.h"

typedef struct __LED_DRIVER_VAR {
    u8  bLedOnOff;                  //<X 坐标
    u8  bFlashChar;                 //<字符位闪烁 bit0:速度闪  bit1:电量闪  bit2:里程闪
    u8  bFlashIcon[8];              //<图标闪烁8*8 64位
    u8  bLedCycle;                  //流水灯 流水标志
    bool bShowTotalMileage;         //总里程显示标志 显示单次里程的时候需要刷掉总里程高位
    u8  bShowBuffSpeed[2];          //<显示缓存 速度2位数  注意速度一段有两个灯
    u8  bShowBuffPower[3];          //<显示缓存 电量188 3位数
    u8  bShowBuffMileage[3];        //<显示缓存 总里程  3位数
    u8  bShowBuffIcon[8];           //<显示缓存 图标
    u8  bDriverBuff[16];            //显示缓存转换成显示驱动的控制

    u8  bBrightness;                //<亮度控制
} _LED_DRIVER_VAR;

extern _LED_DRIVER_VAR led_drvier_var;
extern const u8 LED_NUMBER[10];
extern const u8 LED_LARGE_LETTER[26];
extern const u8 LED_SMALL_LETTER[26];

void led_drvier_setX(u8 X);
void led_drvier_show_string_menu(u8 menu);
void set_LED_fade_out(void);
void set_LED_all_on(void);

void led_drvier_show_null(void);
void led_api_init(void);
void led_api_start_ui(void);
void set_LED_brightness(u8 br);
void set_LED_all_on(void);
u8 led_drvier_show_char(u8 chardata);
void led_drvier_show_string(u8 *str, u8 *buf, u8 offset);
void led_drvier_show_speed(u8 speed);
void led_drvier_show_battery_volt(u8 volt);
void led_drvier_show_battery_percent(u8 percent);
void led_drvier_show_mileage(u16 mileage);
void led_drvier_show_total_mileage(u32 mileage);
void led_drvier_show_icon_bit(u8 bit);
void led_drvier_clean_icon_bit(u8 bit);
void led_drvier_flash_icon_bit(u8 bit);
void led_drvier_clean_flash_icon_bit(u8 bit);
void led_drvier_show_led_cycle_scan(u8 ctr);
void led_drvier_led_cycle_ctr(u8 ctr);
void led_drvier_show_led_bat_level(u8 bat_level, bool flag);
void led_drvier_show_led_log(u8 ctr, bool flag);
void led_drvier_show_led_turn_l(u8 ctr, bool flag);
void led_drvier_show_led_turn_r(u8 ctr, bool flag);
void led_drvier_show_led_main_light(u8 ctr, bool flag);
void led_drvier_show_led_board_light(u8 ctr, bool flag);
void led_drvier_show_led_ready(u8 ctr, bool flag);
void led_drvier_show_led_gears(u8 ctr, bool flag);
void led_drvier_show_led_charge(u8 ctr, bool flag);
void led_drvier_show_led_cruise(u8 ctr, bool flag);
void led_drvier_show_led_fault(u8 ctr, bool flag);
void led_drvier_show_led_motor_fault(u8 ctr, bool flag);
void led_drvier_show_led_ctr_fault(u8 ctr, bool flag);
void led_drvier_show_handle_fault(u8 ctr, bool flag);
void led_drvier_show_speed_kmh(u8 ctr, bool flag);
void led_drvier_show_volt(u8 ctr, bool flag);
void led_drvier_show_percent(u8 ctr, bool flag);
void led_drvier_show_km(u8 ctr, bool flag);
void led_drvier_show_soc(u8 ctr, bool flag);
void icon_show_test(u8 ctr);


#define LED_ALL_ON_TIMES      2000
#define LED_STATUS  led_drvier_var.bShowBuff[4]

#define LED_A   BIT(0)
#define LED_B	BIT(1)
#define LED_C	BIT(2)
#define LED_D	BIT(3)
#define LED_E	BIT(4)
#define LED_F	BIT(5)
#define LED_G	BIT(6)
#define LED_H	BIT(7)

//for LED0
#define LED_PLAY	LED_A
#define LED_PAUSE	LED_B
#define LED_USB		LED_C
#define LED_SD		LED_D
#define LED_2POINT	LED_E
#define LED_MHZ		LED_F
#define LED_MP3		LED_G
#define LED_FM	    LED_H

#define LED_PORT0	BIT(0)
#define LED_PORT1	BIT(12)
#define LED_PORT2	BIT(11)
#define LED_PORT3	BIT(10)
#define LED_PORT4	BIT(6)
#define LED_PORT5	BIT(5)
#define LED_PORT6	BIT(4)

#define LED_PORT_ALL	(LED_PORT0 | LED_PORT1 | LED_PORT2 |LED_PORT3 |LED_PORT4 |LED_PORT5 |LED_PORT6)

// LED UI Control
enum {
    UI_LEFT_STEER = BIT(0),
    UI_RIGHT_STEER = BIT(1),
    UI_SPEED = BIT(2),
    UI_PARKING = BIT(3),
    UI_OUTLINE_MAKER_LAMPS = BIT(4),
    UI_BIG_LAMPS = BIT(5),
    UI_GEARS = BIT(6),
    UI_CRUISE = BIT(7),
    UI_BRAKE = BIT(8),
    UI_MOTOR_FAULT = BIT(9),
    UI_TURN_THE_FAULT = BIT(10),
    UI_CONTROL_FAULT = BIT(11),
    UI_START_UP = BIT(12),
    UI_WATERFALL_LIGHT = BIT(13),
    UI_MILEAGE = BIT(14),
    UI_OILER = BIT(15),
    UI_READY_LAMPS = BIT(16),
    UI_BLUETOOTHE_LAMPS = BIT(17),
    UI_SOC_LAMPS = BIT(18),
    UI_BATTER = BIT(19),
    UI_TOTAL_MILEAGE = BIT(20),
    UI_SHUTDOWM = BIT(21),
    UI_LOG = BIT(22),
    UI_KEY_TEST = BIT(23),
    UI_YIJIANTONG = BIT(24),
    UI_LOW_VBAT = BIT(25),

    SHAKE_PLAY = BIT(26),
    SHAKE_PLAY_2 = BIT(27),
    SHAKE_PLAY_WHEEL = BIT(28),
    KEY_INSERT_CHECK = BIT(29),
    OVERSPEED_CHECK = BIT(30),
};

/*************************************************************************************************/
/*!
 *  \brief      led api main entrance
 *
 *  \param      [in] enable     使能.
 *  \param      [in] blink      是否闪烁.
 *  \param      [in] value      速度||电量||里程等数值传入.
 *
 *  \note
 */
/*************************************************************************************************/
void led_api_main_entrance(u32 cmd, u8 enable, u8 blink, int value);


#endif
#endif	/*	_LED_API_H_	*/


