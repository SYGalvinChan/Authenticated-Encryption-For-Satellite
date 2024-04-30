#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#define MAX_BUFFER_SIZE 0xffff

struct circular_buffer {
	int head;
	int tail;
	int is_empty;
	char buffer[MAX_BUFFER_SIZE];
};

int init_circular_buffer(struct circular_buffer* c_buffer);
int circular_buffer_insert(struct circular_buffer* c_buffer, u8* data, int data_len);
int circular_buffer_pop(struct circular_buffer* c_buffer, u8* data, int max_data_len);

#endif
