SRC = main.c
SRC+= source/scpi_parser.c
SRC+= source/scpi_regs.c
SRC+= source/scpi_builtins.c
SRC+= source/scpi_errors.c

HDRS= source/scpi_parser.h
HDRS+= source/scpi_regs.h
HDRS+= source/scpi_builtins.h
HDRS+= source/scpi_errors.h

CFLAGS      += -std=gnu99
CFLAGS      += -Wall -Wextra -Wshadow
CFLAGS      += -Wwrite-strings -Wold-style-definition -Winline
CFLAGS      += -Wredundant-decls -Wfloat-equal -Wsign-compare -Wunused-function

# gcc bug?
CFLAGS      += -Wno-missing-braces

all: $(SRC) $(HDRS)
	gcc $(CFLAGS) $(SRC) -o main.elf

run: all
	./main.elf

clean:
	rm -f *.o *.d *.so *.elf *.bin *.hex
	cd source
	rm -f *.o *.d *.so *.elf *.bin *.hex


