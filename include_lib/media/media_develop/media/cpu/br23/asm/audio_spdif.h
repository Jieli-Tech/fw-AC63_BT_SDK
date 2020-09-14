#ifndef AUDIO_SPDIF_H
#define AUDIO_SPDIF_H

#include "generic/typedef.h"
#include "generic/list.h"

#define SPDIF_STATE_INIT		1
#define SPDIF_STATE_OPEN      	2
#define SPDIF_STATE_START     	3
#define SPDIF_STATE_STOP      	4



#define SPDIF_IN_PORT_A      BIT(0)    //PA9
#define SPDIF_IN_PORT_B      BIT(1)    //PA10
#define SPDIF_IN_PORT_C      BIT(2)    //PB0
#define SPDIF_IN_PORT_D      BIT(3)    //PB1
#define SPDIF_IN_IOMAP_A     ( BIT(4) | BIT(0))  //input_ch 11
#define SPDIF_IN_IOMAP_B     ( BIT(4) | BIT(1))  //input_ch 10
#define SPDIF_IN_IOMAP_C     ( BIT(4) | BIT(2))  //input_ch 8
#define SPDIF_IN_IOMAP_D     ( BIT(4) | BIT(3))  //input_ch 9

#define SPDIF_OUT_PORT_A      BIT(0)    //PB11
#define SPDIF_OUT_PORT_B      BIT(1)    // ??
#define SPDIF_OUT_PORT_C      BIT(2)   //?
#define SPDIF_OUT_PORT_D      BIT(3)   //?


#define SPDIF_CHANNEL_NUMBER      2
#define SPDIF_DATA_DAM_LEN  96
#define SPDIF_INFO_LEN      6

#define INFO_VALIDITY_BIT     BIT(0)
#define INFO_USER_DATA_BIT    BIT(1)
#define INFO_CHANNEL_STA_BIT  BIT(2)
#define INFO_PARITY_BIT       BIT(3)


#define SS_START	  (JL_SS->CON |= BIT(1))
#define SS_EN_ISR     (JL_SS->CON |= BIT(2))
#define D_PND_CLR	  (JL_SS->CON |= BIT(3))
#define D_PND         (JL_SS->CON &  BIT(4))
#define I_PND_CLR     (JL_SS->CON |= BIT(5))
#define I_PND         (JL_SS->CON &  BIT(6))
#define ERR_CLR       (JL_SS->CON |= BIT(7))
#define CSB_PND_CLR   (JL_SS->DMA_CON |= BIT(13))
#define CSB_PND       (JL_SS->DMA_CON & BIT(14))


#define IS_PND        (JL_SS->IO_CON & BIT(17))
#define IS_PND_CLR    (JL_SS->IO_CON | BIT(16))
#define ET_PND        (JL_SS->IO_CON & BIT(19))
#define ET_PND_CLR    (JL_SS->IO_CON | BIT(18))


#define USING_BUF_INDEX       (JL_SS->CON & BIT(12))
#define USING_INF_BUF_INDEX   (JL_SS->CON & BIT(13))
#define ERROR_VALUE           ((JL_SS->CON &(BIT(8)|BIT(9)|BIT(10)|BIT(11)))>>8)

#define SYNCWORD1 0xF872
#define SYNCWORD2 0x4E1F
#define BURST_HEADER_SZIE 0x8

enum IEC61937DataType {
    IEC61937_AC3                 = 0x01,
    IEC61937_MPEG1_LAYER1        = 0x04,
    IEC61937_MPEG1_LAYER23       = 0x05,
    IEC61937_MPEG2_EXT           = 0x06,
    IEC61937_MPEG2_AAC           = 0x07,
    IEC61937_MPEG2_LAYER1_LSF    = 0x08,
    IEC61937_MPEG2_LAYER2_LSF    = 0x09,
    IEC61937_MPEG2_LAYER3_LSF    = 0x0A,
    IEC61937_DTS1                = 0x0B,
    IEC61937_DTS2                = 0x0C,
    IEC61937_DTS3                = 0x0D,
    IEC61937_ATRAC               = 0x0E,
    IEC61937_ATRAC3              = 0x0F,
    IEC61937_ATRACX              = 0x10,
    IEC61937_DTSHD               = 0x11,
    IEC61937_WMAPRO              = 0x12,
    IEC61937_MPEG2_AAC_LSF_2048  = 0x13,
    IEC61937_MPEG2_AAC_LSF_4096  = 0x14,
    IEC61937_EAC3                = 0x15,
};

struct audio_spdif_hdl {
	u8 status;
	OS_SEM sem;
	u8 channel;
	u32 sample_rate;
	volatile u8  validity_bit_flag;
	volatile u8 error_flag;
	// s32(*p_info_buf)[SPDIF_INFO_LEN];
	void *p_spdif_info_buf[2];
	void *p_spdif_data_buf[2];
	// u16 output_buf_len;
	u8 input_port;
	u8 output_port;
	u8 find_sync_word;
	u16 decoder_type;
	int length_code;
};

void audio_spdif_slave_init(struct audio_spdif_hdl *spdif_hdl);
void audio_spdif_slave_open(struct audio_spdif_hdl *spdif_hdl);
void audio_spdif_slave_start(struct audio_spdif_hdl *spdif_hdl);
void audio_spdif_slave_close(struct audio_spdif_hdl *spdif_hdl);
int audio_spdif_slave_switch_port(struct audio_spdif_hdl *spdif_hdl, u8 port);
#endif
