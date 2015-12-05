SRC = main.c
SRC+= source/scpi_parser.c
SRC+= source/scpi_regs.c
SRC+= source/scpi_builtins.c
SRC+= source/scpi_errors.c

HDRS= source/scpi_parser.h
HDRS+= source/scpi_regs.h
HDRS+= source/scpi_builtins.h
HDRS+= source/scpi_errors.h

all: $(SRC) $(HDRS)
	gcc $(SRC) -o main.elf

run: all
	./main.elf

clean:
	rm -f *.o *.d *.so *.elf *.bin *.hex
	cd source
	rm -f *.o *.d *.so *.elf *.bin *.hex


