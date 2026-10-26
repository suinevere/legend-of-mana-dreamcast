#----------------------
# | Makefile
# | Description: Fetches and patches the pinned PocketStation core, then builds the OpenPSK launcher ROM, the reference runner, the structured game's runners and the VMU-shaped game's runners, the recording one included.
# | Author: suinevere
# | Dependencies: kernel/Makefile, runner/Makefile, runner/core.patch
# | Globals: N/A
# | Params: target -- all (default), vendor, kernel, runner, host, game, game-host, game-alone, game-reads, vmu-host, vmu-counts, vmu-record, clean
# | Returns: N/A
#----------------------
CORE_URL    := https://github.com/pcsx-redux/pocketstation.git
CORE_COMMIT := a18d0a3b342c11117f50189432bb2b2dca834b3c
VENDOR      := build/vendor/pocketstation
STAMP       := build/vendor/.patched-$(CORE_COMMIT)

all: kernel runner

$(VENDOR)/pocketstation.h:
	mkdir -p build/vendor
	git clone -q $(CORE_URL) $(VENDOR)
	git -C $(VENDOR) -c advice.detachedHead=false checkout -q $(CORE_COMMIT)

$(STAMP): $(VENDOR)/pocketstation.h runner/core.patch
	git -C $(VENDOR) checkout -q -- .
	git -C $(VENDOR) apply ../../../runner/core.patch
	touch $@

vendor: $(STAMP)

kernel: vendor
	$(MAKE) -C kernel

runner: vendor
	$(MAKE) -C runner

host:
	$(MAKE) -C runner host

GAME_SRCS = $(wildcard src/rrl/*.c src/game/*.c)
GAME_ARGS = TAG=-game HOOKS_OVERRIDE=../src/game/hooks.txt CSRCS="$(addprefix ../,$(GAME_SRCS))"

RAM_H = build/game/gen/rrl_ram.h

$(RAM_H): src/game/ram_map.txt tools/gen_ram.py
	mkdir -p build/game/gen
	python3 tools/gen_ram.py ps src/game/ram_map.txt $@

game: vendor $(RAM_H)
	$(MAKE) -C runner $(GAME_ARGS)

game-host: $(RAM_H)
	$(MAKE) -C runner host $(GAME_ARGS) HOST_BIN=../build/rrl_host-game HOST_OUT=../build/host-game

GAME_ALONE_ARGS = CSRCS="$(addprefix ../,$(wildcard src/game/*.c))"

game-alone: $(RAM_H)
	$(MAKE) -C runner game-alone $(GAME_ALONE_ARGS)

game-reads: $(RAM_H)
	$(MAKE) -C runner game-alone $(GAME_ALONE_ARGS) GAME_HOST_BIN=../build/game_host-reads GAME_HOST_OUT=../build/game-reads GAME_DEFS=-DGAME_TRACE_READS

VMU_RAM_H = build/vmu/gen/rrl_ram.h
VMU_SRCS  = $(wildcard src/vmu/*.c) runner/vmu_runner.c
VMU_CC    = cc -O2 -std=c11 -Wall -Wextra -Werror -Isrc/vmu -Ibuild/vmu/gen

$(VMU_RAM_H): src/game/ram_map.txt tools/gen_ram.py
	mkdir -p build/vmu/gen
	python3 tools/gen_ram.py vmu src/game/ram_map.txt $@

build/vmu_runner: $(VMU_RAM_H) $(VMU_SRCS) $(wildcard src/vmu/*.h)
	$(VMU_CC) -DVMU_CHECK_BOUNDS $(VMU_SRCS) -o $@

build/vmu_runner-counts: $(VMU_RAM_H) $(VMU_SRCS) $(wildcard src/vmu/*.h)
	$(VMU_CC) -DVMU_CHECK_BOUNDS -DVMU_COUNT_ACCESS $(VMU_SRCS) -o $@

REC_SRC  = build/record/src
REC_GAME = $(addprefix $(REC_SRC)/,$(notdir $(filter-out src/vmu/vmu_host.c,$(wildcard src/vmu/*.c)))) $(REC_SRC)/record_names.c
REC_CC   = $(VMU_CC) -DVMU_CHECK_BOUNDS -DVMU_RECORD -Irunner -Wl,--wrap=vmu_rom8
REC_LIB  = src/vmu/vmu_host.c runner/vmu_record.c

$(REC_SRC)/record_names.c: tools/gen_record.py tools/vmu_callgraph.py $(wildcard src/vmu/*.c src/vmu/*.h)
	rm -rf $(REC_SRC)
	python3 tools/gen_record.py src/vmu $(REC_SRC)
	test -f $@

build/vmu_runner-record: $(VMU_RAM_H) $(REC_SRC)/record_names.c runner/vmu_runner.c runner/vmu_record.c runner/vmu_record.h
	rm -f $@
	$(REC_CC) $(REC_GAME) $(REC_LIB) runner/vmu_runner.c -o $@
	test -f $@

build/record_synthetic: $(VMU_RAM_H) $(REC_SRC)/record_names.c tests/host/record_synthetic.c runner/vmu_record.c runner/vmu_record.h
	rm -f $@
	$(REC_CC) $(REC_GAME) $(REC_LIB) tests/host/record_synthetic.c -o $@
	test -f $@

vmu-host: build/vmu_runner

vmu-counts: build/vmu_runner-counts

vmu-record: build/vmu_runner-record build/record_synthetic

clean:
	$(MAKE) -C kernel clean
	$(MAKE) -C runner clean

.PHONY: all vendor kernel runner host game game-host game-alone game-reads vmu-host vmu-counts vmu-record clean
