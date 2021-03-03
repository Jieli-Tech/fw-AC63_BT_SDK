LIBS = \
--start-group \
   $(AR_DIR)/cpu.a \
   $(AR_DIR)/system.a \
   $(AR_DIR)/btstack.a \
   $(AR_DIR)/btctrler.a \
   $(AR_DIR)/crypto_toolbox_Osize.a \
   $(AR_DIR)/agreement.a \
   $(AR_DIR)/rcsp_stack.a \
   $(AR_DIR)/JL_Phone_Call.a \
   $(AR_DIR)/math.a \
--end-group

ifeq ($(AUDIO_LITE_VERSION),y)
LIBS += \
--start-group \
   $(AR_DIR)/media.a \
   $(AR_DIR)/media_app.a \
   $(AR_DIR)/opus_enc_lib.a \
   $(AR_DIR)/speex_enc_lib.a \
   $(AR_DIR)/bfilterfun_lib.a \
   $(AR_DIR)/wtg_dec_lib.a \
--end-group
endif

LIBS += \
   $(AR_DIR)/update.a \


objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac636n_demo.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6368b_dongle.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6363f_demo.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6366c_demo.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6368a_demo.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6369c_demo.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6369f_demo.o \
