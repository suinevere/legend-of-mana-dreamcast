#----------------------
# | Makefile
# | Description: Fetches and patches the pinned PocketStation core, then builds the OpenPSK launcher ROM and the reference runner.
# | Author: suinevere
# | Dependencies: kernel/Makefile, runner/Makefile, runner/core.patch
# | Globals: N/A
# | Params: target -- all (default), vendor, kernel, runner, clean
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

clean:
	$(MAKE) -C kernel clean
	$(MAKE) -C runner clean

.PHONY: all vendor kernel runner host clean
