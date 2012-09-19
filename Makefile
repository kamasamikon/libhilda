
export CLOUD_PRJ_ROOT := $(CURDIR)
DOT_CFG := $(CLOUD_PRJ_ROOT)/.configure

.PHONY: all clean

all: hilda
clean: hilda.clean 

-include Makefile.defs

#########################################################################
# Banner: show current configuration
#
.PHONY: banner
banner:
	@echo
	@echo "***************************************************************"
	@echo "* Architecture: $(FAINT_PLATFORM)"
	@echo "* Debug: $(FAINT_DEBUG)"
	@echo "***************************************************************"
	@echo

#########################################################################
# neccesary
#
.PHONY: amust
amust: banner

#########################################################################
# Architecture of target board
#
.PHONY: arch.3602
arch.3602: clean
	@./cfgmaker.py $(CLOUD_PRJ_ROOT)/build/config/3602 $(CLOUD_PRJ_ROOT)/.configure

.PHONY: arch.5602
arch.5602: clean
	@./cfgmaker.py $(CLOUD_PRJ_ROOT)/build/config/5602 $(CLOUD_PRJ_ROOT)/.configure

.PHONY: arch.marvell
arch.marvell: clean
	@./cfgmaker.py $(CLOUD_PRJ_ROOT)/build/config/marvell $(CLOUD_PRJ_ROOT)/.configure

.PHONY: arch.hisi
arch.hisi: clean
	@./cfgmaker.py $(CLOUD_PRJ_ROOT)/build/config/hisi $(CLOUD_PRJ_ROOT)/.configure

.PHONY: arch.x86
arch.x86: clean
	@./cfgmaker.py $(CLOUD_PRJ_ROOT)/build/config/x86 $(CLOUD_PRJ_ROOT)/.configure

#########################################################################
# Architecture of target board
#
.PHONY: debug.yes
debug.yes: clean
	@sed -i "s/export FAINT_DEBUG=no/export FAINT_DEBUG=yes/g" $(CLOUD_PRJ_ROOT)/.configure

.PHONY: debug.no
debug.no: clean
	@sed -i "s/export FAINT_DEBUG=yes/export FAINT_DEBUG=no/g" $(CLOUD_PRJ_ROOT)/.configure

#########################################################################
# $(CLOUD_PRJ_ROOT)/hilda
#
.PHONY: hilda
hilda: amust
	make -C $(CLOUD_PRJ_ROOT)/build/platform/linux

.PHONY: hilda.clean
hilda.clean: amust
	make -C $(CLOUD_PRJ_ROOT)/build/platform/linux clean

#########################################################################
# Target::install
#
.PHONY: install install.test
install: hilda
	@./build/install/${FAINT_PLATFORM}
