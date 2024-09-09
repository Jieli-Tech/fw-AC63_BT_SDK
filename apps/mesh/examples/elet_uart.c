#include "elet_uart.h"
#include "elet_nv_cfg.h"
#include "buf_io_uart_tx.h"

#include "asm/uart_dev.h"

#include "board_config.h"
#include "system/includes.h"
#include "asm/power_interface.h"
#include "app_config.h"
#include "board.h"

#undef _DEBUG_H_
#define LOG_TAG_CONST       ELET_UART
#define LOG_TAG             "[ELET_UART]"
#include "debug.h"
const char log_tag_const_v_ELET_UART AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_ELET_UART AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_d_ELET_UART AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_ELET_UART AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_ELET_UART AT(.LOG_TAG_CONST) = 1;


//RX TX可以配置任意IO口
#define UART_TR_TX_PIN                      PIN16_UARTA_TXD
#define UART_TR_RX_PIN                      PIN17_UARTA_RXD
#define UART_TR_BAUD                        115200
//低功耗串口,及自动进入低功耗时间设置
#define UART_TR_LOWPOWER_MODE               false // true //
#define UART_WAKEUP_DELAY                   -1 // 100  //unit:2ms
//模式：单字节还是DMA
#define UART_BYTE_ONE                       0
#define UART_BYTE_DMA                       1
#define UART_BYTE_MODE                      UART_BYTE_DMA
//DMA模式参数
#define UART_DMA_TIMEOUT                    5//6//6   //unit:ms
#define UART_DMA_MAX_LEN                    0x100 //unit:Byte //单包最大长度
//单字节模式：才能开启校验位，奇校验还是偶校验
#define UART_PARITY_CHECK_NO                0//不用校验
#define UART_PARITY_CHECK_EVEN              1//奇
#define UART_PARITY_CHECK_ODD               2//偶
#define UART_PARITY_CHECK_MARK              3//校验位始终为1
#define UART_PARITY_CHECK_SPACE             4//校验位始终为0
#define UART_PARITY_CHECK_MODE              UART_PARITY_CHECK_NO
//串口流控功能，固定IO
#define UART_RTS_CTS_MODE                   true//false //true //
#define UART_TR_RTS_PIN                     IO_PORTA_02
#define UART_TR_CTS_PIN                     IO_PORTA_01

#define UART_TR_BUF_LEN     ((UART_DMA_MAX_LEN + 16) * 5) //最大缓存5包
// static u8 elet_uart_wbuf[UART_DMA_MAX_LEN] __attribute__((aligned(4)));//线程发送
// static u8 elet_uart_rbuf[UART_DMA_MAX_LEN] __attribute__((aligned(4)));//线程读取
static u8 elet_uart_ibuf[UART_DMA_MAX_LEN * 4] __attribute__((aligned(4)));//中断使用
// static u8 elet_uart_cbuf[UART_TR_BUF_LEN] __attribute__((aligned(4)));//缓存使用,可以扩大几倍

// #define UART_CBUF_SIZE          1024  //串口驱动缓存大小(单次数据超过cbuf, 则驱动会丢失数据)
// #define UART_FRAM_SIZE          256   //单次数据包最大值(每次cbuf缓存达到fram或串口收到一次数据, 就会起一次中断)
// static u8 devBuffer_static[UART_CBUF_SIZE] __attribute__((aligned(4)));       //dev DMA memory

static const uart_bus_t *m_uart_bus = NULL;
static volatile bool elet_uart_busy = 0;


uint8_t elet_uart_state_get(void)
{
	return elet_uart_busy;
}

uint32_t elet_uart_fifo_get_data_len(void)
{
	return m_uart_bus->get_data_len();
}

void elet_uart_data_read(uint8_t *p_d, uint32_t *data_len)
{
	OS_ENTER_CRITICAL();
	*data_len = m_uart_bus->read(p_d, *data_len, 0);
	OS_EXIT_CRITICAL();
}

void elet_uart_change_baud(uint32_t baud)
{
	if (m_uart_bus)
	{
		log_info("set_baud");
		m_uart_bus->set_baud(baud);
	}
}

static void elet_uart_putbyte(char a)
{
	if (m_uart_bus)
	{
		m_uart_bus->putbyte(a);
	}
}

void elet_uart_write(char *buf, uint16_t len)
{
	if (m_uart_bus)
	{
		m_uart_bus->write(buf, len);
	}
}

static u32 total_len_write = 0;
int cb_buf_io_uart_tx_peek_data(const void *data, int len, int has_more, void *extra)
{
	int r = 0;
	int size;
	// log_info("cb_buf_io_uart_tx_peek_data");
	// log_info("len %d\n", len);

	if (0 == elet_uart_busy)
	{
		elet_uart_busy = 1;
#if 1
		elet_uart_write(data, len);
		r = len;
		total_len_write += len;
#else
		size = len > 256 ? 256 : len;
		elet_uart_write(data, size);
		r = size;
		total_len_write += size;
#endif
	}

	log_info("%s len %d r %d total %d ", __func__, len, r, total_len_write);

	return r;
}

AT_VOLATILE_RAM_CODE
static void elet_uart_isr_cb(void *ut_bus, u32 status)
{
	struct sys_event e;
	uart_bus_t *ubus = (uart_bus_t *)ut_bus;
	static u32 total_len = 0;

	if (status == UT_TX)
	{
		JL_UART1->CON0 &= ~BIT(2); //关闭TX中断允许
		elet_uart_busy = 0;
		buf_io_uart_tx_send_event();
		return;
	}

	if (ubus->frame_length == 0)
	{
		//UART_BYTE_ONE 需要加处理判断满一帧,再转存
		// int ret = elet_uart_one_frame(ubus->kfifo.buffer, ubus->kfifo.buf_in);
		// if(ret == 0){
		return;
		// }
	}

	if (status == UT_RX || status == UT_RX_OT)
	{
		if (elet_nv_cfg_uart_flow_ctrl_get())
		{
			uart1_flow_ctl_rts_suspend();
		}

		u32 len = ubus->get_data_len_next();
		total_len += len;
		log_info("len_next %d total_len %d", len, total_len);

		// if (elet_bt_pin_cds_stat_get() &&
		//  (elet_ble_conn_stat_get() || elet_spp_conn_stat_get()))
		// {
		//  // u32 len = ubus->get_data_len_next();
		//  // total_len += len;
		//  // log_info("len_next %d total_len %d", len, total_len);
		//  extern int8_t send_data(const uint8_t *data, int len, int flush);
		//  send_data(&ubus->kfifo.buffer[ubus->kfifo.buf_out & (ubus->kfifo.buf_size - 1)], len, 1);
		//  ubus->kfifo.buf_out += len;
		// }
		// else
		{
			elet_at_cmd_post_sem();
		}
	}
}


void elet_uart_init(void)
{
#if 0
	struct uart_platform_data_t u_arg = {0};
	u_arg.tx_pin = UART_DB_TX_PIN;
	u_arg.rx_pin = UART_DB_RX_PIN;
	u_arg.rx_cbuf = devBuffer_static;
	u_arg.rx_cbuf_size = UART_CBUF_SIZE;  //>=
	u_arg.frame_length = UART_FRAM_SIZE;  //协议数据包
	u_arg.rx_timeout = 6;  //ms,兼容波特率较低
	u_arg.isr_cbfun = elet_uart_isr_cb;
	u_arg.baud = elet_nv_cfg_uart_rate_get();
	u_arg.is_9bit = 0;

	m_uart_bus = uart_dev_open(&u_arg);

	if (NULL == m_uart_bus)
	{
		log_error("uart_dev_open() failed\n");
		return;
	}

	// if (elet_nv_cfg_uart_flow_ctrl_get())
	{
		extern void uart1_flow_ctl_init(u8 rts_io, u8 cts_io);
		uart1_flow_ctl_init(UART_DB_RTS_PIN, UART_DB_CTS_PIN);
	}

	if (elet_nv_cfg_uart_flow_ctrl_get())
	{
		JL_UART1->CON1 |= BIT(13) | BIT(0);
		JL_UART1->CON1 |= BIT(14) | BIT(2);
	}
	else
	{
		JL_UART1->CON1 &= ~BIT(0);
		JL_UART1->CON1 &= ~BIT(2);
	}

#else
	log_info("%s[0x%02x 0x%02x]", __func__, UART_TR_TX_PIN, UART_TR_RX_PIN);
	struct uart_platform_data_t uart_arg = {0};
	uart_arg.tx_pin = UART_TR_TX_PIN;
	uart_arg.rx_pin = UART_TR_RX_PIN;
	// uart_arg.rx_cbuf = devBuffer_static;
	// uart_arg.rx_cbuf_size = sizeof(devBuffer_static);
	uart_arg.rx_cbuf = elet_uart_ibuf;
	uart_arg.rx_cbuf_size = sizeof(elet_uart_ibuf);
	// uart_arg.isr_cbfun = elet_uart_isr_hook;
	uart_arg.isr_cbfun = elet_uart_isr_cb;
	uart_arg.baud = elet_nv_cfg_uart_rate_get();
	uart_arg.rx_timeout = UART_DMA_TIMEOUT;//单位ms

	if (UART_PARITY_CHECK_MODE) //奇偶校验不能用DMA
	{
		uart_arg.is_9bit = 1;
		uart_arg.frame_length = 0;//UART_BYTE_ONE
	}
	else
	{
		uart_arg.is_9bit = 0;

		if (UART_BYTE_MODE == UART_BYTE_DMA)
		{
			uart_arg.frame_length = UART_DMA_MAX_LEN; //UART_BYTE_DMA //max:sizeof(elet_uart_ibuf);
		}
		else
		{
			uart_arg.frame_length = 0; //UART_BYTE_ONE
		}
	}

	m_uart_bus = uart_dev_open(&uart_arg);
#if 0
#if UART_RTS_CTS_MODE
	uart_tr_flow_ctl_init(UART_TR_RTS_PIN, UART_TR_CTS_PIN);
	JL_UART1->CON1 |= BIT(13) | BIT(0);
	JL_UART1->CON1 |= BIT(14) | BIT(2);
#endif
#else

	// uart_tr_flow_ctl_init(UART_TR_RTS_PIN, UART_TR_CTS_PIN);

	// if (elet_nv_cfg_uart_flow_ctrl_get())
	// {
	//  JL_UART1->CON1 |= BIT(13) | BIT(0);
	//  JL_UART1->CON1 |= BIT(14) | BIT(2);
	// }
	// else
	{
		JL_UART1->CON1 &= ~BIT(0);
		JL_UART1->CON1 &= ~BIT(2);
	}

#endif
	elet_uart_busy = 0;
#endif
}
