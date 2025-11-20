"""
Interactive Mandelbrot Set Explorer - C-Speed + Matplotlib Beauty
==================================================================

Hybrid implementation combining:
- Ultra-fast C computation (10-100x faster than pure Python)
- Beautiful matplotlib visualization and interactivity
- Infinite zoom capability with adaptive detail

Controls:
- Left Click: Zoom in 3x at the clicked location
- Right Click: Zoom out 3x
- Close window: Exit the program

Setup:
1. Compile C library:
   gcc -O3 -march=native -fopenmp -shared -fPIC mandelbrot_compute.c -o mandelbrot_compute.dll -lm

Author: Chaos Theory Demonstration
Date: 2025
"""

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap
import ctypes
import os
import sys
import time


class FastMandelbrotCompute:
    """C-accelerated Mandelbrot computation engine"""

    def __init__(self):
        # Try to load the C library
        dll_name = 'mandelbrot_compute.dll' if sys.platform == 'win32' else 'mandelbrot_compute.so'
        script_dir = os.path.dirname(os.path.abspath(__file__)) if __file__ else os.getcwd()
        dll_path = os.path.join(script_dir, dll_name)

        try:
            self.lib = ctypes.CDLL(dll_path)
            self.lib.compute_mandelbrot.argtypes = [
                ctypes.c_double, ctypes.c_double, ctypes.c_int,  # xmin, xmax, width
                ctypes.c_double, ctypes.c_double, ctypes.c_int,  # ymin, ymax, height
                ctypes.c_int,                                     # max_iter
                ctypes.POINTER(ctypes.c_int)                      # output array
            ]
            self.lib.compute_mandelbrot.restype = None
            self.use_c = True
            print("✓ C acceleration library loaded successfully!")
        except:
            print("⚠ C library not found, falling back to NumPy (slower)")
            print(f"  Compile with: gcc -O3 -march=native -fopenmp -shared -fPIC mandelbrot_compute.c -o {dll_name} -lm")
            self.use_c = False

    def compute(self, xmin, xmax, width, ymin, ymax, height, max_iter):
        """
        Compute Mandelbrot set using C or fallback to NumPy

        Returns:
            np.ndarray: Integer array of iteration counts
        """
        if self.use_c:
            # Use ultra-fast C computation
            output = np.zeros(height * width, dtype=np.int32)
            self.lib.compute_mandelbrot(
                xmin, xmax, width,
                ymin, ymax, height,
                max_iter,
                output.ctypes.data_as(ctypes.POINTER(ctypes.c_int))
            )
            return output.reshape(height, width)
        else:
            # Fallback to NumPy vectorization
            return self._compute_numpy(xmin, xmax, width, ymin, ymax, height, max_iter)

    def _compute_numpy(self, xmin, xmax, width, ymin, ymax, height, max_iter):
        """NumPy fallback implementation"""
        x = np.linspace(xmin, xmax, width)
        y = np.linspace(ymin, ymax, height)
        X, Y = np.meshgrid(x, y)
        C = X + 1j * Y

        Z = np.zeros_like(C, dtype=complex)
        M = np.full(C.shape, max_iter, dtype=np.int32)

        for i in range(max_iter):
            mask = np.abs(Z) <= 2
            Z[mask] = Z[mask]**2 + C[mask]
            diverged = (np.abs(Z) > 2) & (M == max_iter)
            M[diverged] = i

        return M


class InteractiveMandelbrot:
    """Interactive Mandelbrot visualization with C-speed computation"""

    def __init__(self):
        # Initialize C compute engine
        self.compute_engine = FastMandelbrotCompute()

        # Initial view parameters
        self.xmin, self.xmax = -2.5, 1.0
        self.ymin, self.ymax = -1.25, 1.25
        self.max_iter = 256
        self.width, self.height = 1200, 900  # Higher resolution thanks to C speed!

        # Create figure and axis
        self.fig, self.ax = plt.subplots(figsize=(16, 12), facecolor='black')
        self.ax.set_facecolor('black')

        # Custom colormap for beautiful visualization - expanded color range
        colors = [
            '#000000',  # Black (deepest)
            '#0a0033',  # Deep blue
            '#1a1a66',  # Dark blue
            '#2e3192',  # Royal blue
            '#3d5cb8',  # Medium blue
            '#4a8fd6',  # Sky blue
            '#56c1e8',  # Light cyan
            '#66e9f0',  # Bright cyan
            '#7fffd4',  # Aquamarine
            '#99ff99',  # Light green
            '#ccff66',  # Yellow-green
            '#ffff00',  # Pure yellow
            '#ffcc00',  # Golden yellow
            '#ff9900',  # Orange
            '#ff6600',  # Deep orange
            '#ff3300',  # Red-orange
            '#ff0066',  # Hot pink
            '#cc0099',  # Magenta
            '#8800cc',  # Purple
            '#4400aa',  # Deep purple
            '#220066',  # Very dark purple
            '#000000',  # Back to black
        ]
        self.cmap = LinearSegmentedColormap.from_list('mandelbrot', colors, N=512)

        # Initial plot
        self.im = None
        self.update_plot()

        # Connect click event
        self.fig.canvas.mpl_connect('button_press_event', self.on_click)

        # Instructions
        self.ax.set_title(
            'Interactive Mandelbrot Set - C Speed + Matplotlib Beauty\n'
            'Left Click: Zoom In 3x | Right Click: Zoom Out 3x',
            color='white', fontsize=16, pad=20, weight='bold'
        )

        plt.tight_layout()
        print("\n" + "="*70)
        print("Interactive Mandelbrot Fractal Explorer (C-Accelerated)")
        print("="*70)
        print("Controls:")
        print("  • Left Click  → Zoom In 3x at clicked location")
        print("  • Right Click → Zoom Out 3x")
        print("  • Close Window → Exit")
        print("\nCombining C computation speed with matplotlib visualization!")
        print("="*70 + "\n")
        plt.show()

    def compute_mandelbrot(self):
        """
        Compute the Mandelbrot set using C-accelerated computation

        Returns:
            np.ndarray: Log-scaled iteration counts for smooth visualization
        """
        M = self.compute_engine.compute(
            self.xmin, self.xmax, self.width,
            self.ymin, self.ymax, self.height,
            self.max_iter
        )

        # Logarithmic scaling for smooth color gradients
        return np.log(M + 1)

    def update_plot(self):
        """Recompute and redraw the fractal at current zoom level"""
        zoom_level = 3.5 / (self.xmax - self.xmin)
        print(f"Computing fractal at {self.width}x{self.height}...")
        print(f"  Zoom: {zoom_level:.2f}x | Iterations: {self.max_iter}")

        start_time = time.time()
        mandelbrot_set = self.compute_mandelbrot()
        compute_time = time.time() - start_time

        print(f"✓ Computed in {compute_time:.3f} seconds ({self.width*self.height/compute_time/1e6:.2f} Mpixels/sec)")

        if self.im is None:
            self.im = self.ax.imshow(
                mandelbrot_set,
                extent=[self.xmin, self.xmax, self.ymin, self.ymax],
                cmap=self.cmap,
                interpolation='bilinear',
                origin='lower'
            )
            self.ax.set_aspect('equal')
        else:
            self.im.set_data(mandelbrot_set)
            self.im.set_extent([self.xmin, self.xmax, self.ymin, self.ymax])

        self.ax.set_xlim(self.xmin, self.xmax)
        self.ax.set_ylim(self.ymin, self.ymax)
        self.fig.canvas.draw_idle()
        print()

    def on_click(self, event):
        """Handle mouse clicks for zooming"""
        if event.inaxes != self.ax:
            return

        cx, cy = event.xdata, event.ydata
        if cx is None or cy is None:
            return

        if event.button == 1:  # Left click - zoom in
            zoom_factor = 3.0
            print(f"→ Zooming IN at ({cx:.10f}, {cy:.10f})")
        elif event.button == 3:  # Right click - zoom out
            zoom_factor = 1.0 / 3.0
            print(f"← Zooming OUT from ({cx:.10f}, {cy:.10f})")
        else:
            return

        # Calculate new bounds
        width = (self.xmax - self.xmin) / zoom_factor
        height = (self.ymax - self.ymin) / zoom_factor

        self.xmin = cx - width / 2
        self.xmax = cx + width / 2
        self.ymin = cy - height / 2
        self.ymax = cy + height / 2

        # Adaptive iterations for detail at deep zooms
        zoom_level = 3.5 / (self.xmax - self.xmin)
        self.max_iter = min(1024, int(256 * (1 + np.log10(max(1, zoom_level)))))

        self.update_plot()


def main():
    """Main entry point"""
    try:
        explorer = InteractiveMandelbrot()
    except KeyboardInterrupt:
        print("\n\nExploration interrupted by user.")
    except Exception as e:
        print(f"\n\nError: {e}")
        import traceback
        traceback.print_exc()
    finally:
        print("Thank you for exploring the Mandelbrot set!")


if __name__ == "__main__":
    main()
