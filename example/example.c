#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "scpi_parser.h"

// ------- TESTING ----------

static void send_cmd(const char *cmd)
{
	printf("\n> %s\n", cmd);
	scpi_handle_string(cmd);
}

int main(void)
{
	send_cmd("*IDN?\n"); // builtin commands..
	send_cmd("*SRE 4\n"); // enable SRQ on error
	send_cmd("FOO:BAR:BAZ\n"); // invalid command causes error
	send_cmd("SYST:ERR:COUNT?\n"); // error subsystem
	send_cmd("SYST:ERR:NEXT?; COUNT?; NEXT?\n"); // semicolon works according to spec
	send_cmd("DATA:BLOB #216abcdefghijklmnop\n"); // binary data block

	send_cmd("APPLY:SINE 50, 1.0, 2.17\n"); // floats
	send_cmd("DISP:TEXT \"Hello world\"\n"); // string

	// test user error
	send_cmd("*CLS\n"); // string
	send_cmd("USERERROR\n"); // string
	send_cmd("SYST:ERR:COUNT?\n");
	send_cmd("SYST:ERR:NEXT?\n");

	// test chardata
	send_cmd("CHARD FOOBAR123_MOO_abcdef_HELLO, 12\n");
}


// ---- Test device impl ----


const SCPI_error_desc scpi_user_errors[] = {
	{10, "Custom error"},
	{0} // terminator
};


void scpi_send_byte_impl(uint8_t b)
{
	putchar(b); // device sends a byte
}


const char *scpi_device_identifier(void)
{
	return "FEL CVUT,DDS1,0,0.1";
}


void scpi_service_request_impl(void)
{
	// NOTE: Actual instrument should send SRQ event somehow

	printf("[SRQ] - error: ");
	char buf[256];
	scpi_read_error_noremove(buf); // show error message (for debug)
	printf("%s\n", buf);
}


// ---- INSTRUMENT COMMANDS ----

void cmd_APPL_SIN_cb(const SCPI_argval_t *args)
{
	printf("cb APPLy:SINe %d, %f, %f\n", args[0].INT, args[1].FLOAT, args[2].FLOAT);
}



void cmd_DISP_TEXT_cb(const SCPI_argval_t *args)
{
	printf("cb DISPlay:TEXT %s\n", args[0].STRING);
}


void cmd_DATA_BLOB_cb(const SCPI_argval_t *args)
{
	printf("cb DATA:BLOB <%d>\n", args[0].BLOB_LEN);
}


void cmd_DATA_BLOB_data(const uint8_t *bytes)
{
	printf("binary data: \"%s\"\n", bytes);
}


void cmd_USRERR_cb(const SCPI_argval_t *args)
{
	(void) args;

	printf("cb USRERR - raising user error 10.\n");
	scpi_add_error(10, "Custom error message...");
}


void cmd_CHARD_cb(const SCPI_argval_t *args)
{
	printf("CHARData cb: %s, arg2 = %d\n", args[0].CHARDATA, args[1].INT);
}


// Command definition (mandatory commands are built-in)
const SCPI_command_t scpi_commands[] = {
	{
		.levels = {"APPLy", "SINe"},
		.params = {SCPI_DT_INT, SCPI_DT_FLOAT, SCPI_DT_FLOAT},
		.callback = cmd_APPL_SIN_cb
	},
	{
		.levels = {"DISPlay", "TEXT"},
		.params = {SCPI_DT_STRING},
		.callback = cmd_DISP_TEXT_cb
	},
	{
		.levels = {"DATA", "BLOB"},
		.params = {SCPI_DT_BLOB},
		.callback = cmd_DATA_BLOB_cb,
		.blob_chunk = 4,
		.blob_callback = cmd_DATA_BLOB_data
	},
	{
		.levels = {"USeRERRor"},
		.callback = cmd_USRERR_cb
	},
	{
		.levels = {"CHARData"},
		.params = {SCPI_DT_CHARDATA, SCPI_DT_INT},
		.callback = cmd_CHARD_cb
	},
	{/*END*/}
};
