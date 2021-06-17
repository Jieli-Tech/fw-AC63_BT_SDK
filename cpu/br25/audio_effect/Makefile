

objs += \

ifeq ($(APP_CASE),earphone)
objs += \

else ifeq ($(APP_CASE),soundbox)
objs += \
	  $(ROOT)/cpu/$(CPU)/audio_effect/eq_config.o \
	  $(ROOT)/cpu/$(CPU)/audio_effect/audio_eq_drc_demo.o \

else ifeq ($(APP_CASE),dongle)
objs += \

else ifeq ($(APP_CASE),bt_udisk)
objs += \
	  $(ROOT)/cpu/$(CPU)/audio_effect/eq_config.o \
	  $(ROOT)/cpu/$(CPU)/audio_effect/audio_eq_drc_demo.o \

else ifeq ($(APP_CASE),adapter)
objs += \
	  $(ROOT)/cpu/$(CPU)/audio_effect/eq_config.o \
	  $(ROOT)/cpu/$(CPU)/audio_effect/audio_eq_drc_demo.o \


endif
