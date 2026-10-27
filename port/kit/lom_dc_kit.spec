#----------------------
# | lom_dc_kit.spec
# | Description: PyInstaller one-file build of lom-dc-kit with the stdlib-only converter modules and config/us/symbols bundled.
# | Author: suinevere
# | Dependencies: pyinstaller==6.11.1, port/kit, port/tools, tools/splat_ext/decompress.py, pocketstation/tools/gen_tables.py, config/us/symbols
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import os

#----------------------
# | ROOT / PATHS / NOT_BUNDLED / HIDDEN
# | Description: The repo root, the folders the converter modules import from, the port/tools scripts that need packages the kit does not ship, and the modules bundled by name.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
ROOT = os.path.abspath(os.path.join(SPECPATH, "..", ".."))
PATHS = [ROOT, os.path.join(ROOT, "port", "tools"), os.path.join(ROOT, "tools", "splat_ext"), os.path.join(ROOT, "pocketstation", "tools")]
NOT_BUNDLED = {"gen_button_icons"}
HIDDEN = sorted({os.path.splitext(n)[0] for n in os.listdir(PATHS[1]) if n.endswith(".py")} - NOT_BUNDLED) + ["decompress", "gen_tables", "port.kit.sources"]

a = Analysis([os.path.join(SPECPATH, "lom_dc_kit.py")], pathex=PATHS, hiddenimports=HIDDEN,
             datas=[(os.path.join(ROOT, "config", "us", "symbols"), os.path.join("config", "us", "symbols"))],
             excludes=["tkinter", "unittest", "pydoc", "PIL", "pytest", "capstone", "unicorn"])
pyz = PYZ(a.pure)
exe = EXE(pyz, a.scripts, a.binaries, a.datas, name="lom-dc-kit", console=True, upx=False, strip=False)
