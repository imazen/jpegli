// Copyright (c) the JPEG XL Project Authors.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

// C FFI for jpegli internals - enables Rust parity testing

#ifndef LIB_EXTRAS_JPEGLI_TEST_FFI_H_
#define LIB_EXTRAS_JPEGLI_TEST_FFI_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Error codes
typedef enum {
  JPEGLI_FFI_OK = 0,
  JPEGLI_FFI_ERROR_MEMORY = 1,
  JPEGLI_FFI_ERROR_INVALID_INPUT = 2,
  JPEGLI_FFI_ERROR_INTERNAL = 3,
} jpegli_ffi_error_t;

// ============================================================================
// XYB Color Conversion
// ============================================================================

// Convert sRGB u8 to linear RGB float [0, 1]
void jpegli_srgb_to_linear(
    const uint8_t* srgb,  // width * height * 3 bytes
    size_t width,
    size_t height,
    float* out_linear);   // width * height * 3 floats

// Convert linear RGB to XYB (unscaled, as used in butteraugli)
// intensity_target: typically 255.0 for standard images
void jpegli_linear_to_xyb(
    const float* linear_rgb,  // width * height * 3 floats
    size_t width,
    size_t height,
    float intensity_target,
    float* out_xyb);          // width * height * 3 floats (X, Y, B interleaved)

// Convert XYB to scaled XYB [0, 1] range (as stored in JPEG)
void jpegli_scale_xyb(
    float* xyb,  // width * height * 3 floats, modified in place
    size_t width,
    size_t height);

// All-in-one: sRGB u8 -> scaled XYB
// This is what jpegli uses for encoding
void jpegli_srgb_to_scaled_xyb(
    const uint8_t* srgb,      // width * height * 3 bytes
    size_t width,
    size_t height,
    float intensity_target,
    float* out_scaled_xyb);   // width * height * 3 floats

// Get XYB constants for verification
void jpegli_get_xyb_constants(
    float* opsin_matrix,      // 3x3 = 9 floats (row-major)
    float* opsin_bias,        // 3 floats
    float* scaled_xyb_offset, // 3 floats
    float* scaled_xyb_scale); // 3 floats

// ============================================================================
// Transfer Functions (PQ, HLG, sRGB)
// ============================================================================

// PQ (Perceptual Quantizer) for HDR
float jpegli_pq_eotf(float encoded);           // Display from encoded
float jpegli_pq_inv_eotf(float display);       // Encoded from display

// HLG (Hybrid Log-Gamma) for HDR
float jpegli_hlg_eotf(float encoded);          // Display from encoded
float jpegli_hlg_inv_eotf(float display);      // Encoded from display

// sRGB gamma
float jpegli_srgb_to_linear_single(float srgb);
float jpegli_linear_to_srgb_single(float linear);

// ============================================================================
// Tone Mapping
// ============================================================================

// Rec2408 tone mapping (HDR to SDR)
// source/target_nits: intensity targets
// luminances: 3 floats for R, G, B luminance weights
// rgb: input/output RGB values (modified in place)
void jpegli_rec2408_tone_map(
    float source_nits,
    float target_nits,
    const float* luminances,  // 3 floats
    float* rgb);              // 3 floats, modified in place

// HLG OOTF (Opto-Optical Transfer Function)
// source/target_nits: intensity targets
// luminances: 3 floats for R, G, B luminance weights
// rgb: input/output RGB values (modified in place)
void jpegli_hlg_ootf(
    float source_nits,
    float target_nits,
    const float* luminances,  // 3 floats
    float* rgb);              // 3 floats, modified in place

// Gamut mapping (clamp out-of-gamut colors)
// preserve_saturation: 0.0-1.0
// luminances: 3 floats for R, G, B luminance weights
// rgb: input/output RGB values (modified in place)
void jpegli_gamut_map(
    float preserve_saturation,
    const float* luminances,  // 3 floats
    float* rgb);              // 3 floats, modified in place

// ============================================================================
// Fast Math Functions (for AQ parity testing)
// ============================================================================

// Fast log2 approximation (L1 error ~3.9E-6)
// Uses bit manipulation + rational polynomial, NOT std::log2
float jpegli_fast_log2f(float x);

// Fast pow2 approximation (max relative error ~3e-7)
// Uses bit manipulation + rational polynomial, NOT std::exp2
float jpegli_fast_pow2f(float x);

// Fast power: base^exponent using fast_log2f * fast_pow2f
float jpegli_fast_powf(float base, float exponent);

// ComputeMask from adaptive_quantization.cc - perceptual masking curve
float jpegli_compute_mask(float out_val);

// MaskingSqrt from adaptive_quantization.cc
float jpegli_masking_sqrt(float v);

// RatioOfDerivativesOfCubicRootToSimpleGamma
float jpegli_ratio_of_derivatives(float v, int invert);

// ============================================================================
// DCT Functions (for coefficient parity testing)
// ============================================================================

// Forward 8x8 DCT (scalar reference implementation)
// Input: 64 floats (row-major), level-shifted (pixels - 128)
// Output: 64 floats (row-major DCT coefficients), scaled by 1/8
void jpegli_forward_dct_8x8(const float* input, float* output);

#ifdef __cplusplus
}
#endif

#endif  // LIB_EXTRAS_JPEGLI_TEST_FFI_H_
