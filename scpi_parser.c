#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "scpi_parser.h"

#include <stdio.h>
#include <stdlib.h>


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
	uint8_t err_queue_ptr;

	parser_state_t state; // current parser internal state

	// string buffer, chars collected here until recognized
	char charbuf[256];
	uint16_t charbuf_ptr;

	// recognized complete command level strings (FUNCtion) - exact copy from command struct
	char cur_levels[MAX_LEVEL_COUNT][MAX_CMD_LEN];
	uint8_t cur_level_ptr; // next free level slot index

	bool cmdbuf_kept; // set to 1 after semicolon - cur_levels is kept (removed last part)

	SCPI_command_t *detected_cmd; // command is put here after recognition, used as reference for args

	SCPI_argval_t args[MAX_PARAM_COUNT];
	uint8_t arg_ptr; // next free argument slot index
} pstate = {
	// defaults
	.err_queue_ptr = 0,
	.state = PARS_COMMAND,
	.charbuf_ptr = 0,
	.cur_level_ptr = 0,
	.cmdbuf_kept = false,
	.detected_cmd = NULL,
	.arg_ptr = 0
};



static void pars_cmd_colon(void); // colon starting a command sub-segment
static void pars_cmd_space(void); // space ending a command
static void pars_cmd_newline(void); // LF
static void pars_cmd_semicolon(void); // semicolon right after a command
static void pars_match_level(void); // match charbuf content to a level, advance level ptr (or fail)

// char matching
#define INRANGE(c, a, b) ((c) >= (a) && (c) <= (b))
#define IS_WHITESPACE(c) (INRANGE((c), 0, 9) || INRANGE((c), 11, 32))
#define IS_IDENT_CHAR(c) (INRANGE((c), 'a', 'z') || INRANGE((c), 'A', 'Z') || INRANGE((c), '0', '9') || (c) == '_')
#define IS_INT_CHAR(c) INRANGE((c), '0', '9')
#define IS_FLOAT_CHAR(c) (IS_INT_CHAR((c)) || (c) == '.' || (c) == 'e' || (c) == 'E' || (e) == '+' || (e) == '-')


static void pars_reset_cmd(void)
{
	pstate.state = PARS_COMMAND;
	pstate.charbuf_ptr = 0;
	pstate.cur_level_ptr = 0;
	pstate.cmdbuf_kept = false;
	pstate.detected_cmd = NULL;
	pstate.arg_ptr = 0;
}

static void pars_reset_cmd_keeplevel(void)
{
	pstate.state = PARS_COMMAND;
	pstate.charbuf_ptr = 0;
	// rewind to last colon

	if (pstate.cur_level_ptr > 0) {
		pstate.cur_level_ptr--; // keep prev levels
	}

	pstate.cmdbuf_kept = true;

	pstate.detected_cmd = NULL;
	pstate.arg_ptr = 0;
}


void scpi_receive_byte(const uint8_t b)
{
	// TODO handle blob here
	const char c = (char) b;

	switch (pstate.state) {
		case PARS_COMMAND:
			// Collecting command

			if (pstate.charbuf_ptr == 0 && pstate.cur_level_ptr == 0) {
				if (IS_WHITESPACE(c)) {
					// leading whitespace is ignored
					break;
				}
			}


			if (IS_IDENT_CHAR(c)) {
				// valid command char

				if (pstate.charbuf_ptr < MAX_CMD_LEN) {
					pstate.charbuf[pstate.charbuf_ptr++] = c;
				} else {
					printf("ERROR command part too long.\n");//TODO error
					pstate.state = PARS_DISCARD_LINE;
				}

			} else {
				// invalid or delimiter

				if (IS_WHITESPACE(c)) {
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

		if (pstate.cur_level_ptr == 0 || pstate.cmdbuf_kept) {
			// top level command starts with colon (or after semicolon - reset level)
			pars_reset_cmd();
		} else {
			// colon after nothing - error
			printf("ERROR unexpected colon in command.\n");//TODO error
			pstate.state = PARS_DISCARD_LINE;
		}

	} else {
		// internal colon
		pars_match_level();
	}
}


static void pars_match_level(void)
{
	// terminate segment
	pstate.charbuf[pstate.charbuf_ptr] = '\0';

	//TODO
}

