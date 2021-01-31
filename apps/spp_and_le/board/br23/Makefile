LIBS = \
--start-group \
   $(AR_DIR)/cpu.a \
   $(AR_DIR)/system.a \
   $(AR_DIR)/btstack.a \
   $(AR_DIR)/btctrler.a \
   $(AR_DIR)/crypto_toolbox_Osize.a \
   $(AR_DIR)/rcsp_stack.a \
   $(AR_DIR)/agreement.a \
--end-group

ifeq ($(AUDIO_LITE_VERSION),y)
LIBS += \
--start-group \
   $(AR_DIR)/media.a \
   $(AR_DIR)/media_app.a \
--end-group
endif

LIBS += \
   $(AR_DIR)/update.a \


objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac635n_demo.o \
