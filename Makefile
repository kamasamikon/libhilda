
export HI_PRJ_ROOT := $(CURDIR)

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
	cp -f $(HI_PRJ_ROOT)/build/config/$(shell echo $@ | sed 's/arch.//g') $(HI_PRJ_ROOT)/.configure

.PHONY: arch.5602
arch.5602: clean
	cp -f $(HI_PRJ_ROOT)/build/config/$(shell echo $@ | sed 's/arch.//g') $(HI_PRJ_ROOT)/.configure

.PHONY: arch.marvell
arch.marvell: clean
	cp -f $(HI_PRJ_ROOT)/build/config/$(shell echo $@ | sed 's/arch.//g') $(HI_PRJ_ROOT)/.configure

.PHONY: arch.hisi
arch.hisi: clean
	cp -f $(HI_PRJ_ROOT)/build/config/$(shell echo $@ | sed 's/arch.//g') $(HI_PRJ_ROOT)/.configure

.PHONY: arch.x86
arch.x86: clean
	cp -f $(HI_PRJ_ROOT)/build/config/$(shell echo $@ | sed 's/arch.//g') $(HI_PRJ_ROOT)/.configure

.PHONY: arch.7231
arch.7231: clean
	cp -f $(HI_PRJ_ROOT)/build/config/$(shell echo $@ | sed 's/arch.//g') $(HI_PRJ_ROOT)/.configure

#########################################################################
# Architecture of target board
#
.PHONY: debug.yes
debug.yes: clean
	@sed -i "s/export FAINT_DEBUG=no/export FAINT_DEBUG=yes/g" $(HI_PRJ_ROOT)/.configure

.PHONY: debug.no
debug.no: clean
	@sed -i "s/export FAINT_DEBUG=yes/export FAINT_DEBUG=no/g" $(HI_PRJ_ROOT)/.configure

#########################################################################
# $(HI_PRJ_ROOT)/hilda
#
.PHONY: hilda
hilda: amust
	make -C $(HI_PRJ_ROOT)/build/platform/linux

.PHONY: hilda.clean
hilda.clean: amust
	make -C $(HI_PRJ_ROOT)/build/platform/linux clean

#########################################################################
# Target::install
#
.PHONY: install install.test
install: hilda
	@./build/install/${FAINT_PLATFORM}
