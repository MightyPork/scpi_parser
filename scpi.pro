TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    scpi_parser.c \
    main.c \
    scpi_errors.c \
    scpi_status.c

DISTFILES += \
    style.astylerc \
    .gitignore \
    LICENSE \
    README.md \
    Makefile

HEADERS += \
    scpi_parser.h \
    scpi_errors.h \
    scpi_status.h
