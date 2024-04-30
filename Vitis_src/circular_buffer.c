#include "xstatus.h"
#include "circular_buffer.h"

int init_circular_buffer(struct circular_buffer* c_buffer) {
	c_buffer->head = 0;
	c_buffer->tail = 0;
	c_buffer->is_empty = 1;
	return XST_SUCCESS;
}

int circular_buffer_insert(struct circular_buffer* c_buffer, u8* data, int data_len) {
	if (c_buffer->is_empty) {
		for (int i = 0; i < data_len; i++) {
			c_buffer->buffer[c_buffer->tail] = data[i];
			c_buffer->tail = (c_buffer->tail + 1) % MAX_BUFFER_SIZE;
		}
		c_buffer->is_empty = 0;
		return data_len;
	} else {
		int i;
		for (i = 0; i < data_len; i++) {
			if (c_buffer->tail == c_buffer->head) {
				// circular buffer full
				break;
			} else {
				c_buffer->buffer[c_buffer->tail] = data[i];
				c_buffer->tail = (c_buffer->tail + 1) % MAX_BUFFER_SIZE;
			}
		}
		return i;
	}
}

int circular_buffer_pop(struct circular_buffer* c_buffer, u8* data, int max_data_len) {
	if (c_buffer->is_empty) {
		return 0;
	} else {
		int i;
		for (i = 0; i < max_data_len; i++) {
			if (c_buffer->head == c_buffer->tail) {
				// circular buffer empty
				c_buffer->is_empty = 1;
				break;
			} else {
				data[i] = c_buffer->buffer[c_buffer->head];
				c_buffer->head = (c_buffer->head + 1 ) % MAX_BUFFER_SIZE;
			}
		}
		if (c_buffer->head == c_buffer->tail) {
			// circular buffer empty
			c_buffer->is_empty = 1;
		}
		return i;
	}
}
