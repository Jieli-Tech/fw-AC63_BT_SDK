#ifndef _LED1888_H_
#define _LED1888_H_


#if TCFG_UI_LED1888_ENABLE



#define LED_A   BIT(0)
#define LED_B	BIT(1)
#define LED_C	BIT(2)
#define LED_D	BIT(3)
#define LED_E	BIT(4)
#define LED_F	BIT(5)
#define LED_G	BIT(6)
#define LED_H	BIT(7)


typedef struct _LED188_VAR {
    u8  bCoordinateX;
    u8  bFlashChar;
    u8  bFlashIcon;
    u8  bShowBuff[4];
    u8  bBrightness;
    u8  bShowBuff1[9];
} LED1888_VAR;


void LED1888_setX(u8 X);
void LED1888_clear_icon(void);
void LED1888_clear(void);
void LED1888_init(void);
void LED1888_show_char(u8 chardata);
void LED1888_FlashChar(void);
void LED1888_Clear_FlashChar(void);
void LED1888_show_number(u8 number);
void LED1888_show_string(u8 *str);
void LED1888_show_power_on(void);
void LED1888_show_Hi(void);
void LED1888_show_waiting(void);
void LED1888_show_pause(void);
void LED1888_show_dev(void);
void LED1888_show_filenumber(u16 file_num);
void LED1888_show_volume(u8 vol);
void LED1888_show_freq(u16 freq);
void LED1888_show_freq_setting(u16 freq);
void LED1888_show_IR_number(u16 ir_num);
void LED1888_show_eq(u8 eq);
void LED1888_show_playmode(u8 play_mode);
void LED1888_show_bt_idle(void);
void LED1888_show_bt_call(void);
void LED1888_show_bt_main(void);
void LED1888_show_music_main(void);
void LED1888_show_fm_main(void);
void LED1888_show_aux_main(void);
void LED1888_show_pc_main(void);
void LED1888_show_pc_vol_up(void);
void LED1888_show_pc_vol_down(void);
void LED1888_6p_scan(void);

#endif
#endif/*LED1888_H_*/
