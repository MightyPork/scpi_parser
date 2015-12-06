# SCPI parser

This library provides a simple ("KISS") SCPI implementation for embedded devices (instruments).

The implementation is not 100% complete, but it's sufficient for basic SCPI communication.

- Easy configuration with one const array and callbacks
- Implements SCPI99 (minimal requirements) - including common commands and status registers
- Mandatory parts of the SYST and STAT subsystems are built in (ie. error handling)
- Easy, straightforward API

## Feature overview

### What's supported

- Commands with colon (hierarchical header model)
- Semicolon for chaining commands on the same level
- Long and short command variants (eg. `SYSTem?`)
- String, Int, Float, Bool, CharData arguments
- **Block data argument** with callback each N received bytes - allows virtually unlimite binary data length
- Status Registers
- Error queue with error numbers and messages (and the required SYST:ERR subsystem)

Built-in commands can be overriden by matching user commands.

### Limitations

- Binary block must be the last argument of a command (callback is run after reading the binary block preamble)

### What's missing

- Number units (mV,V,kW,A,Ohm) and metric suffixes (k,M,G,m,u,n)
- DEF, MIN, MAX, INF, NINF argument values for numbers
- Numbered virtual instruments (DAC1:OUT, DAC2:OUT) - currently you need to define the commands twice

Feel free to propose a pull request implementing any missing features.


## How to use it

To test it with regular gcc, run the Makefile in the `example` directory.

The main Makefile builds a library for ARM Cortex M4 (can be easily adjusted for others).

### Stubs to implement

Here's an overview of stubs you have to implement (at the time of writing this readme):

```c
#include "scpi.h"

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
