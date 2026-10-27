#----------------------
# | sources.py
# | Description: Runs the Dreamcast converter on the user's PS1 disc and returns every disc-derived byte source the kit writes into the GDI, by name.
# | Author: suinevere
# | Dependencies: port/kit/paths.py, port/tools/{convert_disc,libgte_tables,akao_data,main_data,vmu_portraits,disc_image}.py, pocketstation/tools/gen_tables.py
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import contextlib
import io
import os

from port.kit import paths

paths.setup()

import akao_data  # noqa: E402
import convert_disc  # noqa: E402
import disc_image  # noqa: E402
import gen_tables  # noqa: E402
import libgte_tables  # noqa: E402
import main_data  # noqa: E402
import vmu_portraits  # noqa: E402

#----------------------
# | RES1506
# | Description: The converted resource Ring Ring Land's VMU GAME keeps pages of.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
RES1506 = "cd/RES/05/RES1506.BIN"


#----------------------
# | _cd_files
# | Description: Every file under work_dir/cd/data keyed "cd/" plus its upper-case /-joined path.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: work_dir -- converter output folder
# | Returns: dict
#----------------------
def _cd_files(work_dir):
    root = os.path.join(work_dir, "cd", "data")
    out = {}
    for folder, _, names in os.walk(root):
        for name in names:
            full = os.path.join(folder, name)
            rel = os.path.relpath(full, root).replace(os.sep, "/").upper()
            with open(full, "rb") as handle:
                out["cd/" + rel] = handle.read()
    return out


#----------------------
# | _quiet
# | Description: Runs a converter command line with its output captured, raising RuntimeError with its last error line if it fails.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: step -- what the step is called in the error, main -- the tool's main, argv -- its arguments
# | Returns: N/A
#----------------------
def _quiet(step, main, argv):
    out, err = io.StringIO(), io.StringIO()
    with contextlib.redirect_stdout(out), contextlib.redirect_stderr(err):
        code = main(argv)
    if code:
        detail = (err.getvalue().strip().splitlines() or ["exit code %d" % code])[-1]
        raise RuntimeError("%s failed: %s" % (step, detail))


#----------------------
# | build
# | Description: Converts the disc into work_dir as pre.makefile does and returns the cd/data files plus the ELF and VMU GAME blocks.
# | Author: suinevere
# | Dependencies: convert_disc, libgte_tables, akao_data, main_data, vmu_portraits, gen_tables, _cd_files, _quiet
# | Globals: RES1506
# | Params: image_path -- the user's PS1 .bin, work_dir -- empty scratch folder
# | Returns: dict name -> bytes
#----------------------
def build(image_path, work_dir):
    argv = ["--image", image_path, "--out", work_dir]
    _quiet("the disc converter", convert_disc.main, ["--target", "dreamcast"] + argv)
    _quiet("libgte table extraction", libgte_tables.main, argv)
    out = _cd_files(work_dir)
    with open(os.path.join(work_dir, "gen", "libgte_tables.bin"), "rb") as handle:
        out["gen/libgte_tables"] = handle.read()
    exe = libgte_tables.read_iso_file(disc_image.DiscImage(image_path), libgte_tables.EXE_NAME)
    out["gen/main_data"] = main_data.block_from_exe(exe)
    out["gen/vmu_pet_screens"] = b"".join(bytes(s) for s in vmu_portraits.disc_screens(image_path))
    out["gen/akao_data"] = akao_data.data_block(image_path)[0]
    out["vmu/rrl_pages"] = gen_tables.kept_pages(out[RES1506])
    return out


#----------------------
# | akao_pointer_words
# | Description: Indices of AKAO data words the build relocates, which the template keeps as literals.
# | Author: suinevere
# | Dependencies: akao_data.data_block
# | Globals: N/A
# | Params: image_path -- the user's PS1 .bin
# | Returns: list of int
#----------------------
def akao_pointer_words(image_path):
    return akao_data.data_block(image_path)[1]
