#
# Makefile
#
CIRCLE_STDLIB_DIR ?= ../circle-stdlib

include $(CIRCLE_STDLIB_DIR)/Config.mk


CIRCLEHOME  ?= $(CIRCLE_STDLIB_DIR)/libs/circle
NEWLIBDIR   ?= $(CIRCLE_STDLIB_DIR)/install/$(NEWLIB_ARCH)
MBEDTLS_DIR ?= $(CIRCLE_STDLIB_DIR)/libs/mbedtls

CFLAGS += \
			-I $(STDDEF_INCPATH) \
	   -I $(CIRCLE_STDLIB_DIR)/include \
	   -I $(NEWLIBDIR)/include \
	   -I $(MBEDTLS_DIR)/include

GIT_BRANCH=`git rev-parse --abbrev-ref HEAD`
GIT_HASH=`git rev-parse --short HEAD`
COMPILE_TIME=`date +'%Y-%m-%d %H:%M:%S %Z'`
CPPFLAGS += -DGIT_HASH="\"$(GIT_HASH)\"" -DCOMPILE_TIME="\"$(COMPILE_TIME)\"" -DGIT_BRANCH="\"$(GIT_BRANCH)\""


EXTRACLEAN = OLED/*.o resid/*.o OLED/*.d resid/*.d

OBJS = lowlevel_arm64.o gpio_defs.o helpers.o latch.o oled.o ./OLED/ssd1306xled.o ./OLED/num2str.o 

### MENU C64 ###
ifeq ($(kernel), menu)
CPPFLAGS += -DWITH_NET=1

CPPFLAGS += -DCOMPILE_MENU=1
OBJS += kernel_menu.o kernel_kernal.o kernel_launch.o kernel_ef.o kernel_fc3.o kernel_ar.o crt.o dirscan.o config.o kernel_rkl.o c64screen.o net.o

CPPFLAGS += -DCOMPILE_MENU_WITH_SOUND=1
OBJS += kernel_sid.o kernel_sid8.o sound.o ./resid/dac.o ./resid/filter.o ./resid/envelope.o ./resid/extfilt.o ./resid/pot.o ./resid/sid.o ./resid/version.o ./resid/voice.o ./resid/wave.o fmopl.o 
CPPFLAGS += -DUSE_VCHIQ_SOUND=$(USE_VCHIQ_SOUND) 

LIBS	= $(CIRCLEHOME)/addon/vc4/sound/libvchiqsound.a \
        $(CIRCLEHOME)/addon/vc4/vchiq/libvchiq.a \
        $(CIRCLEHOME)/addon/linux/liblinuxemu.a \
        $(CIRCLEHOME)/lib/net/libnet.a \
        $(CIRCLEHOME)/addon/wlan/hostap/wpa_supplicant/libwpa_supplicant.a \
        $(CIRCLEHOME)/addon/wlan/libwlan.a
endif

### MENU C16/+4 ###
ifeq ($(kernel), menu264)
CPPFLAGS += -DCOMPILE_MENU=1
OBJS += kernel_menu264.o kernel_launch264.o dirscan.o 264config.o kernel_ramlaunch264.o 264screen.o mygpiopinfiq.o launch264.o net.o

CPPFLAGS += -DCOMPILE_MENU_WITH_SOUND=1
OBJS += kernel_sid264.o sound.o ./resid/dac.o ./resid/filter.o ./resid/envelope.o ./resid/extfilt.o ./resid/pot.o ./resid/sid.o ./resid/version.o ./resid/voice.o ./resid/wave.o fmopl.o 
CPPFLAGS += -DUSE_VCHIQ_SOUND=$(USE_VCHIQ_SOUND) 

LIBS	= $(CIRCLEHOME)/addon/vc4/sound/libvchiqsound.a \
 	      $(CIRCLEHOME)/addon/vc4/vchiq/libvchiq.a \
        $(CIRCLEHOME)/addon/linux/liblinuxemu.a \
        $(CIRCLEHOME)/lib/net/libnet.a 
#        $(CIRCLEHOME)/addon/wlan/hostap/wpa_supplicant/libwpa_supplicant.a \
#        $(CIRCLEHOME)/addon/wlan/libwlan.a
endif

### individual Kernels for C64 ###
ifeq ($(kernel), cart)
OBJS += kernel_cart.o 
endif

ifeq ($(kernel), launch)
OBJS += kernel_launch.o 
endif

ifeq ($(kernel), kernal)
OBJS += kernel_kernal.o 
endif

ifeq ($(kernel), render)
OBJS += kernel_render.o net.o
CPPFLAGS += -DWITH_RENDER
LIBS	= $(CIRCLEHOME)/lib/net/libnet.a \
        $(CIRCLEHOME)/addon/wlan/hostap/wpa_supplicant/libwpa_supplicant.a \
        $(CIRCLEHOME)/addon/wlan/libwlan.a
endif


ifeq ($(kernel), ef)
OBJS += kernel_ef.o crt.o 
endif

ifeq ($(kernel), fc3)
CPPFLAGS += -Wl,-emainFC3
OBJS += kernel_fc3.o crt.o 
endif

ifeq ($(kernel), ar)
OBJS += kernel_ar.o crt.o 
endif

ifeq ($(kernel), ram)
OBJS += kernel_georam.o
endif

ifeq ($(kernel), sid)
OBJS += kernel_sid.o sound.o ./resid/dac.o ./resid/filter.o ./resid/envelope.o ./resid/extfilt.o ./resid/pot.o ./resid/sid.o ./resid/version.o ./resid/voice.o ./resid/wave.o fmopl.o 
endif

ifeq ($(kernel), sid)
LIBS	= $(CIRCLEHOME)/addon/vc4/sound/libvchiqsound.a \
   	      $(CIRCLEHOME)/addon/vc4/vchiq/libvchiq.a \
	      $(CIRCLEHOME)/addon/linux/liblinuxemu.a

CPPFLAGS += -DUSE_VCHIQ_SOUND=$(USE_VCHIQ_SOUND) 
endif

CPPFLAGS += -Wno-comment

CFLAGS += -DMBEDTLS_CONFIG_FILE='<circle-mbedtls/config-circle-mbedtls.h>'

include $(CIRCLEHOME)/Rules.mk

INCLUDE += \
	   -I $(CIRCLE_STDLIB_DIR)/include \
	   -I $(NEWLIBDIR)/include \
	   -I $(MBEDTLS_DIR)/include

LIBS += \
	$(NEWLIBDIR)/lib/libm.a \
	$(NEWLIBDIR)/lib/libc.a \
	$(NEWLIBDIR)/lib/libcirclenewlib.a \
	$(CIRCLE_STDLIB_DIR)/src/circle-mbedtls/libcircle-mbedtls.a \
	$(MBEDTLS_DIR)/library/libmbedtls.a \
	$(MBEDTLS_DIR)/library/libmbedx509.a \
	$(MBEDTLS_DIR)/library/libmbedcrypto.a \
	$(CIRCLEHOME)/addon/SDCard/libsdcard.a \
	$(CIRCLEHOME)/lib/usb/libusb.a \
	$(CIRCLEHOME)/lib/input/libinput.a \
	$(CIRCLEHOME)/addon/fatfs/libfatfs.a \
	$(CIRCLEHOME)/lib/fs/libfs.a \
	$(CIRCLEHOME)/lib/net/libnet.a \
	$(CIRCLEHOME)/lib/sched/libsched.a \
	$(CIRCLEHOME)/lib/libcircle.a

-include $(DEPS)
