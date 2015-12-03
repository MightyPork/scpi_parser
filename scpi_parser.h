#pragma once
#include <stdint.h>
#include <stdbool.h>

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
	char STRING[MAX_STRING_LEN+1]; // terminator
	uint32_t BLOB_LEN;
} SCPI_argval_t;


/**
 * SCPI command preset
 * NOTE: command array is terminated by {0} - zero in levels[0][0]
 */
typedef struct {
	// levels MUST BE FIRST!
	const char levels[MAX_LEVEL_COUNT][MAX_CMD_LEN]; // up to 4 parts

	// called when the command is completed. BLOB arg must be last in the argument list,
	// and only the first part is collected.
	void (*callback)(const SCPI_argval_t *args);

	// Param types - optional (defaults to zeros)
	const SCPI_datatype_t params[MAX_PARAM_COUNT]; // parameter types (0 for unused)

	// --- OPTIONAL (only for blob) ---

	// Number of bytes in a blob callback
	const uint8_t blob_chunk;
	// Blob chunk callback (every blob_chunk bytes)
	void (*blob_callback)(const uint8_t *bytes);
} SCPI_command_t;

// Zero terminated command struct array
extern const SCPI_command_t scpi_cmd_lang[];


// --------------- functions --------------------

/**
 * SCPI parser entry point.
 * All incoming bytes should be sent to this function.
 */
void scpi_handle_byte(const uint8_t b);

