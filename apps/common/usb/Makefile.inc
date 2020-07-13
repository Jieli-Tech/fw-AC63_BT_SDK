ifeq ($(CONFIG_USB_ENABLE),y)
#usb slave
objs += \
	$(ROOT)/apps/common/usb/usb_config.o \
	$(ROOT)/apps/common/usb/device/descriptor.o \
	$(ROOT)/apps/common/usb/device/usb_device.o \
	$(ROOT)/apps/common/usb/device/user_setup.o \
	$(ROOT)/apps/common/usb/device/task_pc.o \

#mass storage
ifeq ($(USB_SLAVE_SUPPORT_MSD),y)
objs += \
	$(ROOT)/apps/common/usb/device/msd.o \
	$(ROOT)/apps/common/usb/device/msd_upgrade.o \

endif

#hid
ifeq ($(USB_SLAVE_SUPPORT_HID),y)
objs += \
	$(ROOT)/apps/common/usb/device/hid.o \

endif



#audio
ifeq ($(USB_SLAVE_SUPPORT_AUDIO),y)
objs += \
	$(ROOT)/apps/common/usb/device/uac1.o \
	$(ROOT)/apps/common/usb/device/uac_stream.o \

endif

#usb host
objs += \
	$(ROOT)/apps/common/usb/usb_host_config.o \
	$(ROOT)/apps/common/usb/host/usb_bulk_transfer.o \
	$(ROOT)/apps/common/usb/host/usb_ctrl_transfer.o \
	$(ROOT)/apps/common/usb/host/usb_host.o \

#udisk
ifeq ($(USB_HOST_SUPPORT_MSD),y)
objs += \
	$(ROOT)/apps/common/usb/host/usb_storage.o
endif

#gamepad
ifeq ($(USB_HOST_SUPPORT_ADB),y)
objs += \
	$(ROOT)/apps/common/usb/host/adb.o \

endif
ifeq ($(USB_HOST_SUPPORT_AOA),y)
objs += \
    $(ROOT)/apps/common/usb/host/aoa.o \

endif
ifeq ($(USB_HOST_SUPPORT_HID),y)
objs += \
    $(ROOT)/apps/common/usb/host/hid.o \

endif
endif


includes += \
	-I$(ROOT)/apps/common/usb/ \
	-I$(ROOT)/apps/common/usb/device \
	-I$(ROOT)/apps/common/usb/host \

