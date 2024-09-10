#include "elet_at_cmd.h"
#include "elet_nv_cfg.h"
#include "elet_uart.h"
#include "board.h"

#include "api/sig_mesh_api.h"
#include "model_api.h"

#include "bt_common.h"
#include "board_config.h"
#include "system/includes.h"
#include "asm/power_interface.h"
#include "app_config.h"

#undef _DEBUG_H_
#define LOG_TAG_CONST       ELET_AT_CMD
#define LOG_TAG             "[ELET_AT_CMD]"
#include "debug.h"
const char log_tag_const_v_ELET_AT_CMD AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_ELET_AT_CMD AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_d_ELET_AT_CMD AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_ELET_AT_CMD AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_ELET_AT_CMD AT(.LOG_TAG_CONST) = 1;

static OS_SEM at_cmd_sem;
static u32 uart_baud_list[] =
{
	9600,
	14400,
	19200,
	38400,
	57600,
	115200,
};

#define PARSE_BUFFER_SIZE      0x100
static u8 parse_buffer[PARSE_BUFFER_SIZE] __attribute__((aligned(4)));
static u32 buff_len = 0;
static u8 rsp_ok[6] = "\r\nOK\r\n";

typedef struct
{
	u16 str_id;
	u16 str_len;
	const char *str;
} str_info_t;

enum
{
	STR_ID_NULL = 0,
	STR_ID_HEAD_AT_CMD,
	STR_ID_HEAD_AT_CHL,

	STR_ID_OK = 0x10,
	STR_ID_ERROR,

	STR_ID_MCID = 0x20,
	STR_ID_CCID,
	STR_ID_PID,
	STR_ID_REBOOT,
	STR_ID_NWK,
	STR_ID_UART,
	STR_ID_VER,
};

static const char at_head_at_cmd[]      = "AT+";
static const char at_head_at_chl[]      = "AT>";
static const char at_str_enter[]        = "\r\n";
static const char at_str_ok[]           = "OK";
static const char at_str_err[]          = "ERR";

static const char at_str_mcid[]         = "MCID";
static const char at_str_ccid[]         = "CCID";
static const char at_str_pid[]          = "PID";
static const char at_str_reboot[]       = "REBOOT";
static const char at_str_nwk[]          = "NWK";
static const char at_str_uart[]         = "UART";
static const char at_str_ver[]          = "VER";

static const char special_char[]        = {'+', '>', '=', '?', '\r', ','};

enum
{
	AT_CMD_OPT_NULL = 0,
	AT_CMD_OPT_SET, //设置
	AT_CMD_OPT_GET, //查询
};

#define INPUT_STR_INFO(id,string)  {.str_id = id, .str = string, .str_len = sizeof(string)-1,}

static const str_info_t at_head_str_table[] =
{
	INPUT_STR_INFO(STR_ID_HEAD_AT_CMD, at_head_at_cmd),
	// INPUT_STR_INFO(STR_ID_HEAD_AT_CHL, at_head_at_chl),
};

static const str_info_t at_cmd_str_table[] =
{
	INPUT_STR_INFO(STR_ID_MCID, at_str_mcid),
	INPUT_STR_INFO(STR_ID_CCID, at_str_ccid),
	INPUT_STR_INFO(STR_ID_PID, at_str_pid),
	INPUT_STR_INFO(STR_ID_REBOOT, at_str_reboot),
	INPUT_STR_INFO(STR_ID_NWK, at_str_nwk),
	INPUT_STR_INFO(STR_ID_UART, at_str_uart),
	INPUT_STR_INFO(STR_ID_VER, at_str_ver),
};

#define AT_STRING_SEND(a) at_cmd_send(a,strlen(a))

#define AT_PARAM_NEXT_P(a) (at_param_t*)&parse_buffer[a->next_offset]
//------------------------------------------
static u16 at_str_length(const u8 *packet, u8 end_char)
{
	u16 len = 0;

	while (*packet++ != end_char)
	{
		len++;
	}

	return len;
}

typedef struct
{
	volatile u8 len;//长度不包含结束符
	u8 next_offset;
	u8 data[0];     //带结束符0
} at_param_t;

static at_param_t *parse_param_split(const u8 *packet, u8 split_char, u8 end_char)
{
	u8 char1;
	int i = 0;
	at_param_t *par = parse_buffer;

	if (*packet == end_char)
	{
		return NULL;
	}

	log_info("%s:%s", __FUNCTION__, packet);

	par->len = 0;

	while (1)
	{
		char1 = packet[i++];

		if (char1 == end_char)
		{
			par->data[par->len] = 0;
			par->next_offset = 0;
			break;
		}
		else if (char1 == split_char)
		{
			par->data[par->len] = 0;
			par->len++;
			par->next_offset = &par->data[par->len] - parse_buffer;

			//init next par
			par = &par->data[par->len];
			par->len = 0;
		}
		else
		{
			par->data[par->len++] = char1;
		}

		if (&par->data[par->len] - parse_buffer >= PARSE_BUFFER_SIZE)
		{
			log_error("parse_buffer over");
			par->next_offset = 0;
			break;
		}
	}

#if 0
	par = parse_buffer;
	log_info("param_split:");

	while (par)
	{
		log_info("len=%d,par:%s", par->len, par->data);

		if (par->next_offset)
		{
			par = AT_PARAM_NEXT_P(par);
		}
		else
		{
			break;
		}
	}

#endif

	return (void *)parse_buffer;
}

//返回实际应该比较的长度
static u8 compare_special_char(u8 *packet)
{
	int i = 0, j = 0;

	while (1)
	{
		for (j = 0; j < sizeof(special_char); j++)
		{
			if (packet[i] == special_char[j])
			{
				if (j < 2)   //如果是+或>则应返回3
				{
					return 3;
				}
				else
				{
					return i;
				}
			}
		}

		i++;
	}
}

static str_info_t *at_check_match_string(u8 *packet, u16 size, char *str_table, int table_size)
{
	int max_count = table_size / sizeof(str_info_t);
	str_info_t *str_p = str_table;
	u8 compare_len = 0;

	compare_len = compare_special_char(packet);

	while (max_count--)
	{
		if (str_p->str_len <= size)
		{

			if (str_p->str_len == compare_len)
			{
				if (0 == memcmp(packet, str_p->str, str_p->str_len))
				{
					return str_p;
				}
			}
		}

		str_p++;
	}

	return NULL;
}

static uint8_t key_str_to_hex(uint8_t str_val)
{
	uint8_t key_value = str_val;

	if ((key_value >= '0') && (key_value <= '9'))
	{
		key_value = key_value - '0';
	}
	else if ((key_value >= 'a') && (key_value <= 'f'))
	{
		key_value = key_value - 'a' + 0x0a;
	}
	else if ((key_value >= 'A') && (key_value <= 'F'))
	{
		key_value = key_value - 'A' + 0x0a;
	}
	else
	{
		key_value = 0;
	}

	return key_value;
}

//字符转换HEX BUF, 00AABBCD -> 0x00,0xaa,0xbb,0xcd,返回长度
static uint16_t change_str_to_hex(uint8_t *str, uint16_t len, uint8_t *hex_buf)
{
	uint8_t value_h, value_l;
	uint16_t cnt, s_cn;

	s_cn = 0;

	for (cnt = 0; cnt < len; cnt += 2)
	{
		if (str[cnt] == 0)
		{
			break;
		}

		value_h = key_str_to_hex(str[cnt++]);
		value_l = key_str_to_hex(str[cnt]);

		str[s_cn] = (value_h << 4) + value_l;
		s_cn++;
	}

	return s_cn;
}

static uint8_t key_hex_to_char(uint8_t hex_val)
{
	uint8_t key_value = hex_val;

	if (key_value <= 9)
	{
		key_value = key_value + '0';
	}
	else if ((key_value >= 0x0a) && (key_value <= 0xf))
	{
		key_value = key_value + 'A';
	}
	else
	{
		key_value = '0';
	}

	return key_value;
}

int at_send_uart_data(uint8_t *packet, int16_t size, int16_t post_event)
{
	buf_io_uart_tx_send_data(packet, size, post_event);
}

static void at_cmd_send(const u8 *packet, int size)
{
	log_info("###at_cmd_send(%d):", size);
	// put_buf(packet, size);
	at_send_uart_data(at_str_enter, 2, 0);
	at_send_uart_data(packet, size, 0);
	at_send_uart_data(at_str_enter, 2, 1);
}

static void at_cmd_send_no_end(const u8 *packet, int size)
{
	at_send_uart_data(at_str_enter, 2, 0);
	at_send_uart_data(packet, size, 1);
}

u32 hex_2_str(u8 *hex, u32 hex_len, u8 *str)
{
	u32 str_len = 0;

	for (u32 i = 0; i < hex_len; i++)   //hex to string
	{
		if (hex[i] < 0x10)
		{
			sprintf(str + i * 2, "0%X", hex[i]);
		}
		else
		{
			sprintf(str + i * 2, "%X", hex[i]);
		}
	}

	str_len = hex_len * 2;
	return str_len;
}

static u32 str_2_hex(u8 *str, u32 str_len, u8 *hex)
{
	u32 hex_len = 0;
	u32 i = 0;

	for (i = 0; i < (str_len / 2); i++)
	{
		hex[i] = key_str_to_hex(str[i * 2]);
		hex[i] <<= 4;
		hex[i] += key_str_to_hex(str[i * 2 + 1]);
		log_info("hex----> %x", hex[i]);
	}

	hex_len = str_len / 2;
	return hex_len;
}

//字符转换十进制，返回值,
static u32 func_char_to_dec(u8 *char_buf, u8 end_char)
{
	u32 cnt = 0;
	u32 value = 0;
	u32 negative_flag = 0;
	u8 val;

	while (char_buf[cnt] != end_char)
	{
		val = char_buf[cnt];
		cnt++;

		if (val == ' ')
		{
			continue;
		}
		else if (val == '-')
		{
			negative_flag = 1;
		}
		else
		{
			if ((val >= '0') && (val <= '9'))
			{
				value = value * 10 + (val - '0');
			}
		}
	}

	if (value && negative_flag)
	{
		value = value * (-1);
	}

	return value;
}

static void at_respond_send_err(u32 err_id)
{
	// u8 buf[32];
	// sprintf(buf, "ERR:%d", err_id);
	// AT_STRING_SEND(buf);
	AT_STRING_SEND("ERROR");
}

static void at_packet_handler(u8 *packet, int size)
{
#if 0
	pr_app_at_parse(packet, size);
#else
	at_param_t *par;
	str_info_t *str_p;
	int ret = -1;
	u8 operator_type = AT_CMD_OPT_NULL; //
	u8 *parse_pt = packet;
	int parse_size = size;
	u8 buf[128] = {0};

	while (NULL == *parse_pt && parse_size)
	{
		parse_pt++;
		parse_size--;
	}

at_cmd_parse_start:
	str_p = at_check_match_string(parse_pt, parse_size, at_head_str_table, sizeof(at_head_str_table));

	if (!str_p)
	{
		log_info("###1unknow at_head:%s", packet);

		if (size >= 3 && 0x5A == packet[0] && 0xAA == packet[size - 1])
		{
			extern void vendor_server_send(uint8_t *p_data, uint32_t len);
			vendor_server_send(&packet[1], size - 2);
		}
		else
		{
			log_info("invalid data");
		}

		return;
	}

	parse_pt   += str_p->str_len;
	parse_size -= str_p->str_len;

	if (str_p->str_id == STR_ID_HEAD_AT_CMD)
	{
		str_p = at_check_match_string(parse_pt, parse_size, at_cmd_str_table, sizeof(at_cmd_str_table));

		if (!str_p)
		{
			log_info("###2unknow at_cmd:%s", packet);
			at_respond_send_err(ERR_AT_CMD);
			return;
		}

		parse_pt    += str_p->str_len;
		parse_size -= str_p->str_len;

		if (parse_pt[0] == '=')
		{
			operator_type = AT_CMD_OPT_SET;
		}
		else if (parse_pt[0] == '?')
		{
			operator_type = AT_CMD_OPT_GET;
		}

		parse_pt++;
	}
	else if (str_p->str_id == STR_ID_HEAD_AT_CMD)
	{
		operator_type = AT_CMD_OPT_SET;
	}

	//    if(operator_type == AT_CMD_OPT_NULL)
	//    {
	//        AT_STRING_SEND(at_str_err);
	//        log_info("###3unknow operator_type:%s", packet);
	//        return;
	//    }

	log_info("str_id:%d", str_p->str_id);

	par = parse_param_split(parse_pt, ' ', '\r');

	log_info("\n par->data: %s", par->data);

	switch (str_p->str_id)
	{
		case STR_ID_ERROR:
			log_info("STR_ID_ERROR\n");
			break;

		case STR_ID_REBOOT:
			log_info("STR_ID_Z\n");

			if (operator_type == AT_CMD_OPT_SET)
			{
				u8 type = func_char_to_dec(par->data, '\0');

				if (1 == type)
				{
					cpu_reset();
				}
				else if (2 == type)
				{
					elet_nv_cfg_init(BTA_NVRAM_RESET_OPCODE);
					cpu_reset();
				}
				else
				{
					AT_STRING_SEND("ERROR");
				}
			}
			else
			{
				AT_STRING_SEND("ERROR");
			}

			break;

		case STR_ID_VER:
			log_info("STR_ID_VER\n");

			if (operator_type == AT_CMD_OPT_GET)
			{
				u8 len = 0;
				sprintf(buf, "+VER:%s", Bluetooth_Complete_Version);
				len = strlen(buf);
				at_cmd_send(buf, len);
			}
			else
			{
				AT_STRING_SEND("ERROR");
			}

			break;

		case STR_ID_MCID:
			if (operator_type == AT_CMD_OPT_SET)
			{
				u16 mcid;

				if (par->len != 4)
				{
					AT_STRING_SEND("ERROR");
					break;
				}

				mcid = strtol(par->data, NULL, 16);

				elet_nv_cfg_mesh_mcid_set(mcid);
				AT_STRING_SEND("OK");
			}
			else
			{
				sprintf(buf, "+MCID:%04X", elet_nv_cfg_mesh_mcid_get());
				at_cmd_send(buf, strlen(buf));
			}

			break;

		case STR_ID_CCID:
			if (operator_type == AT_CMD_OPT_SET)
			{
				u16 ccid;

				if (par->len != 4)
				{
					AT_STRING_SEND("ERROR");
					break;
				}

				ccid = strtol(par->data, NULL, 16);

				elet_nv_cfg_mesh_ccid_set(ccid);
				AT_STRING_SEND("OK");
			}
			else
			{
				sprintf(buf, "+CCID:%04X", elet_nv_cfg_mesh_ccid_get());
				at_cmd_send(buf, strlen(buf));
			}

			break;

		case STR_ID_PID:
			if (operator_type == AT_CMD_OPT_SET)
			{
				u32 pid;

				if (par->len != 6)
				{
					AT_STRING_SEND("ERROR");
					break;
				}

				pid = strtol(par->data, NULL, 16);

				elet_nv_cfg_mesh_pid_set(pid);
				AT_STRING_SEND("OK");
			}
			else
			{
				sprintf(buf, "+PID:%06X", elet_nv_cfg_mesh_pid_get());
				at_cmd_send(buf, strlen(buf));
			}

			break;

		case STR_ID_NWK:
			if (operator_type == AT_CMD_OPT_SET)
			{
				u8 type = func_char_to_dec(par->data, '\0');

				if (1 == type)
				{
					bt_mesh_reset();
					AT_STRING_SEND("OK");
				}
				else
				{
					AT_STRING_SEND("ERROR");
				}
			}
			else
			{
				extern uint8_t elet_prov_complete_flag;
				sprintf(buf, "+NWK:%d", elet_prov_complete_flag);
				at_cmd_send(buf, strlen(buf));
			}

			break;

		case STR_ID_UART:
			log_info("STR_ID_UART\n");

			if (operator_type == AT_CMD_OPT_SET)
			{
				u32 uart_baud_index = func_char_to_dec(par->data, '\0');

				if (uart_baud_index >= (sizeof(uart_baud_list) / sizeof(uart_baud_list[0])))
				{
					AT_STRING_SEND("ERROR");
					break;
				}

				AT_STRING_SEND("OK");
				os_time_dly(10);

				u32 uart_baud = uart_baud_list[uart_baud_index];
				log_info("set baud = %d", uart_baud);

				elet_nv_cfg_uart_rate_set(uart_baud);
				elet_uart_change_baud(uart_baud);
			}
			else
			{
				int i;

				for (i = 0; i < sizeof(uart_baud_list); i++)
				{
					if (uart_baud_list[i] == elet_nv_cfg_uart_rate_get())
					{
						break;
					}
				}

				sprintf(buf, "+UART:%d", i);
				at_cmd_send(buf, strlen(buf));
			}

			break;

		default:
			break;
	}

#endif
}

static void elet_at_cmd_task(void *p)
{
	int ret = 0;

	while (1)
	{
		ret = os_sem_pend(&at_cmd_sem, 0);
		log_debug("%s[os_sem_pend -> ret:%d]", __func__, ret);
		buff_len = PARSE_BUFFER_SIZE;
		elet_uart_data_read(parse_buffer, &buff_len);
		at_packet_handler(parse_buffer, buff_len);
		buff_len = 0;

		if (elet_nv_cfg_uart_flow_ctrl_get())
		{
			uart1_flow_ctl_rts_resume();
		}
	}
}

void elet_at_cmd_post_sem(void)
{
	int ret = OS_NO_ERR;
	ret = os_sem_post(&at_cmd_sem);

	if (ret) log_error("%s[ret:%d]", __func__, ret);

	return;
}

void elet_at_cmd_init(void)
{
	int ret = OS_NO_ERR;
	os_sem_create(&at_cmd_sem, 0);
	os_sem_set(&at_cmd_sem, 0);
	ret = os_task_create(elet_at_cmd_task, NULL, 7, 512, 0, "at_cmd");

	if (ret != OS_NO_ERR) log_error("%s %s create fail 0x%x", __func__, "uart_tr", ret);

	AT_STRING_SEND("+READY");
}
