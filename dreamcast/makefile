#----------------------
# | makefile
# | Description: KallistiOS build of the Legend of Mana Dreamcast port: converts the PS1 disc, compiles src/, the shared port/ sources and the decomp's main executable, links lom.elf with dreamcast.ld, links the CHECKPS and TITLE overlays against it and packs everything into lom.cdi.
# | Author: suinevere
# | Dependencies: $(KOS_BASE)/Makefile.rules, pre.makefile, dreamcast.ld, src, ../port, ../src, ../include, ../port/tools/overlay_link.py, ../port/tools/gen_pending.py, cd
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------

#----------------------
# | TARGET
# | Description: Output ELF binary name linked by kos-cc.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
TARGET = lom.elf

#----------------------
# | SOURCES
# | Description: Every C source under src except src/overlays, whose wrappers compile into the overlays instead of main.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SOURCES = $(shell find src -name '*.c' -not -path 'src/overlays/*')

#----------------------
# | KOS_SOURCES
# | Description: The platform sources that include KallistiOS headers, compiled without the decomp prefix because its PS1 types and macros would collide with KallistiOS's.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
KOS_SOURCES = src/main.c src/boot/overlay_loader_dc.c src/input/pad_dc.c src/system/dc_exception.c src/system/dc_system.c src/system/vblank_dc.c

#----------------------
# | PORT_SOURCES
# | Description: The shared port/ sources the Dreamcast build uses.
# | Author: suinevere
# | Dependencies: ../port/boot, ../port/input, ../port/system, ../port/psyq, ../port/stubs
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PORT_SOURCES = ../port/boot/overlay_header.c ../port/boot/resource_index.c ../port/boot/layout_pack.c ../port/input/pad_publish.c \
	../port/system/vsync_pacer.c ../port/psyq/gte_saturn.c ../port/psyq/libgte_tables.c ../port/stubs/main_stubs.c

#----------------------
# | DECOMP_MAIN
# | Description: The decomp main-executable files the Saturn main link compiles (saturn/src/mainexe/*_unit.c), compiled here by path.
# | Author: suinevere
# | Dependencies: saturn/src/mainexe
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DECOMP_MAIN = ../src/main/main.c ../src/main/field_runtime.c ../src/main/screen_transition.c

#----------------------
# | DECOMP_CHECKPS
# | Description: The decomp CHECKPS files saturn/src/checkps/*_unit.c compile; init.c goes through src/overlays/checkps_init_unit.c for its render-state size check.
# | Author: suinevere
# | Dependencies: saturn/src/checkps, src/overlays/checkps_init_unit.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DECOMP_CHECKPS = ../src/overlays/checkps/cdrom.c ../src/overlays/checkps/font.c ../src/overlays/checkps/kanji.c ../src/overlays/checkps/pattern.c

#----------------------
# | DECOMP_TITLE
# | Description: The decomp TITLE files saturn/src/title/*_unit.c compile; the Dreamcast keeps title.c's own song loaders.
# | Author: suinevere
# | Dependencies: saturn/src/title
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DECOMP_TITLE = ../src/overlays/title/title.c ../src/overlays/title/title_save.c

#----------------------
# | OBJS
# | Description: lom.elf's objects: src/ under build/, port/ under build/port/, the decomp main executable under build/decomp/, and the generated pending stubs.
# | Author: suinevere
# | Dependencies: SOURCES, PORT_SOURCES, DECOMP_MAIN
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DECOMP_OBJ = $(patsubst ../src/%.c,build/decomp/%.o,$(1))
MAIN_OBJS = $(patsubst src/%.c,build/%.o,$(SOURCES)) $(patsubst ../port/%.c,build/port/%.o,$(PORT_SOURCES)) $(call DECOMP_OBJ,$(DECOMP_MAIN))
OBJS = $(MAIN_OBJS) build/gen/dc_pending.o
CHECKPS_OBJS = build/ovl/checkps_vectors.o $(call DECOMP_OBJ,$(DECOMP_CHECKPS)) build/overlays/checkps_init_unit.o
TITLE_OBJS = build/ovl/title_vectors.o $(call DECOMP_OBJ,$(DECOMP_TITLE))

#----------------------
# | CFLAGS
# | Description: US region and Dreamcast-target defines, plus an include path for every src subfolder, the port/ folders, the decomp's include and the generated gen/ headers.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
CFLAGS += -std=gnu11 -O2 -Wall -DVERSION_US -DLOM_DREAMCAST $(patsubst %,-I%,$(shell find src -type d)) -I../port/boot -I../port/input -I../port/system -I../include -Igen

#----------------------
# | DECOMP_FLAGS / DECOMP_STD
# | Description: The decomp prefix and include paths (the Psy-Q headers after newlib's, so its <strings.h> keeps libc's prototypes), and the Saturn DECOMP_STD warning set minus -Wreturn-mismatch, which GCC 13 lacks, for every source that sees PS1 types.
# | Author: suinevere
# | Dependencies: ../port/psyq/lom_prefix.h, saturn/makefile (DECOMP_STD)
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DECOMP_FLAGS = -idirafter ../include/sdk -I../src -I../port/psyq -include ../port/psyq/lom_prefix.h
DECOMP_STD = -fno-builtin -Wno-builtin-declaration-mismatch -Wno-error=incompatible-pointer-types

include $(KOS_BASE)/Makefile.rules

#----------------------
# | KOS_LDFLAGS
# | Description: KallistiOS's link flags with its shlelf.xc swapped for dreamcast.ld, exported so kos-cc links with the overlay arena.
# | Author: suinevere
# | Dependencies: dreamcast.ld
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
export KOS_LDFLAGS := $(filter-out -T%,$(KOS_LDFLAGS)) -Tdreamcast.ld

#----------------------
# | KOS_CDROM_PATCH_DIR / KOS_CDROM_PATCH_MEMBERS / KOS_CDROM_PATCH_OBJS / KOS_LIBKALLISTI
# | Description: The decomp's cdrom_init (src/cd/cdrom_dc.c) collides by name with libkallisti's own GD-ROM bring-up; these are local, renamed copies of the two libkallisti members that define or call it, linked ahead of -lkallisti so KallistiOS's real hardware init keeps running under its own symbol instead of being shadowed by ours.
# | Author: suinevere
# | Dependencies: src/cd/cdrom_dc.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
KOS_CDROM_PATCH_DIR = build/kos_patch
KOS_CDROM_PATCH_MEMBERS = cdrom.o hardware.o
KOS_CDROM_PATCH_OBJS = $(patsubst %,$(KOS_CDROM_PATCH_DIR)/%,$(KOS_CDROM_PATCH_MEMBERS))
KOS_LIBKALLISTI = $(KOS_BASE)/lib/dreamcast/libkallisti.a

#----------------------
# | TOOLCHAIN_LIBS
# | Description: The libc, libm and libgcc archives main links, which the pending generator counts as defined.
# | Author: suinevere
# | Dependencies: kos-cc
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
TOOLCHAIN_LIBS = $(shell kos-cc -print-file-name=libc.a) $(shell kos-cc -print-file-name=libm.a) $(shell kos-cc -print-libgcc-file-name)

.PHONY: all clean pending

#----------------------
# | all
# | Description: Builds the bootable disc image.
# | Author: suinevere
# | Dependencies: lom.cdi
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
all: lom.cdi

include pre.makefile

$(MAIN_OBJS) $(CHECKPS_OBJS) $(TITLE_OBJS): | pre_build

build/%.o: src/%.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) $(DECOMP_FLAGS) $(DECOMP_STD) -c $< -o $@

$(patsubst src/%.c,build/%.o,$(filter-out src/main.c,$(KOS_SOURCES))): build/%.o: src/%.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) -c $< -o $@

#----------------------
# | build/main.o
# | Description: Compiles main.c, then renames its KOS_INIT_FLAGS reference to KallistiOS's cdrom_init (from <dc/cdrom.h>, included through kos.h) to kos_real_cdrom_init, so it binds to the real GD-ROM bring-up and not to the decomp's identically-named cdrom_init (src/cd/cdrom_dc.c); main.c therefore never calls the decomp's cdrom_init itself.
# | Author: suinevere
# | Dependencies: sh-elf-objcopy, src/cd/cdrom_dc.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
build/main.o: src/main.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) -c $< -o $@
	sh-elf-objcopy --redefine-sym _cdrom_init=_kos_real_cdrom_init $@

build/port/%.o: ../port/%.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) $(DECOMP_FLAGS) $(DECOMP_STD) -c $< -o $@

#----------------------
# | build/port/psyq/gte_saturn.o
# | Description: Strict C11 hides newlib's BSD <strings.h> from gte_saturn.c's <string.h>, whose bcopy prototype clashes with the Psy-Q one lom_prefix.h declares.
# | Author: suinevere
# | Dependencies: ../port/psyq/gte_saturn.c, ../port/psyq/lom_prefix.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
build/port/psyq/gte_saturn.o: DECOMP_STD += -std=c11

build/decomp/%.o: ../src/%.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) $(DECOMP_FLAGS) $(DECOMP_STD) -c $< -o $@

build/ovl/%_vectors.o: gen/%_vectors.s
	@mkdir -p $(dir $@)
	kos-cc -c $< -o $@

build/gen/dc_pending.o: gen/dc_pending.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) -c $< -o $@

#----------------------
# | $(KOS_CDROM_PATCH_DIR)/.stamp
# | Description: Extracts the two colliding libkallisti members and renames their cdrom_init symbol to kos_real_cdrom_init, so our decomp cdrom_init stays the only cdrom_init in the final link.
# | Author: suinevere
# | Dependencies: sh-elf-ar, sh-elf-objcopy, $(KOS_LIBKALLISTI)
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(KOS_CDROM_PATCH_DIR)/.stamp: $(KOS_LIBKALLISTI)
	@mkdir -p $(KOS_CDROM_PATCH_DIR)
	cd $(KOS_CDROM_PATCH_DIR) && sh-elf-ar x $(abspath $(KOS_LIBKALLISTI)) $(KOS_CDROM_PATCH_MEMBERS)
	for o in $(KOS_CDROM_PATCH_MEMBERS); do sh-elf-objcopy --redefine-sym _cdrom_init=_kos_real_cdrom_init $(KOS_CDROM_PATCH_DIR)/$$o; done
	touch $@

$(KOS_CDROM_PATCH_OBJS): $(KOS_CDROM_PATCH_DIR)/.stamp

#----------------------
# | $(TARGET)
# | Description: Links lom.elf; gen/dc_pending.ld (pending data aliases) and build/ovl/overlay_externs.ld ride along as implicit linker scripts, and the overlay entry script relinks it when the overlays move.
# | Author: suinevere
# | Dependencies: OBJS, KOS_CDROM_PATCH_OBJS, dreamcast.ld, gen/dc_pending.ld, gen/overlay_entries.ld, build/ovl/overlay_externs.ld
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(TARGET): $(OBJS) $(KOS_CDROM_PATCH_OBJS) dreamcast.ld gen/dc_pending.ld gen/overlay_entries.ld build/ovl/overlay_externs.ld
	kos-cc -o $@ $(OBJS) $(KOS_CDROM_PATCH_OBJS) gen/dc_pending.ld build/ovl/overlay_externs.ld

#----------------------
# | build/ovl/overlay_externs.ld
# | Description: EXTERN()s every symbol CHECKPS and TITLE reference but neither they nor their data scripts define, so --gc-sections keeps it in lom.elf and libc helpers only an overlay calls get linked into main.
# | Author: suinevere
# | Dependencies: sh-elf-nm, CHECKPS_OBJS, TITLE_OBJS, gen/checkps_symbols.ld, gen/title_symbols.ld
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
build/ovl/overlay_externs.ld: $(CHECKPS_OBJS) $(TITLE_OBJS) gen/checkps_symbols.ld gen/title_symbols.ld
	@mkdir -p $(dir $@)
	{ sh-elf-nm -g --defined-only $(CHECKPS_OBJS) $(TITLE_OBJS) | awk 'NF == 3 { print $$3 }'; \
	  awk '$$2 == "=" { print $$1 }' gen/checkps_symbols.ld gen/title_symbols.ld; } | sort -u > build/ovl/overlay_defined.txt
	sh-elf-nm -u $(CHECKPS_OBJS) $(TITLE_OBJS) | awk 'NF == 2 { print $$2 }' | sort -u \
		| comm -23 - build/ovl/overlay_defined.txt | sed 's/.*/EXTERN(&)/' > $@

#----------------------
# | cd/data/OVL/OVL15C.BIN / cd/data/OVL/OVL04C.BIN
# | Description: Links CHECKPS and TITLE at their arena code regions against lom.elf and packs each OVLnnC.BIN.
# | Author: suinevere
# | Dependencies: ../port/tools/overlay_link.py, gen/checkps.ld, gen/title.ld
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
cd/data/OVL/OVL15C.BIN: $(TARGET) $(CHECKPS_OBJS) gen/checkps.ld
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link checkps $(TARGET) build/ovl $@ $(CHECKPS_OBJS)

cd/data/OVL/OVL04C.BIN: $(TARGET) $(TITLE_OBJS) gen/title.ld
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link title $(TARGET) build/ovl $@ $(TITLE_OBJS)

#----------------------
# | lom.cdi
# | Description: Packs the linked ELF and cd/data's contents into a bootable MIL-CD image, so KallistiOS sees them under /cd (overlays in /cd/OVL), after pre_build has refreshed cd/data and both overlays are packed.
# | Author: suinevere
# | Dependencies: lom.elf, pre_build, mkdcdisc, cd, cd/data/OVL/OVL15C.BIN, cd/data/OVL/OVL04C.BIN
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
lom.cdi: $(TARGET) pre_build cd/data/OVL/OVL15C.BIN cd/data/OVL/OVL04C.BIN
	mkdcdisc -e $(TARGET) -o lom.cdi -D cd/data -n "Legend of Mana" --allow-overwrite

#----------------------
# | clean
# | Description: Removes build output, the linked ELF, the packed overlay code files and the disc image.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
clean:
	rm -rf build
	rm -f lom.elf lom.cdi cd/data/OVL/OVL*C.BIN

#----------------------
# | PENDING_DEFINED
# | Description: What defines symbols outside the compiled objects: the toolchain and patched KallistiOS libraries and the linker scripts.
# | Author: suinevere
# | Dependencies: TOOLCHAIN_LIBS, KOS_LIBKALLISTI, KOS_CDROM_PATCH_OBJS, dreamcast.ld, gen/*.ld
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PENDING_DEFINED = $(KOS_LIBKALLISTI) $(KOS_CDROM_PATCH_OBJS) $(TOOLCHAIN_LIBS) dreamcast.ld gen/overlay_entries.ld gen/checkps_symbols.ld gen/title_symbols.ld

#----------------------
# | pending
# | Description: Compiles main, CHECKPS and TITLE, then regenerates gen/dc_pending.c (halting stubs) and gen/dc_pending.ld (data aliases) for everything they reference that nothing defines.
# | Author: suinevere
# | Dependencies: ../port/tools/gen_pending.py, sh-elf-nm, ../config/us/symbols/*.txt, PENDING_DEFINED
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
pending: pre_build $(MAIN_OBJS) $(CHECKPS_OBJS) $(TITLE_OBJS) $(KOS_CDROM_PATCH_OBJS)
	$(PYTHON) ../port/tools/gen_pending.py --nm sh-elf-nm --overlay dreamcast \
		--objects $(MAIN_OBJS) $(CHECKPS_OBJS) $(TITLE_OBJS) \
		--symbols $(wildcard ../config/us/symbols/*.txt) \
		--defined $(PENDING_DEFINED) \
		--alias-defined gen/checkps_symbols.ld gen/title_symbols.ld \
		--out-c gen/dc_pending.c --out-ld gen/dc_pending.ld

#----------------------
# | gen/dc_pending.c / gen/dc_pending.ld
# | Description: Generated by the pending recipe when missing, since dreamcast/gen is not committed; make pending refreshes them.
# | Author: suinevere
# | Dependencies: pending
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
gen/dc_pending.c: | pre_build
	$(MAKE) pending

gen/dc_pending.ld: gen/dc_pending.c ;
