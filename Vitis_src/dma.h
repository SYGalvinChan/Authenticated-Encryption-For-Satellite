#ifndef DMA_H /* prevent circular inclusions */
#define DMA_H /* by using protection macros */

#include "xaxidma.h"
#include "xparameters.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xil_util.h"
#include "xscugic.h"

/*
 * Timeout loop counter for reset
 */
#define RESET_TIMEOUT_COUNTER	10000

/************************** Function Prototypes *****************************/

int init_DMA();

u32 DMA_Xfer(UINTPTR BuffAddr, u32 Length, int Direction);
void flushCache(INTPTR adr, u32 len);
void invalidateCache(INTPTR adr, u32 len);

#endif
