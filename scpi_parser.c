#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include <stdio.h>
#include <stdlib.h>

#include "scpi_parser.h"
#include "scpi_errors.h"

// Config
#define ERR_QUEUE_LEN 4
#define MAX_ERROR_LEN 255
#define MAX_CHARBUF_LEN 255


// Char matching
#define INRANGE(c, a, b) ((c) >= (a) && (c) <= (b))
#define IS_WHITESPACE(c) (INRANGE((c), 0, 9) || INRANGE((c), 11, 32))

#define IS_LCASE_CHAR(c) INRANGE((c), 'a', 'z')
#define IS_UCASE_CHAR(c) INRANGE((c), 'A', 'Z')
#define IS_NUMBER_CHAR(c) INRANGE((c), '0', '9')
#define IS_MULTIPLIER_CHAR(c) ((c) == 'k' || (c) == 'M' || (c) == 'G' || (c) == 'm' || (c) == 'u' || (c) == 'n' || (c) == 'p')

#define IS_IDENT_CHAR(c) (IS_LCASE_CHAR((c)) || IS_UCASE_CHAR((c)) || IS_NUMBER_CHAR((c)) || (c) == '_' || (c) == '*' || (c) == '?')
#define IS_INT_CHAR(c) (IS_NUMBER_CHAR((c)) || (c) == '-' || (c) == '+' || IS_MULTIPLIER_CHAR((c)))
#define IS_FLOAT_CHAR(c) (IS_NUMBER_CHAR((c)) || (c) == '.' || (c) == 'e' || (c) == 'E' || (c) == '+' || (c) == '-' || IS_MULTIPLIER_CHAR((c)))

#define CHAR_TO_LOWER(ucase) ((ucase) + 32)
#define CHAR_TO_UPPER(lcase) ((lcase) - 32)



/** Parser internal state enum */
typedef enum {
	PARS_COMMAND = 0,
	// collect generic arg, terminated with comma or newline. Leading and trailing whitespace ignored.
	PARS_ARG, // generic argument (bool, float...)
	PARS_ARG_STRING, // collect arg - string (special treatment for quotes)
	PARS_ARG_BLOB_PREAMBLE, // #nDDD
	PARS_ARG_BLOB_DISCARD, // discard blob - same as BLOB_BODY, but no callback or buffering
	PARS_ARG_BLOB_BODY, // blob body, callback for each group
	PARS_TRAILING_WHITE, // command ready to run, waiting for end, only whitespace allowed
	PARS_TRAILING_WHITE_NOCB, // discard whitespace until end of line, don't run callback on success
	PARS_DISCARD_LINE, // used after detecting error - drop all chars until \n
} parser_state_t;



/** Parser internal state struct */
static struct {
	char err_queue[ERR_QUEUE_LEN][MAX_ERROR_LEN + 1];
	int8_t err_queue_r;
	int8_t err_queue_w;
	int8_t err_queue_used; // signed for backtracking

	parser_state_t state; // current parser internal state

	// string buffer, chars collected here until recognized
	char charbuf[MAX_CHARBUF_LEN + 1];
	uint16_t charbuf_i;

	int32_t blob_cnt; // preamble counter, if 0, was just #, must read count. Used also for blob body.
	int32_t blob_len; // total blob length to read

	char string_quote; // symbol used to quote string
	bool string_escape; // last char was backslash, next quote is literal

	// recognized complete command level strings (FUNCtion) - exact copy from command struct
	char cur_levels[SCPI_MAX_LEVEL_COUNT][SCPI_MAX_CMD_LEN];
	uint8_t cur_level_i; // next free level slot index

	bool cmdbuf_kept; // set to 1 after semicolon - cur_levels is kept (removed last part)

	const SCPI_command_t * matched_cmd; // command is put here after recognition, used as reference for args

	SCPI_argval_t args[SCPI_MAX_PARAM_COUNT];
	uint8_t arg_i; // next free argument slot index

} pst = {0}; // initialized by all zeros


// buffer for error messages
static char ebuf[256];

// response buffer
static char sbuf[256];

// ---------------- PRIVATE PROTOTYPES ------------------

// Command parsing
static void pars_cmd_colon(void); // colon starting a command sub-segment
static void pars_cmd_space(void); // space ending a command
static void pars_cmd_newline(void); // LF
static void pars_cmd_semicolon(void); // semicolon right after a command

// Command properties (find length of array)
static uint8_t cmd_param_count(const SCPI_command_t *cmd);
static uint8_t cmd_level_count(const SCPI_command_t *cmd);

static bool match_cmd(bool partial);
static bool match_any_cmd_from_array(const SCPI_command_t arr[], bool partial);
static bool match_cmd_do(const SCPI_command_t *cmd, bool partial);
static void run_command_callback(void);

// Argument parsing
static void pars_arg_char(char c);
static void pars_arg_comma(void);
static void pars_arg_newline(void);
static void pars_arg_semicolon(void);
static void pars_blob_preamble_char(uint8_t c);
static void arg_convert_value(void);

static void charbuf_terminate(void);
static void charbuf_append(char c);

// Reset
static void pars_reset_cmd(void);
static void pars_reset_cmd_keeplevel(void);



// ---------------- BUILTIN SCPI COMMANDS ------------------

static void builtin_CLS(const SCPI_argval_t *args)
{
	// clear the registers
	SCPI_REG_SESR.u8 = 0;
	SCPI_REG_OPER.u16 = 0;
	SCPI_REG_QUES.u16 = 0;
	scpi_clear_errors();

	if (scpi_user_CLS) {
		scpi_user_CLS();
	}

	scpi_status_update(); // flags
}


static void builtin_RST(const SCPI_argval_t *args)
{
	if (scpi_user_RST) {
		scpi_user_RST();
	}
}


static void builtin_TSTq(const SCPI_argval_t *args)
{
	if (scpi_user_TSTq) {
		scpi_user_TSTq();
	}
}


static void builtin_IDNq(const SCPI_argval_t *args)
{
	scpi_send_string(scpi_device_identifier());
}


static void builtin_ESE(const SCPI_argval_t *args)
{
	SCPI_REG_SESR_EN.u8 = (uint8_t) args[0].INT;
}


static void builtin_ESEq(const SCPI_argval_t *args)
{
	sprintf(sbuf, "%d", SCPI_REG_SESR_EN.u8);
	scpi_send_string(ebuf);
}


static void builtin_ESRq(const SCPI_argval_t *args)
{
	sprintf(sbuf, "%d", SCPI_REG_SESR.u8);
	scpi_send_string(ebuf);
}


static void builtin_OPC(const SCPI_argval_t *args)
{
	// implementation for instruments with no overlapping commands.
	// Can be overridden in the user commands.
	SCPI_REG_SESR.OPC = 1;
}


static void builtin_OPCq(const SCPI_argval_t *args)
{
	// implementation for instruments with no overlapping commands.
	// Can be overridden in the user commands.
	// (would be): sprintf(sbuf, "%d", SCPI_REG_SESR.OPC);

	scpi_send_string("1");
}


static void builtin_SRE(const SCPI_argval_t *args)
{
	SCPI_REG_SRE.u8 = (uint8_t) args[0].INT;
}


static void builtin_SREq(const SCPI_argval_t *args)
{
	sprintf(sbuf, "%d", SCPI_REG_SRE.u8);
	scpi_send_string(ebuf);
}


static void builtin_STBq(const SCPI_argval_t *args)
{
	sprintf(sbuf, "%d", SCPI_REG_STB.u8);
	scpi_send_string(ebuf);
}


static void builtin_WAI(const SCPI_argval_t *args)
{
	// no-op
}


static const SCPI_command_t scpi_commands_builtin[] = {
	// ---- COMMON COMMANDS ----
	{
		.levels = {"*CLS"},
		.callback = builtin_CLS
	},
	{
		.levels = {"*ESE"},
		.params = {SCPI_DT_INT},
		.callback = builtin_ESE
	},
	{
		.levels = {"*ESE?"},
		.callback = builtin_ESEq
	},
	{
		.levels = {"*ESR?"},
		.callback = builtin_ESRq
	},
	{
		.levels = {"*IDN?"},
		.callback = builtin_IDNq
	},
	{
		.levels = {"*OPC"},
		.callback = builtin_OPC
	},
	{
		.levels = {"*OPCq"},
		.callback = builtin_OPCq
	},
	{
		.levels = {"*RST"},
		.callback = builtin_RST
	},
	{
		.levels = {"*SRE"},
		.params = {SCPI_DT_INT},
		.callback = builtin_SRE
	},
	{
		.levels = {"*SRE?"},
		.callback = builtin_SREq
	},
	{
		.levels = {"*STB?"},
		.callback = builtin_STBq
	},
	{
		.levels = {"*WAI"},
		.callback = builtin_WAI
	},
	{
		.levels = {"*TST?"},
		.callback = builtin_TSTq
	},
	// ---- REQUIRED SYST COMMANDS ----


	{0} // end marker
};



// ------------------- MESSAGE SEND ------------------

/** Send a message to master. Trailing newline is added. */
void scpi_send_string(const char *message)
{
	char c;
	while ((c = *message++) != 0) {
		scpi_send_byte_impl(c);
	}

	scpi_send_byte_impl('\r');
	scpi_send_byte_impl('\n');
}



// ------------------- ERROR QUEUE ---------------------

void scpi_add_error(SCPI_error_t errno, const char *extra)
{
	bool added = true;
	if (pst.err_queue_used >= ERR_QUEUE_LEN) {
		errno = E_DEV_QUEUE_OVERFLOW;
		extra = NULL;
		added = false; // replaced only

		// backtrack
		pst.err_queue_w--;
		pst.err_queue_used--;
		if (pst.err_queue_w < 0) {
			pst.err_queue_w = ERR_QUEUE_LEN - 1;
		}
	}

	scpi_error_string(pst.err_queue[pst.err_queue_w], errno, extra);

	pst.err_queue_w++;
	pst.err_queue_used++;
	if (pst.err_queue_w >= ERR_QUEUE_LEN) {
		pst.err_queue_w = 0;
	}

	scpi_status_update();
}


void scpi_read_error(char *buf)
{
	if (pst.err_queue_used == 0) {
		scpi_error_string(buf, E_NO_ERROR, NULL);
		return;
	}

	strcpy(buf, pst.err_queue[pst.err_queue_r++]);
	pst.err_queue_used--;

	if (pst.err_queue_r >= ERR_QUEUE_LEN) {
		pst.err_queue_r = 0;
	}

	scpi_status_update();
}


void scpi_clear_errors(void)
{
	pst.err_queue_r = 0;
	pst.err_queue_w = 0;
	pst.err_queue_used = 0;

	scpi_status_update();
}


uint8_t scpi_error_count(void)
{
	return pst.err_queue_used;
}


static void err_no_such_command()
{
	char *b = ebuf;
	for (int i = 0; i < pst.cur_level_i; i++) {
		if (i > 0) b += sprintf(b, ":");
		b += sprintf(b, "%s", pst.cur_levels[i]);
	}

	scpi_add_error(E_CMD_UNDEFINED_HEADER, ebuf);
}


static void err_no_such_command_partial()
{
	char *b = ebuf;
	for (int i = 0; i < pst.cur_level_i; i++) {
		b += sprintf(b, "%s:", pst.cur_levels[i]);
	}

	scpi_add_error(E_CMD_UNDEFINED_HEADER, ebuf);
}


// ----------------- INPUT PARSING ----------------

void scpi_handle_byte(const uint8_t b)
{
	const char c = (char) b;

	switch (pst.state) {
		case PARS_COMMAND:
			// Collecting command

			if (IS_IDENT_CHAR(c)) {
				// valid command char

				if (pst.charbuf_i < SCPI_MAX_CMD_LEN) {
					charbuf_append(c);
				} else {
					scpi_add_error(E_CMD_PROGRAM_MNEMONIC_TOO_LONG, NULL);
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
						sprintf(ebuf, "Unexpected '%c' in command.", c);
						scpi_add_error(E_CMD_INVALID_CHARACTER, ebuf);
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
		case PARS_TRAILING_WHITE_NOCB:
			if (IS_WHITESPACE(c)) break;

			if (c == '\n') {
				if (pst.state != PARS_TRAILING_WHITE_NOCB) {
					run_command_callback();
				}

				pars_reset_cmd();
			} else {
				sprintf(ebuf, "Unexpected '%c' in trailing whitespace.", c);
				scpi_add_error(E_CMD_INVALID_CHARACTER, ebuf);
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

		case PARS_ARG_STRING:
			// string

			if (c == pst.string_quote && !pst.string_escape) {
				// end of string
				pst.state = PARS_ARG; // next will be newline or comma (or ignored spaces)
			} else if (c == '\n') {
				scpi_add_error(E_CMD_STRING_DATA_ERROR, "String not terminated (unexpected newline).");

				pst.state = PARS_DISCARD_LINE;
			} else {
				if (pst.string_escape) {
					charbuf_append(c);
					pst.string_escape = false;
				} else {
					if (c == '\\') {
						pst.string_escape = true;
					} else {
						charbuf_append(c);
					}
				}
			}
			break;

		case PARS_ARG_BLOB_PREAMBLE:
			// #<digits><dddddd><BLOB>
			pars_blob_preamble_char(c);
			break;

		case PARS_ARG_BLOB_BODY:
			// binary blob body with callback on buffer full

			charbuf_append(c);
			pst.blob_cnt++;

			if (pst.charbuf_i >= pst.matched_cmd->blob_chunk) {
				charbuf_terminate();

				if (pst.matched_cmd->blob_callback != NULL) {
					pst.matched_cmd->blob_callback((uint8_t *)pst.charbuf);
				}
			}

			if (pst.blob_cnt == pst.blob_len) {
				pst.state = PARS_TRAILING_WHITE_NOCB; // discard trailing whitespace until newline
			}

			break;

		case PARS_ARG_BLOB_DISCARD:
			// binary blob, discard incoming data

			pst.blob_cnt++;

			if (pst.blob_cnt == pst.blob_len) {
				pst.state = PARS_DISCARD_LINE;
			}

			break;
	}
}



// ------------------- RESET INTERNAL STATE ------------------


// public //
/** Discard the rest of the currently processed blob */
void scpi_discard_blob(void)
{
	if (pst.state == PARS_ARG_BLOB_BODY) {
		pst.state = PARS_ARG_BLOB_DISCARD;
	}
}


/** Reset parser state. */
static void pars_reset_cmd(void)
{
	pst.state = PARS_COMMAND;
	pst.charbuf_i = 0;
	pst.cur_level_i = 0;
	pst.cmdbuf_kept = false;
	pst.matched_cmd = NULL;
	pst.arg_i = 0;
	pst.string_escape = false;
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
	pst.string_escape = false;
}


// ------------------- COMMAND HELPERS -------------------

/** Get param count from command struct */
static uint8_t cmd_param_count(const SCPI_command_t *cmd)
{
	for (uint8_t i = 0; i < SCPI_MAX_PARAM_COUNT; i++) {
		if (cmd->params[i] == SCPI_DT_NONE) {
			return i;
		}
	}

	return SCPI_MAX_PARAM_COUNT;
}


/** Get level count from command struct */
static uint8_t cmd_level_count(const SCPI_command_t *cmd)
{
	for (uint8_t i = 0; i < SCPI_MAX_LEVEL_COUNT; i++) {
		if (cmd->levels[i][0] == 0) {
			return i;
		}
	}

	return SCPI_MAX_LEVEL_COUNT;
}



// ----------------- CHAR BUFFER HELPERS -------------------

/** Add a byte to charbuf, error on overflow */
static void charbuf_append(char c)
{
	if (pst.charbuf_i >= MAX_CHARBUF_LEN) {
		scpi_add_error(E_DEV_INPUT_BUFFER_OVERRUN, NULL);
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



// ----------------- PARSING COMMANDS ---------------

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
			scpi_add_error(E_CMD_SYNTAX_ERROR, "Unexpected colon.");

			pst.state = PARS_DISCARD_LINE;
		}

	} else {
		// internal colon - partial match
		if (match_cmd(true)) {
			// ok
		} else {
			// error
			err_no_such_command_partial();

			pst.state = PARS_DISCARD_LINE;
		}
	}
}


/** Semiolon received when collecting command parts */
static void pars_cmd_semicolon(void)
{
	if (pst.cur_level_i == 0 && pst.charbuf_i == 0) {
		// nothing before semicolon
		scpi_add_error(E_CMD_SYNTAX_ERROR, "Semicolon not preceded by command.");
		pars_reset_cmd();
		return;
	}

	if (match_cmd(false)) {
		int req_cnt = cmd_param_count(pst.matched_cmd);

		if (req_cnt == 0) {
			// no param command - OK
			run_command_callback();
			pars_reset_cmd_keeplevel(); // keep level - that's what semicolon does
		} else {
			sprintf(ebuf, "Required %d, got 0.", req_cnt);
			scpi_add_error(E_CMD_MISSING_PARAMETER, ebuf);
			pars_reset_cmd();
		}
	} else {
		err_no_such_command();
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
	if (match_cmd(false)) {
		int req_cnt = cmd_param_count(pst.matched_cmd);

		if (req_cnt == 0) {
			// no param command - OK
			run_command_callback();
			pars_reset_cmd();
		} else {
			// error
			sprintf(ebuf, "Required %d, got 0.", req_cnt);
			scpi_add_error(E_CMD_MISSING_PARAMETER, ebuf);

			pars_reset_cmd();
		}

	} else {
		err_no_such_command();
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

	if (match_cmd(false)) {
		if (cmd_param_count(pst.matched_cmd) == 0) {
			// no commands
			pst.state = PARS_TRAILING_WHITE;
		} else {
			pst.state = PARS_ARG;
		}
	} else {
		// error
		err_no_such_command();
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
	uint8_t pat_i, tst_i;
	bool long_started = false;
	for (pat_i = 0, tst_i = 0; pat_i < strlen(pattern); pat_i++) {
		if (tst_i > testlen) return false; // not match

		const char pat_c = pattern[pat_i];
		const char tst_c = test[tst_i]; // may be at the \0 terminator

		if (IS_LCASE_CHAR(pat_c)) {
			// optional char
			if (char_equals_ci(pat_c, tst_c)) {
				tst_i++; // advance test string
				long_started = true;
			} else {
				if (long_started) return false; // once long variant started, it must be completed.
			}

			continue; // next pi - tc stays in place
		} else {
			// require exact match (case insensitive)
			if (char_equals_ci(pat_c, tst_c)) {
				tst_i++;
			} else {
				return false;
			}
		}
	}

	return (tst_i >= testlen);
}



static bool match_cmd(bool partial)
{
	charbuf_terminate(); // zero-end and rewind index

	// copy to level table
	char *dest = pst.cur_levels[pst.cur_level_i++];
	strcpy(dest, pst.charbuf);


	// User commands are checked first, can override builtin commands
	if (match_any_cmd_from_array(scpi_commands, partial)) {
		return true;
	}

	// Try the built-in commands
	return match_any_cmd_from_array(scpi_commands_builtin, partial);
}


static bool match_any_cmd_from_array(const SCPI_command_t arr[], bool partial)
{
	for (uint16_t i = 0; i < 0xFFFF; i++) {

		const SCPI_command_t *cmd = &arr[i];
		if (cmd->levels[0][0] == 0) break; // end marker

		if (cmd_level_count(cmd) > SCPI_MAX_LEVEL_COUNT) {
			// FAIL, too deep. Bad config
			continue;
		}

		if (match_cmd_do(cmd, partial)) {
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
static bool match_cmd_do(const SCPI_command_t *cmd, bool partial)
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


/** Run the matched command's callback with the arguments */
static void run_command_callback(void)
{
	if (pst.matched_cmd != NULL) {
		pst.matched_cmd->callback(pst.args); // run
	}
}



// ---------------------- PARSING ARGS --------------------------

/** Non-whitespace and non-comma char received in arg. */
static void pars_arg_char(char c)
{
	switch (pst.matched_cmd->params[pst.arg_i]) {
		case SCPI_DT_FLOAT:
			if (!IS_FLOAT_CHAR(c)) {
				sprintf(ebuf, "'%c' not allowed in FLOAT.", c);
				scpi_add_error(E_CMD_INVALID_CHARACTER_IN_NUMBER, ebuf);

				pst.state = PARS_DISCARD_LINE;
			} else {
				charbuf_append(c);
			}
			break;

		case SCPI_DT_INT:
			if (!IS_INT_CHAR(c)) {
				sprintf(ebuf, "'%c' not allowed in INT.", c);
				scpi_add_error(E_CMD_INVALID_CHARACTER_IN_NUMBER, ebuf);

				pst.state = PARS_DISCARD_LINE;
			} else {
				charbuf_append(c);
			}
			break;

		case SCPI_DT_STRING:
			if (c == '\'' || c == '"') {
				pst.state = PARS_ARG_STRING;
				pst.string_quote = c;
				pst.string_escape = false;
			} else {
				scpi_add_error(E_CMD_INVALID_STRING_DATA, "Invalid quote, or chars after string.");
				pst.state = PARS_DISCARD_LINE;
			}
			break;

		case SCPI_DT_BLOB:
			if (c == '#') {
				pst.state = PARS_ARG_BLOB_PREAMBLE;
				pst.blob_cnt = 0;
			} else {
				scpi_add_error(E_CMD_INVALID_BLOCK_DATA, "Block data must start with #");
				pst.state = PARS_DISCARD_LINE;
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
		scpi_add_error(E_CMD_UNEXPECTED_NUMBER_OF_PARAMETERS, "Comma after last argument.");
		pst.state = PARS_DISCARD_LINE;
		return;
	}

	if (pst.charbuf_i == 0) {
		scpi_add_error(E_CMD_SYNTAX_ERROR, "Missing command before comma.");
		pst.state = PARS_DISCARD_LINE;
		return;
	}

	// Convert to the right type

	arg_convert_value();
}


// line ended with \n or ;
static void pars_arg_eol_do(bool keep_levels)
{
	int req_cnt = cmd_param_count(pst.matched_cmd);

	if (pst.arg_i < req_cnt - 1) {
		// not the last arg yet - fail

		if (pst.charbuf_i > 0) pst.arg_i++; // acknowledge the last arg

		sprintf(ebuf, "Required %d, got %d.", req_cnt, pst.arg_i);
		scpi_add_error(E_CMD_MISSING_PARAMETER, ebuf);

		pst.state = PARS_DISCARD_LINE;
		return;
	}

	arg_convert_value();
	run_command_callback();

	if (keep_levels) {
		pars_reset_cmd_keeplevel();
	} else {
		pars_reset_cmd(); // start a new command
	}
}


static void pars_arg_newline(void)
{
	pars_arg_eol_do(false);
}


static void pars_arg_semicolon(void)
{
	pars_arg_eol_do(true);
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
				sprintf(ebuf, "Invalid BOOL value: '%s'", pst.charbuf);
				scpi_add_error(E_CMD_NUMERIC_DATA_ERROR, ebuf);

				pst.state = PARS_DISCARD_LINE;
			}
			break;

		case SCPI_DT_FLOAT:
			j = sscanf(pst.charbuf, "%f", &dest->FLOAT);
			if (j == 0 || pst.charbuf[0] == '\0') { //fail or empty buffer
				sprintf(ebuf, "Invalid FLOAT value: '%s'", pst.charbuf);
				scpi_add_error(E_CMD_NUMERIC_DATA_ERROR, ebuf);

				pst.state = PARS_DISCARD_LINE;
			}
			break;

		case SCPI_DT_INT:
			j = sscanf(pst.charbuf, "%d", &dest->INT);

			if (j == 0 || pst.charbuf[0] == '\0') { //fail or empty buffer
				sprintf(ebuf, "Invalid INT value: '%s'", pst.charbuf);
				scpi_add_error(E_CMD_NUMERIC_DATA_ERROR, ebuf);

				pst.state = PARS_DISCARD_LINE;
			}
			break;

		case SCPI_DT_STRING:
			if (strlen(pst.charbuf) > SCPI_MAX_STRING_LEN) {
				scpi_add_error(E_CMD_INVALID_STRING_DATA, "String too long.");

				pst.state = PARS_DISCARD_LINE;
			} else {
				strcpy(dest->STRING, pst.charbuf); // copy the string
			}

			break;

		default:
			// impossible
			scpi_add_error(E_DEV_SYSTEM_ERROR, "Unexpected argument data type.");
			pst.state = PARS_DISCARD_LINE;
	}

	// proceed to next argument
	pst.arg_i++;
}


static void pars_blob_preamble_char(uint8_t c)
{
	if (pst.blob_cnt == 0) {
		if (!INRANGE(c, '1', '9')) {
			sprintf(ebuf, "Unexpected '%c' in binary data preamble.", c);
			scpi_add_error(E_CMD_BLOCK_DATA_ERROR, ebuf);

			pst.state = PARS_DISCARD_LINE;// (but not enough to remove the blob containing \n)
			return;
		}

		pst.blob_cnt = c - '0'; // 1-9
	} else {
		if (c == '\n') {
			scpi_add_error(E_CMD_BLOCK_DATA_ERROR, "Unexpected newline in binary data preamble.");

			pars_reset_cmd();
			return;
		}

		if (!IS_NUMBER_CHAR(c)) {
			sprintf(ebuf, "Unexpected '%c' in binary data preamble.", c);
			scpi_add_error(E_CMD_BLOCK_DATA_ERROR, ebuf);

			pst.state = PARS_DISCARD_LINE;
			return;
		}

		charbuf_append(c);
		if (--pst.blob_cnt == 0) {
			// end of preamble sequence
			charbuf_terminate();

			sscanf(pst.charbuf, "%d", &pst.blob_len);

			pst.args[pst.arg_i].BLOB_LEN = pst.blob_len;
			run_command_callback();

			// Call handler, enter special blob mode
			pst.state = PARS_ARG_BLOB_BODY;
			pst.blob_cnt = 0;
		}
	}
}
