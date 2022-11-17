
#include "system/app_core.h"
#include "system/includes.h"

#include "app_config.h"
#include "app_action.h"
/* #include "one_line.h" */
#if   LED_162X_EN
#include "tm162x.h"
#include "led_api.h"
#include "ui_common.h"

// 程序默认对应共阳连接方式 IO必须严格对应改了IO或者数码管顺序需要仔细核对修改
// 如果是共阴连接方式程序修改会非常简单


const u8 volt_to_level[MAX_BAT_LEVEL] = {36, 48, 60, 70, 100};
#define DISMODE 0x01 //显示模式设置
#define WRITEMODE_UP 0x40//采取地址自动加一得方式写现存
#define READKEYMODE   0x42 //读取按键命令
#define READSWMODE    0x43
#define WRITEDATAMODE 0x44 //采用固定地址方式写显存
#define WRITELEDMODE  0x45 //采用固定地址方式写LED显存
#define STARTADDRESS  0xc0 //起始地址
#define DISCONMODE    0x8F //显示控制
#define DATACOUNT     (2*grid) //采用地址自动加一方式传输数据的个数


#define DISP_SCAN_TIMES   5
#define TM1668_OFF        0x80 //关显示
#define TM1668_LED_25P    0x81 //低亮
#define TM1668_LED_90P    0x87 //正常


//  第一个表示寄存器地址   第二个表示BIT 位
// 序号必须和H文件中图标编号枚举值严格对应
const u8 ico_status[][2]  = {
    1, 0,      //流水灯0
    3, 0,
    5, 0,
    7, 0,
    9, 0,
    11, 0,
    13, 0,
    15, 0,

    1, 1,      //流水灯8
    3, 1,
    5, 1,
    7, 1,
    9, 1,
    11, 1,
    13, 1,
    15, 1,

    1, 2,      //16
    3, 2,      //17

    12, 7,      //电量灯 18
    10, 7,
    8, 7,
    6, 7,
    4, 7,
    2, 7,

    0, 7,      //24
    14, 6,
    12, 6,
    10, 6,
    8, 6,
    6, 6,
    4, 6,      //29
    2, 6,


    0, 6,      //32

    5, 2,      //LOG 33
    7, 2,
    9, 2,
    11, 2,
    13, 2,
    15, 2,

    0, 0,      //39

    2, 0,      //40
    8, 0,      //41
    10, 0,     //42
    6, 0,      //43
    4, 0,      //44
    14, 1,      //45
    14, 2,      //46
    0, 5,      //47

    2, 5,      //48
    4, 5,      //49
    6, 5,      //50
    14, 7,      //51

    14, 0,     //52
    8, 5,    //53
    10, 5,    //54
    12, 5,    //55

    14, 5,      //56
    14, 3,      //57
    15, 5,     //58
    15, 6,     //59
    15, 7,     //60
    12, 0,      //61
    14, 4,
};


u8 disp_mode = LIGHT_LEVEL_1;



static void delay_us(u32 n)
{
    delay(n);
}

/*----------------------------------------------------------------------------*/
/**@brief    tm1628写一个字节	上升沿写
   @param	 无
   @return	 无
   @note     void indata(u8 p)
*/
/*----------------------------------------------------------------------------*/
void tm162xin(u8 p)
{
    u32 i;
    gpio_write(LED_TM162X_STB, 0);
    // 注意 低位在前
    for (i = 0; i < 8; i++) {
        gpio_write(LED_TM162X_CLK, 0);
        if ((p & 0x01) != 0) {
            gpio_write(LED_TM162X_DIO, 1);
        } else {
            gpio_write(LED_TM162X_DIO, 0);
        }
        gpio_write(LED_TM162X_CLK, 1);
        delay_us(DELAY_TIME_NUM);
        p = p >> 1;
    }
}

void open_tm162xin_en()
{
    gpio_write(LED_TM162X_STB, 1);
    tm162xin(disp_mode);
    gpio_write(LED_TM162X_STB, 1);
}

/*----------------------------------------------------------------------------*/
/**@brief    tm1628采用地址自动加一方式传输地址和数据开始  采用地址自动加一方式
   @param	 无
   @return	 无
   @note     void LED_162X(void)
*/
/*----------------------------------------------------------------------------*/

void led1629_scan(void *priv)
{
    u8 i, j;
    static u8 buff_temp[BYTE_NUM];
    static u8 last_buff[BYTE_NUM];
    static u16 cnt = 0;
    // r_printf("LED_162X");

    led1629_dispBuff_driverBuff();
    memcpy(buff_temp, led_drvier_var.bDriverBuff, BYTE_NUM);
    if (!memcmp(last_buff, buff_temp, 16)) {
        return;
    }


    // memset(led_drvier_var.bDriverBuff,0,16);
    // cnt++;
    // if((cnt % 2000)==0){
    //     if((cnt/2000) >= 16){
    //         cnt=0;
    //     }
    //     led_drvier_var.bDriverBuff[cnt/2000] = 0xff;
    //     r_printf("data:%d  ",cnt/2000);
    // }else{
    //     return;
    // }
    // memcpy(buff_temp,led_drvier_var.bDriverBuff,BYTE_NUM);




    gpio_write(LED_TM162X_DIO, 1);
    gpio_write(LED_TM162X_CLK, 1);
    gpio_write(LED_TM162X_STB, 1);
    tm162xin(STARTADDRESS);

    memcpy(last_buff, buff_temp, BYTE_NUM);
    printf("tm162xin buf:");
    put_buf(buff_temp, BYTE_NUM);
    for (i = 0; i < BYTE_NUM; i++) {
        tm162xin(buff_temp[i]);
    }
    // put_buf(buff_temp,BYTE_NUM);
    gpio_write(LED_TM162X_STB, 1);
    tm162xin(disp_mode);
    gpio_write(LED_TM162X_STB, 1);

    /* sys_timeout_add(NULL, open_tm162xin_en, 5); */
}




u8 led1629_char(u8 chardata)
{
    u8 i, temp;
    if ((chardata >= '0') && (chardata <= '9')) {
        temp = LED_NUMBER[chardata - '0'];
    } else if ((chardata >= 'a') && (chardata <= 'z')) {
        temp = LED_SMALL_LETTER[chardata - 'a'];
    } else if ((chardata >= 'A') && (chardata <= 'Z')) {
        temp = LED_LARGE_LETTER[chardata - 'A'];
    }
    return temp;
}

void led1629_string(u8 *led_dis)
{
    u8 i = 0, j = 0;
    while (0 != *led_dis) {
        // led1629_var.bShowBuff[i] = led1629_char(*led_dis++);
        i++;
    }
}


// 显示内容缓存buff  数据解析转换到数据驱动buff
void led1629_dispBuff_driverBuff(void)
{
    u8 i, j;
    static u8 temp = 0;
    static u16 time_cnt = 0;
    time_cnt++;
    memset(led_drvier_var.bDriverBuff, 0, 16);
    if (led_drvier_var.bLedOnOff == 0) { //全灭
        return;
    } else if (led_drvier_var.bLedOnOff == 0xff) { //全亮
        memset(led_drvier_var.bDriverBuff, 0xff, 16);
        return;
    }


    // 两个大的速度显示位置
    // led_drvier_var.bShowBuffSpeed[0] = show_number[temp];
    // led_drvier_var.bShowBuffSpeed[1] = show_number[temp];
    // 偶数下标的第一位表示第一格  第二位表示第二格     第二第三格组成一个8 第四第五格组成一个8
    for (i = 0; i < 7; i++) {
        if (led_drvier_var.bShowBuffSpeed[0] & BIT(i)) {
            led_drvier_var.bDriverBuff[2 * i] |= BIT(1);
            led_drvier_var.bDriverBuff[2 * i] |= BIT(2);
        }
        if (led_drvier_var.bShowBuffSpeed[1] & BIT(i)) {
            led_drvier_var.bDriverBuff[2 * i] |= BIT(3);
            led_drvier_var.bDriverBuff[2 * i] |= BIT(4);
        }
    }

    // 电量数字显示
    // led_drvier_var.bShowBuffPower[2] = temp;
    // led_drvier_var.bShowBuffPower[0] = show_number[temp];
    // led_drvier_var.bShowBuffPower[1] = show_number[temp];
    // 奇数下标的第一位表示第一格  第二位表示第二格     第二第三格组成一个8 第四第五格组成一个8
    for (i = 0; i < 7; i++) {
        if (led_drvier_var.bShowBuffPower[1] & BIT(i)) {
            led_drvier_var.bDriverBuff[2 * i + 1] |= BIT(3);
        }
        if (led_drvier_var.bShowBuffPower[2] & BIT(i)) {
            led_drvier_var.bDriverBuff[2 * i + 1] |= BIT(4);
        }
    }
    if (led_drvier_var.bShowBuffPower[0]) {
        led_drvier_var.bDriverBuff[15] |= BIT(3);
        led_drvier_var.bDriverBuff[15] |= BIT(4);
    } else {
        led_drvier_var.bDriverBuff[15] &= ~BIT(3);
        led_drvier_var.bDriverBuff[15] &= ~BIT(4);
    }


    // 里程数字显示
    // led_drvier_var.bShowBuffMileage[0] = show_number[temp];
    // led_drvier_var.bShowBuffMileage[1] = show_number[temp];
    // led_drvier_var.bShowBuffMileage[2] = show_number[temp];
    for (i = 0; i < 7; i++) {
        if (led_drvier_var.bShowBuffMileage[0] & BIT(i)) {
            led_drvier_var.bDriverBuff[2 * i + 1] |= BIT(5);
        }
        if (led_drvier_var.bShowBuffMileage[1] & BIT(i)) {
            led_drvier_var.bDriverBuff[2 * i + 1] |= BIT(6);
        }
        if (led_drvier_var.bShowBuffMileage[2] & BIT(i)) {
            led_drvier_var.bDriverBuff[2 * i + 1] |= BIT(7);
        }
    }

    // 图标显示
    // memset(led_drvier_var.bShowBuffIcon,0xff,8);
    for (i = 0; i < (sizeof(led_drvier_var.bShowBuffIcon)); i++) {
        for (j = 0; j < 8; j++) {
            if (led_drvier_var.bShowBuffIcon[i] & BIT(j)) {
                // 判断图标闪烁
                if (time_cnt % (LED_FLASH_TIME * 2) > LED_FLASH_TIME) {
                    if (led_drvier_var.bFlashIcon[i] & BIT(j)) {
                        continue;
                    }
                }
                led_drvier_var.bDriverBuff[ico_status[i * 8 + j][0]] |= BIT(ico_status[i * 8 + j][1]);
            }
        }
    }


    // memset(led_drvier_var.bDriverBuff,0xff,16);
    // temp++;
    // if(temp > 9){
    //     temp = 0;
    // }

}



void led1629_test(void)
{
    static u8 cnt = 0;
    // led1629_dispBuff_driverBuff();
    icon_show_test(cnt);
    cnt++;
    if (cnt >= 25) {
        cnt = 0;
    }
}


void led1629_init(void)
{
    s32 ret;
    gpio_set_direction(LED_TM162X_DIO, 0);
    gpio_set_direction(LED_TM162X_CLK, 0);
    gpio_set_direction(LED_TM162X_STB, 0);
    gpio_set_pull_down(LED_TM162X_DIO, 0);//close pulldown
    gpio_set_pull_down(LED_TM162X_CLK, 0);//close pulldown
    gpio_set_pull_down(LED_TM162X_STB, 0);//close pulldown
    gpio_set_pull_up(LED_TM162X_DIO, 0);  //close pullup
    gpio_set_pull_up(LED_TM162X_CLK, 0);  //close pullup
    gpio_set_pull_up(LED_TM162X_STB, 0);  //close pullup

    gpio_write(LED_TM162X_DIO, 1);
    gpio_write(LED_TM162X_CLK, 1);
    gpio_write(LED_TM162X_STB, 1);
    tm162xin(WRITEMODE_UP);
    gpio_write(LED_TM162X_STB, 1);
    ret = sys_timer_add(NULL, (void *)led1629_scan, DISP_SCAN_TIMES);
    /* ret = sys_timer_add(NULL,(void *)led1629_test,1000); */
}

void led1629_ctr(LED_1688_CTR ctr)
{
    switch (ctr) {
    case LED_188_OFF:
        puts(" LED_188_OFF:    \n");
        gpio_set_direction(LED_TM162X_DIO, 0);
        gpio_set_direction(LED_TM162X_CLK, 0);
        gpio_set_direction(LED_TM162X_STB, 0);
        gpio_set_pull_down(LED_TM162X_DIO, 0);//close pulldown
        gpio_set_pull_down(LED_TM162X_CLK, 0);//close pulldown
        gpio_set_pull_down(LED_TM162X_STB, 0);//close pulldown
        gpio_set_pull_up(LED_TM162X_DIO, 0);  //close pullup
        gpio_set_pull_up(LED_TM162X_CLK, 0);  //close pullup
        gpio_set_pull_up(LED_TM162X_STB, 0);  //close pullup
        gpio_write(LED_TM162X_DIO, 1);
        gpio_write(LED_TM162X_CLK, 1);
        gpio_write(LED_TM162X_STB, 1);
        tm162xin(TM1668_OFF);
        gpio_write(LED_TM162X_STB, 1);
        break;
    case LED_188_NORMAL:
        /* puts(" LED_188_NORMAL:    \n"); */
//            tm162xin(TM1668_LED_25P);
        disp_mode = LIGHT_LEVEL_1;
        break;
    case LED_188_LOW:
        /* puts(" LED_188_LOW:    \n"); */
//            tm162xin(TM1668_LED_90P);
        disp_mode = LIGHT_LEVEL_2;
        break;
    }
}


void led_driver_init(void)
{
    r_printf("led1629_init");
    led1629_init();
}


#endif

