//bin文件输出数据结构汇总

//配置保留ID = 0x00, 0x0001, len = 0x3C

//key_msg, ID = 606, CFG_KEY_MSG_ID, 0x8189, len  = 0x3C = 60
typedef struct _KEY_MSG {
    u8 short_msg;     //短按消息
    u8 long_msg;      //长按消息
    u8 hold_msg;      //hold 消息
    u8 up_msg;        //抬键消息
    u8 double_msg;    //双击消息
    u8 triple_msg;    //三击消息
} KEY_MSG;

typedef struct __KEY_MSG {
    KEY_MSG key_msg[KEY_NUM];  //KEY_NUM指在user_cfg.lua文件配置的num
};


//audio, ID = 524, CFG_AUDIO_ID, 0x8301, len = 0x04
typedef struct _AUDIO_CONFIG {
    u8 sw;
    u8 max_sys_vol;         //最大系统音量
    u8 default_vol;         //开机默认音量
    u8 tone_vol;            //提示音音量, 为0跟随系统音量
} AUDIO_CFG;

//charge, ID = 535, CFG_CHARGE_ID, 0x85C1, 0x05
typedef struct __CHARGE_CONFIG {
    u8 sw;                  //开关
	u8 poweron_en;          //支持开机充电
    u8 full_v;              //充满电压
	u8 full_c;              //充满电流
	u8 charge_c;            //充电电流
} CHARGE_CONFIG;


//蓝牙配置
//mac: ID: 102, CFG_BT_MAC_ADDR, 0x4BC1, len = 0x06
typedef struct __BLUE_MAC {
    u8 mac[6];
} BLUE_MAC_CFG;


//rf_power: ID: 601, CFG_BT_RF_POWER_ID, 0x8601, len = 0x01
typedef struct __BLUE_RF_POWER {
    u8 rf_power;
} RF_POWER_CFG;

//name: ID: 101, CFG_BT_NAME, 0x4B81, len = 32 * 20 = 0x0294 = 660
typedef struct __BLUE_NAME {
    u8 name[32];
} BLUE_NAME_CFG;

//aec_cfg: ID: 604, CFG_AEC_ID, 0x8681, len = 0x14 = 20
typedef struct __AEC {
    u8 mic_again; 
    u8 dac_again; 
    u16 ndt_max_gain;   //default: 384(0 ~ 2048)
    u16 ndt_min_gain;   //default: 64(0 ~ 2048)
    u16 ndt_fade_gain;  //default: 32(0 ~ 2048)
    u16 nearend_thr;    //default: 50(0 ~ 1024)
    u32 nlp_aggress;    //float, default: 4.0f(0 ~ 35)
    u32 nlp_suppress;   //float, default: 5.0f(0 ~ 10)
    u8 aec_mode;        //diable(0), reduce(1), advance(2)
    u8 ul_eq_en;        //disable(0), enable(1)
} _GNU_PACKED_ AEC;

//tws_pair_code: ID: 602, CFG_TWS_PAIR_CODE_ID, 0x8781, len = 0x02
typedef struct __TWS_PAIR_CODE {
    u16 tws_pair_code;
} TWS_PAIR_CODE_CFG;

//status: ID: 605, CFG_UI_TONE_STATUS_ID, 0x8701, len = 0x1D = 29
typedef struct __STATUS {
    u8 charge_start;    //开始充电
    u8 charge_full;     //充电完成
    u8 power_on;        //开机
    u8 power_off;       //关机
    u8 lowpower;        //低电
    u8 max_vol;         //最大音量
    u8 phone_in;        //来电
    u8 phone_out;       //去电
    u8 phone_activ;     //通话中
    u8 bt_init_ok;      //蓝牙初始化完成
    u8 bt_connect_ok;   //蓝牙连接成功
    u8 bt_disconnect;   //蓝牙断开
    u8 tws_connect_ok;   //蓝牙连接成功
    u8 tws_disconnect;   //蓝牙断开
} _GNU_PACKED_ STATUS;

typedef struct __STATUS_CONFIG {
    u8 status_sw;
    STATUS led;    //led status
    STATUS tone;   //tone status
} STATUS_CONFIG_CFG;



//MIC免电容方案需要设置，影响MIC的偏置电压
//[1]:16K
//[2]:7.5K
//[3]:5.1K 
//[4]:6.8K
//[5]:4.7K
//[6]:3.5K
//[7]:2.9K
//[8]:3K
//[9]:2.5K
//[10]:2.1K
//[11]:1.9K
//[12]:2K
//[13]:1.8K
//[14]:1.6K
//[15]:1.5K
//[16]:1K
//[31]:0.6K
//MIC_TYPE, ID: 537, CFG_MIC_TYPE_ID, 0x89C1, len = 0x03
typedef struct __MIC_TYPE_SEL {
    u8 mic_capless; //MIC免电容方案
    u8 mic_bias_res;    //MIC免电容方案需要设置，影响MIC的偏置电压 1:16K 2:7.5K 3:5.1K 4:6.8K 5:4.7K 6:3.5K 7:2.9K  8:3K  9:2.5K 10:2.1K 11:1.9K  12:2K  13:1.8K 14:1.6K  15:1.5K 16:1K 31:0.6K
    u8 mic_ldo_vsel;//00:2.3v 01:2.5v 10:2.7v 11:3.0v
} MIC_TYPE_SEL;

//LOWPOWER_VOLTAGE, ID: 536, CFG_LOWPOWER_VOLTAGE_ID, 0x, len = 0x04
typedef struct __LOWPOWER_VOLTAGE {
    u16 tone_voltage;       //低电提醒电压, 默认340->3.4V
    u16 power_off_voltage;  //低电关机电压, 默认330->3.3V
} LOWPOWER_VOLTAGE;

//AUTO_OFF_TIME, ID: 603, CFG_AUTO_OFF_TIME_ID, 0x, len = 0x01
typedef struct __AUTO_OFF_TIME {
    u8 auto_off_time;  //没有连接自动关机时间, 单位：分钟, 默认3分钟
} AUTO_OFF_TIME;

//LRC_CFG, ID: 607, CFG_LRC_ID, 0x, len = 0x09
typedef struct __LRC_CONFIG {
    u16 lrc_ws_inc;
    u16 lrc_ws_init;
    u16 btosc_ws_inc;
    u16 btosc_ws_init;
    u8 lrc_change_mode;
} _GNU_PACKED_ LRC_CONFIG;

//BLE_CFG, ID: 608, CFG_BLE_ID, 0x, len = 0x0
typedef struct __BLE_CONFIG {
    u8 ble_cfg_en;      //ble配置有效标志
    u8 ble_name[32];    //ble蓝牙名
    u8 ble_rf_power;    //ble发射功率
    u8 ble_addr_en;     //ble 地址有效标志
    u8 ble_addr[6];     //ble地址, 若ble_addr_sw = 1; 则有效
} _GNU_PACKED_ BLE_CONFIG;

