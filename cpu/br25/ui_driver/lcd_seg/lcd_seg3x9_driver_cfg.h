#ifndef _LCD_SEG3X9_CFG_H_
#define _LCD_SEG3X9_CFG_H_


#if TCFG_UI_LCD_SEG3X9_ENABLE
//LCD_SEG3X9 真值表选择
#define UI_LCD_SEG3X9_TRUE_TABLE1

enum HW_SEG_INDEX {
    LCD_SEG0 = 0,
    LCD_SEG1,
    LCD_SEG2,
    LCD_SEG3,
    LCD_SEG4,
    LCD_SEG5,
    LCD_SEG6,
    LCD_SEG7,
    LCD_SEG8,
    LCD_SEG9,
    LCD_SEG10,
    LCD_SEG11,
    LCD_SEG12,
    LCD_SEG13,
    LCD_SEG14,
    LCD_SEG15,
    LCD_SEG16,
    LCD_SEG17,
    LCD_SEG18,
    LCD_SEG19,
    LCD_SEG20,
    LCD_SEG21,
};

enum HW_COM_INDEX {
    LCD_COM0 = 0,
    LCD_COM1,
    LCD_COM2,
    LCD_COM3,
    LCD_COM4,
    LCD_COM5,
};

struct lcd_seg2pin {
    enum HW_COM_INDEX com_index;
    enum HW_SEG_INDEX seg_index;
};

struct lcd_seg_icon_seg2pin {
    UI_LCD_SEG3X9_ICON icon;
    struct lcd_seg2pin seg2pin;
};

//=================================================================================//
//                        		LCD段码屏配置 									   //
//=================================================================================//
#ifdef UI_LCD_SEG3X9_TRUE_TABLE1

/*********** 段码屏3x9真值表**************/
/* 		0    1     2     3     4     5     6	 7 	   8
COM0  	1D   1C    "1"   2D    2C    SD    3D	 3C    USB
COM1  	1E   1G    VOL   2E    2G    MHz   3E	 3G    NONE
COM2  	1F   1A    1B    2F    2A    2B    3F	 3A    3B
*/
static const struct lcd_seg2pin lcd_seg3x9_seg2pin[] = {
    //com_index, seg_index
    {LCD_COM0, LCD_SEG1}, //1A
    {LCD_COM2, LCD_SEG2}, //1B
    {LCD_COM0, LCD_SEG1}, //1C
    {LCD_COM0, LCD_SEG0}, //1D
    {LCD_COM1, LCD_SEG0}, //1E
    {LCD_COM2, LCD_SEG0}, //1F
    {LCD_COM1, LCD_SEG1}, //1G

    {LCD_COM2, LCD_SEG4}, //2A
    {LCD_COM2, LCD_SEG5}, //2B
    {LCD_COM0, LCD_SEG4}, //2C
    {LCD_COM0, LCD_SEG3}, //2D
    {LCD_COM1, LCD_SEG3}, //2E
    {LCD_COM2, LCD_SEG3}, //2F
    {LCD_COM1, LCD_SEG4}, //2G

    {LCD_COM2, LCD_SEG7}, //3A
    {LCD_COM2, LCD_SEG8}, //3B
    {LCD_COM0, LCD_SEG7}, //3C
    {LCD_COM0, LCD_SEG6}, //3D
    {LCD_COM1, LCD_SEG6}, //3E
    {LCD_COM2, LCD_SEG6}, //3F
    {LCD_COM1, LCD_SEG7}, //3G
};


static const struct lcd_seg_icon_seg2pin lcd_seg3x9_icon_seg2pin[] = {
    //icon       		com_index, seg_index
    {LCD_SEG3X9_USB, 		{LCD_COM0, LCD_SEG8}},
    {LCD_SEG3X9_SD,  		{LCD_COM0, LCD_SEG5}},
    {LCD_SEG3X9_VOL, 		{LCD_COM1, LCD_SEG2}},
    {LCD_SEG3X9_MHZ, 		{LCD_COM1, LCD_SEG5}},
    {LCD_SEG3X9_DIGIT1,		{LCD_COM0, LCD_SEG2}},
};

#endif /* #ifdef  UI_LCD_SEG3X9_TRUE_TABLE1 */

#endif /* #ifndef _LCD_SEG3X9_CFG_H_ */
#endif
