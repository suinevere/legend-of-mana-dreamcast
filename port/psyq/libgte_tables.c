/*----------------------
 | libgte_tables.c
 | Description: Embeds gen/libgte_tables.bin, the libgte tables extracted from the user's SLUS_010.13, as the read-only array lom_libgte_tables.
 | Author: suinevere
 | Dependencies: gen/libgte_tables.bin (tools/libgte_tables.py), assembled relative to the build directory
 | Globals: lom_libgte_tables
 | Params: N/A
 | Returns: N/A
 ----------------------*/

/*----------------------
 | LOM_LIBGTE_STR / LOM_LIBGTE_XSTR
 | Description: Stringify a token, after macro expansion for the XSTR form.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: x -- token
 | Returns: string literal
 ----------------------*/
#define LOM_LIBGTE_STR(x) #x
#define LOM_LIBGTE_XSTR(x) LOM_LIBGTE_STR(x)

/*----------------------
 | LOM_LIBGTE_SYMBOL
 | Description: Assembler name of lom_libgte_tables, with the target's C label prefix (an underscore on SH-ELF).
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#define LOM_LIBGTE_SYMBOL LOM_LIBGTE_XSTR(__USER_LABEL_PREFIX__) "lom_libgte_tables"

/*----------------------
 | LOM_LIBGTE_TABLES_BIN
 | Description: Path of the extracted blob, relative to the directory the compiler runs in.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
#ifndef LOM_LIBGTE_TABLES_BIN
#define LOM_LIBGTE_TABLES_BIN "gen/libgte_tables.bin"
#endif

/*----------------------
 | lom_libgte_tables
 | Description: The table blob, word-aligned in .rodata; this file emits nothing else, so the section switch cannot misplace compiler output.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
__asm__(".section .rodata\n"
        ".balign 4\n"
        ".global " LOM_LIBGTE_SYMBOL "\n"
        LOM_LIBGTE_SYMBOL ":\n"
        ".incbin \"" LOM_LIBGTE_TABLES_BIN "\"\n"
        ".text\n");
