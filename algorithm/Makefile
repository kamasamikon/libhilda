export CLOUD_PRJ_ROOT = ../..
-include $(CLOUD_PRJ_ROOT)/Makefile.defs

LOCAL_CFLAGS += -Werror

SUB_OBJS_crc32 = ./crc32.o
SUB_OBJS_crc16 = ./crc16.o
SUB_OBJS_dataqueue = ./dataqueue.o

LOCAL_INCDIRS = -I ../inc
LOCAL_OUT_OBJS = $(SUB_OBJS_crc32) $(SUB_OBJS_crc16) $(SUB_OBJS_sdlist) $(SUB_OBJS_dataqueue)
LOCAL_OUT_LIB = algorithm_lib.a

.PHONY: all clean

all:BUILD_SUB_DIRS $(LOCAL_OUT_LIB)

-include $(CLOUD_PRJ_ROOT)/Makefile.rules

