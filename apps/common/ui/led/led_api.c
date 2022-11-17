/*--------------------------------------------------------------------------*/
/**@file    led_api.c
   @brief   led_api 显示面板控制API接口 不关心ledIO怎么接
   @details
   @author  HUANGXIAOWEI
   @date    2022-08
   @note    AD15
*/
/*----------------------------------------------------------------------------*/
#include "ui_api.h"
#include "system/app_core.h"
#include "system/includes.h"

#if LED_UI_EN
#include "led_api.h"
#include "tm162x.h"
#include "ui_common.h"

#define LOG_TAG_CONST       NORM
#define LOG_TAG             "[led_api]"
#include "debug.h"


_LED_DRIVER_VAR led_drvier_var;
// _LED_ICON_STATUS led_status_var;



const u8 LED_NUMBER[10] = {
    /*0*/
    (u8)(LED_A | LED_B | LED_C | LED_D | LED_E | LED_F),
    /*1*/
    (u8)(LED_B | LED_C),
    /*2*/
    (u8)(LED_A | LED_B | LED_D | LED_E | LED_G),
    /*3*/
    (u8)(LED_A | LED_B | LED_C | LED_D | LED_G),
    /*4*/
    (u8)(LED_B | LED_C | LED_F | LED_G),
    /*5*/
    (u8)(LED_A | LED_C | LED_D | LED_F | LED_G),
    /*6*/
    (u8)(LED_A | LED_C | LED_D | LED_E | LED_F | LED_G),
    /*7*/
    (u8)(LED_A | LED_B | LED_C),
    /*8*/
    (u8)(LED_A | LED_B | LED_C | LED_D | LED_E | LED_F | LED_G),
    /*9*/
    (u8)(LED_A | LED_B | LED_C | LED_D | LED_F | LED_G),
};

const u8 LED_LARGE_LETTER[26] = {
    0x77, 0x40, 0x39, 0x3f, 0x79, ///<ABCDE
    0x71, 0x40, 0x76, 0x06, 0x40, ///<FGHIJ
    0x40, 0x38, 0x40, 0x37, 0x3f, ///<KLMNO
    0x73, 0x40, 0x50, 0x6d, 0x40, ///<PQRST
    0x3e, 0x3e, 0x40, 0x76, 0x40, ///<UVWXY
    0x40///<Z
};

const u8 LED_SMALL_LETTER[26] = {
    0x77, 0x7c, 0x58, 0x5e, 0x79, ///<abcde
    0x71, 0x40, 0x40, 0x40, 0x40, ///<fghij
    0x40, 0x38, 0x40, 0x54, 0x5c, ///<klmno
    0x73, 0x67, 0x50, 0x40, 0x40, ///<pqrst
    0x3e, 0x3e, 0x40, 0x40, 0x40, ///<uvwxy
    0x40///<z
};


/*----------------------------------------------------------------------------*/
/**@brief   LED 清屏函数
   @param   void
   @return  void
   @author  Change.tsai
   @note    void led_drvier_show_null(void)
*/
/*----------------------------------------------------------------------------*/
void led_drvier_show_null(void)
{
    led_drvier_var.bLedOnOff = 0;
    memset(led_drvier_var.bShowBuffSpeed, 0, sizeof(led_drvier_var.bShowBuffSpeed));
    memset(led_drvier_var.bShowBuffPower, 0, sizeof(led_drvier_var.bShowBuffPower));
    memset(led_drvier_var.bShowBuffMileage, 0, sizeof(led_drvier_var.bShowBuffMileage));
    memset(led_drvier_var.bShowBuffIcon, 0, sizeof(led_drvier_var.bShowBuffIcon));
    memset(led_drvier_var.bDriverBuff, 0, sizeof(led_drvier_var.bDriverBuff));
}

void led_drvier_show_normal(void)
{
    led_drvier_var.bLedOnOff = 1;
}

/*----------------------------------------------------------------------------*/
/**@brief   led_drvier 扫描函数
   @param   void
   @return  void
   @author  Change.tsai
   @note    void led_drvier_init(void)
*/
/*----------------------------------------------------------------------------*/
void led_api_init(void)
{
    led_driver_init();
    sys_timeout_add(NULL, led_drvier_show_null, 500);
    /* led_drvier_show_null(); */
    /* led_drvier_show_normal(); */
    /* led_drvier_show_null(); */
    /* set_LED_all_on(); */
    /* sys_timeout_add(NULL,led_drvier_show_normal,LED_ALL_ON_TIMES); */
}
/*----------------------------------------------------------------------------*/
/**@brief   led_drvier 扫描函数
   @param   void
   @return  void
   @author  Change.tsai
   @note    void led_drvier_init(void)
*/
/*----------------------------------------------------------------------------*/
void led_api_start_ui(void)
{
    set_LED_all_on();
    sys_timeout_add(NULL, led_drvier_show_normal, LED_ALL_ON_TIMES);
}
/*----------------------------------------------------------------------------*/
/**@brief   LED 亮度设置
   @param   void
   0 关掉
   1 低亮度
   2 正常亮度
   @return  void
   @author  Change.tsai
   @note    void set_LED_brightness(u8 br)
*/
/*----------------------------------------------------------------------------*/
void set_LED_brightness(u8 br)
{
#if   LED_162X_EN
    led1629_ctr(br);
#endif
}

/*----------------------------------------------------------------------------*/
/**@brief   设置LED亮度全亮
   @param   void
   @return  void
   @author  Change.tsai
   @note    void set_LED_all_on(void)
*/
/*----------------------------------------------------------------------------*/
void set_LED_all_on(void)
{
    led_drvier_var.bLedOnOff = 0xff;
}

/*----------------------------------------------------------------------------*/
/**@brief   led_drvier 单个字符显示函数
   @param   chardata：显示字符
   @return  void
   @author  Change.tsai
   @note    void led_drvier_show_char(u8 chardata)
*/
/*----------------------------------------------------------------------------*/
u8 led_drvier_show_char(u8 chardata)
{
    //必须保证传入的参数符合范围，程序不作判断
    u8 data_temp;
    if ((chardata >= '0') && (chardata <= '9')) {
        data_temp = LED_NUMBER[chardata - '0'];
    } else if ((chardata >= 'a') && (chardata <= 'z')) {
        data_temp = LED_SMALL_LETTER[chardata - 'a'];
    } else if ((chardata >= 'A') && (chardata <= 'Z')) {
        data_temp = LED_LARGE_LETTER[chardata - 'A'];
    } else {
        data_temp = 0;
    }
    led_drvier_show_normal();
    return data_temp;
}

/*----------------------------------------------------------------------------*/
/**@brief   led_drvier 字符串显示函数
   @param   *str：字符串的指针   buf:显示内容存储的指针
   @return  void
   @author  Change.tsai
   @note    void led_drvier_show_string(u8 *str)
*/
/*----------------------------------------------------------------------------*/
void led_drvier_show_string(u8 *str, u8 *buf, u8 offset)
{
    u8 index = offset;
    while (0 != *str) {
        buf[index] = led_drvier_show_char(*str++);
        // log_info("buf[%d]=%x ",index,buf[index]);
        index++;
    }
}






/*----------------------------------------------------------------------------*/
//  速度显示函数
/*----------------------------------------------------------------------------*/
void led_drvier_show_speed(u8 speed)
{
    u8 offset = 0; //不显示前面的0
    memset(led_drvier_var.bShowBuffSpeed, 0, sizeof(led_drvier_var.bShowBuffSpeed));
    if (speed >= 10) {
        itoa2(speed);
    } else {
        itoa1(speed);
        offset = 1;
    }
    led_drvier_show_string((u8 *)bcd_number, led_drvier_var.bShowBuffSpeed, offset);
    led_drvier_show_speed_kmh(1, 0);
}

/*----------------------------------------------------------------------------*/
// 电压显示函数
/*----------------------------------------------------------------------------*/
void led_drvier_show_battery_volt(u8 volt)
{
    u8 offset = 0; //不显示前面的0
    if (led_drvier_var.bShowTotalMileage) {
        printf("no display battery");
        return;
    }
    memset(led_drvier_var.bShowBuffPower, 0, sizeof(led_drvier_var.bShowBuffPower));
    if (volt >= 100) {
        itoa3(volt);
    } else if (volt >= 10) {
        itoa2(volt);
        offset = 1;
    } else {
        itoa1(volt);
        offset = 2;
    }
    led_drvier_show_string((u8 *)bcd_number, led_drvier_var.bShowBuffPower, offset);
    led_drvier_show_volt(1, 0);
}


/*----------------------------------------------------------------------------*/
// 电量百分比显示函数
/*----------------------------------------------------------------------------*/
void led_drvier_show_battery_percent(u8 percent)
{
    u8 offset = 0; //不显示前面的0
    if (led_drvier_var.bShowTotalMileage) {
        return;
    }
    memset(led_drvier_var.bShowBuffPower, 0, sizeof(led_drvier_var.bShowBuffPower));
    if (percent >= 100) {
        itoa3(percent);
    } else if (percent >= 10) {
        itoa2(percent);
        offset = 1;
    } else {
        itoa1(percent);
        offset = 2;
    }
    led_drvier_show_string((u8 *)bcd_number, led_drvier_var.bShowBuffPower, offset);
    led_drvier_show_percent(1, 0);
}


/*----------------------------------------------------------------------------*/
// 单次里程显示函数
/*----------------------------------------------------------------------------*/
void led_drvier_show_mileage(u16 mileage)
{
    u8 offset = 0; //不显示前面的0

    if (led_drvier_var.bShowTotalMileage) {
        led_drvier_var.bShowTotalMileage = 0;
        memset(led_drvier_var.bShowBuffPower, 0, sizeof(led_drvier_var.bShowBuffPower));
        led_drvier_show_percent(0, 0);
        led_drvier_show_volt(0, 0);
    }
    memset(led_drvier_var.bShowBuffMileage, 0, sizeof(led_drvier_var.bShowBuffMileage));
    if (mileage >= 100) {
        itoa3(mileage);
    } else if (mileage >= 10) {
        itoa2(mileage);
        offset = 1;
    } else {
        itoa1(mileage);
        offset = 2;
    }
    led_drvier_show_string((u8 *)bcd_number, led_drvier_var.bShowBuffMileage, offset);
    led_drvier_show_km(1, 0);
}


/*----------------------------------------------------------------------------*/
// 总里程显示函数  左边的电量3个数字也用来显示里程 一共6位
/*----------------------------------------------------------------------------*/
void led_drvier_show_total_mileage(u32 mileage)
{
    u8 offset = 0; //不显示前面的0
    u16 mileage_low = mileage % 1000;
    led_drvier_var.bShowTotalMileage = 1;
    memset(led_drvier_var.bShowBuffMileage, 0, sizeof(led_drvier_var.bShowBuffMileage));
    if ((mileage_low >= 100) || (mileage >= 1000)) {
        itoa3(mileage_low);
    } else if (mileage_low >= 10) {
        itoa2(mileage_low);
        offset = 1;
    } else {
        itoa1(mileage_low);
        offset = 2;
    }
    led_drvier_show_volt(0, 0);
    r_printf("mileage_low:%d  ", mileage_low);
    led_drvier_show_string((u8 *)bcd_number, led_drvier_var.bShowBuffMileage, offset);
    led_drvier_show_km(1, 0);

    // 占用的3位电量显示数字
    memset(led_drvier_var.bShowBuffPower, 0, sizeof(led_drvier_var.bShowBuffPower));
    if (mileage >= 1000) {
        u16 mileage_high;
        mileage_high = mileage / 1000;
        offset = 0; //不显示前面的0
        if (mileage_high >= 100) {
            itoa3(mileage_high);
        } else if (mileage_high >= 10) {
            itoa2(mileage_high);
            offset = 1;
        } else {
            itoa1(mileage_high);
            offset = 2;
        }
        led_drvier_show_string((u8 *)bcd_number, led_drvier_var.bShowBuffPower, offset);
        led_drvier_show_percent(0, 0);
        led_drvier_show_volt(0, 0);
    }
}

/*----------------------------------------------------------------------------*/
// 小数点图标显示控制
/*----------------------------------------------------------------------------*/
void led_drvier_show_led_dot(u8 ctr, bool flag)
{
    if (ctr) {
        led_drvier_show_icon_bit(XIAO_SHU_DIAN);
        if (flag) {
            led_drvier_flash_icon_bit(XIAO_SHU_DIAN);
        } else {
            led_drvier_clean_flash_icon_bit(XIAO_SHU_DIAN);
        }
    } else {
        led_drvier_clean_icon_bit(XIAO_SHU_DIAN);


        led_drvier_clean_flash_icon_bit(XIAO_SHU_DIAN);
    }
}

// P档显示
void led_drvier_show_park(void)
{
    u8 offset = 0; //不显示前面的0
    char string_park[] = " P";
    led_drvier_show_string((u8 *)string_park, led_drvier_var.bShowBuffSpeed, offset);
}

/*----------------------------------------------------------------------------*/
// 图标bit显示设置
/*----------------------------------------------------------------------------*/
void led_drvier_show_icon_bit(u8 bit)
{
    u8 index_byte = 0, index_bit = 0;
    index_byte = bit / 8;
    index_bit = bit % 8;
    // log_info("led_drvier_show_icon_bit byte:%d  bit:%d",index_byte,index_bit);
    /* if(index_byte >= sizeof(led_drvier_var.bShowBuffIcon)){ */
    /*     log_error("led_drvier_show_icon_bit--POINTER-ERR"); */
    /* } */
    led_drvier_var.bShowBuffIcon[index_byte] |= BIT(index_bit);
    led_drvier_show_normal();
}


/*----------------------------------------------------------------------------*/
// 图标bit显示关闭设置
/*----------------------------------------------------------------------------*/
void led_drvier_clean_icon_bit(u8 bit)
{
    u8 index_byte = 0, index_bit = 0;
    index_byte = bit / 8;
    index_bit = bit % 8;
    /* if(index_byte >= sizeof(led_drvier_var.bShowBuffIcon)){ */
    /*     log_error("led_drvier_show_icon_bit--POINTER-ERR"); */
    /* } */
    led_drvier_var.bShowBuffIcon[index_byte] &= ~BIT(index_bit);
    led_drvier_show_normal();
}


/*----------------------------------------------------------------------------*/
// 图标bit 闪烁设置
/*----------------------------------------------------------------------------*/
void led_drvier_flash_icon_bit(u8 bit)
{
    u8 index_byte = 0, index_bit = 0;
    index_byte = bit / 8;
    index_bit = bit % 8;
    led_drvier_var.bFlashIcon[index_byte] |= BIT(index_bit);
}


/*----------------------------------------------------------------------------*/
// 图标bit 闪烁清除设置
/*----------------------------------------------------------------------------*/
void led_drvier_clean_flash_icon_bit(u8 bit)
{
    u8 index_byte = 0, index_bit = 0;
    index_byte = bit / 8;
    index_bit = bit % 8;
    led_drvier_var.bFlashIcon[index_byte] &= ~BIT(index_bit);
}

/*----------------------------------------------------------------------------*/
// 流水灯控制
/*----------------------------------------------------------------------------*/
void led_drvier_led_cycle_ctr(u8 ctr)
{
    led_drvier_var.bLedCycle = ctr;
}

/*----------------------------------------------------------------------------*/
// 流水灯显示扫描
/*----------------------------------------------------------------------------*/
void led_drvier_show_led_cycle_scan(u8 ctr)
{
    u8 temp;
    /* led_drvier_led_cycle_ctr(ctr); */
    if (led_drvier_var.bLedCycle == 0xff) {
        // 全亮
        for (temp = LIU_SHUI_0; temp <= LIU_SHUI_17; temp++) {
            led_drvier_show_icon_bit(temp);
        }
    } else {
        if (led_drvier_var.bLedCycle == 0) {
            for (temp = LIU_SHUI_0; temp <= LIU_SHUI_17; temp++) {
                /* log_char('A'+temp); */
                led_drvier_clean_icon_bit(temp);
            }
            return;
        }
        if (led_drvier_var.bLedCycle > CYCLE_LED_GROUP) {
            led_drvier_var.bLedCycle = 1;
        }
        for (temp = LIU_SHUI_0; temp <= LIU_SHUI_17; temp++) {
            if (temp >= (LIU_SHUI_0 + led_drvier_var.bLedCycle * GROUP_CYCLE_LED_NUMBER)) {
                led_drvier_clean_icon_bit(temp);
            } else {
                led_drvier_show_icon_bit(temp);
            }
        }
        led_drvier_var.bLedCycle++;

        r_printf("led_drvier_var.bLedCycle:%d  temp:%d", led_drvier_var.bLedCycle, temp);
    }
}



/*----------------------------------------------------------------------------*/
// 电池电量格子图标显示   volt_percent=1表示传入的是电压  0表示传入的是百分比
/*----------------------------------------------------------------------------*/
void led_drvier_show_led_bat_level(u8 bat_level, bool flag)
{
    u8 temp;
    if (flag == 0) {
        for (temp = DIAN_LIANG_0; temp < (DIAN_LIANG_0 + (5 * BAT_LEVEL_LED_NUMBER)); temp++) {
            if (temp < (DIAN_LIANG_0 + (bat_level * BAT_LEVEL_LED_NUMBER))) {
                led_drvier_show_icon_bit(temp);
            } else {
                led_drvier_clean_icon_bit(temp);
            }

            /* // 只有一格电量  电量图标闪 */
            /* if (bat_level == 1) { */
            /*     led_drvier_flash_icon_bit(temp); */
            /* } else { */
            /* led_drvier_clean_flash_icon_bit(temp); */
            /* } */
        }
    } else {
        for (temp = DIAN_LIANG_0; temp < (DIAN_LIANG_0 + (bat_level * BAT_LEVEL_LED_NUMBER)); temp++) {
            led_drvier_show_icon_bit(temp);
            // 只有一格电量  电量图标闪
            /* if(bat_level == 1){ */
            led_drvier_flash_icon_bit(temp);
            /* }else{ */
            /* led_drvier_clean_flash_icon_bit(temp); */
            /* } */
        }
    }
}

/*----------------------------------------------------------------------------*/
// 蓝牙显示
/*----------------------------------------------------------------------------*/
void led_drvier_show_bluetooth(u8 ctr, bool flag)
{
    u8 temp;
    if (ctr) {
        /* for(temp=LOG_0;temp <= LOG_5;temp++){ */
        temp = LOG_0;
        led_drvier_show_icon_bit(temp);
        if (flag) {
            led_drvier_flash_icon_bit(temp);
        } else {
            led_drvier_clean_flash_icon_bit(temp);
        }
        /* } */
    } else {
        /* for(temp=LOG_0;temp <= LOG_5;temp++){ */
        temp = LOG_0;
        led_drvier_clean_icon_bit(temp);
        led_drvier_clean_flash_icon_bit(temp);
        /* } */
    }
}
/*----------------------------------------------------------------------------*/
// 左转显示
/*----------------------------------------------------------------------------*/
void led_drvier_show_left(u8 ctr, bool flag)
{
    u8 temp;
    if (ctr) {
        /* for(temp=LOG_0;temp <= LOG_5;temp++){ */
        temp = LOG_1;
        led_drvier_show_icon_bit(temp);
        if (flag) {
            led_drvier_flash_icon_bit(temp);
        } else {
            led_drvier_clean_flash_icon_bit(temp);
        }
        /* } */
    } else {
        /* for(temp=LOG_0;temp <= LOG_5;temp++){ */
        temp = LOG_1;
        led_drvier_clean_icon_bit(temp);
        led_drvier_clean_flash_icon_bit(temp);
        /* } */
    }
}

/*----------------------------------------------------------------------------*/
// 直行显示
/*----------------------------------------------------------------------------*/
void led_drvier_show_forward(u8 ctr, bool flag)
{
    u8 temp;
    if (ctr) {
        /* for(temp=LOG_0;temp <= LOG_5;temp++){ */
        temp = LOG_2;
        led_drvier_show_icon_bit(temp);
        if (flag) {
            led_drvier_flash_icon_bit(temp);
        } else {
            led_drvier_clean_flash_icon_bit(temp);
        }
        /* } */
    } else {
        /* for(temp=LOG_0;temp <= LOG_5;temp++){ */
        temp = LOG_2;
        led_drvier_clean_icon_bit(temp);
        led_drvier_clean_flash_icon_bit(temp);
        /* } */
    }
}
/*----------------------------------------------------------------------------*/
// 掉头显示
/*----------------------------------------------------------------------------*/
void led_drvier_show_back(u8 ctr, bool flag)
{
    u8 temp;
    if (ctr) {
        /* for(temp=LOG_0;temp <= LOG_5;temp++){ */
        temp = LOG_3;
        led_drvier_show_icon_bit(temp);
        if (flag) {
            led_drvier_flash_icon_bit(temp);
        } else {
            led_drvier_clean_flash_icon_bit(temp);
        }
        /* } */
    } else {
        /* for(temp=LOG_0;temp <= LOG_5;temp++){ */
        temp = LOG_3;
        led_drvier_clean_icon_bit(temp);
        led_drvier_clean_flash_icon_bit(temp);
        /* } */
    }
}
/*----------------------------------------------------------------------------*/
// 右转显示
/*----------------------------------------------------------------------------*/
void led_drvier_show_right(u8 ctr, bool flag)
{
    u8 temp;
    if (ctr) {
        /* for(temp=LOG_0;temp <= LOG_5;temp++){ */
        temp = LOG_4;
        led_drvier_show_icon_bit(temp);
        if (flag) {
            led_drvier_flash_icon_bit(temp);
        } else {
            led_drvier_clean_flash_icon_bit(temp);
        }
        /* } */
    } else {
        /* for(temp=LOG_0;temp <= LOG_5;temp++){ */
        temp = LOG_4;
        led_drvier_clean_icon_bit(temp);
        led_drvier_clean_flash_icon_bit(temp);
        /* } */
    }
}

/*----------------------------------------------------------------------------*/
// LOG图标显示--这里已经删除为三极管控制导通
/*----------------------------------------------------------------------------*/
void led_drvier_show_led_log(u8 ctr, bool flag)
{
    u8 temp;
    if (ctr) {
        for (temp = LOG_0; temp <= LOG_5; temp++) {
            led_drvier_show_icon_bit(temp);
            if (flag) {
                led_drvier_flash_icon_bit(temp);
            } else {
                led_drvier_clean_flash_icon_bit(temp);
            }
        }
    } else {
        for (temp = LOG_0; temp <= LOG_5; temp++) {
            led_drvier_clean_icon_bit(temp);
            led_drvier_clean_flash_icon_bit(temp);
        }
    }
}

/*----------------------------------------------------------------------------*/
// 左转显示控制
/*----------------------------------------------------------------------------*/
void led_drvier_show_led_turn_l(u8 ctr, bool flag)
{
    if (ctr) {
        led_drvier_show_icon_bit(ZHUAN_XIANG_ZUO_0);
        led_drvier_show_icon_bit(ZHUAN_XIANG_ZUO_1);
        if (flag) {
            led_drvier_flash_icon_bit(ZHUAN_XIANG_ZUO_0);
            led_drvier_flash_icon_bit(ZHUAN_XIANG_ZUO_1);
        } else {
            led_drvier_clean_flash_icon_bit(ZHUAN_XIANG_ZUO_0);
            led_drvier_clean_flash_icon_bit(ZHUAN_XIANG_ZUO_1);
        }
    } else {
        led_drvier_clean_icon_bit(ZHUAN_XIANG_ZUO_0);
        led_drvier_clean_icon_bit(ZHUAN_XIANG_ZUO_1);


        led_drvier_clean_flash_icon_bit(ZHUAN_XIANG_ZUO_0);
        led_drvier_clean_flash_icon_bit(ZHUAN_XIANG_ZUO_1);
    }
}


/*----------------------------------------------------------------------------*/
// 右转显示控制
/*----------------------------------------------------------------------------*/
void led_drvier_show_led_turn_r(u8 ctr, bool flag)
{
    if (ctr) {
        led_drvier_show_icon_bit(ZHUAN_XIANG_YOU_0);
        led_drvier_show_icon_bit(ZHUAN_XIANG_YOU_1);
        if (flag) {
            led_drvier_flash_icon_bit(ZHUAN_XIANG_YOU_0);
            led_drvier_flash_icon_bit(ZHUAN_XIANG_YOU_1);
        } else {
            led_drvier_clean_flash_icon_bit(ZHUAN_XIANG_YOU_0);
            led_drvier_clean_flash_icon_bit(ZHUAN_XIANG_YOU_1);
        }
    } else {
        led_drvier_clean_icon_bit(ZHUAN_XIANG_YOU_0);
        led_drvier_clean_icon_bit(ZHUAN_XIANG_YOU_1);


        led_drvier_clean_flash_icon_bit(ZHUAN_XIANG_YOU_0);
        led_drvier_clean_flash_icon_bit(ZHUAN_XIANG_YOU_1);
    }
}

/*----------------------------------------------------------------------------*/
// 大灯图标显示控制
/*----------------------------------------------------------------------------*/
void led_drvier_show_led_main_light(u8 ctr, bool flag)
{
    if (ctr) {
        led_drvier_show_icon_bit(DA_DENG);
        if (flag) {
            led_drvier_flash_icon_bit(DA_DENG);
        } else {
            led_drvier_clean_flash_icon_bit(DA_DENG);
        }
    } else {
        led_drvier_clean_icon_bit(DA_DENG);

        led_drvier_clean_flash_icon_bit(DA_DENG);
    }
}


/*----------------------------------------------------------------------------*/
// 廊灯图标显示控制
/*----------------------------------------------------------------------------*/
void led_drvier_show_led_board_light(u8 ctr, bool flag)
{
    if (ctr) {
        led_drvier_show_icon_bit(SHI_LANG_DENG);
        if (flag) {
            led_drvier_flash_icon_bit(SHI_LANG_DENG);
        } else {
            led_drvier_clean_flash_icon_bit(SHI_LANG_DENG);
        }
    } else {
        led_drvier_clean_icon_bit(SHI_LANG_DENG);

        led_drvier_clean_flash_icon_bit(SHI_LANG_DENG);
    }
}


/*----------------------------------------------------------------------------*/
// 油壶图标显示控制
/*----------------------------------------------------------------------------*/
void led_drvier_show_led_oilcan(u8 ctr, bool flag)
{
    if (ctr) {
        led_drvier_show_icon_bit(YOU_HU);
        if (flag) {
            led_drvier_flash_icon_bit(YOU_HU);
        } else {
            led_drvier_clean_flash_icon_bit(YOU_HU);
        }
    } else {
        led_drvier_clean_icon_bit(YOU_HU);
        led_drvier_clean_flash_icon_bit(YOU_HU);
    }
}

/*----------------------------------------------------------------------------*/
// READY图标显示控制
/*----------------------------------------------------------------------------*/
void led_drvier_show_led_ready(u8 ctr, bool flag)
{
    if (ctr) {
        led_drvier_show_icon_bit(READY_0);
        led_drvier_show_icon_bit(READY_1);
        if (flag) {
            led_drvier_flash_icon_bit(READY_0);
            led_drvier_flash_icon_bit(READY_1);
        } else {
            led_drvier_clean_flash_icon_bit(READY_0);
            led_drvier_clean_flash_icon_bit(READY_1);
        }
    } else {
        led_drvier_clean_icon_bit(READY_0);
        led_drvier_clean_icon_bit(READY_1);


        led_drvier_clean_flash_icon_bit(READY_0);
        led_drvier_clean_flash_icon_bit(READY_1);
    }
}


/*----------------------------------------------------------------------------*/
// 挡位图标显示控制
/*----------------------------------------------------------------------------*/
void led_drvier_show_led_gears(u8 ctr, bool flag)
{
    u8 temp;

    if (ctr == 0) {
        for (u8 i = DANG_WEI_1; i <= DANG_WEI_4; i++) {
            led_drvier_clean_icon_bit(i);
            led_drvier_clean_flash_icon_bit(i);
        }
        return ;
    }
    temp = ctr;
    if (temp > (DANG_WEI_4 - DANG_WEI_1)) {
        temp = DANG_WEI_4;
    } else {
        temp += DANG_WEI_1 - 1;
    }
    /* r_printf("led_drvier_show_led_gears-temp:%d  ",temp); */
    if (temp) {
        led_drvier_show_icon_bit(temp);
        if (flag) {
            led_drvier_flash_icon_bit(temp);
        }
    }
    for (u8 i = DANG_WEI_1; i <= DANG_WEI_4; i++) {
        // 挡位只显示一个数字
        if (temp != i) {
            led_drvier_clean_icon_bit(i);
            led_drvier_clean_flash_icon_bit(i);
        }
    }
}


/*----------------------------------------------------------------------------*/
// 充电图标显示控制
/*----------------------------------------------------------------------------*/
void led_drvier_show_led_charge(u8 ctr, bool flag)
{
    if (ctr) {
        led_drvier_show_icon_bit(YOU_HU);
        if (flag) {
            led_drvier_flash_icon_bit(YOU_HU);
        } else {
            led_drvier_clean_flash_icon_bit(YOU_HU);
        }
    } else {
        led_drvier_clean_icon_bit(YOU_HU);

        led_drvier_clean_flash_icon_bit(YOU_HU);
    }
}


/*----------------------------------------------------------------------------*/
// 巡航图标显示控制
/*----------------------------------------------------------------------------*/
void led_drvier_show_led_cruise(u8 ctr, bool flag)
{
    if (ctr) {
        led_drvier_show_icon_bit(XUN_HANG);
        if (flag) {
            led_drvier_flash_icon_bit(XUN_HANG);
        } else {
            led_drvier_clean_flash_icon_bit(XUN_HANG);
        }
    } else {
        led_drvier_clean_icon_bit(XUN_HANG);

        led_drvier_clean_flash_icon_bit(XUN_HANG);
    }
}


/*----------------------------------------------------------------------------*/
// 故障图标显示控制
/*----------------------------------------------------------------------------*/
void led_drvier_show_led_fault(u8 ctr, bool flag)
{
    if (ctr) {
        led_drvier_show_icon_bit(GU_ZHANG);
        if (flag) {
            led_drvier_flash_icon_bit(GU_ZHANG);
        } else {
            led_drvier_clean_flash_icon_bit(GU_ZHANG);
        }
    } else {
        led_drvier_clean_icon_bit(GU_ZHANG);

        led_drvier_clean_flash_icon_bit(GU_ZHANG);
    }
}

/*----------------------------------------------------------------------------*/
// 电机故障图标显示控制
/*----------------------------------------------------------------------------*/
void led_drvier_show_led_motor_fault(u8 ctr, bool flag)
{
    if (ctr) {
        led_drvier_show_icon_bit(M_GU_ZHANG);
        if (flag) {
            led_drvier_flash_icon_bit(M_GU_ZHANG);
        } else {
            led_drvier_clean_flash_icon_bit(M_GU_ZHANG);
        }
    } else {
        led_drvier_clean_icon_bit(M_GU_ZHANG);

        led_drvier_clean_flash_icon_bit(M_GU_ZHANG);
    }
}


/*----------------------------------------------------------------------------*/
// 控制故障图标显示控制
/*----------------------------------------------------------------------------*/
void led_drvier_show_led_ctr_fault(u8 ctr, bool flag)
{
    if (ctr) {
        led_drvier_show_icon_bit(KONG_ZHI_GU_ZHANG);
        if (flag) {
            led_drvier_flash_icon_bit(KONG_ZHI_GU_ZHANG);
        } else {
            led_drvier_clean_flash_icon_bit(KONG_ZHI_GU_ZHANG);
        }
    } else {
        led_drvier_clean_icon_bit(KONG_ZHI_GU_ZHANG);

        led_drvier_clean_flash_icon_bit(KONG_ZHI_GU_ZHANG);
    }
}


/*----------------------------------------------------------------------------*/
// 把手故障图标显示控制
/*----------------------------------------------------------------------------*/
void led_drvier_show_handle_fault(u8 ctr, bool flag)
{
    if (ctr) {
        led_drvier_show_icon_bit(BA_SHOU_GUZHANG);
        if (flag) {
            led_drvier_flash_icon_bit(BA_SHOU_GUZHANG);
        } else {
            led_drvier_clean_flash_icon_bit(BA_SHOU_GUZHANG);
        }
    } else {
        led_drvier_clean_icon_bit(BA_SHOU_GUZHANG);

        led_drvier_clean_flash_icon_bit(BA_SHOU_GUZHANG);
    }
}


/*----------------------------------------------------------------------------*/
// 速度 km/h图标显示控制
/*----------------------------------------------------------------------------*/
void led_drvier_show_speed_kmh(u8 ctr, bool flag)
{
    if (ctr) {
        led_drvier_show_icon_bit(KM_H);
        if (flag) {
            led_drvier_flash_icon_bit(KM_H);
        } else {
            led_drvier_clean_flash_icon_bit(KM_H);
        }
    } else {
        led_drvier_clean_icon_bit(KM_H);

        led_drvier_clean_flash_icon_bit(KM_H);
    }
}


/*----------------------------------------------------------------------------*/
// 电压V图标显示控制
/*----------------------------------------------------------------------------*/
void led_drvier_show_volt(u8 ctr, bool flag)
{
    if (ctr) {
        led_drvier_show_icon_bit(FU_TE);
        if (flag) {
            led_drvier_flash_icon_bit(FU_TE);
        } else {
            led_drvier_clean_flash_icon_bit(FU_TE);
        }
    } else {
        led_drvier_clean_icon_bit(FU_TE);

        led_drvier_clean_flash_icon_bit(FU_TE);
    }
}


/*----------------------------------------------------------------------------*/
// 电量百分比图标显示控制
/*----------------------------------------------------------------------------*/
void led_drvier_show_percent(u8 ctr, bool flag)
{
    if (ctr) {
        led_drvier_show_icon_bit(BAI_FEN_BI);
        if (flag) {
            led_drvier_flash_icon_bit(BAI_FEN_BI);
        } else {
            led_drvier_clean_flash_icon_bit(BAI_FEN_BI);
        }
    } else {
        led_drvier_clean_icon_bit(BAI_FEN_BI);

        led_drvier_clean_flash_icon_bit(BAI_FEN_BI);
    }
}


/*----------------------------------------------------------------------------*/
// 总里程KM图标显示控制
/*----------------------------------------------------------------------------*/
void led_drvier_show_km(u8 ctr, bool flag)
{
    if (ctr) {
        led_drvier_show_icon_bit(KM);
        if (flag) {
            led_drvier_flash_icon_bit(KM);
        } else {
            led_drvier_clean_flash_icon_bit(KM);
        }
    } else {
        led_drvier_clean_icon_bit(KM);

        led_drvier_clean_flash_icon_bit(KM);
    }
}

/*----------------------------------------------------------------------------*/
// SOC图标显示控制
/*----------------------------------------------------------------------------*/
void led_drvier_show_soc(u8 ctr, bool flag)
{
    if (ctr) {
        led_drvier_show_icon_bit(SOC);
        if (flag) {
            led_drvier_flash_icon_bit(SOC);
        } else {
            led_drvier_clean_flash_icon_bit(SOC);
        }
    } else {
        led_drvier_clean_icon_bit(SOC);

        led_drvier_clean_flash_icon_bit(SOC);
    }
}



void icon_show_test(u8 ctr)
{
    // r_printf("ctr:%d ",ctr);
    switch (ctr) {
    case 1:
        led_drvier_show_led_bat_level(1, 0);
        led_drvier_led_cycle_ctr(1);
        led_drvier_show_speed(22);
        led_drvier_show_battery_volt(72);
        led_drvier_show_led_gears(1, 0);
        // led_drvier_show_battery_percent(7);
        // led_drvier_show_mileage(22);
        led_drvier_show_total_mileage(1000);
        break;
    case 2:
        led_drvier_show_led_bat_level(2, 0);
        led_drvier_show_led_gears(2, 0);
        led_drvier_show_mileage(22);
        led_drvier_show_park();

        break;
    case 3:
        led_drvier_show_led_bat_level(3, 0);
        led_drvier_show_led_gears(3, 0);
        led_drvier_show_battery_volt(72);

        break;
    case 4:
        led_drvier_show_led_bat_level(4, 0);
        led_drvier_show_led_gears(4, 0);

        break;
    case 5:
        led_drvier_show_led_bat_level(5, 0);
        break;
    case 6:
        led_drvier_show_led_turn_l(1, 1);
        break;
    case 7:
        led_drvier_show_led_turn_r(1, 1);
        break;
    case 8:
        led_drvier_show_led_main_light(1, 0);
        break;
    case 9:
        led_drvier_show_led_board_light(1, 0);
        break;
    case 10:
        led_drvier_show_led_ready(1, 0);
        break;
    case 11:
        led_drvier_show_led_gears(1, 0);

    case 12:
        led_drvier_show_led_charge(1, 0);
        break;
    case 13:
        led_drvier_show_led_cruise(1, 0);
        break;
    case 14:
        led_drvier_show_led_fault(1, 0);
        break;
    case 15:
        led_drvier_show_led_ctr_fault(1, 0);
        break;
    case 16:
        led_drvier_show_handle_fault(1, 0);
        break;
        break;
    case 22:
        led_drvier_show_led_log(1, 0);
        break;
    case 23:
        led_drvier_show_led_motor_fault(1, 0);
        break;
    case 24:
        led_drvier_show_null();
        break;
    default:
        break;
    }
}

static u8 brightness = 0, waterfall_light_flag = 0;
static u16 waterfall_light_handle = 0;
void led_api_main_entrance(u32 cmd, u8 enable, u8 blink, int value)
{
    //检测大灯是否打开
    if (enable && (cmd == UI_BIG_LAMPS)) {
        brightness = 1;
    } else if (!enable && (cmd == UI_BIG_LAMPS)) {
        brightness = 0;
    }

    if (brightness == 0) {
        set_LED_brightness(1);
    } else {
        set_LED_brightness(2);
    }

    switch (cmd) {
    case UI_LEFT_STEER:
        led_drvier_show_led_turn_r(enable, blink);
        break;

    case UI_RIGHT_STEER:
        led_drvier_show_led_turn_l(enable, blink);
        break;

    case UI_SPEED:
        led_drvier_show_speed(value);
        break;

    case UI_PARKING:
        led_drvier_show_park();
        break;

    case UI_OUTLINE_MAKER_LAMPS:
        led_drvier_show_led_board_light(enable, blink);
        break;

    case UI_BIG_LAMPS:
        led_drvier_show_led_main_light(enable, blink);
        break;

    case UI_OILER:
        led_drvier_show_led_oilcan(enable, blink);
        break;

    case UI_GEARS:
        led_drvier_show_led_gears(value, blink);
        break;

    case UI_CRUISE:
        led_drvier_show_led_cruise(enable, blink);
        break;

    case UI_BRAKE:
        led_drvier_show_led_fault(enable, blink);
        break;

    case UI_MOTOR_FAULT:
        led_drvier_show_led_motor_fault(enable, blink);
        break;

    case UI_TURN_THE_FAULT:
        led_drvier_show_handle_fault(enable, blink);
        break;

    case UI_CONTROL_FAULT:
        led_drvier_show_led_ctr_fault(enable, blink);
        break;

    case UI_START_UP:
        //开机动画
        /* led_drvier_show_led_board_light(enable, blink); */
        /* set_LED_all_on(); */
        /* led_api_main_entrance(UI_LOG, 1, 0, 0); */
        led_drvier_show_battery_volt(value);
        break;

    case UI_WATERFALL_LIGHT:

        if (enable) {
            if (waterfall_light_handle) {
                /* sys_timer_del(waterfall_light_handle); */
                /* waterfall_light_handle = 0; */
                /* waterfall_light_handle = sys_timer_add(NULL, led_drvier_show_led_cycle_scan, 1000); */
            } else {
                printf("open tiemr");
                led_drvier_led_cycle_ctr(enable);
                waterfall_light_flag = 0;
                waterfall_light_handle = sys_timer_add(enable, led_drvier_show_led_cycle_scan, 1000);
            }
        } else {
            if (waterfall_light_handle) {
                printf("close tiemr");
                sys_timer_del(waterfall_light_handle);
                waterfall_light_handle = 0;
                waterfall_light_flag = 0;
            }
            if (waterfall_light_flag == 0) {
                led_drvier_led_cycle_ctr(0xff);
                led_drvier_show_led_cycle_scan(enable);
                waterfall_light_flag = 1;
            }
        }
        /* led_drvier_show_led_board_light(enable, blink); */
        break;

    case UI_MILEAGE:
        led_drvier_show_led_dot(enable, blink);
        led_drvier_show_mileage(value);
        break;

    case UI_TOTAL_MILEAGE:
        led_drvier_show_total_mileage(value);
        break;

    case UI_READY_LAMPS:
        led_drvier_show_led_ready(enable, blink);
        break;

    case UI_BLUETOOTHE_LAMPS:
        led_drvier_show_bluetooth(enable, blink);
        break;

    case UI_SOC_LAMPS:
        led_drvier_show_soc(enable, blink);
        break;

    case UI_BATTER:
        led_drvier_show_led_bat_level(value, blink);
        break;

    case UI_SHUTDOWM:
        /* led_drvier_show_null(); */
        /* led_api_main_entrance(UI_LOG, 0, 0, 0); */
        printf("ui shutdown");
        if (waterfall_light_handle) {
            sys_timer_del(waterfall_light_handle);
            waterfall_light_handle = 0;
        }
        led_drvier_led_cycle_ctr(0);
        led_drvier_show_led_cycle_scan(0);

        led_drvier_var.bShowTotalMileage = 0;
        /* led_drvier_show_normal(); */
        /* sys_timeout_add(NULL, led_drvier_show_null, 500); */
        led_drvier_show_null();
        break;

    case UI_LOG:
        /* led_drvier_show_led_log(enable, blink); */
        break;

    default:
        break;
    }
}

#endif


