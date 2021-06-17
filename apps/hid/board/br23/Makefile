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
   $(AR_DIR)/aec.a \
   $(AR_DIR)/media.a \
   $(AR_DIR)/media_app.a \
   $(AR_DIR)/libAptFilt_pi32v2_OnChip.a \
   $(AR_DIR)/libEchoSuppress_pi32v2_OnChip.a \
   $(AR_DIR)/libNoiseSuppress_pi32v2_OnChip.a \
   $(AR_DIR)/libSplittingFilter_pi32v2_OnChip.a \
   $(AR_DIR)/libFFT_pi32v2_OnChip.a \
   $(AR_DIR)/libOpcore_maskrom_pi32v2_OnChip.a \
   $(AR_DIR)/lib_resample_cal.a \
   $(AR_DIR)/opus_enc_lib.a \
   $(AR_DIR)/speex_enc_lib.a \
   $(AR_DIR)/bfilterfun_lib.a \
   $(AR_DIR)/wtg_dec_lib.a \
   $(AR_DIR)/wtgv2_dec_lib.a \
   $(AR_DIR)/wav_dec_lib.a \
--end-group


ifeq ($(MIDI_DEC_ADDR),y)
LIBS += \
   $(AR_DIR)/lib_midi_dec_addr.a \

else
LIBS += \
   $(AR_DIR)/lib_midi_dec.a \

endif


endif

LIBS += \
   $(AR_DIR)/update.a \


objs += \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac6351d_keyboard.o \
	  $(ROOT)/apps/$(APP_CASE)/board/$(CPU)/board_ac635n_demo.o \
