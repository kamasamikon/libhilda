export CLOUD_PRJ_ROOT = $(CURDIR)/..
-include $(CLOUD_PRJ_ROOT)/STV_Makefile.defs

SUB_OBJS_hilda =  cnode.o ftfs.o karg.o kcfg.o klog.o kmem.o kmodu.o kmque.o kstr.o md5sum.o netif.o opt.o opt-rpc-client.o opt-rpc-common.o opt-rpc-server.o pflock.o sdlist.o trace.o xtcool.o

LOCAL_INCDIRS = -I ../inc
LOCAL_OUT_OBJS = $(SUB_OBJS_hilda)
LOCAL_OUT_LIB = hilda.a
LOCAL_SHARE_LIB = libhilda.so

.PHONY: all clean

all: BUILD_SUB_DIRS $(LOCAL_OUT_LIB) $(LOCAL_SHARE_LIB)

-include $(CLOUD_PRJ_ROOT)/STV_Makefile.rules
