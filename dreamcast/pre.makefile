#----------------------
# | pre.makefile
# | Description: Converts the user's PS1 disc image into the Dreamcast cd/data overlay, resource and index files, gen/*_symbols.ld and the gen/libgte_tables blob, and generates the overlay link scripts, before compiling.
# | Author: suinevere
# | Dependencies: ../port/tools/convert_disc.py, ../port/tools/overlay_link.py, ../port/tools/libgte_tables.py, ../disc/us/LegendOfMana.bin or LOM_PS1_IMAGE
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------

#----------------------
# | PYTHON
# | Description: Interpreter that runs the converter tools.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PYTHON ?= python3

#----------------------
# | LOM_PS1_IMAGE
# | Description: The user's PS1 disc image the converter reads.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
LOM_PS1_IMAGE ?= ../disc/us/LegendOfMana.bin

#----------------------
# | PRE_BUILD_STAMP
# | Description: File the converter touches once it finishes, so a `make` with none of PRE_BUILD_INPUTS changed sees it as up to date and skips the recipe below.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PRE_BUILD_STAMP = gen/.pre_build.stamp

#----------------------
# | PRE_BUILD_INPUTS
# | Description: Everything that changes what the converter produces: the user's PS1 image, every script under port/tools (convert_disc.py's own imports and its helpers).
# |              Also every repo-root tools/splat_ext module convert_disc.py's own sys.path.insert reaches: decompress.py and any splat_ext sibling that could shadow a port/tools module of the same name.
# |              And every overlay symbol spec it reads.
# | Author: suinevere
# | Dependencies: LOM_PS1_IMAGE, ../port/tools, ../tools/splat_ext, ../config/us/symbols
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
PRE_BUILD_INPUTS = $(LOM_PS1_IMAGE) $(wildcard ../port/tools/*.py) $(wildcard ../tools/splat_ext/*.py) $(wildcard ../config/us/symbols/*.txt)

.PHONY: pre_build

#----------------------
# | $(PRE_BUILD_STAMP)
# | Description: Runs the disc converter for the Dreamcast target, whose --out is the project directory holding cd/data and gen, writes gen's overlay entries, regions header, vectors and linker scripts.
# |              Then extracts the libgte tables gte_saturn.c reads; only reruns when PRE_BUILD_INPUTS changed.
# | Author: suinevere
# | Dependencies: PYTHON, PRE_BUILD_INPUTS, ../port/tools/convert_disc.py, ../port/tools/overlay_link.py, ../port/tools/libgte_tables.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
$(PRE_BUILD_STAMP): $(PRE_BUILD_INPUTS)
	$(PYTHON) ../port/tools/convert_disc.py --target dreamcast --image "$(LOM_PS1_IMAGE)" --out .
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast gen --out gen
	$(PYTHON) ../port/tools/libgte_tables.py --image "$(LOM_PS1_IMAGE)" --out .
	@mkdir -p $(dir $@)
	@touch $@

#----------------------
# | pre_build
# | Description: Convenience alias so `make pre_build` still works by name; every other target depends on $(PRE_BUILD_STAMP) directly.
# | Author: suinevere
# | Dependencies: PRE_BUILD_STAMP
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
pre_build: $(PRE_BUILD_STAMP)
