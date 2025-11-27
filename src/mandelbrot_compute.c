/*
 * Mandelbrot Computation Engine - High Precision (long double + __float128)
 * =========================================================================
 *
 * Uses double, long double (80-bit), or __float128 (128-bit) depending on zoom depth.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <quadmath.h>
#include <immintrin.h>

#ifdef _WIN32
    #define EXPORT __declspec(dllexport)
#else
    #define EXPORT
#endif

// Typedefs
typedef long double Real80;
typedef __float128 Real128;

// Helper for parsing
#define STRTOREAL80(s) strtold(s, NULL)
#define STRTOREAL128(s) strtoflt128(s, NULL)

static inline double mandelbrot_point_smooth_double(double cr, double ci, int max_iter) {
    double zr = 0.0;
    double zi = 0.0;
    double zr2 = 0.0;
    double zi2 = 0.0;
    
    double q = (cr - 0.25) * (cr - 0.25) + ci * ci;
    if (q * (q + (cr - 0.25)) < 0.25 * ci * ci) return -max_iter;
    
    int i = 0;
    const double escape = 256.0;
    const double log_2 = 0.6931471805599453;
    
    for (; i < max_iter; i++) {
        if (zr2 + zi2 > escape) {
            double modulus = zr2 + zi2;
            return i + 1.0 - log(log(modulus) / log_2) / log_2;
        }
        zi = 2.0 * zr * zi + ci;
        zr = zr2 - zi2 + cr;
        zr2 = zr * zr;
        zi2 = zi * zi;
    }
    return -max_iter;
}

static inline double mandelbrot_point_smooth_long(Real80 cr, Real80 ci, int max_iter) {
    Real80 zr = 0.0;
    Real80 zi = 0.0;
    Real80 zr2 = 0.0;
    Real80 zi2 = 0.0;
    
    double cr_d = (double)cr;
    double ci_d = (double)ci;
    double q = (cr_d - 0.25) * (cr_d - 0.25) + ci_d * ci_d;
    if (q * (q + (cr_d - 0.25)) < 0.25 * ci_d * ci_d) return -max_iter;
    
    int i = 0;
    const Real80 escape = 256.0;
    const double log_2 = 0.6931471805599453;
    
    for (; i < max_iter; i++) {
        if (zr2 + zi2 > escape) {
            double modulus = (double)(zr2 + zi2);
            return i + 1.0 - log(log(modulus) / log_2) / log_2;
        }
        zi = 2.0 * zr * zi + ci;
        zr = zr2 - zi2 + cr;
        zr2 = zr * zr;
        zi2 = zi * zi;
    }
    return -max_iter;
}

static inline double mandelbrot_point_smooth_quad(Real128 cr, Real128 ci, int max_iter) {
    Real128 zr = 0.0Q;
    Real128 zi = 0.0Q;
    Real128 zr2 = 0.0Q;
    Real128 zi2 = 0.0Q;
    
    double cr_d = (double)cr;
    double ci_d = (double)ci;
    double q = (cr_d - 0.25) * (cr_d - 0.25) + ci_d * ci_d;
    if (q * (q + (cr_d - 0.25)) < 0.25 * ci_d * ci_d) return -max_iter;
    
    int i = 0;
    const Real128 escape = 256.0Q;
    const double log_2 = 0.6931471805599453;
    
    for (; i < max_iter; i++) {
        if (zr2 + zi2 > escape) {
            double modulus = (double)(zr2 + zi2);
            return i + 1.0 - log(log(modulus) / log_2) / log_2;
        }
        zi = 2.0Q * zr * zi + ci;
        zr = zr2 - zi2 + cr;
        zr2 = zr * zr;
        zi2 = zi * zi;
    }
    return -max_iter;
}

EXPORT int get_precision_mode(const char* xmin_str, const char* xmax_str, int width) {
    Real128 xmin = STRTOREAL128(xmin_str);
    Real128 xmax = STRTOREAL128(xmax_str);
    Real128 w = xmax - xmin;
    
    if (w > 1.0e-13Q) return 0; // double
    if (w > 1.0e-18Q) return 1; // long double (extended range)
    return 3; // Perturbation (Quad reference + Double delta)
}

// Perturbation theory implementation
static void compute_mandelbrot_perturbation(
    Real128 center_r, Real128 center_i,
    Real128 dx, Real128 dy,
    int width, int height, int max_iter,
    double* output
) {
    // 1. Compute reference orbit
    // We allocate on heap to avoid stack overflow with large max_iter
    // Using aligned memory for better cache performance
    Real128* refs_r = (Real128*)_mm_malloc(sizeof(Real128) * (max_iter + 1), 64);
    Real128* refs_i = (Real128*)_mm_malloc(sizeof(Real128) * (max_iter + 1), 64);
    
    if (!refs_r || !refs_i) {
        if (refs_r) _mm_free(refs_r);
        if (refs_i) _mm_free(refs_i);
        return; // Allocation failed
    }
    
    // Pre-allocate double arrays to avoid repeated casts in inner loop
    double* refs_r_d = (double*)_mm_malloc(sizeof(double) * (max_iter + 1), 64);
    double* refs_i_d = (double*)_mm_malloc(sizeof(double) * (max_iter + 1), 64);
    
    if (!refs_r_d || !refs_i_d) {
        _mm_free(refs_r);
        _mm_free(refs_i);
        if (refs_r_d) _mm_free(refs_r_d);
        if (refs_i_d) _mm_free(refs_i_d);
        return; // Allocation failed
    }

    Real128 zr = 0.0Q;
    Real128 zi = 0.0Q;
    Real128 zr2 = 0.0Q;
    Real128 zi2 = 0.0Q;
    
    int ref_iter = max_iter;
    
    for (int i = 0; i < max_iter; i++) {
        refs_r[i] = zr;
        refs_i[i] = zi;
        // Pre-cast to double to avoid repeated conversions in inner loop
        refs_r_d[i] = (double)zr;
        refs_i_d[i] = (double)zi;
        
        if (zr2 + zi2 > 4.0Q) {
            ref_iter = i;
            break;
        }
        
        zi = 2.0Q * zr * zi + center_i;
        zr = zr2 - zi2 + center_r;
        zr2 = zr * zr;
        zi2 = zi * zi;
    }

    // 1.5 Compute Linear Approximation (Series Approximation) skipping
    // We want to find how many iterations we can skip using dz_n = B_n * dc
    // B_{n+1} = 2*Z_n*B_n + 1, B_0 = 0
    
    int skip_iter = 0;
    double Br = 0.0;
    double Bi = 0.0;
    
    // Calculate max_dc (radius of view)
    double max_dc_sq = (double)(dx*dx*width*width/4.0Q + dy*dy*height*height/4.0Q);
    double max_dc = sqrt(max_dc_sq);
    
    // Threshold for approximation validity
    // We want |B_n * dc| < threshold
    // If it grows too large, the z^2 term in perturbation becomes significant
    // Use very conservative threshold to preserve detail at deep zooms
    const double approx_threshold = 1.0e-12; 
    
    for (int i = 0; i < ref_iter; i++) {
        // Check magnitude
        double B_mag = sqrt(Br*Br + Bi*Bi);
        if (B_mag * max_dc > approx_threshold) {
            break;
        }
        
        skip_iter = i;
        
        // Update B_{n+1} = 2*Z_n*B_n + 1
        double Zr = (double)refs_r[i];
        double Zi = (double)refs_i[i];
        
        // 2*(Zr + iZi)*(Br + iBi) + 1
        // 2*(ZrBr - ZiBi + i(ZrBi + ZiBr)) + 1
        double next_Br = 2.0 * (Zr * Br - Zi * Bi) + 1.0;
        double next_Bi = 2.0 * (Zr * Bi + Zi * Br);
        
        Br = next_Br;
        Bi = next_Bi;
    }
    
    // Don't skip too much if it's short
    if (skip_iter > ref_iter) skip_iter = ref_iter;
    
    // 2. Parallel perturbation loop
    double dx_d = (double)dx;
    double dy_d = (double)dy;
    
    // Hoist SIMD constants outside loop to avoid recomputation
    const __m256d const_two = _mm256_set1_pd(2.0);
    const __m256d const_four = _mm256_set1_pd(4.0);
    const __m256i const_one_i = _mm256_set1_epi64x(1);
    const __m256i const_four_i = _mm256_set1_epi64x(4);
    
    #ifdef _OPENMP
    #pragma omp parallel for schedule(guided)
    #endif
    for (int py = 0; py < height; py++) {
        // Process 4 pixels at a time using AVX2
        int px = 0;
        for (; px <= width - 4; px += 4) {
            // Delta c for 4 pixels
            double dcr0 = (px + 0 - width / 2.0) * dx_d;
            double dcr1 = (px + 1 - width / 2.0) * dx_d;
            double dcr2 = (px + 2 - width / 2.0) * dx_d;
            double dcr3 = (px + 3 - width / 2.0) * dx_d;
            
            double dci_val = (py - height / 2.0) * dy_d;
            
            __m256d vdcr = _mm256_set_pd(dcr3, dcr2, dcr1, dcr0);
            __m256d vdci = _mm256_set1_pd(dci_val);
            
            // Initialize dz using Linear Approximation
            // dz = B * dc
            // dzr = Br*dcr - Bi*dci
            // dzi = Br*dci + Bi*dcr
            __m256d vBr = _mm256_set1_pd(Br);
            __m256d vBi = _mm256_set1_pd(Bi);
            
            __m256d vdzr, vdzi;
            
            if (skip_iter > 0) {
                vdzr = _mm256_sub_pd(
                    _mm256_mul_pd(vBr, vdcr),
                    _mm256_mul_pd(vBi, vdci)
                );
                vdzi = _mm256_add_pd(
                    _mm256_mul_pd(vBr, vdci),
                    _mm256_mul_pd(vBi, vdcr)
                );
            } else {
                vdzr = _mm256_setzero_pd();
                vdzi = _mm256_setzero_pd();
            }
            
            __m256d vdzr2 = _mm256_mul_pd(vdzr, vdzr);
            __m256d vdzi2 = _mm256_mul_pd(vdzi, vdzi);
            
            // Mask for active pixels (all start active)
            __m256i vmask = _mm256_set1_epi64x(-1);
            
            // Store iteration counts (start at skip_iter)
            __m256i viter = _mm256_set1_epi64x(skip_iter);
            
            // Store final modulus for smoothing
            __m256d vmodulus = _mm256_setzero_pd();
            
            int limit = ref_iter;
            int all_escaped = 0;
            
            // Main loop - Unrolled by 4
            int i = skip_iter;
            for (; i < limit; i+=4) {
                // Check if we can do a full block of 4
                if (i + 4 > limit) {
                    // Handle remaining iterations one by one
                    break;
                }

                // --- Iteration 0 ---
                {
                    double X = refs_r_d[i];
                    double Y = refs_i_d[i];
                    __m256d vX = _mm256_set1_pd(X);
                    __m256d vY = _mm256_set1_pd(Y);
                    
                    // Perturbation: dz = 2*Z*dz + dz^2 + dc
                    // Use FMA: 2*X*dzr - 2*Y*dzi + (dzr^2 - dzi^2 + dcr)
                    
                    __m256d vtwoX = _mm256_mul_pd(const_two, vX);
                    __m256d vtwoY = _mm256_mul_pd(const_two, vY);
                    
                    __m256d term_sq_r = _mm256_add_pd(_mm256_sub_pd(vdzr2, vdzi2), vdcr);
                    __m256d term_sq_i = _mm256_add_pd(_mm256_mul_pd(const_two, _mm256_mul_pd(vdzr, vdzi)), vdci);
                    
                    // next_dzr = 2*X*dzr - 2*Y*dzi + term_sq_r
                    // = fma(2*X, dzr, term_sq_r - 2*Y*dzi)
                    __m256d next_dzr = _mm256_fmadd_pd(vtwoX, vdzr, _mm256_fnmadd_pd(vtwoY, vdzi, term_sq_r));
                    
                    // next_dzi = 2*X*dzi + 2*Y*dzr + term_sq_i
                    __m256d next_dzi = _mm256_fmadd_pd(vtwoX, vdzi, _mm256_fmadd_pd(vtwoY, vdzr, term_sq_i));
                    
                    vdzr = next_dzr;
                    vdzi = next_dzi;
                    vdzr2 = _mm256_mul_pd(vdzr, vdzr);
                    vdzi2 = _mm256_mul_pd(vdzi, vdzi);
                }
                
                // --- Iteration 1 ---
                {
                    double X = refs_r_d[i+1];
                    double Y = refs_i_d[i+1];
                    __m256d vX = _mm256_set1_pd(X);
                    __m256d vY = _mm256_set1_pd(Y);
                    
                    __m256d vtwoX = _mm256_mul_pd(const_two, vX);
                    __m256d vtwoY = _mm256_mul_pd(const_two, vY);
                    
                    __m256d term_sq_r = _mm256_add_pd(_mm256_sub_pd(vdzr2, vdzi2), vdcr);
                    __m256d term_sq_i = _mm256_add_pd(_mm256_mul_pd(const_two, _mm256_mul_pd(vdzr, vdzi)), vdci);
                    
                    __m256d next_dzr = _mm256_fmadd_pd(vtwoX, vdzr, _mm256_fnmadd_pd(vtwoY, vdzi, term_sq_r));
                    __m256d next_dzi = _mm256_fmadd_pd(vtwoX, vdzi, _mm256_fmadd_pd(vtwoY, vdzr, term_sq_i));
                    
                    vdzr = next_dzr;
                    vdzi = next_dzi;
                    vdzr2 = _mm256_mul_pd(vdzr, vdzr);
                    vdzi2 = _mm256_mul_pd(vdzi, vdzi);
                }

                // --- Iteration 2 ---
                {
                    double X = refs_r_d[i+2];
                    double Y = refs_i_d[i+2];
                    __m256d vX = _mm256_set1_pd(X);
                    __m256d vY = _mm256_set1_pd(Y);
                    
                    __m256d vtwoX = _mm256_mul_pd(const_two, vX);
                    __m256d vtwoY = _mm256_mul_pd(const_two, vY);
                    
                    __m256d term_sq_r = _mm256_add_pd(_mm256_sub_pd(vdzr2, vdzi2), vdcr);
                    __m256d term_sq_i = _mm256_add_pd(_mm256_mul_pd(const_two, _mm256_mul_pd(vdzr, vdzi)), vdci);
                    
                    __m256d next_dzr = _mm256_fmadd_pd(vtwoX, vdzr, _mm256_fnmadd_pd(vtwoY, vdzi, term_sq_r));
                    __m256d next_dzi = _mm256_fmadd_pd(vtwoX, vdzi, _mm256_fmadd_pd(vtwoY, vdzr, term_sq_i));
                    
                    vdzr = next_dzr;
                    vdzi = next_dzi;
                    vdzr2 = _mm256_mul_pd(vdzr, vdzr);
                    vdzi2 = _mm256_mul_pd(vdzi, vdzi);
                }

                // --- Iteration 3 ---
                {
                    double X = refs_r_d[i+3];
                    double Y = refs_i_d[i+3];
                    __m256d vX = _mm256_set1_pd(X);
                    __m256d vY = _mm256_set1_pd(Y);
                    
                    __m256d vtwoX = _mm256_mul_pd(const_two, vX);
                    __m256d vtwoY = _mm256_mul_pd(const_two, vY);
                    
                    __m256d term_sq_r = _mm256_add_pd(_mm256_sub_pd(vdzr2, vdzi2), vdcr);
                    __m256d term_sq_i = _mm256_add_pd(_mm256_mul_pd(const_two, _mm256_mul_pd(vdzr, vdzi)), vdci);
                    
                    __m256d next_dzr = _mm256_fmadd_pd(vtwoX, vdzr, _mm256_fnmadd_pd(vtwoY, vdzi, term_sq_r));
                    __m256d next_dzi = _mm256_fmadd_pd(vtwoX, vdzi, _mm256_fmadd_pd(vtwoY, vdzr, term_sq_i));
                    
                    vdzr = next_dzr;
                    vdzi = next_dzi;
                    vdzr2 = _mm256_mul_pd(vdzr, vdzr);
                    vdzi2 = _mm256_mul_pd(vdzi, vdzi);
                }
                
                // --- Check Escape (Once every 4 iterations) ---
                // After 4 iterations, we're now at iteration i+4, so check against that reference
                // But clamp to avoid accessing beyond ref_iter
                int check_idx = (i + 4 < ref_iter) ? i + 4 : ref_iter - 1;
                double X = refs_r_d[check_idx];
                double Y = refs_i_d[check_idx];
                __m256d vX = _mm256_set1_pd(X);
                __m256d vY = _mm256_set1_pd(Y);
                
                __m256d vZ_plus_dz_r = _mm256_add_pd(vX, vdzr);
                __m256d vZ_plus_dz_i = _mm256_add_pd(vY, vdzi);
                
                __m256d vmod = _mm256_add_pd(
                    _mm256_mul_pd(vZ_plus_dz_r, vZ_plus_dz_r),
                    _mm256_mul_pd(vZ_plus_dz_i, vZ_plus_dz_i)
                );
                __m256d vcmp = _mm256_cmp_pd(vmod, const_four, _CMP_GT_OQ);
                __m256i vcmp_i = _mm256_castpd_si256(vcmp);
                
                // For pixels that just escaped, store their iteration count and modulus
                // vcmp_i has -1 for escaped pixels
                // We want to update viter and vmodulus ONLY for newly escaped pixels
                // Newly escaped = (vmask is active) AND (vcmp_i shows escaped)
                __m256i newly_escaped = _mm256_and_si256(vmask, vcmp_i);
                
                // For newly escaped pixels, set iteration to i+4
                __m256i viter_escaped = _mm256_set1_epi64x(i + 4);
                // Update viter: if newly escaped, use i+4, else keep old value
                viter = _mm256_blendv_epi8(viter, viter_escaped, newly_escaped);
                
                // Store modulus for newly escaped pixels
                vmodulus = _mm256_blendv_pd(vmodulus, vmod, _mm256_castsi256_pd(newly_escaped));
                
                // Update mask: pixels that haven't escaped yet remain active
                // vmask = vmask & ~vcmp_i
                vmask = _mm256_andnot_si256(vcmp_i, vmask);
                
                if (_mm256_testz_si256(vmask, vmask)) {
                    all_escaped = 1;
                    break;
                }
                
                // Zero out inactive pixels to prevent explosion
                vdzr = _mm256_and_pd(_mm256_castsi256_pd(vmask), vdzr);
                vdzi = _mm256_and_pd(_mm256_castsi256_pd(vmask), vdzi);
                vdzr2 = _mm256_mul_pd(vdzr, vdzr);
                vdzi2 = _mm256_mul_pd(vdzi, vdzi);
            }
            
            // Finish remaining iterations (if any, or if we broke early but not all escaped?)
            // If we broke because all_escaped, we are done.
            // If we finished loop, we might have 1-3 iters left.
            if (!all_escaped) {
                for (; i < limit; i++) {
                    double X = refs_r_d[i];
                    double Y = refs_i_d[i];
                    __m256d vX = _mm256_set1_pd(X);
                    __m256d vY = _mm256_set1_pd(Y);
                    
                    __m256d vZ_plus_dz_r = _mm256_add_pd(vX, vdzr);
                    __m256d vZ_plus_dz_i = _mm256_add_pd(vY, vdzi);
                    
                    __m256d vmod = _mm256_add_pd(
                        _mm256_mul_pd(vZ_plus_dz_r, vZ_plus_dz_r),
                        _mm256_mul_pd(vZ_plus_dz_i, vZ_plus_dz_i)
                    );
                    
                    __m256d vcmp = _mm256_cmp_pd(vmod, const_four, _CMP_GT_OQ);
                    __m256i vcmp_i = _mm256_castpd_si256(vcmp);
                    
                    // For newly escaped pixels, record iteration and modulus
                    __m256i newly_escaped = _mm256_and_si256(vmask, vcmp_i);
                    __m256i viter_escaped = _mm256_set1_epi64x(i);
                    viter = _mm256_blendv_epi8(viter, viter_escaped, newly_escaped);
                    vmodulus = _mm256_blendv_pd(vmodulus, vmod, _mm256_castsi256_pd(newly_escaped));
                    
                    // Update mask
                    vmask = _mm256_andnot_si256(vcmp_i, vmask);
                    
                    if (_mm256_testz_si256(vmask, vmask)) break;
                    
                    // Perturbation
                    __m256d vtwoX = _mm256_mul_pd(const_two, vX);
                    __m256d vtwoY = _mm256_mul_pd(const_two, vY);
                    
                    __m256d term_sq_r = _mm256_add_pd(_mm256_sub_pd(vdzr2, vdzi2), vdcr);
                    __m256d term_sq_i = _mm256_add_pd(_mm256_mul_pd(const_two, _mm256_mul_pd(vdzr, vdzi)), vdci);
                    
                    __m256d next_dzr = _mm256_fmadd_pd(vtwoX, vdzr, _mm256_fnmadd_pd(vtwoY, vdzi, term_sq_r));
                    __m256d next_dzi = _mm256_fmadd_pd(vtwoX, vdzi, _mm256_fmadd_pd(vtwoY, vdzr, term_sq_i));
                    
                    vdzr = _mm256_and_pd(_mm256_castsi256_pd(vmask), next_dzr);
                    vdzi = _mm256_and_pd(_mm256_castsi256_pd(vmask), next_dzi);
                    vdzr2 = _mm256_mul_pd(vdzr, vdzr);
                    vdzi2 = _mm256_mul_pd(vdzi, vdzi);
                }
            }
            
            // Extract results
            long long iters[4];
            double mods[4];
            _mm256_storeu_si256((__m256i*)iters, viter);
            _mm256_storeu_pd(mods, vmodulus);
            
            for (int k = 0; k < 4; k++) {
                if (iters[k] < limit) {
                    // Escaped
                    double modulus = mods[k];
                    output[py * width + (px + k)] = iters[k] + 1.0 - log(log(modulus) / 0.69314718056) / 0.69314718056;
                } else {
                    // Did not escape
                    output[py * width + (px + k)] = -max_iter;
                }
            }
        }
        
        // Handle remaining pixels
        for (; px < width; px++) {
            // Delta c
            double dcr = (px - width / 2.0) * dx_d;
            double dci = (py - height / 2.0) * dy_d;
            
            double dzr, dzi;
            
            // Init with BLA
            if (skip_iter > 0) {
                dzr = Br * dcr - Bi * dci;
                dzi = Br * dci + Bi * dcr;
            } else {
                dzr = 0.0;
                dzi = 0.0;
            }
            
            double dzr2 = dzr * dzr;
            double dzi2 = dzi * dzi;
            
            int escaped = 0;
            int limit = ref_iter;
            
            for (int i = skip_iter; i < limit; i++) {
                double X = refs_r_d[i];
                double Y = refs_i_d[i];
                
                double Z_plus_dz_r = X + dzr;
                double Z_plus_dz_i = Y + dzi;
                double modulus = Z_plus_dz_r*Z_plus_dz_r + Z_plus_dz_i*Z_plus_dz_i;
                
                if (modulus > 4.0) {
                    output[py * width + px] = i + 1.0 - log(log(modulus) / 0.69314718056) / 0.69314718056;
                    escaped = 1;
                    break;
                }
                
                double two_X = 2.0 * X;
                double two_Y = 2.0 * Y;
                
                double next_dzr = (two_X * dzr - two_Y * dzi) + dzr2 - dzi2 + dcr;
                double next_dzi = (two_X * dzi + two_Y * dzr) + 2.0 * dzr * dzi + dci;
                
                dzr = next_dzr;
                dzi = next_dzi;
                dzr2 = dzr * dzr;
                dzi2 = dzi * dzi;
            }
            
            if (!escaped) {
                output[py * width + px] = -max_iter;
            }
        }
    }
    
    _mm_free(refs_r);
    _mm_free(refs_i);
    _mm_free(refs_r_d);
    _mm_free(refs_i_d);
}

EXPORT void compute_mandelbrot_str(
    const char* xmin_str, const char* xmax_str, int width,
    const char* ymin_str, const char* ymax_str, int height,
    int max_iter,
    double* output
) {
    // Parse as 128-bit first to check width
    Real128 xmin_q = STRTOREAL128(xmin_str);
    Real128 xmax_q = STRTOREAL128(xmax_str);
    Real128 ymin_q = STRTOREAL128(ymin_str);
    Real128 ymax_q = STRTOREAL128(ymax_str);
    
    Real128 w_real = xmax_q - xmin_q;
    
    // Thresholds:
    // double: > 1e-13
    // long double: > 1e-17 (Extended range for 80-bit)
    // Perturbation: <= 1e-17
    
    if (w_real > 1.0e-13Q) {
        // Double precision
        double xmin_d = (double)xmin_q;
        double ymin_d = (double)ymin_q;
        double dx_d = (double)((xmax_q - xmin_q) / width);
        double dy_d = (double)((ymax_q - ymin_q) / height);
        
        #ifdef _OPENMP
        #pragma omp parallel for schedule(guided) collapse(2)
        #endif
        for (int py = 0; py < height; py++) {
            for (int px = 0; px < width; px++) {
                double cr = xmin_d + dx_d * px;
                double ci = ymin_d + dy_d * py;
                output[py * width + px] = mandelbrot_point_smooth_double(cr, ci, max_iter);
            }
        }
    } else if (w_real > 1.0e-17Q) {
        // Long double precision (80-bit)
        Real80 xmin_l = (Real80)xmin_q;
        Real80 ymin_l = (Real80)ymin_q;
        Real80 dx_l = (Real80)((xmax_q - xmin_q) / width);
        Real80 dy_l = (Real80)((ymax_q - ymin_q) / height);

        #ifdef _OPENMP
        #pragma omp parallel for schedule(guided) collapse(2)
        #endif
        for (int py = 0; py < height; py++) {
            for (int px = 0; px < width; px++) {
                Real80 cr = xmin_l + dx_l * px;
                Real80 ci = ymin_l + dy_l * py;
                output[py * width + px] = mandelbrot_point_smooth_long(cr, ci, max_iter);
            }
        }
    } else {
        // Perturbation Theory (Hybrid Quad/Double)
        Real128 center_r = (xmin_q + xmax_q) / 2.0Q;
        Real128 center_i = (ymin_q + ymax_q) / 2.0Q;
        Real128 dx_q = (xmax_q - xmin_q) / width;
        Real128 dy_q = (ymax_q - ymin_q) / height;
        
        compute_mandelbrot_perturbation(center_r, center_i, dx_q, dy_q, width, height, max_iter, output);
    }
}

// Keep the old function for backward compatibility
EXPORT void compute_mandelbrot(
    double xmin, double xmax, int width,
    double ymin, double ymax, int height,
    int max_iter,
    double* output
) {
    double dx = (xmax - xmin) / width;
    double dy = (ymax - ymin) / height;

    #ifdef _OPENMP
    #pragma omp parallel for schedule(guided) collapse(2)
    #endif
    for (int py = 0; py < height; py++) {
        for (int px = 0; px < width; px++) {
            double cr = xmin + dx * px;
            double ci = ymin + dy * py;
            output[py * width + px] = mandelbrot_point_smooth_double(cr, ci, max_iter);
        }
    }
}
