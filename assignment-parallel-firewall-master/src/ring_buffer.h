/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __SO_RINGBUFFER_H__
#define __SO_RINGBUFFER_H__

#include <sys/types.h>
#include <string.h>
#include <pthread.h>
extern _Thread_local size_t so_rb_last_index;
typedef struct so_ring_buffer_t {
	char *data;//pachetele
	int stop_pt_fstop;
	size_t read_pos;//unde citeste next
	size_t write_pos;//unde scrie next
	//size_t next_index;
	size_t len;// cate chestiii sunt in buffer
	size_t cap;//capacitatea
	/* TODO: Add syncronization primitives */
	pthread_mutex_t lock; // se blocheaza
    pthread_cond_t can_write; // mai e loc in el
    pthread_cond_t can_read; // nu e gol
	size_t next_index;
} so_ring_buffer_t;

int     ring_buffer_init(so_ring_buffer_t *rb, size_t cap);
ssize_t ring_buffer_enqueue(so_ring_buffer_t *rb, void *data, size_t size);
ssize_t ring_buffer_dequeue(so_ring_buffer_t *rb, void *data, size_t size);
void    ring_buffer_destroy(so_ring_buffer_t *rb);
void    ring_buffer_stop(so_ring_buffer_t *rb);

#endif /* __SO_RINGBUFFER_H__ */
