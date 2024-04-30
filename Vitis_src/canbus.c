#include "canbus.h"
#include "crypto_system.h"
#include "circular_buffer.h"

/************************** Variable Definitions *****************************/

static XCanPs CanInstance;    /* Instance of the Can driver */
extern XScuGic IntcInstance; /* Instance of the Interrupt Controller driver */

/*
 * Buffers to hold frames to send and receive. These are declared as global so
 * that they are not on the stack.
 * These buffers need to be 32-bit aligned
 */
static __attribute__((aligned(32))) u32 TxFrame[4];
static __attribute__((aligned(32))) u32 RxFrame[4];

volatile struct circular_buffer to_OBC_c_buffer;
volatile struct circular_buffer to_GS_c_buffer;

struct crypto_buffer* tx_buffer;

/*
 * Shared variables used to test the callbacks.
 */
volatile static int LoopbackError;	/* Asynchronous error occurred */
volatile static int RecvDone;		/* Received a frame */
volatile static int SendDone;		/* Frame was sent successfully */

void SendHandler(void *CallBackRef);
void RecvHandler(void *CallBackRef);
void ErrorHandler(void *CallBackRef, u32 ErrorMask);
void EventHandler(void *CallBackRef, u32 Mask);

int init_CANBus() {
	int Status;

	init_circular_buffer(&to_OBC_c_buffer);
	init_circular_buffer(&to_GS_c_buffer);

	XScuGic *IntcInstPtr = &IntcInstance;
	XCanPs *CanInstPtr = &CanInstance;
	XGpioPs Gpio;
	XGpioPs_Config * GpioCfg;

	/*
	* Initialize the transceiver device.
	*/
	GpioCfg = XGpioPs_LookupConfig(XPAR_PS7_GPIO_0_DEVICE_ID);
	if (GpioCfg == NULL) {
	return XST_FAILURE;
	}
	Status = XGpioPs_CfgInitialize(&Gpio, GpioCfg, GpioCfg->BaseAddr);
	if (Status != XST_SUCCESS) {
	return XST_FAILURE;
	}
	/*
	* Setup GPIO9 to low (CAN_STB/SP_MIO9)
	*/
	XGpioPs_SetDirectionPin(&Gpio, 9, 1);
	XGpioPs_SetOutputEnablePin(&Gpio, 9, 1);
	XGpioPs_WritePin(&Gpio, 9, 0);

	XCanPs_Config *ConfigPtr;

	/*
	 * Initialize the Can device.
	 */
	ConfigPtr = XCanPs_LookupConfig(XPAR_XCANPS_0_DEVICE_ID);
	if (ConfigPtr == NULL) {
		return XST_FAILURE;
	}
	XCanPs_CfgInitialize(CanInstPtr,
				ConfigPtr,
				ConfigPtr->BaseAddr);

	/*
	 * Run self-test on the device, which verifies basic sanity of the
	 * device and the driver.
	 */
	Status = XCanPs_SelfTest(CanInstPtr);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Enter Configuration Mode if the device is not currently in
	 * Configuration Mode.
	 */
	XCanPs_EnterMode(CanInstPtr, XCANPS_MODE_CONFIG);
	while(XCanPs_GetMode(CanInstPtr) != XCANPS_MODE_CONFIG);

	/*
	 * Setup Baud Rate Prescaler Register (BRPR) and
	 * Bit Timing Register (BTR).
	 */
	XCanPs_SetBaudRatePrescaler(CanInstPtr, TEST_BRPR_BAUD_PRESCALAR);
	XCanPs_SetBitTiming(CanInstPtr, TEST_BTR_SYNCJUMPWIDTH,
					TEST_BTR_SECOND_TIMESEGMENT,
					TEST_BTR_FIRST_TIMESEGMENT);
	/*
	 * Set interrupt handlers.
	 */
	XCanPs_SetHandler(CanInstPtr, XCANPS_HANDLER_SEND,
			(void *)SendHandler, (void *)CanInstPtr);
	XCanPs_SetHandler(CanInstPtr, XCANPS_HANDLER_RECV,
			(void *)RecvHandler, (void *)CanInstPtr);
	XCanPs_SetHandler(CanInstPtr, XCANPS_HANDLER_ERROR,
			(void *)ErrorHandler, (void *)CanInstPtr);
	XCanPs_SetHandler(CanInstPtr, XCANPS_HANDLER_EVENT,
			(void *)EventHandler, (void *)CanInstPtr);

	/*
	 * Initialize the flags.
	 */
	SendDone = FALSE;
	RecvDone = FALSE;
	LoopbackError = FALSE;

	/*
	 * Connect the device driver handler that will be called when an
	 * interrupt for the device occurs, the handler defined above performs
	 * the specific interrupt processing for the device.
	 */
	Status = XScuGic_Connect(IntcInstPtr, XPAR_XCANPS_0_INTR,
				(Xil_InterruptHandler)XCanPs_IntrHandler,
				(void *)CanInstPtr);
	if (Status != XST_SUCCESS) {
		return Status;
	}

	/*
	 * Enable the interrupt for the CAN device.
	 */
	XScuGic_Enable(IntcInstPtr, XPAR_XCANPS_0_INTR);
	if (Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Enable all interrupts in CAN device.
	 */
	XCanPs_IntrEnable(CanInstPtr, XCANPS_IXR_ALL);

	/*
	 * Enter Normal Mode.
	 */
	XCanPs_EnterMode(CanInstPtr, XCANPS_MODE_NORMAL);
	while(XCanPs_GetMode(CanInstPtr) != XCANPS_MODE_NORMAL);

	return XST_SUCCESS;
}

void send_crypto_buffer(struct crypto_buffer* crypto_buffer_to_send) {
	tx_buffer = crypto_buffer_to_send;
	int data_len;
	int Status;
	u8* frame_data;
	if (tx_buffer->state == SEND_PAYLOAD_ONLY) {
		if (tx_buffer->remaining_bytes == 0) {
			return;
		}
		if (tx_buffer->remaining_bytes >= 8) {
			data_len = 8;
		} else {
			data_len = tx_buffer->remaining_bytes;
		}
		TxFrame[0] = (u32)XCanPs_CreateIdValue((u32) CIM_TO_OBC, 0, 0, 0, 0);
		TxFrame[1] = (u32)XCanPs_CreateDlcValue((u32) data_len);

		frame_data = (u8*)&TxFrame[2];
		for (int i = 0; i < data_len; i++) {
			int index = tx_buffer->block_number * 16 + tx_buffer->block_offset;
			frame_data[i] = tx_buffer->payload_tag[index];
			tx_buffer->block_offset--;
			tx_buffer->remaining_bytes--;
			if (tx_buffer->block_offset < 0) {
				tx_buffer->block_number++;
				tx_buffer->block_offset = 15;
			}
		}

		/*
		 * Now wait until the TX FIFO is not full and send the frame.
		 */
		while (XCanPs_IsTxFifoFull(&CanInstance) == TRUE);
		Status = XCanPs_Send(&CanInstance, TxFrame);
		if (Status != XST_SUCCESS) {
			xil_printf("The frame could not be sent successfully.\r\n");
		}
	} else {
		switch (tx_buffer->state) {
			case SEND_CRYPTO_HEADER:
				if (tx_buffer->block_offset - 3 >= 8) {
					data_len = 8;
				} else {
					data_len = tx_buffer->block_offset - 3;
				}

				TxFrame[0] = (u32)XCanPs_CreateIdValue((u32) CIM_TO_GS, 0, 0, 0, 0);
				TxFrame[1] = (u32)XCanPs_CreateDlcValue((u32) data_len);

				frame_data = (u8*)&TxFrame[2];

				for (int i = 0; i < data_len; i++) {
					frame_data[i] = tx_buffer->crypto_header[tx_buffer->block_offset];
					tx_buffer->block_offset--;
				}

				if (tx_buffer->block_offset < 4) {
					tx_buffer->block_offset = 15;
					tx_buffer->state = SEND_PAYLOAD_WITH_TAG;
				}

				/*
				 * Now wait until the TX FIFO is not full and send the frame.
				 */
				while (XCanPs_IsTxFifoFull(&CanInstance) == TRUE);
				Status = XCanPs_Send(&CanInstance, TxFrame);
				if (Status != XST_SUCCESS) {
					xil_printf("The frame could not be sent successfully.\r\n");
				}
				break;

			case SEND_PAYLOAD_WITH_TAG:
				if (tx_buffer->remaining_bytes >= 8) {
					data_len = 8;
				} else {
					data_len = tx_buffer->remaining_bytes;
				}
				TxFrame[0] = (u32)XCanPs_CreateIdValue((u32) CIM_TO_GS, 0, 0, 0, 0);
				TxFrame[1] = (u32)XCanPs_CreateDlcValue((u32) data_len);

				frame_data = (u8*)&TxFrame[2];

				for (int i = 0; i < data_len; i++) {
					int index = tx_buffer->block_number * 16 + tx_buffer->block_offset;
					frame_data[i] = tx_buffer->payload_tag[index];
					tx_buffer->block_offset--;
					tx_buffer->remaining_bytes--;
					if (tx_buffer->block_offset < 0) {
						tx_buffer->block_number++;
						tx_buffer->block_offset = 15;
					}
				}

				if (tx_buffer->remaining_bytes == 0) {
					tx_buffer->block_number++;
					tx_buffer->block_offset = 15;
					tx_buffer->state = SEND_TAG;
				}

				/*
				 * Now wait until the TX FIFO is not full and send the frame.
				 */
				while (XCanPs_IsTxFifoFull(&CanInstance) == TRUE);
				Status = XCanPs_Send(&CanInstance, TxFrame);
				if (Status != XST_SUCCESS) {
					xil_printf("The frame could not be sent successfully.\r\n");
				}
				break;

			case SEND_TAG:
				if (tx_buffer->block_offset < 0) {
					return;
				}

				if (tx_buffer->block_offset >= 8) {
					data_len = 8;
				} else {
					data_len = tx_buffer->block_offset;
				}

				TxFrame[0] = (u32)XCanPs_CreateIdValue((u32) CIM_TO_GS, 0, 0, 0, 0);
				TxFrame[1] = (u32)XCanPs_CreateDlcValue((u32) data_len);

				frame_data = (u8*)&TxFrame[2];

				for (int i = 0; i < data_len; i++) {
					int index = tx_buffer->block_number * 16 + tx_buffer->block_offset;
					frame_data[i] = tx_buffer->payload_tag[index];
					tx_buffer->block_offset--;
				}

				/*
				 * Now wait until the TX FIFO is not full and send the frame.
				 */
				while (XCanPs_IsTxFifoFull(&CanInstance) == TRUE);
				Status = XCanPs_Send(&CanInstance, TxFrame);
				if (Status != XST_SUCCESS) {
					xil_printf("The frame could not be sent successfully.\r\n");
				}
				break;
		}
	}
}

/*****************************************************************************/
/**
*
* Callback function (called from interrupt handler) to handle confirmation of
* transmit events when in interrupt mode.
*
* @param	CallBackRef is the callback reference passed from the interrupt
*		handler, which in our case is a pointer to the driver instance.
*
* @return	None.
*
* @note		This function is called by the driver within interrupt context.
*
******************************************************************************/
void SendHandler(void *CallBackRef)
{
	/*
	 * The frame was sent successfully. Notify the task context.
	 */
	SendDone = TRUE;
//	xil_printf("Send handler called and frame successfully sent\r\n");
	int Status;
	int data_len;
	u8* frame_data;
	if (tx_buffer->state == SEND_PAYLOAD_ONLY) {
		if (tx_buffer->remaining_bytes == 0) {
			return;
		}
		if (tx_buffer->remaining_bytes >= 8) {
			data_len = 8;
		} else {
			data_len = tx_buffer->remaining_bytes;
		}
		TxFrame[0] = (u32)XCanPs_CreateIdValue((u32) CIM_TO_OBC, 0, 0, 0, 0);
		TxFrame[1] = (u32)XCanPs_CreateDlcValue((u32) data_len);

		frame_data = (u8*)&TxFrame[2];
		for (int i = 0; i < data_len; i++) {
			int index = tx_buffer->block_number * 16 + tx_buffer->block_offset;
			frame_data[i] = tx_buffer->payload_tag[index];
			tx_buffer->block_offset--;
			tx_buffer->remaining_bytes--;
			if (tx_buffer->block_offset < 0) {
				tx_buffer->block_number++;
				tx_buffer->block_offset = 15;
			}
		}
		/*
		 * Now wait until the TX FIFO is not full and send the frame.
		 */
		while (XCanPs_IsTxFifoFull(&CanInstance) == TRUE);
		Status = XCanPs_Send(&CanInstance, TxFrame);
		if (Status != XST_SUCCESS) {
			xil_printf("The frame could not be sent successfully.\r\n");
		}
	} else {
		switch (tx_buffer->state) {
			case SEND_CRYPTO_HEADER:
				if (tx_buffer->block_offset - 3 >= 8) {
					data_len = 8;
				} else {
					data_len = tx_buffer->block_offset - 3;
				}

				TxFrame[0] = (u32)XCanPs_CreateIdValue((u32) CIM_TO_GS, 0, 0, 0, 0);
				TxFrame[1] = (u32)XCanPs_CreateDlcValue((u32) data_len);

				frame_data = (u8*)&TxFrame[2];

				for (int i = 0; i < data_len; i++) {
					frame_data[i] = tx_buffer->crypto_header[tx_buffer->block_offset];
					tx_buffer->block_offset--;
				}

				if (tx_buffer->block_offset < 4) {
					tx_buffer->block_offset = 15;
					tx_buffer->state = SEND_PAYLOAD_WITH_TAG;
				}

				/*
				 * Now wait until the TX FIFO is not full and send the frame.
				 */
				while (XCanPs_IsTxFifoFull(&CanInstance) == TRUE);
				Status = XCanPs_Send(&CanInstance, TxFrame);
				if (Status != XST_SUCCESS) {
					xil_printf("The frame could not be sent successfully.\r\n");
				}
				break;

			case SEND_PAYLOAD_WITH_TAG:
				if (tx_buffer->remaining_bytes >= 8) {
					data_len = 8;
				} else {
					data_len = tx_buffer->remaining_bytes;
				}
				TxFrame[0] = (u32)XCanPs_CreateIdValue((u32) CIM_TO_GS, 0, 0, 0, 0);
				TxFrame[1] = (u32)XCanPs_CreateDlcValue((u32) data_len);

				frame_data = (u8*)&TxFrame[2];

				for (int i = 0; i < data_len; i++) {
					int index = tx_buffer->block_number * 16 + tx_buffer->block_offset;
					frame_data[i] = tx_buffer->payload_tag[index];
					tx_buffer->block_offset--;
					tx_buffer->remaining_bytes--;
					if (tx_buffer->block_offset < 0) {
						tx_buffer->block_number++;
						tx_buffer->block_offset = 15;
					}
				}

				if (tx_buffer->remaining_bytes <= 0) {
					tx_buffer->block_number++;
					tx_buffer->block_offset = 15;
					tx_buffer->state = SEND_TAG;
				}

				/*
				 * Now wait until the TX FIFO is not full and send the frame.
				 */
				while (XCanPs_IsTxFifoFull(&CanInstance) == TRUE);
				Status = XCanPs_Send(&CanInstance, TxFrame);
				if (Status != XST_SUCCESS) {
					xil_printf("The frame could not be sent successfully.\r\n");
				}
				break;

			case SEND_TAG:
				if (tx_buffer->block_offset < 0) {
					tx_buffer->state = RECV_PAYLOAD;
					return;
				}
				if (tx_buffer->block_offset >= 8) {
					data_len = 8;
				} else {
					data_len = tx_buffer->block_offset + 1;
				}

				TxFrame[0] = (u32)XCanPs_CreateIdValue((u32) CIM_TO_GS, 0, 0, 0, 0);
				TxFrame[1] = (u32)XCanPs_CreateDlcValue((u32) data_len);

				frame_data = (u8*)&TxFrame[2];

				for (int i = 0; i < data_len; i++) {
					int index = tx_buffer->block_number * 16 + tx_buffer->block_offset;
					frame_data[i] = tx_buffer->payload_tag[index];
					tx_buffer->block_offset--;
				}

				/*
				 * Now wait until the TX FIFO is not full and send the frame.
				 */
				while (XCanPs_IsTxFifoFull(&CanInstance) == TRUE);
				Status = XCanPs_Send(&CanInstance, TxFrame);
				if (Status != XST_SUCCESS) {
					xil_printf("The frame could not be sent successfully.\r\n");
				}
				break;
		}
	}
}

/*****************************************************************************/
/**
*
* Callback function (called from interrupt handler) to handle frames received in
* interrupt mode.  This function is called once per frame received.
* The driver's receive function is called to read the frame from RX FIFO.
*
* @param	CallBackRef is the callback reference passed from the interrupt
*		handler, which in our case is a pointer to the device instance.
*
* @return	None.
*
* @note		This function is called by the driver within interrupt context.
*
******************************************************************************/
void RecvHandler(void *CallBackRef)
{
	XCanPs *CanPtr = (XCanPs *)CallBackRef;
	int Status;

	Status = XCanPs_Recv(CanPtr, RxFrame);
	if (Status != XST_SUCCESS) {
		LoopbackError = TRUE;
		RecvDone = TRUE;
		return;
	}
//	xil_printf("Receive handler called and frame successfully received\r\n");


	u32 message_id = RxFrame[0] >> 21;
	u32 data_len = RxFrame[1] >> 28;
	u8 *data = (u8*) &RxFrame[2];
	if (message_id == GS_TO_CIM || message_id == 256) {
		circular_buffer_insert(&to_OBC_c_buffer, data, data_len);
	} else {
		circular_buffer_insert(&to_GS_c_buffer, data, data_len);
	}
	RecvDone = TRUE;
}



/*****************************************************************************/
/**
*
* Callback function (called from interrupt handler) to handle error interrupt.
* Error code read from Error Status register is passed into this function.
*
* @param	CallBackRef is the callback reference passed from the interrupt
*		handler, which in our case is a pointer to the driver instance.
* @param	ErrorMask is a bit mask indicating the cause of the error.
*		Its value equals 'OR'ing one or more XCANPS_ESR_* defined in
*		xcanps_hw.h.
*
* @return	None.
*
* @note		This function is called by the driver within interrupt context.
*
******************************************************************************/
void ErrorHandler(void *CallBackRef, u32 ErrorMask)
{
	xil_printf("Error handler called with error mask:%08x\r\n", ErrorMask);

	if(ErrorMask & XCANPS_ESR_ACKER_MASK) {
		/*
		 * ACK Error handling code should be put here.
		 */
	}

	if(ErrorMask & XCANPS_ESR_BERR_MASK) {
		/*
		 * Bit Error handling code should be put here.
		 */
	}

	if(ErrorMask & XCANPS_ESR_STER_MASK) {
		/*
		 * Stuff Error handling code should be put here.
		 */
	}

	if(ErrorMask & XCANPS_ESR_FMER_MASK) {
		/*
		 * Form Error handling code should be put here.
		 */
	}

	if(ErrorMask & XCANPS_ESR_CRCER_MASK) {
		/*
		 * CRC Error handling code should be put here.
		 */
	}

	/*
	 * Set the shared variables.
	 */
	LoopbackError = TRUE;
	RecvDone = TRUE;
	SendDone = TRUE;
}

/*****************************************************************************/
/**
*
* Callback function (called from interrupt handler) to handle the following
* interrupts:
*   - XCANPS_IXR_BSOFF_MASK:	Bus Off Interrupt
*   - XCANPS_IXR_RXOFLW_MASK:	RX FIFO Overflow Interrupt
*   - XCANPS_IXR_RXUFLW_MASK:	RX FIFO Underflow Interrupt
*   - XCANPS_IXR_TXBFLL_MASK:	TX High Priority Buffer Full Interrupt
*   - XCANPS_IXR_TXFLL_MASK:	TX FIFO Full Interrupt
*   - XCANPS_IXR_WKUP_MASK:	Wake up Interrupt
*   - XCANPS_IXR_SLP_MASK:	Sleep Interrupt
*   - XCANPS_IXR_ARBLST_MASK:	Arbitration Lost Interrupt
*
*
* @param	CallBackRef is the callback reference passed from the
*		interrupt Handler, which in our case is a pointer to the
*		driver instance.
* @param	IntrMask is a bit mask indicating pending interrupts.
*		Its value equals 'OR'ing one or more of the XCANPS_IXR_*_MASK
*		value(s) mentioned above.
*
* @return	None.
*
* @note		This function is called by the driver within interrupt context.
* 		This function should be changed to meet specific application
*		needs.
*
******************************************************************************/
void EventHandler(void *CallBackRef, u32 IntrMask)
{
	XCanPs *CanPtr = (XCanPs *)CallBackRef;

	if (IntrMask & XCANPS_IXR_BSOFF_MASK) {
		/*
		 * Entering Bus off status interrupt requires
		 * the CAN device be reset and reconfigured.
		 */
		XCanPs_Reset(CanPtr);
		/*
		 * Enter Configuration Mode if the device is not currently in
		 * Configuration Mode.
		 */
		XCanPs_EnterMode(CanPtr, XCANPS_MODE_CONFIG);
		while(XCanPs_GetMode(CanPtr) != XCANPS_MODE_CONFIG);

		/*
		 * Setup Baud Rate Prescaler Register (BRPR) and
		 * Bit Timing Register (BTR).
		 */
		XCanPs_SetBaudRatePrescaler(CanPtr, TEST_BRPR_BAUD_PRESCALAR);
		XCanPs_SetBitTiming(CanPtr, TEST_BTR_SYNCJUMPWIDTH,
						TEST_BTR_SECOND_TIMESEGMENT,
						TEST_BTR_FIRST_TIMESEGMENT);
		return;
	}

	if(IntrMask & XCANPS_IXR_RXOFLW_MASK) {
		/*
		 * Code to handle RX FIFO Overflow Interrupt should be put here.
		 */
	}

	if(IntrMask & XCANPS_IXR_RXUFLW_MASK) {
		/*
		 * Code to handle RX FIFO Underflow Interrupt
		 * should be put here.
		 */
	}

	if(IntrMask & XCANPS_IXR_TXBFLL_MASK) {
		/*
		 * Code to handle TX High Priority Buffer Full
		 * Interrupt should be put here.
		 */
	}

	if(IntrMask & XCANPS_IXR_TXFLL_MASK) {
		/*
		 * Code to handle TX FIFO Full Interrupt should be put here.
		 */
	}

	if (IntrMask & XCANPS_IXR_WKUP_MASK) {
		/*
		 * Code to handle Wake up from sleep mode Interrupt
		 * should be put here.
		 */
	}

	if (IntrMask & XCANPS_IXR_SLP_MASK) {
		/*
		 * Code to handle Enter sleep mode Interrupt should be put here.
		 */
	}

	if (IntrMask & XCANPS_IXR_ARBLST_MASK) {
		/*
		 * Code to handle Lost bus arbitration Interrupt
		 * should be put here.
		 */
	}
}
