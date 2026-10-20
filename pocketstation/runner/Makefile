#----------------------
# | Makefile
# | Description: Builds the reference runner over the patched PocketStation core with a hook table and decompiled C.
# | Author: suinevere
# | Dependencies: ../build/vendor/pocketstation, pkrun.cc, ../src/psk, ../tools/gen_hooks.py
# | Globals: N/A
# | Params: HOOKS -- hooks.txt; HOOKS_OVERRIDE -- hooks replacing HOOKS entries; CSRCS -- C sources; TAG -- suffix for the build dir and binary; HOST_BIN, HOST_OUT -- host build outputs
# | Returns: N/A
#----------------------
CXX       ?= g++
CC        ?= gcc
CORE       = ../build/vendor/pocketstation
PSK        = ../src/psk
HOOKS     ?= ../src/rrl/hooks.txt
HOOKS_OVERRIDE ?=
CSRCS     ?= $(wildcard ../src/rrl/*.c)
TAG       ?=
OUT        = ../build/runner$(TAG)
BIN        = ../build/pkrun$(TAG)
GEN        = $(OUT)/gen/hooks.c
CXXFLAGS   = -O2 -std=c++20 -Wall -isystem $(CORE) -I$(PSK)
CFLAGS     = -O2 -std=c11 -Wall -Wextra -I$(PSK) -I../src/rrl -I../src/game -I../build/game/gen
CORE_SRCS  = $(wildcard $(CORE)/*.cc)
CORE_OBJS  = $(patsubst $(CORE)/%.cc,../build/runner/core_%.o,$(CORE_SRCS))
CORE_HDRS  = $(wildcard $(CORE)/*.h $(CORE)/cpu/*.inl $(CORE)/cpu/*/*.inl)
C_OBJS     = $(patsubst %.c,$(OUT)/c/%.o,$(notdir $(CSRCS)))

vpath %.c $(sort $(dir $(CSRCS)))

all: $(BIN)

../build/runner/core_%.o: $(CORE)/%.cc $(CORE_HDRS)
	@mkdir -p ../build/runner
	$(CXX) $(CXXFLAGS) -w -c $< -o $@

$(OUT)/pkrun.o: pkrun.cc $(CORE_HDRS) $(PSK)/psk_bus_emu.h $(PSK)/psk_hw.h
	@mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUT)/psk_bus_emu.o: $(PSK)/psk_bus_emu.cc $(CORE_HDRS) $(PSK)/psk_bus_emu.h $(PSK)/psk_hw.h $(PSK)/psk_swi.h
	@mkdir -p $(OUT)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(GEN): $(HOOKS) $(HOOKS_OVERRIDE) ../tools/gen_hooks.py
	@mkdir -p $(OUT)/gen
	python3 ../tools/gen_hooks.py $(HOOKS) $@ $(if $(HOOKS_OVERRIDE),--override $(HOOKS_OVERRIDE))

$(OUT)/hooks.o: $(GEN) $(PSK)/psk_hw.h
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/c/%.o: %.c $(wildcard $(PSK)/*.h ../src/rrl/*.h ../src/game/*.h ../build/game/gen/*.h)
	@mkdir -p $(OUT)/c
	$(CC) $(CFLAGS) -c $< -o $@

$(BIN): $(OUT)/pkrun.o $(OUT)/psk_bus_emu.o $(OUT)/hooks.o $(C_OBJS) $(CORE_OBJS)
	$(CXX) -o $@ $^

HOST_BIN  ?= ../build/rrl_host
HOST_OUT  ?= ../build/host

$(HOST_OUT)/%.o: %.c $(wildcard $(PSK)/*.h ../src/rrl/*.h ../src/game/*.h ../build/game/gen/*.h)
	@mkdir -p $(HOST_OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(HOST_OUT)/psk_host.o: $(PSK)/psk_host.c $(PSK)/psk_host.h $(PSK)/psk_hw.h $(PSK)/psk_swi.h
	@mkdir -p $(HOST_OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(HOST_OUT)/hooks.o: $(GEN) $(PSK)/psk_hw.h
	@mkdir -p $(HOST_OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(HOST_BIN): $(HOST_OUT)/rrl_host.o $(HOST_OUT)/psk_host.o $(HOST_OUT)/hooks.o $(patsubst %.c,$(HOST_OUT)/%.o,$(notdir $(CSRCS)))
	$(CC) -o $@ $^

host: $(HOST_BIN)

clean:
	rm -rf ../build/runner* ../build/pkrun* $(HOST_OUT) $(HOST_BIN)

.PHONY: all clean host
