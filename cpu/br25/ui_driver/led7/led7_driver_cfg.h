#ifndef _LED7_CFG_H_
#define _LED7_CFG_H_


#if TCFG_UI_LED7_ENABLE
#define AT_LED_CONST             AT(.LED_const)
//LED7真值表选择
//7脚LED7
// #define UI_LED7_PIN7_TRUE_TABLE1
//#define UI_LED7_PIN7_TRUE_TABLE2
#define UI_LED7_PIN7_TRUE_TABLE3

//12脚LED7
//#define UI_LED7_PIN12_TRUE_TABLE1
//13脚LED7
// #define UI_LED7_PIN13_TRUE_TABLE1

#ifdef  UI_LED7_PIN7_TRUE_TABLE1
/*********** 数码管(7脚)真值表1 **************/
/*
   0    1    2     3     4     5     6
0  X    1A   1B    1E    USB   X     X
1  1F   X    2A    2B    2E   2D     X
2  1G   2F   X     :     3B   X      X
3  1C   2G   3F    X     3C   4E     X
4  1D   2C   3G    3A    X    4C     4G
5  3D   SD   3E    4D    4F   X      4B
6 CHARGE BT  X     REP   DOT  4A 	 X
*/

// 7断数码管通用数字类转换表
AT_LED_CONST
static const struct seg2pin led7_digit_seg2pin[28] = {
//pinH, pinL
    {0, 1}, //1A
    {0, 2}, //1B
    {3, 0}, //1C
    {4, 0}, //1D
    {0, 3}, //1E
    {1, 0}, //1F
    {2, 0}, //1G

    {1, 2}, //2A
    {1, 3}, //2B
    {4, 1}, //2C
    {1, 5}, //2D
    {1, 4}, //2E
    {2, 1}, //2F
    {3, 1}, //2G

    {4, 3}, //3A
    {2, 4}, //3B
    {3, 4}, //3C
    {5, 0}, //3D
    {5, 2}, //3E
    {3, 2}, //3F
    {4, 2}, //3G

    {6, 5}, //4A
    {5, 6}, //4B
    {4, 5}, //4C
    {5, 3}, //4D
    {3, 5}, //4E
    {5, 4}, //4F
    {4, 6}, //4G
};

// 数码管字母类转换表
AT_LED_CONST
static const struct icon_seg2pin led7_icon_seg2pin[] = {
    //icon       pinH, pinL
    {LED7_CHARGE, 	{6, 0}},
    {LED7_USB, 	  	{0, 4}},
    {LED7_SD, 	  	{5, 1}},
    {LED7_BT, 	  	{6, 1}}, //BT
    {LED7_REPEAT, 	{6, 3}}, //REPEAT
    {LED7_2POINT, 	{2, 3}}, //:
    {LED7_DOT, 		{6, 4}} //.
};

#endif /* #ifdef  UI_LED7_PIN7_TRUE_TABLE1 */


#ifdef  UI_LED7_PIN7_TRUE_TABLE2
/*********** 数码管(7脚)真值表 2**************/
/* 0    1     2     3     4     5     6
0  X    2A    2B    2C    2D    2E    2F
1  1A   X     2G    :     X     MHz   MP3
2  1B   1G    X     4A    4B    4C    4D
3  1C   PLAY  3A    X     4E    4F    4G
4  1D   PAUSE 3B    3E    X     X     X
5  1E   USB   3C    3F    X     X     X
6  1F   SD    3D    3G    X     X     X */

// 7断数码管通用数字类转换表
AT_LED_CONST
static const struct seg2pin led7_digit_seg2pin[28] = {
//pinH, pinL
    {1, 0}, //1A
    {2, 0}, //1B
    {3, 0}, //1C
    {4, 0}, //1D
    {5, 0}, //1E
    {6, 0}, //1F
    {2, 1}, //1G

    {0, 1}, //2A
    {0, 2}, //2B
    {0, 3}, //2C
    {0, 4}, //2D
    {0, 5}, //2E
    {0, 6}, //2F
    {1, 2}, //2G

    {3, 2}, //3A
    {4, 2}, //3B
    {5, 2}, //3C
    {6, 2}, //3D
    {4, 3}, //3E
    {5, 3}, //3F
    {6, 3}, //3G

    {2, 3}, //4A
    {2, 4}, //4B
    {2, 5}, //4C
    {2, 6}, //4D
    {3, 4}, //4E
    {3, 5}, //4F
    {3, 6}, //4G
};

// 数码管字母类转换表
AT_LED_CONST
static const struct icon_seg2pin led7_icon_seg2pin[] = {
    //icon       	pinH, pinL
    {LED7_PLAY, 	{3, 1}},
    {LED7_PAUSE, 	{4, 1}},
    {LED7_USB, 		{5, 1}},
    {LED7_SD, 		{6, 1}},
    {LED7_2POINT, 	{1, 3}},
    {LED7_DOT, 	    {1, 4}},
    {LED7_FM, 		{1, 5}},
    {LED7_MP3, 		{1, 6}},
};

#endif /* #ifdef  UI_LED7_PIN7_TRUE_TABLE2 */

#ifdef  UI_LED7_PIN7_TRUE_TABLE3
/*********** 数码管(7脚)真值表 2**************/
/* 0    1     2     3     4     5      6  (L)
0  X    1A    1B    1E    SD    PAUSE  X
1  1F   X     2A    2B    2E    2D     X
2  1G   2F    X     :     3B   	PLAY   MP3
3  1C   2G    3F    X     3C    4E     X
4  1D   2C    3G    3A    X     4C     4G
5  3D   USB   3E    4D    4F    X      4B
6  X    X     FM    X     .     4A     X */
//(H)

// 7断数码管通用数字类转换表
AT_LED_CONST
static const struct seg2pin led7_digit_seg2pin[28] = {
//pinH, pinL
    {0, 1}, //1A
    {0, 2}, //1B
    {3, 0}, //1C
    {4, 0}, //1D
    {0, 3}, //1E
    {1, 0}, //1F
    {2, 0}, //1G

    {1, 2}, //2A
    {1, 3}, //2B
    {4, 1}, //2C
    {1, 5}, //2D
    {1, 4}, //2E
    {2, 1}, //2F
    {3, 1}, //2G

    {4, 3}, //3A
    {2, 4}, //3B
    {3, 4}, //3C
    {5, 0}, //3D
    {5, 2}, //3E
    {3, 2}, //3F
    {4, 2}, //3G

    {6, 5}, //4A
    {5, 6}, //4B
    {4, 5}, //4C
    {5, 3}, //4D
    {3, 5}, //4E
    {5, 4}, //4F
    {4, 6}, //4G
};

// 数码管字母类转换表
AT_LED_CONST
static const struct icon_seg2pin led7_icon_seg2pin[] = {
    //icon       	pinH, pinL
    {LED7_PLAY, 	{2, 5}},
    {LED7_PAUSE, 	{0, 5}},
    {LED7_USB, 		{5, 1}},
    {LED7_SD, 		{0, 4}},
    {LED7_2POINT, 	{2, 3}},
    {LED7_DOT, 	    {6, 4}},
    {LED7_FM, 		{6, 2}},
    {LED7_MP3, 		{2, 6}},
};

#endif /* #ifdef  UI_LED7_PIN7_TRUE_TABLE2 */



#ifdef  UI_LED7_PIN12_TRUE_TABLE1
/*********** 数码管(12脚)真值表**************/
/* 		0    1     2     3     4     5     6
COM0  	1A   1B    1C    1D    1E    1F    1G
COM1  	2A   2B    2C    2D    2E    2F    2G
COM2  	3A   3B    3C    3D    3E    3F    3G
COM3  	4A   4B    4C    4D    4E    4F    4G
COM4  	AUX  FM    USB   SD    :     WMA   MP3
*/

// 7断数码管通用数字类转换表
AT_LED_CONST
static const struct seg2pin led7_digit_seg2pin[28] = {
//comH, segL
    {0, 0}, //1A
    {0, 1}, //1B
    {0, 2}, //1C
    {0, 3}, //1D
    {0, 4}, //1E
    {0, 5}, //1F
    {0, 6}, //1G

    {1, 0}, //2A
    {1, 1}, //2B
    {1, 2}, //2C
    {1, 3}, //2D
    {1, 4}, //2E
    {1, 5}, //2F
    {1, 6}, //2G

    {2, 0}, //3A
    {2, 1}, //3B
    {2, 2}, //3C
    {2, 3}, //3D
    {2, 4}, //3E
    {2, 5}, //3F
    {2, 6}, //3G

    {3, 0}, //4A
    {3, 1}, //4B
    {3, 2}, //4C
    {3, 3}, //4D
    {3, 4}, //4E
    {3, 5}, //4F
    {3, 6}, //4G
};

//数码管字母类转换表
AT_LED_CONST
static const struct icon_seg2pin led7_icon_seg2pin[] = {
    //icon       comH, segL
    {LED7_AUX, 		{4, 0}},
    {LED7_FM, 		{4, 1}},
    {LED7_USB, 		{4, 2}},
    {LED7_SD, 		{4, 3}},
    {LED7_2POINT, 	{4, 4}},
    {LED7_WMA, 		{4, 5}},
    {LED7_MP3, 		{4, 6}},
};
#endif /* #ifdef  UI_LED7_PIN12_TRUE_TABLE1 */

#ifdef  UI_LED7_PIN13_TRUE_TABLE1
/*********** 数码管(12脚)真值表**************/
/* 		0    1     2     3     4     5     6
COM0  	1A   1B    1C    1D    1E    1F    1G
COM1  	2A   2B    2C    2D    2E    2F    2G
COM2  	3A   3B    3C    3D    3E    3F    3G
COM3  	4A   4B    4C    4D    4E    4F    4G
COM4  	AM   PM    AUX   FM    MUSIC 24H   CLK1
COM5  	CLK2 BT    :     BAT1  BAT2  BAT3  BAT4
*/

// 7断数码管通用数字类转换表
AT_LED_CONST
static const struct seg2pin led7_digit_seg2pin[28] = {
//comL, segH
    {0, 0}, //1A
    {0, 1}, //1B
    {0, 2}, //1C
    {0, 3}, //1D
    {0, 4}, //1E
    {0, 5}, //1F
    {0, 6}, //1G

    {1, 0}, //2A
    {1, 1}, //2B
    {1, 2}, //2C
    {1, 3}, //2D
    {1, 4}, //2E
    {1, 5}, //2F
    {1, 6}, //2G

    {2, 0}, //3A
    {2, 1}, //3B
    {2, 2}, //3C
    {2, 3}, //3D
    {2, 4}, //3E
    {2, 5}, //3F
    {2, 6}, //3G

    {3, 0}, //4A
    {3, 1}, //4B
    {3, 2}, //4C
    {3, 3}, //4D
    {3, 4}, //4E
    {3, 5}, //4F
    {3, 6}, //4G
};

//数码管字母类转换表
AT_LED_CONST
static const struct icon_seg2pin led7_icon_seg2pin[] = {
    //icon          comL, segH
    {LED7_AUX, 		{4, 2}},
    {LED7_FM, 		{4, 3}},
    {LED7_BT, 		{5, 1}},
    {LED7_2POINT, 	{5, 2}},
};
#endif /* #ifdef  UI_LED7_PIN13_TRUE_TABLE1 */

#endif
#endif /* #ifndef _LED7_CFG_H_ */

