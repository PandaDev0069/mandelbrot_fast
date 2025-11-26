# High-Performance Mandelbrot Explorer

![Mandelbrot](https://img.shields.io/badge/Render-OpenGL-green)
![Precision](https://img.shields.io/badge/Precision-128--bit-blue)
![Acceleration](https://img.shields.io/badge/Acceleration-AVX2%20%2B%20OpenMP-orange)

[🇺🇸 English](README.md) | [🇯🇵 日本語](README_JP.md)

A state-of-the-art Mandelbrot Set explorer capable of real-time rendering at
extreme zoom levels ($> 10^{30}$). This project leverages **Perturbation
Theory**, **Series Approximation**, and **AVX2 SIMD** instructions to overcome
the limitations of standard floating-point arithmetic.

## 🚀 Features

- **Extreme Zoom**: Explore depths exceeding $10^{30}$ with perfect precision.
- **Hybrid Precision Engine**: Automatically switches between:
  - `double` (64-bit) for speed at shallow zooms.
  - `long double` (80-bit) for intermediate precision.
  - **Perturbation Theory** (128-bit reference + 64-bit delta) for deep zooms.
- **High Performance**:
  - **AVX2 Vectorization**: Processes 4 pixels per cycle.
  - **OpenMP Parallelism**: Multi-threaded rendering across all CPU cores.
  - **Series Approximation (BLA)**: Skips up to 80% of iterations in deep zooms.
- **Smooth Visualization**:
  - OpenGL-based rendering.
  - Continuous smooth coloring with dynamic histogram normalization.
  - Palette matching standard high-quality renderers.

## 🛠️ Installation & Usage

### Prerequisites

- Python 3.x
- Required Python packages: `glfw`, `PyOpenGL`, `numpy`, `matplotlib`

```bash
pip install glfw PyOpenGL numpy matplotlib

```

- Windows (Pre-compiled DLL included) or GCC for compilation.

### Running the Explorer

Simply run the Python script:

```bash
python smooth_mandelbrot.py
```

### Controls

- **Scroll Wheel**: Zoom in / out at cursor position.
- **Left Click**: Center the view at the cursor.
- **ESC**: Exit the application.

## 🔧 Technical Details

For a deep dive into the mathematics and optimization techniques used (including
Perturbation Theory and Bivariate Linear Approximation), please refer to the
[Final Report](FINAL_REPORT.md) ([🇯🇵 Japanese Version](FINAL_REPORT_JP.md)).

### Compilation (Optional)

If you need to recompile the C backend:

```bash
gcc -shared -o mandelbrot_compute.dll mandelbrot_compute.c -O3 -march=native -fopenmp -lquadmath
```

## 📄 License

This project is open source. Feel free to explore and modify.
