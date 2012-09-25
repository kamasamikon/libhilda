export CLOUD_PRJ_ROOT = ../../..
-include $(CLOUD_PRJ_ROOT)/Makefile.defs

SUB_OBJS_hilda = \
				 $(CLOUD_PRJ_ROOT)/algorithm/crc16.o \
				 $(CLOUD_PRJ_ROOT)/algorithm/crc32.o \
				 $(CLOUD_PRJ_ROOT)/algorithm/dataqueue.o \
				 $(CLOUD_PRJ_ROOT)/algorithm/md5sum.o \
				 $(CLOUD_PRJ_ROOT)/core/cnode.o \
				 $(CLOUD_PRJ_ROOT)/core/ftfs.o \
				 $(CLOUD_PRJ_ROOT)/core/hget.o \
				 $(CLOUD_PRJ_ROOT)/core/karg.o \
				 $(CLOUD_PRJ_ROOT)/core/kcfg.o \
				 $(CLOUD_PRJ_ROOT)/core/klog.o \
				 $(CLOUD_PRJ_ROOT)/core/kmem.o \
				 $(CLOUD_PRJ_ROOT)/core/kmodu.o \
				 $(CLOUD_PRJ_ROOT)/core/kmque.o \
				 $(CLOUD_PRJ_ROOT)/core/kstr.o \
				 $(CLOUD_PRJ_ROOT)/core/netif.o \
				 $(CLOUD_PRJ_ROOT)/core/opt.o \
				 $(CLOUD_PRJ_ROOT)/core/opt-rpc-client.o \
				 $(CLOUD_PRJ_ROOT)/core/opt-rpc-common.o \
				 $(CLOUD_PRJ_ROOT)/core/opt-rpc-server.o \
				 $(CLOUD_PRJ_ROOT)/core/pflock.o \
				 $(CLOUD_PRJ_ROOT)/core/sdlist.o \
				 $(CLOUD_PRJ_ROOT)/core/trace.o \
				 $(CLOUD_PRJ_ROOT)/core/xmldoc.o \
				 $(CLOUD_PRJ_ROOT)/xtcool/linux.o 

LOCAL_CFLAGS += -DMEM_STAT

LDFLAGS += -lexpat

LOCAL_INCDIRS = -I $(CLOUD_PRJ_ROOT)/inc
LOCAL_OUT_OBJS = $(SUB_OBJS_hilda)
LOCAL_OUT_LIB = hilda.a
LOCAL_SHARE_LIB = libhilda.so

.PHONY: all clean

all: BUILD_SUB_DIRS $(LOCAL_OUT_LIB) $(LOCAL_SHARE_LIB)

-include $(CLOUD_PRJ_ROOT)/Makefile.rules
