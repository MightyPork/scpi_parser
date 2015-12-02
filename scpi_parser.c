#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include "scpi_parser.h"

#define MAX_CMD_LEN 12
#define MAX_STRING_LEN 12
#define MAX_LEVEL_COUNT 4
#define MAX_PARAM_COUNT 4

#define ERR_QUEUE_LEN 4
#define MAX_ERROR_LEN 100

/** Argument data types */
typedef enum {
	SCPI_DT_NONE = 0,
	SCPI_DT_FLOAT, // float with support for scientific notation
	SCPI_DT_INT, // integer (may be signed)
	SCPI_DT_BOOL, // 0, 1, ON, OFF
	SCPI_DT_STRING, // quoted string, max 12 chars; no escapes.
	SCPI_DT_BLOB, // binary block, callback: uint32_t holding number of bytes
} SCPI_datatype_t;


/** Arguemnt value (union) */
typedef union {
	float FLOAT;
	int32_t INT;
	bool BOOL;
	char STRING[MAX_STRING_LEN];
	uint32_t BLOB;
} SCPI_argval_t;


/** SCPI command preset */
typedef struct {
	const uint8_t level_cnt; // number of used command levels (colons + 1)
	const char levels[MAX_LEVEL_COUNT][MAX_CMD_LEN]; // up to 4 parts

	const bool quest; // command ends with a question mark

	const uint8_t param_cnt; // parameter count
	const SCPI_datatype_t params[MAX_PARAM_COUNT]; // parameter types (0 for unused)

	// called when the command is completed. BLOB arg must be last in the argument list,
	// and only the first part is collected.
	void (*callback)(const SCPI_argval_t * args);
} SCPI_command_t;


typedef enum {
	PARS_COMMAND,
	PARS_ARG, // collect generic arg, terminated with comma or newline. Leading and trailing whitespace ignored.
	PARS_ARG_STR_APOS, // collect arg - string with single quotes
	PARS_ARG_STR_QUOT, // collect arg - string with double quotes
	PARS_ARG_BLOB_PREAMBLE,
	PARS_DISCARD_LINE, // used after detecting error
} parser_state_t;


/** parser internal state struct */
static struct {
	char err_queue[ERR_QUEUE_LEN][MAX_ERROR_LEN];
	uint8_t err_queue_used;

	parser_state_t state; // current parser internal state

	// string buffer, chars collected here until recognized
	char charbuf[256];
	uint16_t charbuf_ptr;

	// command buffer, built from recognized parts and colons.
	char cmdbuf[(MAX_CMD_LEN + 1)*MAX_LEVEL_COUNT + 1]; // :COMMAND for each level + zero terminator
	uint16_t cmdbuf_ptr;
	bool cmdbuf_kept; // set to 1 after semicolon

	parser_state_t *detected_cmd; // command is put here after recognition, used as reference for args
	uint8_t arg_i; // current argument index (0-based)
} pstate = {
	// defaults
	.err_queue_used = 0,
	.state = PARS_COMMAND,
	.charbuf_ptr = 0,
	.cmdbuf_ptr = 0,
	.cmdbuf_kept = false,
	.detected_cmd = NULL,
	.arg_i = 0
};



static void pars_cmd_colon(void); // colon starting a command sub-segment
static void pars_cmd_space(void); // space ending a command
static void pars_cmd_newline(void); // LF
static void pars_cmd_semicolon(void); // semicolon right after a command

#define INRANGE(x, a, b) ((x) >= (a) && (x) <= (b))
#define IS_WHITE(x) (INRANGE((x), 0, 9) || INRANGE((x), 11, 32))


static void pars_reset_cmd(void)
{
	pstate.state = PARS_COMMAND;
	pstate.charbuf_ptr = 0;
	pstate.cmdbuf_ptr = 0;
	pstate.cmdbuf_kept = false;
	pstate.detected_cmd = NULL;
	pstate.arg_i = 0;
}

static void pars_reset_cmd_keeplevel(void)
{
	pstate.state = PARS_COMMAND;
	pstate.charbuf_ptr = 0;
	// rewind to last colon

	pstate.cmdbuf[pstate.cmdbuf_ptr] = 0; // terminate
	const char *cp = strrchr(pstate.cmdbuf, ':'); // find last colon
	pstate.cmdbuf_ptr = (uint16_t) (cp - pstate.cmdbuf + 1); // location of one char past the colon

	// ↑ FIXME may be broken
	pstate.cmdbuf_kept = true;

	pstate.detected_cmd = NULL;
	pstate.arg_i = 0;
}


void scpi_receive_byte(const uint8_t b)
{
	// TODO handle blob here
	const char c = (char) b;

	switch (pstate.state) {
		case PARS_COMMAND:
			// Collecting command

			if (pstate.charbuf_ptr == 0) {
				if (IS_WHITE(c)) {
					// leading whitespace is ignored
					break;
				}
			}


			if (INRANGE(c, 'a', 'z') || INRANGE(c, 'A', 'Z') || INRANGE(c, '0', '9') || c == '_') {
				// valid command char
				if (pstate.charbuf_ptr < MAX_CMD_LEN) {
					pstate.charbuf[pstate.charbuf_ptr++] = c;
				}
			} else {
				// invalid or delimiter

				if (IS_WHITE(c)) {
					pars_cmd_space(); // whitespace in command - end of command?
					break;
				}

				switch (c) {
					case ':':
						pars_cmd_colon(); // end of a section
						break;

					case '\n': // line terminator
						pars_cmd_newline();
						break;

					case ';': // ends a command, does not reset cmd path.
						pars_cmd_semicolon();
						break;

					default:
						printf("ERROR unexpected char '%c' in command.\n", c);//TODO error
						pstate.state = PARS_DISCARD_LINE;
				}
			}
			break;

		case PARS_DISCARD_LINE:
			// drop it. Clear state on newline.
			if (c == '\r' || c == '\n') {
				pars_reset_cmd();
			}
			break;

			// TODO
	}
}


// whitespace (INRANGE(c, 0, 9) || INRANGE(c, 11, 32))


static void pars_cmd_colon(void)
{
	if (pstate.charbuf_ptr == 0) {
		// No command text before colon

		// TODO FIXME should keep parsed command parts in array rather than a combined string

		if (pstate.cmdbuf_ptr == 0 || pstate.cmdbuf_kept) {
			// top level command starts with optional colon (or after semicolon reset level)
			pars_reset_cmd();
		} else {
			// colon after nothing - error
			printf("ERROR unexpected colon in command.\n");//TODO error
			pstate.state = PARS_DISCARD_LINE;
		}

	} else {
		while(pstate.charbuf_ptr > 0 && IS_WHITE(pstate.charbuf[pstate.charbuf_ptr - 1])) {
			pstate.charbuf_ptr--;
		}

		if (pstate.charbuf_ptr){
			printf("ERROR only whitespace before colon.\n");//TODO error
			pstate.state = PARS_DISCARD_LINE; // this error shouldn't happen
			return;
		}

		pstate.charbuf[pstate.charbuf_ptr] = '\0'; // terminator

	}
}


static void pars_cmd_space(void); // space ending a command
static void pars_cmd_newline(void); // LF
static void pars_cmd_semicolon(void); // semicolon right after a command















//----------------------- TESTS ----------------------

void cmd_aIDNq_cb(const SCPI_argval_t *args);
void cmd_APPL_SIN_cb(const SCPI_argval_t *args);
void cmd_APPL_TRI_cb(const SCPI_argval_t *args);
void cmd_FREQ_cb(const SCPI_argval_t *args);


const SCPI_command_t lang[] = {
	{
		.level_cnt = 1, .levels = {"*IDN"},
		.quest = true,
		.param_cnt = 0, .params = {},
		.callback = cmd_aIDNq_cb
	},
	{
		.level_cnt = 2, .levels = {"APPLy", "SINe"},
		.quest = false,
		.param_cnt = 3, .params = {SCPI_DT_INT, SCPI_DT_FLOAT, SCPI_DT_FLOAT},
		.callback = cmd_APPL_SIN_cb
	},
	{
		.level_cnt = 2, .levels = {"APPLy", "TRIangle"},
		.quest = false,
		.param_cnt = 3, .params = {SCPI_DT_INT, SCPI_DT_FLOAT, SCPI_DT_FLOAT},
		.callback = cmd_APPL_TRI_cb
	},
	{
		.level_cnt = 1, .levels = {"FREQuency"},
		.quest = false,
		.param_cnt = 1, .params = {SCPI_DT_INT},
		.callback = cmd_FREQ_cb
	},
};


int main(int argc, const char**argv)
{
	const char *inp = argv[1];
}


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
