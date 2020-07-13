#include "includes.h"
#include "app_config.h"


#if TCFG_UI_LED1888_ENABLE

#include "ui/ui_api.h"
#include "LED1888.h"

LED1888_VAR LED1888_var;

typedef struct _LED_DISP_ {
    u8 on;
    u8 phase;
    u16 H_time;
    u16 L_time;
    u16 count;
} LED_DISP;

struct ui_led7_env {
    u8 init;
    u8 sys_halfsec;
    u16 count;
    LED_DISP led[2];
    const struct led7_platform_data *user_data;
    u8 lock: 1;
};

static struct ui_led7_env _led7_env = {0};
#define __this 		(&_led7_env)

#define PIN1_H  gpio_direction_output(__this->user_data->pin_cfg.pin7.pin[0], 1)
#define PIN1_L  gpio_direction_output(__this->user_data->pin_cfg.pin7.pin[0], 0)

#define PIN2_H  gpio_direction_output(__this->user_data->pin_cfg.pin7.pin[1], 1)
#define PIN2_L  gpio_direction_output(__this->user_data->pin_cfg.pin7.pin[1], 0)

#define PIN3_H  gpio_direction_output(__this->user_data->pin_cfg.pin7.pin[2], 1)
#define PIN3_L  gpio_direction_output(__this->user_data->pin_cfg.pin7.pin[2], 0)

#define PIN4_H  gpio_direction_output(__this->user_data->pin_cfg.pin7.pin[3], 1)
#define PIN4_L  gpio_direction_output(__this->user_data->pin_cfg.pin7.pin[3], 0)

#define PIN5_H  gpio_direction_output(__this->user_data->pin_cfg.pin7.pin[4], 1)
#define PIN5_L  gpio_direction_output(__this->user_data->pin_cfg.pin7.pin[4], 0)

#define PIN6_H  gpio_direction_output(__this->user_data->pin_cfg.pin7.pin[5], 1)
#define PIN6_L  gpio_direction_output(__this->user_data->pin_cfg.pin7.pin[5], 0)

#define PIN7_H  gpio_direction_output(__this->user_data->pin_cfg.pin7.pin[6], 1)
#define PIN7_L  gpio_direction_output(__this->user_data->pin_cfg.pin7.pin[6], 0)


//数字'0' ~ '9'显示段码表
static const  u8 LED_NUMBER[10] = {
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
static const  u8 LED_LARGE_LETTER[26] = {
    0x77, 0x40, 0x39, 0x3f, 0x79, ///<ABCDE
    0x71, 0x40, 0x76, 0x06, 0x40, ///<FGHIJ
    0x40, 0x38, 0x40, 0x37, 0x3f, ///<KLMNO
    0x73, 0x40, 0x50, 0x6d, 0x78, ///<PQRST
    0x3e, 0x3e, 0x40, 0x76, 0x40, ///<UVWXY
    0x40 ///<Z
};

//字母'a' ~ 'z'显示段码表
static const  u8 LED_SMALL_LETTER[26] = {
    0x77, 0x7c, 0x58, 0x5e, 0x79, ///<abcde
    0x71, 0x40, 0x40, 0x40, 0x40, ///<fghij
    0x40, 0x38, 0x40, 0x54, 0x5c, ///<klmno
    0x73, 0x67, 0x50, 0x40, 0x78, ///<pqrst
    0x3e, 0x3e, 0x40, 0x40, 0x40, ///<uvwxy
    0x40 ///<z
};


/*----------------------------------------------------------------------------*/
/**@brief   LED1888显示坐标设置
   @param   x：显示横坐标
   @return  void
   @author
   @note    void LED5X7_setX(u8 X)
*/
/*----------------------------------------------------------------------------*/
void LED1888_setX(u8 X)
{
    LED1888_var.bCoordinateX = X;
}

/*----------------------------------------------------------------------------*/
/**@brief   LED1888 状态位缓存清除函数
   @param   void
   @return  void
   @author
   @note    void LED1888_clear_icon(void)
*/
/*----------------------------------------------------------------------------*/
void LED1888_clear_icon(void)
{
//    my_memset((u8 *)&LED1888_var, 0x0, sizeof(LED1888_VAR));
//    LED1888_var.bCoordinateX = 1;
}

/*----------------------------------------------------------------------------*/
/**@brief   LED1888 状态位缓存清除函数
   @param   void
   @return  void
   @author
   @note    void LED1888_clear_icon(void)
*/
/*----------------------------------------------------------------------------*/
void LED1888_clear(void)
{

    memset((u8 *)&LED1888_var, 0x0, sizeof(LED1888_VAR));
    __this->sys_halfsec  = 0;
    __this->count = 0;//;
    LED1888_var.bCoordinateX = 0;
}

/*----------------------------------------------------------------------------*/
/**@brief   LED1888 初始化函数
   @param   void
   @return  void
   @author  Change.tsai
   @note    void LED1888_init(void)
*/
/*----------------------------------------------------------------------------*/
void LED1888_init(void)
{
    u8 port = (u8) - 1;
    u8 i = 0;
    if (__this->user_data->pin_type == LED7_PIN7) {
        for (i = 0; i < 7; i++) {
            port = __this->user_data->pin_cfg.pin7.pin[i];
            if (port != 255) {
                gpio_set_pull_down(port, 0);
                gpio_set_pull_up(port, 0);
                gpio_set_direction(port, 1);
            }
        }
    }

}

/*----------------------------------------------------------------------------*/
/**@brief   LED1888 单个字符显示函数
   @param   chardata：显示字符
   @return  void
   @author  Change.tsai
   @note    void LED1888_show_char(u8 chardata)
*/
/*----------------------------------------------------------------------------*/
void LED1888_show_char(u8 chardata)
{
    //必须保证传入的参数符合范围，程序不作判断
//    printf("bCoordinateX = %d\n",LED1888_var.bCoordinateX);
    if ((chardata < ' ') || (chardata > '~') || (LED1888_var.bCoordinateX > 4)) {
        return;
    }
//    printf("bCoordinateX = %d\n",LED1888_var.bCoordinateX);
    if ((chardata >= '0') && (chardata <= '9')) {
        LED1888_var.bShowBuff[LED1888_var.bCoordinateX++] = LED_NUMBER[chardata - '0'];
    } else if ((chardata >= 'a') && (chardata <= 'z')) {
        LED1888_var.bShowBuff[LED1888_var.bCoordinateX++] = LED_SMALL_LETTER[chardata - 'a'];
    } else if ((chardata >= 'A') && (chardata <= 'Z')) {
        LED1888_var.bShowBuff[LED1888_var.bCoordinateX++] = LED_LARGE_LETTER[chardata - 'A'];
    } else if (chardata == '.') {
        LED1888_var.bShowBuff[2] |= BIT(7);
    } else if (chardata == ' ') {
        LED1888_var.bShowBuff[LED1888_var.bCoordinateX++] = 0;
    } else { //if (chardata == '-')     //不可显示
        LED1888_var.bShowBuff[LED1888_var.bCoordinateX++] = BIT(6);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief   LCD_SEG 字符闪烁函数
   @param   void
   @return  void
   @author  Change.tsai
   @note    void  LCD_SEG_FlashChar(void)
*/
/*----------------------------------------------------------------------------*/
void  LED1888_FlashChar(void)
{
//    LED1888_var.seg_icon |= ICON_S1;
    LED1888_var.bFlashChar |= BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4);
//    printf("LED1888_var.bFlashChar = %d\n",LED1888_var.bFlashChar);
}

void  LED1888_Clear_FlashChar(void)
{
//    LED1888_var.seg_icon &= ~ICON_S1;
    LED1888_var.bFlashChar &= ~(BIT(0) | BIT(1) | BIT(2) | BIT(3) | BIT(4));
//    printf("LED1888_var.bFlashChar = %d\n",LED1888_var.bFlashChar);
}

/*----------------------------------------------------------------------------*/
/**@brief   LCD_SEG 单个字符显示函数
   @param   chardata：显示字符
   @return  void
   @author  Change.tsai
   @note    void LCD_SEG4X8_show_number(u8 number)
*/
/*----------------------------------------------------------------------------*/
void LED1888_show_number(u8 number)
{
    LED1888_show_char(number + '0');
}

/*----------------------------------------------------------------------------*/
/**@brief   LED5X7 字符串显示函数
   @param   *str：字符串的指针   offset：显示偏移量
   @return  void
   @author  Change.tsai
   @note    void LED5X7_show_string(u8 *str)
*/
/*----------------------------------------------------------------------------*/
void LED1888_show_string(u8 *str)
{
    while (0 != *str) {
        LED1888_show_char(*str++);
    }
}


void LED1888_show_filenumber(u16 file_num)
{
    LED1888_setX(0);

    u8 bcd_number[5] = {0};	    ///<换算结果显示缓存
    itoa4(file_num, (u8 *)bcd_number);
    if (file_num > 999 && file_num <= 1999) {
        bcd_number[0] = '1';
    } else {
        bcd_number[0] = ' ';
    }
    LED1888_show_string((u8 *)bcd_number);
}

/*----------------------------------------------------------------------------*/
/**@brief   POWER_ON 显示主界面
   @param   void
   @return  void
   @author  Change.tsai
   @note    void LED1888_show_aux_main(void)
*/
/*----------------------------------------------------------------------------*/
void LED1888_show_power_on(void)
{
    LED1888_show_string((u8 *)"E");
}

/*----------------------------------------------------------------------------*/
/**@brief   LED1888 开机显示界面（Hi）
   @param   *str：字符串的指针
   @return  void
   @author  Change.tsai
   @note    void LED1888_show_Hi(u8 *str)
*/
/*----------------------------------------------------------------------------*/
void LED1888_show_Hi(void)
{
    LED1888_show_string((u8 *)" HI");
}

/*----------------------------------------------------------------------------*/
/** @brief:
    @param:
    @return:
    @author:
    @note:
*/
/*----------------------------------------------------------------------------*/
void LED1888_show_waiting(void)
{
    LED1888_show_string((u8 *)" Lod");
}

/*----------------------------------------------------------------------------*/
/** @brief:
    @param:
    @return:
    @author:
    @note:
*/
/*----------------------------------------------------------------------------*/
void LED1888_show_pause(void)
{
    LED1888_show_string((u8 *)" PAU");
}
/*----------------------------------------------------------------------------*/
/**@brief   Music模式 设备显示
   @param   void
   @return  void
   @author  Change.tsai
   @note    void LED1888_show_dev(void)
*/
/*----------------------------------------------------------------------------*/
void LED1888_show_dev(void)
{

}

/*----------------------------------------------------------------------------*/
/**@brief   音量显示函数
   @param   void
   @return  void
   @author  Change.tsai
   @note    void LCD_SEG3X9_show_volume(void)
*/
/*----------------------------------------------------------------------------*/
void LED1888_show_volume(u8 vol)
{
    LED1888_clear_icon();
    LED1888_show_char(' ');
    LED1888_show_char('V');
    LED1888_show_number(vol / 10);
    LED1888_show_number(vol % 10);
}

/*----------------------------------------------------------------------------*/
/**@brief   频率显示
   @param   void
   @return  void
   @author  Change.tsai
   @note    void LCD_SEG_show_freq(void)
*/
/*----------------------------------------------------------------------------*/
void  LED1888_show_freq(u16 freq)
{
    /*FM - Frequency*/

    u8 bcd_number[5] = {0};	  ///<换算结果显示缓存
    itoa4(freq, bcd_number);
    if (freq > 999) {
        bcd_number[0] = '1';
    } else {
        bcd_number[0] = ' ';
    }
    LED1888_show_string((u8 *)bcd_number);
    LED1888_var.bShowBuff[2] |= BIT(7);//小数点
//    if(!strcmp(curr_task->name,"MusicTask") )
//    {
//         puts("\n-----show SD icon--------\n");
//        LED1888_show_dev();
//    }
//
//    if (freq <= 999)
//    {
//        LCD_STATUS_3X9 &= ~ICON_S1;
//    }
//    else
//    {
//        LCD_STATUS_3X9 |= ICON_S1;
//    }
//
//    LCD_STATUS_3X9 |= ICON_MHZ;
}

/*----------------------------------------------------------------------------*/
/**@brief   频率显示
   @param   void
   @return  void
   @author  Change.tsai
   @note    void LCD_SEG_show_freq_setting(void)
*/
/*----------------------------------------------------------------------------*/
void   LED1888_show_freq_setting(u16 freq)
{
    LED1888_var.bFlashChar = 0x1f;

    LED1888_show_freq(freq);
}

/*----------------------------------------------------------------------------*/
/**@brief   红外输入文件号显示函数
   @param   void
   @return  void
   @author  Change.tsai
   @note    void LCD_SEG3X9_show_IR_number(void)
*/
/*----------------------------------------------------------------------------*/
void LED1888_show_IR_number(u16 ir_num)
{
    /*IR File Number info*/
//    LCD_STATUS_3X9 = 0;
    u8 bcd_number[5] = {0};	  ///<换算结果显示缓存
    itoa4(ir_num, bcd_number);
    if (ir_num > 999) {
        bcd_number[0] = '1';
    } else {
        bcd_number[0] = ' ';
    }
    LED1888_show_string((u8 *)bcd_number);
}

/*----------------------------------------------------------------------------*/
/**@brief   EQ显示函数
   @param   void
   @return  void
   @author  Change.tsai
   @note    void LCD_SEG3X9_show_eq(void)
*/
/*----------------------------------------------------------------------------*/
void LED1888_show_eq(u8 eq)
{
#if 1
    LED1888_setX(0);
    LED1888_show_string((u8 *)" Eq");
    LED1888_show_number(eq % 10);
//    LCD_SEG3X9_show_dev();
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief   循环模式显示函数
   @param   void
   @return  void
   @author  Change.tsai
   @note    void LCD_SEG3X9_show_playmode(void)
*/
/*----------------------------------------------------------------------------*/
void LED1888_show_playmode(u8 play_mode)
{
    //printf("\n------play_mode == %d\n",play_mode);
//    LCD_STATUS_3X9 = 0;
    LED1888_setX(0);
    /* LED1888_show_string((u8 *)&playmodestr[play_mode-FOP_MAX-1][0]); */
}

/*----------------------------------------------------------------------------*/
/**@brief   bt 显示空闲
   @param   void
   @return  void
   @author  Change.tsai
   @note    void LED1888_show_bt_idle(void)
*/
/*----------------------------------------------------------------------------*/
void  LED1888_show_bt_idle(void)
{
    LED1888_show_string((u8 *)" H ");
}

/*----------------------------------------------------------------------------*/
/**@brief   bt 显示通话
   @param   void
   @return  void
   @author  Change.tsai
   @note    void LCD_SEG_show_bt_call(void)
*/
/*----------------------------------------------------------------------------*/
void  LED1888_show_bt_call(void)
{
    LED1888_show_string((u8 *)" CAL");
}

/*----------------------------------------------------------------------------*/
/**@brief   bt 显示主界面
   @param   void
   @return  void
   @author  Change.tsai
   @note    void LCD_SEG_show_bt_main(void)
*/
/*----------------------------------------------------------------------------*/
void   LED1888_show_bt_main(void)
{
    LED1888_show_string((u8 *)" bt");
}

/*----------------------------------------------------------------------------*/
/**@brief   Music模式 显示界面
   @param   void
   @return  void
   @author  Change.tsai
   @note    void LED1888_show_music_main(void)
*/
/*----------------------------------------------------------------------------*/
void LED1888_show_music_main(void)
{

}

/*----------------------------------------------------------------------------*/
/**@brief   FM 模式主界面
   @param   void
   @return  void
   @author  Change.tsai
   @note    void LED1888_show_fm_main(void)
*/
/*----------------------------------------------------------------------------*/
void LED1888_show_fm_main(void)
{

}

/*----------------------------------------------------------------------------*/
/**@brief   AUX 显示主界面
   @param   void
   @return  void
   @author  Change.tsai
   @note    void LED1888_show_aux_main(void)
*/
/*----------------------------------------------------------------------------*/
void LED1888_show_aux_main(void)
{
    LED1888_show_string((u8 *)" AUX");
}

void LED1888_show_pc_main(void)
{
#if 0
    LED1888_show_string((u8 *)menu_string[2]);
#endif
}

void LED1888_show_pc_vol_up(void)
{
#if 0
    LED1888_show_string((u8 *)menu_string[3]);
#endif
}

void LED1888_show_pc_vol_down(void)
{
#if 0
    LED1888_show_string((u8 *)menu_string[4]);
#endif
}

void LED1888_show_led0(u32 arg)
{

    if (arg & BIT(31)) {
        __this->led[0].on = 1;
        __this->led[0].phase = !!(arg & BIT(30));
        __this->led[0].H_time = (arg >> 15) & 0x7fff;
        __this->led[0].L_time = (arg) & 0x7fff;
        __this->led[0].count = 0;//__this->led[0].H_time;
    } else {
        __this->led[0].on = 0;
        __this->led[0].phase = 0;//!!arg&BIT(15);
        __this->led[0].H_time = 0;//(arg >> 16)&0x7fff;
        __this->led[0].L_time = 0;//(arg)&0xffff;
        __this->led[0].count = 0;//__this->led[0].H_time;
    }

}

void LED1888_show_led1(u32 arg)
{

    if (arg & BIT(31)) {
        __this->led[1].on = 1;
        __this->led[1].phase = !!(arg & BIT(30));
        __this->led[1].H_time = (arg >> 15) & 0x7fff;
        __this->led[1].L_time = (arg) & 0x7fff;
        __this->led[1].count = 0;//__this->led[1].H_time;
    } else {

        __this->led[1].on = 0;
        __this->led[1].phase = 0;//!!arg&BIT(15);
        __this->led[1].H_time = 0;//(arg >> 16)&0x7fff;
        __this->led[1].L_time = 0;//(arg)&0xffff;
        __this->led[1].count = 0;//__this->led[1].H_time;

    }

}

/*----------------------------------------------------------------------------*/
/**@brief   LED1888 扫描函数
   @param   void
   @return  void
   @author
   @note    void LED1888_6p_scan(void)
*/
/*----------------------------------------------------------------------------*/

void LED1888_6p_scan(void)
{
    static u8 cnt = 0;
    u8 j;
    static u8 bufdat[5];
    u8 flash = LED1888_var.bFlashChar;

    if (!cnt && !__this->lock) {
        bufdat[0] = LED1888_var.bShowBuff[0];
        bufdat[1] = LED1888_var.bShowBuff[1];
        bufdat[2] = LED1888_var.bShowBuff[2];
        bufdat[3] = LED1888_var.bShowBuff[3];
    }

    __this->count++;
    if (__this->count >= 256) {
        __this->sys_halfsec  = !__this->sys_halfsec;
        __this->count = 0;
    }


    if (__this->led[0].on) {
        if (__this->led[0].phase) {
            __this->led[0].count++;
            if (__this->led[0].count >= __this->led[0].H_time + __this->led[0].L_time) {
                __this->led[0].count = 0;
            }
        } else {
            __this->led[0].count = (__this->led[0].count == 0) ? (__this->led[0].H_time + __this->led[0].L_time) : (--__this->led[0].count);
        }
    }

    if (__this->led[1].on) {

        if (__this->led[1].phase) {
            __this->led[1].count++;
            if (__this->led[1].count >= __this->led[1].H_time + __this->led[1].L_time) {
                __this->led[1].count = 0;
            }
        } else {
            __this->led[1].count = (__this->led[1].count == 0) ? (__this->led[1].H_time + __this->led[1].L_time) : (--__this->led[1].count);
        }
    }

    LED1888_init();

    if (__this->sys_halfsec) {
        for (j = 0; j < 4; j++) {
            if (flash & BIT(j)) { //数字位闪烁
                //                putchar('F');
                bufdat[j] = 0x0;
            }
        }
    }

    switch (cnt) {
    case 0:
        PIN1_L;
        if (bufdat[1]&LED_A) {
            PIN2_H;
#ifdef UI_FADE_EN
            PIN2_FADE_PU;
            PIN2_FADE_PD;
#endif
        }
        if (bufdat[1]&LED_B) {
            PIN3_H;
#ifdef UI_FADE_EN
            PIN3_FADE_PU;
            PIN3_FADE_PD;
#endif
        }
        if (bufdat[1]&LED_C) {
            PIN4_H;
#ifdef UI_FADE_EN
            PIN4_FADE_PU;
            PIN4_FADE_PD;
#endif
        }
        if (bufdat[1]&LED_D) {
            PIN5_H;
#ifdef UI_FADE_EN
            PIN5_FADE_PU;
            PIN5_FADE_PD;
#endif
        }
        if (bufdat[1]&LED_E) {
            PIN6_H;
#ifdef UI_FADE_EN
            PIN6_FADE_PU;
            PIN6_FADE_PD;
#endif
        }
        break;

    case 1:
        PIN2_L;
        if (bufdat[2]&LED_A) {
            PIN1_H;
        }
        if (bufdat[1]&LED_F) {
            PIN3_H;
        }
        if (bufdat[1]&LED_G) {
            PIN4_H;
        }
        if (bufdat[2]&LED_F) {
            PIN5_H;
        }
        if (bufdat[2]&LED_G) {
            PIN6_H;
        }
        break;

    case 2:
        PIN3_L;
        if (bufdat[2]&LED_B) {
            PIN1_H;
        }
        if (bufdat[3]&LED_A) {
            PIN2_H;
        }
        if (bufdat[0]&LED_B) {
            PIN4_H;
        }
        if (bufdat[0]&LED_C) {
            PIN5_H;
        }
        if (bufdat[2]&BIT(7)) {
            PIN6_H;
        };
        break;
    case 3:
        PIN4_L;
        if (bufdat[2]&LED_C) {
            PIN1_H;
        }
        if (bufdat[3]&LED_B) {
            PIN2_H;
        }
        if (bufdat[3]&LED_E) {
            PIN3_H;
        }
        break;

    case 4:
        PIN5_L;
        if (bufdat[2]&LED_D) {
            PIN1_H;
        }
        if (bufdat[3]&LED_C) {
            PIN2_H;
        }
        if (bufdat[3]&LED_F) {
            PIN3_H;
        }

        if (__this->led[0].on && (__this->led[0].count <= __this->led[0].H_time)) {
            PIN4_H;
        }

        break;

    case 5:
        PIN6_L;
        if (bufdat[2]&LED_E) {
            PIN1_H;
        }
        if (bufdat[3]&LED_D) {
            PIN2_H;
        }
        if (bufdat[3]&LED_G) {
            PIN3_H;
        }


        if (__this->led[1].on && (__this->led[1].count <= __this->led[1].H_time)) {

            PIN4_H;
        }


        break;

    default:
        break;
    }

    cnt++;
    if (cnt == 6) {
        cnt = 0;
    }
}

#if 0
UI_API LED1888_ui_api = {
    .setX              = LED1888_setX,
    .clear_icon        = LED1888_clear_icon,
    .clear             = LED1888_clear,
    .show_char         = LED1888_show_char,
    .FlashChar         = LED1888_FlashChar,
    .Clear_FlashChar   = LED1888_Clear_FlashChar,
    .show_number       = LED1888_show_number,
    .show_string       = LED1888_show_string,
    .show_power_on     = LED1888_show_power_on,
    .show_Hi           = LED1888_show_Hi,
    .show_waiting      = LED1888_show_waiting,
    .show_pause        = LED1888_show_pause,
    .show_dev          = LED1888_show_dev,
    .show_filenumber   = LED1888_show_filenumber,
    .show_volume       = LED1888_show_volume,
    .show_freq         = LED1888_show_freq,
    .show_freq_setting = LED1888_show_freq_setting,
    .show_IR_number    = LED1888_show_IR_number,
    .show_eq           = LED1888_show_eq,
    .show_playmode     = LED1888_show_playmode,
    .show_bt_idle      = LED1888_show_bt_idle,
    .show_bt_call      = LED1888_show_bt_call,
    .show_bt_main      = LED1888_show_bt_main,
    .show_music_main   = LED1888_show_music_main,
    .show_fm_main      = LED1888_show_fm_main,
    .show_aux_main     = LED1888_show_aux_main,
    .show_pc_main      = LED1888_show_pc_main,
    .show_pc_vol_up    = LED1888_show_pc_vol_up,
    .show_pc_vol_down  = LED1888_show_pc_vol_down,
    .show_led0         = LED1888_show_led0,
    .show_led1         = LED1888_show_led1,
};
#endif

static void LED1888_setXY(u32 x, u32 y)
{
    LED1888_setX(x);
}

static void  LED1888_Flashchar(u32 arg)
{
    LED1888_var.bFlashChar |= arg;//BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4);
}

static void  LED1888_Clear_Flashchar(u32 arg)
{
    LED1888_var.bFlashChar &= ~(arg); //BIT(0)|BIT(1)|BIT(2)|BIT(3)|BIT(4);
}




static void LED1888_show_icon(u32 arg)
{
    switch (arg) {
    case LED7_DOT:
        LED1888_var.bShowBuff[2] |= BIT(7);//小数点
        break;
    default:
        break;
    }
}


static void LED1888_flash_icon(u32 arg)
{

}

static void LED1888_Clear_icon(u32 arg)
{
    switch (arg) {
    case LED7_DOT:
        LED1888_var.bShowBuff[2] &= ~(BIT(7)); //小数点
        break;
    default:
        break;
    }
}


static void LED1888_show_pic(u32 arg)
{

}

static void LED1888_hide_pic(u32 arg)
{

}

static void LED1888_show_lock(u32 en)
{
    __this->lock = !!en;
}



LCD_API LED1888_HW = {
    .clear             = LED1888_clear,
    .setXY             = LED1888_setXY,
    .FlashChar         = LED1888_Flashchar,
    .Clear_FlashChar   = LED1888_Clear_Flashchar,
    .show_icon         = LED1888_show_icon,
    .flash_icon        = LED1888_flash_icon,
    .clear_icon        = LED1888_Clear_icon,
    .show_string       = LED1888_show_string,
    .show_char         = LED1888_show_char,
    .show_number       = LED1888_show_number,
    .show_pic          = LED1888_show_pic,
    .hide_pic          = LED1888_hide_pic,
    .lock              = LED1888_show_lock,
};




/*----------------------------------------------------------------------------*/
/**@brief   led7段数码管初始化
   @param   void
   @return  void
   @author  Change.tsai
   @note    void led7_init(void)
*/
/*----------------------------------------------------------------------------*/
void *led_1888_init(const struct led7_platform_data *_data)
{
    memset(__this, 0x00, sizeof(struct ui_led7_env));

    if (_data == NULL) {
        return NULL;
    }
    memset((u8 *)&LED1888_var, 0x0, sizeof(LED1888_VAR));
    __this->user_data = _data;
    LED1888_init();

    sys_s_hi_timer_add(NULL, LED1888_6p_scan, 2); //2ms

    __this->init = true;
    return &LED1888_HW;
}


#endif
