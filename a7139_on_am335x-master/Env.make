
#LINUX_VER = 3.2.0-04.06.00.08
LINUX_VER = 3.2.0-04.06.00.11

#Cross compiler prefix
CROSS_COMPILE_DIR       = /opt/arm-arago-linux-gnueabi
export CROSS_COMPILE    = $(CROSS_COMPILE_DIR)/bin/arm-arago-linux-gnueabi-

export CC               = $(CROSS_COMPILE)gcc
export CPP              = $(CROSS_COMPILE)g++
export AR               = $(CROSS_COMPILE)ar
export AS               = $(CROSS_COMPILE)as
export LD               = $(CROSS_COMPILE)ld
export NM               = $(CROSS_COMPILE)nm
#export RANLIB          = $(CROSS_COMPILE)ranlib
export STRIP            = $(CROSS_COMPILE)strip
#export SIZE            = $(CROSS_COMPILE)size
export OBJCOPY          = $(CROSS_COMPILE)objcopy
export OBJDUMP          = $(CROSS_COMPILE)objdump

export HOSTCC           = gcc
export HOSTLD           = ld

#The directory that points to the u-boot source tree
UBOOT_DIR               = $(TOP_DIR)/u-boot-2011.09-psp04.06.00.08

#The directory that points to the Linux kernel source tree
LINUX_DIR               = $(TOP_DIR)/kernel
LINUXKERNEL_DIR         = $(LINUX_DIR)
LINUXCONFIG_DIR         = $(LINUX_DIR)

#The directory that points to the fs source tree
FS_DIR                  = $(TOP_DIR)/fs

#The directory that points to the fs
PLATFORM_DIR            = $(FS_DIR)/am335x
INSTALL_ROOTFS_DIR      = $(PLATFORM_DIR)/install_rootfs
INSTALL_DATAFS_DIR      = $(PLATFORM_DIR)/install_datafs
ROOTFS_DIR              = $(PLATFORM_DIR)/rootfs
DATAFS_DIR              = $(PLATFORM_DIR)/datafs

#The directory that points to the compiled image
IMAGE_DIR               = $(TOP_DIR)/image

#The directory that points to the tool
TOOL_DIR                = $(TOP_DIR)/tool
CONFIG_DIR              = $(TOOL_DIR)/configfile
TOOL_PACKIMAGE_DIR      = $(TOOL_DIR)/packimage
TOOL_SDCARD_UPGRADE_DIR = $(TOOL_DIR)/sdcard_upgrade

# for the extend support module
EXTEND_SUPPORT          = SOCKCLI
CFLAGS_EXTEND           = $(addprefix -D, $(EXTEND_SUPPORT))
export CFLAGS_EXTEND

# for the verbose/quite Make Message
V ?= 0
ifeq ($(V), 99)
    ECHO :=
else
    ECHO := @
endif
