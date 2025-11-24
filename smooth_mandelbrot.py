"""
Smooth Mandelbrot Explorer - High Quality Edition
==================================================

Combines:
- Ultra-fast C computation (OpenMP multi-threaded)
- OpenGL texture rendering with smooth coordinate transformation
- Exact color palette matching the matplotlib version
- Dynamic histogram normalization for perfect contrast
- Infinite smooth zooming

Controls:
- Mouse Scroll: Zoom in/out at cursor position
- Left Click: Center view at cursor
- ESC: Exit

Author: GitHub Copilot (based on work by Aashish Panta)
Date: 2025/11/24
"""

import glfw
from OpenGL.GL import *
from OpenGL.GL.shaders import compileProgram, compileShader
import numpy as np
import ctypes
import os
import sys
import time
from threading import Thread, Lock
import matplotlib.pyplot as plt
from matplotlib.colors import LinearSegmentedColormap
from decimal import Decimal, getcontext

# Set high precision for Decimal
getcontext().prec = 50

# Window dimensions - High Resolution
WIDTH, HEIGHT = 1920, 1440

# Shader sources
VERTEX_SHADER = """
#version 330 core
layout (location = 0) in vec2 position;
out vec2 TexCoord;

uniform vec2 relative_offset;
uniform vec2 relative_scale;

void main() {
    vec2 view_center_uv = vec2(0.5) + relative_offset;
    TexCoord = view_center_uv + (position * 0.5) * relative_scale;
    gl_Position = vec4(position, 0.0, 1.0);
}
"""

FRAGMENT_SHADER = """
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D mandelbrotTexture;
uniform sampler2D paletteTexture; // Changed to 2D for better compatibility
uniform float min_val;
uniform float max_val;

void main() {
    float iter = texture(mandelbrotTexture, TexCoord).r;
    
    if (iter < 0.0) {
        // Inside set - black
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        // Log-log smoothing
        float val = log(log(iter + 2.0) + 1.0);
        
        // Dynamic normalization based on current view stats
        float t = (val - min_val) / (max_val - min_val);
        
        // Gamma correction
        t = pow(clamp(t, 0.0, 1.0), 0.8);
        
        // Sample from the palette texture (using 0.5 for y coordinate)
        FragColor = texture(paletteTexture, vec2(t, 0.5));
    }
}
"""

# State management
class AppState:
    def __init__(self):
        # View parameters (using Decimal for high precision)
        self.center_x = Decimal("-0.5")
        self.center_y = Decimal("0.0")
        self.zoom = Decimal("1.0")
        self.max_iter = 512
        
        # Texture parameters (using Decimal)
        self.tex_center_x = Decimal("-0.5")
        self.tex_center_y = Decimal("0.0")
        self.tex_zoom = Decimal("1.0")
        self.tex_width = WIDTH
        self.tex_height = HEIGHT
        
        # Visualization parameters
        self.min_val = 0.0
        self.max_val = 1.0
        
        # Flags
        self.needs_compute = True
        self.computing = False
        self.new_data_available = False
        self.new_data = None
        self.new_data_params = None
        
        self.lock = Lock()

state = AppState()

# C computation engine
class FastMandelbrotCompute:
    def __init__(self):
        dll_name = 'mandelbrot_compute.dll'
        script_dir = os.path.dirname(os.path.abspath(__file__))
        dll_path = os.path.join(script_dir, dll_name)
        
        try:
            self.lib = ctypes.CDLL(dll_path)
            
            # Define argument types for the string-based function
            self.lib.compute_mandelbrot_str.argtypes = [
                ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int,
                ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int,
                ctypes.c_int,
                ctypes.POINTER(ctypes.c_double)
            ]
            self.lib.compute_mandelbrot_str.restype = None
            
            # Helper to check precision mode
            self.lib.get_precision_mode.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]
            self.lib.get_precision_mode.restype = ctypes.c_int
            
            print("✓ C acceleration library loaded successfully!")
        except Exception as e:
            print(f"✗ Failed to load C library: {e}")
            sys.exit(1)
    
    def get_mode(self, xmin, xmax, width):
        xmin_str = str(xmin).encode('utf-8')
        xmax_str = str(xmax).encode('utf-8')
        return self.lib.get_precision_mode(xmin_str, xmax_str, width)

    def compute(self, xmin, xmax, width, ymin, ymax, height, max_iter):
        output = np.zeros(height * width, dtype=np.float64)
        
        # Convert Decimals to strings for C
        xmin_str = str(xmin).encode('utf-8')
        xmax_str = str(xmax).encode('utf-8')
        ymin_str = str(ymin).encode('utf-8')
        ymax_str = str(ymax).encode('utf-8')
        
        self.lib.compute_mandelbrot_str(
            xmin_str, xmax_str, width,
            ymin_str, ymax_str, height,
            max_iter,
            output.ctypes.data_as(ctypes.POINTER(ctypes.c_double))
        )
        return output.reshape(height, width)

compute_engine = FastMandelbrotCompute()

def create_palette_texture():
    """Create a 2D texture (Nx1) from the matplotlib palette"""
    colors = [
        '#000428', '#000764', '#0A1E5C', '#0C2C8A', '#1852B1', '#2B6FCC', '#397DD1', '#5092DD',
        '#6AA7E5', '#83B9E9', '#9BCBEB', '#B0D7EC', '#C4E1ED', '#D5EAF0', '#E3F0F3', '#F0F9FF',
        '#FFF8DC', '#FFEED5', '#FFE4B5', '#FFDAA0', '#FFD18A', '#FFC570', '#FFB347', '#FFA520',
        '#FF9000', '#FF7D00', '#FF6B00', '#FF5800', '#FF4500', '#FF3200', '#FF2400', '#F51C00',
        '#E60000', '#D80000', '#CC0000', '#B30000', '#990000', '#800020', '#660033', '#570040',
        '#4B0082', '#3D0066', '#2F004F', '#240040', '#1A0033', '#120022', '#0D001A', '#050008',
        '#000000'
    ]
    cmap = LinearSegmentedColormap.from_list('mandelbrot_ultra', colors, N=2048)
    gradient = np.linspace(0, 1, 2048)
    data = cmap(gradient).astype(np.float32) # RGBA float32
    
    texture = glGenTextures(1)
    glBindTexture(GL_TEXTURE_2D, texture)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
    # Create Nx1 2D texture
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, 2048, 1, 0, GL_RGBA, GL_FLOAT, data)
    return texture

def compute_thread_func():
    """Background thread for C computation"""
    while True:
        should_compute = False
        with state.lock:
            if state.needs_compute and not state.computing:
                state.computing = True
                state.needs_compute = False
                should_compute = True
                
                cx, cy = state.center_x, state.center_y
                zoom = state.zoom
                max_iter = state.max_iter
                width, height = WIDTH, HEIGHT
        
        if not should_compute:
            time.sleep(0.01)
            continue
            
        # Use Decimal for high precision bounds calculation
        aspect = Decimal(width) / Decimal(height)
        view_w = aspect / zoom
        view_h = Decimal("1.0") / zoom
        
        xmin = cx - view_w / Decimal("2.0")
        xmax = cx + view_w / Decimal("2.0")
        ymin = cy - view_h / Decimal("2.0")
        ymax = cy + view_h / Decimal("2.0")
        
        start_t = time.time()
        
        # Check precision mode
        mode = compute_engine.get_mode(xmin, xmax, width)
        mode_str = ["Double (64-bit)", "Long Double (80-bit)", "Quad (128-bit)", "Perturbation (Hybrid)"][mode]
        
        data = compute_engine.compute(xmin, xmax, width, ymin, ymax, height, max_iter)
        dt = time.time() - start_t
        
        # Calculate dynamic normalization stats
        valid_mask = data > 0
        if np.any(valid_mask):
            subset = data[valid_mask]
            if subset.size > 100000:
                subset = np.random.choice(subset, 100000)
            
            log_vals = np.log(np.log(subset + 2) + 1)
            min_v = np.min(log_vals)
            max_v = np.percentile(log_vals, 99.7)
            
            if max_v <= min_v:
                max_v = min_v + 1.0
        else:
            min_v, max_v = 0.0, 1.0
        
        with state.lock:
            state.new_data = data.astype(np.float32)
            state.new_data_params = (cx, cy, zoom, width, height, min_v, max_v)
            state.new_data_available = True
            state.computing = False
            
            if state.center_x != cx or state.center_y != cy or state.zoom != zoom:
                state.needs_compute = True
        
        print(f"Computed: {dt:.3f}s | Zoom: {zoom:.2e} | Iter: {max_iter} | Mode: {mode_str}")

def scroll_callback(window, xoffset, yoffset):
    with state.lock:
        window_x, window_y = glfw.get_cursor_pos(window)
        ndc_x = Decimal(window_x / WIDTH) * 2 - 1
        ndc_y = -(Decimal(window_y / HEIGHT) * 2 - 1)
        
        aspect = Decimal(WIDTH) / Decimal(HEIGHT)
        view_w = aspect / state.zoom
        view_h = Decimal("1.0") / state.zoom
        
        cursor_world_x = state.center_x + ndc_x * (view_w / 2)
        cursor_world_y = state.center_y + ndc_y * (view_h / 2)
        
        zoom_factor = Decimal("1.1") if yoffset > 0 else Decimal("1.0") / Decimal("1.1")
        state.zoom *= zoom_factor
        
        new_view_w = aspect / state.zoom
        new_view_h = Decimal("1.0") / state.zoom
        
        state.center_x = cursor_world_x - ndc_x * (new_view_w / 2)
        state.center_y = cursor_world_y - ndc_y * (new_view_h / 2)
        
        # Adaptive iterations
        zoom_float = float(state.zoom)
        if zoom_float < 10: state.max_iter = 512
        elif zoom_float < 100: state.max_iter = 1024
        elif zoom_float < 1000: state.max_iter = 2048
        elif zoom_float < 10000: state.max_iter = 4096
        elif zoom_float < 100000: state.max_iter = 8192
        elif zoom_float < 1e12: state.max_iter = 16384
        elif zoom_float < 1e15: state.max_iter = 32768
        elif zoom_float < 1e18: state.max_iter = 65536
        elif zoom_float < 1e21: state.max_iter = 131072
        elif zoom_float < 1e24: state.max_iter = 262144
        elif zoom_float < 1e27: state.max_iter = 524288
        elif zoom_float < 1e30: state.max_iter = 1048576
        else: state.max_iter = 2097152
        
        state.needs_compute = True

def mouse_button_callback(window, button, action, mods):
    if button == glfw.MOUSE_BUTTON_LEFT and action == glfw.PRESS:
        with state.lock:
            window_x, window_y = glfw.get_cursor_pos(window)
            ndc_x = Decimal(window_x / WIDTH) * 2 - 1
            ndc_y = -(Decimal(window_y / HEIGHT) * 2 - 1)
            
            aspect = Decimal(WIDTH) / Decimal(HEIGHT)
            view_w = aspect / state.zoom
            view_h = Decimal("1.0") / state.zoom
            
            state.center_x += ndc_x * (view_w / 2)
            state.center_y += ndc_y * (view_h / 2)
            state.needs_compute = True

def main():
    if not glfw.init():
        return
    
    glfw.window_hint(glfw.CONTEXT_VERSION_MAJOR, 3)
    glfw.window_hint(glfw.CONTEXT_VERSION_MINOR, 3)
    glfw.window_hint(glfw.OPENGL_PROFILE, glfw.OPENGL_CORE_PROFILE)
    
    window = glfw.create_window(WIDTH, HEIGHT, "Smooth Mandelbrot Explorer - High Quality", None, None)
    if not window:
        glfw.terminate()
        return
        
    glfw.make_context_current(window)
    glfw.set_scroll_callback(window, scroll_callback)
    glfw.set_mouse_button_callback(window, mouse_button_callback)
    glfw.swap_interval(1)
    
    try:
        program = compileProgram(
            compileShader(VERTEX_SHADER, GL_VERTEX_SHADER),
            compileShader(FRAGMENT_SHADER, GL_FRAGMENT_SHADER)
        )
    except Exception as e:
        print(e)
        return

    vertices = np.array([-1.0, -1.0, 1.0, -1.0, -1.0, 1.0, 1.0, 1.0], dtype=np.float32)
    vao = glGenVertexArrays(1)
    vbo = glGenBuffers(1)
    glBindVertexArray(vao)
    glBindBuffer(GL_ARRAY_BUFFER, vbo)
    glBufferData(GL_ARRAY_BUFFER, vertices.nbytes, vertices, GL_STATIC_DRAW)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, None)
    glEnableVertexAttribArray(0)
    
    # Create textures
    mandel_texture = glGenTextures(1)
    glBindTexture(GL_TEXTURE_2D, mandel_texture)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE)
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, WIDTH, HEIGHT, 0, GL_RED, GL_FLOAT, None)
    
    palette_texture = create_palette_texture()
    
    # Start compute thread
    t = Thread(target=compute_thread_func, daemon=True)
    t.start()
    
    # Uniform locations
    loc_rel_offset = glGetUniformLocation(program, "relative_offset")
    loc_rel_scale = glGetUniformLocation(program, "relative_scale")
    loc_mandel_tex = glGetUniformLocation(program, "mandelbrotTexture")
    loc_palette_tex = glGetUniformLocation(program, "paletteTexture")
    loc_min_val = glGetUniformLocation(program, "min_val")
    loc_max_val = glGetUniformLocation(program, "max_val")
    
    glUseProgram(program)
    glUniform1i(loc_mandel_tex, 0)
    glUniform1i(loc_palette_tex, 1)
    
    print("Controls:")
    print("  Scroll: Zoom")
    print("  Click: Center")
    
    try:
        while not glfw.window_should_close(window):
            with state.lock:
                if state.new_data_available:
                    glActiveTexture(GL_TEXTURE0)
                    glBindTexture(GL_TEXTURE_2D, mandel_texture)
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, WIDTH, HEIGHT, GL_RED, GL_FLOAT, state.new_data)
                    
                    state.tex_center_x, state.tex_center_y, state.tex_zoom, _, _, state.min_val, state.max_val = state.new_data_params
                    state.new_data_available = False
                
                # Calculate relative offset/scale using Decimal for precision, then convert to float for shader
                # Shader only needs relative values which are small, so float is fine here
                rel_scale = float(state.tex_zoom / state.zoom)
                
                aspect = Decimal(WIDTH) / Decimal(HEIGHT)
                tex_world_w = aspect / state.tex_zoom
                tex_world_h = Decimal("1.0") / state.tex_zoom
                
                off_x = float((state.center_x - state.tex_center_x) / tex_world_w)
                off_y = float((state.center_y - state.tex_center_y) / tex_world_h)
                
                current_min = state.min_val
                current_max = state.max_val
                
            glClear(GL_COLOR_BUFFER_BIT)
            glUseProgram(program)
            
            glUniform2f(loc_rel_offset, off_x, off_y)
            glUniform2f(loc_rel_scale, rel_scale, rel_scale)
            glUniform1f(loc_min_val, current_min)
            glUniform1f(loc_max_val, current_max)
            
            glActiveTexture(GL_TEXTURE0)
            glBindTexture(GL_TEXTURE_2D, mandel_texture)
            glActiveTexture(GL_TEXTURE1)
            glBindTexture(GL_TEXTURE_2D, palette_texture)
            
            glBindVertexArray(vao)
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4)
            
            glfw.swap_buffers(window)
            glfw.poll_events()
            
            if glfw.get_key(window, glfw.KEY_ESCAPE) == glfw.PRESS:
                glfw.set_window_should_close(window, True)
                
    except Exception as e:
        print(f"Error in main loop: {e}")
        import traceback
        traceback.print_exc()
        input("Press Enter to close...")

    glfw.terminate()

if __name__ == "__main__":
    main()
