LIBS = \
--start-group \
   $(AR_DIR)/cpu.a \
   $(AR_DIR)/system.a \
   $(AR_DIR)/btstack.a \
   $(AR_DIR)/btctrler.a \
   $(AR_DIR)/crypto_toolbox_Osize.a \
   $(AR_DIR)/agreement.a \
   $(AR_DIR)/JL_rcsp.a \
--end-group

ifeq ($(OTA_UPDATE),y)
LIBS += \
   $(AR_DIR)/update.a \

endif


objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac696x_demo.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6968_demo.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6969_demo.o \
