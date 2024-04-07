
/*************************************************************

      此文件函数主要是蓝牙发射器接口处理

**************************************************************/

#include "system/app_core.h"
#include "system/includes.h"

#include "app_config.h"
#include "app_task.h"

#include "btstack/btstack_task.h"
#include "btcontroller_modules.h"
#include "btstack/avctp_user.h"
#include "classic/hci_lmp.h"
#include "user_cfg.h"
#include "vm.h"
#include "app_main.h"
#include "key_event_deal.h"
#include "edr_emitter.h"
/* #include "rcsp_bluetooth.h" */

#if (EDR_EMITTER_EN && TCFG_USER_EDR_ENABLE)


#define LOG_TAG_CONST        EDR_EM
#define LOG_TAG             "[EDR_EM]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
#define LOG_DUMP_ENABLE
#define LOG_CLI_ENABLE
#include "debug.h"

/* #undef  log_info */
/* #define log_info y_printf */

#define  SEARCH_BD_ADDR_LIMITED    0 //匹配mac地址搜索
#define  SEARCH_BD_NAME_LIMITED    1 //匹配名字搜索
#define  SEARCH_CUSTOM_LIMITED     2 //搜索结果自定义处理
#define  SEARCH_NULL_LIMITED       3 //搜索到设备就直接连接，不限制

//搜索方式配置
#define SEARCH_LIMITED_MODE        SEARCH_BD_NAME_LIMITED

#define SEARCH_NAME_DEBUG          0

struct list_head inquiry_noname_list;
struct inquiry_noname_remote {
    struct list_head entry;
    u8 match;
    s8 rssi;
    u8 addr[6];
    u32 class;
};

typedef struct  {
    u8 emitter_mode: 2;
    u8 read_name_start: 1;
    u8 bt_search_busy: 1;
    u8 bt_connect_start: 1;
    u8 bt_emitter_start: 1;
    u8 res_bits: 2;
} bt_user_var_t;

static bt_user_var_t bt_user_private_var;
#define  __this  (&bt_user_private_var)

#if (SEARCH_LIMITED_MODE == SEARCH_BD_ADDR_LIMITED)
static const u8 bd_addr_filt[][6] = {
    {0x38, 0x7C, 0x78, 0x1C, 0xFC, 0x02}, /*Bluetooth*/
};
#endif

#if (SEARCH_LIMITED_MODE == SEARCH_BD_NAME_LIMITED)
/* static const u8 bd_name_filt[][30] = { */
/* "BlueTooth_Keyboard  3.0", */
/* "AC630N_mx", */
/* }; */

static const *bd_name_filt;
static u8 bd_name_filt_nums;

#endif



extern u16 get_emitter_curr_channel_state();

//===============================================================================
//define to Device ID SPEC
#define BT_SDP_DID_VendorID                 0x0201
#define BT_SDP_DID_ProductID                0x0202
#define BT_SDP_DID_Version                  0x0203
#define BT_SDP_DID_VendorIDSource           0x0205

//define to hid server
#define BT_SDP_HID_DescriptorList           0x0206

#define BT_PNP_INFO_ID         0x1200
#define BT_UUID_HID_DE         0x1124

/*----------------------------------------------------------------------------*/
/**@brief
   @param    无
   @return   无
   @note    //协议栈回调输出
*/
/*----------------------------------------------------------------------------*/
void sdp_decode_response_info_output(u16 service_uuid, u16 attribute_id, const u8 *packet, int size)
{
    log_info("sdp_output:service_uuid= %04x,attribute_id= %04x,size= %d", service_uuid, attribute_id, size);
    /* put_buf(packet,size); */

    switch (service_uuid) {
    case BT_PNP_INFO_ID: {
        switch (attribute_id) {
        case BT_SDP_DID_VendorID:
        case BT_SDP_DID_ProductID:
        case BT_SDP_DID_Version:
        case BT_SDP_DID_VendorIDSource:
            log_info("PNP_INFO,data_type= %d:", packet[0]);
            put_buf(packet + 1, size - 1);
            break;
        }
    }
    break;
        /*
            case BT_UUID_HID_DE:
                if (BT_SDP_HID_DescriptorList == attribute_id) {
                    log_info("REPORT_MAP:");
                    put_buf(packet, size);
                }
                break;
        */
    }
}

//pin code 轮询功能
static const char pin_code_list[10][4] = {
    {'0', '0', '0', '0'},
    {'1', '2', '3', '4'},
    {'8', '8', '8', '8'},
    {'1', '3', '1', '4'},
    {'4', '3', '2', '1'},
    {'1', '1', '1', '1'},
    {'2', '2', '2', '2'},
    {'3', '3', '3', '3'},
    {'5', '6', '7', '8'},
    {'5', '5', '5', '5'}
};

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙发射链接pincode 轮询
  @param    无
  @return   无
  @note
 */
/*----------------------------------------------------------------------------*/
const char *bt_get_emitter_pin_code(u8 flag)
{
    static u8 index_flag = 0;
    int pincode_num = sizeof(pin_code_list) / sizeof(pin_code_list[0]);
    if (flag == 1) {
        //reset index
        index_flag = 0;
    } else if (flag == 2) {
        //查询是否要开始继续回连尝试pin code。
        if (index_flag >= pincode_num) {
            //之前已经遍历完了
            return NULL;
        } else {
            index_flag++; //准备使用下一个
        }
    } else {
        log_debug("get pin code index %d\n", index_flag);
    }
    return &pin_code_list[index_flag][0];
}

/*----------------------------------------------------------------------------*/
/**@brief
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static void __bt_search_device(void)
{
    if (__this->bt_search_busy) {
        log_info("bt_search_busy >>>>>>>>>>>>>>>>>>>>>>>\n");
        return;
    }

    user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);

    log_info("bt_search_start >>>>>>>>>\n");
    __this->read_name_start = 0;
    __this->bt_search_busy = 1;
    u8 inquiry_length = 20;   // inquiry_length * 1.28s
    user_send_cmd_prepare(USER_CTRL_SEARCH_DEVICE, 1, &inquiry_length);

#if EDR_EMITTER_EN && USER_SUPPORT_PROFILE_SPP
    //spp 主机
    set_start_search_spp_device(1);
#endif

}


/*----------------------------------------------------------------------------*/
/**@brief
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
u8 bt_emitter_get_search_status()
{
    return __this->bt_search_busy;
}

/*----------------------------------------------------------------------------*/
/**@brief
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_emitter_stop_search_device(void)
{
    if (__this->bt_search_busy) {
        __this->bt_search_busy = 0;
        log_info("%s\n", __FUNCTION__);
        user_send_cmd_prepare(USER_CTRL_INQUIRY_CANCEL, 0, NULL);
    }
}

/*----------------------------------------------------------------------------*/
/**@brief
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_emitter_start_search_device(void)
{
    if (__this->emitter_mode != BT_EMITTER_EN || __this->bt_search_busy) {
        return;
    }

    log_info("%s\n", __FUNCTION__);
    while (hci_standard_connect_check() == 0x80) {
        //wait profile connect ok;
        if (get_curr_channel_state()) {
            break;
        }
        os_time_dly(10);
    }

    ////断开链接
    if (get_curr_channel_state() != 0) {
        user_send_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
    } else {
        if (hci_standard_connect_check()) {
            user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
            user_send_cmd_prepare(USER_CTRL_CONNECTION_CANCEL, 0, NULL);
        }
    }
    /* if there are some connected channel ,then disconnect*/
    while (hci_standard_connect_check() != 0) {
        //wait disconnect;
        os_time_dly(10);
    }

    ////关闭可发现可链接
    user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
    user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
    ////切换样机状态
    __bt_search_device();

}

/*----------------------------------------------------------------------------*/
/**@brief
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
bool bt_emitter_connect(u8 *mac)
{
    if (__this->emitter_mode != BT_EMITTER_EN) {
        return false;
    }

    while (hci_standard_connect_check() == 0x80) {
        //wait profile connect ok;
        if (get_curr_channel_state()) {
            break;
        }
        os_time_dly(10);
    }

    ////断开链接
    if (get_curr_channel_state() != 0) {
        user_send_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
    } else {
        if (hci_standard_connect_check()) {
            user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
            user_send_cmd_prepare(USER_CTRL_CONNECTION_CANCEL, 0, NULL);
        }
    }
    /* if there are some connected channel ,then disconnect*/
    while (hci_standard_connect_check() != 0) {
        //wait disconnect;
        os_time_dly(10);
    }

    user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, mac);
    return true;
}

/*----------------------------------------------------------------------------*/
/**@brief
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
bool bt_emitter_connect_last_vm_device(void)
{
    if (connect_last_device_from_vm()) {
        log_info("start connect device vm addr\n");
        return true;
    }
    return false;
}

/*----------------------------------------------------------------------------*/
/**@brief
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_emitter_role_set(u8 flag)
{
    /*如果上一次操作记录跟传进来的参数一致，则不操作*/
    if (__this->emitter_mode == flag) {
        return ;
    }

    while (hci_standard_connect_check() == 0x80) {
        //wait profile connect ok;
        if (__this->emitter_mode == BT_EMITTER_EN) {   ///蓝牙发射器
            log_info("cur_ch:0x%x", get_emitter_curr_channel_state());
            if (get_emitter_curr_channel_state()) {
                break;
            }
        } else {
            log_info("cur_ch:0x%x", get_curr_channel_state());
            if (get_curr_channel_state()) {
                break;
            }
        }
        os_time_dly(10);
    }

    ////断开链接
    if ((get_curr_channel_state() != 0) || (get_emitter_curr_channel_state() != 0)) {
        user_send_cmd_prepare(USER_CTRL_POWER_OFF, 0, NULL);
    } else {
        if (hci_standard_connect_check()) {
            user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
            user_send_cmd_prepare(USER_CTRL_CONNECTION_CANCEL, 0, NULL);
        }
    }
    /* if there are some connected channel ,then disconnect*/
    while (hci_standard_connect_check() != 0) {
        //wait disconnect;
        os_time_dly(10);
    }

    __this->emitter_mode = flag;

    if (flag == BT_EMITTER_EN) {   ///蓝牙发射器
        bredr_bulk_change(0);
        ////关闭可发现可链接
        user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
        user_send_cmd_prepare(USER_CTRL_WRITE_CONN_DISABLE, 0, NULL);
        ////切换样机状态
        __set_emitter_enable_flag(1);

        ////开启搜索设备
        if (0) { // (bt_emitter_connect_last_vm_device()) {
            log_info("start connect device vm addr\n");
        } else {
            __bt_search_device();
        }
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙变量初始化
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_emitter_init(void)
{
    log_info("%s\n", __FUNCTION__);
    memset(__this, 0, sizeof(bt_user_var_t));
    __this->bt_emitter_start = 1;
    INIT_LIST_HEAD(&inquiry_noname_list);
    lmp_set_sniff_establish_by_remote(1);
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙配置搜索name
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/

#if (SEARCH_LIMITED_MODE == SEARCH_BD_NAME_LIMITED)
void bt_emitter_set_match_name(const char *name_table, u8 name_count)
{
    log_info("%s,bd_name_filt_nums= %d\n", __FUNCTION__, name_count);
    bd_name_filt = name_table;
    bd_name_filt_nums = name_count;
}
#endif

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙搜索设备没有名字的设备，放进需要获取名字链表
   @param    status : !=0 获取成功     0：获取失败
  			 addr:设备地址
		     name：设备名字
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_emitter_search_noname(u8 status, u8 *addr, u8 *name)
{
    struct  inquiry_noname_remote *remote, *n;
    if (!__this->bt_emitter_start) {
        return ;
    }

    if (status) {
        log_info("remote_name fail!!!\n");
    } else {
        log_info("remote_name: %s\n", name);
    }

    u8 res = 0;
    local_irq_disable();
    if (status) {
        list_for_each_entry_safe(remote, n, &inquiry_noname_list, entry) {
            if (!memcmp(addr, remote->addr, 6)) {
                list_del(&remote->entry);
                free(remote);
            }
        }
        goto __find_next;
    }
    list_for_each_entry_safe(remote, n, &inquiry_noname_list, entry) {
        if (!memcmp(addr, remote->addr, 6)) {
            res = bt_emitter_search_result(name, strlen(name), addr, remote->class, remote->rssi);
            if (res) {
                log_info("search_match to connect");
                __this->read_name_start = 0;
                remote->match = 1;
                __this->bt_connect_start = 0;
                user_send_cmd_prepare(USER_CTRL_INQUIRY_CANCEL, 0, NULL);
                local_irq_enable();
                return;
            }
            list_del(&remote->entry);
            free(remote);
        }
    }

__find_next:

    __this->read_name_start = 0;
    remote = NULL;
    if (!list_empty(&inquiry_noname_list)) {
        remote =  list_first_entry(&inquiry_noname_list, struct inquiry_noname_remote, entry);
    }

    local_irq_enable();
    if (remote) {
        __this->read_name_start = 1;
        user_send_cmd_prepare(USER_CTRL_READ_REMOTE_NAME, 6, remote->addr);
    } else {
        log_info("send cmd for restart to search");
        user_send_cmd_prepare(USER_CTRL_INQUIRY_CANCEL, 0, NULL);
    }
}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙搜索时间结束
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
void bt_emitter_search_complete(u8 result)
{
    struct  inquiry_noname_remote *remote, *n;
    __this->bt_search_busy = 0;
    set_start_search_spp_device(0);
    u8 wait_connect_flag = 1;

    log_info("%s,%d\n", __FUNCTION__, __LINE__);
    if (__this->bt_connect_start) {
        __this->bt_connect_start = 0;
        log_info("connecting...");
        return;
    }

    if (!list_empty(&inquiry_noname_list)) {
        user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
    }

    if (!result) {
        list_for_each_entry_safe(remote, n, &inquiry_noname_list, entry) {
            if (remote->match) {

#if EDR_EMITTER_EN && USER_SUPPORT_PROFILE_SPP
                user_send_cmd_prepare(USER_CTRL_START_CONNEC_SPP_VIA_ADDR, 6, remote->addr);
#else
                user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR, 6, remote->addr);
#endif
                wait_connect_flag = 0;
            }
            list_del(&remote->entry);
            free(remote);
        }
    }

    __this->read_name_start = 0;
    if (wait_connect_flag) {
        /* log_info("wait conenct\n"); */
        user_send_cmd_prepare(USER_CTRL_WRITE_SCAN_DISABLE, 0, NULL);
        if (!result) {
            user_send_cmd_prepare(USER_CTRL_WRITE_CONN_ENABLE, 0, NULL);
        }
        user_send_cmd_prepare(USER_CTRL_PAGE_CANCEL, 0, NULL);
        __bt_search_device();
        /* #endif */
    }
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙搜索通过地址过滤
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static u8 __search_bd_addr_filt(u8 *addr)
{
#if (SEARCH_LIMITED_MODE == SEARCH_BD_ADDR_LIMITED)
    u8 i;
    log_info("bd_addr:");
    log_info_hexdump(addr, 6);
    for (i = 0; i < (sizeof(bd_addr_filt) / sizeof(bd_addr_filt[0])); i++) {
        if (memcmp(addr, bd_addr_filt[i], 6) == 0) {
            log_info("bd_addr match:%d\n", i);
            return TRUE;
        }
    }
#endif
    return FALSE;
}

/*----------------------------------------------------------------------------*/
/**@brief    蓝牙搜索通过名字过滤
   @param    无
   @return   无
   @note
*/
/*----------------------------------------------------------------------------*/
static u8 __search_bd_name_filt(char *data, u8 len, u32 dev_class, char rssi)
{

#if (SEARCH_LIMITED_MODE == SEARCH_BD_NAME_LIMITED)
    char bd_name[64] = {0};
    u8 i;
    char *targe_name = NULL;
    char char_a = 0, char_b = 0;

    if ((len > (sizeof(bd_name))) || (len == 0)) {
        //printf("bd_name_len error:%d\n", len);
        return FALSE;
    }

    memset(bd_name, 0, sizeof(bd_name));
    memcpy(bd_name, data, len);
    log_info("name:%s,len:%d,class %x ,rssi %d\n", bd_name, len, dev_class, rssi);

#if SEARCH_NAME_DEBUG
    extern char *get_edr_name(void);
    printf("tar name:%s,len:%d\n", get_edr_name(), strlen(get_edr_name()));
    targe_name = (char *)get_edr_name();

#if  1
//不区分大小写
    for (i = 0; i < len; i++) {
        char_a = bd_name[i];
        char_b = targe_name[i];
        if ('A' <= char_a && char_a <= 'Z') {
            char_a += 32;    //转换成小写
        }
        if ('A' <= char_b && char_b <= 'Z') {
            char_b += 32;    //转换成小写
        }
        //printf("{%d-%d}",char_a,char_b);
        if (char_a != char_b) {
            return FALSE;
        }
    }
    log_info("\n*****find dev ok******\n");
    return TRUE;
#else
//区分大小写
    if (memcmp(data, bt_get_emitter_connect_name(), len) == 0) {
        log_info("\n*****find dev ok******\n");
        return TRUE;
    }
    return FALSE;
#endif

#else
    /* for (i = 0; i < (sizeof(bd_name_filt) / sizeof(bd_name_filt[0])); i++) { */
    for (i = 0; i < bd_name_filt_nums; i++) {
        if (memcmp(data, bd_name_filt[i], len) == 0) {
            puts("\n*****find dev ok******\n");
            return TRUE;
        }
    }
    return FALSE;
#endif

#else
    return FALSE;
#endif

}


/*----------------------------------------------------------------------------*/
/**@brief    蓝牙搜索结果回调处理
   @param    name : 设备名字
			 name_len: 设备名字长度
			 addr:   设备地址
			 dev_class: 设备类型
			 rssi:   设备信号强度
   @return   无
   @note
 			蓝牙设备搜索结果，可以做名字/地址过滤，也可以保存搜到的所有设备
 			在选择一个进行连接，获取其他你想要的操作。
 			返回TRUE，表示搜到指定的想要的设备，搜索结束，直接连接当前设备
 			返回FALSE，则继续搜索，直到搜索完成或者超时
*/
/*----------------------------------------------------------------------------*/
u8 bt_emitter_search_result(char *name, u8 name_len, u8 *addr, u32 dev_class, char rssi)
{
    u8 ret = FALSE;
    /* put_buf(addr,6); */

#if (SEARCH_LIMITED_MODE == SEARCH_BD_NAME_LIMITED)
    if (name == NULL) {
        struct inquiry_noname_remote *remote = malloc(sizeof(struct inquiry_noname_remote));
        log_info("search no_name:");
        put_buf(addr, 6);
        remote->match  = 0;
        remote->class = dev_class;
        remote->rssi = rssi;
        memcpy(remote->addr, addr, 6);
        local_irq_disable();
        list_add_tail(&remote->entry, &inquiry_noname_list);
        local_irq_enable();
        if (__this->read_name_start == 0) {
            __this->read_name_start = 1;
            user_send_cmd_prepare(USER_CTRL_READ_REMOTE_NAME, 6, addr);
        }
    }
#endif
    /* #if (RCSP_BTMATE_EN) */
    /*     rcsp_msg_post(RCSP_MSG_BT_SCAN, 5, dev_class, addr, rssi, name, name_len); */
    /* #endif */

#if (SEARCH_LIMITED_MODE == SEARCH_BD_NAME_LIMITED)
    ret = __search_bd_name_filt(name, name_len, dev_class, rssi);
#endif

#if (SEARCH_LIMITED_MODE == SEARCH_BD_ADDR_LIMITED)
    ret = __search_bd_addr_filt(addr);
#endif

#if (SEARCH_LIMITED_MODE == SEARCH_CUSTOM_LIMITED)
    /*以下为搜索结果自定义处理*/
    char bt_name[63] = {0};
    u8 len;
    if (name_len == 0) {
        log_info("No_eir\n");
    } else {
        len = (name_len > 63) ? 63 : name_len;
        /* display bd_name */
        memcpy(bt_name, name, len);
        log_info("name:%s,len:%d,class %x ,rssi %d\n", bt_name, name_len, dev_class, rssi);
    }

    /* display bd_addr */
    log_info_hexdump(addr, 6);

    /* You can connect the specified bd_addr by below api      */
    //user_send_cmd_prepare(USER_CTRL_START_CONNEC_VIA_ADDR,6,addr);
    ret = FALSE;

#endif

#if (SEARCH_LIMITED_MODE == SEARCH_NULL_LIMITED)
    /*没有指定限制，则搜到什么就连接什么*/
    ret = TRUE;
#endif

    if (ret) {
        log_info("get device,start to connect\n");
        __this->bt_connect_start = 1;
    }
    return ret;
}



#endif //#if (EDR_EMITTER_EN && TCFG_USER_EDR_ENABLE)

