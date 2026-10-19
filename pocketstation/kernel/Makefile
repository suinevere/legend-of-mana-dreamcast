#----------------------
# | Makefile
# | Description: Builds build/openpsk-lom.bin: OpenPSK's kernel objects with launch.c in place of its demo main.c.
# | Author: suinevere
# | Dependencies: ../build/vendor/pocketstation/openpsk, launch.c
# | Globals: N/A
# | Params: target -- all (default), clean
# | Returns: N/A
#----------------------
PREFIX    ?= arm-none-eabi
CC         = $(PREFIX)-gcc
OBJCOPY    = $(PREFIX)-objcopy
PSK        = ../build/vendor/pocketstation/openpsk
OUT        = ../build/kernel
ROM        = ../build/openpsk-lom.bin
ARCHFLAGS  = -march=armv4t -marm
CFLAGS     = $(ARCHFLAGS) -ffreestanding -nostdlib -fno-builtin -Os -Wall -Wextra -std=c11 -fomit-frame-pointer -I$(PSK)
LDFLAGS    = $(ARCHFLAGS) -nostdlib -T $(PSK)/openpsk.ld -Wl,--build-id=none
ASM_OBJS   = $(addprefix $(OUT)/,crt0.o swi.o irq.o fiq.o)
C_OBJS     = $(addprefix $(OUT)/,swi_handlers.o syscalls.o com.o)

all: $(ROM)

$(OUT)/%.o: $(PSK)/%.S
	@mkdir -p $(OUT)
	$(CC) $(ARCHFLAGS) -c $< -o $@

$(OUT)/%.o: $(PSK)/%.c $(wildcard $(PSK)/*.h)
	@mkdir -p $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/launch.o: launch.c $(wildcard $(PSK)/*.h)
	@mkdir -p $(OUT)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT)/openpsk-lom.elf: $(ASM_OBJS) $(C_OBJS) $(OUT)/launch.o
	$(CC) $(LDFLAGS) $^ -o $@

$(ROM): $(OUT)/openpsk-lom.elf
	$(OBJCOPY) -O binary $< $@
	truncate -s 16384 $@

clean:
	rm -rf $(OUT) $(ROM)

.PHONY: all clean
