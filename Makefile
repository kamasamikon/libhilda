
export CLOUD_PRJ_ROOT := ${CURDIR}
DOT_CFG := ${CLOUD_PRJ_ROOT}/.configure

.PHONY: all clean

all: hilda
clean: hilda.clean 

.PHONY: mergecheck
mergecheck:
	@echo
	@echo "Close debug"
	@echo
	make debug.no
	@echo
	@echo "Check 5602"
	@echo
	make arch.5602
	make all

	@echo
	@echo "Check x86"
	@echo
	make arch.x86
	make all

	@echo
	@echo "Kongratulations, all check passed"
	@echo

-include STV_Makefile.defs

#########################################################################
# Banner: show current configuration
#
.PHONY: banner
banner:
	@echo
	@echo "***************************************************************"
	@echo "* SDK_ROOT: $(OCTEON_SDK_ROOT)"
	@echo "* SRC_ROOT: $(CAVIUM_SRC_ROOT)"
	@echo "*"
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
.PHONY: arch.x86
arch.x86: clean
	echo -e "Set default gcc to x86"
	./cfg_update arch x86

#########################################################################
# Architecture of target board
#
.PHONY: debug.yes
debug.yes: clean
	./cfg_update debug yes

.PHONY: debug.no
debug.no: clean
	./cfg_update debug no

#########################################################################
# ${CLOUD_PRJ_ROOT}/hilda
#
.PHONY: hilda
hilda: amust
	make -C ${CLOUD_PRJ_ROOT}/src

.PHONY: hilda.clean
hilda.clean: amust
	make -C ${CLOUD_PRJ_ROOT}/src clean

