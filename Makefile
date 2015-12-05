all: main.c scpi_parser.c scpi_parser.h
	gcc main.c scpi_parser.c scpi_errors.c scpi_regs.c scpi_builtins.c -o main.elf

run: all
	./main.elf

