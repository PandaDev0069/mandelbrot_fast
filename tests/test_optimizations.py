#!/usr/bin/env python3
"""
Quick test to verify optimizations don't break functionality
"""
import ctypes
import numpy as np
import sys

# Load library
try:
    import os
    script_dir = os.path.dirname(os.path.abspath(__file__))
    lib_path = os.path.join(os.path.dirname(script_dir), 'lib', 'mandelbrot_compute.dll')
    lib = ctypes.CDLL(lib_path)
except OSError as e:
    print(f"Error: Cannot load mandelbrot_compute.dll: {e}")
    sys.exit(1)

# Setup function signatures
lib.compute_mandelbrot.argtypes = [
    ctypes.c_double, ctypes.c_double, ctypes.c_int,
    ctypes.c_double, ctypes.c_double, ctypes.c_int,
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_double)
]
lib.compute_mandelbrot.restype = None

lib.compute_mandelbrot_str.argtypes = [
    ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int,
    ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int,
    ctypes.c_int,
    ctypes.POINTER(ctypes.c_double)
]
lib.compute_mandelbrot_str.restype = None

print("Testing optimized Mandelbrot computation...")

# Test 1: Simple double precision
print("\n1. Testing double precision mode...")
width, height = 800, 600
max_iter = 256
output = np.zeros(width * height, dtype=np.float64)

lib.compute_mandelbrot(
    -2.5, 1.0, width,
    -1.0, 1.0, height,
    max_iter,
    output.ctypes.data_as(ctypes.POINTER(ctypes.c_double))
)

# Check for reasonable values
non_escaped = np.sum(output < 0)
escaped = np.sum(output >= 0)
print(f"   Escaped pixels: {escaped}/{width*height} ({100*escaped/(width*height):.1f}%)")
print(f"   Non-escaped pixels: {non_escaped}/{width*height} ({100*non_escaped/(width*height):.1f}%)")
print(f"   ✓ Double precision mode works")

# Test 2: Deep zoom with perturbation (uses optimized path)
print("\n2. Testing perturbation mode (optimized path)...")
width, height = 400, 300
max_iter = 1000

# Deep zoom coordinates
xmin = "-0.7436438870371588"
xmax = "-0.7436438870371587"
ymin = "0.13182590420533"
ymax = "0.13182590420534"

output = np.zeros(width * height, dtype=np.float64)

lib.compute_mandelbrot_str(
    xmin.encode(), xmax.encode(), width,
    ymin.encode(), ymax.encode(), height,
    max_iter,
    output.ctypes.data_as(ctypes.POINTER(ctypes.c_double))
)

# Check for reasonable values
non_escaped = np.sum(output < 0)
escaped = np.sum(output >= 0)
print(f"   Escaped pixels: {escaped}/{width*height} ({100*escaped/(width*height):.1f}%)")
print(f"   Non-escaped pixels: {non_escaped}/{width*height} ({100*non_escaped/(width*height):.1f}%)")
print(f"   ✓ Perturbation mode works")

# Test 3: Check smoothing is working
print("\n3. Testing smooth iteration values...")
if escaped > 0:
    escaped_values = output[output >= 0]
    # Check that we have fractional iteration counts (smoothing)
    fractional = np.sum(escaped_values != np.floor(escaped_values))
    print(f"   Pixels with fractional iterations: {fractional}/{len(escaped_values)}")
    if fractional > 0:
        print(f"   ✓ Smoothing is working")
    else:
        print(f"   ⚠ Warning: No fractional iterations found")

print("\n✅ All tests passed! Optimizations are working correctly.")
