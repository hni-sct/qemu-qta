export SHELL := /bin/bash

export AIT2QTA   := ../util/ait2qta
export QEMU_ARM  := ../qemu/bin/qemu-system-arm
export QEMU_RV32 := ../qemu/bin/qemu-system-riscv32

.PHONY: logfile_% verify_% program03.qtdb program03.pdf

PASS=\033[0;92m
FAIL=\033[0;91m

RESET=\033[0m
#RED=\033[31m
#GREEN=\033[32m
BLUE=\033[94m
WHITE=\033[39;49m

NORMAL=\033[21;24m
BOLD=\033[1m

UL=\033[0;4m

# Run tests sequentially for better readability
all:
	@$(MAKE) -s verify_program01
	@echo ''
	@$(MAKE) -s verify_program02
	@echo ''
	@$(MAKE) -s verify_program03

run_%: %.riscv32.elf
	@$(MAKE) -s $(<:.riscv32.elf=.qtdb)
	$(QEMU_RV32) -M spike -nographic -bios none -kernel $< -plugin ../libqta.so,arg=$(<:.riscv32.elf=.qtdb)

run_%: %.arm.elf %.qtdb
	@$(MAKE) -s $(<:.arm.elf=.qtdb)
	$(QEMU_ARM) -M netduino2 -nographic -kernel $< -plugin ../libqta.so,arg=$(<:.arm.elf=.qtdb)

logfile_%: %.riscv32.elf %.qtdb
	$(QEMU_RV32) -M spike -nographic -bios none -kernel $< -plugin ../libqta.so,arg=$(<:.riscv32.elf=.qtdb),arg=$(<:.riscv32.elf=.log) 2> error.log > /dev/null || (echo -e '   $(FAIL)[FAILED]$(RESET) Simulation crashed with $< and $(<:.riscv32.elf=.qtdb).' ; echo -e '            $(RESET)See error.log for further details.' ; exit 1)

logfile_%: %.arm.elf %.qtdb
	$(QEMU_ARM) -M netduino2 -nographic -kernel $< -plugin ../libqta.so,arg=$(<:.arm.elf=.qtdb),arg=$(<:.arm.elf=.log) 2> error.log > /dev/null || (echo -e '   $(FAIL)[FAILED]$(RESET) Simulation crashed with $< and $(<:.arm.elf=.qtdb).' ; echo -e '            $(RESET)See error.log for further details.' ; exit 1)

verify_%: %.ref
	@echo -e '$(RESET)$(BOLD)--------------------------------------------------------------------------------$(RESET)'
	@echo -e '$(RESET)$(BOLD) VERIFY: $(<:.ref=)$(RESET)'
	@echo -e '$(RESET)$(BOLD)--------------------------------------------------------------------------------$(RESET)'
	@$(MAKE) logfile_$(<:.ref=)
	@echo -e '   $(PASS)[PASSED]$(RESET) logfile was written to $(<:.ref=.log).'
	@sed '/^*[[:space:]]*Analysis[[:space:]]started\|duration/d' $(<:.ref=.log) | diff $< - || (echo -e '   $(FAIL)[FAILED]$(RESET) logfile $(<:.ref=.log) does not match reference $<!' ; exit 1)
	@echo -e '   $(PASS)[PASSED]$(RESET) logfile $(<:.ref=.log) matches reference $<.'
	@echo -e '$(RESET)$(BOLD)--------------------------------------------------------------------------------$(RESET)'

program03.qtdb: program03.a3report program03.pdf
	$(AIT2QTA) -i $< -o $@ 2> error.log > /dev/null || (echo -e '   $(FAIL)[FAILED]$(RESET) failed to generate time database $@ with ait2qta!'; echo -e '            $(RESET)See error.log for further details.' ; exit 1)
	@echo -e '   $(PASS)[PASSED]$(RESET) generated time database $@ with ait2qta.'

program03.pdf: program03.a3report
	$(AIT2QTA) -i $< -g $@ 2> error.log > /dev/null || (echo -e '   $(FAIL)[FAILED]$(RESET) failed to generate graphviz pdf $@ with ait2qta!'; echo -e '            $(RESET)See error.log for further details.' ; exit 1)
	@echo -e '   $(PASS)[PASSED]$(RESET) generated graphviz pdf $@ with ait2qta.'

clean:
	rm -f program03.qtdb program03.pdf *.log
