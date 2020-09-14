#include "includes.h"
#include "app_config.h"
#include "system/device/vm.h"

#if TCFG_UI_LED7_ENABLE
#include "ui/ui_api.h"
#include "led7_driver_cfg.h"

/* #define LED7_DEBUG_ENABLE */
#ifdef LED7_DEBUG_ENABLE
#define led7_debug(fmt, ...) 	printf("[LED7] "fmt, ##__VA_ARGS__)
#define led7_error(fmt, ...) 	printf("[LED7 ERR] "fmt, ##__VA_ARGS__)
#else
#define led7_debug(...)
#define led7_error(...)
#endif


//#define  UI_LED7_TRUE_TABLE1
//#define  UI_LED7_TRUE_TABLE2
#define  UI_LED7_TRUE_TABLE3

struct ui_led7_env {
    u8 init;
    LED7_VAR led7_var;
    const struct led7_platform_data *user_data;
    u32 flash_time;
    u8 lock: 1;
};

static struct ui_led7_env _led7_env = {0};

#define __this 		(&_led7_env)

//数字'0' ~ '9'显示段码表
static const  u8 LED7_NUMBER_2_SEG[10] = {
    (u8)(LED_A | LED_B | LED_C | LED_D | LED_E | LED_F), 		 //'0'
    (u8)(LED_B | LED_C), 										 //'1'
    (u8)(LED_A | LED_B | LED_D | LED_E | LED_G), 				 //'2'
    (u8)(LED_A | LED_B | LED_C | LED_D | LED_G),  				 //'3'
    (u8)(LED_B | LED_C | LED_F | LED_G),						 //'4'
    (u8)(LED_A | LED_C | LED_D | LED_F | LED_G), 				 //'5'
    (u8)(LED_A | LED_C | LED_D | LED_E | LED_F | LED_G), 		 //'6'
    (u8)(LED_A | LED_B | LED_C), 								 //'7'
    (u8)(LED_A | LED_B | LED_C | LED_D | LED_E | LED_F | LED_G), //'8'
    (u8)(LED_A | LED_B | LED_C | LED_D | LED_F | LED_G), 		 //'9'
};

//字母'A' ~ 'Z'显示段码表
static const  u8 LED7_LARGE_LETTER_2_SEG[26] = {
    0x77, 0x40, 0x39, 0x3f, 0x79, ///<ABCDE
    0x71, 0x40, 0x76, 0x06, 0x40, ///<FGHIJ
    0x40, 0x38, 0x40, 0x37, 0x3f, ///<KLMNO
    0x73, 0x40, 0x50, 0x6d, 0x78, ///<PQRST
    0x3e, 0x3e, 0x40, 0x76, 0x40, ///<UVWXY
    0x40 ///<Z
};

//字母'a' ~ 'z'显示段码表
static const  u8 LED7_SMALL_LETTER_2_SEG[26] = {
    0x77, 0x7c, 0x58, 0x5e, 0x79, ///<abcde
    0x71, 0x40, 0x40, 0x40, 0x40, ///<fghij
    0x40, 0x38, 0x40, 0x54, 0x5c, ///<klmno
    0x73, 0x67, 0x50, 0x40, 0x78, ///<pqrst
    0x3e, 0x3e, 0x40, 0x40, 0x40, ///<uvwxy
    0x40 ///<z
};

/*----------------------------------------------------------------------------*/
/**@brief  显示图标类
   @param   void
   @return  void
   @author  Change.tsai
   @note    void ui_led7_show_icon(UI_LED7_ICON icon)
*/
/*----------------------------------------------------------------------------*/
void led7_show_icon(UI_LED7_ICON icon)
{
    __this->led7_var.bShowIcon |= icon;
    __this->led7_var.bFlashIcon &= (~icon); //stop display
}

/*----------------------------------------------------------------------------*/
/**@brief  显示图标类
   @param   void
   @return  void
   @author  Change.tsai
   @note    void ui_led7_flash_icon(UI_LED7_ICON icon)
*/
/*----------------------------------------------------------------------------*/
void led7_flash_icon(UI_LED7_ICON icon)
{
    __this->led7_var.bFlashIcon |= icon;
    __this->led7_var.bShowIcon &= (~icon); //stop display
}

/*----------------------------------------------------------------------------*/
/**@brief   led7_drv 状态位缓存清除函数
   @param   void
   @return  void
   @author  Change.tsai
   @note    void led7_clear_icon(void)
*/
/*----------------------------------------------------------------------------*/
void led7_clear_icon(UI_LED7_ICON icon)
{
    __this->led7_var.bShowIcon &= (~icon);
    __this->led7_var.bFlashIcon &= (~icon);
}


/*----------------------------------------------------------------------------*/
/**@brief   led7_drv 清楚所有图标显示
   @param   void
   @return  void
   @author  Change.tsai
   @note    void led7_clear_icon(void)
*/
/*----------------------------------------------------------------------------*/
void led7_clear_all_icon(void)
{
    __this->led7_var.bFlashIcon = 0;
    __this->led7_var.bShowIcon = 0;
    __this->flash_time = jiffies_to_msecs(jiffies);
}


/*----------------------------------------------------------------------------*/
/**@brief   led7_drv 显示坐标设置
   @param   x：显示横坐标
   @return  void
   @author  Change.tsai
   @note    void led7_setX(u8 X)
*/
/*----------------------------------------------------------------------------*/
void led7_setX(u8 X)
{
    __this->led7_var.bCoordinateX = X;
}

/*----------------------------------------------------------------------------*/
/**@brief   led7_drv 清除显示数字和字母
   @param 	void
   @return  void
   @author  Change.tsai
   @note    void led7_clear_string(void)
*/
/*----------------------------------------------------------------------------*/
void led7_clear_string(void)
{
    memset(__this->led7_var.bShowBuff, 0x00, 4);
    __this->led7_var.bFlashChar = 0;
    led7_setX(0);
}


/*----------------------------------------------------------------------------*/
/**@brief   LED 清屏函数
   @param   void
   @return  void
   @author  Change.tsai
   @note    void led7_show_null(void)
*/
/*----------------------------------------------------------------------------*/
void led7_show_null(void)
{
    led7_clear_all_icon();
    led7_clear_string();
}


/*----------------------------------------------------------------------------*/
/**@brief   led7_drv 单个字符显示函数
   @param   chardata：显示字符
   @return  void
   @author  Change.tsai
   @note    void led7_show_char(u8 chardata)
   @ display:
	   ___    ___    ___    ___
	  |___|  |___|  |___|  |___|
	  |___|  |___|  |___|  |___|
	 ---0------1------2------3------> X
*/
/*----------------------------------------------------------------------------*/
void led7_show_char(u8 chardata)
{
    if (__this->led7_var.bCoordinateX >= 4) {
        __this->led7_var.bCoordinateX = 0; //or return
        //return ;
    }
    if ((chardata >= '0') && (chardata <= '9')) {
        __this->led7_var.bShowBuff[__this->led7_var.bCoordinateX++] = LED7_NUMBER_2_SEG[chardata - '0'];
    } else if ((chardata >= 'a') && (chardata <= 'z')) {
        __this->led7_var.bShowBuff[__this->led7_var.bCoordinateX++] = LED7_SMALL_LETTER_2_SEG[chardata - 'a'];
    } else if ((chardata >= 'A') && (chardata <= 'Z')) {
        __this->led7_var.bShowBuff[__this->led7_var.bCoordinateX++] = LED7_LARGE_LETTER_2_SEG[chardata - 'A'];
    } else if (chardata == ':') {
        __this->led7_var.bShowIcon |= LED7_2POINT;
        __this->led7_var.bFlashIcon &= (~LED7_2POINT);
    } else if (chardata == '.') {
        __this->led7_var.bShowIcon |= LED7_DOT;
        __this->led7_var.bFlashIcon &= (~LED7_DOT);
    } else if (chardata == ' ') {
        __this->led7_var.bShowBuff[__this->led7_var.bCoordinateX++] = 0;
    } else {
        __this->led7_var.bShowBuff[__this->led7_var.bCoordinateX++] = LED_G; //显示'-'
    }
}


/*----------------------------------------------------------------------------*/
/**@brief   led7_drv 某一位字符闪烁
   @param   index
   @return  void
   @author  Change.tsai
   @note    void led7_flash_char_start(u8 index)
*/
/*----------------------------------------------------------------------------*/
void led7_flash_char_start(u8 index)
{
    if (index < 4) {
        __this->led7_var.bFlashChar |= BIT(index);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   led7_drv 某一位取消字符闪烁
   @param   index
   @return  void
   @author  Change.tsai
   @note    void led7_flash_char_stop(u8 index)
*/
/*----------------------------------------------------------------------------*/
void led7_flash_char_stop(u8 index)
{
    if (index < 4) {
        __this->led7_var.bFlashChar &= ~BIT(index);
    }
}


/*----------------------------------------------------------------------------*/
/**@brief   led7_drv 字符串显示函数, 默认左对齐, 从x = 0开始显示
   @param   *str：字符串的指针   offset：显示偏移量
   @return  void
   @author  Change.tsai
   @note    void led7_show_string_left(u8 *str)
*/
/*----------------------------------------------------------------------------*/
void led7_show_string_reset_x(u8 *str)
{
    led7_clear_string();
    while (*str != '\0') {
        led7_show_char(*str++);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   led7_drv 字符串显示函数, 默认左对齐, 追加形式
   @param   *str：字符串的指针   offset：显示偏移量
   @return  void
   @author  Change.tsai
   @note    void led7_show_string_left(u8 *str)
*/
/*----------------------------------------------------------------------------*/
void led7_show_string(u8 *str)
{
    while (*str != '\0') {
        led7_show_char(*str++);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   led7_drv 字符串显示函数, 左对齐, 清屏
   @param   *str：字符串的指针
   @return  void
   @author  Change.tsai
   @note    void led7_show_string_left(u8 *str)
*/
/*----------------------------------------------------------------------------*/
void led7_show_string_align_left(u8 *str)
{
    led7_show_string_reset_x(str);
}


/*----------------------------------------------------------------------------*/
/**@brief   led7_drv 字符串显示函数, 右对齐, 清屏
   @param   *str：字符串的指针
   @return  void
   @author  Change.tsai
   @note    void led7_show_string_left(u8 *str)
*/
/*----------------------------------------------------------------------------*/
void led7_show_string_align_right(u8 *str)
{
    u8 cnt = 0;
    u8 *_str = str;
    while (*_str++ != '\0') {
        cnt++;
    }
    if (cnt > 4) {
        led7_error("show string oversize", __func__);
        return;
    }
    led7_clear_string();
    led7_setX(4 - cnt);
    while (*str != '\0') {
        led7_show_char(*str++);
    }
}


/*----------------------------------------------------------------------------*/
/**@brief   数字显示函数, 默认高位显示0
   @param   void
   @return  void
   @author  Change.tsai
   @note    void led7_show_number(u8 val)
*/
/*----------------------------------------------------------------------------*/
void led7_show_number(u16 val)
{
    u8 tmp_buf[5] = {0};

    itoa4(val, tmp_buf);
    led7_show_string_reset_x(tmp_buf);
}



/*----------------------------------------------------------------------------*/
/**@brief   数字显示函数, 高位不显示0
   @param   val, 显示数字
   @return  void
   @author  Change.tsai
   @note    void led7_show_number2(u8 val)
*/
/*----------------------------------------------------------------------------*/
void led7_show_number2(u16 val)
{
    u8 i;
    u8 tmp_buf[5] = {0};

    itoa4(val, tmp_buf);
    for (i = 0; i < 3; i++) {
        if (tmp_buf[i] != '0') {
            break;
        }
    }
    led7_show_string_align_right((u8 *) & (tmp_buf[i]));
}

/*----------------------------------------------------------------------------*/
/**@brief   数字显示函数(追加方式)
   @param   val, 显示数字
   @return  void
   @author  Change.tsai
   @note    void led7_show_number_add(u8 val)
*/
/*----------------------------------------------------------------------------*/
void led7_show_number_add(u16 val)
{
    u8 i;
    u8 tmp_buf[5] = {0};

    itoa4(val, tmp_buf);
    for (i = 0; i < 3; i++) {
        if (tmp_buf[i] != '0') {
            break;
        }
    }
    led7_show_string((u8 *)&tmp_buf[i]);
}


static u8 __ui_led7_get_sys_halfsec(void)
{
    if (((jiffies_to_msecs(jiffies) - __this->flash_time) & 999) > 500) {
        return true;
    } else {
        return false;
    }
}

static void __ui_led7_update_bShowbuf1(void)
{
    u8 k, i, j, temp;
    k = 0;

    __this->led7_var.bShowBuff1[0] = 0; //if pin0 output 1, other output 0/1
    __this->led7_var.bShowBuff1[1] = 0;
    __this->led7_var.bShowBuff1[2] = 0;
    __this->led7_var.bShowBuff1[3] = 0;
    __this->led7_var.bShowBuff1[4] = 0;
    __this->led7_var.bShowBuff1[5] = 0;
    __this->led7_var.bShowBuff1[6] = 0;


    //digit display
    for (i = 0; i < 4; i++) {
        temp = __this->led7_var.bShowBuff[i];
        for (j = 0; j < 7; j++) {
            if (temp & BIT(j)) {
                //the j seg should display
                //look up the seg2pin table, set the pin should be output 0
                __this->led7_var.bShowBuff1[led7_digit_seg2pin[k].pinH] |= BIT(led7_digit_seg2pin[k].pinL);
            }
            k++; //max for 4 * 7
        }
    }
    //char flash
    if (__ui_led7_get_sys_halfsec()) { //500ms display, or char off
        if (__this->led7_var.bFlashChar) {
            for (j = 0; j < 4; j++) {
                if (BIT(j) & __this->led7_var.bFlashChar) { //某一位不显示
                    //look up the seg2pin table, set the pin should be output 0
                    for (i = 0; i < 7; i++) {
                        __this->led7_var.bShowBuff1[led7_digit_seg2pin[7 * j + i].pinH] &= ~(BIT(led7_digit_seg2pin[7 * j + i].pinL));
                    }
                }
            }
        }
    }
    //icon display
    if (__this->led7_var.bShowIcon) {
        for (j = 0; j < 32; j++) {
            if (BIT(j) & __this->led7_var.bFlashIcon) {
                continue;
            }
            if (BIT(j) & __this->led7_var.bShowIcon) {
                for (i = 0; i < ARRAY_SIZE(led7_icon_seg2pin); i++) { //lookup icon exist
                    if (BIT(j) == led7_icon_seg2pin[i].icon) {
                        //look up the seg2pin table, set the pin should be output 0
                        __this->led7_var.bShowBuff1[led7_icon_seg2pin[i].seg2pin.pinH] |= BIT(led7_icon_seg2pin[i].seg2pin.pinL);
                    }
                }
            }
        }
    }
    //icon flash
    if (__this->led7_var.bFlashIcon) {
        if (__ui_led7_get_sys_halfsec()) { //500ms display, or seg off
            for (j = 0; j < 32; j++) {
                if (BIT(j) & __this->led7_var.bFlashIcon) {
                    for (i = 0; i < ARRAY_SIZE(led7_icon_seg2pin); i++) { //lookup icon exist
                        if (BIT(j) == led7_icon_seg2pin[i].icon) {
                            //look up the seg2pin table, set the pin should be output 0
                            __this->led7_var.bShowBuff1[led7_icon_seg2pin[i].seg2pin.pinH] |= BIT(led7_icon_seg2pin[i].seg2pin.pinL);
                        }
                    }
                }
            }
        }
    }
}


/*----------------------------------------------------------------------------*/
/**@brief   把所有IO设置为高阻
   @param   x：显示横坐标
   @return  void
   @author  Change.tsai
   @note    void led7_clear(void)
*/
/*----------------------------------------------------------------------------*/
static void __ui_led7_port_set_hz(u8 port)
{
    gpio_set_pull_down(port, 0);
    gpio_set_pull_up(port, 0);
    gpio_set_direction(port, 1);
}

/*----------------------------------------------------------------------------*/
/**@brief   LED清屏函数, 把所有IO设置为高阻
   @param   x：显示横坐标
   @return  void
   @author  Change.tsai
   @note    void led7_clear(void)
*/
/*----------------------------------------------------------------------------*/
static void __ui_led7_clear_all(void)
{
    u8 port = 0;
    u8 i;

    if (__this->user_data->pin_type == LED7_PIN7) {
        for (i = 0; i < 7; i++) {
            port = __this->user_data->pin_cfg.pin7.pin[i];
            __ui_led7_port_set_hz(port);
        }
    }
    if (__this->user_data->pin_type == LED7_PIN12) {
        for (i = 0; i < 7; i++) {
            port = __this->user_data->pin_cfg.pin12.pin_segl[i];
            __ui_led7_port_set_hz(port);
        }
        for (i = 0; i < 5; i++) {
            port = __this->user_data->pin_cfg.pin12.pin_comh[i];
            __ui_led7_port_set_hz(port);
        }
    }

    if (__this->user_data->pin_type == LED7_PIN13) {
        for (i = 0; i < 7; i++) {
            port = __this->user_data->pin_cfg.pin13.pin_seg[i];
            __ui_led7_port_set_hz(port);
        }
        for (i = 0; i < 6; i++) {
            port = __this->user_data->pin_cfg.pin13.pin_com[i];
            __ui_led7_port_set_hz(port);
        }
    }

}

__attribute__((weak)) u8 led7_skip_vm_flag(void)
{
    return 1;
}

/*----------------------------------------------------------------------------*/
/**@brief   LED扫描函数
   @param   void
   @return  void
   @author  Change.tsai
   @note    void led7_scan(void *param)
*/
/*----------------------------------------------------------------------------*/
static void __led7_scan(void *param)
{
    static u8 cnt = 0;
    u8 seg, i;

    if (__this->init == false) {
        putchar('r');
        return;
    }

    __ui_led7_clear_all();

    if (get_vm_statu() && led7_skip_vm_flag()) {
        return;
    }

    if (!cnt && !__this->lock) {
        __ui_led7_update_bShowbuf1();
    }

    seg = __this->led7_var.bShowBuff1[cnt];

    if (__this->user_data->pin_type == LED7_PIN7) {
        //pin cnt output H
        gpio_direction_output(__this->user_data->pin_cfg.pin7.pin[cnt], 1);
        for (i = 0; i < 7; i++) {
            if (seg & BIT(i)) {
                //pin i output L
                gpio_direction_output(__this->user_data->pin_cfg.pin7.pin[i], 0);
            }
        }
        cnt = (cnt >= 6) ? 0 : cnt + 1;
    } else if (__this->user_data->pin_type == LED7_PIN13) {
        //pin_comh  cnt output L
        gpio_direction_output(__this->user_data->pin_cfg.pin13.pin_com[cnt], 0);
        for (i = 0; i < 7; i++) {
            if (seg & BIT(i)) {
                //pin_segl i output H
                gpio_direction_output(__this->user_data->pin_cfg.pin13.pin_seg[i], 1);
            }
        }
        cnt = (cnt >= 5) ? 0 : cnt + 1;
    } else {
        //pin_comh  cnt output H
        gpio_direction_output(__this->user_data->pin_cfg.pin12.pin_comh[cnt], 1);
        for (i = 0; i < 7; i++) {
            if (seg & BIT(i)) {
                //pin_segl i output L
                gpio_direction_output(__this->user_data->pin_cfg.pin12.pin_segl[i], 0);
            }
        }
        cnt = (cnt >= 4) ? 0 : cnt + 1;
    }
}



static void led7_setXY(u32 x, u32 y)
{
    led7_setX(x);
}

static void  led7_Flashchar(u32 arg)
{
    __this->led7_var.bFlashChar |= arg;
}

static void  led7_Clear_Flashchar(u32 arg)
{
    __this->led7_var.bFlashChar &= (~arg);
}


static void led7_show_pic(u32 arg)
{

}

static void led7_hide_pic(u32 arg)
{

}

static void led7_show_one_number(u8 number)
{
    led7_show_char(number + '0');
}

static void led7_show_lock(u32 en)
{
    __this->lock = !!en;
}

static LCD_API LED7_HW = {
    .clear             = led7_show_null,
    .setXY             = led7_setXY,
    .FlashChar         = led7_Flashchar,
    .Clear_FlashChar   = led7_Clear_Flashchar,
    .show_icon         = led7_show_icon,
    .flash_icon        = led7_flash_icon,
    .clear_icon        = led7_clear_icon,
    .show_string       = led7_show_string,
    .show_char         = led7_show_char,
    .show_number       = led7_show_one_number,
    .show_pic          = led7_show_pic,
    .hide_pic          = led7_hide_pic,
    .lock              = led7_show_lock,
};




/*----------------------------------------------------------------------------*/
/**@brief   led7段数码管初始化
   @param   void
   @return  void
   @author  Change.tsai
   @note    void led7_init(void)
*/
/*----------------------------------------------------------------------------*/
void *led7_init(const struct led7_platform_data *_data)
{
    led7_debug("%s", __func__);
    memset(__this, 0x00, sizeof(struct ui_led7_env));

    if (_data == NULL) {
        return NULL;
    }

    __this->user_data = _data;

    __ui_led7_clear_all();
    sys_hi_timer_add(NULL, __led7_scan, 2); //2ms

    __this->init = true;
    return (void *)(&LED7_HW);
}


//=================================================================================//
//                        		For Test Module                    				   //
//=================================================================================//
#if 0 //for test code
/*
LED7_PLATFORM_DATA_BEGIN(led7_test_data)
	.pin_type = LED7_PIN7,
	.pin_cfg.pin7.pin[0] = IO_PORTB_00,
	.pin_cfg.pin7.pin[1] = IO_PORTB_01,
	.pin_cfg.pin7.pin[2] = IO_PORTB_02,
	.pin_cfg.pin7.pin[3] = IO_PORTB_03,
	.pin_cfg.pin7.pin[4] = IO_PORTB_04,
	.pin_cfg.pin7.pin[5] = IO_PORTB_05,
	.pin_cfg.pin7.pin[6] = IO_PORTB_06,
LED7_PLATFORM_DATA_END()
*/

LED7_PLATFORM_DATA_BEGIN(led7_test_data)
.pin_type = LED7_PIN12,
 .pin_cfg.pin12.pin_segl[0] = IO_PORTB_00,
                              .pin_cfg.pin12.pin_segl[1] = IO_PORTB_01,
                                      .pin_cfg.pin12.pin_segl[2] = IO_PORTB_02,
                                              .pin_cfg.pin12.pin_segl[3] = IO_PORTB_03,
                                                      .pin_cfg.pin12.pin_segl[4] = IO_PORTB_04,
                                                              .pin_cfg.pin12.pin_segl[5] = IO_PORTB_05,
                                                                      .pin_cfg.pin12.pin_segl[6] = IO_PORTB_06,

                                                                              .pin_cfg.pin12.pin_comh[0] = IO_PORTA_00,
                                                                                      .pin_cfg.pin12.pin_comh[1] = IO_PORTB_08,
                                                                                              .pin_cfg.pin12.pin_comh[2] = IO_PORTB_09,
                                                                                                      .pin_cfg.pin12.pin_comh[3] = IO_PORTB_10,
                                                                                                              .pin_cfg.pin12.pin_comh[4] = IO_PORTA_01,
                                                                                                                      LED7_PLATFORM_DATA_END()

                                                                                                                      void led7_scan_test(void)
{
    __ui_led7_scan(NULL);
}

#define LED7_RTC_DISPLAY_TEST 		1
#define LED7_ICON_DISPLAY_TEST 		1
#define LED7_ICON_FLASH_TEST 		0
#define LED7_SHOW_NUMBER_TEST 		0
#define LED7_SHOW_STRING_TEST 		0
#define LED7_SHOW_MODE_MENU_TEST 	0

void led7_test()
{
    if (__this->init == false) {
        ui_led7_init(&led7_test_data); //init
    }
#if LED7_RTC_DISPLAY_TEST
    //test1: rtc display
    static u8 Hour = 18;
    static u8 Min = 52;
    static u8 Sec = 0;

    Sec++;
    if (Sec >= 60) {
        Min++;
        Sec = 0;
    }
    if (Min >= 60) {
        Hour++;
        Min = 0;
    }
    if (Hour >= 24) {
        Hour = 0;
    }
    led7_debug("%d : %d", Hour, Min);
    ui_led7_show_RTC_time(Hour, Min);
#endif /* #if LED7_RTC_DISPLAY_TEST */

#if LED7_ICON_DISPLAY_TEST
    ui_led7_show_icon(LED7_PLAY);
    ui_led7_show_icon(LED7_PAUSE);
    ui_led7_show_icon(LED7_USB); //USB
    ui_led7_show_icon(LED7_SD); //SD
    ui_led7_show_icon(LED7_2POINT); //
    ui_led7_show_icon(LED7_FM);
    ui_led7_show_icon(LED7_DOT); //
    ui_led7_show_icon(LED7_MP3);
    ui_led7_show_icon(LED7_REPEAT); //
    ui_led7_show_icon(LED7_CHARGE); //
    ui_led7_show_icon(LED7_BT); //
    ui_led7_show_icon(LED7_AUX); //
    ui_led7_show_icon(LED7_WMA); //
#endif /* #if LED7_ICON_DISPLAY_TEST */

#if LED7_ICON_FLASH_TEST
    ui_led7_flash_icon(LED7_PLAY);
    ui_led7_flash_icon(LED7_PAUSE);
    ui_led7_flash_icon(LED7_USB);
    ui_led7_flash_icon(LED7_SD);
    ui_led7_flash_icon(LED7_2POINT);
    ui_led7_flash_icon(LED7_FM);
    ui_led7_flash_icon(LED7_DOT);
    ui_led7_flash_icon(LED7_MP3);
    ui_led7_flash_icon(LED7_REPEAT);
    ui_led7_flash_icon(LED7_CHARGE);
    ui_led7_flash_icon(LED7_BT);
    ui_led7_flash_icon(LED7_AUX);
    ui_led7_flash_icon(LED7_WMA);
#endif /* #if LED7_ICON_FLASH_TEST */

#if LED7_SHOW_NUMBER_TEST
    static u8 cnt = 0;
    if (!(cnt % 5)) {
        ui_led7_show_number(123); //高位显示0
    } else {
        ui_led7_show_number2(123); //高位不显示0
    }
    if (cnt == 0) {
        ui_led7_setX(0);
    }
    ui_led7_show_number_add(cnt++);
#endif

#if LED7_SHOW_STRING_TEST
    /* static u8 cnt = '0'; */
    /* if (cnt < ('z' + 1)) { */
    /* ui_led7_show_char(cnt++); //显示数字 */
    /* } else { */
    /* cnt = '0'; */
    /* } */

    /* u8 buf[3] = {0}; */
    /* if (cnt < 'z') { */
    /* buf[0] = cnt; */
    /* buf[1] = cnt + 1; */
    /* buf[2] = '\0'; */
    /* ui_led7_show_string(buf); //显示字符串 */
    /* cnt++; */
    /* } else { */
    /* cnt = '0'; */
    /* } */
#endif

#if LED7_SHOW_MODE_MENU_TEST
    static u8 index = 0;
    /* ui_led7_show_string_menu(index++); */
    /* if (index > ARRAY_SIZE(menu_string_table)) { */
    /* index = 0; */
    /* } */

    //ui_led7_show_music_play_time(index++);
    /* ui_led7_show_volume(index++); */
    /* if (index > 99) { */
    /* index = 0; */
    /* } */
    ui_led7_show_fm_station(index++);
#endif
}

#endif /* for test code mcro */


#endif /* #if TCFG_UI_LED7_ENABLE */
