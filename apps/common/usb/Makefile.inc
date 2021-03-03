ifeq ($(CONFIG_USB_ENABLE),y)
#usb slave
objs += \
	$(ROOT)/apps/common/usb/usb_config.o \
	$(ROOT)/apps/common/usb/device/descriptor.o \
	$(ROOT)/apps/common/usb/device/usb_device.o \
	$(ROOT)/apps/common/usb/device/user_setup.o \
	$(ROOT)/apps/common/usb/device/task_pc.o \

#mass storage
objs += \
	$(ROOT)/apps/common/usb/device/msd.o \
	$(ROOT)/apps/common/usb/device/msd_upgrade.o \

#hid
objs += \
	$(ROOT)/apps/common/usb/device/hid.o \

#audio
objs += \
	$(ROOT)/apps/common/usb/device/uac1.o \
	$(ROOT)/apps/common/usb/device/uac_stream.o \

#usb host
objs += \
	$(ROOT)/apps/common/usb/usb_host_config.o \
	$(ROOT)/apps/common/usb/host/usb_bulk_transfer.o \
	$(ROOT)/apps/common/usb/host/usb_ctrl_transfer.o \
	$(ROOT)/apps/common/usb/host/usb_host.o \

#udisk
objs += \
	$(ROOT)/apps/common/usb/host/usb_storage.o

#gamepad
objs += \
	$(ROOT)/apps/common/usb/host/adb.o \
    $(ROOT)/apps/common/usb/host/aoa.o \
    $(ROOT)/apps/common/usb/host/hid.o \

#usb host audio
objs += \
    $(ROOT)/apps/common/usb/host/audio.o \
    $(ROOT)/apps/common/usb/host/audio_demo.o \

endif

includes += \
	-I$(ROOT)/apps/common/usb/ \
	-I$(ROOT)/apps/common/usb/device \
	-I$(ROOT)/apps/common/usb/host \

