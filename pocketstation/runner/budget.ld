/*----------------------
 | budget.ld
 | Description: Links the budget build into one RAM image at 0: the game's code alone between __game_start and __game_end, then the host backend, the driver and libc, then data, BSS and the heap from end.
 | Author: suinevere
 | Dependencies: N/A
 | Globals: N/A
 | Params: N/A
 | Returns: N/A
 ----------------------*/
ENTRY(_start)

MEMORY {
    RAM (rwx) : ORIGIN = 0x00000000, LENGTH = 16M
}

SECTIONS {
    .text : {
        __game_start = .;
        *libgame.a:*(.text .text.*)
        __game_end = .;
        *(.text .text.*)
    } > RAM
    .rodata : { *(.rodata .rodata.*) } > RAM
    .ARM.exidx : { *(.ARM.exidx*) } > RAM
    .data : { *(.data .data.*) } > RAM
    .bss : {
        __bss_start__ = .;
        *(.bss .bss.* COMMON)
        __bss_end__ = .;
    } > RAM
    end = ALIGN(8);
}
