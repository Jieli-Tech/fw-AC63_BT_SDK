
# make 编译并下载
# make VERBOSE=1 显示编译详细过程
# make clean 清除编译临时文件

# 工具路径设置
ifeq ($(OS), Windows_NT)
# Windows 下工具链位置
TOOL_DIR := C:/JL/pi32/bin
CC    := clang.exe
CXX   := clang.exe
LD    := q32s-lto-wrapper.exe
AR    := q32s-lto-ar.exe
MKDIR := mkdir_win -p
RM    := rm -rf

SYS_LIB_DIR := C:/JL/pi32/q32s-lib
SYS_INC_DIR := C:/JL/pi32/q32s-include

## 后处理脚本
POST_SCRIPT     := ..\..\..\..\cpu\bd29\tools\download.bat
RUN_POST_SCRIPT := $(POST_SCRIPT)
else
# Linux 下工具链位置
TOOL_DIR := /opt/jieli/q32s/bin
CC    := clang
CXX   := clang++
LD    := lto-wrapper
AR    := lto-ar
MKDIR := mkdir -p
RM    := rm -rf

SYS_LIB_DIR := $(TOOL_DIR)/../lib
SYS_INC_DIR := $(TOOL_DIR)/../include

## 后处理脚本
POST_SCRIPT     := ../../../../cpu/bd29/tools/download.sh
RUN_POST_SCRIPT := bash $(POST_SCRIPT)
endif

CC  := $(TOOL_DIR)/$(CC)
CXX := $(TOOL_DIR)/$(CXX)
LD  := $(TOOL_DIR)/$(LD)
AR  := $(TOOL_DIR)/$(AR)
# 输出文件设置
OUT_ELF   := ../../../../cpu/bd29/tools/sdk.elf
OBJ_FILE  := $(OUT_ELF).objs.txt
# 编译路径设置
BUILD_DIR := objs
# 工程路径前缀
ROOT_PREFIX := ../../../..

# 编译参数设置
CFLAGS := \
	-flto \
	-target q32s \
	-integrated-as \
	-fno-builtin \
	-mllvm -inline-threshold=5 \
	-Oz \
	-integrated-as \
	-g \
	-O0 \
	-flto \
	-Os \
	-Wcast-align \
	-w \
	-fallow-pointer-null \
	-Wincompatible-pointer-types \
	-Wundef \
	-fprefer-gnu-section \
	-Wframe-larger-than=256 \
	-fms-extensions \


# 宏定义
DEFINES := \
	-DSUPPORT_MS_EXTENSIONS \
	-DCONFIG_RELEASE_ENABLE \
	-DCONFIG_CPU_BD29 \
	-DCONFIG_FREE_RTOS_ENABLE \
	-DCONFIG_MMU_ENABLE \
	-DCONFIG_MEDIA_ORIGIN_ENABLE \
	-D__GCC_Q32S__ \
	-DCONFIG_NEW_ECC_ENABLE \
	-DEVENT_HANDLER_NUM_CONFIG=2 \
	-DEVENT_TOUCH_ENABLE_CONFIG=0 \
	-DEVENT_POOL_SIZE_CONFIG=256 \
	-DCONFIG_EVENT_KEY_MAP_ENABLE=0 \
	-DTIMER_POOL_NUM_CONFIG=10 \
	-DAPP_ASYNC_POOL_NUM_CONFIG=0 \
	-DVFS_ENABLE=1 \
	-DUSE_SDFILE_NEW=1 \
	-DVFS_FILE_POOL_NUM_CONFIG=0 \
	-DSDFILE_VERSION=0x020000 \
	-DVM_MAX_SIZE_CONFIG=16*1024 \
	-DVM_ITEM_MAX_NUM=128 \
	-DCONFIG_TWS_ENABLE \
	-DCONFIG_HID_CASE_ENABLE \
	-DCONFIG_TRANSFER_ENABLE \
	-DCONFIG_LITE_AEC_ENABLE=0 \
	-DCONFIG_LMP_RESET_ENABLE \
	-DCONFIG_LMP_REFRESH_ENCRYPTION_KEY_ENABLE \
	-DCONFIG_LINK_DISTURB_SCAN_ENABLE=0 \
	-DCONFIG_BTCTRLER_TASK_DEL_ENABLE \
	-DCONFIG_UPDATA_ENABLE \
	-DCONFIG_OTA_UPDATA_ENABLE \
	-DCONFIG_ITEM_FORMAT_VM \
	-DTCFG_APP_BT_EN=1 \
	-DUSB_PC_NO_APP_MODE \
	-D__LD__ \


# 头文件搜索路径
INCLUDES := \
	-I../../../../include_lib \
	-I../../../../include_lib/driver \
	-I../../../../include_lib/driver/device \
	-I../../../../include_lib/driver/cpu/bd29 \
	-I../../../../include_lib/system \
	-I../../../../include_lib/system/generic \
	-I../../../../include_lib/system/device \
	-I../../../../include_lib/system/fs \
	-I../../../../include_lib/system/ui \
	-I../../../../include_lib/btctrler \
	-I../../../../include_lib/btctrler/port/bd29 \
	-I../../../../include_lib/update \
	-I../../../../include_lib/agreement \
	-I../../../../include_lib/btstack/third_party/common \
	-I../../../../include_lib/btstack/third_party/rcsp \
	-I../../../../include_lib/media/media_origin \
	-I../../../../include_lib/media/media_origin/media \
	-I../../../../include_lib/media/media_origin/media/cpu/bd29 \
	-I../../../../include_lib/media/media_origin/media/cpu/bd29/asm \
	-I../../../../include_lib/media/media_origin/media/aec \
	-I../../../../apps/hid/include \
	-I../../../../apps/hid/board/bd29 \
	-I../../../../apps/common \
	-I../../../../apps/common/include \
	-I../../../../apps/common/third_party_profile/jieli \
	-I../../../../apps/common/third_party_profile/jieli/JL_rcsp \
	-I../../../../apps/common/third_party_profile/jieli/JL_rcsp/rcsp_updata \
	-I../../../../apps/common/third_party_profile/jieli/JL_rcsp/bt_trans_data \
	-I../../../../apps/common/third_party_profile/jieli/trans_data_demo \
	-I../../../../apps/common/third_party_profile/common \
	-I../../../../apps/common/usb \
	-I../../../../include_lib/ai_stack/JL_rcsp \
	-I../../../../include_lib/btstack \
	-I../../../../cpu/bd29 \
	-I../../../../apps/common/code_switch/include \
	-I../../../../apps/common/device/optical_mouse_sensor/include \
	-I../../../../apps/common/touch_pad \
	-I../../../../apps/common/usb/device \
	-I../../../../apps/common/usb/host \
	-I$(SYS_INC_DIR) \


# 需要编译的 .c 文件
c_SRC_FILES := \
	../../../../apps/common/code_switch/code_switch.c \
	../../../../apps/common/debug/debug.c \
	../../../../apps/common/device/optical_mouse_sensor/OMSensor_manage.c \
	../../../../apps/common/device/optical_mouse_sensor/hal3205/hal3205.c \
	../../../../apps/common/device/optical_mouse_sensor/hal3212/hal3212.c \
	../../../../apps/common/key/adkey.c \
	../../../../apps/common/key/ctmu_touch_key.c \
	../../../../apps/common/key/iokey.c \
	../../../../apps/common/key/key_driver.c \
	../../../../apps/common/key/matrix_keyboard.c \
	../../../../apps/common/key/touch_key.c \
	../../../../apps/common/third_party_profile/common/custom_cfg.c \
	../../../../apps/common/third_party_profile/jieli/JL_rcsp/rcsp_bluetooth.c \
	../../../../apps/common/third_party_profile/jieli/JL_rcsp/rcsp_updata/rcsp_ch_loader_download.c \
	../../../../apps/common/third_party_profile/jieli/JL_rcsp/rcsp_updata/rcsp_user_update.c \
	../../../../apps/common/third_party_profile/jieli/gatt_common/le_gatt_client.c \
	../../../../apps/common/third_party_profile/jieli/gatt_common/le_gatt_common.c \
	../../../../apps/common/third_party_profile/jieli/gatt_common/le_gatt_server.c \
	../../../../apps/common/touch_pad/SYD9557M.c \
	../../../../apps/common/update/testbox_update.c \
	../../../../apps/common/update/uart_update.c \
	../../../../apps/common/update/uart_update_master.c \
	../../../../apps/common/update/update.c \
	../../../../apps/common/usb/device/cdc.c \
	../../../../apps/common/usb/device/descriptor.c \
	../../../../apps/common/usb/device/hid.c \
	../../../../apps/common/usb/device/msd.c \
	../../../../apps/common/usb/device/msd_upgrade.c \
	../../../../apps/common/usb/device/task_pc.c \
	../../../../apps/common/usb/device/uac1.c \
	../../../../apps/common/usb/device/uac_stream.c \
	../../../../apps/common/usb/device/usb_device.c \
	../../../../apps/common/usb/device/user_setup.c \
	../../../../apps/common/usb/host/adb.c \
	../../../../apps/common/usb/host/aoa.c \
	../../../../apps/common/usb/host/audio.c \
	../../../../apps/common/usb/host/audio_demo.c \
	../../../../apps/common/usb/host/hid.c \
	../../../../apps/common/usb/host/usb_bulk_transfer.c \
	../../../../apps/common/usb/host/usb_ctrl_transfer.c \
	../../../../apps/common/usb/host/usb_host.c \
	../../../../apps/common/usb/host/usb_storage.c \
	../../../../apps/common/usb/usb_config.c \
	../../../../apps/common/usb/usb_host_config.c \
	../../../../apps/hid/app_main.c \
	../../../../apps/hid/board/bd29/board_ac631n_demo.c \
	../../../../apps/hid/config/lib_btctrler_config.c \
	../../../../apps/hid/config/lib_btstack_config.c \
	../../../../apps/hid/config/lib_driver_config.c \
	../../../../apps/hid/config/lib_profile_config.c \
	../../../../apps/hid/config/lib_system_config.c \
	../../../../apps/hid/config/lib_update_config.c \
	../../../../apps/hid/config/log_config.c \
	../../../../apps/hid/examples/gamebox/app_gamebox.c \
	../../../../apps/hid/examples/gamebox/gamebox.c \
	../../../../apps/hid/examples/gamebox/key_mapping.c \
	../../../../apps/hid/examples/idle/app_idle.c \
	../../../../apps/hid/examples/keyboard/app_keyboard.c \
	../../../../apps/hid/examples/keyfob/app_keyfob.c \
	../../../../apps/hid/examples/keypage/app_keypage.c \
	../../../../apps/hid/examples/mouse_dual/app_mouse_dual.c \
	../../../../apps/hid/examples/mouse_single/app_mouse.c \
	../../../../apps/hid/examples/remote_24g_server/app_remote_24g_server.c \
	../../../../apps/hid/examples/standard_keyboard/app_standard_keyboard.c \
	../../../../apps/hid/examples/standard_keyboard/usb_hid_devices.c \
	../../../../apps/hid/examples/voice_remote_control/app_remote_control.c \
	../../../../apps/hid/modules/bt/app_comm_ble.c \
	../../../../apps/hid/modules/bt/app_comm_edr.c \
	../../../../apps/hid/modules/bt/ble_hogp.c \
	../../../../apps/hid/modules/bt/edr_hid_user.c \
	../../../../apps/hid/modules/misc.c \
	../../../../apps/hid/modules/power/app_charge.c \
	../../../../apps/hid/modules/power/app_chargestore.c \
	../../../../apps/hid/modules/power/app_power_manage.c \
	../../../../apps/hid/modules/rtc_alarm.c \
	../../../../apps/hid/modules/user_cfg.c \
	../../../../apps/hid/version.c \
	../../../../cpu/bd29/adc_api.c \
	../../../../cpu/bd29/app_timer.c \
	../../../../cpu/bd29/charge.c \
	../../../../cpu/bd29/chargestore.c \
	../../../../cpu/bd29/iic_eeprom_test.c \
	../../../../cpu/bd29/iic_hw.c \
	../../../../cpu/bd29/iic_slave_test.c \
	../../../../cpu/bd29/iic_soft.c \
	../../../../cpu/bd29/irflt.c \
	../../../../cpu/bd29/mcpwm.c \
	../../../../cpu/bd29/plcnt.c \
	../../../../cpu/bd29/pwm_led.c \
	../../../../cpu/bd29/setup.c \
	../../../../cpu/bd29/spi.c \
	../../../../cpu/bd29/spi_test.c \
	../../../../cpu/bd29/uart_dev.c \
	../../../../cpu/bd29/uart_test.c \


# 需要编译的 .S 文件
S_SRC_FILES :=


# 需要编译的 .s 文件
s_SRC_FILES :=


# 需要编译的 .cpp 文件
cpp_SRC_FILES :=


# 链接参数
LFLAGS := \
	--plugin-opt=-inline-threshold=5 \
	--plugin-opt=save-temps \
	--plugin-opt=-inline-normal-into-special-section=true \
	--plugin-opt=-dont-used-symbol-list=malloc,free,sprintf,printf,puts,putchar \
	--plugin-opt=-warn-stack-size=256 \
	--sort-common \
	--dont-complain-call-overflow \
	--start-group \
	../../../../cpu/bd29/liba/cpu.a \
	../../../../cpu/bd29/liba/system.a \
	../../../../cpu/bd29/liba/btctrler.a \
	../../../../cpu/bd29/liba/btstack.a \
	../../../../cpu/bd29/liba/update.a \
	../../../../cpu/bd29/liba/crypto_toolbox_Osize.a \
	../../../../cpu/bd29/liba/rcsp_stack.a \
	../../../../cpu/bd29/liba/agreement.a \
	--end-group \
	-T../../../../cpu/bd29/sdk.ld \
	-M=../../../../cpu/bd29/tools/sdk.map \
	-flto \
	--plugin-opt=-inline-threshold=5 \


LIBPATHS := \
	-L$(SYS_LIB_DIR) \


LIBS := \
	$(SYS_LIB_DIR)/libm.a \
	$(SYS_LIB_DIR)/libc.a \
	$(SYS_LIB_DIR)/libm.a \
	$(SYS_LIB_DIR)/libcompiler-rt.a \



c_OBJS    := $(c_SRC_FILES:%.c=%.c.o)
S_OBJS    := $(S_SRC_FILES:%.S=%.S.o)
s_OBJS    := $(s_SRC_FILES:%.s=%.s.o)
cpp_OBJS  := $(cpp_SRC_FILES:%.cpp=%.cpp.o)

OBJS      := $(c_OBJS) $(S_OBJS) $(s_OBJS) $(cpp_OBJS)
DEP_FILES := $(OBJS:%.o=%.d)


OBJS      := $(addprefix $(BUILD_DIR)/, $(OBJS:$(ROOT_PREFIX)/%=%))
DEP_FILES := $(addprefix $(BUILD_DIR)/, $(DEP_FILES:$(ROOT_PREFIX)/%=%))


VERBOSE ?= 0
ifeq ($(VERBOSE), 1)
QUITE :=
else
QUITE := @
endif

.PHONY: all clean pre_build

all: pre_build $(OUT_ELF)
	@echo +POST-BUILD
	$(QUITE) $(RUN_POST_SCRIPT) sdk

pre_build:
	@echo +PRE-BUILD
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -E -P ../../../../cpu/bd29/sdk_used_list.c -o ../../../../cpu/bd29/sdk_used_list.used
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -E -P ../../../../cpu/bd29/sdk_ld.c -o ../../../../cpu/bd29/sdk.ld
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -E -P ../../../../cpu/bd29/tools/download.c -o $(POST_SCRIPT)
	$(QUITE) ..\..\..\..\tools\utils\fixbat.exe $(POST_SCRIPT)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -E -P ../../../../cpu/bd29/tools/isd_config_rule.c -o ../../../../cpu/bd29/tools/isd_config.ini

clean:
	$(QUITE) $(RM) $(OUT_ELF)
	$(QUITE) $(RM) $(OBJS)
	$(QUITE) $(RM) $(DEP_FILES)

$(OUT_ELF): $(OBJS)
	@echo +LINK $@
	$(file >$(OBJ_FILE), $(OBJS))
	$(QUITE) $(LD) -o $(OUT_ELF) @$(OBJ_FILE) $(LFLAGS) $(LIBPATHS) $(LIBS)

$(BUILD_DIR)/%.c.o : $(ROOT_PREFIX)/%.c
	@echo +CC $<
	@$(MKDIR) $(@D)
	@$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT "$@" $< -o $(@:.o=.d)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.S.o : $(ROOT_PREFIX)/%.S
	@echo +AS $<
	@$(MKDIR) $(@D)
	@$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT "$@" $< -o $(@:.o=.d)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.s.o : $(ROOT_PREFIX)/%.s
	@echo +AS $<
	@$(MKDIR) $(@D)
	@$(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT "$@" $< -o $(@:.o=.d)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.cpp.o : $(ROOT_PREFIX)/%.cpp
	@echo +CXX $<
	@$(MKDIR) $(@D)
	@$(CXX) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT "$@" $< -o $(@:.o=.d)
	$(QUITE) $(CXX) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

-include $(DEP_FILES)
