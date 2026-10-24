#----------------------
# | Makefile
# | Description: Builds a shared scenario both ways: its op bytes and input events, the rt_test GAME with the scenario and scenario.py's test table built in and its link map, scenario.py's test pet payload, and scenario_host over vmu_host.c.
# | Author: suinevere
# | Dependencies: *.s, *.inc, test/rt_test.s, ../../tools/scenario.py, ../../tools/vmu_map.py, ../../tests/scenarios/*.scn, ../../tests/host/scenario_host.c, ../vmu/vmu_host.c
# | Globals: N/A
# | Params: SCN -- scenario name under tests/scenarios (default target builds it); or targets ../../build/scenario_host, ../../build/vmu_rt/tables.bin, ../../build/vmu_rt/pet.bin, clean
# | Returns: N/A
#----------------------
BUILD     := ../../build/vmu_rt
SCN_DIR   := ../../tests/scenarios
TOOL      := python3 ../../tools/scenario.py
MAP       := python3 ../../tools/vmu_map.py
WATERBEAR := waterbear assemble
RT_SRC    := sfr.inc rt.inc rt_vectors.s rt_ram.s rt_events.s rt_input.s rt_lcd.s rt_file.s rt_sound.s rt_power.s test/rt_test.s
HOST      := ../../build/scenario_host
CC        := gcc
CFLAGS    := -std=c11 -O2 -Wall -Wextra -Werror -I../vmu

ifdef SCN
all: $(BUILD)/$(SCN).vms $(BUILD)/$(SCN).inputs $(BUILD)/pet.bin
else
all: $(HOST)
endif

$(BUILD)/tables.bin: ../../tools/scenario.py
	mkdir -p $(BUILD)
	rm -f $@
	$(TOOL) table $@
	test -s $@

$(BUILD)/pet.bin: ../../tools/scenario.py
	mkdir -p $(BUILD)
	rm -f $@
	$(TOOL) pet $@
	test -s $@

$(BUILD)/%.bin: $(SCN_DIR)/%.scn ../../tools/scenario.py
	mkdir -p $(BUILD)
	$(TOOL) compile $< $@

$(BUILD)/%.inputs: $(SCN_DIR)/%.scn ../../tools/scenario.py
	mkdir -p $(BUILD)
	$(TOOL) inputs $< $@

$(BUILD)/%.vms: $(BUILD)/%.bin $(BUILD)/tables.bin $(RT_SRC) ../../tools/vmu_map.py
	cp $< $(BUILD)/scenario.bin
	rm -f $@
	$(WATERBEAR) test/rt_test.s -o $@
	test -s $@
	$(MAP) test/rt_test.s $@ $(BUILD)/$*.map $(BUILD)/map_probe

$(HOST): ../../tests/host/scenario_host.c ../vmu/vmu_host.c ../vmu/vmu_host.h ../vmu/vmu_hw.h
	mkdir -p ../../build
	$(CC) $(CFLAGS) ../../tests/host/scenario_host.c ../vmu/vmu_host.c -o $@

.PRECIOUS: $(BUILD)/%.bin

clean:
	rm -rf $(BUILD) $(HOST)
