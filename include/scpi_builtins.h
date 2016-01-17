#pragma once
#include <stdint.h>
#include <stdbool.h>

/** Optional *CLS command callback - clear non-SCPI device state */
extern __attribute__((weak)) void scpi_user_CLS(void);

/** Optional *RST command callback - reset non-SCPI device state */
extern __attribute__((weak)) void scpi_user_RST(void);

/** Optional *TST? command callback - perform self test and send response back. */
extern __attribute__((weak)) void scpi_user_TST(void);

/** MANDATORY callback to get the device *IDN? string. */
extern const char *scpi_user_IDN(void);

// Provides:
// const SCPI_command_t scpi_commands_builtin[];
