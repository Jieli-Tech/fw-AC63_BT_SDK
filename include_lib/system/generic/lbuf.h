#ifndef LBUF_H
#define LBUF_H


#include "typedef.h"
#include "list.h"
#include "system/spinlock.h"


struct lbuff_head {
    int magic_a;
    struct list_head head;
    struct list_head free;
    spinlock_t lock;
    u8 align;
    u16 priv_len;
    u32 total_size;
    u32 last_addr;
    void *priv;
    int magic_b;
};

struct lbuff_state {
    u32 avaliable;
    u32 fragment;
    u32 max_continue_len;
    int num;
};



struct lbuff_head *lbuf_init(void *buf, u32 len, int align, int priv_head_len);

void *lbuf_alloc(struct lbuff_head *head, u32 len);

void *lbuf_realloc(void *lbuf, int size);

int lbuf_empty(struct lbuff_head *head);

void lbuf_clear(struct lbuff_head *head);

void lbuf_push(void *lbuf, u8 channel_map);

void *lbuf_pop(struct lbuff_head *head, u8 channel);

int lbuf_free(void *lbuf);

void lbuf_free_check(void *lbuf, u32 rets);

u32 lbuf_free_space(struct lbuff_head *head);

void lbuf_state(struct lbuff_head *head, struct lbuff_state *state);

void lbuf_dump(struct lbuff_head *head);

int lbuf_traversal(struct lbuff_head *head);

int lbuf_avaliable(struct lbuff_head *head, int size);

int lbuf_real_size(void *lbuf);

int lbuf_remain_space(struct lbuff_head *head);

void lbuf_inc_ref(void *lbuf);

#endif


