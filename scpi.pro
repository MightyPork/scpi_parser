TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

DEFINES += __null=0

INCLUDEPATH += source/ \
	include/ \
	/usr/arm-none-eabi/include \
	/usr/lib/gcc/x86_64-unknown-linux-gnu/5.3.0/plugin/include

SOURCES += \
	main.c \
	source/scpi_parser.c \
	source/scpi_errors.c \
	source/scpi_regs.c \
	source/scpi_builtins.c \
	example/example.c

DISTFILES += \
	style.astylerc \
	.gitignore \
	LICENSE \
	README.md \
	Makefile \
    example/Makefile

HEADERS += \
	source/scpi_parser.h \
	source/scpi_errors.h \
	source/scpi_regs.h \
	source/scpi_builtins.h \
	include/scpi_builtins.h \
	include/scpi_errors.h \
	include/scpi_parser.h \
	include/scpi_regs.h \
	include/scpi.h
