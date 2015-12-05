#pragma once
#include <stdint.h>
#include <stdbool.h>

#include "scpi_parser.h"
#include "scpi_errors.h"
#include "scpi_regs.h"


/** *CLS command callback - clear non-SCPI device state */
extern __attribute__((weak)) void scpi_user_CLS(void);

/** *RST command callback - reset non-SCPI device state */
extern __attribute__((weak)) void scpi_user_RST(void);

/** *TST? command callback - perform self test and send response back. */
extern __attribute__((weak)) void scpi_user_TSTq(void);

/** Get device *IDN? string. */
extern const char *scpi_device_identifier(void);

// Provides:
// const SCPI_command_t scpi_commands_builtin[];
