// SPDX-License-Identifier: BSD-3-Clause

#include "ring_buffer.h"
#include <stdlib.h>
#include <string.h>
_Thread_local size_t so_rb_last_index;
int ring_buffer_init(so_ring_buffer_t *ring, size_t cap)
{
	/* TODO: implement ring_buffer_init */
	ring->stop_pt_fstop = 0;
	ring->cap = cap;
	ring->len = 0;
	ring->read_pos = 0;
	ring->write_pos = 0;
	ring->next_index = 0;
	ring->data = (char *)malloc(cap * sizeof(char));
	//pthread_mutex_t *lo;
	//pthread_mutex_init (lo, NULL);
	//ring->lock = *lo;
	//pthread_cond_t *cr;
	//pthread_mutex_init (cr, NULL);
	//ring->can_read = *cr;
	//pthread_cond_t *cw ;
	//pthread_mutex_init (cw, NULL);
	//ring->can_write = *cw;
	pthread_mutex_init(&ring->lock, NULL);
	pthread_cond_init(&ring->can_read, NULL);
	pthread_cond_init(&ring->can_write, NULL);
	return 0;
}

ssize_t ring_buffer_enqueue(so_ring_buffer_t *ring, void *data, size_t size)
{
	pthread_mutex_lock(&ring->lock);
	while (ring->len + size > ring->cap) {
		if (ring->stop_pt_fstop == 1) {
			pthread_mutex_unlock(&ring->lock);
			return -1;
		}
		pthread_cond_wait(&ring->can_write, &ring->lock);
	}
	//for (size_t i = 0; i < size; i ++)
	//{
		//ring->data[ring->write_pos] = ( (char *)data)[i];
		//ring->write_pos ++;
		//ring->write_pos = ring->write_pos % ring->cap;
	//
	if (size <= ring->cap - ring->write_pos) {
		memcpy(ring->data + ring->write_pos, data, size);
	} else {
		memcpy(ring->data + ring->write_pos, data, ring->cap - ring->write_pos);
		memcpy(ring->data, ((char *)data) + (ring->cap - ring->write_pos), size - (ring->cap - ring->write_pos));
	}
	ring->write_pos = (ring->write_pos + size) % ring->cap;
	ring->len += size;
	pthread_cond_signal(&ring->can_read);
	pthread_mutex_unlock(&ring->lock);

	return size;
}

ssize_t ring_buffer_dequeue(so_ring_buffer_t *ring, void *data, size_t size)
{
	pthread_mutex_lock(&ring->lock);
	//if (ring->stop_pt_fstop == 0)
	{
		while (ring->len < /*0 +*/  size) {
			if (ring->stop_pt_fstop == 1) {
				pthread_mutex_unlock(&ring->lock);
				return -1;
			}
			pthread_cond_wait(&ring->can_read, &ring->lock);
		}
		//for (size_t i = 0; i < size; i ++)
		//{
			 //( (char *)data)[i] = ring->data[ring->read_pos];
			//ring->read_pos ++;
			//ring->read_pos = ring->read_pos % ring->cap;
		//}
		if (size <= ring->cap - ring->read_pos) {
			memcpy(data, ring->data + ring->read_pos, size);
		} else {
			memcpy(data, ring->data + ring->read_pos, ring->cap - ring->read_pos);
			memcpy(((char *)data) + (ring->cap - ring->read_pos), ring->data, size - (ring->cap - ring->read_pos));
		}
		ring->read_pos = (ring->read_pos + size) % ring->cap;
		ring->len -= size;
		so_rb_last_index = ring->next_index++;
		pthread_cond_signal(&ring->can_write);
	}
	pthread_mutex_unlock(&ring->lock);

	return size;
}

void ring_buffer_destroy(so_ring_buffer_t *ring)
{
	pthread_mutex_destroy(&ring->lock);
	pthread_cond_destroy(&ring->can_read);
	pthread_cond_destroy(&ring->can_write);
	free(ring->data);
	//free (ring->can_read);
	//free (ring->lock);
	//free (ring->can_write);
}

void ring_buffer_stop(so_ring_buffer_t *ring)
{
	pthread_mutex_lock(&ring->lock);
	ring->stop_pt_fstop = 1;
	pthread_cond_broadcast(&ring->can_read);
	pthread_cond_broadcast(&ring->can_write);
	pthread_mutex_unlock(&ring->lock);
}
