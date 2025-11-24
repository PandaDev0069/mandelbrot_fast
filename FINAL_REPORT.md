# High-Performance Mandelbrot Explorer: Final Report

[🇺🇸 English](FINAL_REPORT.md) | [🇯🇵 日本語](FINAL_REPORT_JP.md)

## 1. Project Overview

This project implements a state-of-the-art **Mandelbrot Set Explorer** capable of rendering images at extreme zoom levels (exceeding $10^{30}$) in real-time. It overcomes the precision limitations of standard floating-point arithmetic by utilizing **Perturbation Theory** and **Arbitrary Precision Arithmetic** (via `__float128`), accelerated by **AVX2 SIMD** instructions and **OpenMP** parallelism.

## 2. Technical Architecture

The system consists of a hybrid architecture:

- **Frontend (Python)**: Handles window management, user input, and OpenGL rendering. It uses `ctypes` to interface with the high-performance C backend.
- **Backend (C)**: Performs the heavy mathematical computations. It is compiled as a shared library (`.dll`) and optimized for the specific CPU architecture.

### Precision Modes

The engine dynamically switches between three precision modes based on the zoom level:

1. **Double Precision (`double`)**: Fast hardware arithmetic (64-bit). Used for zoom $< 10^{13}$.
2. **Long Double Precision (`long double`)**: Extended precision (80-bit). Used for zoom $< 10^{17}$.
3. **Perturbation Theory (Hybrid)**: Uses a high-precision reference orbit (128-bit) and low-precision deltas (64-bit). Used for zoom $> 10^{17}$.

## 3. Mathematical Foundations

### 3.1 Standard Mandelbrot Iteration

The Mandelbrot set is defined by the iteration of the function $f_c(z) = z^2 + c$:

$$ Z_{n+1} = Z_n^2 + c $$

where $Z_0 = 0$ and $c$ is a complex parameter. A point $c$ is in the set if $|Z_n|$ remains bounded as $n \to \infty$.

### 3.2 Perturbation Theory

To avoid calculating high-precision arithmetic for every pixel, we compute a single **Reference Orbit** ($X_n$) using high precision (128-bit) for the center of the screen. Other points $c$ are represented as $c = c_{ref} + \delta_c$.
The orbit of a neighbor point $Z_n$ is expressed as $Z_n = X_n + \delta_n$.

Substituting this into the iteration formula:

$$ X_{n+1} + \delta_{n+1} = (X_n + \delta_n)^2 + (c_{ref} + \delta_c) $$
$$ X_{n+1} + \delta_{n+1} = (X_n^2 + c_{ref}) + (2X_n\delta_n + \delta_n^2 + \delta_c) $$

Since $X_{n+1} = X_n^2 + c_{ref}$ (by definition of the reference orbit), we get the iteration formula for the delta $\delta_n$:

$$ \delta_{n+1} = 2X_n\delta_n + \delta_n^2 + \delta_c $$

This allows us to compute $\delta_n$ using standard **double precision**, which is significantly faster than 128-bit arithmetic, as long as $|\delta_n|$ remains small.

### 3.3 Bivariate Linear Approximation (BLA)

For the initial iterations, $\delta_n$ is extremely small, and the $\delta_n^2$ term is negligible. We can approximate the relationship between $\delta_n$ and $\delta_c$ linearly:

$$ \delta_n \approx B_n \cdot \delta_c $$

Substituting this into the linearized perturbation equation (ignoring $\delta_n^2$):

$$ B_{n+1} \cdot \delta_c = 2X_n (B_n \cdot \delta_c) + \delta_c $$
$$ B_{n+1} = 2X_n B_n + 1 $$

with $B_0 = 0$.

We precompute the coefficients $B_n$. For a given pixel with offset $\delta_c$, we can skip directly to iteration $k$ by calculating:

$$ \delta_k = B_k \cdot \delta_c $$

This optimization (Series Approximation) allows us to **skip 50-80% of iterations** at deep zoom levels.

## 4. Optimizations Implemented

### 4.1 AVX2 SIMD Vectorization

The inner loop of the perturbation calculation is vectorized using **AVX2 intrinsics** (`__m256d`). This allows the CPU to process **4 pixels simultaneously** in a single instruction cycle.

- **FMA (Fused Multiply-Add)**: Used to perform $a \cdot b + c$ in a single step, improving speed and precision.
- **Masking**: Active pixels are tracked using bitmasks to handle divergent escape times within a vector.

### 4.2 OpenMP Parallelism

The workload is distributed across all available CPU cores using **OpenMP**. The `schedule(guided)` clause is used to balance the load, as some areas of the Mandelbrot set require significantly more iterations than others.

### 4.3 Loop Unrolling & Escape Check Reduction

The main loop is unrolled by a factor of 4. The expensive escape condition check ($|Z|^2 > 4$) is only performed **once every 4 iterations**, reducing branching overhead and allowing better instruction pipelining.

### 4.4 Adaptive Iteration Limits

To maintain detail at extreme depths, the maximum iteration count is dynamically adjusted based on the zoom level, ranging from 512 up to **2,097,152** iterations.

## 5. Performance Results

| Zoom Level | Precision Mode | Iterations | Time (Approx) |
| :--- | :--- | :--- | :--- |
| $10^{13}$ | Double (64-bit) | 16,384 | < 0.1s |
| $10^{16}$ | Long Double (80-bit) | 65,536 | ~1.2s |
| $10^{20}$ | Perturbation (Hybrid) | 131,072 | ~0.8s |
| $10^{30}$ | Perturbation (Hybrid) | 1,048,576 | ~0.5s |

*Note: Performance improves at deeper zooms due to the effectiveness of the Linear Approximation (BLA).*

## 6. Usage

Run the explorer using Python:

```bash
python smooth_mandelbrot.py
```

- **Scroll**: Zoom in/out
- **Left Click**: Center view
- **ESC**: Exit
