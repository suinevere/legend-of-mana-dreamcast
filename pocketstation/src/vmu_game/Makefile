#----------------------
# | Makefile
# | Description: Builds the --call test image and callimg.bin with R2's pet save, the real GAME, link maps, gen_ram.py hw's equates and JSON, tables.s with tables.txt, and the test fixtures; the two images go through vmu_relax.py, which shrinks far calls and jumps that reach. Each rule deletes its output first and checks it exists, as Waterbear exits 0 on failure; the game rule also fails past flash bank 0's 64 KB.
# | Author: suinevere
# | Dependencies: callimg.s, callbase.inc, game.s, game.inc, routines.inc, *.s, ../vmu_rt/*.s, ../vmu_rt/*.inc, ../game/ram_map.txt, ../../tools/gen_ram.py, ../../tools/gen_tables.py, ../../tools/vmu_map.py, ../../tools/vmu_flash.py, ../../tools/vmu_relax.py, ../../tests/asm/{stray,wrong_hw,poison,entry_cy,undeclared}.s, dreamcast/build-test/cd-data/RES/05/RES1506.BIN
# | Globals: N/A
# | Params: target -- call (default: callimg.vms, callimg.map, callimg.bin, hw.json), game (rrl.vms, rrl.map), fixtures, clean
# | Returns: N/A
#----------------------
BUILD      := ../../build/vmu_game
TESTS      := ../../tests/asm
WATERBEAR  := waterbear assemble
MAP        := python3 ../../tools/vmu_map.py
GEN_RAM    := python3 ../../tools/gen_ram.py
GEN_TABLES := python3 ../../tools/gen_tables.py
VMU_FLASH  := python3 ../../tools/vmu_flash.py
RELAX      := cd ../../.. && python3 pocketstation/tools/vmu_relax.py
MIRROR     := pocketstation/build/vmu_game/relaxed
RES1506    := ../../../dreamcast/build-test/cd-data/RES/05/RES1506.BIN
RT_SRC     := $(wildcard ../vmu_rt/*.s ../vmu_rt/*.inc)
GAME_SRC   := callbase.inc game.inc routines.inc $(wildcard *.s) $(BUILD)/tables.s
FIXTURES   := stray wrong_hw poison entry_cy undeclared
HEADER_AREA := 1152
FLASH_BANK  := 65536

call: $(BUILD)/callimg.vms $(BUILD)/callimg.map $(BUILD)/callimg.bin $(BUILD)/hw.json

game: $(BUILD)/rrl.vms $(BUILD)/rrl.map

fixtures: $(foreach f,$(FIXTURES),$(BUILD)/test/$(f).vms $(BUILD)/test/$(f).map) $(BUILD)/hw.json

$(BUILD)/rrl_ram.inc: ../game/ram_map.txt ../../tools/gen_ram.py
	mkdir -p $(BUILD)
	rm -f $@ $(BUILD)/hw.json
	$(GEN_RAM) hw ../game/ram_map.txt $@ $(BUILD)/hw.json
	test -s $@
	test -s $(BUILD)/hw.json

$(BUILD)/hw.json: $(BUILD)/rrl_ram.inc
	test -s $@

$(BUILD)/tables.s: $(RES1506) ../../tools/gen_tables.py
	mkdir -p $(BUILD)
	rm -f $@ $(BUILD)/tables.txt
	$(GEN_TABLES) $(RES1506) $@ $(BUILD)/tables.txt
	test -s $@
	test -s $(BUILD)/tables.txt

$(BUILD)/callimg.vms: callimg.s $(GAME_SRC) $(RT_SRC) $(BUILD)/rrl_ram.inc ../../tools/vmu_relax.py
	rm -f $@
	$(RELAX) pocketstation/src/vmu_game/callimg.s $(MIRROR)/callimg pocketstation/build/vmu_game/callimg.vms
	test -s $@

$(BUILD)/callimg.bin: $(BUILD)/callimg.vms $(RES1506) ../../tools/vmu_flash.py
	rm -f $@
	$(VMU_FLASH) $@ $< --res $(RES1506) --name CALLIMG
	test -s $@

$(BUILD)/callimg.map: $(BUILD)/callimg.vms ../../tools/vmu_map.py
	rm -f $@
	$(MAP) $(BUILD)/relaxed/callimg/pocketstation/src/vmu_game/callimg.s $< $@ $(BUILD)/map_probe
	test -s $@

$(BUILD)/rrl.vms: game.s $(GAME_SRC) $(RT_SRC) $(BUILD)/rrl_ram.inc ../../tools/vmu_relax.py
	rm -f $@
	$(RELAX) pocketstation/src/vmu_game/game.s $(MIRROR)/game pocketstation/build/vmu_game/rrl.vms
	test -s $@
	size=$$(wc -c < $@); \
	content=$$(( size - $(HEADER_AREA) )); \
	limit=$$(( $(FLASH_BANK) - $(HEADER_AREA) )); \
	echo "rrl.vms: $$size bytes, $$content past the header, limit $$limit"; \
	if [ $$content -gt $$limit ]; then \
		echo "rrl.vms outgrows flash bank 0: $$content bytes past the header, over the $$limit budget" >&2; \
		rm -f $@; \
		exit 1; \
	fi

$(BUILD)/rrl.map: $(BUILD)/rrl.vms ../../tools/vmu_map.py
	rm -f $@
	$(MAP) $(BUILD)/relaxed/game/pocketstation/src/vmu_game/game.s $< $@ $(BUILD)/map_probe_game
	test -s $@

$(BUILD)/test/%.vms: $(TESTS)/%.s $(GAME_SRC) $(RT_SRC) $(BUILD)/rrl_ram.inc
	mkdir -p $(BUILD)/test
	rm -f $@
	$(WATERBEAR) $< -o $@
	test -s $@

$(BUILD)/test/%.map: $(BUILD)/test/%.vms ../../tools/vmu_map.py
	rm -f $@
	$(MAP) $(TESTS)/$*.s $< $@ $(BUILD)/test/map_probe_$*
	test -s $@

clean:
	rm -rf $(BUILD)

.PHONY: call game fixtures clean
