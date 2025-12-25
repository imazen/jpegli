// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// C wrapper for butteraugli - enables FFI bindings for Rust testing

#ifndef LIB_EXTRAS_BUTTERAUGLI_C_H_
#define LIB_EXTRAS_BUTTERAUGLI_C_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Error codes
typedef enum {
  BUTTERAUGLI_OK = 0,
  BUTTERAUGLI_ERROR_MEMORY = 1,
  BUTTERAUGLI_ERROR_INVALID_INPUT = 2,
  BUTTERAUGLI_ERROR_INTERNAL = 3,
} butteraugli_error_t;

// Compute butteraugli score between two RGB images.
//
// Both images must be linear RGB (not sRGB) with values in [0, 1].
// Data layout: row-major, 3 channels interleaved (RGBRGBRGB...).
//
// Parameters:
//   rgb0, rgb1: Linear RGB image data, width * height * 3 floats each
//   width, height: Image dimensions
//   intensity_target: Nits corresponding to 1.0 (default 80.0)
//   out_score: Output butteraugli score (max of diffmap)
//
// Returns BUTTERAUGLI_OK on success.
butteraugli_error_t butteraugli_compare(
    const float* rgb0,
    const float* rgb1,
    size_t width,
    size_t height,
    float intensity_target,
    double* out_score);

// Compute butteraugli score with full parameters.
//
// Parameters:
//   rgb0, rgb1: Linear RGB image data
//   width, height: Image dimensions
//   hf_asymmetry: High-frequency asymmetry (default 1.0)
//   xmul: X channel multiplier (default 1.0)
//   intensity_target: Nits for 1.0 (default 80.0)
//   out_score: Output butteraugli score
//   out_diffmap: Optional output diffmap (width * height floats, or NULL)
//
// Returns BUTTERAUGLI_OK on success.
butteraugli_error_t butteraugli_compare_full(
    const float* rgb0,
    const float* rgb1,
    size_t width,
    size_t height,
    float hf_asymmetry,
    float xmul,
    float intensity_target,
    double* out_score,
    float* out_diffmap);

// Convert sRGB u8 to linear RGB for butteraugli input.
//
// Parameters:
//   srgb: Input sRGB data (width * height * 3 bytes)
//   width, height: Image dimensions
//   out_linear: Output linear RGB (width * height * 3 floats, pre-allocated)
void butteraugli_srgb_to_linear(
    const uint8_t* srgb,
    size_t width,
    size_t height,
    float* out_linear);

// Utility functions matching Rust API
double butteraugli_score_to_quality(double score);
double butteraugli_quality_to_score(double quality);

// Get intermediate values for debugging/validation
// These allow step-by-step comparison with Rust implementation

// Compute OpsinDynamicsImage (XYB conversion with blur-based sensitivity)
butteraugli_error_t butteraugli_opsin_dynamics(
    const float* linear_rgb,
    size_t width,
    size_t height,
    float intensity_target,
    float* out_xyb);  // width * height * 3 floats

// Compute Gamma function value (for testing FastLog2f)
float butteraugli_gamma(float v);

// Compute FastLog2f (for testing)
float butteraugli_fast_log2f(float v);

// NOTE: Internal functions like OpsinDynamicsImage and SeparateFrequencies
// use Highway SIMD namespacing and cannot be easily exposed via C API.
// Use butteraugli_compare_full with diffmap output for detailed analysis.

#ifdef __cplusplus
}
#endif

#endif  // LIB_EXTRAS_BUTTERAUGLI_C_H_
