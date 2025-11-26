#!/bin/bash
# Build script for Mandelbrot computation engine (Linux/Mac)

echo "Building Mandelbrot Computation Engine..."

# Ensure lib directory exists
mkdir -p lib

# Compile with optimizations
gcc -shared -o lib/mandelbrot_compute.so src/mandelbrot_compute.c \
    -O3 -fopenmp -march=native -mavx2 -mfma -lquadmath -fPIC

if [ $? -eq 0 ]; then
    echo "✓ Build successful!"
    echo "  Output: lib/mandelbrot_compute.so"
    ls -lh lib/mandelbrot_compute.so
else
    echo "✗ Build failed!"
    exit 1
fi
