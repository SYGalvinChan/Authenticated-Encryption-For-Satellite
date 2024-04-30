#ifndef CANBUS_H /* prevent circular inclusions */
#define CANBUS_H /* by using protection macros */

/***************************** Include Files *********************************/

#include "xparameters.h"
#include "xcanps.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xgpiops.h"
#include "xscugic.h"
#include "crypto_system.h"

/************************** Constant Definitions *****************************/
/*
 * Message Id Constant.
 */
#define GS_TO_CIM		1
#define CIM_TO_GS       2
#define OBC_TO_CIM		3
#define CIM_TO_OBC      4

/*
 * The Baud Rate Prescaler Register (BRPR) and Bit Timing Register (BTR)
 * are setup such that CAN baud rate equals 40Kbps, assuming that the
 * the CAN clock is 24MHz. The user needs to modify these values based on
 * the desired baud rate and the CAN clock frequency. For more information
 * see the CAN 2.0A, CAN 2.0B, ISO 11898-1 specifications.
 */

/*
 * Timing parameters to be set in the Bit Timing Register (BTR).
 * These values are for a 40 Kbps baudrate assuming the CAN input clock
 * frequency is 24 MHz.
 */
#define TEST_BTR_SYNCJUMPWIDTH		3
#define TEST_BTR_SECOND_TIMESEGMENT	2
#define TEST_BTR_FIRST_TIMESEGMENT	15

/*
 * The Baud rate Prescalar value in the Baud Rate Prescaler Register
 * needs to be set based on the input clock  frequency to the CAN core and
 * the desired CAN baud rate.
 * This value is for a 40 Kbps baudrate assuming the CAN input clock frequency
 * is 24 MHz.
 */
#define TEST_BRPR_BAUD_PRESCALAR	29


int init_CANBus();
void send_crypto_buffer(struct crypto_buffer* crypto_buffer_to_send);
int align_to_block(int index, int round_down);


#endif // CANBUS_H
