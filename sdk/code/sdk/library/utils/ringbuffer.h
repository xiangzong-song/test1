/*
 * Lite-Rbuffer.h
 *
 *  Created on: 2020-9-27
 *      Author: Weili.Hu
 */

#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_
#include <stdint.h>

typedef struct _LITE_RBUFFER* LR_handler;

LR_handler Lite_ring_buffer_init(int length);
void Lite_ring_buffer_deinit(LR_handler handler);
int Lite_ring_buffer_write_data(LR_handler handler, uint8_t* p_data, int size);
int Lite_ring_buffer_read_data(LR_handler handler, uint8_t* p_buffer, int size);
int Lite_ring_buffer_left_get(LR_handler handler);
int Lite_ring_buffer_size_get(LR_handler handler);
void Lite_ring_buffer_print(LR_handler handler, int b_newline);

#endif /* LITE_RBUFFER_H_ */
