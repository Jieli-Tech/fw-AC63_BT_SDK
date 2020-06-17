#ifndef __ADAPTATION_H__
#define __ADAPTATION_H__

#include "api/basic_depend.h"
#include "generic/list.h"
#include "generic/jiffies.h"
#include "misc/byteorder.h"
#include "misc/slist.h"
#include "kernel/atomic_h.h"
#include "api/sig_mesh_api.h"

/*******************************************************************/
/*
 *-------------------  common adapter
 */
//< error type
#define ENONE           0  /* Err None */
#define	EPERM		    1	/* Operation not permitted */
#define	ENOENT		    2	/* No such file or directory */
#define	ESRCH		    3	/* No such process */
#define	EINTR		    4	/* Interrupted system call */
#define	EIO		        5	/* I/O error */
#define	ENXIO		    6	/* No such device or address */
#define	E2BIG		    7	/* Argument list too long */
#define	ENOEXEC		    8	/* Exec format error */
#define	EBADF		    9	/* Bad file number */
#define	ECHILD		    10	/* No child processes */
#define	EAGAIN		    11	/* Try again */
#define	ENOMEM		    12	/* Out of memory */
#define	EACCES		    13	/* Permission denied */
#define	EFAULT		    14	/* Bad address */
#define	ENOTBLK		    15	/* Block device required */
#define	EBUSY		    16	/* Device or resource busy */
#define	EEXIST		    17	/* File exists */
#define	EXDEV		    18	/* Cross-device link */
#define	ENODEV		    19	/* No such device */
#define	ENOTDIR		    20	/* Not a directory */
#define	EISDIR		    21	/* Is a directory */
#define	EINVAL		    22	/* Invalid argument */
#define	ENFILE		    23	/* File table overflow */
#define	EMFILE		    24	/* Too many open files */
#define	ENOTTY		    25	/* Not a typewriter */
#define	ETXTBSY		    26	/* Text file busy */
#define	EFBIG		    27	/* File too large */
#define	ENOSPC		    28	/* No space left on device */
#define	ESPIPE		    29	/* Illegal seek */
#define	EROFS		    30	/* Read-only file system */
#define	EMLINK		    31	/* Too many links */
#define	EPIPE		    32	/* Broken pipe */
#define	EDOM		    33	/* Math argument out of domain of func */
#define	ERANGE		    34	/* Math result not representable */
#define ENOTSUP         35      /* Unsupported value */
#define EMSGSIZE        36     /* Message size */
#define ENOBUFS         55      /* No buffer space available */
#define ENOTCONN        57     /* Socket is not connected */
#undef ETIMEDOUT
#define ETIMEDOUT       60    /* Connection timed out */
#define EALREADY        69    /* Operation already in progress */
#define ECANCELED       72 /* Operation canceled */
#define EBADMSG         77 /* Invalid STREAMS message */
#define SL_ERROR_BSD_EADDRNOTAVAIL          (-99L)  /* Cannot assign requested address */
#define EADDRNOTAVAIL                       SL_ERROR_BSD_EADDRNOTAVAIL
//< base operation
#define min(a, b)       MIN(a, b)
#define max(a, b)       MAX(a, b)
#define BIT_MASK(n)     (BIT(n) - 1)
#define CONTAINER_OF    container_of
#undef offsetof
#define offsetof        list_offsetof
#define _STRINGIFY(x)   #x

#define ___in_section(a, b, c) \
	__attribute__((section("." _STRINGIFY(a)			\
				"." _STRINGIFY(b)			\
				"." _STRINGIFY(c))))
#define __in_section(a, b, c) ___in_section(a, b, c)
/* Unaligned access */
#define UNALIGNED_GET(p)						\
__extension__ ({							\
	struct  __attribute__((__packed__)) {				\
		__typeof__(*(p)) __v;					\
	} *__p = (__typeof__(__p)) (p);					\
	__p->__v;							\
})

#define popcount(x) __builtin_popcount(x)

#define CODE_UNREACHABLE return 0

#define ALWAYS_INLINE   inline

#define __noinit

#define __ASSERT                ASSERT
#define __ASSERT_NO_MSG(test)   ASSERT(test)
#define BUILD_ASSERT(...)

/**
 * @brief Check for macro definition in compiler-visible expressions
 *
 * This trick was pioneered in Linux as the config_enabled() macro.
 * The madness has the effect of taking a macro value that may be
 * defined to "1" (e.g. CONFIG_MYFEATURE), or may not be defined at
 * all and turning it into a literal expression that can be used at
 * "runtime".  That is, it works similarly to
 * "defined(CONFIG_MYFEATURE)" does except that it is an expansion
 * that can exist in a standard expression and be seen by the compiler
 * and optimizer.  Thus much ifdef usage can be replaced with cleaner
 * expressions like:
 *
 *     if (IS_ENABLED(CONFIG_MYFEATURE))
 *             myfeature_enable();
 *
 * INTERNAL
 * First pass just to expand any existing macros, we need the macro
 * value to be e.g. a literal "1" at expansion time in the next macro,
 * not "(1)", etc...  Standard recursive expansion does not work.
 */
#define IS_ENABLED(config_macro) _IS_ENABLED1(config_macro)

/* Now stick on a "_XXXX" prefix, it will now be "_XXXX1" if config_macro
 * is "1", or just "_XXXX" if it's undefined.
 *   ENABLED:   _IS_ENABLED2(_XXXX1)
 *   DISABLED   _IS_ENABLED2(_XXXX)
 */
#define _IS_ENABLED1(config_macro) _IS_ENABLED2(_XXXX##config_macro)

/* Here's the core trick, we map "_XXXX1" to "_YYYY," (i.e. a string
 * with a trailing comma), so it has the effect of making this a
 * two-argument tuple to the preprocessor only in the case where the
 * value is defined to "1"
 *   ENABLED:    _YYYY,    <--- note comma!
 *   DISABLED:   _XXXX
 *   DISABLED:   _XXXX0
 */
#define _XXXX1 _YYYY,
#define _XXXX0

/* Then we append an extra argument to fool the gcc preprocessor into
 * accepting it as a varargs macro.
 *                         arg1   arg2  arg3
 *   ENABLED:   _IS_ENABLED3(_YYYY,    1,    0)
 *   DISABLED   _IS_ENABLED3(_XXXX 1,  0)
 *   DISABLED   _IS_ENABLED3(_XXXX0 1,  0)
 */
#define _IS_ENABLED2(one_or_two_args) _IS_ENABLED3(one_or_two_args true, false)

/* And our second argument is thus now cooked to be 1 in the case
 * where the value is defined to 1, and 0 if not:
 */
#define _IS_ENABLED3(ignore_this, val, ...) val

static inline int irq_lock(void)
{
    CPU_CRITICAL_ENTER();

    return 0;
}

static inline void irq_unlock(int key)
{
    CPU_CRITICAL_EXIT();
}

/**
 *
 * @brief find most significant bit set in a 32-bit word
 *
 * This routine finds the first bit set starting from the most significant bit
 * in the argument passed in and returns the index of that bit.  Bits are
 * numbered starting at 1 from the least significant bit.  A return value of
 * zero indicates that the value passed is zero.
 *
 * @return most significant bit set, 0 if @a op is 0
 */

static ALWAYS_INLINE unsigned int find_msb_set(u32_t op)
{
    if (!op) {
        return 0;
    }
    return 32 - __builtin_clz(op);
}

/**
 *
 * @brief find least significant bit set in a 32-bit word
 *
 * This routine finds the first bit set starting from the least significant bit
 * in the argument passed in and returns the index of that bit.  Bits are
 * numbered starting at 1 from the least significant bit.  A return value of
 * zero indicates that the value passed is zero.
 *
 * @return least significant bit set, 0 if @a op is 0
 */

static ALWAYS_INLINE unsigned int find_lsb_set(u32_t op)
{
    return __builtin_ffs(op);
}

/*******************************************************************/
/*
 *-------------------   k_delayed_work
 */
#define K_FOREVER       (-1)

/**
 * @brief Generate null timeout delay.
 *
 * This macro generates a timeout delay that that instructs a kernel API
 * not to wait if the requested operation cannot be performed immediately.
 *
 * @return Timeout delay value.
 */
#define K_NO_WAIT 0

/**
 * @brief Generate timeout delay from milliseconds.
 *
 * This macro generates a timeout delay that that instructs a kernel API
 * to wait up to @a ms milliseconds to perform the requested operation.
 *
 * @param ms Duration in milliseconds.
 *
 * @return Timeout delay value.
 */
#define K_MSEC(ms)     (ms)

/**
 * @brief Generate timeout delay from seconds.
 *
 * This macro generates a timeout delay that that instructs a kernel API
 * to wait up to @a s seconds to perform the requested operation.
 *
 * @param s Duration in seconds.
 *
 * @return Timeout delay value.
 */
#define K_SECONDS(s)   K_MSEC((s) * MSEC_PER_SEC)

/**
 * @brief Generate timeout delay from minutes.
 *
 * This macro generates a timeout delay that that instructs a kernel API
 * to wait up to @a m minutes to perform the requested operation.
 *
 * @param m Duration in minutes.
 *
 * @return Timeout delay value.
 */
#define K_MINUTES(m)   K_SECONDS((m) * 60)

/**
 * @brief Generate timeout delay from hours.
 *
 * This macro generates a timeout delay that that instructs a kernel API
 * to wait up to @a h hours to perform the requested operation.
 *
 * @param h Duration in hours.
 *
 * @return Timeout delay value.
 */
#define K_HOURS(h)     K_MINUTES((h) * 60)

u32 k_uptime_get(void);
u32 k_uptime_get_32(void);
u32 k_delayed_work_remaining_get(struct k_delayed_work *timer);
void k_delayed_work_submit(struct k_delayed_work *timer, u32 timeout);
void k_delayed_work_cancel(struct k_delayed_work *timer);
void k_work_submit(struct k_work *work);
void k_delayed_work_init(struct k_delayed_work *timer, void *callback);


/*******************************************************************/
/*
 *-------------------   Ble core
 */
/** @brief Bluetooth UUID types */
enum {
    BT_UUID_TYPE_16,
    BT_UUID_TYPE_32,
    BT_UUID_TYPE_128,
};

/** @brief This is a 'tentative' type and should be used as a pointer only */
struct bt_uuid {
    u8_t type;
};

struct bt_uuid_16 {
    struct bt_uuid uuid;
    u16_t val;
};

struct bt_uuid_32 {
    struct bt_uuid uuid;
    u32_t val;
};

struct bt_uuid_128 {
    struct bt_uuid uuid;
    u8_t val[16];
};

/** Bluetooth Device Address */
typedef struct {
    u8_t  val[6];
} bt_addr_t;

/** Bluetooth LE Device Address */
typedef struct {
    u8_t      type;
    bt_addr_t a;
} bt_addr_le_t;


/*******************************************************************/
/*
 *-------------------   utils
 */
#define bt_hex      bt_hex_real

const char *bt_hex_real(const void *buf, size_t len);

int bt_rand(void *buf, size_t len);

const char *bt_uuid_str(const struct bt_uuid *uuid);



/*******************************************************************/
/*
 *-------------------   hci_core
 */
typedef void (*bt_dh_key_cb_t)(const u8_t key[32]);

struct bt_conn {
    u16_t			handle;
    u16_t           mtu;
    atomic_tt		ref;
};

struct bt_conn_cb {
    /** @brief A new connection has been established.
     *
     *  This callback notifies the application of a new connection.
     *  In case the err parameter is non-zero it means that the
     *  connection establishment failed.
     *
     *  @param conn New connection object.
     *  @param err HCI error. Zero for success, non-zero otherwise.
     */
    void (*connected)(struct bt_conn *conn, u8_t err);

    /** @brief A connection has been disconnected.
     *
     *  This callback notifies the application that a connection
     *  has been disconnected.
     *
     *  @param conn Connection object.
     *  @param reason HCI reason for the disconnection.
     */
    void (*disconnected)(struct bt_conn *conn, u8_t reason);
};

struct bt_pub_key_cb {
    /** @brief Callback type for Public Key generation.
     *
     *  Used to notify of the local public key or that the local key is not
     *  available (either because of a failure to read it or because it is
     *  being regenerated).
     *
     *  @param key The local public key, or NULL in case of no key.
     */
    void (*func)(const u8_t key[64]);

    struct bt_pub_key_cb *_next;
};

const u8_t *bt_pub_key_get(void);

int bt_pub_key_gen(struct bt_pub_key_cb *new_cb);

int bt_dh_key_gen(const u8_t remote_pk[64], bt_dh_key_cb_t cb);

struct bt_conn *bt_conn_ref(struct bt_conn *conn);

void bt_conn_unref(struct bt_conn *conn);

void bt_conn_cb_register(struct bt_conn_cb *cb);

void hci_core_init(void);


/*******************************************************************/
/*
 *-------------------   adv_core
 */
/* Advertising types */
#define BT_LE_ADV_IND                           0x00
#define BT_LE_ADV_DIRECT_IND                    0x01
#define BT_LE_ADV_SCAN_IND                      0x02
#define BT_LE_ADV_NONCONN_IND                   0x03
#define BT_LE_ADV_DIRECT_IND_LOW_DUTY           0x04
/* Needed in advertising reports when getting info about */
#define BT_LE_ADV_SCAN_RSP                      0x04

#define BT_DATA_URI                             0x24 /* URI */
#define BT_DATA_MESH_PROV                       0x29 /* Mesh Provisioning PDU */
#define BT_DATA_MESH_MESSAGE                    0x2a /* Mesh Networking PDU */
#define BT_DATA_MESH_BEACON                     0x2b /* Mesh Beacon */

void bt_mesh_adv_init(void);


/*******************************************************************/
/*
 *-------------------   scan_core
 */
typedef void bt_le_scan_cb_t(const bt_addr_le_t *addr, s8_t rssi,
                             u8_t adv_type, struct net_buf_simple *buf);

int bt_le_scan_start(bt_le_scan_cb_t cb);

int bt_le_scan_stop(void);


/*******************************************************************/
/*
 *-------------------   gatt_core
 */
#define BT_HCI_ERR_REMOTE_USER_TERM_CONN        0x13
#define BT_GATT_ERR(_att_err)                   (-(_att_err))
#define BT_ATT_ERR_INVALID_ATTRIBUTE_LEN	    0x0d
#define BT_ATT_ERR_VALUE_NOT_ALLOWED            0x13
#define BT_GATT_CCC_NOTIFY			            0x0001

u8 *get_server_data_addr(void);

void bt_gatt_service_register(u32 uuid);

void bt_gatt_service_unregister(u32 uuid);

int bt_gatt_notify(struct bt_conn *conn, const void *data, u16_t len);

u16 bt_gatt_get_mtu(struct bt_conn *conn);

void bt_conn_disconnect(struct bt_conn *conn, u8 reason);

void proxy_gatt_init(void);

#endif /* __ADAPTATION_H__ */
