#ifndef vbass_api_h__
#define vbass_api_h__


/*open 跟 run 都是 成功 返回 RET_OK，错误返回 RET_ERR*/
/*魔音结构体*/
typedef struct __VBASS_FUNC_API_ {
    unsigned int (*need_buf)();
    int (*open)(unsigned int *ptr, int sr, int nch, int bass_f, int level);
    int (*init)(unsigned int *ptr, int bass_f, int level);
    int (*run)(unsigned int *ptr, short *inbuf, short *outbuf, int len);
} VBASS_FUNC_API;


extern VBASS_FUNC_API  *get_vbass_func_api();

#endif // reverb_api_h__
