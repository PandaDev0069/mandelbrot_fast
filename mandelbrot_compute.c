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

// Compute Mandelbrot iteration count for a single point - Optimized
static inline int mandelbrot_point(double cr, double ci, int max_iter) {
    double zr = 0.0, zi = 0.0;
    double zr2 = 0.0, zi2 = 0.0;

    // Main cardioid and period-2 bulb checking for instant bailout
    double q = (cr - 0.25) * (cr - 0.25) + ci * ci;
    if (q * (q + (cr - 0.25)) < 0.25 * ci * ci) {
        return max_iter; // Inside main cardioid
    }
    if ((cr + 1.0) * (cr + 1.0) + ci * ci < 0.0625) {
        return max_iter; // Inside period-2 bulb
    }

    // Unrolled loop for better performance (process 4 iterations at once)
    int i = 0;
    for (; i < max_iter - 3; i += 4) {
        // Iteration 1
        if (zr2 + zi2 > 4.0) return i;
        zi = 2.0 * zr * zi + ci;
        zr = zr2 - zi2 + cr;
        zr2 = zr * zr;
        zi2 = zi * zi;

        // Iteration 2
        if (zr2 + zi2 > 4.0) return i + 1;
        zi = 2.0 * zr * zi + ci;
        zr = zr2 - zi2 + cr;
        zr2 = zr * zr;
        zi2 = zi * zi;

        // Iteration 3
        if (zr2 + zi2 > 4.0) return i + 2;
        zi = 2.0 * zr * zi + ci;
        zr = zr2 - zi2 + cr;
        zr2 = zr * zr;
        zi2 = zi * zi;

        // Iteration 4
        if (zr2 + zi2 > 4.0) return i + 3;
        zi = 2.0 * zr * zi + ci;
        zr = zr2 - zi2 + cr;
        zr2 = zr * zr;
        zi2 = zi * zi;
    }

    // Handle remaining iterations
    for (; i < max_iter; i++) {
        if (zr2 + zi2 > 4.0) return i;
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
    // Pre-calculate step sizes for better performance
    double dx = (xmax - xmin) / width;
    double dy = (ymax - ymin) / height;

    // Parallelize with OpenMP - use static scheduling for better cache performance
    #pragma omp parallel for schedule(static) collapse(2)
    for (int py = 0; py < height; py++) {
        for (int px = 0; px < width; px++) {
            double cr = xmin + dx * px;
            double ci = ymin + dy * py;
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
