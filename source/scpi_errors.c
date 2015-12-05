#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "scpi_errors.h"
#include "scpi_regs.h"

#define ERR_QUEUE_LEN 4
#define MAX_ERROR_LEN 255

// --- queue impl ---


static struct {
	char queue[ERR_QUEUE_LEN][MAX_ERROR_LEN + 1];
	int8_t r_pos;
	int8_t w_pos;
	int8_t count; // signed for backtracking
} erq;


void scpi_add_error(SCPI_error_t errno, const char *extra)
{
	bool added = true;
	if (erq.count >= ERR_QUEUE_LEN) {
		errno = E_DEV_QUEUE_OVERFLOW;
		extra = NULL;
		added = false; // replaced only

		// backtrack
		erq.w_pos--;
		erq.count--;
		if (erq.w_pos < 0) {
			erq.w_pos = ERR_QUEUE_LEN - 1;
		}
	}

	scpi_error_string(erq.queue[erq.w_pos], errno, extra);

	erq.w_pos++;
	erq.count++;
	if (erq.w_pos >= ERR_QUEUE_LEN) {
		erq.w_pos = 0;
	}

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


typedef struct {
	const int16_t errno;
	const char *msg;
} SCPI_error_desc;


static const SCPI_error_desc error_table[] = {
	{ 0, "No error"},
	{ -100, "Command error"},
	{ -101, "Invalid character"},
	{ -102, "Syntax error"},
	{ -103, "Invalid separator"},
	{ -104, "Data type error"},
	{ -105, "GET not allowed"},
	{ -108, "Parameter not allowed"},
	{ -109, "Missing parameter"},
	{ -110, "Command header error"},
	{ -111, "Header separator error"},
	{ -112, "Program mnemonic too long"},
	{ -113, "Undefined header"},
	{ -114, "Header suffix out of range"},
	{ -115, "Unexpected number of parameters"},
	{ -120, "Numeric data error"},
	{ -121, "Invalid character in number"},
	{ -123, "Exponent too large"},
	{ -124, "Too many digits"},
	{ -128, "Numeric data not allowed"},
	{ -130, "Suffix error"},
	{ -131, "Invalid suffix"},
	{ -134, "Suffix too long"},
	{ -138, "Suffix not allowed"},
	{ -140, "Character data error"},
	{ -141, "Invalid character data"},
	{ -144, "Character data too long"},
	{ -148, "Character data not allowed"},
	{ -150, "String data error"},
	{ -151, "Invalid string data"},
	{ -158, "String data not allowed"},
	{ -160, "Block data error"},
	{ -161, "Invalid block data"},
	{ -168, "Block data not allowed"},
	{ -170, "Expression error"},
	{ -171, "Invalid expression"},
	{ -178, "Expression data not allowed"},
	{ -180, "Macro error"},
	{ -181, "Invalid outside macro definition"},
	{ -183, "Invalid inside macro definition"},
	{ -184, "Macro parameter error"},
	{ -200, "Execution error"},
	{ -201, "Invalid while in local"},
	{ -202, "Settings lost due to rtl"},
	{ -203, "Command protected"},
	{ -210, "Trigger error"},
	{ -211, "Trigger ignored"},
	{ -212, "Arm ignored"},
	{ -213, "Init ignored"},
	{ -214, "Trigger deadlock"},
	{ -215, "Arm deadlock"},
	{ -220, "Parameter error"},
	{ -221, "Settings conflict"},
	{ -222, "Data out of range"},
	{ -223, "Too much data"},
	{ -224, "Illegal parameter value"},
	{ -225, "Out of memory"},
	{ -226, "Lists not same length"},
	{ -230, "Data corrupt or stale"},
	{ -231, "Data questionable"},
	{ -232, "Invalid format"},
	{ -233, "Invalid version"},
	{ -240, "Hardware error"},
	{ -241, "Hardware missing"},
	{ -250, "Mass storage error"},
	{ -251, "Missing mass storage"},
	{ -252, "Missing media"},
	{ -253, "Corrupt media"},
	{ -254, "Media full"},
	{ -255, "Directory full"},
	{ -256, "File name not found"},
	{ -257, "File name error"},
	{ -258, "Media protected"},
	{ -260, "Expression error"},
	{ -261, "Math error in expression"},
	{ -270, "Macro error"},
	{ -271, "Macro syntax error"},
	{ -272, "Macro execution error"},
	{ -273, "Illegal macro label"},
	{ -274, "Macro parameter error"},
	{ -275, "Macro definition too long"},
	{ -276, "Macro recursion error"},
	{ -277, "Macro redefinition not allowed"},
	{ -278, "Macro header not found"},
	{ -280, "Program error"},
	{ -281, "Cannot create program"},
	{ -282, "Illegal program name"},
	{ -283, "Illegal variable name"},
	{ -284, "Program currently running"},
	{ -285, "Program syntax error"},
	{ -286, "Program runtime error"},
	{ -290, "Memory use error"},
	{ -291, "Out of memory"},
	{ -292, "Referenced name does not exist"},
	{ -293, "Referenced name already exists"},
	{ -294, "Incompatible type"},
	{ -300, "Device-specific error"},
	{ -310, "System error"},
	{ -311, "Memory error"},
	{ -312, "PUD memory lost"},
	{ -313, "Calibration memory lost"},
	{ -314, "Save/recall memory lost"},
	{ -315, "Configuration memory lost"},
	{ -320, "Storage fault"},
	{ -321, "Out of memory"},
	{ -330, "Self-test failed"},
	{ -340, "Calibration failed"},
	{ -350, "Queue overflow"},
	{ -360, "Communication error"},
	{ -361, "Parity error in program message"},
	{ -362, "Framing error in program message"},
	{ -363, "Input buffer overrun"},
	{ -365, "Time out error"},
	{ -400, "Query error"},
	{ -410, "Query INTERRUPTED"},
	{ -420, "Query UNTERMINATED"},
	{ -430, "Query DEADLOCKED"},
	{ -440, "Query UNTERMINATED after indefinite response"},
	{ -500, "Power on"},
	{ -600, "User request"},
	{ -700, "Request control"},
	{ -800, "Operation complete"},
	{ -9999} // end mark
};


const char * scpi_error_message(SCPI_error_t errno)
{
	for (int i = 0; error_table[i].errno != -9999; i++) {
		if (error_table[i].errno == errno) {
			return error_table[i].msg;
		}
	}

	return NULL;
}


void scpi_error_string(char *buffer, SCPI_error_t errno, const char *extra)
{
	const char *msg = scpi_error_message(errno);

	// len - total length, offs - current segment length
	int len, offs;

	len = offs = sprintf(buffer, "%d,\"", errno); // <code>,"
	buffer += offs;

	if (msg == NULL) msg = "Unknown error";

	offs = sprintf(buffer, "%s", msg); // Error message
	len += offs;
	buffer += offs;

	if (extra != NULL) {
		// extra info
		offs = sprintf(buffer, "; "); // ;
		len += offs;
		buffer += offs;

		// copy in the extra string
		int ll = 250 - len - 2;
		int xlen = strlen(extra);
		if (ll > xlen) ll = xlen;
		strncpy(buffer, extra, ll); // Extra msg

		len += ll;
		buffer += ll;
	}

	sprintf(buffer, "\""); // "
}

