# SCPI parser

This library provides a simple ("KISS") SCPI implementation for embedded devices (instruments).

The implementation is not 100% complete, but it's sufficient for basic SCPI communication.

## What's supported

- The hierarchical header model (commands with colon)
- Long and short header variants (eg. `SYSTem?`)
- Easy configuration with one const array of structs and callback functions (see `main.c`)
- Semicolon for chaining commands on the same level
- String, Int, Float, Bool arguments
- Block data argument with callback each N received bytes (configurable)
- Status Register model
- Error queue including error messages from the SCPI spec
- All mandatory SCPI commands (headers) are implemented as built-ins

Built-in commands can be overriden in user command array.

See main.c for example of how to use the library.

## What is missing

- Setting error type flags in the ESR when error is added to queue
- There may be some bugs in the status registers (not tested much)
- Character data argument ("string without quotes", enum-like)
- Number units and metric suffixes (k,M,G,m,u,n)
- DEF, MIN, MAX, INF, NINF argument values for numbers

Support for units and "constants" (DEF etc) in numeric fields will need some larger changes to the numeric parser and `SCPI_argval_t`.

Feel free to propose a pull request implementing any missing features.
