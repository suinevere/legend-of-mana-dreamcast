#!/bin/sh
#----------------------
# | pkrun.sh
# | Description: Runs the reference runner build/pkrun inside the lom-pocket Docker image with the repo at /work.
# | Author: suinevere
# | Dependencies: pocketstation/compile.sh
# | Globals: N/A
# | Params: $@ -- pkrun arguments, paths relative to the repo root
# | Returns: pkrun's exit code
#----------------------
here="$(cd "$(dirname "$0")/.." && (pwd -W 2>/dev/null || pwd))"
MSYS_NO_PATHCONV=1 exec docker run --rm -v "$here:/work" -w /work lom-pocket pocketstation/build/pkrun "$@"
