#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "scpi_errors.h"
#include "scpi_regs.h"

#define ERR_QUEUE_LEN 4
#define MAX_ERROR_LEN 150

// --- queue impl ---


static struct ErrorQueueStruct {
	char queue[ERR_QUEUE_LEN][MAX_ERROR_LEN + 1];
	int8_t r_pos;
	int8_t w_pos;
	int8_t count; // signed for backtracking
} erq;


void scpi_add_error(int16_t errno, const char *extra)
{
	if (erq.count >= ERR_QUEUE_LEN) {
		errno = E_DEV_QUEUE_OVERFLOW;
		extra = NULL;

		// backtrack
		erq.w_pos--;
		erq.count--;
		if (erq.w_pos < 0) {
			erq.w_pos = ERR_QUEUE_LEN - 1;
		}
	}

	// get string & coerce errno to valid value
	errno = scpi_error_string(erq.queue[erq.w_pos], errno, extra);

	// run optional user error callback
	if (scpi_user_error) {
		scpi_user_error(errno, erq.queue[erq.w_pos]);
	}

	erq.w_pos++;
	erq.count++;
	if (erq.w_pos >= ERR_QUEUE_LEN) {
		erq.w_pos = 0;
	}

	// error type status flags
	if (errno >= -499 && errno <= -400) {
		SCPI_REG_SESR.QUERY_ERROR = true;
	} else if ((errno >= -399 && errno <= -300) || errno > 0) {
		SCPI_REG_SESR.DEV_ERROR = true;
	} else if (errno >= -299 && errno <= -200) {
		SCPI_REG_SESR.EXE_ERROR = true;
	} else if (errno >= -199 && errno <= -100) {
		SCPI_REG_SESR.CMD_ERROR = true;
	}

	// update the error queue bit and propagate the above flags
	scpi_status_update();
}


void scpi_read_error_noremove(char *buf)
{
	if (erq.count == 0) {
		scpi_error_string(buf, E_NO_ERROR, NULL);
		return;
	}

	strcpy(buf, erq.queue[erq.r_pos]);
}


void scpi_read_error(char *buf)
{
	if (erq.count == 0) {
		scpi_error_string(buf, E_NO_ERROR, NULL);
		return;
	}

	strcpy(buf, erq.queue[erq.r_pos++]);
	erq.count--;

	if (erq.r_pos >= ERR_QUEUE_LEN) {
		erq.r_pos = 0;
	}

	scpi_status_update();
}


void scpi_clear_errors(void)
{
	erq.r_pos = 0;
	erq.w_pos = 0;
	erq.count = 0;

	scpi_status_update();
}


uint8_t scpi_error_count(void)
{
	return erq.count;
}


// ---- table ----

static const SCPI_error_desc no_error_desc = {0, "No error"};

static const SCPI_error_desc scpi_std_errors[] = {
	{ -100, "Command error"},
	{ -110, "Command header error"},
	{ -120, "Numeric data error"},
	{ -130, "Suffix error"},
	{ -140, "Character data error"},
	{ -150, "String data error"},
	{ -160, "Block data error"},
	{ -170, "Expression error"},
	{ -180, "Macro error"},
	{ -200, "Execution error"},

	{ -210, "Trigger error"},
	{ -220, "Parameter error"},
	{ -230, "Data corrupt or stale"},
	{ -240, "Hardware error"},
	{ -250, "Mass storage error"},
	{ -260, "Expression error"},
	{ -270, "Macro error"},
	{ -280, "Program error"},
	{ -290, "Memory use error"},

	{ -300, "Device-specific error"},
	{ -310, "System error"},
	{ -320, "Storage fault"},
	{ -330, "Self-test failed"},
	{ -340, "Calibration failed"},
	{ -350, "Queue overflow"},
	{ -360, "Communication error"},

	{ -400, "Query error"},

	// Error codes that don't make much sense
#ifdef SCPI_WEIRD_ERRORS
	{ -410, "Query INTERRUPTED"},
	{ -420, "Query UNTERMINATED"},
	{ -430, "Query DEADLOCKED"},
	{ -440, "Query UNTERMINATED after indefinite response"},
	{ -500, "Power on"},
	{ -600, "User request"},
	{ -700, "Request control"},
	{ -800, "Operation complete"},
#endif

	// Fine error codes.
	// Turn off to save space
#ifdef SCPI_FINE_ERRORS
	{ -101, "Invalid character"},
	{ -102, "Syntax error"},
	{ -103, "Invalid separator"},
	{ -104, "Data type error"},
	{ -105, "GET not allowed"},
	{ -108, "Parameter not allowed"},
	{ -109, "Missing parameter"},

	{ -111, "Header separator error"},
	{ -112, "Program mnemonic too long"},
	{ -113, "Undefined header"},
	{ -114, "Header suffix out of range"},
	{ -115, "Unexpected number of parameters"},

	{ -121, "Invalid character in number"},
	{ -123, "Exponent too large"},
	{ -124, "Too many digits"},
	{ -128, "Numeric data not allowed"},

	{ -131, "Invalid suffix"},
	{ -134, "Suffix too long"},
	{ -138, "Suffix not allowed"},

	{ -141, "Invalid character data"},
	{ -144, "Character data too long"},
	{ -148, "Character data not allowed"},

	{ -151, "Invalid string data"},
	{ -158, "String data not allowed"},

	{ -161, "Invalid block data"},
	{ -168, "Block data not allowed"},

	{ -171, "Invalid expression"},
	{ -178, "Expression data not allowed"},

	{ -181, "Invalid outside macro definition"},
	{ -183, "Invalid inside macro definition"},
	{ -184, "Macro parameter error"},

	{ -201, "Invalid while in local"},
	{ -202, "Settings lost due to rtl"},
	{ -203, "Command protected"},

	{ -211, "Trigger ignored"},
	{ -212, "Arm ignored"},
	{ -213, "Init ignored"},
	{ -214, "Trigger deadlock"},
	{ -215, "Arm deadlock"},

	{ -221, "Settings conflict"},
	{ -222, "Data out of range"},
	{ -223, "Too much data"},
	{ -224, "Illegal parameter value"},
	{ -225, "Out of memory"},
	{ -226, "Lists not same length"},

	{ -231, "Data questionable"},
	{ -232, "Invalid format"},
	{ -233, "Invalid version"},

	{ -241, "Hardware missing"},

	{ -251, "Missing mass storage"},
	{ -252, "Missing media"},
	{ -253, "Corrupt media"},
	{ -254, "Media full"},
	{ -255, "Directory full"},
	{ -256, "File name not found"},
	{ -257, "File name error"},
	{ -258, "Media protected"},

	{ -261, "Math error in expression"},

	{ -271, "Macro syntax error"},
	{ -272, "Macro execution error"},
	{ -273, "Illegal macro label"},
	{ -274, "Macro parameter error"},
	{ -275, "Macro definition too long"},
	{ -276, "Macro recursion error"},
	{ -277, "Macro redefinition not allowed"},
	{ -278, "Macro header not found"},

	{ -281, "Cannot create program"},
	{ -282, "Illegal program name"},
	{ -283, "Illegal variable name"},
	{ -284, "Program currently running"},
	{ -285, "Program syntax error"},
	{ -286, "Program runtime error"},

	{ -291, "Out of memory"},
	{ -292, "Referenced name does not exist"},
	{ -293, "Referenced name already exists"},
	{ -294, "Incompatible type"},

	{ -311, "Memory error"},
	{ -312, "PUD memory lost"},
	{ -313, "Calibration memory lost"},
	{ -314, "Save/recall memory lost"},
	{ -315, "Configuration memory lost"},

	{ -321, "Out of memory"},
	{ -361, "Parity error in program message"},
	{ -362, "Framing error in program message"},
	{ -363, "Input buffer overrun"},
	{ -365, "Time out error"},
#endif

	{0} // end mark
};


static const SCPI_error_desc * find_error_desc(const SCPI_error_desc *table, int16_t errno)
{
	for (int i = 0; (i == 0 || table[i].errno != 0); i++) {
		if (table[i].errno == errno) {
			return &table[i];
		}
	}

	return NULL;
}


static const SCPI_error_desc * resolve_error_desc(int16_t errno)
{
	const SCPI_error_desc *desc;

	if (errno == 0) {
		// ok state
		return &no_error_desc;

	} else if (errno < 0) {
		// standard errors

		desc = find_error_desc(scpi_std_errors, errno);
		if (desc != NULL) return desc;

		// not found in table, use group-common error
		errno += -errno % 10; // round to ten

		desc = find_error_desc(scpi_std_errors, errno);
		if (desc != NULL) return desc;

		errno += -errno % 100; // round to hundred

		desc = find_error_desc(scpi_std_errors, errno);
		if (desc != NULL) return desc;

	} else {
		// user error

		desc = find_error_desc(scpi_user_errors, errno);
		if (desc != NULL) return desc;
	}

	return NULL;
}


/**
 * Get error string.
 *
 * @param buffer Buffer for storing the final string. Make sure it's big enough.
 * @param errno Error number
 * @param extra Extra information, appended after the generic message.
 *
 * @returns actual error code. Code may be coerced to closest defined code (categories: tens, hundreds)
 */
int16_t scpi_error_string(char *buffer, int16_t errno, const char *extra)
{
	const SCPI_error_desc *desc = resolve_error_desc(errno);
	const char *msg;

	if (desc != NULL) {
		errno = desc->errno;
		msg = desc->msg;
	} else {
		// bad error code
		msg = "Unknown error";
	}

	// Print.
	if (extra == NULL) {
		sprintf(buffer, "%d,\"%s\"", errno, msg);
	} else {
		sprintf(buffer, "%d,\"%s; %s\"", errno, msg, extra);
	}

	return errno;
}

