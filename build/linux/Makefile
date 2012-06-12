export CLOUD_PRJ_ROOT = ../..
-include $(CLOUD_PRJ_ROOT)/STV_Makefile.defs

SUB_OBJS_hilda = \
../../algorithm/crc16.o \
../../algorithm/crc32.o \
../../algorithm/dataqueue.o \
../../algorithm/md5sum.o \
../../core/cnode.o \
../../core/ftfs.o \
../../core/hget.o \
../../core/karg.o \
../../core/kcfg.o \
../../core/klog.o \
../../core/kmem.o \
../../core/kmodu.o \
../../core/kmque.o \
../../core/kstr.o \
../../core/netif.o \
../../core/opt.o \
../../core/opt-rpc-client.o \
../../core/opt-rpc-common.o \
../../core/opt-rpc-server.o \
../../core/pflock.o \
../../core/sdlist.o \
../../core/trace.o \
../../xtcool/linux.o 

LOCAL_INCDIRS = -I ../../inc
LOCAL_OUT_OBJS = $(SUB_OBJS_hilda)
LOCAL_OUT_LIB = hilda.a
LOCAL_SHARE_LIB = libhilda.so

.PHONY: all clean

all: BUILD_SUB_DIRS $(LOCAL_OUT_LIB) $(LOCAL_SHARE_LIB)

-include $(CLOUD_PRJ_ROOT)/STV_Makefile.rules
