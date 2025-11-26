#!/usr/bin/env pwsh
<#
.SYNOPSIS
    Build script for Mandelbrot computation engine
.DESCRIPTION
    Compiles the C source code with optimizations and places the DLL in the lib directory
#>

Write-Host "Building Mandelbrot Computation Engine..." -ForegroundColor Cyan

# Ensure lib directory exists
New-Item -ItemType Directory -Force -Path "lib" | Out-Null

# Compile with optimizations
$output = & gcc -shared -o lib/mandelbrot_compute.dll src/mandelbrot_compute.c `
    -O3 -fopenmp -march=native -mavx2 -mfma -lquadmath 2>&1

if ($LASTEXITCODE -eq 0) {
    Write-Host "✓ Build successful!" -ForegroundColor Green
    Write-Host "  Output: lib/mandelbrot_compute.dll" -ForegroundColor Gray
} else {
    Write-Host "✗ Build failed!" -ForegroundColor Red
    Write-Host $output -ForegroundColor Red
    exit 1
}

# Check file size
$dll = Get-Item "lib/mandelbrot_compute.dll"
Write-Host "  Size: $([math]::Round($dll.Length / 1KB, 2)) KB" -ForegroundColor Gray
