export CLOUD_PRJ_ROOT = ../..
-include $(CLOUD_PRJ_ROOT)/STV_Makefile.defs

LOCAL_CFLAGS += -Werror

SUB_OBJS_parsetext = ./parsetext.o

LOCAL_INCDIRS = -I ../inc
LOCAL_OUT_OBJS = $(SUB_OBJS_parsetext)
LOCAL_OUT_LIB = parsetext.a

.PHONY: all clean

all:BUILD_SUB_DIRS $(LOCAL_OUT_LIB)

-include $(CLOUD_PRJ_ROOT)/STV_Makefile.rules

