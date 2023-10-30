// Copyright: Ionescu Matei-Stefan - 333CAb - 2023-2024
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "helpers.h"
#include "parallel_march.h"
#include "types.h"
#include "utils.h"

#define MAX_THREADS_NR 12

int main(int argc, char *argv[]) {
	if (argc < 4) {
		fprintf(stderr, "Usage: ./tema1 <in_file> <out_file> <P>\n");
		return 1;
	}

	// set the number of threads used
	int nr_threads = atoi(argv[3]);

	int rc;
	pthread_barrier_t barrier;
	pthread_t tid[MAX_THREADS_NR];
	thread_arg_t thread_args[MAX_THREADS_NR];

	ppm_image *image = NULL;
	ppm_image *scaled_image = NULL;
	ppm_image **contour_map = NULL;
	unsigned char **grid = NULL;

	// initialize barrier
	pthread_barrier_init(&barrier, NULL, nr_threads);

	// read image from file
	image = read_ppm(argv[1]);

	// allocate initial memory
	init_mem(&scaled_image, &image, &contour_map, &grid);

	// create the threads
	for (int i = 0; i < nr_threads; i++) {
		// set thread arguments
		thread_args[i].thread_id = i;
		thread_args[i].nr_threads = nr_threads;
		thread_args[i].barrier = &barrier;
		thread_args[i].contour_map = contour_map;
		thread_args[i].image = image;
		thread_args[i].scaled_image = scaled_image;
		thread_args[i].grid = grid;

		// create the thread
		rc = pthread_create(&tid[i], NULL, thread_function, &thread_args[i]);

		// check if the thread was created successfully
		if (rc) {
			printf("ERROR: failed to create thread number %d\n", i);
			exit(1);
		}
	}

	// join the threads
	for (int i = 0; i < nr_threads; i++) {
		rc = pthread_join(tid[i], NULL);

		// check if the thread was joined successfully
		if (rc) {
			printf("ERROR: failed to join thread number %d\n", i);
			exit(1);
		}
	}

	// write output
	write_ppm(scaled_image, argv[2]);

	// free all the allocated memory
	free_resources(scaled_image, image, contour_map, grid, STEP);

	// destroy barrier
	pthread_barrier_destroy(&barrier);

	return 0;
}