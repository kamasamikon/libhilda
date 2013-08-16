export HI_PRJ_ROOT = ../../..
-include $(HI_PRJ_ROOT)/Makefile.defs

SUB_OBJS_hilda = \
				 $(HI_PRJ_ROOT)/algorithm/crc16.o \
				 $(HI_PRJ_ROOT)/algorithm/crc32.o \
				 $(HI_PRJ_ROOT)/algorithm/md5sum.o \
				 $(HI_PRJ_ROOT)/core/cnode.o \
				 $(HI_PRJ_ROOT)/core/ftfs.o \
				 $(HI_PRJ_ROOT)/core/hget.o \
				 $(HI_PRJ_ROOT)/core/karg.o \
				 $(HI_PRJ_ROOT)/core/kcfg.o \
				 $(HI_PRJ_ROOT)/core/klog.o \
				 $(HI_PRJ_ROOT)/core/kmem.o \
				 $(HI_PRJ_ROOT)/core/kmodu.o \
				 $(HI_PRJ_ROOT)/core/kmque.o \
				 $(HI_PRJ_ROOT)/core/kstr.o \
				 $(HI_PRJ_ROOT)/core/netif.o \
				 $(HI_PRJ_ROOT)/core/opt.o \
				 $(HI_PRJ_ROOT)/core/opt-rpc-client.o \
				 $(HI_PRJ_ROOT)/core/opt-rpc-common.o \
				 $(HI_PRJ_ROOT)/core/opt-rpc-server.o \
				 $(HI_PRJ_ROOT)/core/pflock.o \
				 $(HI_PRJ_ROOT)/core/sdlist.o \
				 $(HI_PRJ_ROOT)/core/trace.o \
				 $(HI_PRJ_ROOT)/core/kmisc.o \
				 $(HI_PRJ_ROOT)/core/strbuf.o \
				 $(HI_PRJ_ROOT)/xtcool/linux.o 

ifdef NEVER
SUB_OBJS_hilda += $(HI_PRJ_ROOT)/core/xmldoc.o
endif

LOCAL_INCDIRS = -I $(HI_PRJ_ROOT)/inc
LOCAL_OUT_OBJS = $(SUB_OBJS_hilda)
LOCAL_OUT_LIB = hilda.a
LOCAL_SHARE_LIB = libhilda.so

.PHONY: all clean

all: BUILD_SUB_DIRS $(LOCAL_OUT_LIB) $(LOCAL_SHARE_LIB)

-include $(HI_PRJ_ROOT)/Makefile.rules
