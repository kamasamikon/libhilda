export HI_PRJ_ROOT = ../..
-include $(HI_PRJ_ROOT)/Makefile.defs

LOCAL_CFLAGS += -Werror

SUB_OBJS_crc32 = ./crc32.o
SUB_OBJS_crc16 = ./crc16.o

LOCAL_INCDIRS = -I ../inc
LOCAL_OUT_OBJS = $(SUB_OBJS_crc32) $(SUB_OBJS_crc16) $(SUB_OBJS_sdlist)
LOCAL_OUT_LIB = algorithm_lib.a

.PHONY: all clean

all:BUILD_SUB_DIRS $(LOCAL_OUT_LIB)

-include $(HI_PRJ_ROOT)/Makefile.rules

