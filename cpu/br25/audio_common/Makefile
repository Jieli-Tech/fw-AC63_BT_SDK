

ifeq ($(APP_CASE),earphone)
objs += \
	  $(ROOT)/cpu/$(CPU)/audio_common/app_audio.o \

else ifeq ($(APP_CASE),soundbox)
objs += \
	  $(ROOT)/cpu/$(CPU)/audio_common/app_audio.o \
	  $(ROOT)/cpu/$(CPU)/audio_common/audio_link.o \
	  $(ROOT)/cpu/$(CPU)/audio_common/audio_iis.o \

else ifeq ($(APP_CASE),dongle)
objs += \
#	  $(ROOT)/cpu/$(CPU)/audio_common/app_audio.o \

else ifeq ($(APP_CASE),bt_udisk)
objs += \
	  $(ROOT)/cpu/$(CPU)/audio_common/app_audio.o \
	  $(ROOT)/cpu/$(CPU)/audio_common/audio_link.o \

else ifeq ($(APP_CASE),adapter)
objs += \
	  $(ROOT)/cpu/$(CPU)/audio_common/app_audio.o \
	  $(ROOT)/cpu/$(CPU)/audio_common/audio_link.o \
	  $(ROOT)/cpu/$(CPU)/audio_common/audio_iis.o \

endif
