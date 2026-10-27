#!/bin/sh
#----------------------
# | compile.sh
# | Description: Builds the Legend of Mana Dreamcast port inside the lom-dc Docker image.
# | Author: suinevere
# | Dependencies: dockerfiles/dreamcast.Dockerfile, makefile
# | Globals: N/A
# | Params: $1 -- "clean" to clean, "pending" to regenerate pending stubs, empty, "all" or "gdi" to build lom.elf, gdi/ and lom.cdi
# | Returns: make's exit code, or 2 on an invalid argument
#----------------------
target="${1:-all}"
case "$target" in
    all|gdi|clean|pending) ;;
    *)
        echo "usage: $0 [all|gdi|clean|pending]" >&2
        exit 2
        ;;
esac
here="$(cd "$(dirname "$0")/.." && pwd)"
exec docker run --rm -v "$here:/work" -w /work lom-dc bash -lc "make -C dreamcast $target"
