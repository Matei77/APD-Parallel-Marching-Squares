// Copyright: Ionescu Matei-Stefan - 333CAb - 2023-2024
#ifndef UTILS_H_
#define UTILS_H_

#include "helpers.h"

// Allocates memory for the contour_map, grid and, if necessary, for the scaled image.
void init_mem(ppm_image **scaled_image, ppm_image **image, ppm_image ***contour_map,
			  unsigned char ***grid);

// Calls `free` method on the utilized resources.
void free_resources(ppm_image *scaled_image, ppm_image *image, ppm_image **contour_map,
					unsigned char **grid, int step_x);

// Updates a particular section of an image with the corresponding contour pixels.
// Used to create the complete contour image.
void update_image(ppm_image *image, ppm_image *contour, int x, int y);

#endif  // UTILS_H_