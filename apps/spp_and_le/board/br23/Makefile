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

ifeq ($(OTA_UPDATE),y)
LIBS += \
   $(AR_DIR)/update.a \

endif


objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac635n_demo.o \
