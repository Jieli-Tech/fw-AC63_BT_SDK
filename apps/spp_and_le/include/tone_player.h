#ifndef _TONE_PLAYER_
#define _TONE_PLAYER_

#include "app_config.h"

#if (!defined CONFIG_CPU_BD29)
#include "audio_config.h"
#endif

#if TCFG_NOR_FS && (TCFG_VIRFAT_FLASH_ENABLE == 0)
#define NOR_FLASH_RES_ROOT_PATH	 "storage/res_nor/C/"
#define TONE_RES_ROOT_PATH		 NOR_FLASH_RES_ROOT_PATH    //内置flash提示音根路径
#else
#define TONE_RES_ROOT_PATH		 SDFILE_RES_ROOT_PATH   	//内置flash提示音根路径
#endif//TCFG_NOR_FS

#define TONE_STOP       0
#define TONE_START      1

#define CONFIG_USE_DEFAULT_SINE     1

#define TONE_DEFAULT_VOL   SYS_MAX_VOL

enum {
    IDEX_TONE_NUM_0,
    IDEX_TONE_NUM_1,
    IDEX_TONE_NUM_2,
    IDEX_TONE_NUM_3,
    IDEX_TONE_NUM_4,
    IDEX_TONE_NUM_5,
    IDEX_TONE_NUM_6,
    IDEX_TONE_NUM_7,
    IDEX_TONE_NUM_8,
    IDEX_TONE_NUM_9,
    IDEX_TONE_BT_MODE,
    IDEX_TONE_BT_CONN,
    IDEX_TONE_BT_DISCONN,
    IDEX_TONE_TWS_CONN,
    IDEX_TONE_TWS_DISCONN,
    IDEX_TONE_LOW_POWER,
    IDEX_TONE_POWER_OFF,
    IDEX_TONE_POWER_ON,
    IDEX_TONE_RING,
    IDEX_TONE_MAX_VOL,
    IDEX_TONE_NORMAL,
    IDEX_TONE_MUSIC,
    IDEX_TONE_LINEIN,
    IDEX_TONE_FM,
    IDEX_TONE_PC,
    IDEX_TONE_RTC,
    IDEX_TONE_RECORD,
    IDEX_TONE_UDISK,
    IDEX_TONE_SD,

    IDEX_TONE_NONE = 0xFF,
};

#define TONE_NUM_0      		TONE_RES_ROOT_PATH"tone/0.*"
#define TONE_NUM_1      		TONE_RES_ROOT_PATH"tone/1.*"
#define TONE_NUM_2				TONE_RES_ROOT_PATH"tone/2.*"
#define TONE_NUM_3				TONE_RES_ROOT_PATH"tone/3.*"
#define TONE_NUM_4				TONE_RES_ROOT_PATH"tone/4.*"
#define TONE_NUM_5				TONE_RES_ROOT_PATH"tone/5.*"
#define TONE_NUM_6				TONE_RES_ROOT_PATH"tone/6.*"
#define TONE_NUM_7				TONE_RES_ROOT_PATH"tone/7.*"
#define TONE_NUM_8				TONE_RES_ROOT_PATH"tone/8.*"
#define TONE_NUM_9				TONE_RES_ROOT_PATH"tone/9.*"
#define TONE_BT_MODE			TONE_RES_ROOT_PATH"tone/bt.*"
#define TONE_BT_CONN       		TONE_RES_ROOT_PATH"tone/bt_conn.*"
#define TONE_BT_DISCONN    		TONE_RES_ROOT_PATH"tone/bt_dconn.*"
#define TONE_TWS_CONN			TONE_RES_ROOT_PATH"tone/tws_conn.*"
#define TONE_TWS_DISCONN		TONE_RES_ROOT_PATH"tone/tws_dconn.*"
#define TONE_LOW_POWER			TONE_RES_ROOT_PATH"tone/low_power.*"
#define TONE_POWER_OFF			TONE_RES_ROOT_PATH"tone/power_off.*"
#define TONE_POWER_ON			TONE_RES_ROOT_PATH"tone/power_on.*"
#define TONE_RING				TONE_RES_ROOT_PATH"tone/ring.*"
#define TONE_MAX_VOL			TONE_RES_ROOT_PATH"tone/vol_max.*"
#define TONE_MUSIC				TONE_RES_ROOT_PATH"tone/music.*"
#define TONE_LINEIN				TONE_RES_ROOT_PATH"tone/linein.*"
#define TONE_PC 				TONE_RES_ROOT_PATH"tone/pc.*"
#define TONE_FM 				TONE_RES_ROOT_PATH"tone/fm.*"
#define TONE_RTC 				TONE_RES_ROOT_PATH"tone/rtc.*"
#define TONE_RECORD 			TONE_RES_ROOT_PATH"tone/record.*"
#define TONE_SPDIF 			    TONE_RES_ROOT_PATH"tone/spdif.*"

#ifdef CONFIG_CPU_BR18
#undef TONE_POWER_ON
#undef TONE_POWER_OFF
#undef TONE_BT_CONN
#undef TONE_BT_DISCONN

#define TONE_POWER_ON			TONE_RES_ROOT_PATH"power_on.mp3"
#define TONE_POWER_OFF			TONE_RES_ROOT_PATH"power_off.*"
#define TONE_BT_CONN       		TONE_RES_ROOT_PATH"bt_conn.mp3"
#define TONE_BT_DISCONN    		TONE_RES_ROOT_PATH"bt_dconn.mp3"
#endif


#define SINE_WTONE_NORAML           0
#define SINE_WTONE_TWS_CONNECT      1
#define SINE_WTONE_TWS_DISCONNECT   2
#define SINE_WTONE_LOW_POWER        4
#define SINE_WTONE_RING             5
#define SINE_WTONE_MAX_VOLUME       6
#define SINE_WTONE_ADSP             7
#define SINE_WTONE_LOW_LATENRY_IN   8
#define SINE_WTONE_LOW_LATENRY_OUT  9


#define TONE_REPEAT_BEGIN(a)  (char *)((0x1 << 30) | (a & 0xffff))
#define TONE_REPEAT_END()     (char *)(0x2 << 30)

#define IS_REPEAT_BEGIN(a)    ((((u32)a >> 30) & 0x3) == 0x1 ? 1 : 0)
#define IS_REPEAT_END(a)      ((((u32)a >> 30) & 0x3) == 0x2 ? 1 : 0)
#define TONE_REPEAT_COUNT(a)  (((u32)a) & 0xffff)

#define DEFAULT_SINE_TONE(a)     (char *)(((u32)0x3 << 30) | (a))
#define IS_DEFAULT_SINE(a)       ((((u32)a >> 30) & 0x3) == 0x3 ? 1 : 0)
#define DEFAULT_SINE_ID(a)       ((u32)a & 0xffff)

#if CONFIG_USE_DEFAULT_SINE
#undef TONE_TWS_CONN
#define TONE_TWS_CONN           DEFAULT_SINE_TONE(SINE_WTONE_TWS_CONNECT)
#undef TONE_TWS_DISCONN
#define TONE_TWS_DISCONN        DEFAULT_SINE_TONE(SINE_WTONE_TWS_DISCONNECT)

#undef TONE_MAX_VOL
#define TONE_MAX_VOL            DEFAULT_SINE_TONE(SINE_WTONE_MAX_VOLUME)

#undef TONE_NORMAL
#define TONE_NORMAL            DEFAULT_SINE_TONE(SINE_WTONE_TWS_DISCONNECT)
#if BT_PHONE_NUMBER

#else
#undef TONE_RING
#define TONE_RING               DEFAULT_SINE_TONE(SINE_WTONE_RING)
#endif

#undef TONE_LOW_POWER
#define TONE_LOW_POWER          DEFAULT_SINE_TONE(SINE_WTONE_LOW_POWER)
#endif

#define TONE_SIN_NORMAL			DEFAULT_SINE_TONE(SINE_WTONE_NORAML)

extern const char *tone_table[];
int tone_play(const char *name, u8 preemption);
int tone_play_init(void);
int tone_play_index(u8 index, u8 preemption);
int tone_play_index_with_callback(u8 index, u8 preemption, void (*user_evt_handler)(void *priv), void *priv);
int tone_file_list_play(const char **list, u8 preemption);
int tone_play_stop(void);
int tone_play_stop_by_index(u8 index);
int tone_play_stop_by_path(char *path);
int tone_get_status();
int tone_play_by_path(const char *name, u8 preemption);

int tone_dec_wait_stop(u32 timeout_ms);

// 按序列号播放提示音
int tone_play_with_callback_by_index(u8 index, // 序列号
                                     u8 preemption, // 打断标记
                                     void (*evt_handler)(void *priv, int flag), // 事件回调接口 //flag: 0正常关闭，1被打断关闭
                                     void *evt_priv // 事件回调私有句柄
                                    );

// 按名字播放提示音
int tone_play_with_callback_by_name(char *name, // 带有路径的文件名
                                    u8 preemption, // 打断标记
                                    void (*evt_handler)(void *priv, int flag), // 事件回调接口 //flag: 0正常关闭，1被打断关闭
                                    void *evt_priv // 事件回调私有句柄
                                   );

// 按列表播放提示音
int tone_play_with_callback_by_list(const char **list, // 文件名列表
                                    u8 preemption, // 打断标记
                                    void (*evt_handler)(void *priv, int flag), // 事件回调接口 //flag: 0正常关闭，1被打断关闭
                                    void *evt_priv // 事件回调私有句柄
                                   );

#endif
