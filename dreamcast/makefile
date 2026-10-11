#----------------------
# | makefile
# | Description: KallistiOS build of the Legend of Mana Dreamcast port: converts the PS1 disc, compiles src/, the shared port/ sources and the decomp's main executable, links lom.elf with dreamcast.ld, links the CHECKPS, TITLE, GNAME and WSEL overlays against it and packs everything into lom.cdi.
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
TARGET ?= lom.elf

#----------------------
# | OBJDIR / CDI
# | Description: Object-tree root and disc image name; test-image overrides both via a submake so the scripted build never shares objects or output with the shipping lom.cdi.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OBJDIR ?= build
CDI ?= lom.cdi

#----------------------
# | EXTRA_CFLAGS
# | Description: Extra defines test-image passes through a submake, e.g. -DLOM_PAD_SCRIPT; empty for the shipping build. Checked below (SOURCES) and folded into CFLAGS further down.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
EXTRA_CFLAGS ?=

#----------------------
# | PAD_SCRIPT_SOURCES
# | Description: Sources that only exist to run a compiled-in pad script; excluded from the shipping SOURCES below and added back only when EXTRA_CFLAGS defines LOM_PAD_SCRIPT (test-image).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PAD_SCRIPT_SOURCES = src/input/pad_script.c src/input/pad_dc_script_map.c

#----------------------
# | SOURCES
# | Description: Every C source under src except src/overlays (whose wrappers compile into the overlays instead of main) and PAD_SCRIPT_SOURCES (added back only for test-image).
# | Author: suinevere
# | Dependencies: PAD_SCRIPT_SOURCES
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
SOURCES = $(filter-out $(PAD_SCRIPT_SOURCES),$(shell find src -name '*.c' -not -path 'src/overlays/*'))
ifneq (,$(findstring LOM_PAD_SCRIPT,$(EXTRA_CFLAGS)))
SOURCES += $(PAD_SCRIPT_SOURCES)
endif

#----------------------
# | KOS_SOURCES
# | Description: The platform sources that include KallistiOS headers, compiled without the decomp prefix because its PS1 types and macros would collide with KallistiOS's.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
KOS_SOURCES = src/main.c src/boot/overlay_loader_dc.c src/input/pad_dc.c src/system/dc_exception.c src/system/dc_system.c src/system/vblank_dc.c src/video/pvr_backend_kos.c

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
# | DECOMP_FIELD_RESIDENT
# | Description: The FIELD files whose text, input-repeat, audio and scene-reset code GNAME runs on top of; linked into main, as the Saturn links FIELD's C statically, until FIELD gets its own overlay.
# | Author: suinevere
# | Dependencies: saturn/src/field
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DECOMP_FIELD_RESIDENT = ../src/overlays/field/ui/field_text.c ../src/overlays/field/ui/field_modal_runtime.c \
	../src/overlays/field/runtime/field_audio_runtime.c ../src/overlays/field/scene/field_scene_load.c \
	../src/overlays/field/actors/field_actor_runtime.c

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
# | DECOMP_GNAME
# | Description: The decomp GNAME file saturn/src/gname/gname_unit.c compiles; the Dreamcast links it alone, so its fade globals keep their names.
# | Author: suinevere
# | Dependencies: saturn/src/gname
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DECOMP_GNAME = ../src/overlays/gname/gname.c

#----------------------
# | OBJS
# | Description: $(TARGET)'s objects: src/ under $(OBJDIR)/, port/ under $(OBJDIR)/port/, the decomp main executable and resident FIELD files under build/decomp/, and the generated pending stubs; then each overlay's objects and all of them together. WSEL compiles the decomp's wsel.c through src/overlays/wsel_unit.c.
# | Author: suinevere
# | Dependencies: SOURCES, PORT_SOURCES, DECOMP_MAIN, DECOMP_FIELD_RESIDENT, DECOMP_CHECKPS, DECOMP_TITLE, DECOMP_GNAME, src/overlays/wsel_unit.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
DECOMP_OBJ = $(patsubst ../src/%.c,build/decomp/%.o,$(1))
MAIN_OBJS = $(patsubst src/%.c,$(OBJDIR)/%.o,$(SOURCES)) $(patsubst ../port/%.c,$(OBJDIR)/port/%.o,$(PORT_SOURCES)) $(call DECOMP_OBJ,$(DECOMP_MAIN) $(DECOMP_FIELD_RESIDENT))
OBJS = $(MAIN_OBJS) $(OBJDIR)/gen/dc_pending.o
CHECKPS_OBJS = build/ovl/checkps_vectors.o $(call DECOMP_OBJ,$(DECOMP_CHECKPS)) build/overlays/checkps_init_unit.o
TITLE_OBJS = build/ovl/title_vectors.o $(call DECOMP_OBJ,$(DECOMP_TITLE))
GNAME_OBJS = build/ovl/gname_vectors.o $(call DECOMP_OBJ,$(DECOMP_GNAME))
WSEL_OBJS = build/ovl/wsel_vectors.o build/overlays/wsel_unit.o
OVERLAY_OBJS = $(CHECKPS_OBJS) $(TITLE_OBJS) $(GNAME_OBJS) $(WSEL_OBJS)

#----------------------
# | OVERLAY_SYMBOLS / OVERLAY_CODE
# | Description: The converter's per-overlay data symbol scripts, and this build's packed overlay code files.
# | Author: suinevere
# | Dependencies: PRE_BUILD_STAMP, OBJDIR
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OVERLAY_SYMBOLS = gen/checkps_symbols.ld gen/title_symbols.ld gen/gname_symbols.ld gen/wsel_symbols.ld
OVERLAY_CODE = $(OBJDIR)/ovl/OVL15C.BIN $(OBJDIR)/ovl/OVL04C.BIN $(OBJDIR)/ovl/OVL05C.BIN $(OBJDIR)/ovl/OVL14C.BIN

#----------------------
# | CFLAGS
# | Description: US region and Dreamcast-target defines, plus an include path for every src subfolder, the port/ folders, the decomp's include, the generated gen/ headers and EXTRA_CFLAGS.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
CFLAGS += -std=gnu11 -O2 -Wall -DVERSION_US -DLOM_DREAMCAST $(patsubst %,-I%,$(shell find src -type d)) -I../port/boot -I../port/input -I../port/system -I../include -Igen $(EXTRA_CFLAGS)

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

.PHONY: all clean pending test-image

#----------------------
# | all
# | Description: Builds the bootable disc image.
# | Author: suinevere
# | Dependencies: $(CDI)
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
all: $(CDI)

include pre.makefile

$(MAIN_OBJS) $(OVERLAY_OBJS): | $(PRE_BUILD_STAMP)

$(OBJDIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) $(DECOMP_FLAGS) $(DECOMP_STD) -MMD -MP -c $< -o $@

$(patsubst src/%.c,$(OBJDIR)/%.o,$(filter-out src/main.c,$(KOS_SOURCES))): $(OBJDIR)/%.o: src/%.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) -MMD -MP -c $< -o $@

#----------------------
# | build/main.o
# | Description: Compiles main.c, then renames its KOS_INIT_FLAGS reference to KallistiOS's cdrom_init (from <dc/cdrom.h>, included through kos.h) to kos_real_cdrom_init, so it binds to the real GD-ROM bring-up and not to the decomp's identically-named cdrom_init (src/cd/cdrom_dc.c); main.c therefore never calls the decomp's cdrom_init itself.
# | Author: suinevere
# | Dependencies: sh-elf-objcopy, src/cd/cdrom_dc.c
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(OBJDIR)/main.o: src/main.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) -MMD -MP -c $< -o $@
	sh-elf-objcopy --redefine-sym _cdrom_init=_kos_real_cdrom_init $@

$(OBJDIR)/port/%.o: ../port/%.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) $(DECOMP_FLAGS) $(DECOMP_STD) -MMD -MP -c $< -o $@

#----------------------
# | build/port/psyq/gte_saturn.o
# | Description: Strict C11 hides newlib's BSD <strings.h> from gte_saturn.c's <string.h>, whose bcopy prototype clashes with the Psy-Q one lom_prefix.h declares.
# | Author: suinevere
# | Dependencies: ../port/psyq/gte_saturn.c, ../port/psyq/lom_prefix.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(OBJDIR)/port/psyq/gte_saturn.o: DECOMP_STD += -std=c11

#----------------------
# | build/cd/cdrom_dc.o
# | Description: Strict C11 hides newlib's BSD <strings.h> from cdrom_dc.c's <string.h>, whose bcopy prototype clashes with the Psy-Q one lom_prefix.h declares; same fix as gte_saturn.o.
# | Author: suinevere
# | Dependencies: src/cd/cdrom_dc.c, ../port/psyq/lom_prefix.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(OBJDIR)/cd/cdrom_dc.o: DECOMP_STD += -std=c11

#----------------------
# | build/decomp/overlays/checkps/kanji.o
# | Description: kanji.c includes <strings.h> directly, so -std=c11 cannot hide its legacy bcopy the way it does for <string.h> callers (newlib still shows bcopy when __POSIX_VISIBLE is unset); raising _POSIX_C_SOURCE past 200112 hides it instead, clashing with lom_prefix.h's Psy-Q bcopy.
# | Author: suinevere
# | Dependencies: ../src/overlays/checkps/kanji.c, ../port/psyq/lom_prefix.h
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
build/decomp/overlays/checkps/kanji.o: DECOMP_STD += -D_POSIX_C_SOURCE=200809L

build/decomp/%.o: ../src/%.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) $(DECOMP_FLAGS) $(DECOMP_STD) -MMD -MP -c $< -o $@

#----------------------
# | build/overlays/%.o
# | Description: Compiles the overlay wrappers in src/overlays into build/overlays whatever OBJDIR is, since every build links the same overlay objects.
# | Author: suinevere
# | Dependencies: src/overlays
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
build/overlays/%.o: src/overlays/%.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) $(DECOMP_FLAGS) $(DECOMP_STD) -MMD -MP -c $< -o $@

build/ovl/%_vectors.o: gen/%_vectors.s
	@mkdir -p $(dir $@)
	kos-cc -c $< -o $@

$(OBJDIR)/gen/dc_pending.o: gen/dc_pending.c
	@mkdir -p $(dir $@)
	kos-cc $(CFLAGS) -MMD -MP -c $< -o $@

#----------------------
# | DEPFILES
# | Description: Pulls in every object's -MMD -MP dependency file, so editing a header these objects include (lom_prefix.h, gpu_tag.h, lom_platform_memory.h, …) rebuilds them; mirrors saturn/makefile's -include of C_OBJECTS/PORT_OBJECTS .d files.
# | Author: suinevere
# | Dependencies: OBJS, OVERLAY_OBJS
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
-include $(OBJS:.o=.d) $(OVERLAY_OBJS:.o=.d)

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
# | Description: Links lom.elf, failing on any section dreamcast.ld does not place; gen/dc_pending.ld (pending data aliases) and build/ovl/overlay_externs.ld ride along as implicit linker scripts, and the overlay entry script relinks it when the overlays move.
# | Author: suinevere
# | Dependencies: OBJS, KOS_CDROM_PATCH_OBJS, dreamcast.ld, gen/dc_pending.ld, gen/overlay_entries.ld, build/ovl/overlay_externs.ld
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(TARGET): $(OBJS) $(KOS_CDROM_PATCH_OBJS) dreamcast.ld gen/dc_pending.ld gen/overlay_entries.ld build/ovl/overlay_externs.ld
	kos-cc -o $@ $(OBJS) $(KOS_CDROM_PATCH_OBJS) gen/dc_pending.ld build/ovl/overlay_externs.ld -Wl,--orphan-handling=error

#----------------------
# | build/ovl/overlay_externs.ld
# | Description: EXTERN()s every symbol the overlays reference but none of them nor their data scripts define, so --gc-sections keeps it in lom.elf and libc helpers only an overlay calls get linked into main.
# | Author: suinevere
# | Dependencies: sh-elf-nm, OVERLAY_OBJS, OVERLAY_SYMBOLS
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
build/ovl/overlay_externs.ld: $(OVERLAY_OBJS) $(OVERLAY_SYMBOLS)
	@mkdir -p $(dir $@)
	{ sh-elf-nm -g --defined-only $(OVERLAY_OBJS) | awk 'NF == 3 { print $$3 }'; \
	  awk '$$2 == "=" { print $$1 }' $(OVERLAY_SYMBOLS); } | sort -u > build/ovl/overlay_defined.txt
	sh-elf-nm -u $(OVERLAY_OBJS) | awk 'NF == 2 { print $$2 }' | sort -u \
		| comm -23 - build/ovl/overlay_defined.txt | sed 's/.*/EXTERN(&)/' > $@

#----------------------
# | $(OBJDIR)/ovl/OVL15C.BIN / $(OBJDIR)/ovl/OVL04C.BIN / $(OBJDIR)/ovl/OVL05C.BIN / $(OBJDIR)/ovl/OVL14C.BIN
# | Description: Links CHECKPS, TITLE, GNAME and WSEL at their arena code regions against $(TARGET), per-OBJDIR so lom.elf's and lom_test.elf's EXTERNs (different addresses, since pad_dc.o/state_report_dc.o differ) never overwrite each other's packed overlay.
# | Author: suinevere
# | Dependencies: ../port/tools/overlay_link.py, gen/checkps.ld, gen/title.ld, gen/gname.ld, gen/wsel.ld
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(OBJDIR)/ovl/OVL15C.BIN: $(TARGET) $(CHECKPS_OBJS) gen/checkps.ld
	@mkdir -p $(dir $@)
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link checkps $(TARGET) $(OBJDIR)/ovl $@ $(CHECKPS_OBJS)

$(OBJDIR)/ovl/OVL04C.BIN: $(TARGET) $(TITLE_OBJS) gen/title.ld
	@mkdir -p $(dir $@)
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link title $(TARGET) $(OBJDIR)/ovl $@ $(TITLE_OBJS)

$(OBJDIR)/ovl/OVL05C.BIN: $(TARGET) $(GNAME_OBJS) gen/gname.ld
	@mkdir -p $(dir $@)
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link gname $(TARGET) $(OBJDIR)/ovl $@ $(GNAME_OBJS)

$(OBJDIR)/ovl/OVL14C.BIN: $(TARGET) $(WSEL_OBJS) gen/wsel.ld
	@mkdir -p $(dir $@)
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast link wsel $(TARGET) $(OBJDIR)/ovl $@ $(WSEL_OBJS)

#----------------------
# | $(OBJDIR)/cd-data/.stamp
# | Description: Stages a per-build copy of the shared cd/data tree (the converter's output, via $(PRE_BUILD_STAMP)) with that build's own packed overlay code laid on top, so packing never mixes one build's data with another build's overlay code.
# | Author: suinevere
# | Dependencies: PRE_BUILD_STAMP, OVERLAY_CODE
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(OBJDIR)/cd-data/.stamp: $(PRE_BUILD_STAMP) $(OVERLAY_CODE)
	rm -rf $(OBJDIR)/cd-data
	cp -a cd/data $(OBJDIR)/cd-data
	cp -f $(OVERLAY_CODE) $(OBJDIR)/cd-data/OVL/
	touch $@

#----------------------
# | $(CDI)
# | Description: Packs the linked ELF and this build's staged cd-data tree into a bootable MIL-CD image, so KallistiOS sees them under /cd (overlays in /cd/OVL); -N drops mkdcdisc's data-track padding, which only speeds up loading on real GD-ROM drives and costs nothing this build needs (Flycast and GDEMU both boot the unpadded disc the same as the padded one).
# | Author: suinevere
# | Dependencies: $(TARGET), mkdcdisc, $(OBJDIR)/cd-data/.stamp
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(CDI): $(TARGET) $(OBJDIR)/cd-data/.stamp
	mkdcdisc -e $(TARGET) -o $(CDI) -D $(OBJDIR)/cd-data -n "Legend of Mana" -N --allow-overwrite

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
# | gen/pad_script_table.h
# | Description: Compiles tests/pad_scripts/new_game.pad into the pad_script_steps[] table test-image's pad_dc.o includes under LOM_PAD_SCRIPT.
# | Author: suinevere
# | Dependencies: tools/gen_pad_script.py, tests/pad_scripts/new_game.pad
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
gen/pad_script_table.h: tests/pad_scripts/new_game.pad tools/gen_pad_script.py
	@mkdir -p $(dir $@)
	$(PYTHON) tools/gen_pad_script.py tests/pad_scripts/new_game.pad $@

#----------------------
# | test-image
# | Description: Builds dreamcast/lom_test.cdi from a separate build-test/ object tree with -DLOM_PAD_SCRIPT and the compiled new_game.pad table, so the scripted build never touches the shipping lom.cdi's objects.
# | Author: suinevere
# | Dependencies: gen/pad_script_table.h, OBJDIR, TARGET, CDI, EXTRA_CFLAGS
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
test-image: gen/pad_script_table.h
	$(MAKE) OBJDIR=build-test TARGET=lom_test.elf CDI=lom_test.cdi EXTRA_CFLAGS=-DLOM_PAD_SCRIPT all

#----------------------
# | PENDING_DEFINED
# | Description: What defines symbols outside the compiled objects: the toolchain and patched KallistiOS libraries and the linker scripts.
# | Author: suinevere
# | Dependencies: TOOLCHAIN_LIBS, KOS_LIBKALLISTI, KOS_CDROM_PATCH_OBJS, dreamcast.ld, gen/*.ld
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PENDING_DEFINED = $(KOS_LIBKALLISTI) $(KOS_CDROM_PATCH_OBJS) $(TOOLCHAIN_LIBS) dreamcast.ld gen/overlay_entries.ld $(OVERLAY_SYMBOLS)

#----------------------
# | pending
# | Description: Compiles main, CHECKPS, TITLE, GNAME and WSEL, then regenerates gen/dc_pending.c (halting stubs) and gen/dc_pending.ld (data aliases) for everything they reference that nothing defines.
# | Author: suinevere
# | Dependencies: ../port/tools/gen_pending.py, sh-elf-nm, ../config/us/symbols/*.txt, PENDING_DEFINED
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
pending: $(PRE_BUILD_STAMP) $(MAIN_OBJS) $(OVERLAY_OBJS) $(KOS_CDROM_PATCH_OBJS)
	$(PYTHON) ../port/tools/gen_pending.py --nm sh-elf-nm --overlay dreamcast \
		--objects $(MAIN_OBJS) $(OVERLAY_OBJS) \
		--symbols $(wildcard ../config/us/symbols/*.txt) \
		--defined $(PENDING_DEFINED) \
		--alias-defined $(OVERLAY_SYMBOLS) \
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
gen/dc_pending.c: | $(PRE_BUILD_STAMP)
	$(MAKE) pending

gen/dc_pending.ld: gen/dc_pending.c ;
