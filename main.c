#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "scpi_parser.h"
#include "scpi_errors.h"



void cmd_aIDNq_cb(const SCPI_argval_t *args)
{
	printf("cb  *IDN?\n");
}


void cmd_APPL_SIN_cb(const SCPI_argval_t *args)
{
	printf("cb  APPLy:SINe\n");
}


void cmd_APPL_TRI_cb(const SCPI_argval_t *args)
{
	printf("cb  APPLy:TRIangle\n");
}


void cmd_FREQ_cb(const SCPI_argval_t *args)
{
	printf("cb  FREQuency %d\n", args[0].INT);
}

void cmd_DISP_TEXT_cb(const SCPI_argval_t *args)
{
	printf("cb  DISPlay:TEXT %s, %d\n", args[0].STRING, args[1].BOOL);
}


void cmd_DATA_BLOB_cb(const SCPI_argval_t *args)
{
	printf("cb  DATA:BLOB %f, <%d>\n", args[0].FLOAT, args[1].BLOB_LEN);
}


void cmd_DATA_BLOB_data(const uint8_t *bytes)
{
	printf("blob item: %s\n", bytes);
}

void cmd_STQENq_cb(const SCPI_argval_t *args)
{
	printf("STATUS:QUEUE:ENABLE:NEXT?\n");
}

void cmd_STQEq_cb(const SCPI_argval_t *args)
{
	printf("STATUS:QUEUE:ENABLE?\n");
}

void cmd_STQE_cb(const SCPI_argval_t *args)
{
	printf("STATUS:QUEUE:ENABLE %d\n", args[0].BOOL);
}

const SCPI_command_t scpi_cmd_lang[] = {
	{
		.levels = {"*IDN?"},
		.params = {},
		.callback = cmd_aIDNq_cb
	},
	{
		.levels = {"APPLy", "SINe"},
		.params = {SCPI_DT_INT, SCPI_DT_FLOAT, SCPI_DT_FLOAT},
		.callback = cmd_APPL_SIN_cb
	},
	{
		.levels = {"APPLy", "TRIangle"},
		.params = {SCPI_DT_INT, SCPI_DT_FLOAT, SCPI_DT_FLOAT},
		.callback = cmd_APPL_TRI_cb
	},
	{
		.levels = {"FREQuency"},
		.params = {SCPI_DT_INT},
		.callback = cmd_FREQ_cb
	},
	{
		.levels = {"DISPlay", "TEXT"},
		.params = {SCPI_DT_STRING, SCPI_DT_BOOL},
		.callback = cmd_DISP_TEXT_cb
	},
	{
		.levels = {"DATA", "BLOB"},
		.params = {SCPI_DT_FLOAT, SCPI_DT_BLOB},
		.callback = cmd_DATA_BLOB_cb,
		.blob_chunk = 4,
		.blob_callback = cmd_DATA_BLOB_data
	},
	{
		.levels = {"STATus", "QUEue", "ENABle", "NEXT?"},
		.callback = cmd_STQENq_cb
	},
	{
		.levels = {"STATus", "QUEue", "ENABle?"},
		.callback = cmd_STQEq_cb
	},
	{
		.levels = {"STATus", "QUEue", "ENABle"},
		.params = {SCPI_DT_BOOL},
		.callback = cmd_STQE_cb
	},
	{0} // end marker
};



int main()
{

//	const char *inp = "*IDN?\n";
//	const char *inp = "FREQ 50\n";
	const char *inp = "DISP:TEXT 'ban\\\\ana', OFF\nDISP:TEXT \"dblquot!\", 1\r\nFREQ 50\r\n";

//	const char *inp = "DATA:BLOB 13.456, #216AbcdEfghIjklMnop\nFREQ 50\r\n";
//	const char *inp = "STAT:QUE:ENAB?;ENAB \t   1;ENAB?;:*IDN?\n";

	for (int i = 0; i < strlen(inp); i++) {
		scpi_handle_byte(inp[i]);
	}

	char buf[250];
	scpi_error_string(buf, E_EXE_DATA_QUESTIONABLE, "The data smells fishy");

	printf("%s\n", buf);
}
