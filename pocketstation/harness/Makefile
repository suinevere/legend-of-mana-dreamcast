#----------------------
# | Makefile
# | Description: Patches a copy of the vendored LC86K core under build/harness/, builds vmu_harness, and assembles .s fixtures with Waterbear.
# | Author: suinevere
# | Dependencies: lc86k/cpu.c, lc86k/cpu.h, lc86k/cpu.patch, vmu_harness.c
# | Globals: N/A
# | Params: target -- all (default), clean; or ../build/asm/NAME.vms to assemble ../tests/asm/NAME.s
# | Returns: N/A
#----------------------
BUILD      := ../build/harness
OUT        := ../build/vmu_harness
STAMP      := $(BUILD)/.patched
CC         := gcc
CFLAGS     := -std=c11 -O2 -Wall -Wextra -I$(BUILD)/lc86k

# The exact command later tasks reuse to assemble an LC86K source file with Waterbear.
WATERBEAR := waterbear assemble

all: $(OUT)

$(STAMP): lc86k/cpu.c lc86k/cpu.h lc86k/cpu.patch
	mkdir -p $(BUILD)/lc86k
	cp lc86k/cpu.c lc86k/cpu.h $(BUILD)/lc86k/
	cd $(BUILD)/lc86k && patch -p0 < ../../../harness/lc86k/cpu.patch
	touch $@

$(OUT): vmu_harness.c $(STAMP)
	mkdir -p ../build
	$(CC) $(CFLAGS) vmu_harness.c $(BUILD)/lc86k/cpu.c -o $@

../build/asm/%.vms: ../tests/asm/%.s
	mkdir -p ../build/asm
	rm -f $@
	$(WATERBEAR) $< -o $@
	test -s $@

clean:
	rm -rf $(BUILD) $(OUT) ../build/asm
