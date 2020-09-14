# apps include
includes += \
	-I$(ROOT)/apps/$(APP_CASE)/board/$(CPU) \
	-I$(ROOT)/apps/$(APP_CASE)/include \
	-I$(ROOT)/apps/common \
	-I$(ROOT)/apps/common/include \
	-I$(ROOT)/apps/common/config/include \
	-I$(ROOT)/apps/mesh/include \
	-I$(ROOT)/apps/common/third_party_profile/common/ \

# include_lib include
includes += \
	-I$(ROOT)/include_lib \
	-I$(ROOT)/include_lib/driver \
	-I$(ROOT)/include_lib/driver/device \
	-I$(ROOT)/include_lib/driver/cpu/$(CPU) \
	-I$(ROOT)/include_lib/system \
	-I$(ROOT)/include_lib/system/generic \
	-I$(ROOT)/include_lib/system/device \
	-I$(ROOT)/include_lib/system/fs \
	-I$(ROOT)/include_lib/system/os/FreeRTOS/ \
	-I$(ROOT)/include_lib/update \
	-I$(ROOT)/include_lib/btstack \
	-I$(ROOT)/include_lib/btctrler \
	-I$(ROOT)/include_lib/btctrler/port/$(CPU) \


# APP case code
objs += \
	$(ROOT)/apps/$(APP_CASE)/lib_config/lib_btctrler_config.o \
	$(ROOT)/apps/$(APP_CASE)/lib_config/lib_driver_config.o \
	$(ROOT)/apps/$(APP_CASE)/lib_config/lib_system_config.o \
	$(ROOT)/apps/$(APP_CASE)/lib_config/lib_profile_config.o \
	$(ROOT)/apps/$(APP_CASE)/lib_config/lib_update_config.o \

objs += \
	$(ROOT)/apps/$(APP_CASE)/app_mesh.o \
	$(ROOT)/apps/$(APP_CASE)/app_main.o \
	$(ROOT)/apps/$(APP_CASE)/app_idle.o \
	$(ROOT)/apps/$(APP_CASE)/version.o \
	$(ROOT)/apps/$(APP_CASE)/log_config.o \
	$(ROOT)/apps/$(APP_CASE)/user_cfg.o \
	$(ROOT)/apps/common/update/update.o \

objs += \
	$(ROOT)/apps/common/key/iokey.o \
	$(ROOT)/apps/common/key/adkey.o \
    $(ROOT)/apps/common/key/key_driver.o \

# SIG Mesh source code
MESH_C = \
	$(wildcard common/third_party_profile/sig_mesh/net/*.c) \
	$(wildcard common/third_party_profile/sig_mesh/*.c) \
	$(wildcard common/third_party_profile/sig_mesh/kernel/*.c) \
	$(wildcard common/third_party_profile/sig_mesh/api/*.c) \
	$(wildcard common/third_party_profile/sig_mesh/adaptation/*.c) \
	$(wildcard common/third_party_profile/sig_mesh/adaptation/kernel/*.c) \
	$(wildcard common/third_party_profile/sig_mesh/adaptation/ble_core/*.c) \
	common/third_party_profile/sig_mesh/tinycrypt/source/cmac_mode.c \
	common/third_party_profile/sig_mesh/tinycrypt/source/aes_encrypt.c \
	common/third_party_profile/sig_mesh/tinycrypt/source/utils.c \

# SIG Mesh source include
MESH_H = \
	-I$(abspath common/third_party_profile) \
	-I$(abspath common/third_party_profile/sig_mesh) \
	-I$(abspath common/third_party_profile/sig_mesh/tinycrypt/include) \
	-I$(abspath common/third_party_profile/sig_mesh/adaptation) \

# Mesh examples
MESH_C += \
	$(wildcard mesh/api/*.c) \
	$(wildcard mesh/examples/*.c) \

# Mesh examples include
MESH_H += \
	-I$(ROOT)/apps/mesh/api \

objs += $(patsubst %.c, %.o, $(MESH_C))

includes += $(MESH_H)

