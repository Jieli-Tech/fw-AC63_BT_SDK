
#include "system/includes.h"
#include "spp_user.h"
#include "string.h"
#include "circular_buf.h"
#include "btstack/avctp_user.h"
#include  "app_config.h"
#include  "classic/tws_api.h"
#include  "bt_common.h"

#if (USER_SUPPORT_PROFILE_SPP==1)

#define log_info printf

#define USER_SPP_SEND_POOL_SIZE (256L)
static u8 user_spp_send_pool[USER_SPP_SEND_POOL_SIZE] __attribute__((aligned(4)));
static void (*user_spp_send_wakeup)(void) = NULL;
static volatile u8 user_spp_send_busy = 0;
void (*spp_recieve_cbk)(void *priv, u8 *buf, u16 len) = NULL;
void (*spp_state_cbk)(u8 state) = NULL;

extern void rcsp_dev_select(u8 type);

void user_spp_send_ok_callback(int err_code)
{
    user_spp_send_busy = 0;
    /* putchar('K'); */
    if (user_spp_send_wakeup) {
        user_spp_send_wakeup();
    }
}

static void user_spp_state_cbk(u8 new_state)
{
    if (spp_state_cbk) {
        spp_state_cbk(new_state);
    }
}


static void user_spp_regiser_wakeup_send(void *priv, void *cbk)
{
    user_spp_send_wakeup = cbk;
}

static void user_spp_disconnect(void *priv)
{
    user_spp_send_busy = 0;
    user_spp_state_cbk(SPP_USER_ST_WAIT_DISC);
    user_send_cmd_prepare(USER_CTRL_SPP_DISCONNECT, 0, NULL);
}

static void user_spp_regiest_state_cbk(void *priv, void *cbk)
{
    spp_state_cbk = cbk;
}


static void user_spp_regiser_recieve(void *priv, void *cbk)
{
    spp_recieve_cbk = cbk;
}


static u32 user_spp_send(void *priv, u8 *buf, u32 len)
{
    /* log_info("\n-spp_tx(%d):",len); */
    /* put_buf(buf,len); */

    if (user_spp_send_busy == 1) {
        printf("SPP_USER_ERR_SEND_BUFF_BUSY\n");
        return SPP_USER_ERR_SEND_BUFF_BUSY;
    }

    if (len) {
        if (len > sizeof(user_spp_send_pool)) {
            printf("SPP_USER_ERR_SEND_OVER_LIMIT,%d\n", len);
            return SPP_USER_ERR_SEND_OVER_LIMIT;
        }

        user_spp_send_busy = 1;
        memcpy(user_spp_send_pool, buf, len);
        u32 ret = user_send_cmd_prepare(USER_CTRL_SPP_SEND_DATA, len, user_spp_send_pool);
        if (ret) {
            user_spp_send_busy = 0;
            return SPP_USER_ERR_SEND_FAIL;
        }
    }

    return SPP_USER_ERR_NONE;
}

void user_spp_data_handler(u8 packet_type, u16 ch, u8 *packet, u16 size)
{
    switch (packet_type) {
    case 1:
        log_info("spp connect ########################\n");
        user_spp_send_busy = 0;

        user_spp_state_cbk(SPP_USER_ST_CONNECT);
        break;
    case 2:
        log_info("spp disconnect ########################\n");
        user_spp_state_cbk(SPP_USER_ST_DISCONN);

        break;
    case 7:
        /* log_info("\n-spp_rx(%d)######:", size); */
        /* put_buf(packet, size); */

        if (spp_recieve_cbk) {
            spp_recieve_cbk((void *)ch, packet, size);
        }

        //收发测试，自动发送收到的数据;for test
        /* user_spp_send(0,packet,size); */

        break;
    }
}


static int user_spp_busy_state(void)
{
    return user_spp_send_busy;
}

static const struct spp_operation_t spp_operation = {
    .disconnect = (void *)user_spp_disconnect,
    .send_data = (void *)user_spp_send,
    .regist_wakeup_send = (void *)user_spp_regiser_wakeup_send,
    .regist_recieve_cbk = (void *)user_spp_regiser_recieve,
    .regist_state_cbk = (void *)user_spp_regiest_state_cbk,
    .busy_state = user_spp_busy_state,
};


void spp_get_operation_table(struct spp_operation_t **interface_pt)
{
    user_spp_send_busy = 0;
    *interface_pt = (void *)&spp_operation;
}


#endif
