#ifndef __SIG_MESH_API_H__
#define __SIG_MESH_API_H__

#include "api/basic_depend.h"
#include "api/mesh_config.h"
#include "api/access.h"
#include "api/main.h"
#include "api/proxy.h"
#include "api/cfg_cli.h"
#include "api/cfg_srv.h"
#include "api/health_cli.h"
#include "api/health_srv.h"

/*******************************************************************/
/*
 *-------------------   common
 */
#define __noinit

#define ADV_SCAN_UNIT(_ms) ((_ms) * 8 / 5)

#define BT_MESH_ADDR_IS_UNICAST(addr) ((addr) && (addr) < 0x8000)
#define BT_MESH_ADDR_IS_GROUP(addr) ((addr) >= 0xc000 && (addr) <= 0xff00)
#define BT_MESH_ADDR_IS_VIRTUAL(addr) ((addr) >= 0x8000 && (addr) < 0xc000)
#define BT_MESH_ADDR_IS_RFU(addr) ((addr) >= 0xff00 && (addr) <= 0xfffb)

// -- output terminal color
#define RedBold             "\033[31;1;7m" // 红色加粗
#define RedBoldBlink        "\033[31;1;5m" // 红色加粗、闪烁
#define BlueBold            "\033[34;1;7m" // 蓝色加粗
#define BlueBoldBlink       "\033[34;1;5m" // 蓝色加粗、闪烁
#define PurpleBold          "\033[35;1m"   // 紫色加粗
#define PurpleBoldBlink     "\033[35;1;5m" // 紫色加粗、闪烁
#define Reset               "\033[0;25m"   // 颜色复位

/*******************************************************************/
/*
 *-------------------   models
 */

/** @def NET_BUF_SIMPLE_DEFINE
 *  @brief Define a net_buf_simple stack variable.
 *
 *  This is a helper macro which is used to define a net_buf_simple object
 *  on the stack.
 *
 *  @param _name Name of the net_buf_simple object.
 *  @param _size Maximum data storage for the buffer.
 */
#define NET_BUF_SIMPLE_DEFINE(_name, _size)     \
	u8_t net_buf_data_##_name[_size];       \
	struct net_buf_simple _name = {         \
		.data   = net_buf_data_##_name, \
		.len    = 0,                    \
		.size   = _size,                \
		.__buf  = net_buf_data_##_name, \
	}

/** @def NET_BUF_SIMPLE_DEFINE_STATIC
 *  @brief Define a static net_buf_simple variable.
 *
 *  This is a helper macro which is used to define a static net_buf_simple
 *  object.
 *
 *  @param _name Name of the net_buf_simple object.
 *  @param _size Maximum data storage for the buffer.
 */
#define NET_BUF_SIMPLE_DEFINE_STATIC(_name, _size)        \
	static __noinit u8_t net_buf_data_##_name[_size]; \
	static struct net_buf_simple _name = {            \
		.data   = net_buf_data_##_name,           \
		.len    = 0,                              \
		.size   = _size,                          \
		.__buf  = net_buf_data_##_name,           \
	}

/** @brief Simple network buffer representation.
 *
 *  This is a simpler variant of the net_buf object (in fact net_buf uses
 *  net_buf_simple internally). It doesn't provide any kind of reference
 *  counting, user data, dynamic allocation, or in general the ability to
 *  pass through kernel objects such as FIFOs.
 *
 *  The main use of this is for scenarios where the meta-data of the normal
 *  net_buf isn't needed and causes too much overhead. This could be e.g.
 *  when the buffer only needs to be allocated on the stack or when the
 *  access to and lifetime of the buffer is well controlled and constrained.
 *
 */
struct net_buf_simple {
    /** Pointer to the start of data in the buffer. */
    u8_t *data;

    /** Length of the data behind the data pointer. */
    u16_t len;

    /** Amount of data that this buffer can store. */
    u16_t size;

    /** Start of the data storage. Not to be accessed directly
     *  (the data pointer should be used instead).
     */
    u8_t *__buf;
};

/**
 * @brief Add a unsigned char variable at the tail of the buffer.
 *
 * @param buf Target buffer head address.
 *
 * @param val The variable will set.
 *
 * @return The tail address of the buffer.
 */
u8 *buffer_add_u8_at_tail(void *buf, u8 val);

/**
 * @brief Add a little-endian unsigned short variable at the tail of the buffer.
 *
 * @param buf Target buffer head address.
 *
 * @param val The variable will set.
 */
void buffer_add_le16_at_tail(void *buf, u16 val);

/**
 * @brief Add a big-endian unsigned short variable at the tail of the buffer.
 *
 * @param buf Target buffer head address.
 *
 * @param val The variable will set.
 */
void buffer_add_be16_at_tail(void *buf, u16 val);

/**
 * @brief Add a little-endian unsigned long variable at the tail of the buffer.
 *
 * @param buf Target buffer head address.
 *
 * @param val The variable will set.
 */
void buffer_add_le32_at_tail(void *buf, u32 val);

/**
 * @brief Add a big-endian unsigned long variable at the tail of the buffer.
 *
 * @param buf Target buffer head address.
 *
 * @param val The variable will set.
 */
void buffer_add_be32_at_tail(void *buf, u32 val);

/**
 * @brief Get the unsigned char variable from the buffer head address.
 *
 * @param buf Target buffer head address.
 *
 * @return Target variable.
 */
u8 buffer_pull_u8_from_head(void *buf);

/**
 * @brief Get the little-endian unsigned short variable from the buffer head address.
 *
 * @param buf Target buffer head address.
 *
 * @return Target variable.
 */
u16 buffer_pull_le16_from_head(void *buf);

/**
 * @brief Get the big-endian unsigned short variable from the buffer head address.
 *
 * @param buf Target buffer head address.
 *
 * @return Target variable.
 */
u16 buffer_pull_be16_from_head(void *buf);

/**
 * @brief Get the little-endian unsigned long variable from the buffer head address.
 *
 * @param buf Target buffer head address.
 *
 * @return Target variable.
 */
u32 buffer_pull_le32_from_head(void *buf);

/**
 * @brief Get the big-endian unsigned long variable from the buffer head address.
 *
 * @param buf Target buffer head address.
 *
 * @return Target variable.
 */
u32 buffer_pull_be32_from_head(void *buf);

/**
 * @brief Memcpy a array at the tail of the buffer.
 *
 * @param buf Target buffer head address.
 *
 * @param mem The source memory address.
 *
 * @param len The copy length.
 *
 * @return The result of the process : 0 is succ.
 */
void *buffer_memcpy(void *buf, const void *mem, u32 len);

/**
 * @brief Memset at the tail of the buffer.
 *
 * @param buf Target buffer head address.
 *
 * @param mem The set value.
 *
 * @param len The set length.
 *
 * @return The result of the process : 0 is succ.
 */
void *buffer_memset(struct net_buf_simple *buf, u8 val, u32 len);

/**
 * @brief Init the opcode at the head of the buffer.
 *
 * @param opcode Big-endian opcode.
 *
 * @return Little-endian opcode.
 */
u32 buffer_head_init(u32 opcode);

/**
 * @brief Loading the node info from storage (such as flash and so on).
 */
void settings_load(void);

#endif /* __SIG_MESH_API_H__ */
