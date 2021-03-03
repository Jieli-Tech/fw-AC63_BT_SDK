
includes += \
	-I$(ROOT)/apps/$(APP_CASE)/include/ \
	-I$(ROOT)/apps/$(APP_CASE)/board/$(CPU)/ \
	-I$(ROOT)/apps/common/ \
	-I$(ROOT)/apps/common/include/ \
	-I$(ROOT)/apps/common/power_manage/ \
	-I$(ROOT)/apps/$(APP_CASE)/board/$(CPU)/ \
	-I$(ROOT)/cpu/$(CPU)/ \
	-I$(ROOT)/apps/common/third_party_profile/jieli/ \
	-I$(ROOT)/apps/common/third_party_profile/jieli/trans_data_demo/ \
	-I$(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp \
	-I$(ROOT)/apps/common/third_party_profile/common/ \
    -I$(ROOT)/include_lib/ai_stack/JL_rcsp/ \
	-I$(ROOT)/include_lib/btstack/ \
    -I$(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/bt_trans_data/ \
    -I$(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/rcsp_updata/ \

objs += \
	$(ROOT)/apps/$(APP_CASE)/lib_config/lib_btctrler_config.o \
	$(ROOT)/apps/$(APP_CASE)/lib_config/lib_driver_config.o \
	$(ROOT)/apps/$(APP_CASE)/lib_config/lib_system_config.o \
	$(ROOT)/apps/$(APP_CASE)/lib_config/lib_profile_config.o \
	$(ROOT)/apps/$(APP_CASE)/lib_config/lib_btstack_config.o \
	$(ROOT)/apps/$(APP_CASE)/lib_config/lib_update_config.o \

objs += \
	$(ROOT)/apps/common/device/norflash/norflash.o \
	$(ROOT)/apps/$(APP_CASE)/app_main.o \
	$(ROOT)/apps/$(APP_CASE)/app_idle.o \
	$(ROOT)/apps/$(APP_CASE)/app_spp_and_le.o \
	$(ROOT)/apps/$(APP_CASE)/app_at_com.o \
	$(ROOT)/apps/$(APP_CASE)/app_dongle.o \
	$(ROOT)/apps/$(APP_CASE)/at_cmds.o \
	$(ROOT)/apps/$(APP_CASE)/at_uart.o \
	$(ROOT)/apps/$(APP_CASE)/misc.o \
	$(ROOT)/apps/$(APP_CASE)/version.o \
	$(ROOT)/apps/$(APP_CASE)/log_config.o \
	$(ROOT)/apps/$(APP_CASE)/user_cfg.o \
	$(ROOT)/apps/$(APP_CASE)/app_charge.o \
	$(ROOT)/apps/$(APP_CASE)/app_power_manage.o \
	$(ROOT)/apps/$(APP_CASE)/rtc_alarm.o \
	$(ROOT)/apps/$(APP_CASE)/usb_hid_devices.o \
	$(ROOT)/apps/common/update/update.o \
	$(ROOT)/apps/common/third_party_profile/common/custom_cfg.o \
	$(ROOT)/apps/common/third_party_profile/jieli/trans_data_demo/spp_trans_data.o \
	$(ROOT)/apps/common/third_party_profile/jieli/spp_at_com.o \
    $(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/rcsp_updata/rcsp_ch_loader_download.o \
    $(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/rcsp_bluetooth.o \
    $(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/rcsp_updata/rcsp_user_update.o \
#	$(ROOT)/apps/common/third_party_profile/common/3th_profile_api.o \
	$(ROOT)/apps/common/third_party_profile/jieli/trans_data_demo/spp_user.o \

objs += \
	$(ROOT)/apps/common/key/iokey.o \
	$(ROOT)/apps/common/key/adkey.o \
    $(ROOT)/apps/common/key/key_driver.o \
#	$(ROOT)/apps/common/code_switch/code_switch.o \

# ble demo
objs += \
	$(ROOT)/apps/common/third_party_profile/jieli/trans_data_demo/le_trans_data.o \
	$(ROOT)/apps/common/third_party_profile/jieli/le_client_demo.o \
	$(ROOT)/apps/common/third_party_profile/jieli/le_at_com.o \
	$(ROOT)/apps/common/third_party_profile/jieli/le_at_client.o \

#bt_comoon_api
objs += \
	$(ROOT)/apps/common/bt_common/bt_test_api.o \

#audio
ifeq ($(AUDIO_LITE_VERSION),y)
includes += \
	-I$(ROOT)/apps/common/audio/  \
	-I$(ROOT)/cpu/$(CPU)/audio_dec \
	-I$(ROOT)/cpu/$(CPU)/audio_enc \
	-I$(ROOT)/cpu/$(CPU) \

objs += \
	$(ROOT)/apps/$(APP_CASE)/lib_config/lib_media_config.o \
	$(ROOT)/apps/common/audio/sine_make.o \
	$(ROOT)/apps/common/audio/decode/decode.o \

ifneq ($(CPU),br30)
	objs += \
	$(ROOT)/cpu/$(CPU)/audio_dec/audio_dec_pc.o \
	$(ROOT)/cpu/$(CPU)/audio_dec/audio_usb_mic.o
endif

ifeq ($(MEDIA_NEW_ENABLE),y)
objs += \
	$(ROOT)/apps/$(APP_CASE)/tone_table.o
endif

-include $(ROOT)/apps/common/jl_kws/Makefile

endif


-include $(ROOT)/apps/common/usb/Makefile.inc
