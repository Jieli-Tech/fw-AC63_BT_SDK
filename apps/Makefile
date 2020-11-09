
objs += \
$(ROOT)/apps/common/debug/debug.o \

include $(ROOT)/cpu/$(CPU)/Makefile

include $(ROOT)/apps/$(APP_CASE)/Makefile

include $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/Makefile

ifeq ($(APP_CASE),earphone)
include $(ROOT)/apps/$(APP_CASE)/aec/$(CPU)/Makefile
else ifeq ($(APP_CASE),qcy_case)
include $(ROOT)/apps/$(APP_CASE)/aec/$(CPU)/Makefile
else ifeq ($(APP_CASE),soundbox)
include $(ROOT)/apps/$(APP_CASE)/aec/$(CPU)/Makefile
else ifeq ($(APP_CASE),earphone_ai)
include $(ROOT)/apps/$(APP_CASE)/aec/$(CPU)/Makefile
else ifeq ($(APP_CASE),watch)
include $(ROOT)/apps/$(APP_CASE)/aec/$(CPU)/Makefile
else ifeq ($(APP_CASE),bt_udisk)
include $(ROOT)/apps/$(APP_CASE)/aec/$(CPU)/Makefile
endif

GEN_LIB=n

-include $(MAKE_RULE)
