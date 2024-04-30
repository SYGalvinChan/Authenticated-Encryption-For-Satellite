#ifndef CRYPTO_MODULE_H
#define CRYPTO_MODULE_H
#include "dma.h"

#define KEY_LENGTH 16
#define RESERVED_LENGTH 4
#define CRYPTO_HEADER_LENGTH 12
#define MAX_PAYLOAD_LENGTH 65535
#define TAG_LENGTH 16

// Directions
#define GS_TO_OBC 0
#define OBC_TO_GS 1

// States for inserting into crypto buffer
#define RECV_CRYPTO_HEADER 		0
#define RECV_PAYLOAD       		1
#define RECV_PAYLOAD_LAST_BLOCK 2
#define RECV_TAG           		3
#define CRYPTO_PACKET_COMPLETE  4

// States for sending crypto buffer over CAN Bus
#define SEND_CRYPTO_HEADER 		5
#define SEND_PAYLOAD_ONLY  		6
#define SEND_PAYLOAD_WITH_TAG   7
#define SEND_TAG           		8

struct crypto_buffer {
	int block_number;
	int block_offset;
	int remaining_bytes;
	int state;
	char recv_tag[TAG_LENGTH];
	char key[KEY_LENGTH];
	char crypto_header[RESERVED_LENGTH + CRYPTO_HEADER_LENGTH];
	char payload_tag[MAX_PAYLOAD_LENGTH + TAG_LENGTH];
};

int init_crypto_system();
int encrypt(struct crypto_buffer* txBuffer);
int decrypt(struct crypto_buffer* txBuffer);
void insert_crypto_buffer(u8 *data, u32 data_len, int direction);
void encrypt_to_GS();

int align_to_block(int index, int round_down);
#endif // CRYPTO_MODULE_H
