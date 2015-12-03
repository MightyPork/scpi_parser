all: main.c scpi_parser.c scpi_parser.h
	gcc main.c scpi_parser.c scpi_errors.c -o main.elf

run: all
	./main.elf

