/*--------------------------------------------------------------------------*/
/**@file    led_driver.c
   @brief   led_driver 模块驱动根据原理图修改下面的IO配置和顺序
   @details
   @author  HUANGXIAOWEI
   @date    2022-08
   @note    AD15
*/
/*----------------------------------------------------------------------------*/
#include "ui_api.h"
#include "typedef.h"

#if LED_IO_DRVIER
#include "led_api.h"
#include "led_driver.h"
#include "ui_common.h"
#include "audio.h"
#include "dac_api.h"

#define LOG_TAG_CONST       NORM
#define LOG_TAG             "[led_driver]"
#include "debug.h"



const u8 volt_to_level[MAX_BAT_LEVEL] = {36, 48, 60, 70, 100};


//速度数字 每个段两个灯
u8 speed_num[][4]  = {
    {1, 2, 1, 3},
    {2, 0, 2, 1},
    {2, 6, 2, 7},
    {2, 4, 2, 5},
    {2, 2, 2, 3},
    {1, 4, 1, 5},
    {1, 6, 1, 7},


    {3, 0,   3, 1},
    {3, 6,   3, 7},
    {4, 4,   4, 5},
    {4, 2,   4, 3},
    {4, 0,   4, 1},
    {3, 2,   3, 3},
    {3, 4,   3, 5},
};
//电量数字 每个段1个灯
u8 power_num[][2]  = {
    {0xff, 0xff},
    {11, 1},
    {11, 0},
    {0xff, 0xff},
    {0xff, 0xff},
    {0xff, 0xff},
    {0xff, 0xff},

    {11, 4},
    {12, 0},
    {12, 2},
    {11, 6},
    {11, 3},
    {11, 2},
    {11, 5},

    {12, 4},
    {12, 7},
    {13, 0},
    {12, 6},
    {12, 3},
    {12, 1},
    {12, 5},
};
//里程数字 每个段1个灯
u8 mileage_num[][2]  = {
    {13, 5},
    {14, 0},
    {14, 1},
    {13, 7},
    {13, 4},
    {13, 3},
    {13, 6},

    {14, 4},
    {14, 7},
    {15, 0},
    {14, 6},
    {14, 0},
    {14, 2},
    {14, 5},

    {15, 3},
    {15, 6},
    {15, 7},
    {15, 5},
    {15, 2},
    {15, 1},
    {15, 4},
};

//  第一个表示正极 用A组IO   第二个表示负极 用B组IO
// 序号必须和H文件中图标编号枚举值严格对应
u8 ico_status[][2]  = {
    8, 3,      //流水灯0
    8, 4,
    8, 5,
    9, 0,
    9, 1,
    9, 2,
    9, 3,
    9, 4,

    9, 5,      //8
    9, 6,
    9, 7,      //10
    10, 0,
    10, 1,
    10, 2,
    10, 3,
    10, 4,

    10, 5,     //16
    10, 6,     //17

    8, 0,      //电量灯 18
    8, 1,
    8, 2,      //20
    7, 5,
    7, 6,
    7, 7,

    7, 2,      //24
    7, 3,
    7, 4,
    6, 7,
    7, 0,
    7, 1,      //29
    6, 4,
    6, 5,

    6, 6,      //32

    0, 4,      //LOG 33
    0, 5,
    0, 6,
    0, 7,
    1, 0,
    1, 1,

    0, 0,      //39

    0, 1,      //40
    5, 0,      //41
    5, 1,      //42
    0, 2,      //43
    0, 3,      //44
    4, 6,      //45
    4, 7,      //46
    5, 2,      //47

    5, 3,      //48
    5, 4,      //49
    5, 5,      //50
    8, 7,      //51

    5, 6,      //52
    5, 7,      //53
    6, 0,      //54
    6, 2,      //55

    6, 3,      //56
    6, 1,      //57
    13, 1,     //58
    13, 2,     //59
    10, 7,     //60
    8, 6,      //61

};


/*----------------------------------------------------------------------------*/
/**@brief   LED清屏函数
   @param   x：显示横坐标
   @return  void
   @author  Change.tsai
   @note    void led_drvier_clear(void)
*/
/*----------------------------------------------------------------------------*/
AT_RAM
void led_drvier_clear(void)
{
    JL_PORTA->DIR &= ~0xffff;
    JL_PORTA->PU &= ~0xffff;
    JL_PORTA->PD &= ~0xffff;
    JL_PORTA->OUT &= ~0xffff;
    // JL_PORTA->OUT |= 0xffff;
    JL_PORTA->HD0 |= 0xffff;
    JL_PORTA->HD1 |= 0xffff;


    JL_PORTB->DIR &= ~0xff;
    JL_PORTB->PD &= ~0xff;
    JL_PORTB->PU &= ~0xff;
    JL_PORTB->OUT |= 0xff;
    JL_PORTB->HD0 |= 0xff;
    JL_PORTB->HD1 |= 0xff;
}




AT_RAM
void LED_drive7(void)
{
    u8 k, i, j, temp, temp_flash;
    k = 0;
    static u16 flash_cnt = 0;
    flash_cnt++;

    // 显示缓存清除
    memset(led_drvier_var.bDriverBuff, 0, sizeof(led_drvier_var.bDriverBuff));

    // led_drvier_var.bShowBuffSpeed[0] = LED_NUMBER[3];//0x7f;
    // led_drvier_var.bShowBuffSpeed[1] = LED_NUMBER[3];//0x7f;

    // u8  bShowBuffSpeed[2];          //<显示缓存 速度2位数  注意速度一段有两个灯
    k = 0;
    for (i = 0; i < sizeof(led_drvier_var.bShowBuffSpeed); i++) {
        temp = led_drvier_var.bShowBuffSpeed[i];
        for (j = 0; j < 7; j++) {
            if (temp & BIT(j)) {
                if (speed_num[k][1] == 0xff) { //空
                    k++;
                    continue;
                }
                led_drvier_var.bDriverBuff[speed_num[k][0]] |= BIT(speed_num[k][1]);
                led_drvier_var.bDriverBuff[speed_num[k][2]] |= BIT(speed_num[k][3]);
            }
            k++;
        }
    }
    // led_drvier_var.bShowBuffPower[0] = LED_NUMBER[3];//0x7f;
    // led_drvier_var.bShowBuffPower[1] = LED_NUMBER[3];//0x7f;
    // led_drvier_var.bShowBuffPower[2] = LED_NUMBER[3];//0x7f;
    // u8  bShowBuffPower[3];          //<显示缓存 电量188 3位数
    k = 0;
    for (i = 0; i < sizeof(led_drvier_var.bShowBuffPower); i++) {
        temp = led_drvier_var.bShowBuffPower[i];
        for (j = 0; j < 7; j++) {
            if (temp & BIT(j)) {
                if (power_num[k][1] == 0xff) { //空
                    k++;
                    continue;
                }
                led_drvier_var.bDriverBuff[power_num[k][0]] |= BIT(power_num[k][1]);
            }
            k++;
        }
    }
    // u8  bShowBuffMileage[3];        //<显示缓存 总里程  3位数
    // led_drvier_var.bShowBuffMileage[0] = LED_NUMBER[3];//0x7f;
    // led_drvier_var.bShowBuffMileage[1] = LED_NUMBER[3];//0x7f;
    // led_drvier_var.bShowBuffMileage[2] = LED_NUMBER[3];//0x7f;
    k = 0;
    for (i = 0; i < sizeof(led_drvier_var.bShowBuffMileage); i++) {
        temp = led_drvier_var.bShowBuffMileage[i];
        for (j = 0; j < 7; j++) {
            if (temp & BIT(j)) {
                if (mileage_num[k][1] == 0xff) { //空
                    k++;
                    continue;
                }
                led_drvier_var.bDriverBuff[mileage_num[k][0]] |= BIT(mileage_num[k][1]);
            }
            k++;
        }
    }
    // u8  bShowBuffIcon[8];           //<显示缓存 图标
    k = 0;
    for (i = 0; i < sizeof(led_drvier_var.bShowBuffIcon); i++) {
        temp = led_drvier_var.bShowBuffIcon[i];
        temp_flash = led_drvier_var.bFlashIcon[i];
        for (j = 0; j <= 7; j++) {
            if (temp & BIT(j)) {
                if (ico_status[k][1] == 0xff) { //空
                    k++;
                    continue;
                }
                if (temp_flash & BIT(j)) {
                    if ((flash_cnt % ICON_FLASH_PERIOD) < (ICON_FLASH_PERIOD / 2)) {
                        led_drvier_var.bDriverBuff[ico_status[k][0]] &= ~BIT(ico_status[k][1]);
                    } else {
                        led_drvier_var.bDriverBuff[ico_status[k][0]] |= BIT(ico_status[k][1]);
                    }
                } else {
                    led_drvier_var.bDriverBuff[ico_status[k][0]] |= BIT(ico_status[k][1]);
                }
            }
            k++;
        }
    }
}



/*----------------------------------------------------------------------------*/
/**@brief   LED扫描函数
   @param   void
   @return  void
   @author  Change.tsai
   @note    void led_drvier_scan(void)
*/
/*----------------------------------------------------------------------------*/
AT_RAM
void led_driver_scan(void)
{
    static u8 cnt = 0;

    u8 seg;

    LED_drive7();

    led_drvier_clear();

    cnt++;
    if (cnt >= 8) {
        cnt = 0;
    }
    JL_PORTB->OUT &= ~BIT(cnt);

    for (u8 i = 0; i < 16; i++) {
        seg = led_drvier_var.bDriverBuff[i];
        if (seg & BIT(cnt)) {
            JL_PORTA->OUT |= BIT(i);
        }
    }
    return;
}

#endif


