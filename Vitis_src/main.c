#include "dma.h"

extern volatile u32 TxDone;
extern volatile u32 RxDone;
extern volatile u32 Error;

__attribute__((aligned(16))) char txBuffer[20 * 16];
__attribute__((aligned(16))) char rxBuffer[20 * 16];

int testcase_1();
int testcase_2();

int main(void)
{
	xil_printf("\r\n--- Entering main() --- \r\n");

	int Status;

	Status = InitDMA();

	if (Status != XST_SUCCESS) {
		xil_printf("DMA Init Failed\r\n");
		return XST_FAILURE;
	}
	xil_printf("--------Testcase 1--------\r\n");
	Status = testcase_1();
	if (Status != XST_SUCCESS) {
		xil_printf("Testcase 1 Failed\r\n");
		return XST_FAILURE;
	}
	xil_printf("--------Testcase 2--------\r\n");
	Status = testcase_2();
	if (Status != XST_SUCCESS) {
		xil_printf("Testcase 2 Failed\r\n");
		return XST_FAILURE;
	}

}

int testcase_1() {
	int Status;
	char key[]           = {0xee, 0x84, 0xe1, 0x9c, 0xda, 0x87, 0xa7, 0x62, 0x91, 0xea, 0xaf, 0x20, 0x54, 0xae, 0xf8, 0x12};
	char crypto_header[] = {0x13, 0x36, 0x00, 0x15, 0xf2, 0xcb, 0x94, 0x9b, 0x8f, 0xb0, 0x01, 0x3e, 0x00, 0x00, 0x00, 0x01};
	char plaintext[]     = {0x4d, 0xf6, 0x4b, 0xff, 0x1f, 0xa1, 0x18, 0x95, 0xaf, 0x33, 0x7e, 0xb6, 0x6b, 0x66, 0xe1, 0x29,
							0x1f, 0xda, 0x3c, 0xf8, 0x88};
	char ciphertext[]    = {0xf5, 0x65, 0xa6, 0x8f, 0x46, 0xfc, 0x5e, 0x88, 0xd8, 0xe8, 0xea, 0xcd, 0x2b, 0x39, 0x20, 0xba,
							0x91, 0xa0, 0x33, 0x24, 0xa9};
	char tag[]           = {0x63, 0x24, 0x61, 0x89, 0xb3, 0x04, 0xc8, 0x38, 0x39, 0x39, 0x1a, 0x15, 0x03, 0x4e, 0x72, 0xe1};

	for (int i = 0; i < 16; i++) {
		txBuffer[i] = key[15 - i];
	}
	for (int i = 0; i < 16; i++) {
		txBuffer[16 + i] = crypto_header[15 - i];
	}
	for (int i = 0; i < 16; i++) {
		txBuffer[32 + i] = plaintext[15 - i];
	}
	for (int i = 0; i < 5; i++) {
		txBuffer[48 + i] = plaintext[20 - i];
	}

	xil_printf("----------- TxBuffer Start -----------\r\n");

	for (int i = 0; i < 4 * 16; i++) {
		xil_printf("%02x ", txBuffer[i]);
		if (i % 16 == 15) {
			xil_printf("\r\n");
		}
	}
	xil_printf("------------ TxBuffer End ------------\r\n");

	flushCache((UINTPTR) txBuffer, 4 * 16);
	Status = DMA_Xfer((UINTPTR) rxBuffer, 3 * 16, XAXIDMA_DEVICE_TO_DMA);

	if (Status != XST_SUCCESS) {
		xil_printf("ERROR: DMA TRANSFER FROM CRYPTO IP TO MEMORY\r\n");
		return XST_FAILURE;
	}

	Status = DMA_Xfer((UINTPTR) txBuffer, 4 * 16, XAXIDMA_DMA_TO_DEVICE);

	if (Status != XST_SUCCESS) {
		xil_printf("ERROR: DMA TRANSFER FROM MEMORY TO CRYPTO IP\r\n");
		return XST_FAILURE;
	}

	xil_printf("Issued both DMA Transfer requests\r\n");

	while (!RxDone || Error);

	if (Error) {
		xil_printf("Error Flag Set\r\n");
		return XST_FAILURE;
	} else {
		xil_printf("Received data\r\n");
	}

	xil_printf("----------- RxBuffer Start -----------\r\n");
	invalidateCache((UINTPTR) rxBuffer, 4 * 16);

	for (int i = 0; i < 3 * 16; i++) {
		xil_printf("%02x ", rxBuffer[i]);
		if (i % 16 == 15) {
			xil_printf("\r\n");
		}
	}
	xil_printf("------------ RxBuffer End ------------\r\n");

	for (int i = 0; i < 16; i++) {
		if (rxBuffer[15 - i] != ciphertext[i]) {
			xil_printf("Ciphertext incorrect at byte %d\r\n", i);
			return XST_FAILURE;
		}
	}
	for (int i = 0; i < 5; i++) {
		if (rxBuffer[31 - i] != ciphertext[16 + i]) {
			xil_printf("Ciphertext incorrect at byte %d\r\n", 16 + i);
			return XST_FAILURE;
		}
	}
	for (int i = 0; i < 16; i++) {
		if (rxBuffer[32 + i] != tag[15 - i]) {
			xil_printf("Tag incorrect\r\n");
			return XST_FAILURE;
		}
	}

	xil_printf("Encryption successful\r\n");

	return XST_SUCCESS;
}

int testcase_2() {
	int Status;
	char key[]           = {0x47, 0xc4, 0x89, 0x0c, 0xb4, 0x03, 0x44, 0x55, 0x47, 0xb6, 0x4f, 0x6f, 0x8c, 0x85, 0xdc, 0x64};
	char crypto_header[] = {0x12, 0x69, 0x00, 0x44, 0x5b, 0xff, 0x83, 0xf6, 0x76, 0x6d, 0x5e, 0x73, 0x00, 0x00, 0x00, 0x01};
	char plaintext[]     = {0xdb, 0xdd, 0xbe, 0xc4, 0xba, 0x17, 0x19, 0x05, 0xf1, 0x91, 0xf2, 0x73, 0x83, 0xc6, 0xc4, 0x10,
	                        0x11, 0xb8, 0xb1, 0x8b, 0x3f, 0x93, 0xf4, 0xdc, 0xe9, 0xbd, 0x84, 0xd4, 0x2f, 0xc4, 0x36, 0xc2,
	                        0x23, 0x8b, 0x59, 0x74, 0xef, 0x7d, 0xdb, 0xf8, 0xcf, 0xd0, 0x28, 0x4b, 0x89, 0x6f, 0x41, 0x35,
	                        0xc8, 0x33, 0x00, 0x9b, 0xa0, 0x4b, 0x6b, 0xab, 0x28, 0x4e, 0xba, 0xbc, 0x5a, 0x80, 0x47, 0xa6,
	                        0x32, 0x5c, 0xf1, 0xa4};
	char ciphertext[]    = {0x08, 0x06, 0x02, 0x1c, 0xca, 0xf3, 0xd2, 0xcc, 0xb9, 0x68, 0xd8, 0xe3, 0x14, 0xf2, 0xbb, 0xbc,
	                        0x2b, 0x94, 0x85, 0x77, 0xde, 0x43, 0x7d, 0x7b, 0x68, 0xc6, 0xb2, 0x14, 0xf5, 0xda, 0x89, 0x2b,
	                        0x6b, 0x74, 0xc6, 0xe6, 0x54, 0x5c, 0x84, 0x25, 0x8e, 0x25, 0x4a, 0xd4, 0x22, 0xaf, 0x0e, 0x3c,
	                        0x92, 0x9b, 0x3d, 0x96, 0xec, 0xba, 0xc1, 0x09, 0xf7, 0x94, 0x84, 0x38, 0xae, 0x1b, 0xfd, 0x88,
	                        0x8a, 0xd1, 0xdf, 0xcf};
	char tag[]           = {0x7c, 0x7b, 0x23, 0xcb, 0xf5, 0xf8, 0x20, 0x24, 0x72, 0x78, 0xec, 0xc9, 0x13, 0x37, 0x2a, 0x1f};

	for (int i = 0; i < 16; i++) {
		txBuffer[i] = key[15 - i];
	}
	for (int i = 0; i < 16; i++) {
		txBuffer[16 + i] = crypto_header[15 - i];
	}
	for (int i = 0; i < 16; i++) {
		txBuffer[32 + i] = plaintext[15 - i];
	}
	for (int i = 0; i < 16; i++) {
		txBuffer[48 + i] = plaintext[31 - i];
	}
	for (int i = 0; i < 16; i++) {
		txBuffer[64 + i] = plaintext[47 - i];
	}
	for (int i = 0; i < 16; i++) {
		txBuffer[80 + i] = plaintext[63 - i];
	}
	for (int i = 0; i < 4; i++) {
		txBuffer[96 + i] = plaintext[67 - i];
	}

	xil_printf("----------- TxBuffer Start -----------\r\n");

	for (int i = 0; i < 7 * 16; i++) {
		xil_printf("%02x ", txBuffer[i]);
		if (i % 16 == 15) {
			xil_printf("\r\n");
		}
	}
	xil_printf("------------ TxBuffer End ------------\r\n");

	flushCache((UINTPTR) txBuffer, 7 * 16);
	Status = DMA_Xfer((UINTPTR) rxBuffer, 6 * 16, XAXIDMA_DEVICE_TO_DMA);

	if (Status != XST_SUCCESS) {
		xil_printf("ERROR: DMA TRANSFER FROM CRYPTO IP TO MEMORY\r\n");
		return XST_FAILURE;
	}

	Status = DMA_Xfer((UINTPTR) txBuffer, 7 * 16, XAXIDMA_DMA_TO_DEVICE);

	if (Status != XST_SUCCESS) {
		xil_printf("ERROR: DMA TRANSFER FROM MEMORY TO CRYPTO IP\r\n");
		return XST_FAILURE;
	}

	xil_printf("Issued both DMA Transfer requests\r\n");

	while (!RxDone || Error);

	if (Error) {
		xil_printf("Error Flag Set\r\n");
		return XST_FAILURE;
	} else {
		xil_printf("Received data\r\n");
	}

	xil_printf("----------- RxBuffer Start -----------\r\n");
	invalidateCache((UINTPTR) rxBuffer, 7 * 16);

	for (int i = 0; i < 6 * 16; i++) {
		xil_printf("%02x ", rxBuffer[i]);
		if (i % 16 == 15) {
			xil_printf("\r\n");
		}
	}
	xil_printf("------------ RxBuffer End ------------\r\n");

	for (int i = 0; i < 16; i++) {
		if (rxBuffer[15 - i] != ciphertext[i]) {
			xil_printf("Ciphertext incorrect at byte %d\r\n", i);
			return XST_FAILURE;
		}
	}
	for (int i = 0; i < 16; i++) {
		if (rxBuffer[31 - i] != ciphertext[16 + i]) {
			xil_printf("Ciphertext incorrect at byte %d\r\n", 16 + i);
			return XST_FAILURE;
		}
	}
	for (int i = 0; i < 16; i++) {
		if (rxBuffer[47 - i] != ciphertext[32 + i]) {
			xil_printf("Ciphertext incorrect at byte %d\r\n", 32 + i);
			return XST_FAILURE;
		}
	}
	for (int i = 0; i < 16; i++) {
		if (rxBuffer[63 - i] != ciphertext[48 + i]) {
			xil_printf("Ciphertext incorrect at byte %d\r\n", 48 + i);
			return XST_FAILURE;
		}
	}
	for (int i = 0; i < 4; i++) {
		if (rxBuffer[79 - i] != ciphertext[64 + i]) {
			xil_printf("Ciphertext incorrect at byte %d\r\n", 64 + i);
			return XST_FAILURE;
		}
	}
	for (int i = 0; i < 16; i++) {
		if (rxBuffer[80 + i] != tag[15 - i]) {
			xil_printf("Tag incorrect\r\n");
			return XST_FAILURE;
		}
	}

	xil_printf("Encryption successful\r\n");

	return XST_SUCCESS;
}
