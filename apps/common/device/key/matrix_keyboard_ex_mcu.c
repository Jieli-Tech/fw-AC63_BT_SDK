#include "matrix_keyboard.h"
#include "key_driver.h"
#include "gpio.h"
#include "system/event.h"
#include "app_config.h"
#include "timer.h"
#include "asm/power_interface.h"
#include "asm/power/p33.h"
#include "ex_mcu_uart.h"

//本矩阵按键函数，现主要针对bd19做了修改，别的系列未考虑

#if TCFG_MATRIX_KEY_ENABLE && TCFG_EX_MCU_ENABLE
#define MATRIX_NO_KEY           0x1         //没有按键时的IO电平
#define MATRIX_LONG_TIME        35
#define MATRIX_HOLD_TIME        35+10
#define MATRIX_KEY_FILTER_TIME  2

static volatile u8 is_key_active = 0;
u8 notify_key_array[8] = {0};
u8 key_map[COL_MAX] = {0};
static u16 key_status_array[6] = {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff};
static int matrix_key_wakeup_hold_timer = 0;
static int matrix_key_timer = 0;
static int shake_hands_timer_id = 0;//握手定时器id
static matrix_key_st key_st[ROW_MAX][COL_MAX];
static matrix_key_param *this = NULL;
extern struct ex_mcu_platform_data ex_mcu_data;
//ex_mcu变量
static u8 ex_mcu_buf[6] __attribute__((aligned(4), section(".common"))); //串口数据交互buffer
//ex_mcu通信指令
static u8 const shake_hands_ack_cmd[5] __attribute((aligned(4))) = {0x55, 0xaa, 0x20, 0x22, 0x08};//从机回应握手指令
static u8 const shake_hands_send_cmd[5] __attribute((aligned(4))) = {0x05, 0x06, 0x08, 0x05, 0x06}; //向从机握手指令
static u8 const start_code[5] __attribute((aligned(4))) = {0x05, 0x01, 0x01, 0x01, 0x05};//从机扫描开始指令
static u8 const powerdown_code[5] __attribute((aligned(4))) = {0x06, 0x01, 0x02, 0x03, 0x10};//从机powerdown指令
static u8 const poweroff_code[5] __attribute((aligned(4))) = {0x07, 0x01, 0x02, 0x03, 0x10};//不建议放在全局,有4k对其操作

//-------------------------------------------
static void ex_mcu_key_scan_start(void)
{
    ex_mcu_data.tx_buf_client(start_code, sizeof(start_code));
}

extern void p33_soft_reset(void);
static void ex_mcu_key_scan_recive(void)
{
    static u16 num = 0;
    u8 ret;
    ret = ex_mcu_data.rx_buf_client(ex_mcu_buf, 6, 10);
    if (ret == 0) {
        num++;
        if (num == 100) {
            printf("no ad15 uart data to set reset");
            p33_soft_reset();
            while (1);
        }
    } else {
        num = 0;
    }
}

void ex_mcu_enter_powerdown(void)
{
    ex_mcu_data.tx_buf_client(powerdown_code, sizeof(powerdown_code));
    /* ex_mcu_matrix_key_set_io_state(IO_STATUS_INPUT_PULL_UP, this->row_pin_list, this->row_num); */
}

void ex_mcu_exit_powerdown(u32 gpio)
{
    //退出powerdown输出下降沿唤醒从机,使用寄存器操作
    u8 port_value;
    switch (gpio / IO_GROUP_NUM) {
    case 0://JL_PORTA
        port_value = gpio % IO_GROUP_NUM;
        /* printf("%d---%d ",gpio / IO_GROUP_NUM, port_value); */
        JL_PORTA->DIR &= ~BIT(port_value);
        JL_PORTA->OUT |= BIT(port_value);
        JL_PORTA->OUT &= ~BIT(port_value);
        break;
    case 1://JL_PORTB
        port_value = gpio % IO_GROUP_NUM;
        JL_PORTB->DIR &= ~BIT(port_value);
        JL_PORTB->OUT |= BIT(port_value);
        JL_PORTB->OUT &= ~BIT(port_value);
        break;
    case 2://JL_PORTD
        port_value = gpio % IO_GROUP_NUM;
        JL_PORTD->DIR &= ~BIT(port_value);
        JL_PORTD->OUT |= BIT(port_value);
        JL_PORTD->OUT &= ~BIT(port_value);
        break;
    default:
        break;
    }
}

void ex_mcu_enter_poweroff(void)
{
    printf("To send power off cmd!!");
    ex_mcu_data.tx_buf_client(poweroff_code, sizeof(poweroff_code));
}

static void ex_mcu_full_key_array(u8 row, u8 col, u8 st)
{
    u8 offset = 0;
    u8 mark = 0, mark_offset = 0;
    //最多推6个按键出来，如果需要推多个按键需要自行修改，每个u16 低八位标识row 高八位标识col
    u16 key_value = (row | col << 8);
    for (offset = 0; offset < sizeof(key_status_array) / sizeof(u16); offset++) {
        if (key_status_array[offset] == key_value && st == MATRIX_KEY_UP) {         //找到列表中有当前按键且按键状态抬起,则将按键移除列表
            mark = 1;                                                               //记录相同键值所在位置
            mark_offset = offset;
            if (mark_offset == sizeof(key_status_array) / sizeof(u16) - 1) {
                key_status_array[mark_offset] = 0xffff;
                break;
            }
        } else if (key_status_array[offset] == 0xffff) {
            if (mark) {
                memcpy(key_status_array + mark_offset, key_status_array + mark_offset + 1, (offset - mark_offset - 1) * 2);
                key_status_array[offset - 1] = 0xffff;
            } else if (st == MATRIX_KEY_SHORT) {                                    //需要状态为短按才会把键值填充到数组中，防止按键满了之后把抬起也填充进去
                key_status_array[offset] = key_value;
            }
            break;
        } else if (mark && (offset == sizeof(key_status_array) / sizeof(u16) - 1)) {
            memcpy(key_status_array + mark_offset, key_status_array + mark_offset + 1, (sizeof(key_status_array) / sizeof(u16) - mark_offset - 1) * 2);
            key_status_array[sizeof(key_status_array) / sizeof(u16) - 1] = 0xffff;
            break;
        }
    }
}


void ex_mcu_full_key_map(u8 row, u8 col, u8 st)
{
    if (st) {
        key_map[col] |= BIT(row);
    } else {
        key_map[col] &= ~BIT(row);
    }
}

void P33_AND_WKUP_EDGE(u8 data);
void P33_OR_WKUP_EDGE(u8 data);

enum {
    IO_STATUS_HIGH_DRIVER = 0,
    IO_STATUS_OUTPUT_HIGH,
    IO_STATUS_OUTPUT_LOW,
    IO_STATUS_INPUT_PULL_DOWN,
    IO_STATUS_INPUT_PULL_UP,
};

void ex_mcu_matrix_key_set_io_state(u8 state, u32 *io_table, u8 len)
{
    u8 i = 0;
    u8 portdp_value = 0;
    u8 portdm_value = 0;
    u32 porta_value = 0;
    u32 portb_value = 0;
    u32 portd_value = 0;
    u32 portpm_value = 0;

    for (; i < len; i++) {
        switch (io_table[i] / IO_GROUP_NUM) {
        case 0: //JL_PORTA
            porta_value |= BIT(io_table[i] % IO_GROUP_NUM);
            break;
        case 1: //JL_PORTB
            portb_value |= BIT(io_table[i] % IO_GROUP_NUM);
            break;
        case 2: //JL_PORTD
            portd_value |= BIT(io_table[i] % IO_GROUP_NUM);
            break;
        case 3: //DP1/DM1
            portpm_value = io_table[i] % IO_GROUP_NUM;
            if (portpm_value == 3) {
                portdp_value = 1;
            } else if (portpm_value == 4) {
                portdm_value = 1;
            }
            break;
        default:
            break;
        }
    }
    switch (state) {
    case IO_STATUS_HIGH_DRIVER:
        JL_PORTA->DIR |= porta_value;
        JL_PORTA->DIE &= ~porta_value;
        JL_PORTA->PU &= ~porta_value;
        JL_PORTA->PD &= ~porta_value;
        JL_PORTB->DIR |= portb_value;
        JL_PORTB->DIE &= ~portb_value;
        JL_PORTB->PU &= ~portb_value;
        JL_PORTB->PD &= ~portb_value;
        JL_PORTD->DIR |= portd_value;
        JL_PORTD->DIE &= ~portd_value;
        JL_PORTD->PU &= ~portd_value;
        JL_PORTD->PD &= ~portd_value;
        //dp1 dm1操作
        if (portdp_value == 1) {
            JL_USB1->CON0 &= ~BIT(0);//USB_PHY_OFF 来自usb1_iomode(1);
            JL_USB1_IO->CON0 |= BIT(12) | BIT(14);
            JL_USB1_IO->CON0 &= ~BIT(4);//关闭DP上拉
            JL_USB1_IO->CON0 &= ~BIT(6);//关闭DP下拉
            JL_USB1_IO->CON0 |= BIT(2);//DP设置为输入
            JL_USB1_IO->CON0 &= ~BIT(8);//1.2v DP数字引脚
            JL_USB1_IO->CON0 &= ~BIT(10);//3.3v DP数字引脚
        }
        if (portdm_value == 1) {
            JL_USB1->CON0 &= ~BIT(0);//USB_PHY_OFF 来自usb1_iomode(1);
            JL_USB1_IO->CON0 |= BIT(12) | BIT(14);
            JL_USB1_IO->CON0 &= ~BIT(5);//关闭DM上拉
            JL_USB1_IO->CON0 &= ~BIT(7);//关闭Dm下拉
            JL_USB1_IO->CON0 |= BIT(3);//Dm设置为输入
            JL_USB1_IO->CON0 &= ~BIT(9);//1.2v DM数字引脚
            JL_USB1_IO->CON0 &= ~BIT(11);//3.3v DM数字引脚
        }
        break;
    case IO_STATUS_OUTPUT_HIGH:
        JL_PORTA->DIR &= ~porta_value;
        JL_PORTA->OUT |= porta_value;
        JL_PORTB->DIR &= ~portb_value;
        JL_PORTB->OUT |= portb_value;
        JL_PORTD->DIR &= ~portd_value;
        JL_PORTD->OUT |= portd_value;
        //dp1 dm1操作
        if (portdp_value == 1) {
            JL_USB1->CON0 &= ~BIT(0);//USB_PHY_OFF 来自usb1_iomode(1);
            JL_USB1_IO->CON0 |= BIT(12) | BIT(14);
            JL_USB1_IO->CON0 &= ~BIT(2);//DP设置为输出
            JL_USB1_IO->CON0 |= BIT(0);//DP输出高电平
        }
        if (portdm_value == 1) {
            JL_USB1->CON0 &= ~BIT(0);//USB_PHY_OFF 来自usb1_iomode(1);
            JL_USB1_IO->CON0 |= BIT(12) | BIT(14);
            JL_USB1_IO->CON0 &= ~BIT(3);//DM设置为输出
            JL_USB1_IO->CON0 |= BIT(1);//DM输出高电平
        }
        break;
    case IO_STATUS_OUTPUT_LOW:
        JL_PORTA->DIR &= ~porta_value;
        JL_PORTA->OUT &= ~porta_value;
        JL_PORTB->DIR &= ~portb_value;
        JL_PORTB->OUT &= ~portb_value;
        JL_PORTD->DIR &= ~portd_value;
        JL_PORTD->OUT &= ~portd_value;
        //dp1 dm1操作
        if (portdp_value == 1) {
            JL_USB1->CON0 &= ~BIT(0);//USB_PHY_OFF 来自usb1_iomode(1);
            JL_USB1_IO->CON0 |= BIT(12) | BIT(14);
            JL_USB1_IO->CON0 &= ~BIT(2);//DP设置为输出
            JL_USB1_IO->CON0 &= ~BIT(0);//DP输出低电平
        }
        if (portdm_value == 1) {
            JL_USB1->CON0 &= ~BIT(0);//USB_PHY_OFF 来自usb1_iomode(1);
            JL_USB1_IO->CON0 |= BIT(12) | BIT(14);
            JL_USB1_IO->CON0 &= ~BIT(3);//DM设置为输出
            JL_USB1_IO->CON0 &= ~BIT(1);//DM输出低电平
        }
        break;
    case IO_STATUS_INPUT_PULL_UP:
        JL_PORTA->DIR |= porta_value;
        JL_PORTA->DIE &= ~porta_value;
        JL_PORTA->DIEH |= porta_value;
        JL_PORTA->PU  |= porta_value;
        JL_PORTA->PD  &= ~porta_value;
        JL_PORTB->DIR |= portb_value;
        JL_PORTB->DIE &= ~portb_value;
        JL_PORTB->DIEH |= portb_value;
        JL_PORTB->PU  |= portb_value;
        JL_PORTB->PD  &= ~portb_value;
        JL_PORTD->DIR |= portd_value;
        JL_PORTD->DIE &= ~portd_value;
        JL_PORTD->DIEH |= portd_value;
        JL_PORTD->PU  |= portd_value;
        JL_PORTD->PD  &= ~portd_value;
        //dp1 dm1操作
        if (portdp_value == 1) {
            JL_USB1->CON0 &= ~BIT(0);//USB_PHY_OFF 来自usb1_iomode(1);
            JL_USB1_IO->CON0 |= BIT(12) | BIT(14);
            JL_USB1_IO->CON0 |= BIT(4);//开DP上拉
            JL_USB1_IO->CON0 &= ~BIT(6);//关闭DP下拉
            JL_USB1_IO->CON0 |= BIT(2);//DP设置为输入
            JL_USB1_IO->CON0 &= ~BIT(8);//1.2v DP数字引脚
            JL_USB1_IO->CON0 |= BIT(10);//3.3v DP数字引脚使能
        }
        if (portdm_value == 1) {
            JL_USB1->CON0 &= ~BIT(0);//USB_PHY_OFF 来自usb1_iomode(1);
            JL_USB1_IO->CON0 |= BIT(12) | BIT(14);
            JL_USB1_IO->CON0 |= BIT(5);//打开DM上拉
            JL_USB1_IO->CON0 &= ~BIT(7);//关闭Dm下拉
            JL_USB1_IO->CON0 |= BIT(3);//Dm设置为输入
            JL_USB1_IO->CON0 &= ~BIT(9);//1.2v DM数字引脚
            JL_USB1_IO->CON0 |= BIT(11);//3.3v DM数字引脚使能
        }
        break;
    }
}

static u32 key_scan_value_analyse()
{
    static u16 count = 0;
    u32 key_scan_value = 0;
    key_scan_value = (ex_mcu_buf[2] << 16) | (ex_mcu_buf[3] << 8) | (ex_mcu_buf[4]);
    if (key_scan_value && (ex_mcu_buf[0] == 0x05) && (ex_mcu_buf[5] == 0x06)) {
        /* put_buf(ex_mcu_buf, sizeof(ex_mcu_buf)); */
        /* putchar('%'); */
        count = 0;
        return key_scan_value;
    }
    return 0;
}

static void ex_mcu_port_edge_wakeup_control(u16 data)
{

    if (data) {
        P3_WKUP_CPND0 = data;
        P3_WKUP_CPND1 = data >> 8;
    }

    P3_WKUP_EN0 = data;
    P3_WKUP_EN1 = data >> 8;
}

void ex_mcu_matrix_key_scan(void)
{
    u8 row, col, cur_col;
    u32 tmp;
    u8 cur_key_value = MATRIX_NO_KEY, notify = 0;
    static u8 wk_toggle = 0;
    //g_printf("scan...\n");
    //P33_LEVEL_WKUP_EN(0);
    extern void P33_DUMP_WKUP(void);
    extern void P33_PORT_DUMP(void);

    __asm__ volatile("%0 =icfg" : "=r"(tmp));
    if ((tmp & BIT(9)) == 0) {
        /* putchar('@'); */
        return;
    }

    /* putchar('%'); */
    ex_mcu_port_edge_wakeup_control(0);
    //不直接调用gpio.c的接口主要是由于想控制扫描时间。这样能把时间从1.8ms缩减到800us
    ex_mcu_matrix_key_set_io_state(IO_STATUS_HIGH_DRIVER, this->row_pin_list, this->row_num);
    is_key_active = 0;

    for (row = 0; row < this->row_num; row ++) {
        ex_mcu_key_scan_start();
        ex_mcu_matrix_key_set_io_state(IO_STATUS_OUTPUT_LOW, &(this->row_pin_list[row]), 1);
        ex_mcu_key_scan_recive();
        //gpio_direction_output(this->col_pin_list[col], !MATRIX_NO_KEY);
        for (col = 0; col < this->col_num; col ++) {
            cur_key_value = !(((key_scan_value_analyse()) >> col)&BIT(0));
            if (cur_key_value != MATRIX_NO_KEY) {
                is_key_active = 1;
            }
            if (cur_key_value != (key_st[row][col]).last_st) {
                if (cur_key_value == MATRIX_NO_KEY) { //按键抬起判断
                    (key_st[row][col]).press_cnt = 0;
                    printf("row:%d  col:%d   [UP]\n", row, col);
                    (MATRIX_NO_KEY) ? P33_OR_WKUP_EDGE(row) : P33_AND_WKUP_EDGE(row);//在抬起(上升沿/高电平)之后避免P33识别错误,将P33设置为下降沿/低电平唤醒
                    //P33_AND_WKUP_EDGE(row);
                    ex_mcu_full_key_map(row, col, 0);
                    notify = 1;
                } else {            //按键初次按下
                    printf("row:%d  col:%d   [SHORT]\n", row, col);
                    ex_mcu_full_key_map(row, col, 1);
                    (MATRIX_NO_KEY) ? P33_AND_WKUP_EDGE(row) : P33_OR_WKUP_EDGE(row);//在抬起下降沿/低电平之后避免P33识别错误,将P33设置为(上升沿/高电平)唤醒
                    //P33_OR_WKUP_EDGE(row);
                    notify = 1;
                }
            } else {        //判断是否按键抬起
                if (cur_key_value != MATRIX_NO_KEY) {
                    (key_st[row][col]).press_cnt ++;
                    if ((key_st[row][col]).press_cnt == MATRIX_LONG_TIME) {
                        printf("row:%d  col:%d   [LONG]\n", row, col);
                        notify = 1;
                    } else if ((key_st[row][col]).press_cnt == MATRIX_HOLD_TIME) {
                        printf("row:%d  col:%d   [HOLD]\n", row, col);
                        notify = 1;
                        (key_st[row][col]).press_cnt = MATRIX_LONG_TIME;
                    } else {
                        //press_cnt还未累加够
                    }
                }
            }
            (key_st[row][col]).last_st = cur_key_value;
        }
        /* gpio_direction_input(this->col_pin_list[col]); */
        ex_mcu_matrix_key_set_io_state(IO_STATUS_HIGH_DRIVER, this->row_pin_list, this->row_num);
    }

    ex_mcu_matrix_key_set_io_state(IO_STATUS_INPUT_PULL_UP, this->row_pin_list, this->row_num);

    ex_mcu_port_edge_wakeup_control(0xff);

    if (notify) {
        struct sys_event e;
        e.type = SYS_MATRIX_KEY_EVENT;
        e.u.matrix_key.map = key_map;
        e.arg  = (void *)DEVICE_EVENT_FROM_KEY;
        sys_event_notify(&e);
    }
    /* extern void P33_PORT_DUMP(void); */
    /* P33_PORT_DUMP();  */
    /* return 1; */
}

void ex_mcu_matrix_key_wakeup_timeout_handler(void *arg)
{
    is_key_active = 0;
}

void ex_mcu_matrix_key_wakeup_keep(void)
{
    is_key_active = 1;
    if (matrix_key_wakeup_hold_timer) {
        sys_s_hi_timeout_del(matrix_key_wakeup_hold_timer);
    }
    matrix_key_wakeup_hold_timer = sys_s_hi_timerout_add(NULL, ex_mcu_matrix_key_wakeup_timeout_handler, 5);
}

void ex_mcu_matrix_key_wakeup(u8 idx, u32 gpio)
{
    //先唤醒ex_mcu设备
    ex_mcu_exit_powerdown(TCFG_EX_WAKEUP_PORT);
    /* r_printf("matrix_key wkup!\n"); */
    ex_mcu_matrix_key_wakeup_keep();
}

static void key_scan_shake_hands()
{
    u32 ret = 0;
    u8 shake_hands_flag = 0;
    u8 *__rx_buf;

    ex_mcu_data.tx_buf_client(shake_hands_send_cmd, sizeof(shake_hands_send_cmd));
    ex_mcu_data.rx_buf_client(ex_mcu_buf, sizeof(shake_hands_ack_cmd), 20);
    ret = sizeof(ex_mcu_buf) - 1;
    //收够足够长度内容进行打印
    printf("ret ======= %d", ret);
    put_buf(ex_mcu_buf, sizeof(shake_hands_ack_cmd));
    //接收到的长度为ack_cmd的长度
    if (ret == sizeof(shake_hands_ack_cmd)) {
        printf("ack info:\n");
        __rx_buf = ex_mcu_buf;
        for (u8 i = 0; i < sizeof(shake_hands_ack_cmd); i++) {
            if (__rx_buf[i] != shake_hands_ack_cmd[i]) {
                shake_hands_flag = 0;
                printf("ack info err\n");
                break;
            } else {
                shake_hands_flag = 1;
                printf("shake succ");
            }
        }
    } else {
        shake_hands_flag = 0;
        printf("ack info err\n");
    }

    if (matrix_key_timer) {
        sys_s_hi_timer_del(matrix_key_timer);
    }

    if (shake_hands_flag == 1 && shake_hands_timer_id) {
        sys_timer_del(shake_hands_timer_id);
        matrix_key_timer = sys_s_hi_timer_add(NULL, ex_mcu_matrix_key_scan, 5);
    }
}

int ex_mcu_matrix_key_init(matrix_key_param *param)
{
    if (!param) {
        return -1;      //参数无效
    }
    this = param;
    u8 row, col;
    for (row = 0; row < this->row_num; row ++) {
        (MATRIX_NO_KEY) ? P33_OR_WKUP_EDGE(row) : P33_AND_WKUP_EDGE(row);
        for (col = 0; col < this->col_num; col ++) {
            (key_st[row][col]).last_st = MATRIX_NO_KEY;
            (key_st[row][col]).press_cnt = 0;
        }
    }

    if (shake_hands_timer_id) {
        sys_timer_del(shake_hands_timer_id);
    }

    port_edge_wkup_set_callback(ex_mcu_matrix_key_wakeup);
    shake_hands_timer_id = sys_timer_add(NULL, key_scan_shake_hands, 20);
    return 0;
}

static u8 matrix_key_idle_query(void)
{
    return !is_key_active;
}

REGISTER_LP_TARGET(matrix_key_lp_target) = {
    .name = "matrix_key",
    .is_idle = matrix_key_idle_query,
};

#endif
