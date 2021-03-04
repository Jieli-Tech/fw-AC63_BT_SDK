LIBS = \
--start-group \
   $(AR_DIR)/cpu.a \
   $(AR_DIR)/system.a \
   $(AR_DIR)/btstack.a \
   $(AR_DIR)/btctrler.a \
   $(AR_DIR)/agreement.a \
   $(AR_DIR)/rcsp_stack.a \
--end-group

ifeq ($(CRYPTO_TOOLBOX_OSIZE_IN_MASKROM),n)
LIBS += \
   $(AR_DIR)/crypto_toolbox_Osize.a \

endif
LIBS += \
   $(AR_DIR)/update.a \



objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac632n_demo.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6321a_demo.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6323a_demo.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6328a_keyfob.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6328b_demo.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6329b_demo.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6329c_demo.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6329e_demo.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6329f_demo.o \
