#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "source/scpi_parser.h"

// ------- TESTING ----------

int main()
{
	scpi_handle_string("*IDN?\n");
	scpi_handle_string("*SRE 4\n");
	scpi_handle_string("FOO:BAR:BAZ\n");
	scpi_handle_string("SYST:ERR:COUNT?\n");
	scpi_handle_string("SYST:ERR?\n");
	scpi_handle_string("SYST:ERR?\n");
	scpi_handle_string("DATA:BLOB #216abcdefghijklmnop\n");
	scpi_handle_string("SYST:ERR?\n");
}


// ---- Test device impl ----

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
	printf("[SRQ]\n");
}


// ---- COMMANDS ----

void cmd_APPL_SIN_cb(const SCPI_argval_t *args)
{
	printf("cb  APPLy:SINe %d, %f, %f\n", args[0].INT, args[0].FLOAT, args[0].FLOAT);
}



void cmd_DISP_TEXT_cb(const SCPI_argval_t *args)
{
	printf("cb  DISPlay:TEXT %s\n", args[0].STRING);
}


void cmd_DATA_BLOB_cb(const SCPI_argval_t *args)
{
	printf("cb  DATA:BLOB <%d>\n", args[1].BLOB_LEN);
}


void cmd_DATA_BLOB_data(const uint8_t *bytes)
{
	printf("blob item: %s\n", bytes);
}


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
	{0} // end marker
};
