// Copyright: Ionescu Matei-Stefan - 333CAb - 2023-2024
#ifndef TYPES_H_
#define TYPES_H_

#include <pthread.h>

#include "helpers.h"

typedef struct {
	int thread_id;
	int nr_threads;
	pthread_barrier_t *barrier;
	ppm_image **contour_map;
	ppm_image *image;
	ppm_image *scaled_image;
	unsigned char **grid;

} thread_arg_t;

#endif // TYPES_H_