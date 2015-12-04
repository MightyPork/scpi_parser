#pragma once
#include <stdint.h>
#include <stdbool.h>

struct __attribute__((packed)) SCPI_SR_QUEST_struct {
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


struct __attribute__((packed)) SCPI_SR_OPER_struct {
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


struct __attribute__((packed)) SCPI_SR_SESR_struct {
	bool OP_COMPLETE: 1;
	bool REQ_CONTROL: 1;
	bool QUERY_ERROR: 1;
	bool DEV_ERROR: 1;
	bool EXE_ERROR: 1;
	bool CMD_ERROR: 1;
	bool USER_REQUEST: 1;
	bool POWER_ON: 1;
};


struct __attribute__((packed)) SCPI_SR_STB_struct {
	bool BIT_0: 1;
	bool BIT_1: 1;
	bool ERROR_QUEUE: 1;
	bool QUEST: 1;
	bool MSG_AVAIL: 1;
	bool SESR: 1;
	bool RQS: 1; // request service
	bool OPER: 1;
};


// QUESTionable register
extern struct SCPI_SR_QUEST_struct SCPI_SR_QUEST;
extern struct SCPI_SR_QUEST_struct SCPI_SR_QUEST_EN; // picks what to use for the STB bit

// OPERation status register
extern struct SCPI_SR_OPER_struct SCPI_SR_OPER;
extern struct SCPI_SR_OPER_struct SCPI_SR_OPER_EN; // picks what to use for the STB bit

// Standard Event Status register
extern struct SCPI_SR_SESR_struct SCPI_SR_SESR;
extern struct SCPI_SR_SESR_struct SCPI_SR_SESR_EN; // ESE

// Status byte
extern struct SCPI_SR_STB_struct SCPI_SR_STB;
extern struct SCPI_SR_STB_struct SCPI_SR_STB_EN; // SRE
