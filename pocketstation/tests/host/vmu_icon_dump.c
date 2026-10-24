/*----------------------
 | vmu_icon_dump.c
 | Description: Writes what R2's vmu_icon_from_ps1 makes of a PS1 file (frame count byte, 16 little-endian palette entries, the frames) so tests can pin vmu_flash.py's package to it.
 | Author: suinevere
 | Dependencies: dreamcast/src/card/vmu_format.c, vmu_format.h
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#include <stdio.h>
#include <string.h>

#include "vmu_format.h"

/*----------------------
 | main
 | Description: Reads the first 0x200 bytes of a PS1 file and writes the converted icon to an output file.
 | Author: suinevere
 | Dependencies: vmu_icon_from_ps1
 | Globals: N/A
 | Params: argc, argv -- IN.ps1 OUT.bin
 | Returns: 0, or 2 on an I/O error
 ----------------------*/
int main(int argc, char **argv)
{
    static unsigned char ps1[0x200], frames[3 * 512];
    unsigned short palette[16];
    FILE *in, *out;
    if (argc != 3 || !(in = fopen(argv[1], "rb")) || fread(ps1, 1, sizeof ps1, in) != sizeof ps1) return 2;
    fclose(in);
    memset(palette, 0, sizeof palette);
    int count = vmu_icon_from_ps1(ps1, palette, frames);
    if (!(out = fopen(argv[2], "wb"))) return 2;
    fputc(count, out);
    for (int i = 0; i < 16; i++) {
        fputc(palette[i] & 0xFF, out);
        fputc(palette[i] >> 8, out);
    }
    fwrite(frames, 1, 512u * (unsigned)count, out);
    fclose(out);
    return 0;
}
