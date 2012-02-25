export CLOUD_PRJ_ROOT = $(CURDIR)/../..
-include $(CLOUD_PRJ_ROOT)/STV_Makefile.defs

SUB_OBJS_xtcool = sdlist.o klog.o kmem.o kmque.o opt.o opt-rpc-client.o opt-rpc-common.o opt-rpc-server.o xtcool.o cnode.o md5sum.o pflock.o netif.o ftfs.o kstr.o karg.o kcfg.o

LOCAL_INCDIRS = -I ../inc
LOCAL_OUT_OBJS = $(SUB_OBJS_xtcool)
LOCAL_OUT_LIB = sys_lib.a
LOCAL_SHARE_LIB = libsys_lib.so

.PHONY: all clean

all:BUILD_SUB_DIRS $(LOCAL_OUT_LIB) $(LOCAL_SHARE_LIB)

-include $(CLOUD_PRJ_ROOT)/STV_Makefile.rules
