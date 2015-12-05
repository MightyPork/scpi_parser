#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "source/scpi_parser.h"


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

const SCPI_command_t scpi_commands[] = {
	{
		.levels = {"APPLy", "SINe"},
		.params = {SCPI_DT_INT, SCPI_DT_FLOAT, SCPI_DT_FLOAT},
		.callback = cmd_APPL_SIN_cb
	},
	{
		.levels = {"APPLy", "TRIangle"},
		.params = {SCPI_DT_BOOL, SCPI_DT_FLOAT, SCPI_DT_FLOAT},
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



void scpi_send_byte_impl(uint8_t b)
{
	putchar(b);
}


const char *scpi_device_identifier(void)
{
	return "FEL CVUT,DDS1,0,0.1";
}

int main()
{
	char buf[256];

//	SCPI_REG_QUEST.CURR = 1;
//	SCPI_REG_QUEST_EN.u16 = 0xFFFF;

//	printf("0x%x\n", SCPI_REG_QUEST.u16);

//	scpi_status_update();


//	return 0;


//	const char *inp = "*IDN?\n";
//	const char *inp = "FREQ 50\n";
//	const char *inp = "DISP:TEXT 'ban\\\\ana', OFF\nDISP:TEXT \"dblquot!\", 1\r\nFREQ 50\r\n";

	//const char *inp = "FOO\nDATA:BLOB 13.456, #216AbcdEfghIjklMnop\nFREQ 50\r\nAPPLY:MOO\n";
	//const char *inp = "STAT:QUE:ENAB?;ENAB \t   1;ENAB?;:*IDN?\n";

	const char *inp = "*idn?;*sre?;*cls;";

	for (int i = 0; i < strlen(inp); i++) {
		scpi_handle_byte(inp[i]);
	}


	//scpi_error_string(buf, E_EXE_DATA_QUESTIONABLE, "The data smells fishy");

	//scpi_read_error(buf), printf("%s\n", buf);

//	scpi_add_error(E_CMD_BLOCK_DATA_ERROR, NULL);

//	scpi_read_error(buf), printf("%s\n", buf);
//	scpi_read_error(buf), printf("%s\n", buf);
//	scpi_add_error(E_CMD_DATA_TYPE_ERROR, "LOL OOPS");
//	scpi_add_error(E_CMD_CHARACTER_DATA_TOO_LONG, "Too long data very overflow such fail");
//	scpi_add_error(E_CMD_MACRO_PARAMETER_ERROR, "maaaacrooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo");
//	scpi_add_error(E_CMD_EXPONENT_TOO_LARGE, NULL);
//	scpi_add_error(E_DEV_CALIBRATION_FAILED, NULL);
//	scpi_add_error(E_DEV_MEMORY_ERROR, NULL);
//	scpi_read_error(buf), printf("%s\n", buf);
//	scpi_read_error(buf), printf("%s\n", buf);
//	scpi_read_error(buf), printf("%s\n", buf);
//	scpi_read_error(buf), printf("%s\n", buf);
//	scpi_add_error(E_DEV_MEMORY_ERROR, "lel");
//	scpi_read_error(buf), printf("%s\n", buf);
//	scpi_read_error(buf), printf("%s\n", buf);
//	scpi_read_error(buf), printf("%s\n", buf);
//	scpi_read_error(buf), printf("%s\n", buf);
}
