#include "elet_nv_cfg.h"

#include "syscfg_id.h"

#include "board_config.h"
#include "system/includes.h"
#include "asm/power_interface.h"
#include "app_config.h"

#undef _DEBUG_H_
#define LOG_TAG_CONST       ELET_NV_CFG
#define LOG_TAG             "[ELET_NV_CFG]"
#include "debug.h"
const char log_tag_const_v_ELET_NV_CFG AT(.LOG_TAG_CONST) = 0;
const char log_tag_const_i_ELET_NV_CFG AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_d_ELET_NV_CFG AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_w_ELET_NV_CFG AT(.LOG_TAG_CONST) = 1;
const char log_tag_const_e_ELET_NV_CFG AT(.LOG_TAG_CONST) = 1;


static const nv_cfg_t nv_cfg_default =
{
	.data_init_flag     = BTA_NVRAM_FlASH_DATA_IS_INIT,
	.data_len           = sizeof(nv_cfg_t),

	.fw_num             = USER_VERSION,

	.mesh_name          = "ET23_MESH",
	.mesh_mcid          = 0x0917,
	.mesh_ccid          = 0x0302,
	.mesh_pid           = 0x000001,

	.uart_rate          = 115200,
	.uart_flow_ctrl     = 0,
};

static nv_cfg_t nv_cfg = {0};


extern u32 hex_2_str(u8 *hex, u32 hex_len, u8 *str);

uint8_t *elet_nv_cfg_mesh_name_get(void)
{
	return nv_cfg.mesh_name;
}

void elet_nv_cfg_mesh_mcid_set(uint16_t mesh_mcid)
{
	if (nv_cfg.mesh_mcid == mesh_mcid) return;

	nv_cfg.mesh_mcid = mesh_mcid;
	syscfg_write(AT_CHAR_MESH_MCID, (uint8_t *)&nv_cfg.mesh_mcid, sizeof(uint16_t) / sizeof(uint8_t));
}

uint16_t elet_nv_cfg_mesh_mcid_get(void)
{
	return nv_cfg.mesh_mcid;
}

void elet_nv_cfg_mesh_ccid_set(uint16_t mesh_ccid)
{
	if (nv_cfg.mesh_ccid == mesh_ccid) return;

	nv_cfg.mesh_ccid = mesh_ccid;
	syscfg_write(AT_CHAR_MESH_CCID, (uint8_t *)&nv_cfg.mesh_ccid, sizeof(uint16_t) / sizeof(uint8_t));
}

uint16_t elet_nv_cfg_mesh_ccid_get(void)
{
	return nv_cfg.mesh_ccid;
}

void elet_nv_cfg_mesh_pid_set(uint32_t mesh_pid)
{
	// if (0 == memcmp(nv_cfg.mesh_pid, mesh_pid, sizeof(nv_cfg.mesh_pid))) return;

	// memcpy(nv_cfg.mesh_pid, mesh_pid, sizeof(nv_cfg.mesh_pid));
	// syscfg_write(AT_CHAR_MESH_PID, nv_cfg.mesh_pid, sizeof(nv_cfg.mesh_pid));
	if (nv_cfg.mesh_pid == mesh_pid) return;

	nv_cfg.mesh_pid = mesh_pid;
	syscfg_write(AT_CHAR_MESH_PID, (uint8_t *)&nv_cfg.mesh_pid, sizeof(uint32_t) / sizeof(uint8_t));
}

uint32_t elet_nv_cfg_mesh_pid_get(void)
{
	return nv_cfg.mesh_pid;
}

void elet_nv_cfg_uart_rate_set(uint32_t uart_rate)
{
	if (nv_cfg.uart_rate == uart_rate) return;

	nv_cfg.uart_rate = uart_rate;
	syscfg_write(AT_CHAR_UART_RATE, (uint8_t *)&nv_cfg.uart_rate, sizeof(uint32_t) / sizeof(uint8_t));
}

uint32_t elet_nv_cfg_uart_rate_get(void)
{
	return nv_cfg.uart_rate;
}

void elet_nv_cfg_uart_flow_ctrl_set(uint8_t uart_flow_ctrl)
{
	if (nv_cfg.uart_flow_ctrl == uart_flow_ctrl) return;

	nv_cfg.uart_flow_ctrl = uart_flow_ctrl;
	syscfg_write(AT_CHAR_UART_FLOW_CONTROL, &nv_cfg.uart_flow_ctrl, 1);
}

uint8_t elet_nv_cfg_uart_flow_ctrl_get(void)
{
	return nv_cfg.uart_flow_ctrl;
}

void elet_nv_cfg_reset_default(void)
{
	elet_nv_cfg_init(BTA_NVRAM_RESET_OPCODE);
}

void elet_nv_cfg_init(uint32_t operation)
{
	uint32_t data_init_flag;
	uint32_t cfg_len;
	uint32_t fw_num;

	log_info("elet_nv_cfg_init %X", operation);

	syscfg_read(AT_CHAR_CFG_INIT, (uint8_t *)&data_init_flag, sizeof(uint32_t) / sizeof(uint8_t));
	log_info("data_init_flag %X", data_init_flag);
	syscfg_read(AT_CHAR_CFG_LEN, (uint8_t *)&cfg_len, sizeof(uint32_t) / sizeof(uint8_t));
	log_info("cfg_len %u", cfg_len);
	syscfg_read(AT_CHAR_CFG_FW, (uint8_t *)&fw_num, sizeof(uint32_t) / sizeof(uint8_t));
	log_info("fw_num %u", fw_num);

	if (BTA_NVRAM_FlASH_DATA_IS_INIT != data_init_flag ||
		USER_VERSION != fw_num ||
		BTA_NVRAM_RESET_OPCODE == operation)
	{
		log_info("nv_cfg reset");
		memcpy(&nv_cfg, &nv_cfg_default, sizeof(nv_cfg_t));

		syscfg_write(AT_CHAR_CFG_INIT, (uint8_t *)&nv_cfg.data_init_flag, sizeof(uint32_t) / sizeof(uint8_t));
		syscfg_write(AT_CHAR_CFG_LEN, (uint8_t *)&nv_cfg.data_len, sizeof(uint32_t) / sizeof(uint8_t));
		syscfg_write(AT_CHAR_CFG_FW, (uint8_t *)&nv_cfg.fw_num, sizeof(uint32_t) / sizeof(uint8_t));

		syscfg_write(AT_CHAR_MESH_NAME, nv_cfg.mesh_name, sizeof(nv_cfg.mesh_name));
		syscfg_write(AT_CHAR_MESH_MCID, (uint8_t *)&nv_cfg.mesh_mcid, sizeof(uint16_t) / sizeof(uint8_t));
		syscfg_write(AT_CHAR_MESH_CCID, (uint8_t *)&nv_cfg.mesh_ccid, sizeof(uint16_t) / sizeof(uint8_t));
		syscfg_write(AT_CHAR_MESH_PID, (uint8_t *)&nv_cfg.mesh_pid, sizeof(uint32_t) / sizeof(uint8_t));

		syscfg_write(AT_CHAR_UART_RATE, (uint8_t *)&nv_cfg.uart_rate, sizeof(uint32_t) / sizeof(uint8_t));
		syscfg_write(AT_CHAR_UART_FLOW_CONTROL, (uint8_t *)&nv_cfg.uart_flow_ctrl, 1);
	}
	else
	{
		syscfg_read(AT_CHAR_MESH_NAME, nv_cfg.mesh_name, sizeof(nv_cfg.mesh_name));
		syscfg_read(AT_CHAR_MESH_MCID, (uint8_t *)&nv_cfg.mesh_mcid, sizeof(uint16_t) / sizeof(uint8_t));
		syscfg_read(AT_CHAR_MESH_CCID, (uint8_t *)&nv_cfg.mesh_ccid, sizeof(uint16_t) / sizeof(uint8_t));
		syscfg_read(AT_CHAR_MESH_PID, (uint8_t *)&nv_cfg.mesh_pid, sizeof(uint32_t) / sizeof(uint8_t));

		syscfg_read(AT_CHAR_UART_RATE, (uint8_t *)&nv_cfg.uart_rate, sizeof(uint32_t) / sizeof(uint8_t));
		syscfg_read(AT_CHAR_UART_FLOW_CONTROL, (uint8_t *)&nv_cfg.uart_flow_ctrl, 1);
	}

	log_info("mesh_mcid 0x%04x", nv_cfg.mesh_mcid);
	log_info("uart_rate %d", nv_cfg.uart_rate);
	log_info("uart_flow_ctrl %x", nv_cfg.uart_flow_ctrl);
}