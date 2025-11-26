# Project Overview

## High-Performance Mandelbrot Explorer

A production-ready, optimized Mandelbrot set renderer capable of extreme zoom levels (>10³⁰) with real-time performance.

## Quick Start

```bash
# 1. Install dependencies
pip install glfw PyOpenGL numpy matplotlib

# 2. Build the computation engine
./build.ps1        # Windows
./build.sh         # Linux/Mac

# 3. Run the explorer
python src/smooth_mandelbrot.py

# 4. Run tests
python tests/test_optimizations.py
python tests/benchmark_optimizations.py
```

## Architecture

### Computation Engine (`src/mandelbrot_compute.c`)

The core computation is written in C with:

- **Adaptive Precision**: Auto-selects between double (64-bit), long double (80-bit), or perturbation theory (128-bit reference + 64-bit delta)
- **SIMD Vectorization**: AVX2 instructions process 4 pixels simultaneously
- **Multi-threading**: OpenMP parallelizes across all CPU cores
- **Series Approximation**: Bivariate Linear Approximation (BLA) skips iterations in deep zooms
- **Recent Optimizations**: Pre-cast arrays, hoisted constants, aligned memory (~1.5-2× speedup)

### Visualization (`src/smooth_mandelbrot.py`)

The front-end uses:

- **OpenGL**: Hardware-accelerated texture rendering
- **GLSL Shaders**: Real-time smooth coloring and normalization
- **Decimal Precision**: Python's Decimal type for exact center coordinates
- **Smooth Zoom**: Interpolated texture rendering while computing

## Performance Characteristics

| Zoom Level | Precision Mode | Iterations | Time (800×600) |
|------------|----------------|------------|----------------|
| 1-10       | Double         | 512        | ~0.02s         |
| 10³-10⁶    | Long Double    | 2048       | ~0.05s         |
| 10⁶-10³⁰   | Perturbation   | 5000+      | ~0.05-0.4s     |

**Throughput**: 2.5-32 Giga-iterations/second

## Key Features

### Precision
- Supports zooms beyond 10³⁰ using perturbation theory
- 128-bit quad precision reference orbit
- 64-bit double precision per-pixel deltas

### Performance
- AVX2 SIMD: 4-way parallel pixel computation
- OpenMP: Multi-core parallelization
- Series Approximation: Skips ~80% of iterations at deep zoom
- Optimized memory layout: Aligned allocations, pre-cast arrays

### Visualization
- Smooth continuous coloring (no banding)
- Dynamic histogram normalization
- High-quality color palette (49 carefully chosen colors)
- Real-time OpenGL rendering at 60 FPS

## Mathematical Techniques

### Perturbation Theory

Instead of computing:
```
Z_{n+1} = Z_n² + C
```

We compute a reference orbit once:
```
X_{n+1} = X_n² + C₀     (high precision)
```

Then per-pixel deltas:
```
δ_{n+1} = 2X_n·δ_n + δ_n² + δC     (double precision)
```

This allows deep zooms with double precision performance.

### Series Approximation (BLA)

For the first N iterations where δ is small:
```
δ_n ≈ B_n · δC
```

Where B_n satisfies:
```
B_{n+1} = 2X_n·B_n + 1
```

We precompute B_n and skip direct iteration, providing 5-10× speedup in deep zooms.

## File Organization

```
mandelbrot_fast/
├── src/                          # Source code
│   ├── smooth_mandelbrot.py      # Main application (OpenGL + Python)
│   └── mandelbrot_compute.c      # Computation engine (C + AVX2)
├── lib/                          # Compiled binaries (generated)
│   └── mandelbrot_compute.dll    # Windows DLL / .so on Linux
├── tests/                        # Testing and benchmarking
│   ├── test_optimizations.py     # Correctness verification
│   └── benchmark_optimizations.py # Performance measurement
├── docs/                         # Documentation
│   ├── FINAL_REPORT.md           # Detailed technical report
│   ├── FINAL_REPORT_JP.md        # Japanese version
│   ├── OPTIMIZATIONS.md          # Optimization guide
│   └── README_JP.md              # Japanese README
├── build.ps1                     # Windows build script
├── build.sh                      # Linux/Mac build script
└── README.md                     # Main documentation
```

## Dependencies

### Runtime
- Python 3.7+
- NumPy
- PyOpenGL
- GLFW
- Matplotlib

### Build Time
- GCC 7+ (with quadmath support)
- OpenMP
- AVX2-capable CPU (Intel Haswell+, AMD Excavator+)

## Compilation Options

The C engine is compiled with aggressive optimizations:

```bash
gcc -shared -o lib/mandelbrot_compute.dll src/mandelbrot_compute.c \
    -O3                # Maximum optimization
    -fopenmp           # Multi-threading
    -march=native      # CPU-specific instructions
    -mavx2             # Enable AVX2 SIMD
    -mfma              # Fused multiply-add
    -lquadmath         # 128-bit float support
```

## Testing

### Correctness Test
```bash
python tests/test_optimizations.py
```

Verifies:
- Double precision mode
- Perturbation mode
- Smooth iteration values
- Memory safety

### Performance Benchmark
```bash
python tests/benchmark_optimizations.py
```

Measures:
- Render time vs zoom level
- Throughput (Megapixels/sec, Giga-iterations/sec)
- Optimization effectiveness

## Controls

When running the explorer:

- **Scroll Wheel**: Zoom in/out at cursor position
- **Left Click**: Center view at cursor
- **ESC**: Exit

## Future Optimizations

Potential 2-5× additional speedup:

1. **Medium Effort**:
   - Vectorize remainder pixels with masked AVX2 ops
   - Adaptive escape check frequency
   - Prefetch upcoming reference values

2. **High Effort**:
   - AVX-512 port (2× wider vectors)
   - GPU/CUDA offload (10-50× potential)
   - Tile-based cache-aware processing

See [OPTIMIZATIONS.md](OPTIMIZATIONS.md) for detailed analysis.

## Authors

- Aashish Panta
- With assistance from GitHub Copilot

## License

Open source - free to use and modify.
