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
	PARS_DISCARD_WHITESPACE_ENDL,
	PARS_DISCARD_LINE, // used after detecting error
} parser_state_t;


/** parser internal state struct */
static struct {
	char err_queue[ERR_QUEUE_LEN][MAX_ERROR_LEN];
	uint8_t err_queue_i;

	parser_state_t state; // current parser internal state

	// string buffer, chars collected here until recognized
	char charbuf[256];
	uint16_t charbuf_i;

	// recognized complete command level strings (FUNCtion) - exact copy from command struct
	char cur_levels[MAX_LEVEL_COUNT][MAX_CMD_LEN];
	uint8_t cur_level_i; // next free level slot index

	bool cmdbuf_kept; // set to 1 after semicolon - cur_levels is kept (removed last part)

	const SCPI_command_t * matched_cmd; // command is put here after recognition, used as reference for args

	SCPI_argval_t args[MAX_PARAM_COUNT];
	uint8_t arg_i; // next free argument slot index
} pstate = {
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


// char matching
#define INRANGE(c, a, b) ((c) >= (a) && (c) <= (b))
#define IS_WHITESPACE(c) (INRANGE((c), 0, 9) || INRANGE((c), 11, 32))

#define IS_LCASE_CHAR(c) INRANGE((c), 'a', 'z')
#define IS_UCASE_CHAR(c) INRANGE((c), 'A', 'Z')
#define IS_NUMBER_CHAR(c) INRANGE((c), '0', '9')

#define IS_IDENT_CHAR(c) (IS_LCASE_CHAR((c)) || IS_UCASE_CHAR((c)) || IS_NUMBER_CHAR((c)) || (c) == '_' || (c) == '*' || (c) == '?')
#define IS_INT_CHAR(c) IS_NUMBER_CHAR((c))
#define IS_FLOAT_CHAR(c) (IS_NUMBER_CHAR((c)) || (c) == '.' || (c) == 'e' || (c) == 'E' || (e) == '+' || (e) == '-')

#define CHAR_TO_LOWER(ucase) ((ucase) + 32)
#define CHAR_TO_UPPER(lcase) ((lcase) - 32)



static void pars_reset_cmd(void)
{
	pstate.state = PARS_COMMAND;
	pstate.charbuf_i = 0;
	pstate.cur_level_i = 0;
	pstate.cmdbuf_kept = false;
	pstate.matched_cmd = NULL;
	pstate.arg_i = 0;
}


static void pars_reset_cmd_keeplevel(void)
{
	pstate.state = PARS_COMMAND;
	pstate.charbuf_i = 0;
	// rewind to last colon

	if (pstate.cur_level_i > 0) {
		pstate.cur_level_i--; // keep prev levels
	}

	pstate.cmdbuf_kept = true;

	pstate.matched_cmd = NULL;
	pstate.arg_i = 0;
}


void scpi_handle_byte(const uint8_t b)
{
	// TODO handle blob here
	const char c = (char) b;

	switch (pstate.state) {
		case PARS_COMMAND:
			// Collecting command

			if (pstate.charbuf_i == 0 && pstate.cur_level_i == 0) {
				if (IS_WHITESPACE(c)) {
					// leading whitespace is ignored
					break;
				}
			}


			if (IS_IDENT_CHAR(c)) {
				// valid command char

				if (pstate.charbuf_i < MAX_CMD_LEN) {
					pstate.charbuf[pstate.charbuf_i++] = c;
				} else {
					printf("ERROR command part too long.\n");//TODO error
					pstate.state = PARS_DISCARD_LINE;
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
//						pars_cmd_semicolon();
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


static void pars_cmd_colon(void)
{
	if (pstate.charbuf_i == 0) {
		// No command text before colon

		if (pstate.cur_level_i == 0 || pstate.cmdbuf_kept) {
			// top level command starts with colon (or after semicolon - reset level)
			pars_reset_cmd();
		} else {
			// colon after nothing - error
			printf("ERROR unexpected colon in command.\n");//TODO error
			pstate.state = PARS_DISCARD_LINE;
		}

	} else {
		// internal colon - partial match
		if (pars_match_cmd(true)) {
			printf("OK partial cmd, last segment = %s\n", pstate.cur_levels[pstate.cur_level_i - 1]);
		} else {
			printf("ERROR no such command (colon).\n");//TODO error
			pstate.state = PARS_DISCARD_LINE;
		}
	}
}


static void pars_cmd_newline(void)
{
	if (pstate.cur_level_i == 0 && pstate.charbuf_i == 0) {
		// nothing before newline
		pars_reset_cmd();
		return;
	}

	// complete match
	if (pars_match_cmd(false)) {
		if (pstate.matched_cmd->param_cnt == 0) {
			// no param command - OK
			pstate.matched_cmd->callback(pstate.args); // args are empty
		} else {
			printf("ERROR command missing arguments.\n");//TODO error
			pars_reset_cmd();
		}
	} else {
		printf("ERROR no such command (newline) %s.\n", pstate.charbuf);//TODO error
		pstate.state = PARS_DISCARD_LINE;
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


// proto
static bool try_match_cmd(const SCPI_command_t *cmd, bool partial);


static bool pars_match_cmd(bool partial)
{
	// terminate segment
	pstate.charbuf[pstate.charbuf_i] = '\0';
	pstate.charbuf_i = 0; // rewind

	char *dest = pstate.cur_levels[pstate.cur_level_i++];
	strcpy(dest, pstate.charbuf); // copy to level table

	for (uint16_t i = 0; i < scpi_cmd_lang_len; i++) {

		const SCPI_command_t *cmd = &scpi_cmd_lang[i];

		if (cmd->level_cnt > MAX_LEVEL_COUNT) {
			// FAIL, too deep. Bad config
			continue;
		}

		if (try_match_cmd(cmd, partial)) {
			if (partial) {
				// match found, OK
				return true;
			} else {
				// exact match found
				pstate.matched_cmd = cmd;
				return true;
			}
		}
	}

	return false;
}


/** Try to match current state to a given command */
static bool try_match_cmd(const SCPI_command_t *cmd, bool partial)
{
	if (pstate.cur_level_i > cmd->level_cnt) return false; // command too short
	if (pstate.cur_level_i == 0) return false; // nothing to match

	if (partial) {
		if (pstate.cur_level_i == cmd->level_cnt) {
			return false; // would be exact match
		}
	} else {
		if (pstate.cur_level_i != cmd->level_cnt) {
			return false; // can be only partial match
		}
	}

	// check for match up to current index
	for (uint8_t j = 0; j < pstate.cur_level_i; j++) {
		if (!level_str_matches(pstate.cur_levels[j], cmd->levels[j])) {
			return false;
		}
	}

	return true;
}
