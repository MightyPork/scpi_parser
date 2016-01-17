#pragma once
#include <stdint.h>
#include <stdbool.h>

#define SCPI_MAX_CMD_LEN 16 // 12 according to spec
#define SCPI_MAX_STRING_LEN 64 // 12 according to spec
#define SCPI_MAX_LEVEL_COUNT 4
#define SCPI_MAX_PARAM_COUNT 4

/** Argument data types */
typedef enum {
	SCPI_DT_NONE = 0,
	SCPI_DT_FLOAT, // float with support for scientific notation
	SCPI_DT_INT, // integer (may be signed)
	SCPI_DT_BOOL, // 0, 1, ON, OFF
	SCPI_DT_CHARDATA, // string without quotes - [A-Za-z0-9_]
	SCPI_DT_STRING, // quoted string, max 12 chars; no escapes.
	SCPI_DT_BLOB, // binary block, callback: uint32_t holding number of bytes
} SCPI_datatype_t;


/** Arguemnt value (union) */
typedef union {
	float FLOAT;

	int32_t INT;
	uint32_t BLOB_LEN;

	bool BOOL;

	char STRING[SCPI_MAX_STRING_LEN + 1];
	char CHARDATA[SCPI_MAX_STRING_LEN + 1];
} SCPI_argval_t;


// ------ CONFIGURATION --------

/**
 * SCPI command preset
 * NOTE: command array is terminated by {0} - zero in levels[0][0]
 */
typedef struct {
	// levels MUST BE FIRST!
	const char levels[SCPI_MAX_LEVEL_COUNT][SCPI_MAX_CMD_LEN + 2]; // up to 4 parts (+? and \0)

	// called when the command is completed. BLOB arg must be last in the argument list,
	// and only the first part is collected.
	void (*callback)(const SCPI_argval_t *args);

	// Param types - optional (defaults to zeros)
	const SCPI_datatype_t params[SCPI_MAX_PARAM_COUNT]; // parameter types (0 for unused)

	// --- OPTIONAL (only for blob) ---

	// Number of bytes in a blob callback
	const uint8_t blob_chunk;
	// Blob chunk callback (every blob_chunk bytes)
	void (*blob_callback)(const uint8_t *bytes);
} SCPI_command_t;


// ---------------- USER CONFIG ----------------

/** Zero terminated command struct array - must be defined. */
extern const SCPI_command_t scpi_commands[];

/** Built-in SCPI commands, provided by scpi_builtins.h */
extern const SCPI_command_t scpi_commands_builtin[];

/** Send a byte to master (may be buffered) */
extern void scpi_send_byte_impl(uint8_t b);

/** Character sequence used as a newline in responses. */
extern const char *scpi_eol;


// --------------- functions --------------------

/**
 * SCPI parser entry point.
 * All incoming bytes should be sent to this function.
 */
void scpi_handle_byte(const uint8_t b);

/**
 * SCPI parser - handle a string (multiple chars) at once.
 * String is interpreted as is, nothing is added. Must be terminated with \0.
 */
void scpi_handle_string(const char* str);


/** Discard the rest of the currently processed blob */
void scpi_discard_blob(void);

/** Send a string to master. \r\n is added. */
void scpi_send_string(const char *message);

/** Send a string without a line terminator */
void scpi_send_string_raw(const char *message);

/** Clear the error queue */
void scpi_clear_errors(void);

