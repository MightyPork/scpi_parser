###############################################################################

# Makefile to build a library for ARM CortexM3

FP_FLAGS      = -mfloat-abi=hard -mfpu=fpv4-sp-d16
ARCH_FLAGS    = -mthumb -mcpu=cortex-m4 $(FP_FLAGS)

INCL_DIR      = include
SRC_DIR       = source

LIBNAME       = arm_cortexM4_scpi

OBJS          = $(SRC_DIR)/scpi_parser.o
OBJS         += $(SRC_DIR)/scpi_regs.o
OBJS         += $(SRC_DIR)/scpi_errors.o
OBJS         += $(SRC_DIR)/scpi_builtins.o

JUNK          = *.o *.d *.elf *.bin *.hex *.srec *.list *.map *.dis *.disasm *.a

###############################################################################

PREFIX  ?= arm-none-eabi

CC      := $(PREFIX)-gcc
AR      := $(PREFIX)-ar

###############################################################################

CFLAGS      += -Os -ggdb -std=gnu99 -Wfatal-errors
CFLAGS      += -Wall -Wextra -Wshadow
CFLAGS      += -Wwrite-strings -Wold-style-definition -Winline -Wmissing-noreturn -Wstrict-prototypes
CFLAGS      += -Wredundant-decls -Wfloat-equal -Wsign-compare
CFLAGS      += -fno-common -ffunction-sections -fdata-sections -Wunused-function
CFLAGS      += -I$(INCL_DIR)

CFLAGS += -DSCPI_FINE_ERRORS
CFLAGS += -DSCPI_WEIRD_ERRORS

###############################################################################

all: lib

%.o: %.c
	$(Q)$(CC) $(CFLAGS) $(ARCH_FLAGS) -o $(*).o -c $(*).c

lib: lib/lib$(LIBNAME).a

lib/lib$(LIBNAME).a: $(OBJS)
	$(Q)$(AR) rcs $@ $(OBJS)

clean:
	$(Q)$(RM) $(JUNK)
	$(Q)cd source && $(RM) $(JUNK)
	$(Q)cd lib && $(RM) $(JUNK)
	$(Q)cd example && $(RM) $(JUNK)

.PHONY: clean all lib
