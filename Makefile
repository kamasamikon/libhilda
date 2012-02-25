export CLOUD_PRJ_ROOT = ..
-include $(CLOUD_PRJ_ROOT)/STV_Makefile.defs

LOCAL_CFLAGS += -Werror

SUB_DIRS = src algorithm_lib networkinfo_lib parsetext_lib

MAKEDIRS += $(SUB_DIRS)

.PHONY: all clean

-include $(CLOUD_PRJ_ROOT)/STV_Makefile.rules

