
includes += \
	-I$(ROOT)/apps/$(APP_CASE)/include/ \
	-I$(ROOT)/apps/$(APP_CASE)/board/$(CPU)/ \
	-I$(ROOT)/apps/common/ \
	-I$(ROOT)/apps/common/include/ \
	-I$(ROOT)/apps/$(APP_CASE)/board/$(CPU)/ \
	-I$(ROOT)/apps/common/third_party_profile/jieli/ \
	-I$(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp \
	-I$(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/rcsp_updata \
	-I$(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/bt_trans_data \
	-I$(ROOT)/apps/common/third_party_profile/jieli/trans_data_demo \
	-I$(ROOT)/apps/common/third_party_profile/common/ \
	-I$(ROOT)/apps/common/usb/ \
    -I$(ROOT)/include_lib/ai_stack/JL_rcsp/ \
    -I$(ROOT)/include_lib/btstack/ \

# 添加外设
includes += \
	-I$(ROOT)/apps/common/code_switch/include/ \
	-I$(ROOT)/apps/common/device/optical_mouse_sensor/include/ \
	-I$(ROOT)/apps/common/device/optical_mouse_sensor/include/hal3205 \
	-I$(ROOT)/apps/common/touch_pad \

objs += \
	$(ROOT)/apps/$(APP_CASE)/lib_config/lib_btctrler_config.o \
	$(ROOT)/apps/$(APP_CASE)/lib_config/lib_driver_config.o \
	$(ROOT)/apps/$(APP_CASE)/lib_config/lib_system_config.o \
	$(ROOT)/apps/$(APP_CASE)/lib_config/lib_profile_config.o \
	$(ROOT)/apps/$(APP_CASE)/lib_config/lib_update_config.o \

objs += \
	$(ROOT)/apps/$(APP_CASE)/app_main.o \
	$(ROOT)/apps/$(APP_CASE)/app_idle.o \
	$(ROOT)/apps/$(APP_CASE)/misc.o \
	$(ROOT)/apps/$(APP_CASE)/version.o \
	$(ROOT)/apps/$(APP_CASE)/log_config.o \
	$(ROOT)/apps/$(APP_CASE)/user_cfg.o \
	$(ROOT)/apps/$(APP_CASE)/rtc_alarm.o \
	$(ROOT)/apps/common/third_party_profile/common/custom_cfg.o \
    $(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/rcsp_bluetooth.o \
    $(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/rcsp_updata/rcsp_user_update.o \
	$(ROOT)/apps/common/third_party_profile/jieli/JL_rcsp/rcsp_updata/rcsp_ch_loader_download.o \
	$(ROOT)/apps/common/update/update.o \
	$(ROOT)/apps/common/third_party_profile/jieli/hid_user.o \

objs += \
	$(ROOT)/apps/common/code_switch/code_switch.o \
	$(ROOT)/apps/common/device/optical_mouse_sensor/OMSensor_manage.o \
	$(ROOT)/apps/common/device/optical_mouse_sensor/hal3205/hal3205.o \
	$(ROOT)/apps/common/key/matrix_keyboard.o \
	$(ROOT)/apps/common/key/iokey.o \
	$(ROOT)/apps/common/key/adkey.o \
    $(ROOT)/apps/common/key/key_driver.o \
    $(ROOT)/apps/common/touch_pad/SYD9557M.o \

# ble demo
objs += \
	$(ROOT)/apps/common/third_party_profile/jieli/le_hogp.o \
#	$(ROOT)/apps/common/ble/mesh_onoff_app.o \

# user case
objs += \
	$(ROOT)/apps/$(APP_CASE)/app_keyboard.o \
	$(ROOT)/apps/$(APP_CASE)/app_mouse.o \
	$(ROOT)/apps/$(APP_CASE)/app_keyfob.o \
	$(ROOT)/apps/$(APP_CASE)/app_standard_keyboard.o \
	$(ROOT)/apps/$(APP_CASE)/app_keypage.o \

