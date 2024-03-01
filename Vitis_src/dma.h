#ifndef DMA_H /* prevent circular inclusions */
#define DMA_H /* by using protection macros */

#include "xaxidma.h"
#include "xparameters.h"
#include "xil_exception.h"
#include "xil_printf.h"
#include "xil_util.h"
#include "xscugic.h"

/************************** Constant Definitions *****************************/

/*
 * Device hardware build related constants.
 */

#define DMA_DEV_ID		XPAR_AXIDMA_0_DEVICE_ID

#define RX_INTR_ID		XPAR_FABRIC_AXIDMA_0_S2MM_INTROUT_VEC_ID
#define TX_INTR_ID		XPAR_FABRIC_AXIDMA_0_MM2S_INTROUT_VEC_ID

#define INTC_DEVICE_ID          XPAR_SCUGIC_SINGLE_DEVICE_ID

#define INTC		XScuGic
#define INTC_HANDLER	XScuGic_InterruptHandler

/*
 * Timeout loop counter for reset
 */
#define RESET_TIMEOUT_COUNTER	10000

/************************** Function Prototypes *****************************/

int InitDMA();

u32 DMA_Xfer(UINTPTR BuffAddr, u32 Length, int Direction);
void flushCache(INTPTR adr, u32 len);
void invalidateCache(INTPTR adr, u32 len);

int SetupIntrSystem(INTC * IntcInstancePtr,
			   XAxiDma * AxiDmaPtr, u16 TxIntrId, u16 RxIntrId);
void DisableIntrSystem(INTC * IntcInstancePtr,
					u16 TxIntrId, u16 RxIntrId);

void TxIntrHandler(void *Callback);
void RxIntrHandler(void *Callback);

#endif
