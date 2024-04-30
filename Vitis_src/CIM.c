#include "dma.h"
#include "canbus.h"
#include "interrupt_controller.h"
#include "circular_buffer.h"
#include "crypto_system.h"

extern volatile struct circular_buffer to_OBC_c_buffer;
extern volatile struct circular_buffer to_GS_c_buffer;

#define TIMEOUT 1000000

int main(void)
{
	xil_printf("--------- Entering main() ---------\r\n");
	int Status;

	Status = init_interrupt_controller();
	if (Status != XST_SUCCESS) {
		xil_printf("Interrupt Controller Init Failed\r\n");
		return XST_FAILURE;
	}

	Status = init_DMA();
	if (Status != XST_SUCCESS) {
		xil_printf("DMA Init Failed\r\n");
		return XST_FAILURE;
	}

	Status = init_crypto_system();
	if (Status != XST_SUCCESS) {
		xil_printf("Crypto Module Init Failed\r\n");
		return XST_FAILURE;
	}

	Status = init_CANBus();
	if (Status != XST_SUCCESS) {
		xil_printf("CANBUS Init Failed\r\n");
		return XST_FAILURE;
	}

	Xil_ExceptionEnable();

	xil_printf("--------Initialization Done--------\r\n");

	xil_printf("--------Enter Infinite Loop--------\r\n");

	u8 tmp[100];
	int counter = 0;
	while(1) {
		if (!to_OBC_c_buffer.is_empty) {
			int amt_popped = circular_buffer_pop(&to_OBC_c_buffer, tmp, 100);
			insert_crypto_buffer(tmp, amt_popped, GS_TO_OBC);
		}

		if (!to_GS_c_buffer.is_empty) {
			counter = 0;
			int amt_popped = circular_buffer_pop(&to_GS_c_buffer, tmp, 100);
			insert_crypto_buffer(tmp, amt_popped, OBC_TO_GS);
		} else {
			counter++;
			if (counter >= TIMEOUT) {
				counter = 0;
				encrypt_to_GS();
			}
		}
	}
}

