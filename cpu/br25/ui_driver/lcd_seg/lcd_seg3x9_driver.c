#include "includes.h"
#include "app_config.h"


#if TCFG_UI_LCD_SEG3X9_ENABLE
#include "ui/ui_api.h"
#include "lcd_seg3x9_driver_cfg.h"

//#define LCD_SEG3X9_DEBUG_ENABLE
#ifdef LCD_SEG3X9_DEBUG_ENABLE
#define lcd_seg3x9_debug(fmt, ...) 	printf("[LED_SEG3X9] "fmt, ##__VA_ARGS__)
#else
#define lcd_seg3x9_debug(...)
#endif /* #ifdef LCD_SEG3X9_DEBUG_ENABLE */

#define lcd_seg3x9_err_error(fmt, ...) 	printf("[LCD_SEG3X9 ERR] "fmt, ##__VA_ARGS__)

struct ui_lcd_seg3x9_env {
    u8 init;
    LCD_SEG3X9_VAR lcd_seg_var;
    const struct lcd_seg3x9_platform_data *user_data;
    u32 flash_timer; //500ms亮灭, 周期时钟
};

static struct ui_lcd_seg3x9_env _lcd_seg_env = {0};
#define __this 		(&_lcd_seg_env)

#define JL_LCDC JL_LCD

#define LED_A   BIT(0)
#define LED_B	BIT(1)
#define LED_C	BIT(2)
#define LED_D	BIT(3)
#define LED_E	BIT(4)
#define LED_F	BIT(5)
#define LED_G	BIT(6)
#define LED_H	BIT(7)
//数字'0' ~ '9'显示段码表
static const  u8 LCD_SEG_NUMBER_2_SEG[10] = {
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
static const  u8 LCD_SEG_LARGE_LETTER_2_SEG[26] = {
    0x77, 0x40, 0x39, 0x3f, 0x79, ///<ABCDE
    0x71, 0x40, 0x76, 0x06, 0x40, ///<FGHIJ
    0x40, 0x38, 0x40, 0x37, 0x3f, ///<KLMNO
    0x73, 0x40, 0x50, 0x6d, 0x78, ///<PQRST
    0x3e, 0x3e, 0x40, 0x76, 0x40, ///<UVWXY
    0x40 ///<Z
};

//字母'a' ~ 'z'显示段码表
static const  u8 LCD_SEG_SMALL_LETTER_2_SEG[26] = {
    0x77, 0x7c, 0x58, 0x5e, 0x79, ///<abcde
    0x71, 0x40, 0x40, 0x40, 0x40, ///<fghij
    0x40, 0x38, 0x40, 0x54, 0x5c, ///<klmno
    0x73, 0x67, 0x50, 0x40, 0x78, ///<pqrst
    0x3e, 0x3e, 0x40, 0x40, 0x40, ///<uvwxy
    0x40 ///<z
};

//=================================================================================//
//                        		与IC芯片相关配置 								   //
//=================================================================================//
struct HW_PIN2SEG {
    enum HW_SEG_INDEX seg_index;
    u8 pin_index;
};

struct HW_PIN2COM {
    enum HW_COM_INDEX com_index;
    u8 pin_index;
};

static const struct HW_PIN2SEG hw_pin2seg_mapping[22] = {
    {LCD_SEG0, IO_PORTA_00},
    {LCD_SEG1, IO_PORTA_01},
    {LCD_SEG2, IO_PORTA_02},
    {LCD_SEG3, IO_PORTA_03},
    {LCD_SEG4, IO_PORTA_04},
    {LCD_SEG5, IO_PORTA_05},
    {LCD_SEG6, IO_PORTA_06},
    {LCD_SEG7, IO_PORTA_07},
    {LCD_SEG8, IO_PORTA_08},
    {LCD_SEG9, IO_PORTA_09},
    {LCD_SEG10, IO_PORTA_10},
    {LCD_SEG11, IO_PORTA_11},
    {LCD_SEG12, IO_PORTA_12},
    {LCD_SEG13, IO_PORTA_13},
    {LCD_SEG14, IO_PORTA_14},
    {LCD_SEG15, IO_PORTA_15},

    {LCD_SEG16, IO_PORTC_00},
    {LCD_SEG17, IO_PORTC_01},
    {LCD_SEG18, IO_PORTB_10},
    {LCD_SEG19, IO_PORTD_07},
    {LCD_SEG20, IO_PORTC_06},
    {LCD_SEG21, IO_PORTC_07},
};

static const struct HW_PIN2COM hw_pin2com_mapping[6] = {
    {LCD_COM0, IO_PORTC_05},
    {LCD_COM1, IO_PORTC_04},
    {LCD_COM2, IO_PORTC_03},
    {LCD_COM3, IO_PORTC_02},
    {LCD_COM4, IO_PORTC_01},
    {LCD_COM5, IO_PORTC_00},
};

static enum HW_COM_INDEX __match_com_index(u8 gpio)
{
    for (u8 i = 0; i < 3; i++) {
        if (hw_pin2com_mapping[i].pin_index == gpio) {
            return hw_pin2com_mapping[i].com_index;
        }
    }

    return 0;
}

//LCD COM & SEG
static void __lcd_seg3x9_show_segN(enum HW_COM_INDEX com, enum HW_SEG_INDEX seg, u8 value)
{
    enum HW_COM_INDEX com_index;

    com_index = __match_com_index(__this->user_data->pin_cfg.pin_com[com]); //To CHIP connect

    //SEG -- COM0/1/2
    //IO  -- DIE/HD/PD
    //COM, SEG
    switch (com_index) {
    case LCD_COM0:
        gpio_set_die(__this->user_data->pin_cfg.pin_seg[seg], value);
        break;
    case LCD_COM1:
        gpio_set_hd0(__this->user_data->pin_cfg.pin_seg[seg], value);
        break;
    case LCD_COM2:
        gpio_set_pull_down(__this->user_data->pin_cfg.pin_seg[seg], value);
        break;
    default:
        break;
    }
}


static void __lcd_seg3x9_reflash_char(u8 index)
{
    //show char
    if (index >= 3) {
        return;
    }

    if (BIT(index) & __this->lcd_seg_var.bFlashChar) {
        return;
    }

    for (u8 i = 0; i < 7; i++) {
        if (__this->lcd_seg_var.bShowBuff[index] & BIT(i)) {
            __lcd_seg3x9_show_segN(lcd_seg3x9_seg2pin[i + index * 7].com_index,
                                   lcd_seg3x9_seg2pin[i + index * 7].seg_index, 1);
        } else {
            __lcd_seg3x9_show_segN(lcd_seg3x9_seg2pin[i + index * 7].com_index,
                                   lcd_seg3x9_seg2pin[i + index * 7].seg_index, 0);
        }
    }
}

static void __lcd_seg3x9_reflash_string()
{
    u8 j = 0;
    for (j = 0; j < 3; j++) {
        __lcd_seg3x9_reflash_char(j);
    }
}

//刷新icon常亮显示
static void __lcd_seg3x9_reflash_icon()
{
    //show icon
    u8 i = 0;
    u8 j = 0;
    if (__this->lcd_seg_var.bShowIcon) {
        for (j = 0; j < 32; j++) {
            if (BIT(j) & __this->lcd_seg_var.bFlashIcon) {
                continue;
            }
            if (BIT(j) & __this->lcd_seg_var.bShowIcon) {
                for (i = 0; i < ARRAY_SIZE(lcd_seg3x9_icon_seg2pin); i++) { //lookup icon exist
                    if (BIT(j) == lcd_seg3x9_icon_seg2pin[i].icon) {
                        //look up the seg2pin table, set the pin should be output 0
                        __lcd_seg3x9_show_segN(lcd_seg3x9_icon_seg2pin[i].seg2pin.com_index,
                                               lcd_seg3x9_icon_seg2pin[i].seg2pin.seg_index, 1);
                    } else {
                        __lcd_seg3x9_show_segN(lcd_seg3x9_icon_seg2pin[i].seg2pin.com_index,
                                               lcd_seg3x9_icon_seg2pin[i].seg2pin.seg_index, 0);
                    }
                }
            }
        }
    }
}


//刷新icon闪烁显示
static void __lcd_seg3x9_flash_show_icon(u8 is_on)
{
    //show/off icon
    u8 i = 0;
    u8 j = 0;
    if (__this->lcd_seg_var.bFlashIcon) {
        for (j = 0; j < 32; j++) {
            if (BIT(j) & __this->lcd_seg_var.bFlashIcon) {
                for (i = 0; i < ARRAY_SIZE(lcd_seg3x9_icon_seg2pin); i++) { //lookup icon exist
                    if (BIT(j) == lcd_seg3x9_icon_seg2pin[i].icon) {
                        //look up the seg2pin table, set the pin should be output 0
                        if (is_on) {
                            __lcd_seg3x9_show_segN(lcd_seg3x9_icon_seg2pin[i].seg2pin.com_index,
                                                   lcd_seg3x9_icon_seg2pin[i].seg2pin.seg_index, 1);
                        } else {
                            __lcd_seg3x9_show_segN(lcd_seg3x9_icon_seg2pin[i].seg2pin.com_index,
                                                   lcd_seg3x9_icon_seg2pin[i].seg2pin.seg_index, 0);
                        }
                    }
                }
            }
        }
    }
}


static void __lcd_seg3x9_flash_show_char(u8 is_on)
{
    //show/off icon
    u8 i = 0;
    u8 j = 0;
    for (j = 0; j < 3; j++) {
        if (__this->lcd_seg_var.bFlashChar & BIT(j)) {
            if (is_on) {
                for (u8 i = 0; i < 7; i++) {
                    if (__this->lcd_seg_var.bShowBuff[j] & BIT(i)) {
                        __lcd_seg3x9_show_segN(lcd_seg3x9_seg2pin[i + j * 7].com_index,
                                               lcd_seg3x9_seg2pin[i + j * 7].seg_index, 1);
                    }
                }
            } else {
                for (u8 i = 0; i < 7; i++) {
                    __lcd_seg3x9_show_segN(lcd_seg3x9_seg2pin[i + j * 7].com_index,
                                           lcd_seg3x9_seg2pin[i + j * 7].seg_index, 0);
                }
            }
        }
    }
}

static void __lcd_seg3x9_flash_show_timer_handle(void *priv)
{
    static u8 flash_on = 0;
    if (__this->lcd_seg_var.bFlashIcon) {
        __lcd_seg3x9_flash_show_icon(flash_on);
    }
    if (__this->lcd_seg_var.bFlashChar) {
        __lcd_seg3x9_flash_show_char(flash_on);
    }
    flash_on = !flash_on;
}



static void __lcd_seg3x9_reflash_screen()
{
    __lcd_seg3x9_reflash_string();
    __lcd_seg3x9_reflash_icon();
}


static void __lcd_seg3x9_clear_screen()
{
    //clear all
    for (u8 i = 0; i < 9; i++) {
        gpio_set_die(__this->user_data->pin_cfg.pin_seg[i], 0);
        gpio_set_hd0(__this->user_data->pin_cfg.pin_seg[i], 0);
        gpio_set_pull_down(__this->user_data->pin_cfg.pin_seg[i], 0);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   lcd_seg 单个字符显示函数
   @param   chardata：显示字符
   @return  void
   @note    void __lcd_seg3x9_show_char(u8 chardata)
   @ display:
	   ___    ___    ___
	  |___|  |___|  |___|
	  |___|  |___|  |___|
	 ---0------1------2----> X
*/
/*----------------------------------------------------------------------------*/
static void __lcd_seg3x9_show_char(u8 chardata)
{
    if (__this->lcd_seg_var.bCoordinateX >= 3) {
        __this->lcd_seg_var.bCoordinateX = 0; //or return
        //return ;
    }
    if ((chardata >= '0') && (chardata <= '9')) {
        __this->lcd_seg_var.bShowBuff[__this->lcd_seg_var.bCoordinateX++] = LCD_SEG_NUMBER_2_SEG[chardata - '0'];
    } else if ((chardata >= 'a') && (chardata <= 'z')) {
        __this->lcd_seg_var.bShowBuff[__this->lcd_seg_var.bCoordinateX++] = LCD_SEG_SMALL_LETTER_2_SEG[chardata - 'a'];
    } else if ((chardata >= 'A') && (chardata <= 'Z')) {
        __this->lcd_seg_var.bShowBuff[__this->lcd_seg_var.bCoordinateX++] = LCD_SEG_LARGE_LETTER_2_SEG[chardata - 'A'];
    } else if (chardata == ':') {
        __this->lcd_seg_var.bShowIcon |= LCD_SEG3X9_2POINT;
        __this->lcd_seg_var.bFlashIcon &= (~LCD_SEG3X9_2POINT);
    } else if (chardata == '.') {
        /* __this->lcd_seg_var.bShowIcon |= LCD_SEG_DOT; */
        /* __this->lcd_seg_var.bFlashIcon &= (~LCD_SEG_DOT); */
    } else if (chardata == ' ') {
        __this->lcd_seg_var.bShowBuff[__this->lcd_seg_var.bCoordinateX++] = 0;
    } else {
        __this->lcd_seg_var.bShowBuff[__this->lcd_seg_var.bCoordinateX++] = LED_G; //显示'-'
    }

    __lcd_seg3x9_reflash_char(__this->lcd_seg_var.bCoordinateX - 1);
}

///////////////// API:
void lcd_seg3x9_setX(u8 X)
{
    __this->lcd_seg_var.bCoordinateX = X;
}

void lcd_seg3x9_clear_string(void)
{
    memset(__this->lcd_seg_var.bShowBuff, 0x00, 4);
    __this->lcd_seg_var.bFlashChar = 0;

    lcd_seg3x9_setX(0);
    __lcd_seg3x9_reflash_string();
}

void lcd_seg3x9_show_char(u8 chardata)
{
    __lcd_seg3x9_show_char(chardata);
}

void lcd_seg3x9_show_string(u8 *str)
{
    while (*str != '\0') {
        __lcd_seg3x9_show_char(*str++);
    }
}

void lcd_seg3x9_show_icon(UI_LCD_SEG3X9_ICON icon)
{
    __this->lcd_seg_var.bShowIcon |= icon;
    __this->lcd_seg_var.bFlashIcon &= (~icon); //stop display
    __lcd_seg3x9_reflash_icon();
}

void lcd_seg3x9_flash_icon(UI_LCD_SEG3X9_ICON icon)
{
    __this->lcd_seg_var.bFlashIcon |= icon;
    __this->lcd_seg_var.bShowIcon &= (~icon); //stop display
    __lcd_seg3x9_reflash_icon();
}

void lcd_seg3x9_flash_char_start(u8 index)
{
    if (index < 3) {
        __this->lcd_seg_var.bFlashChar |= BIT(index);
    }
}

void lcd_seg3x9_flash_char_stop(u8 index)
{
    if (index < 3) {
        __this->lcd_seg_var.bFlashChar &= ~BIT(index);
    }
}

void lcd_seg3x9_clear_icon(UI_LCD_SEG3X9_ICON icon)
{
    __this->lcd_seg_var.bShowIcon &= (~icon);
    __this->lcd_seg_var.bFlashIcon &= (~icon);
    __lcd_seg3x9_reflash_icon();
}

void lcd_seg3x9_clear_all_icon(void)
{
    __this->lcd_seg_var.bFlashIcon = 0;
    __this->lcd_seg_var.bShowIcon = 0;
    __lcd_seg3x9_reflash_icon();
}

void lcd_seg3x9_show_null(void)
{
    lcd_seg3x9_clear_string();
    lcd_seg3x9_clear_all_icon();
}


/*----------------------------------------------------------------------------*/
/**@brief   字符串显示函数, 默认左对齐, 从x = 0开始显示
   @param   *str：字符串的指针
   @return  void
   @note    void lcd_seg_show_string_left(u8 *str)
*/
/*----------------------------------------------------------------------------*/
void lcd_seg3x9_show_string_reset_x(u8 *str)
{
    lcd_seg3x9_clear_string();
    while (*str != '\0') {
        __lcd_seg3x9_show_char(*str++);
    }
    __lcd_seg3x9_reflash_screen();
}

void lcd_seg3x9_show_string_align_right(u8 *str)
{
    u8 cnt = 0;
    u8 *_str = str;
    while (*_str++ != '\0') {
        cnt++;
    }
    if (cnt > 3) {
        lcd_seg3x9_err_error("show string oversize", __func__);
        return;
    }
    lcd_seg3x9_clear_string();
    lcd_seg3x9_setX(3 - cnt);
    while (*str != '\0') {
        lcd_seg3x9_show_char(*str++);
    }
    __lcd_seg3x9_reflash_screen();
}

void lcd_seg3x9_show_string_align_left(u8 *str)
{
    lcd_seg3x9_show_string_reset_x(str);
}

void lcd_seg3x9_show_number(u16 val)
{
    u8 tmp_buf[5] = {0};

    if (val > 999) {
        lcd_seg3x9_show_icon(LCD_SEG3X9_DIGIT1);
    } else {
        lcd_seg3x9_clear_icon(LCD_SEG3X9_DIGIT1);
    }
    itoa4(val, tmp_buf);
    lcd_seg3x9_show_string_reset_x(&tmp_buf[1]);
}

//数字显示函数, 高位不显示0
void lcd_seg3x9_show_number2(u16 val)
{
    u8 i;
    u8 tmp_buf[5] = {0};

    if (val > 999) {
        lcd_seg3x9_show_icon(LCD_SEG3X9_DIGIT1);
    } else {
        lcd_seg3x9_clear_icon(LCD_SEG3X9_DIGIT1);
    }

    itoa4(val, tmp_buf);
    for (i = 0; i < 2; i++) {
        if (tmp_buf[i + 1] != '0') {
            break;
        }
    }
    lcd_seg3x9_show_string_align_right((u8 *) & (tmp_buf[i + 1]));
}

//数字显示函数(追加方式)
void lcd_seg3x9_show_number_add(u16 val)
{
    u8 i;
    u8 tmp_buf[5] = {0};

    if (__this->lcd_seg_var.bCoordinateX == 0) {
        if (val > 999) {
            lcd_seg3x9_show_icon(LCD_SEG3X9_DIGIT1);
        } else {
            lcd_seg3x9_clear_icon(LCD_SEG3X9_DIGIT1);
        }
    }

    itoa4(val, tmp_buf);
    for (i = 0; i < 3; i++) {
        if (tmp_buf[i] != '0') {
            break;
        }
    }
    lcd_seg3x9_show_string((u8 *)&tmp_buf[i]);
}



/*----------------------------------------------------------------------------*/
/**@brief   LCD段码屏初始化
   @param   void
   @return  void
   @note    void lcd_seg3x9_init(const struct lcd_seg3x9_platform_data *_data)
*/
/*----------------------------------------------------------------------------*/
void lcd_seg3x9_init(const struct lcd_seg3x9_platform_data *_data)
{
    if ((__this->init == true) || (_data == NULL)) {
        return;
    }
    memset(__this, 0x00, sizeof(struct ui_lcd_seg3x9_env));
    //CLK_CON1[9:8]
    //00b: wclk;
    //01b: rtosl_clk;
    //10b: lsb_clk;
    //11b: lrc_clk;
    SFR(JL_CLOCK->CLK_CON1, 8, 2, 0);  //LCDC时钟源选择wclk 32k

    JL_LCDC->CON0 = 0;
    //板级配置参数
    JL_LCDC->CON0 |= (_data->bias << 2);    //[3:2]:0x01:1/2 Bias。0x02:1/3。0x03:1/4
    JL_LCDC->CON0 |= (_data->vlcd << 4); 	//[6:4]: VLCDS

    //固定配置参数
    JL_LCDC->CON0 |= (1 << 7);                //[7]: 帧频率控制, 默认使用 32KHz / 64
    JL_LCDC->CON0 |= (0b01 << 12);          //[10:11]: CHGMOD一直用强充电模式
    JL_LCDC->CON0 |= (LCD_SEG3X9_COM_NUMBER_3 << 14); 			//[15:14]: COM_NUMBER_3;

    //SEG使能
    JL_LCDC->SEG_IOEN0 = 0;
    JL_LCDC->SEG_IOEN1 = 0;

    u8 i = 0;
    u8 j = 0;
    for (j = 0; j < 9; j++) {
        for (i = 0; i < ARRAY_SIZE(hw_pin2seg_mapping); i++) {
            if (_data->pin_cfg.pin_seg[j] == hw_pin2seg_mapping[i].pin_index) {
                if (hw_pin2seg_mapping[i].pin_index >= IO_PORTC_00) {
                    JL_LCDC->SEG_IOEN1 |= BIT(hw_pin2seg_mapping[i].seg_index - 16);  //SEG_PC口允许位
                } else {
                    JL_LCDC->SEG_IOEN0 |= BIT(hw_pin2seg_mapping[i].seg_index); 	  //SEG_PA口允许位
                }
            }
        }
    }

    JL_LCDC->CON0 |= BIT(0);                //LCD_EN

    sys_timer_add(NULL, __lcd_seg3x9_flash_show_timer_handle, 500);

    __this->user_data = _data;
    __this->init = true;
}


//=================================================================================//
//                        		FOR TEST CODE 								       //
//=================================================================================//
#if 0
LCD_SEG3X9_PLATFORM_DATA_BEGIN(lcd_seg3x9_test_data)
/* .vlcd = LCD_VOLTAGE_3_3V, */
/* .bias = LCD_BIAS_1_3, */
.vlcd = LCD_SEG3X9_VOLTAGE_3_2V,
//.bias = LCD_BIAS_1_3,
 .bias = LCD_SEG3X9_BIAS_1_2,
  .pin_cfg.pin_com[0] = IO_PORTC_05,
                        .pin_cfg.pin_com[1] = IO_PORTC_04,
                                .pin_cfg.pin_com[2] = IO_PORTC_03,

                                        .pin_cfg.pin_seg[0] = IO_PORTA_00,
                                                .pin_cfg.pin_seg[1] = IO_PORTA_01,
                                                        .pin_cfg.pin_seg[2] = IO_PORTA_02,
                                                                .pin_cfg.pin_seg[3] = IO_PORTA_03,
                                                                        .pin_cfg.pin_seg[4] = IO_PORTA_04,
                                                                                .pin_cfg.pin_seg[5] = IO_PORTA_07,
                                                                                        .pin_cfg.pin_seg[6] = IO_PORTA_12,
                                                                                                .pin_cfg.pin_seg[7] = IO_PORTA_10,
                                                                                                        .pin_cfg.pin_seg[8] = IO_PORTA_09,
                                                                                                                LCD_SEG3X9_PLATFORM_DATA_END()

                                                                                                                void lcd_seg3x9_test(void)
{
    lcd_seg3x9_init(&lcd_seg3x9_test_data);
    static u16 cnt = 0;
    lcd_seg3x9_show_number(cnt++);
    lcd_seg3x9_show_icon(LCD_SEG3X9_MHZ);
}
#endif

#endif /* #if TCFG_UI_LCD_SEG3X9_ENABLE */

