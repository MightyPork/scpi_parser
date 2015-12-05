# SCPI parser

This library provides a simple ("KISS") SCPI implementation for embedded devices (instruments).

The implementation is not 100% complete, but it's sufficient for basic SCPI communication.

## What's supported

- The hierarchical header model (commands with colon)
- Semicolon for chaining commands on the same level
- String, Int, Float, Bool arguments
- Block data argument with callback each N received bytes (configurable)
- Status Register model
- Error queue including error messages from the SCPI spec
- All mandatory SCPI commands (headers) are implemented as built-ins

Built-in commands can be overriden in user command array.

See main.c for example of how to use the library.

## What is missing



