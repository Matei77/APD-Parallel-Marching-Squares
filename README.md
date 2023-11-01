**Name: Ionescu Matei-Ștefan**  
**Group: 333CAb**

# APD Homework #1 - Parallel Marching Squares Algorithm

The program creates the contour of topological maps utilizing the marching
squares alorithm. The program is parallelized using the Pthreads library.

## Project organization
- **tema1_par.c** - contains the ``main`` function which is *mainly* used for
various intializations and thread creation.

- **parallel_march.c** - contains the ``thread_function`` which is called by
each thread. Also, the functions ``march``, ``build_grid_of_points`` and
``bicubilc_interpolation`` are present in this file and are called in the
``thread_function`` in order to create the topological map in parallel.

- **utils.c** - contains the functions used to allocate and free momory,
``init_men`` and ``free_resources``, and the ``update_image`` function which
updates a particular section of an image with the corresponding countour pixel.

- **types.h** - contains the definition of the ``thread_arg_t`` type which is
used to pass arguments to the ``thread_function``.

- **helpers.c** - contains functions provided by the APD Team, that are used
to read the input and write the output and be utilized in some steps of the
marching squares algorithm.


## How the program works step by step
1. When the program starts the ``main`` function is called and reads the input,
initializes memory and then creates the threads.

2. Each thread will start in the ``thread_function``. If the image read needs
to be scaled the ``bicubic_interpolation`` function will be called and the
threads will then wait at the barrier until all of them are done with the
scaling task.

3. Each thread will call the ``bulid_grid_of_points`` function, then start
working to build the grid of points required in order to create the topological
map. The threads will wait at a barrier until all of them finish.

4. Finally the treads will call the ``march`` function and will start to update
the image, changing each section of the initial image with the corresponding
contour.

5. Going back to the ``main`` function, the threads are joined, the output is
written and the resources are freed.


## Paralellization
Three parts of the algorithm have been parallelized:
- The **bicubic interpolation** has been paralellized by assigning each thread
an equal number of lines of pixels to work on

- The **building of the grid** has been parallelized by assigning each thread
an equal number of lines of points, for which to calculate the binary value.

- The last part of the Marching Squares Algorithm, which consists of **updating
the initial image** by chaning sections of the image with its corresponding
countours, has been parallelized by assigning each thread an equal number of
lines of points for which to change the sections accordingly.

## Syncronization
The threads are syncronized using a **barrier**. The syncronization happens after
the image scaling using bicubic interpolation step and after building the grid
of points. 

## Notes
- The program passes the test on the checker with the score 120/120p.
- The speed-up of creating 2 threads is around 2.00.
- The speed-up of creating 4 threads is around 3.30.
