/* SPDX-License-Identifier: BSD-3-Clause */

#ifndef __SO_CONSUMER_H__
#define __SO_CONSUMER_H__
#include <stdio.h>
#include "ring_buffer.h"
#include "packet.h"
typedef struct so_consumer_ctx_t {
	struct so_ring_buffer_t *producer_rb;
	FILE *log_file;
	pthread_mutex_t *log_mutex;
	pthread_mutex_t *dequeue_mutex;
	pthread_cond_t *log_cond;
	size_t *next_index_to_log;
    size_t *next_index_assigned;
    /* TODO: add synchronization primitives for timestamp ordering */
} so_consumer_ctx_t;
extern so_consumer_ctx_t *context;
int create_consumers(pthread_t *tids,
					int num_consumers,
					so_ring_buffer_t *rb,
					const char *out_filename);

#endif /* __SO_CONSUMER_H__ */
