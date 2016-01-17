SRC   = example.c
SRC  += ../source/scpi_parser.c
SRC  += ../source/scpi_regs.c
SRC  += ../source/scpi_builtins.c
SRC  += ../source/scpi_errors.c

INCL_DIR  = ../include

CFLAGS       = -std=gnu99
CFLAGS      += -Wall -Wextra -Wshadow
CFLAGS      += -Wwrite-strings -Wold-style-definition -Winline
CFLAGS      += -Wredundant-decls -Wfloat-equal -Wsign-compare -Wunused-function

CFLAGS += -DSCPI_FINE_ERRORS

CC = gcc

%.o: %.c


all: example.elf

example.elf: $(SRC)
	$(Q)$(CC) $(CFLAGS) -I$(INCL_DIR) -o example.elf $(SRC)

run: example.elf
	./example.elf

clean:
	rm -f *.o *.d *.so *.elf *.bin *.hex
	cd ../source
	rm -f *.o *.d *.so *.elf *.bin *.hex
