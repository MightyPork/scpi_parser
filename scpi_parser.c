#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "scpi_parser.h"

#include <stdio.h>
#include <stdlib.h>

typedef enum {
	PARS_COMMAND,
	// collect generic arg, terminated with comma or newline. Leading and trailing whitespace ignored.
	PARS_ARG,
	PARS_ARG_STR_APOS, // collect arg - string with single quotes
	PARS_ARG_STR_QUOT, // collect arg - string with double quotes
	PARS_ARG_BLOB_PREAMBLE,
	// command with no args terminated by whitespace, discard whitespace until newline and then run callback.
	// error on non-whitespace
	PARS_TRAILING_WHITE,
	PARS_DISCARD_LINE, // used after detecting error
} parser_state_t;

#define MAX_CHARBUF_LEN 256

/** parser internal state struct */
static struct {
	char err_queue[ERR_QUEUE_LEN][MAX_ERROR_LEN];
	uint8_t err_queue_i;

	parser_state_t state; // current parser internal state

	// string buffer, chars collected here until recognized
	char charbuf[MAX_CHARBUF_LEN];
	uint16_t charbuf_i;

	int8_t blob_preamble_cnt; // preamble counter, if 0, was just #, must read count

	// recognized complete command level strings (FUNCtion) - exact copy from command struct
	char cur_levels[MAX_LEVEL_COUNT][MAX_CMD_LEN];
	uint8_t cur_level_i; // next free level slot index

	bool cmdbuf_kept; // set to 1 after semicolon - cur_levels is kept (removed last part)

	const SCPI_command_t * matched_cmd; // command is put here after recognition, used as reference for args

	SCPI_argval_t args[MAX_PARAM_COUNT];
	uint8_t arg_i; // next free argument slot index
} pst = {
	// defaults
	.err_queue_i = 0,
	.state = PARS_COMMAND,
	.charbuf_i = 0,
	.cur_level_i = 0,
	.cmdbuf_kept = false,
	.matched_cmd = NULL,
	.arg_i = 0
};


static void pars_cmd_colon(void); // colon starting a command sub-segment
static void pars_cmd_space(void); // space ending a command
static void pars_cmd_newline(void); // LF
static void pars_cmd_semicolon(void); // semicolon right after a command
static bool pars_match_cmd(bool partial);
static void pars_arg_char(char c);
static void pars_arg_comma(void);
static void pars_arg_newline(void);
static void pars_arg_semicolon(void);
static void pars_blob_preamble_char(uint8_t c);

static uint8_t cmd_param_count(const SCPI_command_t *cmd);
static uint8_t cmd_level_count(const SCPI_command_t *cmd);

static bool try_match_cmd(const SCPI_command_t *cmd, bool partial);
static void charbuf_terminate(void);
static void charbuf_append(char c);
static void pars_run_callback(void);
static void arg_convert_value(void);

// char matching
#define INRANGE(c, a, b) ((c) >= (a) && (c) <= (b))
#define IS_WHITESPACE(c) (INRANGE((c), 0, 9) || INRANGE((c), 11, 32))

#define IS_LCASE_CHAR(c) INRANGE((c), 'a', 'z')
#define IS_UCASE_CHAR(c) INRANGE((c), 'A', 'Z')
#define IS_NUMBER_CHAR(c) INRANGE((c), '0', '9')

#define IS_IDENT_CHAR(c) (IS_LCASE_CHAR((c)) || IS_UCASE_CHAR((c)) || IS_NUMBER_CHAR((c)) || (c) == '_' || (c) == '*' || (c) == '?')
#define IS_INT_CHAR(c) IS_NUMBER_CHAR((c))
#define IS_FLOAT_CHAR(c) (IS_NUMBER_CHAR((c)) || (c) == '.' || (c) == 'e' || (c) == 'E' || (c) == '+' || (c) == '-')

#define CHAR_TO_LOWER(ucase) ((ucase) + 32)
#define CHAR_TO_UPPER(lcase) ((lcase) - 32)



/** Reset parser state. */
static void pars_reset_cmd(void)
{
	pst.state = PARS_COMMAND;
	pst.charbuf_i = 0;
	pst.cur_level_i = 0;
	pst.cmdbuf_kept = false;
	pst.matched_cmd = NULL;
	pst.arg_i = 0;
}


/** Reset parser state, keep level (semicolon) */
static void pars_reset_cmd_keeplevel(void)
{
	pst.state = PARS_COMMAND;
	pst.charbuf_i = 0;
	// rewind to last colon

	if (pst.cur_level_i > 0) {
		pst.cur_level_i--; // keep prev levels
	}

	pst.cmdbuf_kept = true;

	pst.matched_cmd = NULL;
	pst.arg_i = 0;
}


static uint8_t cmd_param_count(const SCPI_command_t *cmd)
{
	for (uint8_t i = 0; i < MAX_PARAM_COUNT; i++) {
		if (cmd->params[i] == SCPI_DT_NONE) {
			return i;
		}
	}

	return MAX_PARAM_COUNT;
}


static uint8_t cmd_level_count(const SCPI_command_t *cmd)
{
	for (uint8_t i = 0; i < MAX_LEVEL_COUNT; i++) {
		if (cmd->levels[i][0] == 0) {
			return i;
		}
	}

	return MAX_LEVEL_COUNT;
}


/** Add a byte to charbuf, error on overflow */
static void charbuf_append(char c)
{
	if (pst.charbuf_i >= MAX_CHARBUF_LEN) {
		printf("ERROR string buffer overflow.\n");//TODO error
		pst.state = PARS_DISCARD_LINE;
	}

	pst.charbuf[pst.charbuf_i++] = c;
}


/** Terminate charbuf and rewind the pointer to start */
static void charbuf_terminate(void)
{
	pst.charbuf[pst.charbuf_i] = '\0';
	pst.charbuf_i = 0;
}


/** Run the matched command's callback with the arguments */
static void pars_run_callback(void)
{
	if (pst.matched_cmd != NULL) {
		pst.matched_cmd->callback(pst.args); // run
	}
}


void scpi_handle_byte(const uint8_t b)
{
	// TODO handle blob here
	const char c = (char) b;

	switch (pst.state) {
		case PARS_COMMAND:
			// Collecting command

			if (IS_IDENT_CHAR(c)) {
				// valid command char

				if (pst.charbuf_i < MAX_CMD_LEN) {
					charbuf_append(c);
				} else {
					printf("ERROR command part too long.\n");//TODO error
					pst.state = PARS_DISCARD_LINE;
				}

			} else {
				// invalid or delimiter

				if (IS_WHITESPACE(c)) {
					pars_cmd_space(); // whitespace in command - end of command, start of args (?)
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
						pst.state = PARS_DISCARD_LINE;
				}
			}
			break;

		case PARS_DISCARD_LINE:
			// drop it. Clear state on newline.
			if (c == '\r' || c == '\n') {
				pars_reset_cmd();
			}
			break;

		case PARS_TRAILING_WHITE:
			if (IS_WHITESPACE(c)) break;

			if (c == '\n') {
				pars_run_callback();
				pars_reset_cmd();
			} else {
				printf("ERROR unexpected char '%c' in trailing whitespace.\n", c);//TODO error
				pst.state = PARS_DISCARD_LINE;
			}

			break; // whitespace discarded

		case PARS_ARG:
			if (IS_WHITESPACE(c)) break; // discard

			switch (c) {
				case ',':
					pars_arg_comma();
					break;

				case '\n':
					pars_arg_newline();
					break;

				case ';':
					pars_arg_semicolon();
					break;

				default:
					pars_arg_char(c);
			}
			break;

			// TODO escape sequence in string

		case PARS_ARG_STR_APOS:
			if (c == '\'') {
				// end of string
				pst.state = PARS_ARG; // next will be newline or comma (or ignored spaces)
			} else if (c == '\n') {
				printf("ERROR string literal not terminated.\n");//TODO error
				pst.state = PARS_DISCARD_LINE;
			} else {
				charbuf_append(c);
			}
			break;

		case PARS_ARG_STR_QUOT:
			if (c == '"') {
				// end of string
				pst.state = PARS_ARG; // next will be newline or comma (or ignored spaces)
			} else if (c == '\n') {
				printf("ERROR string literal not terminated.\n");//TODO error
				pst.state = PARS_DISCARD_LINE;
			} else {
				charbuf_append(c);
			}
			break;

		case PARS_ARG_BLOB_PREAMBLE:
			// #<digits><dddddd><BLOB>
			pars_blob_preamble_char(c);
	}
}


/** Colon received when collecting command parts */
static void pars_cmd_colon(void)
{
	if (pst.charbuf_i == 0) {
		// No command text before colon

		if (pst.cur_level_i == 0 || pst.cmdbuf_kept) {
			// top level command starts with colon (or after semicolon - reset level)
			pars_reset_cmd();
		} else {
			// colon after nothing - error
			printf("ERROR unexpected colon in command.\n");//TODO error
			pst.state = PARS_DISCARD_LINE;
		}

	} else {
		// internal colon - partial match
		if (pars_match_cmd(true)) {
			// ok
		} else {
			printf("ERROR no such command (colon).\n");//TODO error
			pst.state = PARS_DISCARD_LINE;
		}
	}
}


static void pars_cmd_semicolon(void)
{
	if (pst.cur_level_i == 0 && pst.charbuf_i == 0) {
		// nothing before semicolon
		printf("ERROR semicolon not allowed here.\n");//TODO error
		pars_reset_cmd();
		return;
	}

	if (pars_match_cmd(false)) {
		if (cmd_param_count(pst.matched_cmd) == 0) {
			// no param command - OK
			pars_run_callback();
			pars_reset_cmd_keeplevel(); // keep level - that's what semicolon does
		} else {
			printf("ERROR command missing arguments.\n");//TODO error
			pars_reset_cmd();
		}
	} else {
		printf("ERROR no such command %s.\n", pst.charbuf);//TODO error
		pst.state = PARS_DISCARD_LINE;
	}
}


/** Newline received when collecting command - end command and execute. */
static void pars_cmd_newline(void)
{
	if (pst.cur_level_i == 0 && pst.charbuf_i == 0) {
		// nothing before newline
		pars_reset_cmd();
		return;
	}

	// complete match
	if (pars_match_cmd(false)) {
		if (cmd_param_count(pst.matched_cmd) == 0) {
			// no param command - OK
			pars_run_callback();
			pars_reset_cmd();
		} else {
			printf("ERROR command missing arguments.\n");//TODO error
			pars_reset_cmd();
		}
	} else {
		printf("ERROR no such command %s.\n", pst.charbuf);//TODO error
		pst.state = PARS_DISCARD_LINE;
	}
}


/** Whitespace received when collecting command parts */
static void pars_cmd_space(void)
{
	if (pst.cur_level_i == 0 && pst.charbuf_i == 0) {
		// leading whitespace, ignore
		return;
	}

	if (pars_match_cmd(false)) {
		if (cmd_param_count(pst.matched_cmd) == 0) {
			// no commands
			pst.state = PARS_TRAILING_WHITE;
		} else {
			pst.state = PARS_ARG;
		}
	} else {
		printf("ERROR no such command: %s.\n", pst.charbuf);//TODO error
		pst.state = PARS_DISCARD_LINE;
	}
}


/** Check if chars equal, ignore case */
static bool char_equals_ci(char a, char b)
{
	if (IS_LCASE_CHAR(a)) {

		if (IS_LCASE_CHAR(b)) {
			return a == b;
		} else if (IS_UCASE_CHAR(b)) {
			return a == CHAR_TO_LOWER(b);
		} else {
			return false;
		}

	} else if (IS_UCASE_CHAR(a)) {

		if (IS_UCASE_CHAR(b)) {
			return a == b;
		} else if (IS_LCASE_CHAR(b)) {
			return a == CHAR_TO_UPPER(b);
		} else {
			return false;
		}

	} else {
		return a == b; // exact match, not letters
	}
}


/** Check if command matches a pattern */
static bool level_str_matches(const char *test, const char *pattern)
{
	const uint8_t testlen = strlen(test);
	uint8_t pi, ti;
	for (pi = 0, ti = 0; pi < strlen(pattern); pi++) {
		if (ti > testlen) return false; // not match

		const char pc = pattern[pi];
		const char tc = test[ti]; // may be at the \0 terminator

		if (IS_LCASE_CHAR(pc)) {
			// optional char
			if (char_equals_ci(pc, tc)) {
				ti++; // advance test string
			}

			continue; // next pi - tc stays in place
		} else {
			// require exact match (case insensitive)
			if (char_equals_ci(pc, tc)) {
				ti++;
			} else {
				return false;
			}
		}
	}

	return (ti >= testlen);
}



static bool pars_match_cmd(bool partial)
{
	charbuf_terminate(); // zero-end and rewind index

	// copy to level table
	char *dest = pst.cur_levels[pst.cur_level_i++];
	strcpy(dest, pst.charbuf);

	for (uint16_t i = 0; i < 0xFFFF; i++) {

		const SCPI_command_t *cmd = &scpi_cmd_lang[i];
		if (cmd->levels[0][0] == 0) break; // end marker

		if (cmd_level_count(cmd) > MAX_LEVEL_COUNT) {
			// FAIL, too deep. Bad config
			continue;
		}

		if (try_match_cmd(cmd, partial)) {
			if (partial) {
				// match found, OK
				return true;
			} else {
				// exact match found
				pst.matched_cmd = cmd;
				return true;
			}
		}
	}

	return false;
}


/** Try to match current state to a given command */
static bool try_match_cmd(const SCPI_command_t *cmd, bool partial)
{
	const uint8_t level_cnt = cmd_level_count(cmd);
	if (pst.cur_level_i > level_cnt) return false; // command too short
	if (pst.cur_level_i == 0) return false; // nothing to match

	if (partial) {
		if (pst.cur_level_i == level_cnt) {
			return false; // would be exact match
		}
	} else {
		if (pst.cur_level_i != level_cnt) {
			return false; // can be only partial match
		}
	}

	// check for match up to current index
	for (uint8_t j = 0; j < pst.cur_level_i; j++) {
		if (!level_str_matches(pst.cur_levels[j], cmd->levels[j])) {
			return false;
		}
	}

	return true;
}


/** Non-whitespace and non-comma char received in arg. */
static void pars_arg_char(char c)
{
	switch (pst.matched_cmd->params[pst.arg_i]) {
		case SCPI_DT_STRING:
			if (c == '\'') {
				pst.state = PARS_ARG_STR_APOS;
			} else if (c == '"') {
				pst.state = PARS_ARG_STR_QUOT;
			} else {
				printf("ERROR unexpected char '%c', should be ' or \"\n", c);//TODO error
				pst.state = PARS_DISCARD_LINE;
			}
			break;

		case SCPI_DT_BLOB:
			if (c == '#') {
				pst.state = PARS_ARG_BLOB_PREAMBLE;
				pst.blob_preamble_cnt = 0;
			} else {
				printf("ERROR unexpected char '%c', binary block should start with #\n", c);//TODO error
				pst.state = PARS_DISCARD_LINE;
			}
			break;

		case SCPI_DT_FLOAT:
			if (!IS_FLOAT_CHAR(c)) {
				printf("ERROR unexpected char '%c' in float.\n", c);//TODO error
				pst.state = PARS_DISCARD_LINE;
			} else {
				charbuf_append(c);
			}
			break;

		case SCPI_DT_INT:
			if (!IS_INT_CHAR(c)) {
				printf("ERROR unexpected char '%c' in int.\n", c);//TODO error
				pst.state = PARS_DISCARD_LINE;
			} else {
				charbuf_append(c);
			}
			break;

		default:
			charbuf_append(c);
			break;
	}
}


/** Received a comma while collecting an arg */
static void pars_arg_comma(void)
{
	if (pst.arg_i == cmd_param_count(pst.matched_cmd) - 1) {
		// it was the last argument
		// comma illegal
		printf("ERROR unexpected comma after the last argument\n");//TODO error
		pst.state = PARS_DISCARD_LINE;
		return;
	}

	if (pst.charbuf_i == 0) {
		printf("ERROR empty argument is not allowed.\n");//TODO error
		pst.state = PARS_DISCARD_LINE;
		return;
	}

	// Convert to the right type

	arg_convert_value();
}


static void pars_arg_newline(void)
{
	if (pst.arg_i < cmd_param_count(pst.matched_cmd) - 1) {
		// not the last arg yet - fail
		printf("ERROR not enough arguments!\n");//TODO error
		pst.state = PARS_DISCARD_LINE;
		return;
	}

	arg_convert_value();
	pars_run_callback();

	pars_reset_cmd(); // start a new command
}


static void pars_arg_semicolon(void)
{
	if (pst.arg_i < cmd_param_count(pst.matched_cmd) - 1) {
		// not the last arg yet - fail
		printf("ERROR not enough arguments!\n");//TODO error
		pst.state = PARS_DISCARD_LINE;
		return;
	}

	arg_convert_value();
	pars_run_callback();

	pars_reset_cmd_keeplevel(); // start a new command, keep level
}


/** Convert BOOL, FLOAT or INT char to arg type and advance to next */
static void arg_convert_value(void)
{
	charbuf_terminate();

	SCPI_argval_t *dest = &pst.args[pst.arg_i];
	int j;

	switch (pst.matched_cmd->params[pst.arg_i]) {
		case SCPI_DT_BOOL:
			if (strcasecmp(pst.charbuf, "1") == 0) {
				dest->BOOL = 1;
			} else if (strcasecmp(pst.charbuf, "0") == 0) {
				dest->BOOL = 0;
			} else if (strcasecmp(pst.charbuf, "ON") == 0) {
				dest->BOOL = 1;
			} else if (strcasecmp(pst.charbuf, "OFF") == 0) {
				dest->BOOL = 0;
			} else {
				printf("ERROR argument mismatch for type BOOL\n");//TODO error
				pst.state = PARS_DISCARD_LINE;
			}
			break;

		case SCPI_DT_FLOAT:
			j = sscanf(pst.charbuf, "%f", &dest->FLOAT);
			if (j == 0) {
				printf("ERROR failed to convert %s to FLOAT\n", pst.charbuf);//TODO error
				pst.state = PARS_DISCARD_LINE;
			}
			break;

		case SCPI_DT_INT:
			j = sscanf(pst.charbuf, "%d", &dest->INT);

			if (j == 0) {
				printf("ERROR failed to convert %s to INT\n", pst.charbuf);//TODO error
				pst.state = PARS_DISCARD_LINE;
			}
			break;

		case SCPI_DT_STRING:
			if (strlen(pst.charbuf) > MAX_STRING_LEN) {
				printf("ERROR string too long.\n");//TODO error
				pst.state = PARS_DISCARD_LINE;
			} else {
				strcpy(dest->STRING, pst.charbuf); // copy the string
			}

			break;

		default:
			// impossible
			printf("ERROR unexpected data type\n");//TODO error
			pst.state = PARS_DISCARD_LINE;
	}

	// proceed to next argument
	pst.arg_i++;
}

static void pars_blob_preamble_char(uint8_t c)
{
	if (pst.blob_preamble_cnt == 0) {
		if (!INRANGE(c, '1', '9')) {
			printf("ERROR expected ASCII 1-9 after #\n");//TODO error
			pst.state = PARS_DISCARD_LINE;
			return;
		}

		pst.blob_preamble_cnt = c - '0'; // 1-9
	} else {
		if (c == '\n') {
			printf("ERROR unexpected newline in blob preamble\n");//TODO error
			pars_reset_cmd();
			return;
		}

		if (!IS_INT_CHAR(c)) {
			printf("ERROR expected ASCII 0-9 after #n\n");//TODO error
			pst.state = PARS_DISCARD_LINE;
			return;
		}

		charbuf_append(c);
		if (--pst.blob_preamble_cnt == 0) {
			// end of preamble sequence
			charbuf_terminate();

			int bytecnt;
			sscanf(pst.charbuf, "%d", &bytecnt);

			printf("BLOB byte count = %d\n", bytecnt); // TODO

			// Call handler, enter special blob mode
			pst.state = PARS_DISCARD_LINE;//FIXME
		}
	}
}
