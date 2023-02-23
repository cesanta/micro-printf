PROG ?= unit_test                   # Program we are building
DELETE = rm -rf                     # Command to remove files
OUT ?= -o $(PROG)                   # Compiler argument for output file
SOURCES = main.c ../micro_printf.c  # Source code files
CFLAGS = -W -Wall -Wextra -g -I..   # Build options

ifeq ($(OS),Windows_NT)   # Windows settings. Assume MinGW compiler. To use VC: make CC=cl CFLAGS=/MD OUT=/Feprog.exe
  PROG ?= unit_test.exe         # Use .exe suffix for the binary
  CC = gcc                      # Use MinGW gcc compiler
  DELETE = cmd /C del /Q /F /S  # Command prompt command to delete files
  OUT ?= -o $(PROG)             # Build output
endif

run: $(PROG)              # Default target. Build and run program
	$(RUN) ./$(PROG) $(ARGS)

$(PROG): $(SOURCES)       # Build program from sources
	$(CC) $(SOURCES) $(CFLAGS) $(CFLAGS_EXTRA) $(OUT)

define build
  @arm-none-eabi-gcc footprint.c ../micro_printf.c -o $1.elf -ffunction-sections -fdata-sections -I.. $2 $3 -mthumb -e main -nostartfiles -nostdlib --specs nano.specs -lc -lgcc -Wl,--gc-sections -Wl,-Map=$1.map
  @arm-none-eabi-objcopy -O binary $1.elf $1.bin
  @ls -l $1.bin
endef

M7 = -Os -mcpu=cortex-m7 -mfloat-abi=hard -mfpu=fpv4-sp-d16
M0 = -Os -mcpu=cortex-m0 -mfloat-abi=soft
footprint:
	$(call build,m7_nofloat,$(M7),-DNOFLOAT)
	$(call build,m7,$(M7),)
	$(call build,m7_std_nofloat,$(M7),-DSTD)
	$(call build,m7_std,$(M7),-DSTD -u _printf_float)
	$(call build,m0_nofloat,$(M0),-DNOFLOAT)
	$(call build,m0,$(M0),)
	$(call build,m0_std_nofloat,$(M0),-DSTD)
	$(call build,m0_std,$(M0),-DSTD -u _printf_float)

clean:                    # Cleanup. Delete built program and all build artifacts
	$(DELETE) $(PROG) *.o *.obj *.exe *.dSYM *.elf *.bin *.map

