#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "scpi_regs.h"
#include "scpi_errors.h"
#include "scpi_parser.h"

SCPI_REG_QUES_t SCPI_REG_QUES;
SCPI_REG_QUES_t SCPI_REG_QUES_EN = {.u16 = 0xFFFF};

SCPI_REG_OPER_t SCPI_REG_OPER;
SCPI_REG_OPER_t SCPI_REG_OPER_EN = {.u16 = 0xFFFF};

SCPI_REG_SESR_t SCPI_REG_SESR = {.POWER_ON = 1}; // indicates the startup condition
SCPI_REG_SESR_t SCPI_REG_SESR_EN;

SCPI_REG_STB_t SCPI_REG_STB;
SCPI_REG_STB_t SCPI_REG_SRE;

/** Update status registers (propagate using enable registers) */
void scpi_status_update(void)
{
	// propagate to STB
	SCPI_REG_STB.ERRQ = scpi_error_count() > 0;
	SCPI_REG_STB.QUES = SCPI_REG_QUES.u16 & SCPI_REG_QUES_EN.u16;
	SCPI_REG_STB.OPER = SCPI_REG_OPER.u16 & SCPI_REG_OPER_EN.u16;
	SCPI_REG_STB.SESR = SCPI_REG_SESR.u8 & SCPI_REG_SESR_EN.u8;
	SCPI_REG_STB.MAV = false; // TODO!!!

	// Request Service
	SCPI_REG_STB.RQS = SCPI_REG_STB.u8 & SCPI_REG_SRE.u8;


	// Run service request callback
	if (SCPI_REG_STB.RQS) {
		if (scpi_service_request_impl) {
			scpi_service_request_impl();
		}
	}
}
