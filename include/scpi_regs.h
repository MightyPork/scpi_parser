#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef union {
	struct __attribute__((packed)) {
		bool VOLT: 1;
		bool CURR: 1;
		bool TIME: 1;
		bool POWER: 1;
		bool TEMP: 1;
		bool FREQ: 1;
		bool PHASE: 1;
		bool MODUL: 1;
		bool CALIB: 1;
		bool BIT_9: 1; // user defined
		bool BIT_10: 1;
		bool BIT_11: 1;
		bool BIT_12: 1;
		bool INSTR_SUM: 1; // instrument summary
		bool COMMAND_WARNING: 1; // command warning
		bool RESERVED: 1;
	};

	uint16_t u16;
} SCPI_REG_QUES_t;


typedef union {
	struct __attribute__((packed)) {
		bool CALIB: 1;
		bool SETTING: 1;
		bool RANGING: 1;
		bool SWEEP: 1;
		bool MEAS: 1;
		bool WAIT_TRIG: 1; // waiting for trigger
		bool WAIT_ARM: 1; // waiting for ARM
		bool CORRECTING: 1;
		bool BIT_8: 1; // user defined
		bool BIT_9: 1;
		bool BIT_10: 1;
		bool BIT_11: 1;
		bool BIT_12: 1;
		bool INSTR_SUM: 1; // instrument summary
		bool PROG_RUN: 1; // program running
		bool RESERVED: 1;
	};

	uint16_t u16;
} SCPI_REG_OPER_t;


typedef union {
	struct __attribute__((packed)) {
		bool OPC: 1; // operation complete - only for instruments with overlapping commands
		bool REQ_CONTROL: 1; // GPIB-only
		bool QUERY_ERROR: 1;
		bool DEV_ERROR: 1;
		bool EXE_ERROR: 1;
		bool CMD_ERROR: 1;
		bool USER_REQUEST: 1;
		bool POWER_ON: 1;
	};

	uint8_t u8;
} SCPI_REG_SESR_t;


typedef union {
	struct __attribute__((packed)) {
		bool BIT_0: 1;
		bool BIT_1: 1;
		bool ERRQ: 1; // error queue
		bool QUES: 1;
		bool MAV: 1; // message available
		bool SESR: 1;
		bool RQS: 1; // request service
		bool OPER: 1;
	};

	uint8_t u8;
} SCPI_REG_STB_t;


// QUESTionable register
extern SCPI_REG_QUES_t SCPI_REG_QUES;
extern SCPI_REG_QUES_t SCPI_REG_QUES_EN; // picks what to use for the STB bit

// OPERation status register
extern SCPI_REG_OPER_t SCPI_REG_OPER;
extern SCPI_REG_OPER_t SCPI_REG_OPER_EN; // picks what to use for the STB bit

// Standard Event Status register
extern SCPI_REG_SESR_t SCPI_REG_SESR;
extern SCPI_REG_SESR_t SCPI_REG_SESR_EN; // ESE

// Status byte
extern SCPI_REG_STB_t SCPI_REG_STB;
extern SCPI_REG_STB_t SCPI_REG_SRE; // SRE


/** Update the status registers (perform propagation) */
void scpi_status_update(void);


/**
 * Service Request callback.
 * User may choose to implement it (eg. send request to master),
 * or leave unimplemented.
 *
 * SRQ is issued when an event enabled in the status registers (namely SRE) occurs.
 * See the SCPI spec for details.
 */
extern __attribute__((weak)) void scpi_user_SRQ(void);
