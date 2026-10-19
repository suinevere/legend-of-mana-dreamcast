#!/bin/sh
#----------------------
# | compile.sh
# | Description: Builds the PocketStation reference runner and OpenPSK launcher inside the lom-pocket Docker image.
# | Author: suinevere
# | Dependencies: dockerfiles/pocketstation.dockerfile, pocketstation/Makefile
# | Globals: N/A
# | Params: $1 -- make target, default all
# | Returns: make's exit code
#----------------------
target="${1:-all}"
here="$(cd "$(dirname "$0")/.." && (pwd -W 2>/dev/null || pwd))"
MSYS_NO_PATHCONV=1 exec docker run --rm -v "$here:/work" -w /work lom-pocket make -C pocketstation "$target"
