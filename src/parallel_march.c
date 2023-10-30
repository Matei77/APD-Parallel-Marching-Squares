// Copyright: Ionescu Matei-Stefan - 333CAb - 2023-2024
#include "parallel_march.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"
#include "helpers.h"
#include "types.h"

#define STEP 8
#define SIGMA 200

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))


void bicubic_interpolation(thread_arg_t *arg) {
	ppm_image *scaled_image = arg->scaled_image;
	ppm_image *image = arg->image;

	uint8_t sample[3];

	// set the start and end index for each thread
	int start = arg->thread_id * (double)scaled_image->x / arg->nr_threads;
	int end = MIN((arg->thread_id + 1) * (double)scaled_image->x / arg->nr_threads,
	 			  scaled_image->x);

	// use bicubic interpolation for scaling
	for (int i = start; i < end; i++) {
		for (int j = 0; j < scaled_image->y; j++) {
			float u = (float)i / (float)(scaled_image->x - 1);
			float v = (float)j / (float)(scaled_image->y - 1);
			sample_bicubic(image, u, v, sample);

			scaled_image->data[i * scaled_image->y + j].red = sample[0];
			scaled_image->data[i * scaled_image->y + j].green = sample[1];
			scaled_image->data[i * scaled_image->y + j].blue = sample[2];
		}
	}
}

// Builds a grid of points with values which can be either 0 or 1, depending on how the
// pixel values compare to the `sigma` reference value.
void bulid_grid_of_points(thread_arg_t *arg) {
	int step_x, step_y;
	step_x = step_y = STEP;
	ppm_image *image = arg->scaled_image;
    unsigned char **grid = arg->grid;

	// get number of points in the grid on x and y axis
    int grid_x_points = image->x / step_x;
    int grid_y_points = image->y / step_y;

    
    // set the start and end index for each thread
    int start_x = arg->thread_id * (double)grid_x_points / arg->nr_threads;            
	int end_x = MIN((arg->thread_id + 1) * (double)grid_x_points / arg->nr_threads, grid_x_points);

    int start_y = arg->thread_id * (double)grid_y_points / arg->nr_threads;            
	int end_y = MIN((arg->thread_id + 1) * (double)grid_y_points / arg->nr_threads, grid_y_points);

    // build a grid of points with values which can be either 0 or 1, depending on how the
    // pixel values compare to the `sigma` reference value.
    for (int i = start_x; i < end_x; i++) {
        for (int j = 0; j < grid_y_points; j++) {
            ppm_pixel curr_pixel = image->data[i * step_x * image->y + j * step_y];

            unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

            if (curr_color > SIGMA) {
                grid[i][j] = 0;
            } else {
                grid[i][j] = 1;
            }
        }
    }
    grid[grid_x_points][grid_y_points] = 0;


    for (int i = start_x; i < end_x; i++) {
        ppm_pixel curr_pixel = image->data[i * step_x * image->y + image->x - 1];

        unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

        if (curr_color > SIGMA) {
            grid[i][grid_y_points] = 0;
        } else {
            grid[i][grid_y_points] = 1;
        }
    }
    for (int j = start_y; j < end_y; j++) {
        ppm_pixel curr_pixel = image->data[(image->x - 1) * image->y + j * step_y];

        unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

        if (curr_color > SIGMA) {
            grid[grid_x_points][j] = 0;
        } else {
            grid[grid_x_points][j] = 1;
        }
    }
}

void march(thread_arg_t *arg) {
	int step_x, step_y;
	step_x = step_y = STEP;
	ppm_image *image = arg->scaled_image;
    unsigned char **grid = arg->grid;

	// get number of points in the grid on x and y axis
    int grid_x_points = image->x / step_x;
    int grid_y_points = image->y / step_y;

    
    // set the start and end index for each thread
    int start = arg->thread_id * (double)grid_x_points / arg->nr_threads;            
	int end = MIN((arg->thread_id + 1) * (double)grid_x_points / arg->nr_threads, grid_x_points);

	for (int i = start; i < end; i++) {
        for (int j = 0; j < grid_y_points; j++) {
            unsigned char k = 8 * grid[i][j] + 4 * grid[i][j + 1] + 2 * grid[i + 1][j + 1] + 1 * grid[i + 1][j];
            update_image(image, arg->contour_map[k], i * step_x, j * step_y);
        }
    }
}

void *thread_function(void *arg) {
	thread_arg_t *thread_arg = (thread_arg_t *)arg;

	if (thread_arg->image != thread_arg->scaled_image) {
		bicubic_interpolation(thread_arg);
		pthread_barrier_wait(thread_arg->barrier);
	}

	bulid_grid_of_points(thread_arg);
	pthread_barrier_wait(thread_arg->barrier);

	march(thread_arg);

	pthread_exit(NULL);
}