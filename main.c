#include <stdio.h>
#include <stdlib.h>
#include "scpi_parser.h"

void cmd_aIDNq_cb(const SCPI_argval_t *args);
void cmd_APPL_SIN_cb(const SCPI_argval_t *args);
void cmd_APPL_TRI_cb(const SCPI_argval_t *args);
void cmd_FREQ_cb(const SCPI_argval_t *args);

const uint16_t scpi_cmd_lang_len = 4;
const SCPI_command_t scpi_cmd_lang[4] = {
	{
		.level_cnt = 1, .levels = {"*IDN?"},
		.param_cnt = 0, .params = {},
		.callback = cmd_aIDNq_cb
	},
	{
		.level_cnt = 2, .levels = {"APPLy", "SINe"},
		.param_cnt = 3, .params = {SCPI_DT_INT, SCPI_DT_FLOAT, SCPI_DT_FLOAT},
		.callback = cmd_APPL_SIN_cb
	},
	{
		.level_cnt = 2, .levels = {"APPLy", "TRIangle"},
		.param_cnt = 3, .params = {SCPI_DT_INT, SCPI_DT_FLOAT, SCPI_DT_FLOAT},
		.callback = cmd_APPL_TRI_cb
	},
	{
		.level_cnt = 1, .levels = {"FREQuency"},
		.param_cnt = 1, .params = {SCPI_DT_INT},
		.callback = cmd_FREQ_cb
	},
};


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
	printf("cb  FREQuency\n");
}



int main(int argc, const char**argv)
{
	const char *inp = argv[1];

//	printf("%d\n", char_equals_ci('a','A'));
//	printf("%d\n", char_equals_ci('z','z'));
//	printf("%d\n", char_equals_ci('z','Z'));
//	printf("%d\n", char_equals_ci('M','M'));
//	printf("%d\n", char_equals_ci('M','q'));
//	printf("%d\n", char_equals_ci('*','*'));
//	printf("%d\n", char_equals_ci('a',' '));

//	printf("%d\n", level_str_matches("A","A"));
//	printf("%d\n", level_str_matches("AbCdE","ABCDE"));
//	printf("%d\n", level_str_matches("*IDN?","*IDN?"));
//	printf("%d\n", level_str_matches("*IDN?","*IDN"));
//	printf("%d\n", level_str_matches("MEAS","MEASure"));
	//printf("%d\n", level_str_matches("*FBAZ?","*FxyzBAZ?"));
}
