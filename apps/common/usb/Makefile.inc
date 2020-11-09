ifeq ($(CONFIG_USB_ENABLE),y)
#usb slave
objs += \
	$(ROOT)/apps/common/usb/usb_config.o \
	$(ROOT)/apps/common/usb/device/descriptor.o \
	$(ROOT)/apps/common/usb/device/usb_device.o \
	$(ROOT)/apps/common/usb/device/user_setup.o \

#    $(error $(USB_SLAVE_SUPPORT_HID) $(USB_SLAVE_SUPPORT_AUDIO) $(USB_SLAVE_SUPPORT_MSD))

ifeq ($(findstring y, $(USB_SLAVE_SUPPORT_HID) $(USB_SLAVE_SUPPORT_AUDIO) $(USB_SLAVE_SUPPORT_MSD) $(USB_SLAVE_USER_STACK)), y)
objs += \
	$(ROOT)/apps/common/usb/device/task_pc.o \

endif


#mass storage
ifeq ($(USB_SLAVE_SUPPORT_MSD),y)

CC_DEFINE += -DUSB_SLAVE_SUPPORT_MSD

objs += \
	$(ROOT)/apps/common/usb/device/msd.o \
	$(ROOT)/apps/common/usb/device/msd_upgrade.o \

endif

#hid
ifeq ($(USB_SLAVE_SUPPORT_HID),y)

CC_DEFINE += -DUSB_SLAVE_SUPPORT_HID

objs += \
	$(ROOT)/apps/common/usb/device/hid.o \

endif

ifeq ($(USB_SLAVE_USER_STACK),y)
CC_DEFINE += -DUSB_SLAVE_USER_STACK

endif


#audio
ifeq ($(USB_SLAVE_SUPPORT_AUDIO),y)

CC_DEFINE += -DUSB_SLAVE_SUPPORT_AUDIO

objs += \
	$(ROOT)/apps/common/usb/device/uac1.o \
	$(ROOT)/apps/common/usb/device/uac_stream.o \

endif

#usb host

ifeq ($(findstring y, $(USB_HOST_SUPPORT_MSD) $(USB_HOST_SUPPORT_HID) $(USB_HOST_SUPPORT_ADB) $(USB_HOST_SUPPORT_AOA)), y)
objs += \
	$(ROOT)/apps/common/usb/usb_host_config.o \
	$(ROOT)/apps/common/usb/host/usb_bulk_transfer.o \
	$(ROOT)/apps/common/usb/host/usb_ctrl_transfer.o \
	$(ROOT)/apps/common/usb/host/usb_host.o \

endif

#udisk
ifeq ($(USB_HOST_SUPPORT_MSD),y)
CC_DEFINE += -DUSB_HOST_SUPPORT_MSD
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

