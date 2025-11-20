/*
 * Mandelbrot Computation Engine - Ultra-Fast C Backend for Python
 * ================================================================
 *
 * High-performance C library that computes Mandelbrot iterations
 * and outputs binary data for Python to visualize with matplotlib.
 *
 * Compilation (creates DLL/shared library for Python):
 *   gcc -O3 -march=native -fopenmp -shared -fPIC mandelbrot_compute.c -o mandelbrot_compute.dll -lm
 *
 * Or on Linux/Mac:
 *   gcc -O3 -march=native -fopenmp -shared -fPIC mandelbrot_compute.c -o mandelbrot_compute.so -lm
 *
 * Or as standalone program (outputs binary data to stdout):
 *   gcc -O3 -march=native -fopenmp -DSTANDALONE mandelbrot_compute.c -o mandelbrot_compute -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef _WIN32
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT
#endif

// Compute Mandelbrot iteration count for a single point
static inline int mandelbrot_point(double cr, double ci, int max_iter) {
    double zr = 0.0, zi = 0.0;
    double zr2 = 0.0, zi2 = 0.0;

    for (int i = 0; i < max_iter; i++) {
        if (zr2 + zi2 > 4.0) {
            return i;
        }
        zi = 2.0 * zr * zi + ci;
        zr = zr2 - zi2 + cr;
        zr2 = zr * zr;
        zi2 = zi * zi;
    }
    return max_iter;
}

// Main computation function exported to Python via ctypes
EXPORT void compute_mandelbrot(
    double xmin, double xmax, int width,
    double ymin, double ymax, int height,
    int max_iter,
    int* output
) {
    // Parallelize with OpenMP
    #pragma omp parallel for schedule(dynamic) collapse(2)
    for (int py = 0; py < height; py++) {
        for (int px = 0; px < width; px++) {
            double cr = xmin + (xmax - xmin) * px / width;
            double ci = ymin + (ymax - ymin) * py / height;
            output[py * width + px] = mandelbrot_point(cr, ci, max_iter);
        }
    }
}

#ifdef STANDALONE
// Standalone mode: read params from stdin, write binary to stdout
int main(void) {
    double xmin, xmax, ymin, ymax;
    int width, height, max_iter;

    // Read parameters
    if (scanf("%lf %lf %d %lf %lf %d %d",
              &xmin, &xmax, &width, &ymin, &ymax, &height, &max_iter) != 7) {
        fprintf(stderr, "Error: Invalid input parameters\n");
        return 1;
    }

    // Allocate output buffer
    int* output = (int*)malloc(width * height * sizeof(int));
    if (!output) {
        fprintf(stderr, "Error: Cannot allocate memory\n");
        return 1;
    }

    // Compute
    compute_mandelbrot(xmin, xmax, width, ymin, ymax, height, max_iter, output);

    // Write binary data to stdout
    fwrite(output, sizeof(int), width * height, stdout);

    free(output);
    return 0;
}
#endif
