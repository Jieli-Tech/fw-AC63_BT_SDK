#ifndef _BLE_BEACON_H

#define _BLE_BEACON_H

#include "system/app_core.h"
#include "system/includes.h"

#include "app_config.h"
#include "app_action.h"

#include "btstack/btstack_task.h"
#include "btstack/le/ble_api.h"
#include "btstack/bluetooth.h"
#include "btstack//le/le_user.h"
#include "user_cfg.h"
#include "vm.h"
#include "btcontroller_modules.h"
#include "bt_common.h"
#include "3th_profile_api.h"

// #include "le_trans_data.h"
#include "le_common.h"

#include "rcsp_bluetooth.h"
#include "JL_rcsp_api.h"



#pragma pack(1)    //为了保证数据存储地址的连续性, 采用1字节对齐;
typedef  struct {
    u8 length;             //数据总长度(固定值0x1a)
    u8 ad_type;            //自定义广播包(固定值0xff)
    u16 company_id;        //公司id
    u8 type;               //设备类型
    u8 last_length;        //表示余下的数据长度(固定值0x15)
    u8 uuid[16];           //蓝牙信标uuid; 应用过程中要确保uuid的唯一性
    u16 major;
    u16 minor;             //major和minor由发布者定义,是信息的主要承载部分
    u8 tx_power;           //1m处信号强度校准
} IBEACON_PARA;



typedef struct {
    u8 length;                  //长度值为0x03;(固定)
    u8 ad_type1;                //广播数据类型:0x03(表明是16位完整列表uuid) (固定)
    u16 complete_list_uuid;     //complete_list_uuid

    u8 length_last;             //剩余数据长度0x17; (固定)
    u8 ad_type2;                //广播数据类型为服务数据类型: 0x16 (固定)
    u16 eddystone_uuid;         //不知道这个是什么, 固定为0xfeaa吧(固定)
    u8 frametype;               //uid的框架类型, 为0x00; (固定)
    u8 tx_power;                //注意, 这里是0米处射频信号强度校准功率
    u8 name_space[10];          //命名空间;可以用于给信标分组
    u8 instance[6];             //可以用于区分同组信标
    u16 reserve;                //保留供将来使用，但必须为0x00
} EDDYSTONE_UID;



typedef  struct {
    u8 length;                  //长度值为0x03;(固定)
    u8 ad_type1;                //广播数据类型:0x03(表明是16位完整列表uuid) (固定)
    u16 complete_list_uuid;     //complete_list_uuid

    u8 length_last;             //剩余数据长度0x11; (固定)
    u8 ad_type2;                //广播数据类型为服务数据类型: 0x16 (固定)
    u16 eddystone_uuid;         //不知道这个是什么, 固定为0xfeaa吧(固定)
    u8 frametype;               //框架类型, 为0x20; (固定)
    u8 tlm_version;             //TLM版本
    u16 v_battle;               //电池电压,单位:1mV
    u16 temp;                   //信标温度, 单位:0.00390625℃
    u32 adv_cnt;                //开机后广告PDU运行计数
    u32 sec_cnt;                //开机后运行时间; 单位:0.1s

} EDDYSTONE_TLM;





typedef  struct {
    u8 length;                  //长度值为0x03;(固定)
    u8 ad_type1;                //广播数据类型:0x03(表明是16位完整列表uuid) (固定)
    u16 complete_list_uuid;     //complete_list_uuid

    u8 length_last;             //剩余数据长度; 通过计算得到
    u8 ad_type2;                //广播数据类型为服务数据类型: 0x16 (固定)
    u16 eddystone_uuid;         //不知道这个是什么, 固定为0xfeaa吧(固定)
    u8 frametype;               //框架类型, 为0x10; (固定)
    u8 tx_power;                //1m射频校准功率
    u8 *url_cheme[17];          //url网址; 通过转换网址得到; 转换后最长只能支持17字节
} EDDYSTONE_URL;


typedef  struct {
    u8 length;                  //长度值为0x03;(固定)
    u8 ad_type1;                 //广播数据类型:0x03(表明是16位完整列表uuid) (固定)
    u16 complete_list_uuid;     //complete_list_uuid

    u8 length_last;             //剩余数据长度;(固定0x0d)
    u8 ad_type2;                 //广播数据类型为服务数据类型: 0x16 (固定)
    u16 eddystone_uuid;          //不知道这个是什么, 固定为0xfeaa吧(固定)
    u8 frametype;               //框架类型, 为0x30; (固定)
    u8 tx_power;                //注意是0m射频校准功率
    u8 eid[8];                 //8字节加密临时标识符
} EDDYSTONE_EID;





typedef  struct {
    u8 length;                  //长度值为0x03;(固定)
    u8 ad_type1;                 //广播数据类型:0x03(表明是16位完整列表uuid) (固定)
    u16 complete_list_uuid;     //complete_list_uuid

    u8 length_last;             //剩余数据长度0x15; (固定)
    u8 ad_type2;                 //广播数据类型为服务数据类型: 0x16 (固定)
    u16 eddystone_uuid;          //不知道这个是什么, 固定为0xfeaa吧(固定)
    u8 frametype;               //框架类型, 为0x20; (固定)
    u8 tlm_version;             //TLM版本,版本固定为0x1

    u8 etml[12];                //12字节加密数据
    u16 random;                 //随机数,要与加密时用到的随机数相同
    u16 check;                  //AES-EAX计算出来的校验和
} EDDYSTONE_ETLM;

#pragma pack()




#define IBEACON_PACKET sizeof(IBEACON_PARA)
#define EDDYSTONE_UID_PACKET sizeof(EDDYSTONE_UID)
#define EDDYSTONE_TLM_PACKET sizeof(EDDYSTONE_TLM)
#define EDDYSTONE_URL_PACKET 00
#define EDDYSTONE_EID_PACKET sizeof(EDDYSTONE_EID)
#define EDDYSTONE_ETLM_PACKET sizeof(EDDYSTONE_ETLM)

u8 make_beacon_packet(u8 *buf, void *packet, u8 packet_type, u8 *web);
void get_eid(u8 *share_key, u32 cnt, u8 K, u8 *eid_out);
void get_etml(u8 *share_key, u32 cnt, u8 K, EDDYSTONE_TLM *tlm_data, u8 *etml_out);
















#endif
