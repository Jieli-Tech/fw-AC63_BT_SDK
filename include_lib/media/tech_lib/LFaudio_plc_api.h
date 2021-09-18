#ifndef _LFaudio_PLC_API_H
#define _LFaudio_PLC_API_H


typedef struct _LFaudio_PLC_API {
    unsigned int (*need_buf)();
    void (*open)(unsigned char *ptr, int nch, int mode);
    int (*run)(unsigned char *ptr, short *inbuf, short *obuf, short len, short err_flag); //len是按多少个点的，inbuf跟obuf可以同址的
} LFaudio_PLC_API;


extern LFaudio_PLC_API *get_lfaudioPLC_api();

#endif

#if 0

#define  PACKET_LEN         30
#define  PACKET_FILL_VAL    0x1500

/*调用示例*/
{
    int bufsize;
    unsigned char *bufptr;
    BT15_REPAIR_API *test_repair = get_repair_api();

    bufsize = test_repair->need_buf(PACKET_LEN);
    bufptr = malloc(bufsize);                           //开辟空间
    test_repair->open(bufptr, PACKET_LEN);              //传入参数,参数1是buf地址，参数2是包长度

    while (1) {
        fread(inbuf, 2, PACKET_LEN, fpin);
        if (feof(fpin)) {
            break;    //input data
        }

        test_repair->run(bufptr, inbuf, outbuf, PACKET_LEN, err);   //err=1是差错帧，要不然为0

        fwrite(outbuf, 2, PACKET_LEN, fpout);                       //output data

    }

    free(bufptr);

}
#endif
