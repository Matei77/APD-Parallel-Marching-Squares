// Author: APD team, except where source was noted

#include "helpers.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "types.h"

#define CONTOUR_CONFIG_COUNT    16
#define FILENAME_MAX_SIZE       50
#define STEP                    8
#define SIGMA                   200
#define RESCALE_X               2048
#define RESCALE_Y               2048
#define MAX_THREADS_NR          12

#define CLAMP(v, min, max) if(v < min) { v = min; } else if(v > max) { v = max; }

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

// Creates a map between the binary configuration (e.g. 0110_2) and the corresponding pixels
// that need to be set on the output image. An array is used for this map since the keys are
// binary numbers in 0-15. Contour images are located in the './contours' directory.
ppm_image **init_contour_map() {
    ppm_image **map = (ppm_image **)malloc(CONTOUR_CONFIG_COUNT * sizeof(ppm_image *));
    if (!map) {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }

    for (int i = 0; i < CONTOUR_CONFIG_COUNT; i++) {
        char filename[FILENAME_MAX_SIZE];
        sprintf(filename, "./contours/%d.ppm", i);
        map[i] = read_ppm(filename);
    }

    return map;
}

ppm_image *rescale_image(ppm_image *image) {
    uint8_t sample[3];

    // we only rescale downwards
    if (image->x <= RESCALE_X && image->y <= RESCALE_Y) {
        return image;
    }

    // alloc memory for image
    ppm_image *new_image = (ppm_image *)malloc(sizeof(ppm_image));
    if (!new_image) {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }
    new_image->x = RESCALE_X;
    new_image->y = RESCALE_Y;

    new_image->data = (ppm_pixel*)malloc(new_image->x * new_image->y * sizeof(ppm_pixel));
    if (!new_image) {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }

    // use bicubic interpolation for scaling
    for (int i = 0; i < new_image->x; i++) {
        for (int j = 0; j < new_image->y; j++) {
            float u = (float)i / (float)(new_image->x - 1);
            float v = (float)j / (float)(new_image->y - 1);
            sample_bicubic(image, u, v, sample);

            new_image->data[i * new_image->y + j].red = sample[0];
            new_image->data[i * new_image->y + j].green = sample[1];
            new_image->data[i * new_image->y + j].blue = sample[2];
        }
    }

    free(image->data);
    free(image);

    return new_image;
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

// Calls `free` method on the utilized resources.
void free_resources(ppm_image *image, ppm_image **contour_map, unsigned char **grid, int step_x) {
    for (int i = 0; i < CONTOUR_CONFIG_COUNT; i++) {
        free(contour_map[i]->data);
        free(contour_map[i]);
    }
    free(contour_map);

    for (int i = 0; i <= image->x / step_x; i++) {
        free(grid[i]);
    }
    free(grid);

    free(image->data);
    free(image);
}


// Allocates memory for a grid based on the image size and step size
unsigned char **create_grid(ppm_image *image, int step_x, int step_y) {
    int grid_x_points_nr = image->x / step_x;
    int grid_y_points_nr = image->y / step_y;

    
    unsigned char **grid = (unsigned char **)malloc((grid_x_points_nr + 1) * sizeof(unsigned char*));
    if (!grid) {
        fprintf(stderr, "Unable to allocate memory\n");
        exit(1);
    }

    for (int i = 0; i <= grid_x_points_nr; i++) {
        grid[i] = (unsigned char *)malloc((grid_y_points_nr + 1) * sizeof(unsigned char));
        if (!grid[i]) {
            fprintf(stderr, "Unable to allocate memory\n");
            exit(1);
        }
    }

    return grid;
}

void *thread_function(void *arg) {
    // creating the grid
    thread_arg_t *thread_arg = (thread_arg_t *) arg;
    int step_x = STEP;
    int step_y = STEP;
    ppm_image *image = thread_arg->image;
    unsigned char **grid = thread_arg->grid;

    // get number of points in the grid on x and y axis
    int grid_x_points_nr = image->x / step_x;
    int grid_y_points_nr = image->y / step_y;

    
    // set the start and end index for each thread
    int start_x = thread_arg->thread_id * (double)grid_x_points_nr / thread_arg->nr_threads;            
	int end_x = MIN((thread_arg->thread_id + 1) * (double)grid_x_points_nr / thread_arg->nr_threads, grid_x_points_nr);
    printf("thread: %d is computing lines %d-%d\n", thread_arg->thread_id, start_x, end_x);

    int start_y = thread_arg->thread_id * (double)grid_y_points_nr / thread_arg->nr_threads;            
	int end_y = MIN((thread_arg->thread_id + 1) * (double)grid_y_points_nr / thread_arg->nr_threads, grid_y_points_nr);

    // build a grid of points with values which can be either 0 or 1, depending on how the
    // pixel values compare to the `sigma` reference value.
    for (int i = start_x; i < end_x; i++) {
        for (int j = 0; j < grid_y_points_nr; j++) {
            ppm_pixel curr_pixel = image->data[i * step_x * image->y + j * step_y];

            unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

            if (curr_color > SIGMA) {
                grid[i][j] = 0;
            } else {
                grid[i][j] = 1;
            }
        }
    }
    grid[grid_x_points_nr][grid_y_points_nr] = 0;


    for (int i = start_x; i < end_x; i++) {
        ppm_pixel curr_pixel = image->data[i * step_x * image->y + image->x - 1];

        unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

        if (curr_color > SIGMA) {
            grid[i][grid_y_points_nr] = 0;
        } else {
            grid[i][grid_y_points_nr] = 1;
        }
    }
    for (int j = start_y; j < end_y; j++) {
        ppm_pixel curr_pixel = image->data[(image->x - 1) * image->y + j * step_y];

        unsigned char curr_color = (curr_pixel.red + curr_pixel.green + curr_pixel.blue) / 3;

        if (curr_color > SIGMA) {
            grid[grid_x_points_nr][j] = 0;
        } else {
            grid[grid_x_points_nr][j] = 1;
        }
    }


    pthread_barrier_wait(thread_arg->barrier);

    for (int i = start_x; i < end_x; i++) {
        for (int j = 0; j < grid_y_points_nr; j++) {
            unsigned char k = 8 * grid[i][j] + 4 * grid[i][j + 1] + 2 * grid[i + 1][j + 1] + 1 * grid[i + 1][j];
            update_image(image, thread_arg->contour_map[k], i * step_x, j * step_y);
        }
    }
    
    pthread_exit(NULL);
}

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

    // initialize barrier
	pthread_barrier_init(&barrier, NULL, nr_threads);

    // read image from file
    ppm_image *image = read_ppm(argv[1]);

    // initialize contour map
    ppm_image **contour_map = init_contour_map();

    // rescale the image
    ppm_image *scaled_image = rescale_image(image);
	
    // create grid
    unsigned char **grid = create_grid(scaled_image, STEP, STEP);

    // create the threads
	for (int i = 0; i < nr_threads; i++) {
        // set thread arguments
        thread_args[i].thread_id = i;
        thread_args[i].nr_threads = nr_threads;
        thread_args[i].barrier = &barrier;
        thread_args[i].contour_map = contour_map;
        thread_args[i].image = scaled_image;
        thread_args[i].grid = grid;

        // create the thread
		rc = pthread_create(&tid[i], NULL, thread_function, &thread_args[i]);

        // check if the thread was created successfully
        if (rc) {
            printf("ERROR: failed to create thread number %d\n", i);
            exit(1);
        }
	}

    // 3. March the squares
    // march(scaled_image, grid, contour_map, step_x, step_y);

	// join the threads
	for (int i = 0; i < nr_threads; i++) {
		rc = pthread_join(tid[i], NULL);

        // check if the thread was joined successfully
        if (rc) {
            printf("ERROR: failed to join thread number %d\n", i);
            exit(1);
        }
	}

    // 4. Write output
    write_ppm(scaled_image, argv[2]);

    free_resources(scaled_image, contour_map, grid, STEP);

    pthread_barrier_destroy(&barrier);

    return 0;
}
