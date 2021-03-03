
#define     USB_MALLOC_ENABLE           0
#define     USB_HOST_ASYNC              1
#define     USB_H_MALLOC_ENABLE         1
#define     USB_DEVICE_CLASS_CONFIG (SPEAKER_CLASS|MIC_CLASS|HID_CLASS|MASSSTORAGE_CLASS)

///////////MassStorage Class

#define     MSD_BULK_EP_OUT             1
#define     MSD_BULK_EP_IN              1


#define     MAXP_SIZE_BULKOUT           64
#define     MAXP_SIZE_BULKIN            64

#define     MSD_STR_INDEX               7

///////////HID class
#define     HID_EP_IN                   2
#define     HID_EP_OUT                  2

#define     MAXP_SIZE_HIDOUT            8
#define     MAXP_SIZE_HIDIN             8
/* #define     MAXP_SIZE_HIDOUT            64 */
/* #define     MAXP_SIZE_HIDIN             64 */



/////////////Audio Class
#define     UAC_ISO_INTERVAL            1
//speaker class
#define     SPK_AUDIO_RATE              48000
#define     SPK_AUDIO_RES               16

#define     SPK_CHANNEL                 2
#define     SPK_FRAME_LEN               (((SPK_AUDIO_RATE) * SPK_AUDIO_RES / 8 * SPK_CHANNEL)/1000+4)

#define     SPK_PCM_Type                (SPK_AUDIO_RES >> 4)                // 0=8 ,1=16
#define     SPK_AUDIO_TYPE              (0x02 - SPK_PCM_Type)           // TYPE1_PCM16


#define     SPK_ISO_EP_OUT              3

#define     SPEAKER_STR_INDEX           0

#define     SPK_INPUT_TERMINAL_ID       1
#define     SPK_FEATURE_UNIT_ID         2
#define     SPK_OUTPUT_TERMINAL_ID      3

/////////////Microphone Class

#define     MIC_SamplingFrequency       1

#if MIC_SamplingFrequency   == 1

#define     MIC_AUDIO_RATE              48000

#else

#define     MIC_AUDIO_RATE              192000
#define     MIC_AUDIO_RATE_1            44100
#define     MIC_AUDIO_RATE_2            48000
#define     MIC_AUDIO_RATE_4            96000

#endif

#define     MIC_AUDIO_RES               16

#define     MIC_CHANNEL                 1

#define     MIC_FRAME_LEN               ((MIC_AUDIO_RATE * MIC_AUDIO_RES / 8 * MIC_CHANNEL)/1000)

#define     MIC_PCM_TYPE                (MIC_AUDIO_RES >> 4)                // 0=8 ,1=16
#define     MIC_AUDIO_TYPE              (0x02 - MIC_PCM_TYPE)



#define     MIC_ISO_EP_IN               3

#define     MIC_STR_INDEX               0

#define     MIC_INPUT_TERMINAL_ID       4
#define     MIC_FEATURE_UNIT_ID         5
#define     MIC_OUTPUT_TERMINAL_ID      6
#define     MIC_SELECTOR_UNIT_ID        7
