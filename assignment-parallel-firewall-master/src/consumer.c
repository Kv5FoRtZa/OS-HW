// SPDX-License-Identifier: BSD-3-Clause

#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "consumer.h"
#include "ring_buffer.h"
#include "packet.h"
#include "utils.h"
so_consumer_ctx_t *context;
//FILE *log_file;
void *consumer_thread(void *arg)
{
	so_consumer_ctx_t *ctx = arg;
	so_packet_t  pachet;

	while (1) {
		int sz = ring_buffer_dequeue(ctx->producer_rb, &pachet, PKT_SZ);

		if (sz == -1)
			break;
			//pthread_mutex_unlock (ctx->dequeue_mutex);
		//pthread_mutex_lock (ctx->dequeue_mutex);
		//size_t my_index = * (ctx->next_index_assigned);
		// (* (ctx->next_index_assigned))++;
		//pthread_mutex_unlock (ctx->dequeue_mutex);
		size_t my_index = so_rb_last_index;
		so_action_t rez = process_packet(&pachet);
		unsigned long hash = packet_hash(&pachet);
		unsigned long timestamp = pachet.hdr.timestamp;
		char sir[64];
		int len = snprintf(sir, sizeof(sir), "%s %016lx %lu\n", RES_TO_STR(rez), hash, timestamp);

		pthread_mutex_lock(ctx->log_mutex);

		while (my_index != *(ctx->next_index_to_log))
			pthread_cond_wait(ctx->log_cond, ctx->log_mutex);
		//if (rez == DROP)
		{
			fwrite(sir, 1, len, ctx->log_file);
			fflush(ctx->log_file);
		}
		//else
		{
			//fprintf (ctx->log_file, "%s %016lx %lu\n", RES_TO_STR (rez),hash,timestamp);
		}
		(*(ctx->next_index_to_log))++;
		pthread_cond_broadcast(ctx->log_cond);
		pthread_mutex_unlock(ctx->log_mutex);
	}
	return NULL;
}

int create_consumers(pthread_t *tids,
					 int num_consumers,
					 struct so_ring_buffer_t *rb,
					 const char *out_filename)
{
	// (void) tids;
	// (void) num_consumers;
	// (void) rb;
	// (void) out_filename;
	FILE *log_file = fopen(out_filename, "w");
	//ring_buffer_init (&rb,1);
	pthread_mutex_t *log_mutex = malloc(sizeof(pthread_mutex_t));
	pthread_mutex_t *dequeue_mutex = malloc(sizeof(pthread_mutex_t));
	pthread_cond_t  *log_cond  = malloc(sizeof(pthread_cond_t));
	size_t *next_index_to_log = malloc(sizeof(size_t));
	size_t *next_index_assigned = malloc(sizeof(size_t));

	pthread_mutex_init(log_mutex, NULL);
	pthread_mutex_init(dequeue_mutex, NULL);
	pthread_cond_init(log_cond, NULL);
	*next_index_to_log   = 0;
	*next_index_assigned = 0;
	context = calloc(num_consumers, sizeof(so_consumer_ctx_t));
	for (int i = 0; i < num_consumers; i++) {
		context[i].log_file = log_file;
		context[i].producer_rb = rb;
		context[i].log_mutex = log_mutex;
		context[i].dequeue_mutex = dequeue_mutex;
		context[i].log_cond = log_cond;
		context[i].next_index_to_log   = next_index_to_log;
		context[i].next_index_assigned = next_index_assigned;
		pthread_create(&tids[i], NULL, consumer_thread, &context[i]);
	}
	//fclose (log_file);
	//ring_buffer_destroy (&rb);
	return num_consumers;
}

