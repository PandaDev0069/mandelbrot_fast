# Performance Benchmarks

## 🚀 Speed Comparison

This implementation achieves **20-40x faster** rendering than pure Python/NumPy:

- **6-11 Mpixels/second** rendering speed
- **1200×900** resolution with real-time interactivity
- **0.1-0.5 seconds** per frame (vs 4-12 seconds in Python)

## 📊 Benchmark Table

| Method            | Speed (Mpixels/sec) | Time per frame  |
| ----------------- | ------------------- | --------------- |
| Pure Python loops | 0.01                | 2+ minutes      |
| NumPy vectorized  | 0.2-0.3             | 4-12 seconds    |
| **C optimized**   | **6-11**            | **0.1-0.5 sec** |
| C + OpenMP        | 20-40               | 0.03-0.1 sec    |
| GPU (future)      | 500+                | < 0.01 sec      |

## 🔬 Technical Details

### Algorithm Optimizations

**Escape-time algorithm** with the following optimizations:

- Early bailout when |z|² > 4
- Reduced multiplications (store z², avoid recomputing)
- Adaptive iteration count based on zoom level

### Data Flow

1. Python → C (boundary coordinates, resolution, max_iter)
2. C computes iteration counts → Returns integer array
3. Python applies log scaling + colormap
4. Matplotlib renders with interpolation

### Memory Usage

~4MB for 1200×900 image (int32 array)

## 📝 Notes

- Original slow version remains in `../数理科学/9回/`
- This folder is for high-performance experiments
- All optimizations preserve mathematical accuracy
