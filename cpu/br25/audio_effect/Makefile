

objs += \

ifeq ($(APP_CASE),earphone)
objs += \
	  $(ROOT)/cpu/$(CPU)/audio_effect/audio_drc.o \
	  $(ROOT)/cpu/$(CPU)/audio_effect/audio_eq.o \
	  $(ROOT)/cpu/$(CPU)/audio_effect/eq_config.o \

else ifeq ($(APP_CASE),soundbox)
objs += \
	  $(ROOT)/cpu/$(CPU)/audio_effect/audio_drc.o \
	  $(ROOT)/cpu/$(CPU)/audio_effect/audio_eq.o \
	  $(ROOT)/cpu/$(CPU)/audio_effect/audio_reverb.o \
	  $(ROOT)/cpu/$(CPU)/audio_effect/eq_config.o \
	  $(ROOT)/cpu/$(CPU)/audio_effect/audio_pitch.o \

else ifeq ($(APP_CASE),dongle)
objs += \

endif
