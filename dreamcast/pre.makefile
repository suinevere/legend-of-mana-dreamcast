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

.PHONY: pre_build

#----------------------
# | pre_build
# | Description: Runs the disc converter for the Dreamcast target, whose --out is the project directory holding cd/data and gen, writes gen's overlay entries, regions header, vectors and linker scripts, then extracts the libgte tables gte_saturn.c reads.
# | Author: suinevere
# | Dependencies: PYTHON, LOM_PS1_IMAGE, ../port/tools/convert_disc.py, ../port/tools/overlay_link.py, ../port/tools/libgte_tables.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
pre_build:
	$(PYTHON) ../port/tools/convert_disc.py --target dreamcast --image "$(LOM_PS1_IMAGE)" --out .
	$(PYTHON) ../port/tools/overlay_link.py --target dreamcast gen --out gen
	$(PYTHON) ../port/tools/libgte_tables.py --image "$(LOM_PS1_IMAGE)" --out .
