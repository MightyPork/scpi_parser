TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
    scpi_parser.c \
    main.c

DISTFILES += \
    style.astylerc \
    .gitignore \
    LICENSE \
    README.md

HEADERS += \
    scpi_parser.h
