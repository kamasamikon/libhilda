export HI_PRJ_ROOT = ../../..
-include $(HI_PRJ_ROOT)/Makefile.defs

SUB_OBJS_hilda_core = \
				 $(HI_PRJ_ROOT)/core/karg.o \
				 $(HI_PRJ_ROOT)/core/klog.o \
				 $(HI_PRJ_ROOT)/core/klogger.o \
				 $(HI_PRJ_ROOT)/core/kmem.o \
				 $(HI_PRJ_ROOT)/core/kstr.o \
				 $(HI_PRJ_ROOT)/core/kopt.o \
				 $(HI_PRJ_ROOT)/core/kopt-rpc-common.o \
				 $(HI_PRJ_ROOT)/core/kopt-rpc-server.o \
				 $(HI_PRJ_ROOT)/core/kmisc.o \
				 $(HI_PRJ_ROOT)/core/kbuf.o \
				 $(HI_PRJ_ROOT)/core/kmque.o \
				 $(HI_PRJ_ROOT)/xtcool/linux.o 

SUB_OBJS_hilda_all = \
				 $(HI_PRJ_ROOT)/algorithm/md5sum.o \
				 $(HI_PRJ_ROOT)/core/knode.o \
				 $(HI_PRJ_ROOT)/core/ftfs.o \
				 $(HI_PRJ_ROOT)/core/karg.o \
				 $(HI_PRJ_ROOT)/core/kcfg.o \
				 $(HI_PRJ_ROOT)/core/klog.o \
				 $(HI_PRJ_ROOT)/core/klogger.o \
				 $(HI_PRJ_ROOT)/core/kmem.o \
				 $(HI_PRJ_ROOT)/core/kmodu.o \
				 $(HI_PRJ_ROOT)/core/kmque.o \
				 $(HI_PRJ_ROOT)/core/kstr.o \
				 $(HI_PRJ_ROOT)/core/netif.o \
				 $(HI_PRJ_ROOT)/core/kopt.o \
				 $(HI_PRJ_ROOT)/core/kopt-rpc-client.o \
				 $(HI_PRJ_ROOT)/core/kopt-rpc-common.o \
				 $(HI_PRJ_ROOT)/core/kopt-rpc-server.o \
				 $(HI_PRJ_ROOT)/core/pflock.o \
				 $(HI_PRJ_ROOT)/core/trace.o \
				 $(HI_PRJ_ROOT)/core/kmisc.o \
				 $(HI_PRJ_ROOT)/core/kbuf.o \
				 $(HI_PRJ_ROOT)/xtcool/linux.o 

ifeq ($(HILDA_PLATFORM),x86)
SUB_OBJS_hilda = $(SUB_OBJS_hilda_all)
# SUB_OBJS_hilda += $(HI_PRJ_ROOT)/core/xmldoc.o
# $(HI_PRJ_ROOT)/core/hget.o \
else
SUB_OBJS_hilda = $(SUB_OBJS_hilda_core)
endif

LOCAL_INCDIRS = -I $(HI_PRJ_ROOT)/inc
LOCAL_OUT_OBJS = $(SUB_OBJS_hilda)
LOCAL_OUT_LIB = libhilda.a
LOCAL_SHARE_LIB = libhilda.so

LDFLAGS += -Wl,-Map,hilda.map

.PHONY: all clean

all: BUILD_SUB_DIRS $(LOCAL_OUT_LIB) $(LOCAL_SHARE_LIB)

-include $(HI_PRJ_ROOT)/Makefile.rules
