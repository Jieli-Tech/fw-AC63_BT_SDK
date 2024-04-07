#ifndef POWER_INTERFACE_H
#define POWER_INTERFACE_H

// #include "asm/hwi.h"
//
#include "generic/typedef.h"
#define NEW_BASEBAND_COMPENSATION       0

#define AT_VOLATILE_RAM             AT(.volatile_ram)
#define AT_VOLATILE_RAM_CODE        AT(.volatile_ram_code)

/*复位原因*/
enum {
    /*主系统*/
    MSYS_P11_RST = 0,
    MSYS_DVDD_POR_RST = 1,
    MSYS_DVDD_OK_RST = 2,
    MSYS_SOFT_RST = 5,
    MSYS_P2M_RST = 6,
    MSYS_POWER_RETURN = 7,
    /*P11*/
    P11_PVDD_POR_RST = 8,
    P11_IVS_RST = 9,
    P11_P33_RST = 10,
    P11_WDT_RST = 11,
    P11_SOFT_RST = 12,
    P11_MSYS_RST = 13,
    P11_POWER_RETURN = 15,
    /*P33*/
    P33_VDDIO_POR_RST = 16,
    P33_VDDIO_LVD_RST = 17,
    P33_VCM_RST = 18,
    P33_PPINR_RST = 19,
    P33_P11_RST = 20,
    P33_SOFT_RST = 21,
    P33_POWER_RETURN = 23,
    /*SUB*/
    P33_EXCEPTION_SOFT_RST = 24,
    P33_ASSERT_SOFT_RST = 25,
    MSYS_P11_RST_RTC_WAKEUP = 26,
};

#define SLEEP_EN                            BIT(2)

enum {
    OSC_TYPE_LRC = 0,
    OSC_TYPE_BT_OSC,
};

//修改支持预编译方式定义
#ifndef   PWR_NO_CHANGE
#define   PWR_NO_CHANGE             0
#define   PWR_LDO33                 1
#define   PWR_LDO15                 2
#define   PWR_DCDC15                3
#define   PWR_DCDC15_FOR_CHARGE     4
#endif

enum {
    LONG_4S_RESET = 0,
    LONG_8S_RESET,
};

//Macro for VDDIOM_VOL_SEL
enum {
    VDDIOM_VOL_20V = 0,
    VDDIOM_VOL_22V,
    VDDIOM_VOL_24V,
    VDDIOM_VOL_26V,
    VDDIOM_VOL_28V,
    VDDIOM_VOL_30V, //default
    VDDIOM_VOL_32V,
    VDDIOM_VOL_34V,
};

//Macro for VDDIOW_VOL_SEL
enum {
    VDDIOW_VOL_20V = 0,
    VDDIOW_VOL_22V,
    VDDIOW_VOL_24V,
    VDDIOW_VOL_26V,
    VDDIOW_VOL_28V,
    VDDIOW_VOL_30V,
    VDDIOW_VOL_32V,
    VDDIOW_VOL_34V,
};

struct low_power_param {
    u8 osc_type;
    u32 btosc_hz;
    u8  delay_us;
    u8  config;
    u8  btosc_disable;

    u8 vddiom_lev;
    u8 vddiow_lev;
    u8 pd_wdvdd_lev;
    u8 lpctmu_en;
    u8 vddio_keep;

    u32 osc_delay_us;

    u8 vd13_cap_en;
    u8 rtc_clk;
    u8 light_sleep_attribute;

};

#define BLUETOOTH_RESUME    BIT(1)

#define RISING_EDGE         0
#define FALLING_EDGE        1
#define BOTH_EDGE           2

enum {
    PORT_FLT_NULL = 0,
    PORT_FLT_256us,
    PORT_FLT_512us,
    PORT_FLT_1ms,
    PORT_FLT_2ms,
    PORT_FLT_4ms,
    PORT_FLT_8ms,
    PORT_FLT_16ms,
};

struct port_wakeup {
    u8 pullup_down_enable;        //
    u8 edge;        //[0]: Rising / [1]: Falling [2]: Rising and Falling
    u8 both_edge;
    u8 filter;
    u8 iomap;       //Port Group-Port Index
};


struct lvd_wakeup {
    u8 attribute;   //Relate operation bitmap OS_RESUME | BLUETOOTH_RESUME
};


//<Max hardware wakeup port
#define MAX_WAKEUP_PORT     12
#define MAX_WAKEUP_ANA_PORT 3

struct wakeup_param {
    const struct port_wakeup *port[MAX_WAKEUP_PORT];
    const struct port_wakeup *aport[MAX_WAKEUP_ANA_PORT];
    const struct charge_wakeup *charge;
    const struct alarm_wakeup *alram;
    const struct lvd_wakeup *lvd;
    const struct sub_wakeup *sub;
};

struct reset_param {
    u8 en;
    u8 mode;
    u8 level;
    u8 iomap;   //Port Group, Port Index
};

struct low_power_operation {

    const char *name;

    u32(*get_timeout)(void *priv);

    void (*suspend_probe)(void *priv);

    void (*suspend_post)(void *priv, u32 usec);

    void (*resume)(void *priv, u32 usec);

    void (*resume_post)(void *priv, u32 usec);
};

u32 __tus_carry(u32 x);

u8 __power_is_poweroff(void);

void poweroff_recover(void);

void power_init(const struct low_power_param *param);

u8 power_is_low_power_probe(void);

u8 power_is_low_power_post(void);

void power_set_soft_poweroff();

void power_set_soft_poweroff_advance();

void power_set_mode(u8 mode);

void power_set_charge_mode(u8 mode);

void power_keep_dacvdd_en(u8 en);

struct soft_flag0_t {
    u8 wdt_dis: 1;
    u8 poweroff: 1;
    u8 is_port_b: 1;
    u8 res: 5;
};
struct soft_flag1_t {
    u8 usbdp: 2;
    u8 usbdm: 2;
    u8 uart_key_port: 1;
    u8 ldoin: 3;
};
struct soft_flag2_t {
    u8 pa7: 4;
    u8 pb4: 4;
};
struct soft_flag3_t {
    u8 pc3: 4;
    u8 pc5: 4;
};

struct boot_soft_flag_t {
    union {
        struct soft_flag0_t boot_ctrl;
        u8 value;
    } flag0;
    union {
        struct soft_flag1_t misc;
        u8 value;
    } flag1;
    union {
        struct soft_flag2_t pa7_pb4;
        u8 value;
    } flag2;
    union {
        struct soft_flag3_t pc3_pc5;
        u8 value;
    } flag3;
};
enum soft_flag_io_stage {
    SOFTFLAG_HIGH_RESISTANCE,
    SOFTFLAG_PU,
    SOFTFLAG_PD,

    SOFTFLAG_OUT0,
    SOFTFLAG_OUT0_HD0,
    SOFTFLAG_OUT0_HD,
    SOFTFLAG_OUT0_HD0_HD,

    SOFTFLAG_OUT1,
    SOFTFLAG_OUT1_HD0,
    SOFTFLAG_OUT1_HD,
    SOFTFLAG_OUT1_HD0_HD,
};
void mask_softflag_config(const struct boot_soft_flag_t *softflag);
void power_set_callback(u8 mode, void (*powerdown_enter)(u8 step), void (*powerdown_exit)(u32), void (*soft_poweroff_enter)(void));

// u8 power_is_poweroff_post(void);
#define  power_is_poweroff_post()   0

void power_set_proweroff(void);

u8 power_reset_source_dump(void);
/*-----------------------------------------------------------*/

void low_power_request(void);

void *low_power_get(void *priv, const struct low_power_operation *ops);

void low_power_put(void *priv);

void low_power_sys_request(void *priv);

void *low_power_sys_get(void *priv, const struct low_power_operation *ops);

void low_power_sys_put(void *priv);

u8 low_power_sys_is_idle(void);

s32 low_power_trace_drift(u32 usec);

u32 rtc_lrc_trim(u32 usec, u16 *remain);

void low_power_reset_osc_type(u8 type);

u8 low_power_get_default_osc_type(void);

u8 low_power_get_osc_type(void);
/*-----------------------------------------------------------*/

void power_wakeup_index_enable(u8 index);

void power_wakeup_index_disable(u8 index);

void power_wakeup_disable_with_port(u8 port);

void power_wakeup_enable_with_port(u8 port);

void power_wakeup_set_edge(u8 port_io, u8 edge);

void power_awakeup_index_enable(u8 index);

void power_awakeup_index_disable(u8 index);

void power_awakeup_disable_with_port(u8 port);

void power_awakeup_enable_with_port(u8 port);

void power_wakeup_init(const struct wakeup_param *param);
void port_edge_wkup_set_callback(void (*wakeup_callback)(u8 index, u8 gpio));
void aport_edge_wkup_set_callback(void (*wakeup_callback)(u8 index, u8 gpio, u8 edge));

void alm_wkup_set_callback(void (*wakeup_callback)(u8 index));

void power_wakeup_init_test();

u8 get_wakeup_source(void);

u8 is_ldo5v_wakeup(void);
// void power_wakeup_callback(JL_SignalEvent_t cb_event);
u8 is_alarm_wakeup(void);

void p33_soft_reset(void);
/*-----------------------------------------------------------*/

void power_reset_close();

void lrc_debug(u8 a, u8 b);

void sdpg_config(int enable);

void p11_init(void);
/*-----------------------------------------------------------*/
u8 get_wvdd_trim_level();

u8 get_pvdd_trim_level();

void update_wvdd_pvdd_trim_level(u8 wvdd_level, u8 pvdd_level);

u32 get_reset_source_value(void);

u8 check_wvdd_pvdd_trim(u8 tieup);
/*-----------------------------------------------------------*/



//配置Low power target 睡眠深度
// -- LOW_POWER_MODE_SLEEP : 系统掉电，RAM 进入省电模式，数字逻辑不掉电，模拟掉电
// -- LOW_POWER_MODE_LIGHT_SLEEP : 系统不掉电，BTOSC 保持，系统运行RC
// -- LOW_POWER_MODE_DEEP_SLEEP : 数字逻辑不掉电，模拟掉电
enum LOW_POWER_LEVEL {
    LOW_POWER_MODE_SLEEP = 0,
    LOW_POWER_MODE_LIGHT_SLEEP,
    LOW_POWER_MODE_DEEP_SLEEP,
};

#define LOWPOWER_LIGHT_SLEEP_ATTRIBUTE_KEEP_CLOCK 		BIT(0)

typedef u8(*idle_handler_t)(void);
typedef enum LOW_POWER_LEVEL(*level_handler_t)(void);

struct lp_target {
    char *name;
    level_handler_t level;
    idle_handler_t is_idle;
};

#define REGISTER_LP_TARGET(target) \
        const struct lp_target target sec(.lp_target)


extern const struct lp_target lp_target_begin[];
extern const struct lp_target lp_target_end[];

#define list_for_each_lp_target(p) \
    for (p = lp_target_begin; p < lp_target_end; p++)
/*-----------------------------------------------------------*/

struct deepsleep_target {
    char *name;
    u8(*enter)(void);
    u8(*exit)(void);
};

#define DEEPSLEEP_TARGET_REGISTER(target) \
        const struct deepsleep_target target sec(.deepsleep_target)


extern const struct deepsleep_target deepsleep_target_begin[];
extern const struct deepsleep_target deepsleep_target_end[];

#define list_for_each_deepsleep_target(p) \
    for (p = deepsleep_target_begin; p < deepsleep_target_end; p++)
/*-----------------------------------------------------------*/

#endif
