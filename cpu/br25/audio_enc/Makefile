


objs += \

ifeq ($(APP_CASE),earphone)
objs += \
	  $(ROOT)/cpu/$(CPU)/audio_enc/audio_enc.o \
	  $(ROOT)/cpu/$(CPU)/audio_enc/audio_enc_file.o \
	  $(ROOT)/cpu/$(CPU)/audio_enc/audio_enc_recoder.o \

else ifeq ($(APP_CASE),soundbox)
objs += \
	  $(ROOT)/cpu/$(CPU)/audio_enc/audio_enc.o \
	  $(ROOT)/cpu/$(CPU)/audio_enc/audio_enc_file.o \
	  $(ROOT)/cpu/$(CPU)/audio_enc/audio_enc_recoder.o \
	  $(ROOT)/cpu/$(CPU)/audio_enc/audio_sbc_codec.o \
	  $(ROOT)/cpu/$(CPU)/audio_enc/audio_mic_codec.o \
	  $(ROOT)/cpu/$(CPU)/audio_enc/audio_dongle_codec.o \
	  $(ROOT)/cpu/$(CPU)/audio_enc/audio_recorder_mix.o \

else ifeq ($(APP_CASE),dongle)
objs += \

else ifeq ($(APP_CASE),bt_udisk)
objs += \
	  $(ROOT)/cpu/$(CPU)/audio_enc/audio_enc.o \
	  $(ROOT)/cpu/$(CPU)/audio_enc/audio_enc_file.o \
	  $(ROOT)/cpu/$(CPU)/audio_enc/audio_enc_recoder.o \
	  $(ROOT)/cpu/$(CPU)/audio_enc/audio_sbc_codec.o \
	  $(ROOT)/cpu/$(CPU)/audio_enc/audio_mic_codec.o \
	  $(ROOT)/cpu/$(CPU)/audio_enc/audio_dongle_codec.o \
	  $(ROOT)/cpu/$(CPU)/audio_enc/audio_recorder_mix.o \

endif
