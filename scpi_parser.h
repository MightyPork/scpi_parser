#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "scpi_errors.h"
#include "scpi_regs.h"

#define SCPI_MAX_CMD_LEN 12
#define SCPI_MAX_STRING_LEN 12
#define SCPI_MAX_LEVEL_COUNT 4
#define SCPI_MAX_PARAM_COUNT 4

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
	char STRING[SCPI_MAX_STRING_LEN + 1]; // terminator
	uint32_t BLOB_LEN;
} SCPI_argval_t;


// ------ CONFIGURATION --------

/**
 * SCPI command preset
 * NOTE: command array is terminated by {0} - zero in levels[0][0]
 */
typedef struct {
	// levels MUST BE FIRST!
	const char levels[SCPI_MAX_LEVEL_COUNT][SCPI_MAX_CMD_LEN]; // up to 4 parts

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

// Zero terminated command struct array - must be defined.
extern const SCPI_command_t scpi_commands[];

/** Send a byte to master (may be buffered) */
extern void scpi_send_byte_impl(uint8_t b);

/** *CLS command callback - clear non-SCPI device state */
extern __attribute__((weak)) void scpi_user_CLS(void);

/** *RST command callback - reset non-SCPI device state */
extern __attribute__((weak)) void scpi_user_RST(void);

/** *TST? command callback - perform self test and send response back. */
extern __attribute__((weak)) void scpi_user_TSTq(void);

/** Get device identifier. Implemented as a callback to allow dynamic serial number */
extern const char *scpi_device_identifier(void);



// --------------- functions --------------------

/**
 * SCPI parser entry point.
 * All incoming bytes should be sent to this function.
 */
void scpi_handle_byte(const uint8_t b);

/** Add error to the error queue */
void scpi_add_error(SCPI_error_t errno, const char *extra);

/** Get number of errors in the error queue */
uint8_t scpi_error_count(void);

/**
 * Read and remove one entry from the error queue.
 * Returns 0,"No error" if the queue is empty.
 *
 * The entry is copied to the provided buffer, which must be 256 chars long.
 */
void scpi_read_error(char *buf);

/** Discard the rest of the currently processed blob */
void scpi_discard_blob(void);

/** Send a string to master. \r\n is added. */
void scpi_send_string(const char *message);

/** Clear the error queue */
void scpi_clear_errors(void);

