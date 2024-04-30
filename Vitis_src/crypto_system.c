#include "crypto_system.h"
#include "canbus.h"
#include "xtime_l.h"

volatile int crypto_system_busy = 0;
__attribute__((aligned(16))) struct crypto_buffer to_OBC;
__attribute__((aligned(16))) struct crypto_buffer to_GS;

extern volatile u32 TxDone;
extern volatile u32 RxDone;
extern volatile u32 Error;

int init_crypto_system() {
	to_OBC.remaining_bytes = 0;
	to_OBC.block_number = 0;
	to_OBC.block_offset = 15;
	to_OBC.state = RECV_CRYPTO_HEADER;

	to_GS.remaining_bytes = 0;
	to_GS.block_number = 0;
	to_GS.block_offset = 15;
	to_GS.state = RECV_PAYLOAD;
	return XST_SUCCESS;
}
void create_crypto_header(char* crypto_header, u64 replay_counter, u16 payload_len, u8 key_index, u8 direction) {
	*((u64*) &crypto_header[4]) = replay_counter;
	*((u16*) &crypto_header[12]) = payload_len;
	*((u8*) &crypto_header[14]) = key_index;
	*((u8*) &crypto_header[15]) = direction;
}

int encrypt(struct crypto_buffer* txBuffer) {
	crypto_system_busy = 1;
	int status;

	char key[] = {0xee, 0x84, 0xe1, 0x9c, 0xda, 0x87, 0xa7, 0x62, 0x91, 0xea, 0xaf, 0x20, 0x54, 0xae, 0xf8, 0x12};
	for (int i = 0; i < 16; i++) {
		txBuffer->key[i] = key[15 - i];
	}
	// TODO Generate Crypto Header
	create_crypto_header(&txBuffer->crypto_header, 0xF2CB949B8FB0013E, txBuffer->remaining_bytes, 0x36, 0x13);
	txBuffer->crypto_header[0] = 1; // 0 for decrypt, 1 for encrypt
	txBuffer->block_number++;
	xil_printf("----------- Before sending to Crypto Module -----------\r\n");
	int print_len = txBuffer->block_number * 16 + 4 * sizeof(int) + TAG_LENGTH + KEY_LENGTH + RESERVED_LENGTH + CRYPTO_HEADER_LENGTH + TAG_LENGTH;
	char* print_buffer = (char *) txBuffer;
	for (int i = 0; i < print_len; i++) {
		xil_printf("%02x ", print_buffer[i]);
		if (i % 16 == 15) {
			xil_printf("\r\n");
		}
	}
	xil_printf("----------- Before sending to Crypto Module -----------\r\n");
	txBuffer->crypto_header[0] = 1; // 0 for decrypt, 1 for encrypt
	flushCache((UINTPTR) txBuffer, sizeof(struct crypto_buffer));

	int dma_transer_size;

	dma_transer_size = txBuffer->block_number * 16 + TAG_LENGTH;
	status = DMA_Xfer((UINTPTR) &txBuffer->payload_tag, dma_transer_size, XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS) {
		xil_printf("ERROR: DMA TRANSFER FROM CRYPTO IP TO MEMORY\r\n");
		return XST_FAILURE;
	}

	dma_transer_size = KEY_LENGTH + RESERVED_LENGTH + CRYPTO_HEADER_LENGTH + txBuffer->block_number * 16;
	status = DMA_Xfer((UINTPTR) &txBuffer->key, dma_transer_size, XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS) {
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

	invalidateCache((UINTPTR) txBuffer, sizeof(struct crypto_buffer));
	xil_printf("----------- After sending to Crypto Module -----------\r\n");
	for (int i = 0; i < print_len; i++) {
		xil_printf("%02x ", print_buffer[i]);
		if (i % 16 == 15) {
			xil_printf("\r\n");
		}
	}
	xil_printf("----------- After sending to Crypto Module -----------\r\n");
	crypto_system_busy = 0;
	return XST_SUCCESS;
}

int decrypt(struct crypto_buffer* txBuffer) {
	crypto_system_busy = 1;
	int status;

	char key[] = {0xee, 0x84, 0xe1, 0x9c, 0xda, 0x87, 0xa7, 0x62, 0x91, 0xea, 0xaf, 0x20, 0x54, 0xae, 0xf8, 0x12};
	for (int i = 0; i < 16; i++) {
		txBuffer->key[i] = key[15 - i];
	}
	XTime start_t = 0;
	XTime end_t;
	XTime_SetTime(start_t);
	XTime_GetTime(&start_t);
//	xil_printf("----------- Before sending to Crypto Module -----------\r\n");
//	int print_len = align_to_block(txBuffer->remaining_bytes, 0) + 4 * sizeof(int) + TAG_LENGTH + KEY_LENGTH + RESERVED_LENGTH + CRYPTO_HEADER_LENGTH + TAG_LENGTH;
//	char* print_buffer = (char *) txBuffer;
//	for (int i = 0; i < print_len; i++) {
//		xil_printf("%02x ", print_buffer[i]);
//		if (i % 16 == 15) {
//			xil_printf("\r\n");
//		}
//	}
//	xil_printf("----------- Before sending to Crypto Module -----------\r\n");
	txBuffer->crypto_header[0] = 0; // 0 for decrypt, 1 for encrypt
	flushCache((UINTPTR) txBuffer, sizeof(struct crypto_buffer));

	int dma_transer_size;

	dma_transer_size = align_to_block(txBuffer->remaining_bytes, 0) + TAG_LENGTH;
	status = DMA_Xfer((UINTPTR) &txBuffer->payload_tag, dma_transer_size, XAXIDMA_DEVICE_TO_DMA);
	if (status != XST_SUCCESS) {
		xil_printf("ERROR: DMA TRANSFER FROM CRYPTO IP TO MEMORY\r\n");
		return XST_FAILURE;
	}

	dma_transer_size = KEY_LENGTH + RESERVED_LENGTH + CRYPTO_HEADER_LENGTH + align_to_block(txBuffer->remaining_bytes, 0);
	status = DMA_Xfer((UINTPTR) &txBuffer->key, dma_transer_size, XAXIDMA_DMA_TO_DEVICE);
	if (status != XST_SUCCESS) {
		xil_printf("ERROR: DMA TRANSFER FROM MEMORY TO CRYPTO IP\r\n");
		return XST_FAILURE;
	}

//	xil_printf("Issued both DMA Transfer requests\r\n");

	while (!RxDone || Error);
	if (Error) {
		xil_printf("Error Flag Set\r\n");
		return XST_FAILURE;
	} else {
//		xil_printf("Received data\r\n");
	}

	invalidateCache((UINTPTR) txBuffer, sizeof(struct crypto_buffer));
	XTime_GetTime(&end_t);
	xil_printf("%x\r\n", start_t - end_t);
//	xil_printf("----------- After sending to Crypto Module -----------\r\n");
//	for (int i = 0; i < print_len; i++) {
//		xil_printf("%02x ", print_buffer[i]);
//		if (i % 16 == 15) {
//			xil_printf("\r\n");
//		}
//	}
//	xil_printf("----------- After sending to Crypto Module -----------\r\n");

	char* generated_tag = &txBuffer->payload_tag[align_to_block(txBuffer->remaining_bytes, 0)];
	for (int i = 0; i < TAG_LENGTH; i++) {
		if (generated_tag[i] != txBuffer->recv_tag[i]) {
			xil_printf("Tag incorrect\r\n");
			return XST_FAILURE;
		}
	}
	crypto_system_busy = 0;
	return XST_SUCCESS;
}

void insert_crypto_buffer(u8 *data, u32 data_len, int direction) {

	struct crypto_buffer* tx_buffer;

	if (direction == GS_TO_OBC) {
		tx_buffer = &to_OBC;
		for (int i = 0; i < data_len; i++) {
			switch (tx_buffer->state) {

				case RECV_CRYPTO_HEADER:
					tx_buffer->crypto_header[tx_buffer->block_offset] = data[i];
					tx_buffer->block_offset--;
					if (tx_buffer->block_offset < 4) {
						tx_buffer->remaining_bytes = *((u16*) &tx_buffer->crypto_header[12]);
						if (tx_buffer->remaining_bytes <= 16) {
							tx_buffer->state = RECV_PAYLOAD_LAST_BLOCK;
							tx_buffer->block_offset = tx_buffer->remaining_bytes - 1;
						} else {
							tx_buffer->state = RECV_PAYLOAD;
							tx_buffer->block_offset = 15;
						}
					}
					break;

				case RECV_PAYLOAD:
					tx_buffer->payload_tag[tx_buffer->block_number * 16 + tx_buffer->block_offset] = data[i];
					tx_buffer->block_offset--;
					tx_buffer->remaining_bytes--;

					if (tx_buffer->block_offset < 0) {
						tx_buffer->block_number++;
						if (tx_buffer->remaining_bytes <= 16) {
							tx_buffer->state = RECV_PAYLOAD_LAST_BLOCK;
							tx_buffer->block_offset = tx_buffer->remaining_bytes - 1;
						} else {
							tx_buffer->block_offset = 15;
						}
					}
					break;

				case RECV_PAYLOAD_LAST_BLOCK:
					tx_buffer->payload_tag[tx_buffer->block_number * 16 + tx_buffer->block_offset] = data[i];
					tx_buffer->block_offset--;
					tx_buffer->remaining_bytes--;
					if (tx_buffer->remaining_bytes == 0) {
						tx_buffer->state = RECV_TAG;
						tx_buffer->block_number++;
						tx_buffer->block_offset = 15;
						break;
					}
					break;

				case RECV_TAG:
					tx_buffer->recv_tag[tx_buffer->block_offset] = data[i];
					tx_buffer->block_offset--;
					if (tx_buffer->block_offset < 0) {
						tx_buffer->state = CRYPTO_PACKET_COMPLETE;
						tx_buffer->block_number = 0;
						tx_buffer->block_offset = 15;
						tx_buffer->remaining_bytes = *((u16*) &tx_buffer->crypto_header[12]);
						decrypt(tx_buffer);
						tx_buffer->state = SEND_PAYLOAD_ONLY;
						send_crypto_buffer(tx_buffer);
						while(tx_buffer->remaining_bytes > 0);
						tx_buffer->block_number = 0;
						tx_buffer->block_offset = 15;
						tx_buffer->remaining_bytes = -1;
						tx_buffer->state = RECV_CRYPTO_HEADER;
					}
					break;
			}
		}
	} else {
		tx_buffer = &to_GS;

		for (int i = 0; i < data_len; i++) {
			tx_buffer->payload_tag[tx_buffer->block_number * 16 + tx_buffer->block_offset] = data[i];
			tx_buffer->block_offset--;
			tx_buffer->remaining_bytes++;

			if (tx_buffer->block_offset < 0) {
				tx_buffer->block_number++;
				if (tx_buffer->remaining_bytes == MAX_PAYLOAD_LENGTH) {
					encrypt(tx_buffer);
					tx_buffer->block_number = 0;
					tx_buffer->block_offset = 15;
					tx_buffer->state = SEND_CRYPTO_HEADER;
					send_crypto_buffer(tx_buffer);
					tx_buffer->remaining_bytes = 0;
					tx_buffer->block_number = 0;
					tx_buffer->block_offset = 15;
					tx_buffer->state = RECV_PAYLOAD;
				} else {
					tx_buffer->block_offset = 15;
				}
			}
		}

	}
}

void encrypt_to_GS() {
	struct crypto_buffer* tx_buffer = &to_GS;
	if (tx_buffer->remaining_bytes == 0) {
		return;
	}
	xil_printf("YEY\r\n");
	int i = 0;
	tx_buffer->block_offset++;
	while (tx_buffer->block_offset < 16) {
		tx_buffer->payload_tag[tx_buffer->block_number * 16 + i] = tx_buffer->payload_tag[tx_buffer->block_number * 16 + tx_buffer->block_offset];
		tx_buffer->block_offset++;
		i++;
	}
	encrypt(tx_buffer);
	tx_buffer->block_number = 0;
	tx_buffer->block_offset = 15;
	tx_buffer->remaining_bytes = *((u16*) &tx_buffer->crypto_header[12]);
	tx_buffer->state = SEND_CRYPTO_HEADER;
	send_crypto_buffer(tx_buffer);
	while(tx_buffer->state != RECV_PAYLOAD);
	tx_buffer->block_number = 0;
	tx_buffer->block_offset = 15;
	tx_buffer->remaining_bytes = 0;
	tx_buffer->state = RECV_PAYLOAD;

}


int align_to_block(int index, int round_down) {
	int remainder = index % 16;
	int block = index / 16;
	if (remainder == 0 || round_down) {
		return block * 16;
	} else {
		return (block + 1) * 16;
	}
}
