
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
.PHONY: arch.3602
arch.3602: clean
	@./cfgmaker.py ${CLOUD_PRJ_ROOT}/mkcfg/3602 ${CLOUD_PRJ_ROOT}/.configure

.PHONY: arch.5602
arch.5602: clean
	@./cfgmaker.py ${CLOUD_PRJ_ROOT}/mkcfg/5602 ${CLOUD_PRJ_ROOT}/.configure

.PHONY: arch.marvell
arch.marvell: clean
	@./cfgmaker.py ${CLOUD_PRJ_ROOT}/mkcfg/marvell ${CLOUD_PRJ_ROOT}/.configure

.PHONY: arch.x86
arch.x86: clean
	@./cfgmaker.py ${CLOUD_PRJ_ROOT}/mkcfg/x86 ${CLOUD_PRJ_ROOT}/.configure

#########################################################################
# Architecture of target board
#
.PHONY: debug.yes
debug.yes: clean
	@sed -i "s/export FAINT_DEBUG=no/export FAINT_DEBUG=yes/g" ${CLOUD_PRJ_ROOT}/.configure

.PHONY: debug.no
debug.no: clean
	@sed -i "s/export FAINT_DEBUG=yes/export FAINT_DEBUG=no/g" ${CLOUD_PRJ_ROOT}/.configure

#########################################################################
# ${CLOUD_PRJ_ROOT}/hilda
#
.PHONY: hilda
hilda: amust
	make -C ${CLOUD_PRJ_ROOT}/build/linux

.PHONY: hilda.clean
hilda.clean: amust
	make -C ${CLOUD_PRJ_ROOT}/build/linux clean

