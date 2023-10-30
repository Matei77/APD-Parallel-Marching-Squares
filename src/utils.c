// Copyright: Ionescu Matei-Stefan - 333CAb - 2023-2024
#include "utils.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "helpers.h"
#include "types.h"

#define CONTOUR_CONFIG_COUNT 16
#define FILENAME_MAX_SIZE 50
#define STEP 8
#define RESCALE_X 2048
#define RESCALE_Y 2048

// Allocates memory for the contour_map, grid and, if necessary, for the scaled image.
void init_mem(ppm_image **scaled_image, ppm_image **image, ppm_image ***contour_map,
			  unsigned char ***grid) {
	// allocate memory for countour_map
	*contour_map = (ppm_image **)malloc(CONTOUR_CONFIG_COUNT * sizeof(ppm_image *));
	if (!(*contour_map)) {
		fprintf(stderr, "Unable to allocate memory\n");
		exit(1);
	}

	for (int i = 0; i < CONTOUR_CONFIG_COUNT; i++) {
		char filename[FILENAME_MAX_SIZE];
		sprintf(filename, "./contours/%d.ppm", i);
		(*contour_map)[i] = read_ppm(filename);
	}

	// by default the scaled image is the same as the original image
	*scaled_image = *image;

	// if the the image exceeds the limits, allocate memory for the scaled image
	if (!((*image)->x <= RESCALE_X && (*image)->y <= RESCALE_Y)) {
		ppm_image *new_image = (ppm_image *)malloc(sizeof(ppm_image));
		if (!new_image) {
			fprintf(stderr, "Unable to allocate memory\n");
			exit(1);
		}
		new_image->x = RESCALE_X;
		new_image->y = RESCALE_Y;

		new_image->data = (ppm_pixel *)malloc(new_image->x * new_image->y * sizeof(ppm_pixel));
		if (!new_image) {
			fprintf(stderr, "Unable to allocate memory\n");
			exit(1);
		}

		*scaled_image = new_image;
	}

	// allocate memory for the grid
	int grid_x_points_nr = (*scaled_image)->x / STEP;
	int grid_y_points_nr = (*scaled_image)->y / STEP;

	*grid = (unsigned char **)malloc((grid_x_points_nr + 1) * sizeof(unsigned char *));
	if (!(*grid)) {
		fprintf(stderr, "Unable to allocate memory\n");
		exit(1);
	}

	for (int i = 0; i <= grid_x_points_nr; i++) {
		(*grid)[i] = (unsigned char *)malloc((grid_y_points_nr + 1) * sizeof(unsigned char));
		if (!(*grid)[i]) {
			fprintf(stderr, "Unable to allocate memory\n");
			exit(1);
		}
	}
}

// Calls `free` method on the utilized resources.
void free_resources(ppm_image *scaled_image, ppm_image *image, ppm_image **contour_map,
					unsigned char **grid, int step_x) {
	for (int i = 0; i < CONTOUR_CONFIG_COUNT; i++) {
		free(contour_map[i]->data);
		free(contour_map[i]);
	}
	free(contour_map);

	for (int i = 0; i <= scaled_image->x / step_x; i++) {
		free(grid[i]);
	}
	free(grid);

	if (image != scaled_image) {
		free(image->data);
		free(image);
	}

	free(scaled_image->data);
	free(scaled_image);
}

// Updates a particular section of an image with the corresponding contour pixels.
// Used to create the complete contour image.
void update_image(ppm_image *image, ppm_image *contour, int x, int y) {
	for (int i = 0; i < contour->x; i++) {
		for (int j = 0; j < contour->y; j++) {
			int contour_pixel_index = contour->x * i + j;
			int image_pixel_index = (x + i) * image->y + y + j;

			image->data[image_pixel_index].red = contour->data[contour_pixel_index].red;
			image->data[image_pixel_index].green = contour->data[contour_pixel_index].green;
			image->data[image_pixel_index].blue = contour->data[contour_pixel_index].blue;
		}
	}
}
