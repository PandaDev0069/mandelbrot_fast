# Technical Documentation

## 🎨 Color Palette Customization

### Current Scheme

Dark blue → Cyan → Yellow → Orange → Red → Black

### Modifying Colors in Python

Edit the gradient in `interactive_mandelbrot_fast.py`:

```python
colors = ['#000033', '#000055', '#0E4C92', '#2E8BC0', '#19D3F3',
          '#FFF000', '#FF6B35', '#C1292E', '#5C0029', '#000000']
```

### Modifying Colors in C

For palette-based coloring directly in C (around line 65 in
`mandelbrot_compute.c`):

```c
palette[i].r = ...;
palette[i].g = ...;
palette[i].b = ...;
```

## 🔧 Compilation Instructions

### Windows

```bash
gcc -O3 -march=native -shared mandelbrot_compute.c -o mandelbrot_compute.dll -lm
```

### Linux/macOS

```bash
gcc -O3 -march=native -shared -fPIC mandelbrot_compute.c -o mandelbrot_compute.so -lm
```

### Optimization Flags Explained

- `-O3` - Maximum optimization level
- `-march=native` - Use CPU-specific instructions (AVX, SSE, etc.)
- `-shared` - Create shared library (DLL/SO)
- `-fPIC` - Position-independent code (Linux/macOS)
- `-lm` - Link math library

## 🏗️ Architecture

### File Structure

- `mandelbrot_compute.c` - C computation engine
- `mandelbrot_compute.dll` - Compiled shared library
- `interactive_mandelbrot_fast.py` - Python visualization frontend

### Data Types

- **Input**: double precision (float64) coordinates
- **Computation**: double precision complex arithmetic
- **Output**: 32-bit integer iteration counts
- **Rendering**: 8-bit RGB after Python colormap application

### Memory Layout

For a 1200×900 image:

- Raw iteration data: ~4MB (int32 array)
- RGB output: ~3MB (uint8 array, 3 channels)
- Total working memory: ~10MB (with buffers)

## 🔌 Python-C Interface

### ctypes Binding

The Python code uses `ctypes` to call the C library:

```python
lib = ctypes.CDLL('./mandelbrot_compute.dll')
lib.mandelbrot_compute.argtypes = [
    ctypes.c_int,    # width
    ctypes.c_int,    # height
    ctypes.c_double, # x_min
    ctypes.c_double, # x_max
    ctypes.c_double, # y_min
    ctypes.c_double, # y_max
    ctypes.c_int,    # max_iter
    ndpointer(dtype=np.int32, ndim=2, flags='C_CONTIGUOUS')
]
```

### Function Signature

C function declaration:

```c
void mandelbrot_compute(
    int width, int height,
    double x_min, double x_max,
    double y_min, double y_max,
    int max_iter,
    int* output
);
```

## 🧮 Algorithm Details

### Escape Time Algorithm

For each pixel (x, y):

1. Map to complex coordinate c = x + yi
2. Initialize z = 0
3. Iterate: z = z² + c
4. Count iterations until |z| > 2 (or max_iter reached)
5. Return iteration count

### Optimizations Applied

1. **Early bailout**: Check |z|² > 4 instead of |z| > 2 (avoids sqrt)
2. **Reduced operations**: Store zr² and zi² to avoid recomputation
3. **Adaptive max_iter**: Increase with zoom level for detail
4. **Memory efficiency**: Single output array, no intermediate buffers

### Numerical Stability

- Uses double precision (15-17 decimal digits)
- Stable for zoom levels up to ~10^14
- For deeper zooms, perturbation theory or arbitrary precision needed
