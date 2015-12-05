#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "scpi_builtins.h"
#include "scpi_parser.h"
#include "scpi_errors.h"
#include "scpi_regs.h"

// response buffer
static char sbuf[256];


// ---------------- BUILTIN SCPI COMMANDS ------------------

static void builtin_CLS(const SCPI_argval_t *args)
{
	(void)args;

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
	(void)args;

	if (scpi_user_RST) {
		scpi_user_RST();
	}
}


static void builtin_TSTq(const SCPI_argval_t *args)
{
	(void)args;

	if (scpi_user_TSTq) {
		scpi_user_TSTq();
	}
}


static void builtin_IDNq(const SCPI_argval_t *args)
{
	(void)args;

	scpi_send_string(scpi_device_identifier());
}


static void builtin_ESE(const SCPI_argval_t *args)
{
	SCPI_REG_SESR_EN.u8 = (uint8_t) args[0].INT;
	scpi_status_update();
}


static void builtin_ESEq(const SCPI_argval_t *args)
{
	(void)args;

	sprintf(sbuf, "%d", SCPI_REG_SESR_EN.u8);
	scpi_send_string(sbuf);
}


static void builtin_ESRq(const SCPI_argval_t *args)
{
	(void)args;

	sprintf(sbuf, "%d", SCPI_REG_SESR.u8);
	scpi_send_string(sbuf);

	SCPI_REG_SESR.u8 = 0; // register cleared
	scpi_status_update();
}


static void builtin_OPC(const SCPI_argval_t *args)
{
	(void)args;

	// implementation for instruments with no overlapping commands.
	// Can be overridden in the user commands.
	SCPI_REG_SESR.OPC = 1;
	scpi_status_update();
}


static void builtin_OPCq(const SCPI_argval_t *args)
{
	(void)args;

	// implementation for instruments with no overlapping commands.
	// Can be overridden in the user commands.
	// (would be): sprintf(sbuf, "%d", SCPI_REG_SESR.OPC);

	scpi_send_string("1");
}


static void builtin_SRE(const SCPI_argval_t *args)
{
	SCPI_REG_SRE.u8 = (uint8_t) args[0].INT;
	scpi_status_update();
}


static void builtin_SREq(const SCPI_argval_t *args)
{
	(void)args;

	sprintf(sbuf, "%d", SCPI_REG_SRE.u8);
	scpi_send_string(sbuf);
}


static void builtin_STBq(const SCPI_argval_t *args)
{
	(void)args;

	sprintf(sbuf, "%d", SCPI_REG_STB.u8);
	scpi_send_string(sbuf);
}


static void builtin_WAI(const SCPI_argval_t *args)
{
	(void)args;

	// no-op
}


static void builtin_SYST_ERR_NEXTq(const SCPI_argval_t *args)
{
	(void)args;

	scpi_read_error(sbuf);
	scpi_send_string(sbuf);
}


// optional
static void builtin_SYST_ERR_ALLq(const SCPI_argval_t *args)
{
	(void)args;

	int cnt = 0;
	while (scpi_error_count()) {
		scpi_read_error(sbuf);
		if (cnt++ > 0) scpi_send_string_raw(",");
		scpi_send_string_raw(sbuf);
	}

	scpi_send_string_raw("\r\n"); // eol
}


static void builtin_SYST_ERR_CODE_NEXTq(const SCPI_argval_t *args)
{
	(void)args;

	scpi_read_error(sbuf);

	// end at comma
	for (int i = 0; i < 256; i++) {
		if (sbuf[i] == ',') {
			sbuf[i] = 0;
			break;
		}
	}

	scpi_send_string(sbuf);
}


// optional
static void builtin_SYST_ERR_CODE_ALLq(const SCPI_argval_t *args)
{
	(void)args;

	int cnt = 0;
	while (scpi_error_count()) {
		scpi_read_error(sbuf);
		if (cnt++ > 0) scpi_send_string_raw(",");

		// end at comma
		for (int i = 0; i < 256; i++) {
			if (sbuf[i] == ',') {
				sbuf[i] = 0;
				break;
			}
		}

		scpi_send_string_raw(sbuf);
	}

	scpi_send_string_raw("\r\n"); // eol
}


// optional
static void builtin_SYST_ERR_COUNq(const SCPI_argval_t *args)
{
	(void)args;

	sprintf(sbuf, "%d", scpi_error_count());
	scpi_send_string(sbuf);
}


// optional, custom
static void builtin_SYST_ERR_CLEAR(const SCPI_argval_t *args)
{
	(void)args;

	scpi_clear_errors();
}


static void builtin_SYST_VERSq(const SCPI_argval_t *args)
{
	(void)args;

	scpi_send_string("1999.0"); // implemented SCPI version
}


static void builtin_STAT_OPER_EVENq(const SCPI_argval_t *args)
{
	(void)args;

	// read and clear
	sprintf(sbuf, "%d", SCPI_REG_OPER.u16);
	SCPI_REG_OPER.u16 = 0x0000;
	scpi_send_string(sbuf);
	scpi_status_update();
}


static void builtin_STAT_OPER_CONDq(const SCPI_argval_t *args)
{
	(void)args;

	// read and keep
	sprintf(sbuf, "%d", SCPI_REG_OPER.u16);
	scpi_send_string(sbuf);
}


static void builtin_STAT_OPER_ENAB(const SCPI_argval_t *args)
{
	SCPI_REG_OPER.u16 = (uint16_t) args[0].INT; // set enable flags
	scpi_status_update();
}


static void builtin_STAT_OPER_ENABq(const SCPI_argval_t *args)
{
	(void)args;

	sprintf(sbuf, "%d", SCPI_REG_OPER_EN.u16);
	scpi_send_string(sbuf);
}


static void builtin_STAT_QUES_EVENq(const SCPI_argval_t *args)
{
	(void)args;

	// read and clear
	sprintf(sbuf, "%d", SCPI_REG_QUES.u16);
	SCPI_REG_QUES.u16 = 0x0000;
	scpi_send_string(sbuf);
	scpi_status_update();
}


static void builtin_STAT_QUES_CONDq(const SCPI_argval_t *args)
{
	(void)args;

	// read and keep
	sprintf(sbuf, "%d", SCPI_REG_QUES.u16);
	scpi_send_string(sbuf);
}


static void builtin_STAT_QUES_ENAB(const SCPI_argval_t *args)
{
	SCPI_REG_QUES.u16 = (uint16_t) args[0].INT; // set enable flags
	scpi_status_update();
}


static void builtin_STAT_QUES_ENABq(const SCPI_argval_t *args)
{
	(void)args;

	sprintf(sbuf, "%d", SCPI_REG_QUES_EN.u16);
	scpi_send_string(sbuf);
}


static void builtin_STAT_PRES(const SCPI_argval_t *args)
{
	(void)args;

	// Command required by SCPI spec, useless for this SCPI implementation.
	// This is meant to preset transition and filter registers, which are not used here.

	// Do not use this command, only defined to satisfy the spec.

	SCPI_REG_QUES_EN.u16 = 0;
	SCPI_REG_OPER_EN.u16 = 0;
	scpi_status_update();
}


const SCPI_command_t scpi_commands_builtin[] = {
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

	// ---- REQUIRED SUBSYSTEM COMMANDS ----

	// SYSTem
	{
		.levels = {"SYSTem", "ERRor?"},
		.callback = builtin_SYST_ERR_NEXTq
	},
	{
		.levels = {"SYSTem", "ERRor", "NEXT?"},
		.callback = builtin_SYST_ERR_NEXTq
	},
	{
		.levels = {"SYSTem", "ERRor", "ALL?"}, // optional
		.callback = builtin_SYST_ERR_ALLq
	},
	{
		.levels = {"SYSTem", "ERRor", "CLEAR"}, // custom, added for convenience
		.callback = builtin_SYST_ERR_CLEAR
	},
	{
		.levels = {"SYSTem", "ERRor", "COUNt?"}, // optional
		.callback = builtin_SYST_ERR_COUNq
	},
	{
		.levels = {"SYSTem", "ERRor", "CODE?"}, // optional
		.callback = builtin_SYST_ERR_CODE_NEXTq
	},
	{
		.levels = {"SYSTem", "ERRor", "CODE", "NEXT?"}, // optional
		.callback = builtin_SYST_ERR_CODE_NEXTq
	},
	{
		.levels = {"SYSTem", "ERRor", "CODE", "ALL?"}, // optional
		.callback = builtin_SYST_ERR_CODE_ALLq
	},
	{
		.levels = {"SYSTem", "VERSion?"},
		.callback = builtin_SYST_VERSq
	},
	// STATus:OPERation
	{
		.levels = {"STATus", "OPERation?"},
		.callback = builtin_STAT_OPER_EVENq
	},
	{
		.levels = {"STATus", "OPERation", "EVENt?"},
		.callback = builtin_STAT_OPER_EVENq
	},
	{
		.levels = {"STATus", "OPERation", "CONDition?"},
		.callback = builtin_STAT_OPER_CONDq
	},
	{
		.levels = {"STATus", "OPERation", "ENABle"},
		.params = {SCPI_DT_BOOL},
		.callback = builtin_STAT_OPER_ENAB
	},
	{
		.levels = {"STATus", "OPERation", "ENABle?"},
		.callback = builtin_STAT_OPER_ENABq
	},
	// STATus:QUEStionable
	{
		.levels = {"STATus", "QUEStionable?"},
		.callback = builtin_STAT_QUES_EVENq
	},
	{
		.levels = {"STATus", "QUEStionable", "EVENt?"},
		.callback = builtin_STAT_QUES_EVENq
	},
	{
		.levels = {"STATus", "QUEStionable", "CONDition?"},
		.callback = builtin_STAT_QUES_CONDq
	},
	{
		.levels = {"STATus", "QUEStionable", "ENABle"},
		.params = {SCPI_DT_BOOL},
		.callback = builtin_STAT_QUES_ENAB
	},
	{
		.levels = {"STATus", "QUEStionable", "ENABle?"},
		.callback = builtin_STAT_QUES_ENABq
	},
	// STATus:PRESet
	{
		.levels = {"STATus", "PRESet"},
		.callback = builtin_STAT_PRES
	},

	{/*END*/}
};














