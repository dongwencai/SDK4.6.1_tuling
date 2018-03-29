#
# Component Makefile
#

# LightDuer Module


DUER_SRC = middleware/third_party/lib_lightduer


##C_FILES += $(DUER_SRC)/src/duerapp.c             \

#C_FILES += 
#		   $(DUER_SRC)/src/duerapp_alarm.c       \
#		   $(DUER_SRC)/src/duerapp_dcs.c         \
#		   $(DUER_SRC)/src/duerapp_device_info.c \
#		   $(DUER_SRC)/src/duerapp_key.c         \
#		   $(DUER_SRC)/src/duerapp_ota.c         \
#		   $(DUER_SRC)/src/duerapp_profile_config.c \
#		   $(DUER_SRC)/src/duerapp_sdcard_config.c  \
#		   $(DUER_SRC)/src/duerapp_wifi_config.c    \
#		   $(DUER_SRC)/src/duerapp_recorder.c 
		   

#include path
CFLAGS += -I$(SOURCE_DIR)/middleware/third_party/lib_lightduer/include
CFLAGS += -I$(SOURCE_DIR)/middleware/third_party/lib_lightduer/src

LIBS += $(SOURCE_DIR)/middleware/third_party/lib_lightduer/libduer-device.a





