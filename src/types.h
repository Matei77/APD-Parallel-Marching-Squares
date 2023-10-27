#ifndef TYPES_H_
#define TYPES_H_

#include <pthread.h>

typedef struct {
	int thread_id;
	int nr_threads;
	pthread_barrier_t *barrier;
	ppm_image **contour_map;
	ppm_image *image;
	unsigned char **grid;

} thread_arg_t;

#endif