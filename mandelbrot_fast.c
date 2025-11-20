/*
 * Ultra-Fast Mandelbrot Set Generator - Pure C, No Dependencies
 * ==============================================================
 *
 * High-performance Mandelbrot fractal renderer that outputs to BMP files.
 * No external libraries required - pure C with standard library only.
 *
 * Compilation:
 *   gcc -O3 -march=native -fopenmp mandelbrot_fast.c -o mandelbrot_fast -lm
 *
 * Or without OpenMP:
 *   gcc -O3 -march=native mandelbrot_fast.c -o mandelbrot_fast -lm
 *
 * Usage:
 *   mandelbrot_fast.exe                    (generates default view)
 *   mandelbrot_fast.exe 0.0 0.0 1.0        (center_x, center_y, zoom)
 *   mandelbrot_fast.exe -0.5 0.0 1.0 256   (with custom iterations)
 *
 * Author: Chaos Theory Demonstration
 * Date: 2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

// Image dimensions
#define WIDTH  1920
#define HEIGHT 1080
#define MAX_ITERATIONS_DEFAULT 256

// BMP file header structures
#pragma pack(push, 1)
typedef struct {
    unsigned short type;
    unsigned int size;
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned int offset;
} BMPHeader;

typedef struct {
    unsigned int size;
    int width;
    int height;
    unsigned short planes;
    unsigned short bits;
    unsigned int compression;
    unsigned int imagesize;
    int xresolution;
    int yresolution;
    unsigned int ncolors;
    unsigned int importantcolors;
} BMPInfoHeader;
#pragma pack(pop)

// Color structure
typedef struct {
    unsigned char r, g, b;
} Color;

// Color palette
Color palette[256];

// Initialize beautiful color palette (matches Python version)
void init_palette(void) {
    for (int i = 0; i < 256; i++) {
        double t = i / 255.0;

        if (t < 0.16) {
            double s = t / 0.16;
            palette[i].r = (unsigned char)(0 + s * 14);
            palette[i].g = (unsigned char)(0 + s * 76);
            palette[i].b = (unsigned char)(51 + s * 95);
        } else if (t < 0.42) {
            double s = (t - 0.16) / 0.26;
            palette[i].r = (unsigned char)(14 + s * 32);
            palette[i].g = (unsigned char)(76 + s * 67);
            palette[i].b = (unsigned char)(146 + s * 65);
        } else if (t < 0.6425) {
            double s = (t - 0.42) / 0.2225;
            palette[i].r = (unsigned char)(46 + s * 179);
            palette[i].g = (unsigned char)(143 + s * 68);
            palette[i].b = (unsigned char)(211 + s * 32);
        } else if (t < 0.78) {
            double s = (t - 0.6425) / 0.1375;
            palette[i].r = (unsigned char)(225 + s * 30);
            palette[i].g = (unsigned char)(211 + s * 44);
            palette[i].b = (unsigned char)(243 - s * 243);
        } else if (t < 0.89) {
            double s = (t - 0.78) / 0.11;
            palette[i].r = (unsigned char)(255 - s * 62);
            palette[i].g = (unsigned char)(255 - s * 148);
            palette[i].b = (unsigned char)(0 + s * 53);
        } else {
            double s = (t - 0.89) / 0.11;
            palette[i].r = (unsigned char)(193 - s * 98);
            palette[i].g = (unsigned char)(107 - s * 65);
            palette[i].b = (unsigned char)(53 - s * 12);
        }
    }
}

// Compute Mandelbrot iteration count for a single point
// Optimized with early bailout and reduced operations
static inline int mandelbrot_point(double cr, double ci, int max_iter) {
    double zr = 0.0, zi = 0.0;
    double zr2 = 0.0, zi2 = 0.0;

    for (int i = 0; i < max_iter; i++) {
        // Check for divergence
        if (zr2 + zi2 > 4.0) {
            return i;
        }

        // Compute next iteration: z = z^2 + c
        zi = 2.0 * zr * zi + ci;
        zr = zr2 - zi2 + cr;
        zr2 = zr * zr;
        zi2 = zi * zi;
    }
    return max_iter;
}

// Get color for iteration count with smooth gradient
Color get_color(int iterations, int max_iter) {
    if (iterations == max_iter) {
        Color c = {0, 0, 0};
        return c;
    }

    // Logarithmic scaling for smooth colors
    double log_iter = log((double)(iterations + 1)) / log((double)(max_iter + 1));
    int idx = (int)(log_iter * 255.0);
    if (idx < 0) idx = 0;
    if (idx > 255) idx = 255;

    return palette[idx];
}

// Save image as BMP file
int save_bmp(const char* filename, unsigned char* data, int width, int height) {
    FILE* f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "Error: Cannot create file %s\n", filename);
        return 0;
    }

    // Calculate padding
    int padding = (4 - (width * 3) % 4) % 4;
    int imagesize = (width * 3 + padding) * height;

    // BMP Header
    BMPHeader header;
    header.type = 0x4D42; // 'BM'
    header.size = 54 + imagesize;
    header.reserved1 = 0;
    header.reserved2 = 0;
    header.offset = 54;

    // BMP Info Header
    BMPInfoHeader info;
    info.size = 40;
    info.width = width;
    info.height = height;
    info.planes = 1;
    info.bits = 24;
    info.compression = 0;
    info.imagesize = imagesize;
    info.xresolution = 2835;
    info.yresolution = 2835;
    info.ncolors = 0;
    info.importantcolors = 0;

    // Write headers
    fwrite(&header, sizeof(BMPHeader), 1, f);
    fwrite(&info, sizeof(BMPInfoHeader), 1, f);

    // Write pixel data (BMP is bottom-up)
    unsigned char pad[3] = {0, 0, 0};
    for (int y = height - 1; y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 3;
            // BMP uses BGR format
            fputc(data[idx + 2], f); // B
            fputc(data[idx + 1], f); // G
            fputc(data[idx + 0], f); // R
        }
        fwrite(pad, 1, padding, f);
    }

    fclose(f);
    return 1;
}

// Render Mandelbrot set
void render_mandelbrot(unsigned char* image,
                      double center_x, double center_y, double zoom,
                      int max_iterations) {

    double x_min = center_x - 2.0 / zoom;
    double x_max = center_x + 2.0 / zoom;
    double y_min = center_y - 1.5 / zoom;
    double y_max = center_y + 1.5 / zoom;

    printf("Rendering Mandelbrot set...\n");
    printf("  Resolution: %dx%d\n", WIDTH, HEIGHT);
    printf("  Center: (%.10f, %.10f)\n", center_x, center_y);
    printf("  Zoom: %.2fx\n", zoom);
    printf("  Iterations: %d\n", max_iterations);

    clock_t start = clock();

    // Parallelize with OpenMP if available
    #pragma omp parallel for schedule(dynamic)
    for (int py = 0; py < HEIGHT; py++) {
        for (int px = 0; px < WIDTH; px++) {
            // Map pixel to complex plane
            double cr = x_min + (x_max - x_min) * px / WIDTH;
            double ci = y_min + (y_max - y_min) * py / HEIGHT;

            // Calculate iterations
            int iter = mandelbrot_point(cr, ci, max_iterations);

            // Get color
            Color c = get_color(iter, max_iterations);

            // Store in image buffer (RGB format)
            int idx = (py * WIDTH + px) * 3;
            image[idx + 0] = c.r;
            image[idx + 1] = c.g;
            image[idx + 2] = c.b;
        }

        // Progress indicator (only from master thread)
        #pragma omp master
        {
            if ((py + 1) % (HEIGHT / 20) == 0) {
                printf("  Progress: %d%%\r", (py + 1) * 100 / HEIGHT);
                fflush(stdout);
            }
        }
    }

    clock_t end = clock();
    double cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;

    printf("\n✓ Rendering complete in %.2f seconds!\n", cpu_time);
    printf("  Performance: %.2f megapixels/second\n\n",
           (WIDTH * HEIGHT / 1000000.0) / cpu_time);
}

int main(int argc, char* argv[]) {
    printf("\n%s\n", "======================================================================");
    printf("Ultra-Fast Mandelbrot Set Generator (Pure C Implementation)\n");
    printf("%s\n", "======================================================================");

    // Parse command line arguments
    double center_x = -0.5;
    double center_y = 0.0;
    double zoom = 1.0;
    int max_iterations = MAX_ITERATIONS_DEFAULT;

    if (argc >= 4) {
        center_x = atof(argv[1]);
        center_y = atof(argv[2]);
        zoom = atof(argv[3]);
    }
    if (argc >= 5) {
        max_iterations = atoi(argv[4]);
    }

    // Auto-adjust iterations based on zoom
    if (argc < 5 && zoom > 1.0) {
        max_iterations = (int)(256 * (1 + log10(zoom)));
        if (max_iterations > 2048) max_iterations = 2048;
    }

    printf("\n");

    // Initialize color palette
    init_palette();

    // Allocate image buffer
    unsigned char* image = (unsigned char*)malloc(WIDTH * HEIGHT * 3);
    if (!image) {
        fprintf(stderr, "Error: Cannot allocate memory for image\n");
        return 1;
    }

    // Render
    render_mandelbrot(image, center_x, center_y, zoom, max_iterations);

    // Generate filename
    char filename[256];
    snprintf(filename, sizeof(filename),
             "mandelbrot_x%.3f_y%.3f_z%.0f.bmp",
             center_x, center_y, zoom);

    // Save to file
    printf("Saving to %s...\n", filename);
    if (save_bmp(filename, image, WIDTH, HEIGHT)) {
        printf("✓ Image saved successfully!\n");
    } else {
        fprintf(stderr, "✗ Failed to save image\n");
        free(image);
        return 1;
    }

    // Cleanup
    free(image);

    printf("\n%s\n", "======================================================================");
    printf("Usage examples:\n");
    printf("  %s                           (default view)\n", argv[0]);
    printf("  %s -0.5 0.0 10.0             (zoom 10x at center)\n", argv[0]);
    printf("  %s -0.75 0.1 100.0 512       (zoom 100x, 512 iterations)\n", argv[0]);
    printf("  %s 0.3 0.5 1000.0            (deep zoom)\n", argv[0]);
    printf("%s\n", "======================================================================");

    return 0;
}
