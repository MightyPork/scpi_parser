TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += source/

SOURCES += \
	main.c \
	source/scpi_parser.c \
	source/scpi_errors.c \
	source/scpi_regs.c \
	source/scpi_builtins.c

DISTFILES += \
	style.astylerc \
	.gitignore \
	LICENSE \
	README.md \
	Makefile

HEADERS += \
	source/scpi_parser.h \
	source/scpi_errors.h \
	source/scpi_regs.h \
	source/scpi_builtins.h
