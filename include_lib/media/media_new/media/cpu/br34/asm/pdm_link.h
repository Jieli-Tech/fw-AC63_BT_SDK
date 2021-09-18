#ifndef _PDM_LINK_H_
#define _PDM_LINK_H_

#define PLNK_CH			2
#define PLNK_LEN	    256
#define PLNK_BUF_NUM	2	//DualBuffer

#define PLNK_BUF_SIZE	(PLNK_CH * PLNK_BUF_NUM * PLNK_LEN)

typedef struct {
    u8 ch_num;
    u16 sr;
    s16 buf[PLNK_BUF_SIZE];
    void (*output)(void *priv, void *dat, u16 len, u8 buf_flag);
} audio_plnk_t;

void audio_plnk_test();
int audio_plnk_set_buf(audio_plnk_t *hdl, u16 irq_len);
int audio_plnk_open(audio_plnk_t *hdl);
int audio_plnk_close(void);
#endif/*_PDM_LINK_H_*/
