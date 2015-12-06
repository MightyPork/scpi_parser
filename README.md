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

## How to use

See main.c for example of how to use the library.

Here's an overview of stubs you have to implement (at the time of writing this readme):

```c
#include "scpi_parser.h"

// Receive byte - call:
//   scpi_handle_byte()
//   scpi_handle_string()


// ---- DEVICE IMPLEMENTATION ----

const SCPI_error_desc scpi_user_errors[] = {
	{10, "Custom error"}, // add your custom errors here (positive numbers)
	{/*END*/} // <-- end marker
};


void scpi_send_byte_impl(uint8_t b)
{
	// send the byte to master (over UART?)
}


const char *scpi_device_identifier(void)
{
	// fill in your device info
	// possible to eg. read a serial # from EEPROM
	return "<manufacturer>,<product>,<serial#>,<version>";
}


/* Custom commands */
const SCPI_command_t scpi_commands[] = {
	// see the struct definition for more details. Examples:
	{
		.levels = {"APPLy", "SINe"},
		.params = {SCPI_DT_INT, SCPI_DT_FLOAT, SCPI_DT_FLOAT},
		.callback = cmd_APPL_SIN_cb
	},
	{
		.levels = {"DATA", "BLOB"},
		.params = {SCPI_DT_BLOB},
		.callback = cmd_DATA_BLOB_cb, // <-- command callback
		.blob_chunk = 4,
		.blob_callback = cmd_DATA_BLOB_data // <-- data callback
	},
	{/*END*/} // <-- important! Marks end of the array
};

// ---- OPTIONAL CALLBACKS ----

void scpi_service_request_impl(void)
{
	// Called when the SRQ flag in Status Byte is set.
	// Device should somehow send the request to master.
}

// Device specific implementation of common commands
// (status registers etc are handled internally)
void scpi_user_CLS(void) { /*...*/ }
void scpi_user_RST(void) { /*...*/ }
void scpi_user_TSTq(void) { /*...*/ }

```

See the header files for more info.

## What is missing

- Number units and metric suffixes (k,M,G,m,u,n)
- DEF, MIN, MAX, INF, NINF argument values for numbers

Support for units and "constants" (DEF etc) in numeric fields will need some larger changes to the numeric parser and `SCPI_argval_t`.

Feel free to propose a pull request implementing any missing features.
