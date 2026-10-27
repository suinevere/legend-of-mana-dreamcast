#----------------------
# | lom_dc_kit.py
# | Description: The lom-dc-kit command: makes the Legend of Mana Dreamcast GDI from the user's own PS1 .cue or .bin.
# | Author: suinevere
# | Dependencies: port/kit/{fill,disc_input}.py, port/kit/_version.py (written at freeze time)
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
import argparse
import os
import sys

sys.path.insert(0, os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..")))

from port.kit import disc_input, fill  # noqa: E402

try:
    from port.kit._version import VERSION  # noqa: E402
except ImportError:
    #----------------------
    # | VERSION
    # | Description: The release tag the executable was frozen from, or "dev" from source.
    # | Author: suinevere
    # | Dependencies: port/kit/_version.py
    # | Globals: N/A
    # | Params: N/A
    # | Returns: N/A
    #----------------------
    VERSION = "dev"

#----------------------
# | OUT_NAME
# | Description: The GDI folder made beside the user's disc when no --out is given.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: N/A
#----------------------
OUT_NAME = "Legend of Mana (Dreamcast)"


#----------------------
# | default_kit_dir
# | Description: The kit folder beside the executable (frozen) or beside this file (source).
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: N/A
# | Returns: str
#----------------------
def default_kit_dir():
    base = os.path.dirname(sys.executable if getattr(sys, "frozen", False) else os.path.abspath(__file__))
    return os.path.join(base, "kit")


#----------------------
# | _is_terminal
# | Description: Whether stream is an interactive terminal, false for streams without isatty.
# | Author: suinevere
# | Dependencies: N/A
# | Globals: N/A
# | Params: stream -- file-like object
# | Returns: bool
#----------------------
def _is_terminal(stream):
    try:
        return stream.isatty()
    except (AttributeError, ValueError):
        return False


#----------------------
# | main
# | Description: Parses arguments, or asks for a dragged-in path when launched from a terminal with none, makes the GDI and reports failures in one line.
# | Author: suinevere
# | Dependencies: fill.make_gdi, disc_input.resolve, default_kit_dir, OUT_NAME, VERSION
# | Globals: N/A
# | Params: argv -- arguments, stdin -- input stream for the prompt, kit_dir -- kit folder override, interactive -- force the prompt behaviour on or off
# | Returns: exit code
#----------------------
def main(argv=None, stdin=None, kit_dir=None, interactive=None):
    argv = sys.argv[1:] if argv is None else argv
    stdin = sys.stdin if stdin is None else stdin
    if interactive is None:
        interactive = not argv and _is_terminal(stdin)
    parser = argparse.ArgumentParser(prog="lom-dc-kit", description="Make the Legend of Mana Dreamcast GDI from your PS1 disc image")
    parser.add_argument("disc", nargs="?", help="your Legend of Mana (USA) .cue or .bin")
    parser.add_argument("-o", "--out", help="GDI folder to create (default: '%s' beside your disc)" % OUT_NAME)
    parser.add_argument("--kit", default=kit_dir, help=argparse.SUPPRESS)
    parser.add_argument("--overwrite", action="store_true", help="replace the output folder if it exists")
    parser.add_argument("--version", action="store_true", help="print the release this kit is from")
    args = parser.parse_args(argv)
    if args.version:
        print("lom-dc-kit %s" % VERSION)
        return 0
    if not args.disc:
        if not interactive:
            parser.print_usage()
            return 2
        print("Drag your Legend of Mana (USA) .cue or .bin onto this window and press Enter:")
        args.disc = stdin.readline().strip().strip('"')
    code = 0
    try:
        image = disc_input.resolve(args.disc)
        out = args.out or os.path.join(os.path.dirname(os.path.abspath(args.disc.strip().strip('"'))), OUT_NAME)
        done = fill.make_gdi(args.kit or default_kit_dir(), image, out, overwrite=args.overwrite, progress=print)
        print("Done: %s" % done)
    except (fill.KitError, disc_input.DiscError, RuntimeError, ValueError, OSError) as exc:
        print("lom-dc-kit: %s" % exc, file=sys.stderr)
        code = 1
    if interactive:
        print("Press Enter to close.")
        stdin.readline()
    return code


if __name__ == "__main__":
    sys.exit(main())
