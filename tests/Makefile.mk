export SHELL := /bin/bash

export AIT2QTA   := $(abspath ../../util/ait2qta)
export QEMU_ARM  := $(abspath ../../qemu/bin/qemu-system-arm)
export QEMU_RV32 := $(abspath ../../qemu/bin/qemu-system-riscv32)
export QEMU_LIBQTA := $(abspath ../../libqta.so)

PASS=\033[0;92m
FAIL=\033[0;91m
RESET=\033[0m
NORMAL=\033[21;24m
BOLD=\033[1m

.PHONY: logfile_% verify_% %.qtdb %.pdf

verify_%: %.ref
	@echo -e '$(RESET)$(BOLD)--------------------------------------------------------------------------------$(RESET)'
	@echo -e '$(RESET)$(BOLD) VERIFY: $(<:.ref=)$(RESET)'
	@$(MAKE) logfile_$(<:.ref=)
	@echo -e '   $(PASS)[PASSED]$(RESET) logfile was written to $(<:.ref=.log).'
	@sed '/^*[[:space:]]*Analysis[[:space:]]started\|duration/d' $(<:.ref=.log) | diff $< - || (echo -e '   $(FAIL)[FAILED]$(RESET) logfile $(<:.ref=.log) does not match reference $<!' ; exit 1)
	@echo -e '   $(PASS)[PASSED]$(RESET) logfile $(<:.ref=.log) matches reference $<.'

%.qtdb: %.a3report
	$(AIT2QTA) -i $< -o $@ > /dev/null || (echo -e '   $(FAIL)[FAILED]$(RESET) failed to generate time database $@ with ait2qta!'; exit 1)
	@echo -e '   $(PASS)[PASSED]$(RESET) generated time database $@ with ait2qta.'

%.pdf: %.a3report
	$(AIT2QTA) -i $< -g $@ > /dev/null || (echo -e '   $(FAIL)[FAILED]$(RESET) failed to generate graphviz pdf $@ with ait2qta!'; exit 1)
	@echo -e '   $(PASS)[PASSED]$(RESET) generated graphviz pdf $@ with ait2qta.'
